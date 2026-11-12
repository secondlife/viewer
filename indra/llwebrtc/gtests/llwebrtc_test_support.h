/**
 * @file llwebrtc_test_support.h
 * @brief Shared fixtures and recording observers for llwebrtc unit tests.
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

#ifndef LL_LLWEBRTC_TEST_SUPPORT_H
#define LL_LLWEBRTC_TEST_SUPPORT_H

#include "llgtest.h"
#include "llwebrtc.h"
#include "llwebrtc_testpeer.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// llwebrtc keeps a single process-wide instance.  The main test executable
// initializes it once for the whole run with a GoogleTest global environment
// (llwebrtc_test_environment.cpp); the shutdown tests instead init and
// terminate it in every test.  All callbacks arrive on webrtc's own threads,
// so everything the observers below record is guarded.

namespace llwebrtc_test
{

// Log sink handed to llwebrtc::init().  It lives for the whole process,
// since webrtc may log right up until terminate() returns.
class RecordingLogCallback : public llwebrtc::LLWebRTCLogCallback
{
public:
    struct Entry
    {
        LogLevel    mLevel;
        std::string mMessage;
    };

    void LogMessage(LogLevel level, const std::string& message) override;

    std::vector<Entry> entries() const;

    // True if any logged message contains 'text'.
    bool contains(const std::string& text) const;

private:
    mutable std::mutex mMutex;
    std::vector<Entry> mEntries;
};

// Process-wide log callback used by the global test environment.
RecordingLogCallback& logCallback();

// Brings up / tears down the platform socket layer (Winsock) that llwebrtc
// expects its host to have initialized.  No-ops off Windows.
bool startNetworking();
void stopNetworking();

class RecordingDevicesObserver : public llwebrtc::LLWebRTCDevicesObserver
{
public:
    void OnDevicesChanged(const llwebrtc::LLWebRTCVoiceDeviceList& render_devices,
                          const llwebrtc::LLWebRTCVoiceDeviceList& capture_devices) override
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mRenderDevices  = render_devices;
            mCaptureDevices = capture_devices;
        }
        mCalls.increment();
    }

    llwebrtc::LLWebRTCVoiceDeviceList renderDevices() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mRenderDevices;
    }

    llwebrtc::LLWebRTCVoiceDeviceList captureDevices() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCaptureDevices;
    }

    llgtest::AsyncCounter mCalls;

private:
    mutable std::mutex                mMutex;
    llwebrtc::LLWebRTCVoiceDeviceList mRenderDevices;
    llwebrtc::LLWebRTCVoiceDeviceList mCaptureDevices;
};

class RecordingSignalingObserver : public llwebrtc::LLWebRTCSignalingObserver
{
public:
    void OnIceGatheringState(EIceGatheringState state) override { mIceGatheringStateChanges.increment(); }
    void OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate& candidate) override { mIceCandidates.increment(); }
    void OnOfferAvailable(const std::string& sdp) override { mOffer.set(sdp); }
    void OnRenegotiationNeeded() override { mRenegotiationNeeded.increment(); }
    void OnPeerConnectionClosed() override { mClosed.increment(); }

    void OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface* audio_interface) override
    {
        mAudioInterface = audio_interface;
        mAudioEstablished.increment();
    }

    void OnDataChannelReady(llwebrtc::LLWebRTCDataInterface* data_interface) override
    {
        mDataInterface = data_interface;
        mDataChannelReady.increment();
    }

    void OnStatsDelivered(const llwebrtc::LLWebRTCStatsMap& stats_data) override
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mLastStats = stats_data;
        }
        mStatsDelivered.increment();
    }

    llwebrtc::LLWebRTCStatsMap lastStats() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLastStats;
    }

    llgtest::AsyncValue<std::string> mOffer;
    llgtest::AsyncCounter            mIceGatheringStateChanges;
    llgtest::AsyncCounter            mIceCandidates;
    llgtest::AsyncCounter            mRenegotiationNeeded;
    llgtest::AsyncCounter            mClosed;
    llgtest::AsyncCounter            mAudioEstablished;
    llgtest::AsyncCounter            mDataChannelReady;
    llgtest::AsyncCounter            mStatsDelivered;

    std::atomic<llwebrtc::LLWebRTCAudioInterface*> mAudioInterface{ nullptr };
    std::atomic<llwebrtc::LLWebRTCDataInterface*>  mDataInterface{ nullptr };

private:
    mutable std::mutex         mMutex;
    llwebrtc::LLWebRTCStatsMap mLastStats;
};

struct ReceivedMessage
{
    std::string mData;
    bool        mBinary{ false };
};

// Records messages received on llwebrtc's data channel.
class RecordingDataObserver : public llwebrtc::LLWebRTCDataObserver
{
public:
    void OnDataReceived(const std::string& data, bool binary) override
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mMessages.push_back({ data, binary });
        }
        mReceived.increment();
    }

    std::vector<ReceivedMessage> messages() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mMessages;
    }

    llgtest::AsyncCounter mReceived;

private:
    mutable std::mutex           mMutex;
    std::vector<ReceivedMessage> mMessages;
};

// Stands in for the Second Life voice server: owns a test peer
// (gtests/testpeer) and records everything it reports.  The test peer is
// destroyed with this object, after which it makes no more callbacks.
class TestServer : public llwebrtc_testpeer::LLWebRTCTestPeerObserver
{
public:
    TestServer();
    ~TestServer();

    TestServer(const TestServer&)            = delete;
    TestServer& operator=(const TestServer&) = delete;

    // Null if the test peer couldn't be created.
    llwebrtc_testpeer::LLWebRTCTestPeer* peer() const { return mPeer; }

    // Waits for the answer to an offer passed to peer()->answerOffer().
    testing::AssertionResult waitForAnswer(std::string& answer);

    // Waits until the server's side of the connection has reached 'state'.
    bool waitForState(EConnectionState state, std::chrono::milliseconds timeout = llgtest::DEFAULT_TIMEOUT)
    {
        return mStates[state].waitFor(1, timeout);
    }

    std::vector<ReceivedMessage> messages() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mMessages;
    }

    llgtest::AsyncCounter mDataChannelOpen;
    llgtest::AsyncCounter mReceived;

    // LLWebRTCTestPeerObserver
    void OnAnswerAvailable(const std::string& sdp) override { mAnswer.set({ true, sdp }); }
    void OnAnswerFailed(const std::string& error) override { mAnswer.set({ false, error }); }
    void OnConnectionStateChange(EConnectionState state) override { mStates[state].increment(); }
    void OnDataChannelOpen() override { mDataChannelOpen.increment(); }
    void OnDataReceived(const std::string& data, bool binary) override
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mMessages.push_back({ data, binary });
        }
        mReceived.increment();
    }

private:
    llwebrtc_testpeer::LLWebRTCTestPeer*            mPeer{ nullptr };
    llgtest::AsyncValue<std::pair<bool, std::string>> mAnswer; // {succeeded, sdp or error}
    llgtest::AsyncCounter                           mStates[CONNECTION_CLOSED + 1];
    mutable std::mutex                              mMutex;
    std::vector<ReceivedMessage>                    mMessages;
};

// Runs an llwebrtc connection through offer and answer with 'server':
// initializeConnection(), wait for the offer, have the server answer it.
// The answer is returned rather than applied, so a test can tamper with it
// before passing it to AnswerAvailable().
testing::AssertionResult negotiateAnswer(llwebrtc::LLWebRTCPeerConnectionInterface* connection,
                                         RecordingSignalingObserver& observer,
                                         TestServer& server,
                                         std::string& answer);

// The whole handshake: negotiateAnswer(), AnswerAvailable(), then wait
// until llwebrtc reports audio and the data channel established, and the
// server sees the data channel open.  ICE candidates aren't forwarded to
// the server; it learns llwebrtc's address from llwebrtc's connectivity
// checks (a peer-reflexive candidate), which keeps the test peer's lifetime
// independent of llwebrtc's callbacks.
testing::AssertionResult connectToServer(llwebrtc::LLWebRTCPeerConnectionInterface* connection,
                                         RecordingSignalingObserver& observer,
                                         TestServer& server);

} // namespace llwebrtc_test

#endif // LL_LLWEBRTC_TEST_SUPPORT_H
