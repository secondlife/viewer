/**
 * @file llvoicewebrtc_fake_llwebrtc.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llvoicewebrtc_fake_llwebrtc.h"

namespace fake_llwebrtc
{

void FakeAudio::setMute(bool mute)
{
    mMuted = mute;
    mMuteCalls.push_back(mute);
}

void FakeData::sendData(const std::string& data, bool binary)
{
    mSent.push_back(data);
}

void FakeData::unsetDataObserver(llwebrtc::LLWebRTCDataObserver* observer)
{
    if (mObserver == observer)
    {
        mObserver = nullptr;
    }
}

bool FakePeerConnection::initializeConnection(const InitOptions& options)
{
    ++mInitializeCalls;
    mOptions = options;
    return mInitializeResult;
}

bool FakePeerConnection::shutdownConnection()
{
    ++mShutdownCalls;
    if (mCloseOnShutdown)
    {
        closed();
    }
    return true;
}

void FakePeerConnection::unsetSignalingObserver(llwebrtc::LLWebRTCSignalingObserver* observer)
{
    if (mObserver == observer)
    {
        mObserver = nullptr;
    }
}

void FakePeerConnection::offer(const std::string& sdp)
{
    if (mObserver)
    {
        mObserver->OnOfferAvailable(sdp);
    }
}

void FakePeerConnection::establishAudio()
{
    if (mObserver)
    {
        mObserver->OnAudioEstablished(&mAudio);
    }
}

void FakePeerConnection::dataChannelReady()
{
    if (mObserver)
    {
        mObserver->OnDataChannelReady(&mData);
    }
}

void FakePeerConnection::receive(const std::string& data, bool binary)
{
    if (mData.mObserver)
    {
        mData.mObserver->OnDataReceived(data, binary);
    }
}

void FakePeerConnection::renegotiationNeeded()
{
    if (mObserver)
    {
        mObserver->OnRenegotiationNeeded();
    }
}

void FakePeerConnection::iceCandidate(const std::string& candidate, const std::string& sdp_mid, int mline_index)
{
    if (mObserver)
    {
        llwebrtc::LLWebRTCIceCandidate ice_candidate;
        ice_candidate.mCandidate  = candidate;
        ice_candidate.mSdpMid     = sdp_mid;
        ice_candidate.mMLineIndex = mline_index;
        mObserver->OnIceCandidate(ice_candidate);
    }
}

void FakePeerConnection::iceGatheringState(llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState state)
{
    if (mObserver)
    {
        mObserver->OnIceGatheringState(state);
    }
}

void FakePeerConnection::stats(const llwebrtc::LLWebRTCStatsMap& stats)
{
    if (mObserver)
    {
        mObserver->OnStatsDelivered(stats);
    }
}

void FakePeerConnection::closed()
{
    if (mObserver)
    {
        mObserver->OnPeerConnectionClosed();
    }
}

void FakeDevices::setAudioConfig(AudioConfig config)
{
    mAudioConfigs.push_back(config);
}

void FakeDevices::unsetDevicesObserver(llwebrtc::LLWebRTCDevicesObserver* observer)
{
    if (mObserver == observer)
    {
        mObserver = nullptr;
    }
}

void FakeDevices::setMute(bool mute, int delay_ms)
{
    mMuteCalls.push_back({ mute, delay_ms });
}

FakeLLWebRTC& FakeLLWebRTC::get()
{
    static FakeLLWebRTC sInstance;
    return sInstance;
}

void FakeLLWebRTC::reset()
{
    mConnections.clear();
    mDevices.reset();
    mLogCallback                = nullptr;
    mInitialized                = false;
    mInitCalls                  = 0;
    mTerminateCalls             = 0;
    mLiveConnectionsAtTerminate     = 0;
    mUnclosedConnectionsAtTerminate = 0;
    mFreeCallsAfterTerminate        = 0;
}

FakePeerConnection* FakeLLWebRTC::connection(size_t index) const
{
    return index < mConnections.size() ? mConnections[index].get() : nullptr;
}

FakePeerConnection* FakeLLWebRTC::latest() const
{
    return mConnections.empty() ? nullptr : mConnections.back().get();
}

size_t FakeLLWebRTC::liveConnections() const
{
    size_t live = 0;
    for (auto& connection : mConnections)
    {
        if (!connection->mFreed)
        {
            ++live;
        }
    }
    return live;
}

} // namespace fake_llwebrtc

//----------------------------------------------------------------------------
// The llwebrtc entry points.

namespace llwebrtc
{
using fake_llwebrtc::FakeLLWebRTC;

void init(LLWebRTCLogCallback* logSink)
{
    FakeLLWebRTC& webrtc = FakeLLWebRTC::get();
    ++webrtc.mInitCalls;
    webrtc.mInitialized = true;
    webrtc.mLogCallback = logSink;
    // A fresh device interface each init, as the real library makes.
    webrtc.mDevices = std::make_unique<fake_llwebrtc::FakeDevices>();
}

void terminate()
{
    FakeLLWebRTC& webrtc = FakeLLWebRTC::get();
    ++webrtc.mTerminateCalls;
    webrtc.mLiveConnectionsAtTerminate = webrtc.liveConnections();
    for (auto& connection : webrtc.mConnections)
    {
        if (!connection->mFreed && connection->mShutdownCalls == 0)
        {
            ++webrtc.mUnclosedConnectionsAtTerminate;
        }
    }
    // Like the real library, close whatever is still open and drop the
    // observers; peer connections are unusable afterwards.
    for (auto& connection : webrtc.mConnections)
    {
        if (!connection->mFreed)
        {
            connection->closed();
            connection->mObserver = nullptr;
            connection->mFreed    = true;
        }
    }
    webrtc.mInitialized = false;
    webrtc.mLogCallback = nullptr;
}

LLWebRTCDeviceInterface* getDeviceInterface()
{
    FakeLLWebRTC& webrtc = FakeLLWebRTC::get();
    return webrtc.mInitialized ? webrtc.mDevices.get() : nullptr;
}

LLWebRTCPeerConnectionInterface* newPeerConnection()
{
    FakeLLWebRTC& webrtc = FakeLLWebRTC::get();
    if (!webrtc.mInitialized)
    {
        return nullptr;
    }
    webrtc.mConnections.push_back(std::make_unique<fake_llwebrtc::FakePeerConnection>());
    return webrtc.mConnections.back().get();
}

void freePeerConnection(LLWebRTCPeerConnectionInterface* connection)
{
    FakeLLWebRTC& webrtc = FakeLLWebRTC::get();
    if (webrtc.mTerminateCalls > 0 && !webrtc.mInitialized)
    {
        ++webrtc.mFreeCallsAfterTerminate;
    }
    for (auto& candidate : webrtc.mConnections)
    {
        if (candidate.get() == connection)
        {
            candidate->mObserverSetWhenFreed = candidate->mObserver != nullptr;
            candidate->mFreed                = true;
        }
    }
}

} // namespace llwebrtc
