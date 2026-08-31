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

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
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

        // Diagnostic only (added 2026-08-19 to prove or disprove whether shutdown()'s
        // caller can ever conclude this thread is done while run()'s loop body is
        // still actually executing on the detached OS thread -- LLThread::start()
        // calls mThreadp->detach() immediately, so there is no real join() anywhere
        // in this lifecycle; isStopped() becoming true is the *only* signal the
        // destroying thread ever gets). true for the entire duration of each loop
        // iteration's real work, false only while sleeping between iterations or
        // after the loop has actually exited. Check this immediately after
        // shutdown() returns, before actually freeing anything -- remove once the
        // actual root cause of the LLEmbeddedBrowser::findTab() crash is confirmed.
        bool isInsideRunLoop() const { return mInsideRunLoop.load(); }

    private:
        LLEmbeddedBrowser* mBrowser;
        unsigned int mBrowserId;
        std::atomic<bool> mInsideRunLoop{false};
};

// All state for a single browser instance (one per floater or prim face
// showing media). Each tab drives its own update thread and pixel buffer
// so that N tabs don't serialize on each other.
class LLEmbeddedBrowserTab
{
    public:
        // isUI: true for 2D floater/UI media, false for in-world/prim media -- selects
        // which of the producer's two cookie-store contexts this tab's browser is
        // created in (see kRequestSlot's own comment in cefshm_protocol.h). maxWidth/
        // maxHeight are LLEmbeddedBrowser's own current ceiling (EmbeddedBrowserMaxWidth/
        // Height), sent to the producer in kRequestSlot so it can size this slot's own
        // shared-memory segment to it instead of always reserving the producer's
        // absolute maximum -- see that opcode's own comment in cefshm_protocol.h.
        LLEmbeddedBrowserTab(LLEmbeddedBrowser* browser, unsigned int id, const std::string& url, unsigned int width, unsigned int height, bool isUI, unsigned int maxWidth, unsigned int maxHeight);
        ~LLEmbeddedBrowserTab();

        // Stops this tab's own update thread, blocking until it has genuinely exited
        // run() -- safe to call more than once (a no-op if already stopped). Exists
        // as its own method, called explicitly by LLEmbeddedBrowser::destroy() before
        // its own shared_ptr copy can possibly go out of scope, because relying solely
        // on the destructor's own call to this isn't enough: a concurrent findTab()
        // on this tab's own update thread can end up holding the very last reference,
        // meaning the destructor -- and this same shutdown -- could otherwise run *on
        // that update thread itself*, which then blocks waiting for itself to stop.
        // Calling this first, from the thread that's actually dropping the map's own
        // reference, guarantees the update thread has already exited by the time any
        // shared_ptr copy's destructor can run, regardless of which one ends up being
        // last.
        void stopUpdateThread();

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
        // Back/forward/stop, straight into llCefBrowserManager::GoBack()/GoForward()/
        // StopLoad(). Fire-and-forget, same as cut()/copy()/paste().
        void goBack();
        void goForward();
        void stopLoad();
        // Straight into llCefBrowserManager::Reload(). ignoreCache mirrors the legacy
        // plugin's own browse_reload(bool) -- see LLViewerMediaImpl::navigateReload().
        void reload(bool ignoreCache);
        // Cached from the producer's own kEventNavStateChanged (sent alongside every
        // load-start/load-end -- see cefshm_protocol.h), not a live round-trip query --
        // this is safe to call every frame (e.g. to enable/disable a back/forward
        // button), the same way LLPluginClassMedia::getHistoryBackAvailable() is.
        bool canGoBack() const;
        bool canGoForward() const;
        void resize(unsigned int width, unsigned int height);
        // Zooms the page's rendered content (CSS-pixel-level, like a browser's Ctrl+/Ctrl-)
        // without touching the pixel buffer size -- see LLViewerMediaImpl::setPageZoomFactor().
        void setPageZoom(float zoom);
        // Fire-and-forget, matching LLPluginClassMedia::executeJavaScript() -- no result is
        // returned to the caller.
        void executeJavaScript(const std::string& code);
        // Cut/copy/paste the current selection/clipboard, straight into
        // llCefBrowserManager::Cut()/Copy()/Paste(). Fire-and-forget, same as
        // executeJavaScript() -- no completion is reported.
        void cut();
        void copy();
        void paste();
        unsigned int getWidth() const;
        unsigned int getHeight() const;

