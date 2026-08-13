/**
 * @file llembeddedbrowser.h
 * @brief Definition of LLEmbeddedBrowser class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llsingleton.h"
#include "llmutex.h"

class LLEmbeddedBrowser;
class LLEmbeddedBrowserTab;
class LLSubscriber;

class LLEmbeddedBrowserUpdateThread :
    public LLThread {
    public:
        LLEmbeddedBrowserUpdateThread(LLEmbeddedBrowser* browser, unsigned int id)
            : LLThread("EmbeddedBrowserUpdate"),
              mBrowser(browser),
              mBrowserId(id)
        {}

        void run() override;

    private:
        LLEmbeddedBrowser* mBrowser;
        unsigned int mBrowserId;
};

// All state for a single browser instance (one per floater or prim face
// showing media). Each tab drives its own update thread and pixel buffer
// so that N tabs don't serialize on each other.
class LLEmbeddedBrowserTab
{
    public:
        LLEmbeddedBrowserTab(LLEmbeddedBrowser* browser, unsigned int id, const std::string& url, unsigned int width, unsigned int height);
        ~LLEmbeddedBrowserTab();

        void update();
        const unsigned char* getPixels();
        // Atomically snapshots the current pixel buffer together with the width/height
        // it was produced at, all under one lock -- unlike getPixels()/getWidth()/
        // getHeight() called separately, the result can't end up mismatched by a
        // concurrent resize(), and the caller owns an independent copy that stays
        // valid even if this tab is resized or destroyed immediately afterward (e.g.
        // when handed off to an async GL upload on another thread).
        bool copyPixels(std::vector<unsigned char>& out_pixels, unsigned int& out_width, unsigned int& out_height);
        void navigate(const std::string& url);
        void resize(unsigned int width, unsigned int height);
        unsigned int getWidth() const;
        unsigned int getHeight() const;

    private:
        // Best-effort: claims the cefshm_producer control channel, requests a view,
        // and stores the resulting per-view LLSubscriber in mSub plus sends the tab's
        // current URL as that view's initial navigation. Blocks the calling thread (the
        // tab's own update thread, never called concurrently with itself) for up to a
        // few seconds while it retries the control-channel handshake, matching
        // llcefshm-example's own CefShmConsumer::connectToProducer(). Returns false if
        // no producer is reachable right now; update() just retries on a later tick.
        bool connectToProducer();

        mutable LLMutex mPixelMutex;
        std::unique_ptr<LLEmbeddedBrowserUpdateThread> mUpdateThread;
        std::unique_ptr<LLSubscriber> mSub;
        unsigned char* mPixels = nullptr;
        unsigned int mWidth = 0;
        unsigned int mHeight = 0;
        const unsigned int mDepth = 4;
        std::string mCurrentUrl;
};

class LLEmbeddedBrowser : public LLSingleton<LLEmbeddedBrowser> {
        LLSINGLETON(LLEmbeddedBrowser);

    public:
        ~LLEmbeddedBrowser();

        void init();
        void reset();

        unsigned int create(const std::string& url, unsigned int width, unsigned int height);
        void destroy(unsigned int id);
        void update(unsigned int id);
        void updateAll();
        const unsigned char* getPixels(unsigned int id);
        bool copyPixels(unsigned int id, std::vector<unsigned char>& out_pixels, unsigned int& out_width, unsigned int& out_height);
        unsigned int getWidth(unsigned int id);
        unsigned int getHeight(unsigned int id);
        void navigate(unsigned int id, const std::string& url);
        void resize(unsigned int id, unsigned int width, unsigned int height);

        // Caps requested create() dimensions -- callers (e.g. newview, which knows about
        // EmbeddedBrowserMaxWidth/Height in settings.xml) should call this once before
        // creating tabs. Defaults to 4096x4096 if never called.
        void setMaxDimensions(unsigned int max_width, unsigned int max_height);

    private:
        // Looks up a tab under mTabsMutex and returns a shared_ptr copy rather than a
        // reference into the map, so callers can safely call (potentially slow) methods
        // on the returned tab with mTabsMutex already released -- a concurrent destroy()
        // erasing the map entry only drops the map's reference; the tab object itself
        // stays alive until the caller's shared_ptr copy also goes out of scope. This
        // keeps mTabsMutex held only for a brief map lookup, never for the duration of a
        // tab's own (per-tab-mutex-protected) work, which is the whole point of giving
        // each tab its own thread and buffer.
        std::shared_ptr<LLEmbeddedBrowserTab> findTab(unsigned int id);

        mutable LLMutex mTabsMutex;
        std::map<unsigned int, std::shared_ptr<LLEmbeddedBrowserTab>> mTabs;
        unsigned int mNextTabId = 0;
        unsigned int mMaxWidth = 4096;
        unsigned int mMaxHeight = 4096;
};
