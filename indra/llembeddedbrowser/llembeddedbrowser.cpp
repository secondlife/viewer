
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
#include "cefshm_protocol.h"

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
}

LLEmbeddedBrowserTab::LLEmbeddedBrowserTab(LLEmbeddedBrowser* browser, unsigned int id, const std::string& url, unsigned int width, unsigned int height) :
    mWidth(width),
    mHeight(height),
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

LLEmbeddedBrowserTab::~LLEmbeddedBrowserTab()
{
    if (mUpdateThread)
    {
        mUpdateThread->shutdown();
        mUpdateThread.reset();
    }

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
    if (!ctrl->send(kRequestSlot, nullptr, 0, 0, &req_id))
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
    if (!mCurrentUrl.empty())
    {
        mSub->send_text(kSetUrl, mCurrentUrl);
    }
    // The producer always starts a fresh view at its own default (960x540), regardless
    // of what this tab's create()/resize() actually asked for -- ask it to match right
    // away rather than sitting at the wrong size until some later, unrelated resize().
    std::uint8_t payload[8];
    pack_size(payload, mWidth, mHeight);
    mSub->send(kResize, payload, 8);
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

    if (width == mWidth && height == mHeight)
    {
        return;
    }

    // Just a hint to the producer -- the local buffer is reconciled in update() once a
    // frame published at the new size actually arrives, same as llcefshm-example's own
    // consumer does, rather than resizing mPixels ahead of that round trip.
    if (mSub)
    {
        std::uint8_t payload[8];
        pack_size(payload, width, height);
        mSub->send(kResize, payload, 8);
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

LLEmbeddedBrowser::LLEmbeddedBrowser()
{
    std::cout << "LLEmbeddedBrowser created" << std::endl;
}

LLEmbeddedBrowser::~LLEmbeddedBrowser()
{
    std::cout << "LLEmbeddedBrowser destroyed" << std::endl;
}

void LLEmbeddedBrowser::init()
{
    std::cout << "Initializing LLEmbeddedBrowser" << std::endl;
}

void LLEmbeddedBrowser::reset()
{
    LLMutexLock lock(&mTabsMutex);
    mTabs.clear();
}

std::shared_ptr<LLEmbeddedBrowserTab> LLEmbeddedBrowser::findTab(unsigned int id)
{
    LLMutexLock lock(&mTabsMutex);
    auto it = mTabs.find(id);
    return (it != mTabs.end()) ? it->second : nullptr;
}

unsigned int LLEmbeddedBrowser::create(const std::string& url, unsigned int width, unsigned int height)
{
    width = llmin(width, mMaxWidth);
    height = llmin(height, mMaxHeight);

    LLMutexLock lock(&mTabsMutex);
    unsigned int id = mNextTabId++;
    mTabs[id] = std::make_shared<LLEmbeddedBrowserTab>(this, id, url, width, height);
    return id;
}

void LLEmbeddedBrowser::setMaxDimensions(unsigned int max_width, unsigned int max_height)
{
    mMaxWidth = max_width;
    mMaxHeight = max_height;
}

void LLEmbeddedBrowser::destroy(unsigned int id)
{
    LLMutexLock lock(&mTabsMutex);
    mTabs.erase(id);
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
        mBrowser->update(mBrowserId);

        unsigned long long pixels = (unsigned long long)mBrowser->getWidth(mBrowserId) * (unsigned long long)mBrowser->getHeight(mBrowserId);

        unsigned int fps = max_fps;
        if (pixels > 0)
        {
            unsigned long long computed_fps = llclamp(budget_pixels_per_sec / pixels, (unsigned long long)min_fps, (unsigned long long)max_fps);
            fps = (unsigned int)computed_fps;
        }

        ms_sleep(1000 / fps);
    }
}
