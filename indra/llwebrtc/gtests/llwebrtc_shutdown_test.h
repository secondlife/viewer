/**
 * @file llwebrtc_shutdown_test.h
 * @brief Fixture shared by the llwebrtc::terminate() tests.
 *
 * Used by llwebrtc_shutdown_test.cpp and, when the build is configured
 * with LL_TESTS_AUDIO_DEVICES=ON, llwebrtc_shutdown_audio_device_test.cpp.
 * Both are linked into UNIT_TEST_llwebrtc_shutdown, so everything here is in
 * a named namespace: gtest requires every test in a suite to use the same
 * fixture class.
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

#ifndef LLWEBRTC_SHUTDOWN_TEST_H
#define LLWEBRTC_SHUTDOWN_TEST_H

#include "llwebrtc_test_support.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace llwebrtc_shutdown_test
{

using llwebrtc::LLWebRTCPeerConnectionInterface;
using llwebrtc_test::RecordingDataObserver;
using llwebrtc_test::RecordingDevicesObserver;
using llwebrtc_test::RecordingLogCallback;
using llwebrtc_test::RecordingSignalingObserver;
using llwebrtc_test::TestServer;
using testing::IsEmpty;

// How long to keep watching for stray callbacks after terminate() returns.
inline constexpr std::chrono::milliseconds LATE_CALLBACK_GRACE{ 250 };

// llwebrtc logs this when terminate() blows its budget and abandons the
// shutdown thread.
inline constexpr const char* TERMINATE_TIMEOUT_LOG = "timed out waiting for WebRTC thread shutdown";

// Anything handed to llwebrtc as a callback target is kept alive for the
// rest of the process.  If llwebrtc does call back after terminate() -- the
// bug these tests look for -- it should be reported as a test failure, not
// turn into a use-after-free that takes down the whole run.
template <typename T, typename... Args>
T* makeImmortal(Args&&... args)
{
    static auto* sKeepAlive = new std::vector<std::shared_ptr<void>>;
    auto object = std::make_shared<T>(std::forward<Args>(args)...);
    sKeepAlive->push_back(object);
    return object.get();
}

// Collects every callback that arrives after it has been closed.
class LateCallGate
{
public:
    void close() { mClosed = true; }

    void record(const char* callback)
    {
        if (mClosed)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mLateCalls.emplace_back(callback);
        }
    }

    std::vector<std::string> lateCalls() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mLateCalls;
    }

private:
    std::atomic<bool>        mClosed{ false };
    mutable std::mutex       mMutex;
    std::vector<std::string> mLateCalls;
};

class GatedLogCallback : public RecordingLogCallback
{
public:
    explicit GatedLogCallback(LateCallGate& gate) : mGate(gate) {}

    void LogMessage(LogLevel level, const std::string& message) override
    {
        mGate.record("LogMessage");
        RecordingLogCallback::LogMessage(level, message);
    }

private:
    LateCallGate& mGate;
};

class GatedDevicesObserver : public RecordingDevicesObserver
{
public:
    explicit GatedDevicesObserver(LateCallGate& gate) : mGate(gate) {}

    void OnDevicesChanged(const llwebrtc::LLWebRTCVoiceDeviceList& render_devices,
                          const llwebrtc::LLWebRTCVoiceDeviceList& capture_devices) override
    {
        mGate.record("OnDevicesChanged");
        RecordingDevicesObserver::OnDevicesChanged(render_devices, capture_devices);
    }

private:
    LateCallGate& mGate;
};

class GatedSignalingObserver : public RecordingSignalingObserver
{
public:
    explicit GatedSignalingObserver(LateCallGate& gate) : mGate(gate) {}

    void OnIceGatheringState(EIceGatheringState state) override
    {
        mGate.record("OnIceGatheringState");
        RecordingSignalingObserver::OnIceGatheringState(state);
    }
    void OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate& candidate) override
    {
        mGate.record("OnIceCandidate");
        RecordingSignalingObserver::OnIceCandidate(candidate);
    }
    void OnOfferAvailable(const std::string& sdp) override
    {
        mGate.record("OnOfferAvailable");
        RecordingSignalingObserver::OnOfferAvailable(sdp);
    }
    void OnRenegotiationNeeded() override
    {
        mGate.record("OnRenegotiationNeeded");
        RecordingSignalingObserver::OnRenegotiationNeeded();
    }
    void OnPeerConnectionClosed() override
    {
        mGate.record("OnPeerConnectionClosed");
        RecordingSignalingObserver::OnPeerConnectionClosed();
    }
    void OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface* audio_interface) override
    {
        mGate.record("OnAudioEstablished");
        RecordingSignalingObserver::OnAudioEstablished(audio_interface);
    }
    void OnDataChannelReady(llwebrtc::LLWebRTCDataInterface* data_interface) override
    {
        mGate.record("OnDataChannelReady");
        RecordingSignalingObserver::OnDataChannelReady(data_interface);
    }
    void OnStatsDelivered(const llwebrtc::LLWebRTCStatsMap& stats_data) override
    {
        mGate.record("OnStatsDelivered");
        RecordingSignalingObserver::OnStatsDelivered(stats_data);
    }

private:
    LateCallGate& mGate;
};

class GatedDataObserver : public RecordingDataObserver
{
public:
    explicit GatedDataObserver(LateCallGate& gate) : mGate(gate) {}

    void OnDataReceived(const std::string& data, bool binary) override
    {
        mGate.record("OnDataReceived");
        RecordingDataObserver::OnDataReceived(data, binary);
    }

private:
    LateCallGate& mGate;
};

struct ShutdownConnection
{
    LLWebRTCPeerConnectionInterface* mConnection{ nullptr };
    GatedSignalingObserver*          mObserver{ nullptr };
    GatedDataObserver*               mDataObserver{ nullptr }; // live connections only
    TestServer*                      mServer{ nullptr };       // live connections only
};

class LLWebRTCShutdownTest : public testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(llwebrtc_test::startNetworking()) << "failed to initialize the socket layer";
        mNetworkingStarted = true;
        start();
    }

    void TearDown() override
    {
        if (mRunning)
        {
            llwebrtc::terminate();
            mRunning = false;
        }
        // Test servers outlive llwebrtc, as the real server does.
        mServers.clear();
        if (mNetworkingStarted)
        {
            llwebrtc_test::stopNetworking();
        }
    }

    // Initializes a fresh llwebrtc instance with its own gate and log sink.
    void start()
    {
        mGate = makeImmortal<LateCallGate>();
        mLog  = makeImmortal<GatedLogCallback>(*mGate);

        llwebrtc::init(mLog);
        mRunning = true;

        mDevices = llwebrtc::getDeviceInterface();
        ASSERT_NE(mDevices, nullptr) << "llwebrtc::init() did not create a device interface";
    }

    // Runs llwebrtc::terminate() and checks everything it promises the viewer.
    void terminateAndVerify()
    {
        ASSERT_TRUE(mRunning);

        const auto started = std::chrono::steady_clock::now();
        llwebrtc::terminate();
        const auto elapsed_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();

        // From here on, any callback is a contract violation.
        mGate->close();
        mRunning = false;
        mDevices = nullptr;

        RecordProperty("terminate_ms", std::to_string(elapsed_ms));

        EXPECT_EQ(llwebrtc::getDeviceInterface(), nullptr) << "terminate() left a device interface behind";
        EXPECT_FALSE(mLog->contains(TERMINATE_TIMEOUT_LOG))
            << "terminate() exceeded its shutdown budget (" << elapsed_ms
            << " ms) and abandoned the webrtc threads";

        std::this_thread::sleep_for(LATE_CALLBACK_GRACE);
        EXPECT_THAT(mGate->lateCalls(), IsEmpty()) << "callbacks reached the viewer after terminate() returned";
    }

    GatedSignalingObserver* newObserver() { return makeImmortal<GatedSignalingObserver>(*mGate); }

    GatedDevicesObserver* newDevicesObserver() { return makeImmortal<GatedDevicesObserver>(*mGate); }

    // Creates a connection with a registered observer, as the viewer's
    // LLVoiceWebRTCConnection constructor does.
    ShutdownConnection openConnection()
    {
        ShutdownConnection connection;
        connection.mObserver   = newObserver();
        connection.mConnection = llwebrtc::newPeerConnection();
        EXPECT_NE(connection.mConnection, nullptr);
        if (connection.mConnection)
        {
            connection.mConnection->setSignalingObserver(connection.mObserver);
        }
        return connection;
    }

    // Opens a connection and waits for its offer, i.e. a connection that has
    // a local description and is gathering ICE candidates on the network
    // thread, just short of needing a server.
    ShutdownConnection openConnectionWithOffer()
    {
        ShutdownConnection connection = openConnection();
        if (connection.mConnection)
        {
            EXPECT_TRUE(connection.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));
            EXPECT_TRUE(connection.mObserver->mOffer.wait().has_value()) << "connection never produced an offer";
        }
        return connection;
    }

    // Opens a connection to its own test server and completes the handshake
    // up to, but not including, applying the server's answer.
    ShutdownConnection openConnectionWithAnswer(std::string& answer)
    {
        ShutdownConnection connection = openConnection();
        mServers.push_back(std::make_unique<TestServer>());
        connection.mServer = mServers.back().get();
        EXPECT_TRUE(llwebrtc_test::negotiateAnswer(connection.mConnection, *connection.mObserver, *connection.mServer, answer));
        return connection;
    }

    // A live call: connected to its own test server, audio established and
    // the data channel open, with a data observer registered.  Check
    // mConnected afterwards.
    ShutdownConnection openLiveConnection()
    {
        ShutdownConnection connection = openConnection();
        mServers.push_back(std::make_unique<TestServer>());
        connection.mServer       = mServers.back().get();
        connection.mDataObserver = makeImmortal<GatedDataObserver>(*mGate);

        testing::AssertionResult connected =
            llwebrtc_test::connectToServer(connection.mConnection, *connection.mObserver, *connection.mServer);
        EXPECT_TRUE(connected);
        mConnected = mConnected && connected;
        if (connected)
        {
            connection.mObserver->mDataInterface.load()->setDataObserver(connection.mDataObserver);
        }
        return connection;
    }

    LateCallGate*                            mGate{ nullptr };
    GatedLogCallback*                        mLog{ nullptr };
    llwebrtc::LLWebRTCDeviceInterface*       mDevices{ nullptr };
    std::vector<std::unique_ptr<TestServer>> mServers;
    bool                                     mRunning{ false };
    bool                                     mNetworkingStarted{ false };
    bool                                     mConnected{ true };
};

} // namespace llwebrtc_shutdown_test

#endif // LLWEBRTC_SHUTDOWN_TEST_H
