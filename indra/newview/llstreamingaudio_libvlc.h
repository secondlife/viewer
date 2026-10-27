/**
 * @file llstreamingaudio_libvlc.h
 * @brief Definition of LLStreamingAudio_LibVLC, a streaming-audio implementation for parcel
 *        audio/music backed by LibVLC running in SLVlcProducer -- reached only over IPC
 *        (llshmframe), the same transport and process prim/RTSP media already uses, rather
 *        than linking libvlc directly into this process. See doc/Embedded_Browser.md's own
 *        note on why: the vendored libvlc package is GPL v2, this project is LGPL v2.1, and
 *        keeping every use of libvlc confined to SLVlcProducer (reached only via IPC) is
 *        what the 2026-09-30 SLCefProducer/SLVlcProducer split was for in the first place --
 *        this class is the last place that split's goal hadn't yet reached ("Phase 2").
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

#ifndef LL_STREAMINGAUDIO_LIBVLC_H
#define LL_STREAMINGAUDIO_LIBVLC_H

#include "stdtypes.h" // from llcommon

#include "llstreamingaudio.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class LLStreamedAudioSource;
class LLSubscriber;

class LLStreamingAudio_LibVLC : public LLStreamingAudioInterface
{
public:
    LLStreamingAudio_LibVLC();
    /*virtual*/ ~LLStreamingAudio_LibVLC();

    /*virtual*/ void start(const std::string& url);
    /*virtual*/ void stop();
    /*virtual*/ void pause(int pause);
    /*virtual*/ void update();
    /*virtual*/ int isPlaying();
    /*virtual*/ void setGain(F32 vol);
    /*virtual*/ F32 getGain();
    /*virtual*/ std::string getURL();

private:
    // Explicitly non-blocking, ticked once per update() call (every frame, on the main/
    // render thread -- see update()'s own comment for why this can never sleep/block the
    // way LLEmbeddedBrowserTab::connectToProducer() does). One state-machine step happens
    // per tick; a step that isn't ready yet just waits for the next one.
    enum class State
    {
        NotConnected,     // nothing attempted yet, or the last attempt/connection failed
        ClaimingControl,  // control channel open; waiting to own its command channel
        AwaitingSlotReply, // kRequestSlot sent; waiting for kSlotAssigned/kSlotUnavailable
        Connected,        // per-view channel open and owned
    };

    // Drains any events the producer sent since the last tick (today, just
    // kEventPlaybackStateChanged -- see mCachedWirePlaying). Called once per tick while
    // Connected.
    void pumpInboundEvents();

    // Sends the full "what this stream should be doing right now" burst to a freshly
    // (re)connected per-view channel: kSetVolume, then kSetUrl (if a URL is set), then
    // kSetPlaybackAction(Pause) (if paused was requested) -- volume before URL because a
    // LibVLC-backed slot's kSetUrl starts playback immediately, matching
    // LLEmbeddedBrowserTab::connectToProducer()'s own replay-on-reconnect ordering.
    // Self-healing: a SLVlcProducer crash/relaunch mid-stream resumes automatically at the
    // same URL/gain/pause-state once update() reconnects, with no caller-visible action
    // needed.
    void sendReplayBurst();

    // Viewer-side playback of the stream (see mAudio): creates it the first time it's
    // possible, along with mAudioThread. Main thread.
    void ensureAudioStream();
    // mAudioThread's body, and one pass of it -- drains mAudioSub into mAudio. The second
    // runs with mAudioSubMutex held.
    void audioThreadLoop();
    void pumpAudio();

    State mState = State::NotConnected;
    std::unique_ptr<LLSubscriber> mCtrl; // control-channel claim, held only during the handshake
    std::unique_ptr<LLSubscriber> mSub;  // per-view channel once Connected
    std::uint64_t mPendingReqId = 0;
    std::chrono::steady_clock::time_point mStepDeadline{};    // ClaimingControl/AwaitingSlotReply give-up point
    std::chrono::steady_clock::time_point mNextAttemptTime{}; // backoff before NotConnected tries again

    // Local cache -- authoritative regardless of connection state, replayed on (re)connect
    // (see sendReplayBurst()). getGain()/getURL() are already pure reads of these, same as
    // the pre-IPC implementation; isPlaying() is reconstructed from mCachedWirePlaying/
    // mPauseRequested below.
    std::string mURL;       // raw, exactly as passed to start() -- what getURL() returns
    std::string mStreamUrl; // trimmed/label-stripped (see start()) -- what's actually sent as kSetUrl
    F32 mGain;
    bool mPauseRequested = false;
    // Last kEventPlaybackStateChanged payload -- deliberately NOT reset on a transient
    // disconnect, so isPlaying() keeps reporting the last known state (rather than
    // snapping to "stopped") through a brief SLVlcProducer crash/relaunch window, matching
    // sendReplayBurst()'s own self-healing intent.
    bool mCachedWirePlaying = false;

    // Viewer-side playback (EmbeddedBrowserAudioViaViewer): SLVlcProducer hands this
    // stream's decoded audio over on its own audio channel (see kSlotFlagAudioCapture in
    // cefshm_protocol.h) and it plays through OpenAL here, non-positionally -- instead of
    // libvlc playing it itself. Null if that isn't possible (no audio engine, or one that
    // can't do caller-fed streams), in which case capture is never requested.
    std::shared_ptr<LLStreamedAudioSource> mAudio;

    // Drains the audio channel on its own thread rather than in update(): update() only
    // runs once per frame, so any frame hitch longer than the stream's prebuffer would be
    // an audible dropout -- the same reason LLEmbeddedBrowserTab drains on its own thread.
    // The main-thread state machine still owns connecting; it just hands this thread the
    // audio channel (mAudioSub) once a connection grants capture, and takes it back on
    // disconnect.
    std::thread mAudioThread;
    std::atomic<bool> mAudioThreadQuit{ false };
    std::mutex mAudioSubMutex;           // guards everything below
    std::unique_ptr<LLSubscriber> mAudioSub;
    std::string mAudioChannelName;
    std::chrono::steady_clock::time_point mNextAudioClaimAttempt{};
    std::chrono::steady_clock::time_point mAudioClaimDeadline{};
    bool mAudioClaimLogged = false;
    bool mAudioClaimWarned = false; // the "still can't be claimed" warning -- separate, so a late success still logs
    std::vector<float> mAudioScratch;    // aligned copy of one packet's samples
};

#endif // LL_STREAMINGAUDIO_LIBVLC_H
