/**
 * @file llvoicewebrtc_fake_llwebrtc.h
 * @brief A scriptable stand-in for llwebrtc, for testing llvoicewebrtc.cpp.
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

// Defines llwebrtc::init(), terminate(), getDeviceInterface(),
// newPeerConnection() and freePeerConnection(), in place of the real
// library.  Every call is recorded, and tests play the part of llwebrtc's
// threads by firing the observer callbacks themselves (offer(),
// establishAudio(), receive(), ...).  The voice client queues those
// callbacks onto its work queue exactly as it does for the real library.

#ifndef LLVOICEWEBRTC_FAKE_LLWEBRTC_H
#define LLVOICEWEBRTC_FAKE_LLWEBRTC_H

#include "llwebrtc.h"

#include <memory>
#include <string>
#include <vector>

namespace fake_llwebrtc
{

class FakeAudio final : public llwebrtc::LLWebRTCAudioInterface
{
public:
    void setMute(bool mute) override;
    void setReceiveVolume(float volume) override { mReceiveVolume = volume; }
    void setSendVolume(float volume) override { mSendVolume = volume; }

    bool              mMuted{ false };
    std::vector<bool> mMuteCalls;
    float             mReceiveVolume{ -1.f };
    float             mSendVolume{ -1.f };
};

class FakeData final : public llwebrtc::LLWebRTCDataInterface
{
public:
    void sendData(const std::string& data, bool binary) override;
    void setDataObserver(llwebrtc::LLWebRTCDataObserver* observer) override { mObserver = observer; }
    void unsetDataObserver(llwebrtc::LLWebRTCDataObserver* observer) override;

    llwebrtc::LLWebRTCDataObserver* mObserver{ nullptr };
    std::vector<std::string>        mSent;
};

class FakePeerConnection final : public llwebrtc::LLWebRTCPeerConnectionInterface
{
public:
    bool initializeConnection(const InitOptions& options) override;
    bool shutdownConnection() override;
    void setSignalingObserver(llwebrtc::LLWebRTCSignalingObserver* observer) override { mObserver = observer; }
    void unsetSignalingObserver(llwebrtc::LLWebRTCSignalingObserver* observer) override;
    void AnswerAvailable(const std::string& sdp) override { mAnswers.push_back(sdp); }
    void gatherConnectionStats() override { ++mStatsRequests; }

    //------------------------------------------------------------------
    // What llwebrtc would report, delivered to the signaling observer.
    void offer(const std::string& sdp = "v=0 fake offer");
    void establishAudio();
    void dataChannelReady();
    void receive(const std::string& data, bool binary = false); // from the server, over the data channel
    void renegotiationNeeded();
    void iceCandidate(const std::string& candidate, const std::string& sdp_mid = "0", int mline_index = 0);
    void iceGatheringState(llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState state);
    void stats(const llwebrtc::LLWebRTCStatsMap& stats);
    void closed();

    //------------------------------------------------------------------
    // Settings
    bool mInitializeResult{ true };
    // Report the connection closed as soon as shutdownConnection() is
    // called, as the real library does a moment later.  Turn off to hold a
    // connection open, e.g. to see a drain time out.
    bool mCloseOnShutdown{ true };

    //------------------------------------------------------------------
    // What the voice client did
    llwebrtc::LLWebRTCSignalingObserver* mObserver{ nullptr };
    int                                  mInitializeCalls{ 0 };
    InitOptions                          mOptions;
    std::vector<std::string>             mAnswers;
    int                                  mShutdownCalls{ 0 };
    int                                  mStatsRequests{ 0 };
    bool                                 mFreed{ false };
    bool                                 mObserverSetWhenFreed{ false };

    FakeAudio mAudio;
    FakeData  mData;
};

class FakeDevices final : public llwebrtc::LLWebRTCDeviceInterface
{
public:
    void setAudioConfig(AudioConfig config) override;
    void refreshDevices() override { ++mRefreshes; }
    void setCaptureDevice(const std::string& id) override { mCaptureDevice = id; ++mCaptureDeviceSets; }
    void setRenderDevice(const std::string& id) override { mRenderDevice = id; ++mRenderDeviceSets; }
    void setVoiceEnabled(bool enable) override { mVoiceEnabled = enable; }
    void setStopCaptureOnMute(bool stop) override { mStopCaptureOnMute = stop; }
    void setDevicesObserver(llwebrtc::LLWebRTCDevicesObserver* observer) override { mObserver = observer; }
    void unsetDevicesObserver(llwebrtc::LLWebRTCDevicesObserver* observer) override;
    void setTuningMode(bool enable) override { mTuningMode = enable; }
    float getTuningAudioLevel() override { return mTuningAudioLevel; }
    float getPeerConnectionAudioLevel() override { return mPeerConnectionAudioLevel; }
    void setMicGain(float gain) override { mMicGain = gain; }
    void setTuningMicGain(float gain) override { mTuningMicGain = gain; }
    void setMute(bool mute, int delay_ms) override;

    llwebrtc::LLWebRTCDevicesObserver* mObserver{ nullptr };
    std::vector<AudioConfig>           mAudioConfigs;
    int                                mRefreshes{ 0 };
    std::string                        mCaptureDevice;
    std::string                        mRenderDevice;
    int                                mCaptureDeviceSets{ 0 };
    int                                mRenderDeviceSets{ 0 };
    bool                               mVoiceEnabled{ false };
    bool                               mStopCaptureOnMute{ false };
    bool                               mTuningMode{ false };
    float                              mTuningAudioLevel{ 0.f };
    float                              mPeerConnectionAudioLevel{ 0.f };
    float                              mMicGain{ -1.f };
    float                              mTuningMicGain{ -1.f };
    struct MuteCall
    {
        bool mMute;
        int  mDelayMs;
    };
    std::vector<MuteCall> mMuteCalls;
};

// The library's global state.
class FakeLLWebRTC
{
public:
    static FakeLLWebRTC& get();
    void                 reset();

    // Peer connections made since reset(), in order.  They're kept after
    // being freed, so tests can check how they were released.
    std::vector<std::unique_ptr<FakePeerConnection>> mConnections;
    FakePeerConnection*                              connection(size_t index) const;
    FakePeerConnection*                              latest() const;
    size_t                                           liveConnections() const;

    std::unique_ptr<FakeDevices>   mDevices;
    llwebrtc::LLWebRTCLogCallback* mLogCallback{ nullptr };
    bool                           mInitialized{ false };
    int                            mInitCalls{ 0 };
    int                            mTerminateCalls{ 0 };
    // Live connections when terminate() was called, and how many of those
    // the voice client hadn't yet asked to shut down.
    size_t                         mLiveConnectionsAtTerminate{ 0 };
    size_t                         mUnclosedConnectionsAtTerminate{ 0 };
    // freePeerConnection() calls made after terminate().
    int                            mFreeCallsAfterTerminate{ 0 };

private:
    FakeLLWebRTC() = default;
};

} // namespace fake_llwebrtc

#endif // LLVOICEWEBRTC_FAKE_LLWEBRTC_H
