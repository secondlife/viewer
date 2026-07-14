/**
 * @file llcompositor.h
 * @brief Compositor that owns the swap chain and presents an ordered list
 *        of LLCompositables.
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

#ifndef LL_LLCOMPOSITOR_H
#define LL_LLCOMPOSITOR_H

#include "llswapchain.h"
#include "llcompositable.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class LLWindow;
class LLTestSquareCompositable;
class LLGLSLShader;

// Owns the swap chain and an ordered list of LLCompositables. Each frame we
// draw every layer's front buffer into the window and present.
//
// Runs on the OS main thread (driven from LLAppViewer::doFrame) while the
// viewer renders on its own thread. Compositables never block our presents.
class LLCompositor
{
public:
    // Defined out of line - the unique_ptr member needs the complete type.
    LLCompositor();
    ~LLCompositor();

    LLCompositor(const LLCompositor&)            = delete;
    LLCompositor& operator=(const LLCompositor&) = delete;

    // Stand up the swap chain. Called once GL is up.
    void attachToWindow(LLWindow* window, U32 width, U32 height);

    // Propagate window resize to the swap chain.
    void resize(U32 width, U32 height);

    // Tear everything down. Called before window destruction.
    void release();

    bool isInitialized() const { return mSwapChain.isAttached(); }

    // Add a compositable. Lower-index layers draw first (bottom). Order is
    // fixed at registration; no re-ordering yet.
    void addCompositable(LLCompositable* c);

    // Remove a previously added compositable.
    void removeCompositable(LLCompositable* c);

    // Show/hide the bring-up refresh overlay (the colored sync-rate
    // squares). Callable from any thread - the squares own GL contexts,
    // so they're created/destroyed on our own thread at the next
    // present.
    void setShowRefreshOverlay(bool show) { mPendingShowRefresh.store(show ? 1 : 0, std::memory_order_relaxed); }

    // Composite and present one frame: draw every layer's front buffer in
    // registration order (bottom first), present, fire the sync signal.
    // We never wait on producers - a layer with no new content just gets
    // its last front re-drawn.
    void presentFrame();

    // Signal shutdown so producers stop waiting on us. Once set, the
    // compositor stops presenting and any producer parked waiting for a
    // present (e.g. the mailbox back-pressure spins) bails out, so the
    // viewer thread can reach its quit check and the join completes.
    // Callable from any thread; checked via isShutdownRequested().
    void requestShutdown() { mShutdownRequested.store(true, std::memory_order_relaxed); }
    bool isShutdownRequested() const { return mShutdownRequested.load(std::memory_order_relaxed); }

    // Present every Nth vblank (1 = every vblank). The driver does the
    // pacing and the sync signal inherits the divided cadence. Callable
    // from any thread - applied on our own context at the next present.
    void setSwapInterval(S32 interval) { mPendingSwapInterval.store(interval, std::memory_order_relaxed); }

    // Producers pace themselves to the present (vblank) clock by waiting on a
    // published monotonic present index. waitForPresent blocks until the index
    // reaches `target` (or the compositor shuts down / should_stop fires) and
    // returns the current index; a value < target means it was interrupted.
    // Pace to every Nth present by waiting for index = last + N.
    U64 waitForPresent(U64 target, const std::function<bool()>& should_stop = {});

    // Permanently release every producer parked in waitForPresent (full
    // compositor/viewer shutdown). Sticky: subsequent waits return at once.
    void interruptSync();

    // Wake producers parked in waitForPresent so they re-check their own
    // should_stop - for a producer disconnecting while the compositor keeps
    // running (e.g. a debug test square toggled off). Non-sticky.
    void wakeSyncWaiters();

    // Read access for resize handling and similar plumbing. Presenting
    // stays in here.
    const LLSwapChain& getSwapChain() const { return mSwapChain; }

    // The blit shader we composite with (compositorblitV/F.glsl, owned
    // and compiled by LLViewerShaderMgr). Until it's set and compiled
    // we present without drawing any layers. Callable from any thread:
    // the shader manager clears this before re-linking the program and
    // sets it again after, and the lock makes that handoff wait out any
    // in-flight composite instead of deleting the program under it.
    void setBlitShader(LLGLSLShader* shader)
    {
        std::lock_guard<std::mutex> lock(mBlitShaderMutex);
        mBlitShader = shader;
    }

    // Debug overlay stats (Show Render Info). Per-layer numbers come
    // straight from the producer's own metrics - we do no bookkeeping
    // on this side.
    struct LayerStatsSnapshot
    {
        std::string name;
        F32 frameMs;      // producer's last full frame cycle, ms
        F32 fps;          // produced frames per second (1000/frameMs)
    };

    // Wall-clock CPU time of the most recent presentFrame, ms.
    F32 getLastPresentMs() const { return mLastPresentMs.load(std::memory_order_relaxed); }

    // Presents per second over a 1s window.
    F32 getPresentFps() const { return mPresentFps.load(std::memory_order_relaxed); }

    // Snapshot of per-layer stats in composition order.
    void getLayerStats(std::vector<LayerStatsSnapshot>& out) const;

private:
    LLSwapChain                   mSwapChain;
    LLWindow*                     mWindow = nullptr;
    std::vector<LLCompositable*>  mCompositables;
    std::mutex                    mSyncMutex;
    std::condition_variable       mSyncCV;
    U64                           mPresentIndex = 0;     // published present (vblank) count
    bool                          mSyncInterrupted = false; // sticky full-shutdown release
    std::atomic<S32>              mPendingSwapInterval{-1}; // applied at next present; -1 = none
    std::atomic<bool>            mShutdownRequested{false}; // set on shutdown to unblock producers

    // Bring-up refresh overlay: colored squares that repaint at
    // different sync intervals, behind RenderCompositorShowRefresh.
    // Created/torn down on our own thread (they own GL contexts) when
    // the pending flag flips.
    void createRefreshOverlay();
    void destroyRefreshOverlay();
    std::vector<std::unique_ptr<LLTestSquareCompositable>> mTestSquares;
    std::atomic<S32> mPendingShowRefresh{-1}; // applied at next present; -1 = no change
    bool             mRefreshShown = false;

    // Guards the list structure only - registration vs the stats overlay's
    // iteration on the viewer thread. The present loop runs on the same
    // thread as all mutations, so it reads lock-free.
    mutable std::mutex      mCompositablesMutex;
    std::atomic<F32>        mLastPresentMs{0.f};
    std::atomic<F32>        mPresentFps{0.f};
    U32                     mPresentCount = 0;        // compositor-thread-only
    F64                     mPresentWindowStart = 0.0; // compositor-thread-only

    // Set by the viewer once LLViewerShaderMgr has compiled it. Guarded by
    // mBlitShaderMutex: presentFrame holds the lock across the composite so
    // a shader reload on the viewer thread can't delete the program while
    // we're drawing with it.
    LLGLSLShader* mBlitShader = nullptr;
    std::mutex    mBlitShaderMutex;
};

#endif // LL_LLCOMPOSITOR_H
