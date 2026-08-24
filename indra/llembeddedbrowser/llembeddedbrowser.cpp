
/**
 * @file llembeddedbrowser.cpp
 * @brief Implementation of LLEmbeddedBrowser class
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

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "linden_common.h"

#include "llembeddedbrowser.h"

#include "llthread.h"

#include <shmframe/llshmframe.h>
#include <shmframe/llShmFrameVersion.h>
#include "cefshm_protocol.h"

#include "llapp.h"
#include "llcontrol.h"
#include "lldir.h"
#include "llprocess.h"

// llembeddedbrowser sits below newview in the link graph, so gSavedSettings
// (defined in llviewercontrol.cpp) is only reachable via extern -- resolved
// at final-link time, matching the same pattern llplugin/llpluginclassmedia.cpp
// already uses for the same reason.
extern LLControlGroup gSavedSettings;

using namespace cefshm_demo;

namespace {
    // Matches llcefshm-example's own CefShmConsumer::connectToProducer() timings --
    // long enough to outlast a losing control-channel race against another consumer
    // (that library steals a crashed claimant's channel after ~2s) or a producer
    // that just hasn't started yet.
    constexpr auto kControlClaimRetryInterval = std::chrono::milliseconds(50);
    constexpr auto kControlClaimTimeout       = std::chrono::seconds(3);
    constexpr auto kSlotRequestTimeout        = std::chrono::seconds(2);
    constexpr auto kSlotReplyPollInterval     = std::chrono::milliseconds(5);

    // How long a one-shot, best-effort control-channel broadcast (kShutdownProducer,
    // kSetOpenIDCookie) will retry claiming the channel before giving up silently.
    constexpr auto kControlBroadcastTimeout = std::chrono::milliseconds(500);

    // How long LLEmbeddedBrowser::reset() waits for a graceful kShutdownProducer
    // request to actually exit the producer before giving up and falling back to
    // LLProcess::kill() (a hard TerminateProcess() on Windows, no cleanup at all).
    constexpr auto kShutdownGracePeriod  = std::chrono::seconds(2);
    constexpr auto kShutdownPollInterval = std::chrono::milliseconds(50);

    // Bounds how aggressively LLEmbeddedBrowser::maybeRelaunchProducer() will
    // respawn SLCefProducer: multiple tabs' background threads can all notice
    // "not running" within milliseconds of each other (debounced by the backoff
    // below), and a second concurrent Viewer instance racing for the same
    // control-channel name would otherwise tight-loop respawning a process that
    // dies almost immediately every time (its own control-channel LLPublisher::
    // create() fails against the first instance's live one) -- capping total
    // attempts bounds that worst case at a few wasted process launches rather
    // than an unbounded spin.
    constexpr int  kMaxRelaunchAttempts = 3;
    constexpr auto kRelaunchBackoff     = std::chrono::seconds(5);
}

LLEmbeddedBrowserTab::LLEmbeddedBrowserTab(LLEmbeddedBrowser* browser, unsigned int id, const std::string& url, unsigned int width, unsigned int height, bool isUI, unsigned int maxWidth, unsigned int maxHeight) :
    mIsUI(isUI),
    mWidth(width),
    mHeight(height),
    mRequestedWidth(width),
    mRequestedHeight(height),
    mMaxWidth(maxWidth),
    mMaxHeight(maxHeight),
    mCurrentUrl(url)
{
    // Zero-initialized: this shows as black until connectToProducer() succeeds and the
    // first real frame arrives, rather than whatever garbage new[] handed back -- the
    // handshake with cefshm_producer can take up to a few seconds, unlike the
    // checkerboard placeholder this replaces, which painted its first frame instantly.
    mPixels = new unsigned char[(size_t)mWidth * mHeight * mDepth]();

    mUpdateThread = std::make_unique<LLEmbeddedBrowserUpdateThread>(browser, id);
    mUpdateThread->start();
}

void LLEmbeddedBrowserTab::stopUpdateThread()
{
    if (!mUpdateThread) return;

    mUpdateThread->shutdown();

    // Diagnostic only -- see isInsideRunLoop()'s own comment. If this ever fires,
    // it proves shutdown() concluded the thread was done while its detached OS
    // thread was still genuinely inside run()'s loop body. Confirmed to fire in
    // practice (2026-08-19) for exactly the self-referential-destruction case
    // LLEmbeddedBrowser::destroy()'s own call to this method now prevents --
    // safe to remove once that fix has had a real soak test.
    if (mUpdateThread->isInsideRunLoop())
    {
        LL_ERRS("EmbeddedBrowser") << "LLEmbeddedBrowserTab::stopUpdateThread(): "
            "shutdown() returned but the update thread is still inside its run() loop "
            "body -- this proves the thread-lifecycle race, not corrupted memory "
            "contents after the fact." << LL_ENDL;
    }
}

LLEmbeddedBrowserTab::~LLEmbeddedBrowserTab()
{
    stopUpdateThread();
    mUpdateThread.reset();

    {
        LLMutexLock lock(&mPixelMutex);
        mSub.reset(); // clean detach -- lets cefshm_producer free this slot right away
    }

    delete[] mPixels;
    mPixels = nullptr;
}

bool LLEmbeddedBrowserTab::connectToProducer()
{
    // Claim the control channel. A losing race must destroy this LLSubscriber and
    // open() a fresh one to retry -- poll() on an already-connected instance can never
    // re-attempt the claim, it only re-validates the session it already has.
    std::unique_ptr<LLSubscriber> ctrl;
    const auto claim_deadline = std::chrono::steady_clock::now() + kControlClaimTimeout;
    for (;;)
    {
        ctrl = LLSubscriber::open(kControlChannelName);
        if (!ctrl->connected())
        {
            // The one failure branch in this method that means "no producer
            // process at all," as opposed to one that's merely busy/racing
            // -- see LLEmbeddedBrowser::maybeRelaunchProducer().
            LLEmbeddedBrowser::instance().maybeRelaunchProducer();
            return false; // no cefshm_producer reachable right now
        }
        if (ctrl->owns_command_channel()) break;

        if (std::chrono::steady_clock::now() >= claim_deadline)
        {
            return false;
        }
        ctrl.reset();
        std::this_thread::sleep_for(kControlClaimRetryInterval);
    }

    std::uint64_t req_id = 0;
    std::uint8_t request_payload[9];
    const std::uint32_t request_len = pack_request_slot(request_payload, mIsUI, mMaxWidth, mMaxHeight);
    if (!ctrl->send(kRequestSlot, request_payload, request_len, 0, &req_id))
    {
        return false;
    }

    LLCommand reply;
    bool got_reply = false;
    const auto reply_deadline = std::chrono::steady_clock::now() + kSlotRequestTimeout;
    while (std::chrono::steady_clock::now() < reply_deadline)
    {
        if (ctrl->receive(reply) && reply.reply_to == req_id) { got_reply = true; break; }
        std::this_thread::sleep_for(kSlotReplyPollInterval);
    }
    if (!got_reply)
    {
        return false;
    }

    std::uint32_t index = 0;
    if (reply.type != kSlotAssigned || !unpack_u32(reply.data.data(), reply.data.size(), index))
    {
        return false; // producer has no free slot right now
    }

    ctrl.reset(); // release the control claim for the next requester

    auto sub = LLSubscriber::open(kChannelPrefix + std::to_string(index));
    if (!sub->connected() || !sub->owns_command_channel())
    {
        return false;
    }

    LLMutexLock lock(&mPixelMutex);
    mSub = std::move(sub);
    mSlotIndex = index;
    mHasSlotIndex = true;
    // The producer always starts a fresh view at its own default (960x540), regardless
    // of what this tab's create()/resize() actually asked for -- ask it to match right
    // away, and BEFORE the initial navigate below, rather than sitting at the wrong size
    // until some later, unrelated resize(). Sending kSetUrl first (as this used to) let
    // the real page start loading/laying itself out at 960x540 -- if the page decides its
    // responsive layout via its own JS rather than pure CSS media queries, that wrong-size
    // decision can stick even once the buffer is resized correctly a moment later, which
    // is exactly what an intermittently mis-sized login page looks like. Uses
    // mRequestedWidth/mRequestedHeight, not mWidth/mHeight: this handshake can take up to
    // a few seconds (see mUpdateThread's own comment), and a resize() requested by the
    // caller anytime during that window only ever updates the former (see resize()).
    std::uint8_t payload[8];
    pack_size(payload, mRequestedWidth, mRequestedHeight);
    mSub->send(kResize, payload, 8);
    if (!mCurrentUrl.empty())
    {
        mSub->send_text(kSetUrl, mCurrentUrl);
    }

    if (mHadDisconnected)
    {
        mHadDisconnected = false;
        LLEmbeddedBrowserEvent event;
        event.type = LLEmbeddedBrowserEventType::ProducerReconnected;
        mEvents.push_back(event);
    }

    // A real connection just succeeded, so any earlier relaunch attempts
    // (this episode or a prior one) are no longer relevant -- give a later,
    // unrelated crash its own fresh attempt budget.
    LLEmbeddedBrowser::instance().resetRelaunchAttempts();

    return true;
}

void LLEmbeddedBrowserTab::update()
{
    if (!mSub)
    {
        connectToProducer(); // best-effort; failure just leaves the current buffer and retries next tick
        return;
    }

    LLCommand cmd;
    while (mSub->receive(cmd))
    {
        LLEmbeddedBrowserEvent event;
        switch (cmd.type)
        {
            case kEventLoadStart:
                event.type = LLEmbeddedBrowserEventType::LoadStart;
                break;
            case kEventLoadEnd:
                event.type = LLEmbeddedBrowserEventType::LoadEnd;
                unpack_u32(cmd.data.data(), cmd.data.size(), event.mValue);
                break;
            case kEventTitleChanged:
                event.type = LLEmbeddedBrowserEventType::TitleChanged;
                event.mText = std::string(cmd.text());
                break;
            case kEventAddressChanged:
                event.type = LLEmbeddedBrowserEventType::AddressChanged;
                event.mText = std::string(cmd.text());
                break;
            case kEventCursorChanged:
                event.type = LLEmbeddedBrowserEventType::CursorChanged;
                unpack_u32(cmd.data.data(), cmd.data.size(), event.mValue);
                break;
            case kEventClickLinkHref:
                event.type = LLEmbeddedBrowserEventType::ClickLinkHref;
                unpack_click_href(cmd.data.data(), cmd.data.size(), event.mText, event.mTarget);
                break;
            case kEventClickLinkNoFollow:
                event.type = LLEmbeddedBrowserEventType::ClickLinkNoFollow;
                unpack_click_nofollow(cmd.data.data(), cmd.data.size(), event.mText,
                                      event.mUserGesture, event.mIsRedirect);
                break;
            case kEventFileDialogRequest: {
                event.type = LLEmbeddedBrowserEventType::FileDialogRequest;
                std::int64_t dialogId = 0;
                unpack_file_dialog_request(cmd.data.data(), cmd.data.size(), dialogId, event.mValue, event.mText);
                event.mDialogId = dialogId;
                break;
            }
            case kEventStatusTextChanged:
                event.type = LLEmbeddedBrowserEventType::StatusTextChanged;
                event.mText = std::string(cmd.text());
                break;
            case kEventConsoleMessage: {
                event.type = LLEmbeddedBrowserEventType::ConsoleMessage;
                std::int32_t line = 0;
                unpack_console_message(cmd.data.data(), cmd.data.size(), event.mText, event.mTarget, line);
                event.mValue = static_cast<unsigned int>(line);
                break;
            }
            case kEventVersionInfo:
                // Global info about whatever producer is connected, not a per-tab UI
                // event -- doesn't go through mEvents.
                LLEmbeddedBrowser::instance().setCefBrowserVersion(std::string(cmd.text()));
                continue;
            case kEventNavStateChanged:
                // Cached state, polled every frame via canGoBack()/canGoForward() (to
                // enable/disable a back/forward button) -- not a discrete occurrence,
                // so doesn't go through mEvents either.
                if (cmd.data.size() >= 2)
                {
                    LLMutexLock lock(&mPixelMutex);
                    mCanGoBack    = cmd.data[0] != 0;
                    mCanGoForward = cmd.data[1] != 0;
                }
                continue;
            default:
                continue; // not an event opcode this tab understands
        }

        LLMutexLock lock(&mPixelMutex);
        mEvents.push_back(event);
    }

    std::vector<unsigned char> frame_buf;
    LLFrameInfo info{};
    const LLReadResult result = mSub->read_latest(frame_buf, info);

    if (result == LLReadResult::Disconnected)
    {
        LLMutexLock lock(&mPixelMutex);
        mSub.reset(); // producer went away -- connectToProducer() retries on a later tick
        mHasSlotIndex = false; // stale once disconnected -- a reconnect may land on a different slot
        if (!mHadDisconnected)
        {
            mHadDisconnected = true;
            LLEmbeddedBrowserEvent event;
            event.type = LLEmbeddedBrowserEventType::ProducerDisconnected;
            mEvents.push_back(event);
        }
        return;
    }

    if (result != LLReadResult::Ok || info.width == 0 || info.height == 0)
    {
        return; // NoNewFrame/Contended/Throttled/BufferTooSmall -- nothing to display yet
    }

    LLMutexLock lock(&mPixelMutex);
    if (info.width != mWidth || info.height != mHeight)
    {
        delete[] mPixels;
        mWidth = info.width;
        mHeight = info.height;
        mPixels = new unsigned char[(size_t)mWidth * mHeight * mDepth];
    }

    // CEF's OnPaint (and this whole shm pipeline) hands back top-down rows, but prim-face
    // rendering has no orientation compensation of its own anywhere (unlike LLMediaCtrl's
    // floater quad, which picks its UV winding based on the media source's self-reported
    // coordinate convention) -- it just trusts mPixels' row order to already match what
    // the CEF media plugin has always supplied for prim faces (bottom-up), so this flips
    // on the way in rather than leaving that to a UV fix that doesn't exist for prims.
    const size_t row_bytes = (size_t)mWidth * mDepth;
    const unsigned char* src = frame_buf.data();
    for (unsigned int y = 0; y < mHeight; ++y)
    {
        memcpy(mPixels + (size_t)y * row_bytes, src + (size_t)(mHeight - 1 - y) * row_bytes, row_bytes);
    }
}

const unsigned char* LLEmbeddedBrowserTab::getPixels()
{
    LLMutexLock lock(&mPixelMutex);

    return mPixels;
}

bool LLEmbeddedBrowserTab::copyPixels(std::vector<unsigned char>& out_pixels, unsigned int& out_width, unsigned int& out_height)
{
    LLMutexLock lock(&mPixelMutex);

    out_width = mWidth;
    out_height = mHeight;
    out_pixels.assign(mPixels, mPixels + (size_t)mWidth * mHeight * mDepth);
    return true;
}

void LLEmbeddedBrowserTab::navigate(const std::string& url)
{
    LLMutexLock lock(&mPixelMutex);
    mCurrentUrl = url;
    if (mSub)
    {
        mSub->send_text(kSetUrl, url);
    }
}

void LLEmbeddedBrowserTab::executeJavaScript(const std::string& code)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        mSub->send_text(kExecuteJavaScript, code);
    }
}

void LLEmbeddedBrowserTab::resize(unsigned int width, unsigned int height)
{
    LLMutexLock lock(&mPixelMutex);

    if (width == mRequestedWidth && height == mRequestedHeight)
    {
        return;
    }
    mRequestedWidth = width;
    mRequestedHeight = height;

    // Just a hint to the producer -- the local buffer is reconciled in update() once a
    // frame published at the new size actually arrives, same as llcefshm-example's own
    // consumer does, rather than resizing mPixels ahead of that round trip. If mSub isn't
    // connected yet, mRequestedWidth/mRequestedHeight above is what connectToProducer()
    // sends as its own initial resize once it does connect -- this call itself is a
    // no-op rather than lost, unlike relying on mWidth/mHeight (which don't advance until
    // a frame actually arrives at the new size).
    if (mSub)
    {
        std::uint8_t payload[8];
        pack_size(payload, width, height);
        mSub->send(kResize, payload, 8);
    }
}

void LLEmbeddedBrowserTab::setPageZoom(float zoom)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::uint8_t payload[4];
        pack_f32(payload, zoom);
        mSub->send(kSetPageZoom, payload, 4);
    }
}

void LLEmbeddedBrowserTab::mouseMove(int x, int y)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::uint8_t payload[8];
        pack_i32x2(payload, x, y);
        mSub->send(kMouseMove, payload, 8);
    }
}

void LLEmbeddedBrowserTab::mouseButton(int x, int y, unsigned char button, bool is_down)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        // action: 0 = up, 1 = down, matching cef_mouse_up()'s convention in cefshm_producer.cpp.
        std::uint8_t payload[10];
        const std::uint32_t n = pack_mouse_button(payload, x, y, button, is_down ? 1 : 0);
        mSub->send(kMouseButton, payload, n);
    }
}

void LLEmbeddedBrowserTab::scrollWheel(int x, int y, int deltaY)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::uint8_t payload[12];
        const std::uint32_t n = pack_scroll(payload, x, y, deltaY);
        mSub->send(kScrollWheel, payload, n);
    }
}

void LLEmbeddedBrowserTab::keyEvent(unsigned int msg, unsigned int wParam, unsigned int lParam)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::uint8_t payload[12];
        const std::uint32_t n = pack_key_event(payload, msg, wParam, lParam);
        mSub->send(kKeyEvent, payload, n);
    }
}

void LLEmbeddedBrowserTab::setFocus(bool focus)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::uint8_t payload[1] = { focus ? std::uint8_t(1) : std::uint8_t(0) };
        mSub->send(kSetFocus, payload, 1);
    }
}

void LLEmbeddedBrowserTab::setMuted(bool muted)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::uint8_t payload[1] = { muted ? std::uint8_t(1) : std::uint8_t(0) };
        mSub->send(kSetMuted, payload, 1);
    }
}

void LLEmbeddedBrowserTab::setRenderRate(unsigned int targetFps, unsigned int priorityTier, const std::string& url)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::vector<std::uint8_t> payload(5 + url.size());
        const std::uint32_t n = pack_render_rate(payload.data(), std::uint32_t(targetFps),
                                                  std::uint8_t(priorityTier), url);
        mSub->send(kSetRenderRate, payload.data(), n);
    }
}

bool LLEmbeddedBrowserTab::getSlotIndex(unsigned int& out_index) const
{
    LLMutexLock lock(&mPixelMutex);
    if (mHasSlotIndex)
    {
        out_index = mSlotIndex;
    }
    return mHasSlotIndex;
}

void LLEmbeddedBrowserTab::cut()
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        mSub->send(kCut);
    }
}

void LLEmbeddedBrowserTab::copy()
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        mSub->send(kCopy);
    }
}

void LLEmbeddedBrowserTab::paste()
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        mSub->send(kPaste);
    }
}

void LLEmbeddedBrowserTab::goBack()
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        mSub->send(kGoBack);
    }
}

void LLEmbeddedBrowserTab::goForward()
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        mSub->send(kGoForward);
    }
}

void LLEmbeddedBrowserTab::stopLoad()
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        mSub->send(kStopLoad);
    }
}

bool LLEmbeddedBrowserTab::canGoBack() const
{
    LLMutexLock lock(&mPixelMutex);
    return mCanGoBack;
}

bool LLEmbeddedBrowserTab::canGoForward() const
{
    LLMutexLock lock(&mPixelMutex);
    return mCanGoForward;
}

void LLEmbeddedBrowserTab::respondToFileDialog(long long dialogId, const std::vector<std::string>& filePaths)
{
    LLMutexLock lock(&mPixelMutex);
    if (mSub)
    {
        std::size_t size = 12;
        for (const auto& path : filePaths)
        {
            size += 4 + path.size();
        }
        std::vector<std::uint8_t> payload(size);
        const std::uint32_t n = pack_file_dialog_response(payload.data(), dialogId, filePaths);
        mSub->send(kFileDialogResponse, payload.data(), n);
    }
}

bool LLEmbeddedBrowserTab::popEvent(LLEmbeddedBrowserEvent& out_event)
{
    LLMutexLock lock(&mPixelMutex);
    if (mEvents.empty())
    {
        return false;
    }
    out_event = mEvents.front();
    mEvents.pop_front();
    return true;
}

unsigned int LLEmbeddedBrowserTab::getWidth() const
{
    LLMutexLock lock(&mPixelMutex);
    return mWidth;
}

unsigned int LLEmbeddedBrowserTab::getHeight() const
{
    LLMutexLock lock(&mPixelMutex);
    return mHeight;
}

namespace {
    // Claims the control channel for a single fire-and-forget broadcast, retrying for
    // up to timeout. Returns a connected, owning LLSubscriber, or nullptr if no
    // producer is reachable right now or the channel couldn't be claimed in time --
    // both are silently-fine outcomes for every caller below (a best-effort broadcast
    // to a producer that isn't running, or isn't running yet, has nothing to reach).
    std::unique_ptr<LLSubscriber> claimControlChannel(std::chrono::milliseconds timeout)
    {
        const auto claim_deadline = std::chrono::steady_clock::now() + timeout;
        for (;;)
        {
            auto ctrl = LLSubscriber::open(kControlChannelName);
            if (!ctrl->connected())
            {
                return nullptr;
            }
            if (ctrl->owns_command_channel())
            {
                return ctrl;
            }
            if (std::chrono::steady_clock::now() >= claim_deadline)
            {
                return nullptr;
            }
            std::this_thread::sleep_for(kControlClaimRetryInterval);
        }
    }

    // Asks a running producer to shut down gracefully (kShutdownProducer) rather than
    // being killed outright -- see that opcode's own comment in cefshm_protocol.h.
    // Does not itself wait for the producer to actually exit; the caller does that.
    void requestGracefulShutdown()
    {
        if (auto ctrl = claimControlChannel(kControlBroadcastTimeout))
        {
            ctrl->send(kShutdownProducer);
        }
    }

    // See LLEmbeddedBrowser::setOpenIDCookie()'s own comment.
    void broadcastOpenIDCookie(const std::string& url, const std::string& name, const std::string& value,
                               const std::string& domain, const std::string& path, bool httpOnly, bool secure,
                               bool alsoPrimContext)
    {
        auto ctrl = claimControlChannel(kControlBroadcastTimeout);
        if (!ctrl)
        {
            return;
        }

        std::vector<std::uint8_t> payload(url.size() + name.size() + value.size() + domain.size() +
                                           path.size() + 5 * 4 + 3);
        const std::uint32_t n = pack_openid_cookie(payload.data(), url, name, value, domain, path, httpOnly, secure,
                                                    alsoPrimContext);
        ctrl->send(kSetOpenIDCookie, payload.data(), n);
    }
}

LLEmbeddedBrowser::LLEmbeddedBrowser()
{
    //std::cout << "LLEmbeddedBrowser created" << std::endl;
}

LLEmbeddedBrowser::~LLEmbeddedBrowser()
{
    //std::cout << "LLEmbeddedBrowser destroyed" << std::endl;
    mAliveCanary = kDeadCanary;
    mTrailingCanary = kDeadCanary;
}

void LLEmbeddedBrowser::init()
{
    //std::cout << "Initializing LLEmbeddedBrowser" << std::endl;

    if (!gSavedSettings.getBOOL("UseEmbeddedBrowser"))
    {
        return; // nothing to launch -- the legacy plugin path handles all media instead
    }

    LLMutexLock lock(&mProducerMutex);
    launchProducer();
}

void LLEmbeddedBrowser::reset()
{
    {
        LLMutexLock lock(&mProducerMutex);
        if (LLProcess::isRunning(mProducerProcess))
        {
            // Ask nicely first: LLProcess::kill() is a hard TerminateProcess() on
            // Windows (see its own implementation -- apr_proc_kill() with sig = -1),
            // which gives CEF's on-disk cookie/history/etc. stores no chance to flush
            // whatever they haven't yet committed. Wait out a short grace period for
            // the producer to exit on its own before falling back to the hard kill
            // below, which still runs unconditionally as a safety net (a hung/
            // unresponsive producer, or one that never got the request at all,
            // must not block Viewer shutdown).
            requestGracefulShutdown();

            const auto deadline = std::chrono::steady_clock::now() + kShutdownGracePeriod;
            while (LLProcess::isRunning(mProducerProcess) && std::chrono::steady_clock::now() < deadline)
            {
                std::this_thread::sleep_for(kShutdownPollInterval);
            }
        }
        LLProcess::kill(mProducerProcess); // null-safe -- also a no-op if it already exited above
        mProducerProcess.reset();
    }

    // Same deadlock hazard as destroy() above (see its own comment): extract
    // every tab under the lock, then let their actual destruction -- each
    // one's blocking mUpdateThread->shutdown() join -- happen after
    // releasing mTabsMutex, so each tab's own background thread can still
    // reach findTab() to notice it should quit. Holding the lock across
    // clear() here would deadlock on exit the same way destroy() used to on
    // an individual floater close, just for every still-open tab at once.
    std::map<unsigned int, std::shared_ptr<LLEmbeddedBrowserTab>> tabs;
    {
        LLMutexLock lock(&mTabsMutex);
        tabs.swap(mTabs);
    }

    // Same reasoning as destroy()'s own call to this: stop every update thread
    // explicitly, here, before any of these shared_ptr copies can go out of
    // scope below -- a concurrent findTab() on one of these tabs' own update
    // threads could otherwise end up holding the last reference to it, running
    // that tab's destructor (and its own shutdown() call) on its own thread.
    for (auto& [id, tab] : tabs)
    {
        if (tab) tab->stopUpdateThread();
    }
    // tabs' destructors run here, unlocked -- safe now regardless of which
    // thread ends up holding each one's last reference.
}

bool LLEmbeddedBrowser::launchProducer()
{
    const std::string exe_path = gDirUtilp->getSLCefProducerLauncher();
    if (exe_path.empty())
    {
        LL_WARNS() << "SLCefProducer is not available on this platform" << LL_ENDL;
        return false;
    }

    LLProcess::Params params;
    params.executable = exe_path;
    params.cwd        = gDirUtilp->getLLPluginDir(); // SLCefProducer.exe's own directory -- see getSLCefProducerLauncher()
    if (gSavedSettings.getBOOL("EmbeddedBrowserProducerConsole"))
    {
        params.args.add("--console");
    }
    if (gSavedSettings.getBOOL("EmbeddedBrowserDebugging"))
    {
        // No in-process DevTools popup (CefBrowserHost::ShowDevTools) -- that opens a
        // real, GPU-composited native window, which reliably crashed/hung the renderer
        // on at least one real machine tested (see git history for the investigation).
        // Chrome's remote-debugging protocol serves the same DevTools UI over HTTP
        // instead, with no native window in this process at all -- open
        // http://localhost:<port> (see the producer's own log line) in any desktop
        // browser. 0 (the setting's own default) means disabled, matching CEF's own
        // remote_debugging_port convention.
        const unsigned int remote_debugging_port = gSavedSettings.getU32("EmbeddedBrowserRemoteDebuggingPort");
        if (remote_debugging_port > 0)
        {
            params.args.add("--remote-debugging-port=" + std::to_string(remote_debugging_port));
        }
    }
    // SLCefProducer.exe is a standalone process with no gDirUtilp of its own, so it
    // can't compute the per-user cache location itself -- pass it explicitly, under
    // the same parent directory the legacy CEF plugin uses for its own cache
    // (gDirUtilp->getCacheDir(false), see LLViewerMediaImpl::newSourceFromMediaType()'s
    // "cef_cache" -- see media_plugin_cef.cpp's set_user_data_path handler), rather
    // than under the application/install folder.
    //
    // Deliberately a SIBLING of "cef_cache", not nested inside it: unlike the legacy
    // plugin's own per-process-id throwaway caches, this producer's profile is a single
    // persistent one for the whole Viewer session/across sessions (cookies, local
    // storage, login state, not just disposable cache), and LLAppViewer::
    // purgeCefStaleCaches() unconditionally wipes "cef_cache" and everything under it
    // on every single startup -- nesting our persistent profile inside it would get it
    // deleted every time the Viewer launches.
    params.args.add("--cache-dir=" + gDirUtilp->add(gDirUtilp->getCacheDir(false), "cef_profile"));

    LLProcessPtr proc = LLProcess::create(params);
    if (!proc)
    {
        LL_WARNS() << "Failed to launch SLCefProducer (" << exe_path << ")" << LL_ENDL;
        return false;
    }

    LL_INFOS() << "Launched SLCefProducer, pid " << proc->getProcessID() << LL_ENDL;
    mProducerProcess = proc;
    return true;
}

void LLEmbeddedBrowser::maybeRelaunchProducer()
{
    if (!gSavedSettings.getBOOL("UseEmbeddedBrowser"))
    {
        return;
    }

    if (LLApp::isExiting())
    {
        // reset() (called from LLAppViewer::cleanup()) kills the producer and
        // only *afterwards* stops every tab's update thread -- see its own
        // comment. A tab thread that's still alive in that window can notice
        // the producer is gone and land here, right as the Viewer is on its
        // way out. Relaunching a brand new SLCefProducer.exe at that point
        // just races reset()'s own teardown: this is exactly what produced a
        // second, orphaned-looking producer process observed right as the
        // Viewer exits.
        return;
    }

    LLMutexLock lock(&mProducerMutex);

    if (mProducerProcess && LLProcess::isRunning(mProducerProcess))
    {
        return; // still alive -- this was a transient shm hiccup, not a real crash
    }

    if (mProducerRelaunchAttempts >= kMaxRelaunchAttempts)
    {
        return; // gave up already this episode -- see the constant's own comment
    }

    const auto now = std::chrono::steady_clock::now();
    if (mProducerRelaunchAttempts > 0 && now - mLastRelaunchAttempt < kRelaunchBackoff)
    {
        return; // debounce: another tab's thread likely just tried this
    }

    mLastRelaunchAttempt = now;
    ++mProducerRelaunchAttempts;

    LL_WARNS() << "SLCefProducer is not running (relaunch attempt " << mProducerRelaunchAttempts
               << "/" << kMaxRelaunchAttempts << ")" << LL_ENDL;
    launchProducer();
}

void LLEmbeddedBrowser::resetRelaunchAttempts()
{
    LLMutexLock lock(&mProducerMutex);
    mProducerRelaunchAttempts = 0;
}

std::shared_ptr<LLEmbeddedBrowserTab> LLEmbeddedBrowser::findTab(unsigned int id)
{
    if (mAliveCanary != kAliveCanary || mTrailingCanary != kAliveCanary)
    {
        LL_ERRS("EmbeddedBrowser") << "LLEmbeddedBrowser::findTab(" << id << ") called on a singleton whose "
            "memory doesn't match what its own constructor left it as (leading canary = 0x"
            << std::hex << mAliveCanary << (mAliveCanary == kDeadCanary ? " [destructed]" : "")
            << ", trailing canary = 0x" << mTrailingCanary << (mTrailingCanary == kDeadCanary ? " [destructed]" : "")
            << ", expected 0x" << kAliveCanary << " for both" << std::dec
            << ") -- this points at heap corruption or a stale/dangling pointer, not a logic bug in this "
            "function itself." << LL_ENDL;
    }

    LLMutexLock lock(&mTabsMutex);
    auto it = mTabs.find(id);
    return (it != mTabs.end()) ? it->second : nullptr;
}

unsigned int LLEmbeddedBrowser::create(const std::string& url, unsigned int width, unsigned int height, bool isUI)
{
    width = llmin(width, mMaxWidth);
    height = llmin(height, mMaxHeight);

    LLMutexLock lock(&mTabsMutex);
    unsigned int id = mNextTabId++;
    mTabs[id] = std::make_shared<LLEmbeddedBrowserTab>(this, id, url, width, height, isUI, mMaxWidth, mMaxHeight);
    return id;
}

void LLEmbeddedBrowser::setOpenIDCookie(const std::string& url, const std::string& name, const std::string& value,
                                        const std::string& domain, const std::string& path, bool httpOnly, bool secure,
                                        bool alsoPrimContext)
{
    broadcastOpenIDCookie(url, name, value, domain, path, httpOnly, secure, alsoPrimContext);
}

void LLEmbeddedBrowser::setMaxDimensions(unsigned int max_width, unsigned int max_height)
{
    mMaxWidth = max_width;
    mMaxHeight = max_height;
}

/*static*/
std::string LLEmbeddedBrowser::getShmFrameVersion()
{
    return std::to_string(LLSHMFRAME_VERSION_MAJOR) + "." +
           std::to_string(LLSHMFRAME_VERSION_MINOR) + " (" + LLSHMFRAME_VERSION_GITHASH + ")";
}