        // Input: fire-and-forget sends over the per-view command channel, same as
        // navigate()/resize() -- silently do nothing if not yet connected (no queueing;
        // an input event that arrives before the first frame has nothing useful to hit
        // anyway). x/y are canvas-space pixels, matching this tab's current width/height.
        void mouseMove(int x, int y);
        // button matches LLViewerMediaImpl's own convention (0 = left, the only value any
        // current caller passes); is_down false = button-up. click_count matches CEF's own
        // SendMouseClickEvent() semantics (2 for the down half of a double-click, 1 otherwise,
        // including the up half of a double-click's second press -- see LLViewerMediaImpl's
        // mPendingDoubleClickUp for why that up also needs 2, not 1).
        void mouseButton(int x, int y, unsigned char button, bool is_down, unsigned char click_count = 1);
        void scrollWheel(int x, int y, int deltaY);
        // msg/wParam/lParam: a raw Win32 keyboard message triple, straight from
        // LLWindowWin32::getNativeKeyData(). Windows-only, matching the producer's own
        // SendKeyEvent.
        void keyEvent(unsigned int msg, unsigned int wParam, unsigned int lParam);
        // Drives CEF's own caret blink and focus/blur page JS -- call with true when the
        // LLMediaCtrl hosting this tab gains keyboard focus, false when it loses it.
        void setFocus(bool focus);
        // Binary mute/unmute of this tab's audio -- see kSetMuted's own comment in
        // cefshm_protocol.h for why this can't be a continuous volume level.
        void setMuted(bool muted);
        // Caps how often the producer paints this tab (0 = unthrottled/full rate) --
        // see kSetRenderRate's own comment in cefshm_protocol.h. priorityTier and url
        // are diagnostic only, echoed in the producer's own console/log output.
        void setRenderRate(unsigned int targetFps, unsigned int priorityTier, const std::string& url);
        // The real producer slot index this tab is connected to (see kSlotAssigned),
        // for correlating a consumer-side log line with the producer's own console
        // output -- false if not yet connected (or disconnected again).
        bool getSlotIndex(unsigned int& out_index) const;
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
        const bool mIsUI;
        unsigned char* mPixels = nullptr;
        unsigned int mWidth = 0;
        unsigned int mHeight = 0;
        // The most recently requested size, tracked separately from mWidth/mHeight
        // (which only advance once a frame published at that size actually arrives -
        // see update()). resize() updates this unconditionally, even before mSub is
        // connected, so a resize requested during the connection handshake (which can
        // take up to a few seconds) isn't silently dropped -- connectToProducer() uses
        // this, not mWidth/mHeight, for its own initial resize.
        unsigned int mRequestedWidth = 0;
        unsigned int mRequestedHeight = 0;
        // LLEmbeddedBrowser's own max-dimension ceiling at the moment this tab was
        // created (a snapshot, not live -- see the constructor's own comment), sent
        // to the producer via kRequestSlot so it can size this slot's shared-memory
        // segment accordingly.
        const unsigned int mMaxWidth;
        const unsigned int mMaxHeight;
        const unsigned int mDepth = 4;
        std::string mCurrentUrl;
        // Latest state reported by the producer's kEventNavStateChanged, under
        // mPixelMutex like mCurrentUrl -- see canGoBack()/canGoForward().
        bool mCanGoBack = false;
        bool mCanGoForward = false;
        // The real producer slot index from the most recent kSlotAssigned -- see
        // getSlotIndex(). Cleared back to "unknown" on disconnect (see
        // mHadDisconnected) so a stale index from a previous connection episode
        // can't be reported after this tab moves to a different slot.
        bool mHasSlotIndex = false;
        unsigned int mSlotIndex = 0;
        std::deque<LLEmbeddedBrowserEvent> mEvents;
};

class LLEmbeddedBrowser : public LLSingleton<LLEmbeddedBrowser> {
        LLSINGLETON(LLEmbeddedBrowser);

    public:
        ~LLEmbeddedBrowser();

        void init();
        void reset();

        // isUI: true for 2D floater/UI media, false for in-world/prim media -- see
        // LLEmbeddedBrowserTab's own constructor comment.
        unsigned int create(const std::string& url, unsigned int width, unsigned int height, bool isUI);
        void destroy(unsigned int id);
        void update(unsigned int id);

