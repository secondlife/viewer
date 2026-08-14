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

#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llsingleton.h"
#include "llmutex.h"

class LLEmbeddedBrowser;
class LLEmbeddedBrowserTab;
class LLSubscriber;
class LLProcess;

// Mirrors a subset of cefshm_demo::Opcode's kEvent* commands -- see
// cefshm_protocol.h. Deliberately not the full LLPluginClassMediaOwner::
// EMediaEvent set; just enough for load-state and title/location/cursor
// feedback (see LLViewerMediaImpl, which translates these into that enum).
enum class LLEmbeddedBrowserEventType
{
    LoadStart,
    LoadEnd,          // mValue = HTTP status code
    TitleChanged,     // mText = new title
    AddressChanged,   // mText = new URL
    CursorChanged,    // mValue = an llCefCursorType value, opaque here
    ClickLinkHref,    // mText = url, mTarget = target frame/window name -- a link wants to
                      // open in a new window/tab (target="_blank", window.open(), etc.)
    ClickLinkNoFollow, // mText = url, mUserGesture/mIsRedirect describe how navigation to
                       // this recognized custom URL scheme (e.g. "secondlife://") was triggered
    FileDialogRequest, // mDialogId = an opaque id to echo back to respondToFileDialog(); mValue =
                       // an llCefFileDialogMode ordinal (Open=0, OpenMultiple=1, OpenFolder=2,
                       // Save=3); mText = the dialog's suggested/default file path
    StatusTextChanged,     // mText = new status-bar text (e.g. a hovered link's URL)
    ConsoleMessage,        // mText = console.log/warn/error message, mTarget = its source URL,
                           // mValue = line number
    ProducerDisconnected,  // the shm connection to cefshm_producer was lost -- fires exactly once
                           // per outage (edge-triggered on the connected->disconnected transition),
                           // never once per retry, so it's safe to notify the user from this
    ProducerReconnected    // the connection came back after a ProducerDisconnected -- also
                           // edge-triggered, fires exactly once per recovery
};

