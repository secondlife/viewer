/**
 * @file llcompositor.cpp
 * @brief LLCompositor implementation.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llcompositor.h"

#include "lltestsquarecompositable.h"
#include "lltimer.h"
#include "llrendertarget.h"
#include "llimagegl.h"
#include "llgl.h"
#include "llglstates.h"
#include "llglslshader.h"
#include "llwindow.h"

#include <algorithm>
#include <chrono>

LLCompositor::LLCompositor() = default;

LLCompositor::~LLCompositor()
{
    release();
}

void LLCompositor::attachToWindow(LLWindow* window, U32 width, U32 height)
{
    mWindow = window;
    mSwapChain.attachToWindow(window, width, height);
}

void LLCompositor::createRefreshOverlay()
{
    // Runs on our own thread - the squares each spin up a producer
    // thread with its own shared GL context, which needs our context
    // current (it is, inside presentFrame).
    if (mRefreshShown || !mWindow)
    {
        return;
    }
    mTestSquares.emplace_back(std::make_unique<LLTestSquareCompositable>(
        "Blue Square", 0, 64, 255, 1u, 40, 40, mWindow));
    mTestSquares.emplace_back(std::make_unique<LLTestSquareCompositable>(
        "Orange Square", 255, 128, 0, 2u, 180, 40, mWindow));
    mTestSquares.emplace_back(std::make_unique<LLTestSquareCompositable>(
        "Red Square", 255, 0, 0, 3u, 320, 40, mWindow));
    mTestSquares.emplace_back(std::make_unique<LLTestSquareCompositable>(
        "Purple Square", 160, 0, 255, 4u, 460, 40, mWindow));
    for (auto& square : mTestSquares)
    {
        square->connect(*this);
        addCompositable(square.get());
    }
    mRefreshShown = true;
}

void LLCompositor::destroyRefreshOverlay()
{
    if (!mRefreshShown)
    {
        return;
    }
    // Drop them from the draw list before tearing down so the present
    // loop never touches a destroyed square.
    for (auto& square : mTestSquares)
    {
        removeCompositable(square.get());
        square->disconnect();
    }
    mTestSquares.clear();
    mRefreshShown = false;
}

void LLCompositor::resize(U32 width, U32 height)
{
    mSwapChain.resize(width, height);
}

void LLCompositor::release()
{
    destroyRefreshOverlay();
    mCompositables.clear();
    mBlitShader = nullptr; // owned by LLViewerShaderMgr
    mSwapChain.release();
    // The window is destroyed right after us; drop our pointer and any
    // queued work so a late present/toggle can't touch it.
    mWindow = nullptr;
    mPendingSwapInterval.store(-1, std::memory_order_relaxed);
    mPendingShowRefresh.store(-1, std::memory_order_relaxed);
}

void LLCompositor::addCompositable(LLCompositable* c)
{
    llassert(c != nullptr);
    // The stats overlay iterates this list from the viewer thread.
    std::lock_guard<std::mutex> lock(mCompositablesMutex);
    mCompositables.push_back(c);
}

void LLCompositor::removeCompositable(LLCompositable* c)
{
    std::lock_guard<std::mutex> lock(mCompositablesMutex);
    mCompositables.erase(
        std::remove(mCompositables.begin(), mCompositables.end(), c),
        mCompositables.end());
}

void LLCompositor::getLayerStats(std::vector<LayerStatsSnapshot>& out) const
{
    out.clear();
    std::lock_guard<std::mutex> lock(mCompositablesMutex);
    for (LLCompositable* c : mCompositables)
    {
        const F32 frame_ms = c->lastFrameMs();
        out.push_back({c->compositableName(),
                       frame_ms,
                       frame_ms > 0.f ? 1000.f / frame_ms : 0.f});
    }
}


void LLCompositor::presentFrame()
{
    LL_PROFILE_ZONE_SCOPED;

    // Once shutdown is requested we stop presenting so producers waiting
    // on a present unblock and the viewer thread can be joined.
    if (mShutdownRequested.load(std::memory_order_relaxed))
    {
        return;
    }

    // The RT stack should be empty when we get here.
    llassert(LLRenderTarget::getCurrentBoundTarget() == nullptr);

    // Apply a pending swap interval on our own context.
    const S32 pending_interval = mPendingSwapInterval.exchange(-1, std::memory_order_relaxed);
    if (pending_interval >= 0 && mWindow)
    {
        mWindow->setSwapInterval(pending_interval);
    }

    // Apply a pending refresh-overlay toggle here, where our GL context
    // is current for the squares' context creation/teardown.
    const S32 pending_show = mPendingShowRefresh.exchange(-1, std::memory_order_relaxed);
    if (pending_show == 1)
    {
        createRefreshOverlay();
    }
    else if (pending_show == 0)
    {
        destroyRefreshOverlay();
    }

    // Draw every layer's front texture as a quad straight into the default
    // framebuffer, bottom first. Binding the texture for sampling is also
    // what pulls the producer context's writes into this one.

    const GLint dst_w = (GLint)mSwapChain.getWidth();
    const GLint dst_h = (GLint)mSwapChain.getHeight();

    const F64 present_start = LLTimer::getTotalSeconds();

    // Acquire the present surface through the swap-chain seam (GL: FBO 0, full
    // window) and composite the layers straight into it.
    mSwapChain.acquireNextImage();
    mSwapChain.bindForRender();

    {
        // Hold the shader handoff lock across the composite. A shader reload
        // on the viewer thread clears mBlitShader, re-links the program, and
        // sets it again - the lock makes that wait out an in-flight composite
        // instead of deleting the program we're drawing with.
        std::lock_guard<std::mutex> shader_lock(mBlitShaderMutex);

        // Not drawable: no compiled shader yet (startup, or mid-reload), or a
        // degenerate swap chain (the NDC math below divides by these). We
        // still run the loop either way - the mailbox drain must not stop.
        const bool can_draw = mBlitShader && mBlitShader->mProgramObject
                              && dst_w > 0 && dst_h > 0;

        // Composite state, scoped and cache-aware so this thread's GL state
        // mirrors stay truthful - on single-thread fallbacks the world render
        // shares this context and trusts those caches.
        LLGLDisable blend(GL_BLEND);
        LLGLDisable scissor(GL_SCISSOR_TEST);
        LLGLDepthTest depth(GL_FALSE, GL_FALSE);
        gGL.setColorMask(true, true);

        if (can_draw)
        {
            mBlitShader->bind();
        }

        // Snapshot the layer list under its lock so a concurrent add/remove
        // can't invalidate our iterator mid-present. We drop the lock before
        // touching any layer - the readiness gate below covers per-layer
        // lifetime.
        std::vector<LLCompositable*> layers;
        {
            std::lock_guard<std::mutex> lock(mCompositablesMutex);
            layers = mCompositables;
        }

        for (LLCompositable* c : layers)
        {
            // The producer must have finished allocating its front buffer and
            // not be tearing it down: otherwise frontBuffer()'s RT is
            // mid-mutation on the producer thread and its mTex is unsafe to
            // read.
            if (!c->isReady())
            {
                continue;
            }

            // Mailbox claim for queue-paced layers. This runs even on
            // presents that can't draw - the claim is what unblocks the
            // producer's publish wait, and skipping it would starve the
            // world thread behind a shader reload or a degenerate window.
            c->tryAcquireNewFront();

            if (!can_draw)
            {
                continue;
            }

            LLRenderTarget& front = c->frontBuffer();

            // Don't hold an outer lease while calling accessors that take
            // their own - leases don't nest. Each call below takes its own;
            // only the texture guard's lease spans the draw.
            if (!front.isComplete())
            {
                continue; // layer not allocated yet (early init)
            }

            S32 dst_x = 0, dst_y = 0;
            c->compositeOffset(dst_x, dst_y);

            // The cross-context primitive is the color texture (LLImageGL), not
            // the FBO-centric RT. Sample it directly: the guard holds its shared
            // lease across the draw and its fence orders the GPU. Single-context
            // layers (no attachment image) fall back to the raw RT name (no
            // lease needed).
            LLImageGL* sync = front.getColorAttachmentImage();
            LLScopedTexName src_tex_guard;
            U32 src_tex;
            if (sync)
            {
                src_tex_guard = sync->getTexName();
                src_tex = src_tex_guard.get();
            }
            else
            {
                src_tex = front.getTexture(0);
            }

            if (src_tex == 0)
            {
                // A layer that reached ready with no usable texture name.
                // Never bind name 0 and draw whatever's resident there.
                llassert(false);
                continue;
            }

            // Read the layer size under the texture guard's shared lease -
            // resize() publishes the new size under the matching unique lease,
            // so size and storage stay paired.
            const GLint w = (GLint)front.getWidth();
            const GLint h = (GLint)front.getHeight();

            // Wait on the producer's fence so we only sample finished pixels.
            if (sync)
            {
                sync->waitFrameCompleteFence();
            }

            // Layer rect in NDC; GL origin bottom-left matches the
            // compositeOffset convention.
            const F32 x0 = 2.f * (F32)dst_x / (F32)dst_w - 1.f;
            const F32 y0 = 2.f * (F32)dst_y / (F32)dst_h - 1.f;
            const F32 x1 = 2.f * (F32)(dst_x + w) / (F32)dst_w - 1.f;
            const F32 y1 = 2.f * (F32)(dst_y + h) / (F32)dst_h - 1.f;

            // Bind and draw the layer quad.
            static const LLStaticHashedString sBlitRect("blit_rect");
            gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, src_tex);
            mBlitShader->uniform4f(sBlitRect, x0, y0, x1, y1);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

            // Reverse fence: the producer waits on this before writing into
            // the buffer again.
            if (sync)
            {
                sync->placeReadCompleteFence();
            }
        }

        if (can_draw)
        {
            mBlitShader->unbind();
        }
    }

    // Layers composited into the present surface; flush any batched GL and
    // present (the old img.flush() path flushed gGL before swapBuffers).
    gGL.flush();
    mSwapChain.present();

    const F64 present_end = LLTimer::getTotalSeconds();
    mLastPresentMs.store(
        (F32)((present_end - present_start) * 1000.0),
        std::memory_order_relaxed);

    // Present rate over a 1s window.
    ++mPresentCount;
    if (mPresentWindowStart == 0.0)
    {
        mPresentWindowStart = present_end;
        mPresentCount = 0;
    }
    else if (present_end - mPresentWindowStart >= 1.0)
    {
        mPresentFps.store((F32)(mPresentCount / (present_end - mPresentWindowStart)),
                          std::memory_order_relaxed);
        mPresentCount = 0;
        mPresentWindowStart = present_end;
    }

    // Publish the present so producers parked in waitForPresent can pace
    // themselves to the vblank clock. LIVENESS INVARIANT: this must bump on
    // EVERY present - including when we re-present an unchanged frame
    // (tryAcquireNewFront returned false) - because the viewer thread makes
    // progress only when this index advances. Never gate present() on having
    // new output, or the vsync gate deadlocks.
    {
        std::lock_guard<std::mutex> lk(mSyncMutex);
        ++mPresentIndex;
    }
    mSyncCV.notify_all();
}

U64 LLCompositor::waitForPresent(U64 target, const std::function<bool()>& should_stop)
{
    std::unique_lock<std::mutex> lk(mSyncMutex);
    auto ready = [&]{
        return mSyncInterrupted
            || (should_stop && should_stop())
            || mPresentIndex >= target;
    };
    // wait_for (not wait) so an external should_stop set without a notify is
    // still picked up promptly - the present notify handles the common case.
    while (!ready())
    {
        mSyncCV.wait_for(lk, std::chrono::milliseconds(100));
    }
    return mPresentIndex; // < target means interrupted; the caller should stop
}

void LLCompositor::interruptSync()
{
    {
        std::lock_guard<std::mutex> lk(mSyncMutex);
        mSyncInterrupted = true;
    }
    mSyncCV.notify_all();
}

void LLCompositor::wakeSyncWaiters()
{
    mSyncCV.notify_all();
}