std::string LLEmbeddedBrowser::getCefBrowserVersion() const
{
    std::lock_guard<std::mutex> lock(mCefVersionMutex);
    return mCefBrowserVersion;
}

void LLEmbeddedBrowser::setCefBrowserVersion(const std::string& version)
{
    std::lock_guard<std::mutex> lock(mCefVersionMutex);
    mCefBrowserVersion = version;
}

void LLEmbeddedBrowser::destroy(unsigned int id)
{
    // Extract the tab and erase it from the map under the lock, but let its
    // actual destruction happen after releasing mTabsMutex. ~LLEmbeddedBrowserTab()
    // blocks on mUpdateThread->shutdown(), which waits for that tab's own
    // background thread to notice isQuitting() -- but that thread can only get
    // there via LLEmbeddedBrowser::update(id), which itself needs mTabsMutex
    // (through findTab()). Holding the lock across the blocking join here would
    // deadlock the background thread against itself: it could never re-acquire
    // the mutex to reach the point where it checks isQuitting(), so shutdown()
    // would never see isStopped() until its own 60s force-kill fallback.
    std::shared_ptr<LLEmbeddedBrowserTab> tab;
    {
        LLMutexLock lock(&mTabsMutex);
        auto it = mTabs.find(id);
        if (it != mTabs.end())
        {
            tab = it->second;
            mTabs.erase(it);
        }
    }

    if (tab)
    {
        // Stop this tab's own update thread here, explicitly, on the calling
        // (main) thread, BEFORE letting our local `tab` shared_ptr go out of
        // scope below. This is not redundant with ~LLEmbeddedBrowserTab()'s own
        // shutdown() call: findTab() (called from that same background thread's
        // own LLEmbeddedBrowser::update(id)) can race this erase() above and end
        // up holding the very last reference to this tab -- meaning the tab's
        // destructor, and the shutdown() call inside it, can otherwise run *on
        // that tab's own update thread*, which then blocks waiting for itself to
        // stop and eventually self-terminates via LLThread::shutdown()'s 60s
        // force-kill fallback, corrupting the process heap on the way out (this
        // is the actual, confirmed cause of a run of hard-to-diagnose crashes --
        // see LLEmbeddedBrowserUpdateThread::isInsideRunLoop()'s own comment).
        // Stopping the thread here first guarantees it has already, genuinely
        // exited run() by the time anyone's shared_ptr copy can possibly trigger
        // the destructor, regardless of which thread ends up holding that last
        // reference.
        tab->stopUpdateThread();
    }
    // tab's destructor (if this was the last reference) runs here, unlocked --
    // safe now no matter which thread it happens to run on.
}