struct LLEmbeddedBrowserEvent
{
    LLEmbeddedBrowserEventType type;
    std::string mText;
    std::string mTarget;
    unsigned int mValue = 0;
    bool mUserGesture = false;
    bool mIsRedirect = false;
    long long mDialogId = 0;
};

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
        // Fire-and-forget, matching LLPluginClassMedia::executeJavaScript() -- no result is
        // returned to the caller.
        void executeJavaScript(const std::string& code);
        unsigned int getWidth() const;
        unsigned int getHeight() const;

        // Input: fire-and-forget sends over the per-view command channel, same as
        // navigate()/resize() -- silently do nothing if not yet connected (no queueing;
        // an input event that arrives before the first frame has nothing useful to hit
        // anyway). x/y are canvas-space pixels, matching this tab's current width/height.
        void mouseMove(int x, int y);
        // button matches LLViewerMediaImpl's own convention (0 = left, the only value any
        // current caller passes); is_down false = button-up.
        void mouseButton(int x, int y, unsigned char button, bool is_down);
        void scrollWheel(int x, int y, int deltaY);
        // msg/wParam/lParam: a raw Win32 keyboard message triple, straight from
        // LLWindowWin32::getNativeKeyData(). Windows-only, matching the producer's own
        // SendKeyEvent.
        void keyEvent(unsigned int msg, unsigned int wParam, unsigned int lParam);
        // Drives CEF's own caret blink and focus/blur page JS -- call with true when the
        // LLMediaCtrl hosting this tab gains keyboard focus, false when it loses it.
        void setFocus(bool focus);
        // Completes a pending FileDialogRequest event -- dialogId must be the value from
        // that event's mDialogId; pass an empty filePaths to indicate the user canceled.
        void respondToFileDialog(long long dialogId, const std::vector<std::string>& filePaths);

        // Pops the oldest queued event (received from the producer since the last call),
        // false if none are pending. Call in a loop to drain all of them -- unlike
        // getPixels()/getWidth() this is NOT a snapshot of current state, it's a FIFO of
        // discrete occurrences (e.g. a load-start followed by a load-end in the same tick
        // both need to surface, not collapse into "latest state").
        bool popEvent(LLEmbeddedBrowserEvent& out_event);

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
        // Set when a ProducerDisconnected event is pushed, cleared (and a matching
        // ProducerReconnected pushed) the next time connectToProducer() succeeds -- makes both
        // events edge-triggered on the connected/disconnected transition rather than firing on
        // every poll/retry.
        bool mHadDisconnected = false;
        unsigned char* mPixels = nullptr;
        unsigned int mWidth = 0;
        unsigned int mHeight = 0;
        const unsigned int mDepth = 4;
        std::string mCurrentUrl;
        std::deque<LLEmbeddedBrowserEvent> mEvents;
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
        void executeJavaScript(unsigned int id, const std::string& code);

        void mouseMove(unsigned int id, int x, int y);
        void mouseButton(unsigned int id, int x, int y, unsigned char button, bool is_down);
        void scrollWheel(unsigned int id, int x, int y, int deltaY);
        void keyEvent(unsigned int id, unsigned int msg, unsigned int wParam, unsigned int lParam);
        void setFocus(unsigned int id, bool focus);
        void respondToFileDialog(unsigned int id, long long dialogId, const std::vector<std::string>& filePaths);
        bool popEvent(unsigned int id, LLEmbeddedBrowserEvent& out_event);

        // Caps requested create() dimensions -- callers (e.g. newview, which knows about
        // EmbeddedBrowserMaxWidth/Height in settings.xml) should call this once before
        // creating tabs. Defaults to 4096x4096 if never called.
        void setMaxDimensions(unsigned int max_width, unsigned int max_height);

        // The llshmframe library this Viewer was built against -- a build-time constant,
        // always available regardless of whether any producer is connected.
        static std::string getShmFrameVersion();

        // The llCefBrowser/CEF/Chromium version block the most recently connected
        // cefshm_producer reported over the wire (see kEventVersionInfo in
        // cefshm_protocol.h) -- multi-line, matching how the legacy Dullahan-based
        // plugin's own LIBCEF_VERSION is formatted. Empty until at least one tab has
        // completed its connection handshake.
        std::string getCefBrowserVersion() const;

        // Called by LLEmbeddedBrowserTab on receiving kEventVersionInfo -- not meant for
        // other callers.
        void setCefBrowserVersion(const std::string& version);

        // Called by LLEmbeddedBrowserTab::connectToProducer() on its one failure branch
        // that means "no producer process reachable at all" (as opposed to one that's
        // merely busy/racing another consumer, where relaunching would just kill a
        // perfectly healthy producer). A no-op if SLCefProducer is already running, if
        // UseEmbeddedBrowser is off, or if a relaunch was already attempted recently or
        // too many times this session -- see the constants in llembeddedbrowser.cpp.
        void maybeRelaunchProducer();

        // Called by LLEmbeddedBrowserTab::connectToProducer() on every successful
        // connect -- a real connection means whatever relaunch attempts led to it (if
        // any) worked, so a later, unrelated crash should get its own fresh budget
        // rather than inheriting an already-exhausted one.
        void resetRelaunchAttempts();

    private:
        // Launches SLCefProducer via LLProcess, storing the result in mProducerProcess.
        // Returns false (leaving mProducerProcess untouched) if SLCefProducer isn't
        // available on this platform (see LLDir::getSLCefProducerLauncher()) or the
        // launch itself failed. Caller must hold mProducerMutex.
        bool launchProducer();

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

        mutable LLMutex mCefVersionMutex;
        std::string mCefBrowserVersion;

        // Guards mProducerProcess and the relaunch bookkeeping below -- touched from
        // init()/reset() on the main thread and from maybeRelaunchProducer()/
        // resetRelaunchAttempts() on any tab's own background update thread.
        mutable LLMutex mProducerMutex;
        std::shared_ptr<LLProcess> mProducerProcess;
        int mProducerRelaunchAttempts = 0;
        std::chrono::steady_clock::time_point mLastRelaunchAttempt;
};