        // Broadcasts a cookie to the (currently or eventually) running producer's UI
        // context, and -- only if alsoPrimContext is also true -- the prim context too
        // (see kSetOpenIDCookie's own comment in cefshm_protocol.h; the caller's own
        // static policy switch for this lives in LLViewerMedia::getOpenIDCookieCoro()).
        // Not tied to any particular tab: this is the embedded-browser equivalent of
        // LLPluginClassMedia::injectOpenIDCookie(), except CEF's cookie store is already
        // shared by every tab within a context (unlike the legacy plugin's one-
        // isolated-cache-per-process model), so setting it once here is sufficient for
        // every current and future tab in that context -- no per-instance injection
        // needed. Best-effort/fire-and-forget: silently does nothing if no producer is
        // reachable right now (matches the legacy plugin's own "no OpenID cookie yet"
        // no-op case, e.g. before login completes).
        void setOpenIDCookie(const std::string& url, const std::string& name, const std::string& value,
                              const std::string& domain, const std::string& path, bool httpOnly, bool secure,
                              bool alsoPrimContext);
        void updateAll();
        const unsigned char* getPixels(unsigned int id);
        bool copyPixels(unsigned int id, std::vector<unsigned char>& out_pixels, unsigned int& out_width, unsigned int& out_height);
        unsigned int getWidth(unsigned int id);
        unsigned int getHeight(unsigned int id);
        void navigate(unsigned int id, const std::string& url);
        void goBack(unsigned int id);
        void goForward(unsigned int id);
        void stopLoad(unsigned int id);
        void reload(unsigned int id, bool ignoreCache);
        bool canGoBack(unsigned int id);
        bool canGoForward(unsigned int id);
        void resize(unsigned int id, unsigned int width, unsigned int height);
        void setPageZoom(unsigned int id, float zoom);
        void executeJavaScript(unsigned int id, const std::string& code);
        void cut(unsigned int id);
        void copy(unsigned int id);
        void paste(unsigned int id);

        void mouseMove(unsigned int id, int x, int y);
        void mouseButton(unsigned int id, int x, int y, unsigned char button, bool is_down, unsigned char click_count = 1);
        void scrollWheel(unsigned int id, int x, int y, int deltaY);
        void keyEvent(unsigned int id, unsigned int msg, unsigned int wParam, unsigned int lParam);
        void setFocus(unsigned int id, bool focus);
        void setMuted(unsigned int id, bool muted);
        void setRenderRate(unsigned int id, unsigned int targetFps, unsigned int priorityTier, const std::string& url);
        bool getSlotIndex(unsigned int id, unsigned int& out_index);
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

        // Diagnostic only (added 2026-08-19 while chasing a crash inside LLMutex::
        // lock(), reached via findTab() on a per-tab background update thread, under
        // heavy concurrent-media load): a crash that specific means `this` (or
        // something it points at) was invalid at the moment of the call, but static
        // code reading alone couldn't confirm whether that's a genuine use-after-free/
        // heap corruption or something else. A plain member read here can't be made
        // any safer than the mutex lock itself was if `this` is fully unmapped -- but
        // if the real cause is heap corruption or a stale pointer into a still-mapped,
        // reused block (the more likely case on Windows' default heap, which doesn't
        // unmap small freed blocks immediately), checking this turns a raw,
        // uninformative access violation into a clear, actionable diagnosis instead.
        // Remove once the actual root cause is confirmed and fixed.
        static constexpr std::uint32_t kAliveCanary = 0x8bb1a91e;
        static constexpr std::uint32_t kDeadCanary  = 0xdeadc0de;
        std::uint32_t mAliveCanary = kAliveCanary;

        mutable LLMutex mTabsMutex;
        std::map<unsigned int, std::shared_ptr<LLEmbeddedBrowserTab>> mTabs;
        unsigned int mNextTabId = 0;
        unsigned int mMaxWidth = 4096;
        unsigned int mMaxHeight = 4096;

        // A plain std::mutex, not LLMutex: this is read from LLFloaterAbout's
        // fetchServerReleaseNotesCoro coroutine (via getCefBrowserVersion(), see
        // llappviewer.cpp's getViewerInfo()) as well as from ordinary code, and
        // LLMutex asserts it is never locked from within a coroutine. A real
        // OS-level mutex is still correct here -- the actual writer is a genuine
        // background OS thread (LLEmbeddedBrowserTab's update thread), not another
        // coroutine, so there's no cooperative-scheduling deadlock risk to guard
        // against, just a short-lived std::string read/write to protect.
        mutable std::mutex mCefVersionMutex;
        std::string mCefBrowserVersion;

        // Guards mProducerProcess and the relaunch bookkeeping below -- touched from
        // init()/reset() on the main thread and from maybeRelaunchProducer()/
        // resetRelaunchAttempts() on any tab's own background update thread.
        mutable LLMutex mProducerMutex;
        std::shared_ptr<LLProcess> mProducerProcess;
        int mProducerRelaunchAttempts = 0;
        std::chrono::steady_clock::time_point mLastRelaunchAttempt;

        // Brackets the other end of the object from mAliveCanary above -- diagnostic
        // only, same removal note applies. A hit on this one but not the leading one
        // (or vice versa) narrows a stray write to one side of the object; both
        // hit together, or with very different values, suggests something broader
        // (e.g. an oversized copy) rather than a single stray pointer-sized write.
        std::uint32_t mTrailingCanary = kAliveCanary;
};