void LLEmbeddedBrowser::resize(unsigned int id, unsigned int width, unsigned int height)
{
    width = llmin(width, mMaxWidth);
    height = llmin(height, mMaxHeight);

    if (auto tab = findTab(id))
    {
        tab->resize(width, height);
    }
}

void LLEmbeddedBrowser::setPageZoom(unsigned int id, float zoom)
{
    if (auto tab = findTab(id))
    {
        tab->setPageZoom(zoom);
    }
}

void LLEmbeddedBrowser::update(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->update();
    }
}

void LLEmbeddedBrowser::updateAll()
{
    std::vector<std::shared_ptr<LLEmbeddedBrowserTab>> tabs;
    {
        LLMutexLock lock(&mTabsMutex);
        tabs.reserve(mTabs.size());
        for (auto& entry : mTabs)
        {
            tabs.push_back(entry.second);
        }
    }
    for (auto& tab : tabs)
    {
        tab->update();
    }
}

const unsigned char* LLEmbeddedBrowser::getPixels(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->getPixels();
    }
    return nullptr;
}

bool LLEmbeddedBrowser::copyPixels(unsigned int id, std::vector<unsigned char>& out_pixels, unsigned int& out_width, unsigned int& out_height)
{
    if (auto tab = findTab(id))
    {
        return tab->copyPixels(out_pixels, out_width, out_height);
    }
    return false;
}

