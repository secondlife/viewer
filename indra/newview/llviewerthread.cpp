/**
 * @file llviewerthread.cpp
 * @brief LLViewerThread implementation.
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

#include "llviewerprecompiledheaders.h"

#include "llviewerthread.h"

#include "llappviewer.h"
#include "llrender.h"
#include "llviewerwindow.h"
#include "llwindow.h"

#if LL_DARWIN
#include "llwindowmacosx-objc.h"
#endif

LLViewerThread::LLViewerThread(LLWindow* window)
:   LLThread("Viewer"),
    mWindow(window),
    mContext(nullptr)
{
    llassert(mWindow != nullptr);
    // The shared context has to be created on the OS main thread,
    // which is where this constructor runs.
    mContext = mWindow->createSharedContext();
    if (!mContext)
    {
        // An llassert alone compiles out in Release, and running without a
        // context doesn't fail loudly on its own: wglMakeCurrent(hdc, NULL)
        // succeeds, so the whole render thread would just no-op its GL with
        // no diagnostic pointing here. Die with the cause named instead.
        LL_ERRS() << "Failed to create the viewer render context, verify graphics driver installed and current." << LL_ENDL;
    }
}

LLViewerThread::~LLViewerThread()
{
    // run() normally destroys the context before the thread exits. If
    // we still hold it here, the thread never started - drop it now.
    if (mContext)
    {
        mWindow->destroySharedContext(mContext);
        mContext = nullptr;
    }
}

void LLViewerThread::run()
{
    // Anchor the viewer thread identity before any render code asks
    // about it.
    set_viewer_thread();

    // Make our shared GL context current. Everything we render from
    // here on targets this context.
    mWindow->makeContextCurrent(mContext);

    // gGL is thread_local, so this thread needs its own init. Pass
    // true so we can build vertex buffers here too.
    gGL.init(true);

    // Allocate our render targets now that our context is current.
    // FBOs aren't shared between contexts, but textures are, which is
    // how the compositor gets at our color buffers.
    if (gViewerWindow)
    {
        const U32 w = gViewerWindow->getWindowWidthRaw();
        const U32 h = gViewerWindow->getWindowHeightRaw();
        LLAppViewer::instance()->allocateRenderTargets(w, h);
    }

    LL_INFOS("Viewer") << "Viewer thread running, context current." << LL_ENDL;

    // The frame loop. Each tick runs input, idle, and the render, then
    // waits on the compositor's present. On shutdown the compositor
    // unblocks us and we fall out of the loop.
    while (!isQuitting())
    {
#if LL_DARWIN
        // This thread has no Cocoa run loop; drain an autorelease pool
        // per tick so transient ObjC objects don't leak until exit.
        ll_macos_run_in_autorelease_pool([]()
        {
            LLAppViewer::instance()->viewerThreadTick();
        });
#else
        LLAppViewer::instance()->viewerThreadTick();
#endif
    }

    LL_INFOS("Viewer") << "Viewer thread exiting, destroying context."
                       << LL_ENDL;

    // The old main loop's exit tail (final snapshot, voice, service
    // pump, watchdog), run while the world and our GL context are still
    // intact - cleanup is blocked joining us.
#if LL_DARWIN
    ll_macos_run_in_autorelease_pool([]()
    {
        LLAppViewer::instance()->viewerThreadShutdown();
    });
#else
    LLAppViewer::instance()->viewerThreadShutdown();
#endif

    // Tear down per-thread gGL before destroying the context.
    gGL.shutdown();

    // The context has to be destroyed on the thread that created it.
    mWindow->destroySharedContext(mContext);
    mContext = nullptr;
}
