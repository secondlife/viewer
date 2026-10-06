/**
 * @file llstreamingaudio_libvlc.cpp
 * @brief LLStreamingAudio_LibVLC implementation -- see llstreamingaudio_libvlc.h.
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
#include "linden_common.h"

#include "llstreamingaudio_libvlc.h"

#include "llembeddedbrowser.h"
#include "cefshm_protocol.h"
#include <shmframe/llshmframe.h>

using namespace cefshm_demo;

namespace {
    // How long ClaimingControl/AwaitingSlotReply will keep polling (one non-blocking
    // check per update() tick, never a sleep) before giving up this attempt and falling
    // back to NotConnected -- mirrors LLEmbeddedBrowserTab::connectToProducer()'s own
    // kControlClaimTimeout/kSlotRequestTimeout, just paced by frame ticks instead of
    // std::this_thread::sleep_for(), since this runs on the main thread (see update()'s
    // own comment on why it must never block).
    constexpr auto kClaimControlTimeout = std::chrono::seconds(3);
    constexpr auto kSlotReplyTimeout    = std::chrono::seconds(2);

    // How long NotConnected waits before trying again after a failed attempt (no
    // producer reachable, slot unavailable, or a reply never arrived) -- avoids
    // hammering the control channel every single frame while SLVlcProducer is down or
    // relaunching.
    constexpr auto kReconnectBackoff = std::chrono::seconds(1);
}

LLStreamingAudio_LibVLC::LLStreamingAudio_LibVLC()
    : mGain(1.f)
{
    // No IPC here -- SLVlcProducer may not even have finished launching this early in
    // startup (LLEmbeddedBrowser::init() already kicked it off, but asynchronously --
    // see llappviewer.cpp's init ordering). The first real connect attempt happens on
    // the first update() tick, harmless since no parcel has been seen yet at the login
    // screen anyway.
}

LLStreamingAudio_LibVLC::~LLStreamingAudio_LibVLC()
{
    // Best-effort only -- LLEmbeddedBrowser::reset() runs before this destructor (see
    // llappviewer.cpp's cleanup ordering), so SLVlcProducer may already be gone by now;
    // a failed send() here is a silent, harmless no-op (LLSubscriber::send() just
    // returns false once the producer is gone). Deliberately NOT kShutdownProducer --
    // that would tear down the whole shared SLVlcProducer process, which prim-media
    // LibVLC tabs may still be using.
    if (mState == State::Connected && mSub && !mStreamUrl.empty())
    {
        std::uint8_t action = 2; // Stop
        mSub->send(kSetPlaybackAction, &action, 1);
    }
}

void LLStreamingAudio_LibVLC::start(const std::string& url)
{
    if (url.empty())
    {
        LL_INFOS() << "setting parcel audio stream to NULL" << LL_ENDL;
        if (mState == State::Connected && mSub && !mStreamUrl.empty())
        {
            std::uint8_t action = 2; // Stop
            mSub->send(kSetPlaybackAction, &action, 1);
        }
        mURL.clear();
        mStreamUrl.clear();
        mPauseRequested = false;
        mCachedWirePlaying = false;
        return;
    }

    mURL = url; // keep the original url here for comparison purposes (matches getURL() callers' expectations)

    std::string stream_url = url;
    LLStringUtil::trim(stream_url);
    size_t pos = stream_url.find(' ');
    if (pos != std::string::npos)
    {
        // Some parcel owners label their stream this way, e.g. "http://example.com/stream My Station"
        // -- ignore the label, matches the pre-existing in-process implementation's own parsing.
        stream_url = stream_url.substr(0, pos);
    }
    mStreamUrl = stream_url;
    mPauseRequested = false;
    // Not reset to false here -- a genuinely fresh kSetUrl should report "not yet known
    // to be playing" until the producer's own kEventPlaybackStateChanged confirms it, but
    // leaving it at whatever it was avoids a one-tick "stopped" flicker if this is really
    // just a reconnect replaying the same URL. The producer always starts playback
    // immediately on kSetUrl, so this corrects itself within a tick or two regardless.

    LL_INFOS() << "Starting parcel audio stream: " << mStreamUrl << LL_ENDL;

    if (mState == State::Connected && mSub)
    {
        mSub->send_text(kSetUrl, mStreamUrl);
    }
    // else: nothing connected yet -- sendReplayBurst() sends this once update() finishes
    // connecting, same as LLEmbeddedBrowserTab's own mCurrentUrl/mVolume replay.
}

void LLStreamingAudio_LibVLC::stop()
{
    LL_INFOS() << "Stopping parcel audio stream." << LL_ENDL;
    if (mState == State::Connected && mSub && !mStreamUrl.empty())
    {
        std::uint8_t action = 2; // Stop
        mSub->send(kSetPlaybackAction, &action, 1);
    }
    mURL.clear();
    mStreamUrl.clear();
    mPauseRequested = false;
    mCachedWirePlaying = false;
}

void LLStreamingAudio_LibVLC::pause(int pause)
{
    mPauseRequested = (pause != 0);
    if (mState == State::Connected && mSub && !mStreamUrl.empty())
    {
        LL_INFOS() << (mPauseRequested ? "Pausing" : "Unpausing") << " parcel audio stream." << LL_ENDL;
        std::uint8_t action = mPauseRequested ? 1 : 0; // Pause : Play
        mSub->send(kSetPlaybackAction, &action, 1);
    }
}

void LLStreamingAudio_LibVLC::sendReplayBurst()
{
    std::uint8_t vol_payload[1];
    vol_payload[0] = static_cast<std::uint8_t>(llclamp(mGain, 0.f, 1.f) * 100.0f + 0.5f);
    mSub->send(kSetVolume, vol_payload, 1);

    if (!mStreamUrl.empty())
    {
        mSub->send_text(kSetUrl, mStreamUrl);
        if (mPauseRequested)
        {
            std::uint8_t action = 1; // Pause
            mSub->send(kSetPlaybackAction, &action, 1);
        }
    }
}

void LLStreamingAudio_LibVLC::pumpInboundEvents()
{
    LLShmCommand cmd;
    while (mSub->receive(cmd))
    {
        if (cmd.type == kEventPlaybackStateChanged && !cmd.data.empty())
        {
            mCachedWirePlaying = cmd.data[0] != 0;
        }
        else if (cmd.type == kEventVersionInfo)
        {
            // Same opcode SLVlcProducer sends to any prim-media LibVLC tab (see
            // allocate_slot() in llvlcproducer.cpp) -- this is the only other consumer
            // of it, since this class doesn't go through LLEmbeddedBrowserTab at all.
            // Whichever one connects first in a session populates the About floater's
            // LIBVLC_VERSION field.
            LLEmbeddedBrowser::instance().setVlcProducerVersion(std::string(cmd.text()));
        }
        // kEventLoadStart/kEventLoadEnd: nothing consumes these for parcel audio today
        // (no "now playing"/buffering UI -- see LLStreamingAudioInterface's own
        // interface, which has no hook for either) -- silently drained so the command
        // ring never backs up.
    }
}

void LLStreamingAudio_LibVLC::update()
{
    const auto now = std::chrono::steady_clock::now();

    switch (mState)
    {
    case State::NotConnected:
    {
        if (now < mNextAttemptTime) return;

        mCtrl = LLSubscriber::open(kVlcControlChannelName);
        if (!mCtrl->connected())
        {
            // No SLVlcProducer reachable at all -- same meaning as
            // LLEmbeddedBrowserTab::connectToProducer()'s own identical branch.
            mCtrl.reset();
            LLEmbeddedBrowser::instance().maybeRelaunchProducer(LLEmbeddedBrowserBackend::LibVlc);
            mNextAttemptTime = now + kReconnectBackoff;
            return;
        }
        mStepDeadline = now + kClaimControlTimeout;
        mState = State::ClaimingControl;
        return; // try the claim itself on the next tick, not blocking this one
    }
    case State::ClaimingControl:
    {
        if (!mCtrl || !mCtrl->connected())
        {
            mCtrl.reset();
            mState = State::NotConnected;
            mNextAttemptTime = now + kReconnectBackoff;
            return;
        }
        if (!mCtrl->owns_command_channel())
        {
            if (now >= mStepDeadline)
            {
                mCtrl.reset();
                mState = State::NotConnected;
                mNextAttemptTime = now + kReconnectBackoff;
            }
            return; // racing another claimant -- try again next tick
        }

        std::uint8_t payload[11];
        // isUI/maxWidth/maxHeight are irrelevant here -- SLVlcProducer overrides an
        // audio-only slot's geometry to 1x1 regardless of what's requested (see
        // llvlcproducer.cpp's own kRequestSlot handler), and isUI has no meaning
        // outside CEF's two cookie-store contexts.
        const std::uint32_t len = pack_request_slot(payload, /*isUI*/ true, 1, 1,
            static_cast<std::uint8_t>(LLEmbeddedBrowserBackend::LibVlc), /*audioOnly*/ true);
        if (!mCtrl->send(kRequestSlot, payload, len, 0, &mPendingReqId))
        {
            return; // transient (outbound ring full) -- retry next tick
        }
        mStepDeadline = now + kSlotReplyTimeout;
        mState = State::AwaitingSlotReply;
        return;
    }
    case State::AwaitingSlotReply:
    {
        if (!mCtrl || !mCtrl->connected())
        {
            mCtrl.reset();
            mState = State::NotConnected;
            mNextAttemptTime = now + kReconnectBackoff;
            return;
        }

        LLShmCommand reply;
        while (mCtrl->receive(reply))
        {
            if (reply.reply_to != mPendingReqId) continue;

            if (reply.type == kSlotAssigned)
            {
                std::uint32_t index = 0;
                if (!unpack_u32(reply.data.data(), reply.data.size(), index))
                {
                    break; // malformed -- fall through to the deadline/backoff path below
                }
                mCtrl.reset(); // release the control claim for the next requester

                auto sub = LLSubscriber::open(kVlcChannelPrefix + std::to_string(index));
                if (!sub->connected() || !sub->owns_command_channel())
                {
                    mState = State::NotConnected;
                    mNextAttemptTime = now + kReconnectBackoff;
                    return;
                }

                mSub = std::move(sub);
                mState = State::Connected;
                sendReplayBurst();
                // A real connection just succeeded, so any earlier relaunch attempts are
                // no longer relevant -- give a later, unrelated crash its own fresh
                // budget (shared with any open prim-media LibVLC tabs).
                LLEmbeddedBrowser::instance().resetRelaunchAttempts(LLEmbeddedBrowserBackend::LibVlc);
                return;
            }

            // kSlotUnavailable (producer's 32-slot ceiling is full) or anything else
            // unexpected -- give up this attempt.
            mCtrl.reset();
            mState = State::NotConnected;
            mNextAttemptTime = now + kReconnectBackoff;
            return;
        }

        if (now >= mStepDeadline)
        {
            mCtrl.reset();
            mState = State::NotConnected;
            mNextAttemptTime = now + kReconnectBackoff;
        }
        return;
    }
    case State::Connected:
    {
        // poll() is what actually detects a lost/restarted producer when nothing else
        // is calling read_latest() to do it implicitly (see llshmframe.h's own comment
        // on poll() -- a command-only caller must call it directly). A false return
        // here means SLVlcProducer is genuinely gone, not a transient hiccup.
        if (!mSub || !mSub->poll())
        {
            mSub.reset();
            mState = State::NotConnected;
            mNextAttemptTime = now; // a real disconnect, not a race -- retry right away
            return;
        }
        pumpInboundEvents();
        return;
    }
    }
}

int LLStreamingAudio_LibVLC::isPlaying()
{
    if (mStreamUrl.empty()) return 0; // stopped -- no stream configured at all
    if (mCachedWirePlaying) return 1; // active and playing
    return mPauseRequested ? 2 : 0;   // paused, or not yet confirmed playing
}

void LLStreamingAudio_LibVLC::setGain(F32 vol)
{
    mGain = llclamp(vol, 0.f, 1.f);
    if (mState == State::Connected && mSub)
    {
        std::uint8_t payload[1];
        payload[0] = static_cast<std::uint8_t>(mGain * 100.0f + 0.5f);
        mSub->send(kSetVolume, payload, 1);
    }
}

F32 LLStreamingAudio_LibVLC::getGain()
{
    return mGain;
}

std::string LLStreamingAudio_LibVLC::getURL()
{
    return mURL;
}