unsigned int LLEmbeddedBrowser::getWidth(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->getWidth();
    }
    return 0;
}

unsigned int LLEmbeddedBrowser::getHeight(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->getHeight();
    }
    return 0;
}

void LLEmbeddedBrowser::navigate(unsigned int id, const std::string& url)
{
    if (auto tab = findTab(id))
    {
        tab->navigate(url);
    }
}

void LLEmbeddedBrowser::goBack(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->goBack();
    }
}

void LLEmbeddedBrowser::goForward(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->goForward();
    }
}

void LLEmbeddedBrowser::stopLoad(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->stopLoad();
    }
}

bool LLEmbeddedBrowser::canGoBack(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->canGoBack();
    }
    return false;
}

bool LLEmbeddedBrowser::canGoForward(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        return tab->canGoForward();
    }
    return false;
}

void LLEmbeddedBrowser::executeJavaScript(unsigned int id, const std::string& code)
{
    if (auto tab = findTab(id))
    {
        tab->executeJavaScript(code);
    }
}

void LLEmbeddedBrowser::mouseMove(unsigned int id, int x, int y)
{
    if (auto tab = findTab(id))
    {
        tab->mouseMove(x, y);
    }
}

void LLEmbeddedBrowser::mouseButton(unsigned int id, int x, int y, unsigned char button, bool is_down)
{
    if (auto tab = findTab(id))
    {
        tab->mouseButton(x, y, button, is_down);
    }
}

void LLEmbeddedBrowser::scrollWheel(unsigned int id, int x, int y, int deltaY)
{
    if (auto tab = findTab(id))
    {
        tab->scrollWheel(x, y, deltaY);
    }
}

void LLEmbeddedBrowser::keyEvent(unsigned int id, unsigned int msg, unsigned int wParam, unsigned int lParam)
{
    if (auto tab = findTab(id))
    {
        tab->keyEvent(msg, wParam, lParam);
    }
}

void LLEmbeddedBrowser::setFocus(unsigned int id, bool focus)
{
    if (auto tab = findTab(id))
    {
        tab->setFocus(focus);
    }
}

void LLEmbeddedBrowser::setMuted(unsigned int id, bool muted)
{
    if (auto tab = findTab(id))
    {
        tab->setMuted(muted);
    }
}

void LLEmbeddedBrowser::setRenderRate(unsigned int id, unsigned int targetFps, unsigned int priorityTier,
                                       const std::string& url)
{
    if (auto tab = findTab(id))
    {
        tab->setRenderRate(targetFps, priorityTier, url);
    }
}

bool LLEmbeddedBrowser::getSlotIndex(unsigned int id, unsigned int& out_index)
{
    if (auto tab = findTab(id))
    {
        return tab->getSlotIndex(out_index);
    }
    return false;
}

void LLEmbeddedBrowser::cut(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->cut();
    }
}

void LLEmbeddedBrowser::copy(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->copy();
    }
}

void LLEmbeddedBrowser::paste(unsigned int id)
{
    if (auto tab = findTab(id))
    {
        tab->paste();
    }
}

void LLEmbeddedBrowser::respondToFileDialog(unsigned int id, long long dialogId, const std::vector<std::string>& filePaths)
{
    if (auto tab = findTab(id))
    {
        tab->respondToFileDialog(dialogId, filePaths);
    }
}

bool LLEmbeddedBrowser::popEvent(unsigned int id, LLEmbeddedBrowserEvent& out_event)
{
    if (auto tab = findTab(id))
    {
        return tab->popEvent(out_event);
    }
    return false;
}

void LLEmbeddedBrowserUpdateThread::run()
{
    // Scale the update rate down for large tabs so the per-frame full-buffer fill/lock
    // cost stays roughly bounded regardless of tab size: tabs up to 1280x720 run at
    // max_fps, and the rate falls off as pixel count grows, floored at min_fps.
    //
    // These numbers were originally tuned for the checkerboard-placeholder generator
    // (bounding the CPU cost of *painting* a synthetic pattern); now that this pulls
    // real CEF frames, the same throttle governs input-to-display latency too -- the
    // budget below was raised (2026-08-13) after real interactive testing showed the
    // old 512x512-at-60fps / 10fps-floor numbers made mouse-move feedback feel sluggish
    // on anything larger than a small thumbnail. Revisit downward again if this proves
    // too costly on low-end hardware -- see the memory-comparison work elsewhere in this
    // project for why that tradeoff matters here.
    const unsigned int max_fps = 60;
    const unsigned int min_fps = 30;
    const unsigned long long budget_pixels_per_sec = 1280ull * 720ull * max_fps;

    while (! isQuitting())
    {
        mInsideRunLoop.store(true);

        mBrowser->update(mBrowserId);

        unsigned long long pixels = (unsigned long long)mBrowser->getWidth(mBrowserId) * (unsigned long long)mBrowser->getHeight(mBrowserId);

        unsigned int fps = max_fps;
        if (pixels > 0)
        {
            unsigned long long computed_fps = llclamp(budget_pixels_per_sec / pixels, (unsigned long long)min_fps, (unsigned long long)max_fps);
            fps = (unsigned int)computed_fps;
        }

        mInsideRunLoop.store(false);

        ms_sleep(1000 / fps);
    }
}
