/**
 * @file llwebrtc_shutdown_test.cpp
 * @brief Tests for llwebrtc::terminate() and connection teardown.
 *
 * Every test here runs its own llwebrtc::init() / llwebrtc::terminate(), so
 * these live in their own executable (UNIT_TEST_llwebrtc_shutdown) rather than
 * alongside the tests that share one long-lived instance.
 *
 * What terminate() promises the viewer, and what these tests check:
 *   - Every connection still open is closed, and its observers hear
 *     OnPeerConnectionClosed, before terminate() returns.
 *   - Once terminate() returns, nothing calls back into the viewer: no
 *     signaling, device or log callbacks.  The viewer's observers and log
 *     sink may be destroyed the moment it returns.
 *   - It completes inside its own shutdown budget rather than timing out and
 *     abandoning (leaking) the webrtc threads.
 *
 * The scenarios follow what LLWebRTCVoiceClient::terminate() really does:
 * drainConnections() calls shutdownConnection() on each connection and waits
 * (best effort, with a deadline) for them to close, then llwebrtc::terminate()
 * runs with those connections closed or still closing, still registered, and
 * never freed.
 *
 * Tests that open real audio devices are in
 * llwebrtc_shutdown_audio_device_test.cpp, which is only built into this
 * executable when the build is configured with LL_TESTS_AUDIO_DEVICES=ON.
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

#include "llwebrtc_shutdown_test.h"

#include <algorithm>
#include <condition_variable>

#if defined(_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <filesystem>
#endif

using namespace llwebrtc_shutdown_test;

namespace
{

// llwebrtc logs this from OnConnectionChange() when a connection goes
// 'disconnected' (PeerConnectionState::kDisconnected).
const char* const DISCONNECTED_LOG = "Peer Connection State Change disconnected";

// terminate()'s shutdown budget (WEBRTC_TERMINATE_TIMEOUT in llwebrtc.cpp).
constexpr std::chrono::seconds TERMINATE_BUDGET{ 10 };

// Number of threads in this process, or -1 if this platform has no way to
// count them.
int processThreadCount()
{
#if defined(_WIN32)
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    const DWORD   pid = GetCurrentProcessId();
    int           count = 0;
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry))
    {
        do
        {
            if (entry.th32OwnerProcessID == pid)
            {
                ++count;
            }
            entry.dwSize = sizeof(entry);
        } while (Thread32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return count;
#elif defined(__APPLE__)
    thread_act_array_t     threads = nullptr;
    mach_msg_type_number_t count   = 0;
    if (task_threads(mach_task_self(), &threads, &count) != KERN_SUCCESS)
    {
        return -1;
    }
    for (mach_msg_type_number_t i = 0; i < count; ++i)
    {
        mach_port_deallocate(mach_task_self(), threads[i]);
    }
    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(threads), count * sizeof(thread_act_t));
    return static_cast<int>(count);
#elif defined(__linux__)
    std::error_code ec;
    int             count = 0;
    for (std::filesystem::directory_iterator it("/proc/self/task", ec), end; !ec && it != end; it.increment(ec))
    {
        ++count;
    }
    return ec ? -1 : count;
#else
    return -1;
#endif
}

// Waits for the thread count to come down to 'target' -- threads take a
// moment to exit after they're stopped.  'last' is the final count seen.
bool waitForThreadCountAtMost(int target, std::chrono::milliseconds timeout, int& last)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true)
    {
        last = processThreadCount();
        if (last <= target)
        {
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool waitForLog(const RecordingLogCallback& log, const std::string& text, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!log.contains(text))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return true;
}

// A signaling observer whose OnPeerConnectionClosed() doesn't return until
// release() -- like a viewer callback stuck on a lock.  It's called inline
// on the signaling thread while terminate() closes the connections, so it
// holds up the whole shutdown.
class BlockingCloseObserver : public GatedSignalingObserver
{
public:
    using GatedSignalingObserver::GatedSignalingObserver;

    void OnPeerConnectionClosed() override
    {
        GatedSignalingObserver::OnPeerConnectionClosed();

        // Bounded, so a test that fails before releasing can't hang the run.
        std::unique_lock<std::mutex> lock(mReleaseMutex);
        mReleaseCondition.wait_for(lock, std::chrono::seconds(60), [this] { return mReleased; });
    }

    void release()
    {
        {
            std::lock_guard<std::mutex> lock(mReleaseMutex);
            mReleased = true;
        }
        mReleaseCondition.notify_all();
    }

private:
    std::mutex              mReleaseMutex;
    std::condition_variable mReleaseCondition;
    bool                    mReleased{ false };
};

// For tests that make terminate() run out of budget on purpose.
class LLWebRTCShutdownTimeoutTest : public LLWebRTCShutdownTest
{
protected:
    void TearDown() override
    {
        if (mBlocker)
        {
            mBlocker->release();
        }
        LLWebRTCShutdownTest::TearDown();
    }

    // Runs terminate() with a connection whose observer blocks in
    // OnPeerConnectionClosed(), and a second connection queued behind it,
    // so that terminate() times out and abandons the shutdown to its
    // detached thread with the second connection still open.
    void terminateAndTimeOut()
    {
        mBlocker = makeImmortal<BlockingCloseObserver>(*mGate);
        LLWebRTCPeerConnectionInterface* blocked = llwebrtc::newPeerConnection();
        ASSERT_NE(blocked, nullptr);
        blocked->setSignalingObserver(mBlocker);
        mQueued = openConnection(); // closed after 'blocked'

        const auto started = std::chrono::steady_clock::now();
        llwebrtc::terminate();
        const auto elapsed = std::chrono::steady_clock::now() - started;

        mGate->close();
        mRunning = false;
        mDevices = nullptr;

        EXPECT_TRUE(mLog->contains(TERMINATE_TIMEOUT_LOG)) << "terminate() didn't report timing out";
        EXPECT_GE(elapsed, TERMINATE_BUDGET - std::chrono::milliseconds(500)) << "terminate() returned before its budget ran out";
        EXPECT_LT(elapsed, TERMINATE_BUDGET + std::chrono::seconds(2)) << "terminate() overran its budget";
        EXPECT_EQ(llwebrtc::getDeviceInterface(), nullptr) << "terminate() left a device interface behind";
        EXPECT_EQ(mBlocker->mClosed.count(), 1);
        EXPECT_EQ(mQueued.mObserver->mClosed.count(), 0) << "expected the second connection to be stuck behind the first";
    }

    // Unblocks the stuck observer and waits for the abandoned shutdown to
    // finish: the queued connection closed and the old instance's threads
    // gone.
    void releaseAndWaitForAbandonedShutdown()
    {
        const int threads_before = processThreadCount();
        mBlocker->release();

        EXPECT_TRUE(mQueued.mObserver->mClosed.waitFor(1)) << "the abandoned shutdown never closed the remaining connection";

        if (threads_before >= 0)
        {
            // At least the network, worker and signaling threads, and the
            // detached shutdown thread itself.
            int last = -1;
            EXPECT_TRUE(waitForThreadCountAtMost(threads_before - 4, std::chrono::seconds(10), last))
                << "the abandoned shutdown didn't stop its threads (" << threads_before << " threads before, " << last
                << " after)";
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    BlockingCloseObserver* mBlocker{ nullptr };
    ShutdownConnection     mQueued;
};

} // namespace


//
// Library lifecycle
//

TEST(LLWebRTCLifecycle, TerminateWithoutInitIsHarmless)
{
    ASSERT_EQ(llwebrtc::getDeviceInterface(), nullptr);
    llwebrtc::terminate();
    EXPECT_EQ(llwebrtc::getDeviceInterface(), nullptr);
}

TEST_F(LLWebRTCShutdownTest, TerminateImmediatelyAfterInit)
{
    // init() posts an initial device enumeration to the worker thread;
    // terminate() has to cope with that still being queued.
    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, TerminateTwiceIsHarmless)
{
    terminateAndVerify();
    llwebrtc::terminate();
    EXPECT_EQ(llwebrtc::getDeviceInterface(), nullptr);
}

TEST(LLWebRTCLifecycle, ConnectionCallsWithoutInitAreHarmless)
{
    ASSERT_EQ(llwebrtc::getDeviceInterface(), nullptr);
    EXPECT_EQ(llwebrtc::newPeerConnection(), nullptr);
    llwebrtc::freePeerConnection(nullptr);
}

TEST_F(LLWebRTCShutdownTest, ConnectionCallsAfterTerminateAreHarmless)
{
    // A viewer connection created or destroyed after shutdown -- by a late
    // coroutine, say -- must not crash.  terminate() has already destroyed
    // the connection, so freePeerConnection() is handed a dangling pointer
    // here, just as the viewer would hand it one; it must ignore it without
    // touching it.
    ShutdownConnection connection = openConnection();
    ASSERT_NE(connection.mConnection, nullptr);
    terminateAndVerify();

    EXPECT_EQ(llwebrtc::newPeerConnection(), nullptr);
    llwebrtc::freePeerConnection(connection.mConnection);
    llwebrtc::freePeerConnection(nullptr);
}

TEST_F(LLWebRTCShutdownTest, InitWhileRunningKeepsExistingInstance)
{
    RecordingLogCallback* other_log = makeImmortal<RecordingLogCallback>();
    llwebrtc::init(other_log);
    EXPECT_EQ(llwebrtc::getDeviceInterface(), mDevices);

    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, RepeatedInitTerminateCycles)
{
    for (int cycle = 0; cycle < 5; ++cycle)
    {
        SCOPED_TRACE("cycle " + std::to_string(cycle));

        ShutdownConnection connection = openConnectionWithOffer();
        terminateAndVerify();
        EXPECT_GE(connection.mObserver->mClosed.count(), 1);

        if (cycle < 4)
        {
            start();
        }
    }
}

TEST_F(LLWebRTCShutdownTest, NoLoggingAfterTerminate)
{
    // Generate some log traffic right up to the moment of shutdown.
    ShutdownConnection connection = openConnection();
    ASSERT_NE(connection.mConnection, nullptr);
    connection.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions());
    mDevices->refreshDevices();

    terminateAndVerify(); // the gate covers LogMessage too
    EXPECT_FALSE(mLog->entries().empty()) << "expected some log output before terminate()";
}

//
// Device observers
//

TEST_F(LLWebRTCShutdownTest, TerminateDuringDeviceRefreshStorm)
{
    // Device enumeration runs on the worker thread and may kick off a device
    // re-deploy via the signaling thread; terminate() must flush all of that
    // before it drops the observer list.
    GatedDevicesObserver* observer = newDevicesObserver();
    mDevices->setDevicesObserver(observer);

    for (int i = 0; i < 50; ++i)
    {
        mDevices->refreshDevices();
    }

    // The viewer doesn't unregister its devices observer before terminate().
    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, TerminateWithDeviceObserverStillRegistered)
{
    GatedDevicesObserver* observer = newDevicesObserver();
    mDevices->setDevicesObserver(observer);
    mDevices->refreshDevices();
    ASSERT_TRUE(observer->mCalls.waitFor(1));

    terminateAndVerify();
}

//
// Connections left for terminate() to close
//

TEST_F(LLWebRTCShutdownTest, TerminateClosesUninitializedConnection)
{
    ShutdownConnection connection = openConnection();
    ASSERT_NE(connection.mConnection, nullptr);

    terminateAndVerify();

    // Delivered synchronously, before terminate() returned.
    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateClosesConnectionWithOfferInFlight)
{
    // initializeConnection() posts the peer connection setup to the signaling
    // thread and CreateOffer() completes asynchronously after that, so this
    // tears down in the middle of offer creation.
    ShutdownConnection connection = openConnection();
    ASSERT_NE(connection.mConnection, nullptr);
    ASSERT_TRUE(connection.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));

    terminateAndVerify();

    EXPECT_GE(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateClosesConnectionAfterOffer)
{
    // The local description is set and ICE gathering is under way on the
    // network thread when terminate() runs.
    ShutdownConnection connection = openConnectionWithOffer();
    ASSERT_NE(connection.mConnection, nullptr);

    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateClosesEstateSizedSessionSet)
{
    // An estate session can hold ten live connections (the current region,
    // up to eight neighbours and a group/ad-hoc session).  terminate() closes
    // them serially, inside one budget it also spends on the audio devices.
    constexpr int CONNECTION_COUNT = 10;

    std::vector<ShutdownConnection> connections;
    for (int i = 0; i < CONNECTION_COUNT; ++i)
    {
        connections.push_back(openConnectionWithOffer());
        ASSERT_NE(connections.back().mConnection, nullptr);
    }

    terminateAndVerify();

    for (int i = 0; i < CONNECTION_COUNT; ++i)
    {
        EXPECT_EQ(connections[i].mObserver->mClosed.count(), 1) << "connection " << i;
    }
}

//
// The viewer's drain-then-terminate sequence
//

TEST_F(LLWebRTCShutdownTest, TerminateAfterConnectionsDrained)
{
    // drainConnections() completed in time: every connection has been shut
    // down and reported closed, but none has been freed or unregistered.
    std::vector<ShutdownConnection> connections;
    for (int i = 0; i < 3; ++i)
    {
        connections.push_back(openConnectionWithOffer());
        ASSERT_NE(connections.back().mConnection, nullptr);
    }
    for (auto& connection : connections)
    {
        ASSERT_TRUE(connection.mConnection->shutdownConnection());
    }
    for (auto& connection : connections)
    {
        ASSERT_TRUE(connection.mObserver->mClosed.waitFor(1));
    }

    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, TerminateWhileShutdownStillInFlight)
{
    // drainConnections() ran out of time: shutdownConnection() has been
    // called but the close posted to the signaling thread hasn't happened
    // yet when terminate() takes over.
    std::vector<ShutdownConnection> connections;
    for (int i = 0; i < 3; ++i)
    {
        connections.push_back(openConnectionWithOffer());
        ASSERT_NE(connections.back().mConnection, nullptr);
    }
    for (auto& connection : connections)
    {
        connection.mConnection->shutdownConnection();
    }

    terminateAndVerify();

    for (auto& connection : connections)
    {
        EXPECT_GE(connection.mObserver->mClosed.count(), 1);
    }
}

TEST_F(LLWebRTCShutdownTest, TerminateWithMixOfConnectionStates)
{
    ShutdownConnection uninitialized = openConnection();
    ShutdownConnection offering      = openConnection();
    ShutdownConnection offered       = openConnectionWithOffer();
    ShutdownConnection closing       = openConnectionWithOffer();
    ShutdownConnection closed        = openConnectionWithOffer();

    ASSERT_NE(offering.mConnection, nullptr);
    ASSERT_NE(closing.mConnection, nullptr);
    ASSERT_NE(closed.mConnection, nullptr);

    closed.mConnection->shutdownConnection();
    ASSERT_TRUE(closed.mObserver->mClosed.waitFor(1));

    offering.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions());
    closing.mConnection->shutdownConnection();

    terminateAndVerify();

    EXPECT_EQ(uninitialized.mObserver->mClosed.count(), 1);
    EXPECT_GE(offering.mObserver->mClosed.count(), 1);
    EXPECT_EQ(offered.mObserver->mClosed.count(), 1);
    EXPECT_GE(closing.mObserver->mClosed.count(), 1);
}

//
// Connections freed before terminate()
//

TEST_F(LLWebRTCShutdownTest, TerminateAfterConnectionClosedAndFreed)
{
    // The normal path for a session that ended before shutdown.
    ShutdownConnection connection = openConnectionWithOffer();
    ASSERT_NE(connection.mConnection, nullptr);

    connection.mConnection->shutdownConnection();
    ASSERT_TRUE(connection.mObserver->mClosed.waitFor(1));
    connection.mConnection->unsetSignalingObserver(connection.mObserver);
    llwebrtc::freePeerConnection(connection.mConnection);

    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateAfterConnectionFreedWithoutShutdown)
{
    // A connection freed without being shut down first is no longer tracked,
    // so terminate() can't close it; its webrtc peer connection is still
    // alive and gathering ICE candidates when the threads are torn down.
    ShutdownConnection connection = openConnectionWithOffer();
    ASSERT_NE(connection.mConnection, nullptr);

    connection.mConnection->unsetSignalingObserver(connection.mObserver);
    llwebrtc::freePeerConnection(connection.mConnection);

    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, TerminateAfterConnectionFreedWithOfferInFlight)
{
    ShutdownConnection connection = openConnection();
    ASSERT_NE(connection.mConnection, nullptr);
    ASSERT_TRUE(connection.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));

    connection.mConnection->unsetSignalingObserver(connection.mObserver);
    llwebrtc::freePeerConnection(connection.mConnection);

    terminateAndVerify();
}

//
// Live calls (connected to a test server)
//

TEST_F(LLWebRTCShutdownTest, TerminateWithLiveCall)
{
    // ICE, DTLS and SCTP are all up, so closing has real network work to
    // do: data channel close, SCTP shutdown and transport teardown.
    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);

    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateWithEstateSizedSetOfLiveCalls)
{
    // Ten live connections closed serially inside one shutdown budget.
    constexpr int CONNECTION_COUNT = 10;

    std::vector<ShutdownConnection> connections;
    for (int i = 0; i < CONNECTION_COUNT; ++i)
    {
        connections.push_back(openLiveConnection());
    }
    ASSERT_TRUE(mConnected);

    terminateAndVerify();

    for (int i = 0; i < CONNECTION_COUNT; ++i)
    {
        EXPECT_EQ(connections[i].mObserver->mClosed.count(), 1) << "connection " << i;
    }
}

TEST_F(LLWebRTCShutdownTest, TerminateWithStatsRequestInFlight)
{
    // Closing a peer connection flushes any pending GetStats request and
    // delivers its result inline, from inside terminate().  The viewer's
    // drainConnections() notes that a request left in flight can make
    // Close() wait on the network thread with no timeout.
    //
    // gatherConnectionStats() posts the request to the signaling thread
    // ahead of terminate()'s close, so the request is always outstanding
    // when the close runs.  Repeated, since how far it has got varies.
    for (int cycle = 0; cycle < 10; ++cycle)
    {
        SCOPED_TRACE("cycle " + std::to_string(cycle));
        if (cycle > 0)
        {
            start();
        }

        ShutdownConnection connection = openLiveConnection();
        ASSERT_TRUE(mConnected);

        connection.mConnection->gatherConnectionStats();
        terminateAndVerify();
        ASSERT_FALSE(HasFailure());
    }
}

TEST_F(LLWebRTCShutdownTest, TerminateWhileDataIsFlowing)
{
    // Messages in flight in both directions when terminate() runs.
    constexpr int MESSAGE_COUNT = 500;

    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);
    connection.mServer->peer()->setEchoData(true);

    llwebrtc::LLWebRTCDataInterface* data = connection.mObserver->mDataInterface;
    for (int i = 0; i < MESSAGE_COUNT; ++i)
    {
        data->sendData("from viewer " + std::to_string(i));
        connection.mServer->peer()->sendData("from server " + std::to_string(i), false);
    }

    terminateAndVerify(); // the gate covers OnDataReceived too
}

TEST_F(LLWebRTCShutdownTest, FreeConnectionWithDataSendsPending)
{
    // The viewer's normal (non-terminate) close path, with sends still
    // queued on the network thread when the connection is freed.
    constexpr int MESSAGE_COUNT = 500;

    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);

    llwebrtc::LLWebRTCDataInterface* data = connection.mObserver->mDataInterface;
    for (int i = 0; i < MESSAGE_COUNT; ++i)
    {
        data->sendData(std::string(1000, 'x'));
    }

    data->unsetDataObserver(connection.mDataObserver);
    connection.mConnection->shutdownConnection();
    ASSERT_TRUE(connection.mObserver->mClosed.waitFor(1));
    connection.mConnection->unsetSignalingObserver(connection.mObserver);
    llwebrtc::freePeerConnection(connection.mConnection);

    // Give the queued sends time to run against the freed connection.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, TerminateWhileConnecting)
{
    // The answer has been applied and ICE checks and the DTLS handshake are
    // under way when terminate() runs -- e.g. quitting just after a login or
    // teleport.  Most dangerous is the connection reaching 'connected' in
    // the middle of terminate(), after the webrtc threads have been handed
    // to its shutdown thread but before the connections are closed.  That
    // window is narrow, so sweep the delay between the answer and
    // terminate() to land the handshake at different points in shutdown.
    constexpr int                       CYCLES = 40;
    constexpr std::chrono::milliseconds STEP{ 3 };

    for (int cycle = 0; cycle < CYCLES; ++cycle)
    {
        SCOPED_TRACE("delay " + std::to_string(cycle * STEP.count()) + " ms");
        if (cycle > 0)
        {
            start();
        }

        std::string answer;
        ShutdownConnection connection = openConnectionWithAnswer(answer);
        ASSERT_FALSE(answer.empty());

        connection.mConnection->AnswerAvailable(answer);
        std::this_thread::sleep_for(cycle * STEP);
        terminateAndVerify();

        EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
        ASSERT_FALSE(HasFailure());
    }
}

TEST_F(LLWebRTCShutdownTest, TerminateWithUnresponsiveServer)
{
    // The server has hung or vanished, so nothing llwebrtc sends during
    // shutdown is answered.  Closing must not wait on the peer.
    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);

    connection.mServer->peer()->freeze();
    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateAfterServerEndedSession)
{
    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);

    connection.mServer->peer()->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

TEST_F(LLWebRTCShutdownTest, TerminateAfterDrainingLiveCalls)
{
    // The viewer's exit path when drainConnections() finishes in time: live
    // calls are shut down and report closed, but are never freed.
    std::vector<ShutdownConnection> connections;
    for (int i = 0; i < 3; ++i)
    {
        connections.push_back(openLiveConnection());
    }
    ASSERT_TRUE(mConnected);

    for (auto& connection : connections)
    {
        connection.mConnection->shutdownConnection();
    }
    for (auto& connection : connections)
    {
        ASSERT_TRUE(connection.mObserver->mClosed.waitFor(1));
    }

    terminateAndVerify();
}

TEST_F(LLWebRTCShutdownTest, TerminateWhileDrainingLiveCalls)
{
    // drainConnections() ran out of time with live calls still closing.
    std::vector<ShutdownConnection> connections;
    for (int i = 0; i < 3; ++i)
    {
        connections.push_back(openLiveConnection());
    }
    ASSERT_TRUE(mConnected);

    for (auto& connection : connections)
    {
        connection.mConnection->shutdownConnection();
    }
    terminateAndVerify();

    for (auto& connection : connections)
    {
        EXPECT_GE(connection.mObserver->mClosed.count(), 1);
    }
}

TEST_F(LLWebRTCShutdownTest, TerminateWithRenegotiationTimerPending)
{
    // Once a connection goes disconnected, llwebrtc schedules a check 10s
    // later that may ask the viewer to renegotiate.  terminate() runs while
    // that check is still pending; it must never fire.
    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);

    connection.mServer->peer()->freeze();
    std::this_thread::sleep_for(std::chrono::seconds(7)); // disconnected after ~5s

    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mRenegotiationNeeded.count(), 0);
}

TEST_F(LLWebRTCShutdownTest, FreeConnectionWithRenegotiationTimerPending)
{
    // As above, but the viewer closes and frees the connection while the
    // check is pending (e.g. the user left the region), and llwebrtc keeps
    // running past the moment it fires.
    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);

    connection.mServer->peer()->freeze();
    std::this_thread::sleep_for(std::chrono::seconds(7)); // disconnected after ~5s

    connection.mObserver->mDataInterface.load()->unsetDataObserver(connection.mDataObserver);
    connection.mConnection->shutdownConnection();
    ASSERT_TRUE(connection.mObserver->mClosed.waitFor(1));
    connection.mConnection->unsetSignalingObserver(connection.mObserver);
    llwebrtc::freePeerConnection(connection.mConnection);

    // Run past the point the check fires (10s after the disconnect).
    std::this_thread::sleep_for(std::chrono::seconds(10));
    EXPECT_EQ(connection.mObserver->mRenegotiationNeeded.count(), 0);

    terminateAndVerify();
}

//
// Calls and state changes racing terminate()
//

TEST_F(LLWebRTCShutdownTest, TerminateWithConnectionCallsQueued)
{
    // The viewer adjusts mute and volume, sends data and asks for stats right
    // up to quitting.  Each of those posts to the signaling or network
    // thread, and some of those tasks post again from there.  Queue a burst
    // on every connection and terminate() straight away, so that work is
    // still running -- and still posting -- as shutdown starts.  Anything
    // posted once terminate() has started must be dropped, not handed to the
    // webrtc threads terminate() has already taken.
    constexpr int CYCLES           = 3;
    constexpr int CONNECTION_COUNT = 3;
    constexpr int CALLS            = 20;

    for (int cycle = 0; cycle < CYCLES; ++cycle)
    {
        SCOPED_TRACE("cycle " + std::to_string(cycle));
        if (cycle > 0)
        {
            start();
        }

        std::vector<ShutdownConnection> connections;
        for (int i = 0; i < CONNECTION_COUNT; ++i)
        {
            connections.push_back(openLiveConnection());
        }
        ASSERT_TRUE(mConnected);

        for (auto& connection : connections)
        {
            llwebrtc::LLWebRTCAudioInterface* audio = connection.mObserver->mAudioInterface;
            llwebrtc::LLWebRTCDataInterface*  data  = connection.mObserver->mDataInterface;
            ASSERT_NE(audio, nullptr);
            ASSERT_NE(data, nullptr);
            for (int i = 0; i < CALLS; ++i)
            {
                const float volume = static_cast<float>(i) / CALLS;
                audio->setMute(i % 2 == 0);
                audio->setReceiveVolume(volume);
                audio->setSendVolume(volume);
                data->sendData("burst " + std::to_string(i));
                connection.mConnection->gatherConnectionStats();
            }
        }
        // And one is being shut down as well, as drainConnections() would.
        connections.front().mConnection->shutdownConnection();

        terminateAndVerify();

        for (int i = 0; i < CONNECTION_COUNT; ++i)
        {
            EXPECT_GE(connections[i].mObserver->mClosed.count(), 1) << "connection " << i;
        }
        ASSERT_FALSE(HasFailure());
    }
}

TEST_F(LLWebRTCShutdownTest, TerminateWhileConnectionDropping)
{
    // When a connection goes 'disconnected', llwebrtc schedules its
    // renegotiation check on the signaling thread.  If that transition lands
    // after terminate() has started, the post must be dropped rather than
    // reach the webrtc threads terminate() has already taken.  As in
    // TerminateWhileConnecting the window is narrow, so the first cycle times
    // how long a frozen server takes to drop the connection, and the rest
    // sweep terminate() across that point.
    constexpr int                       CYCLES = 8;
    constexpr std::chrono::milliseconds FIRST_OFFSET{ -150 };
    constexpr std::chrono::milliseconds STEP{ 50 };

    std::chrono::milliseconds disconnect_after{ 0 };
    for (int cycle = 0; cycle < CYCLES; ++cycle)
    {
        const std::chrono::milliseconds offset = FIRST_OFFSET + STEP * (cycle - 1);
        SCOPED_TRACE(cycle == 0 ? std::string("measuring") : "offset " + std::to_string(offset.count()) + " ms");
        if (cycle > 0)
        {
            start();
        }

        ShutdownConnection connection = openLiveConnection();
        ASSERT_TRUE(mConnected);

        const auto frozen = std::chrono::steady_clock::now();
        connection.mServer->peer()->freeze();
        if (cycle == 0)
        {
            ASSERT_TRUE(waitForLog(*mLog, DISCONNECTED_LOG, std::chrono::seconds(15)))
                << "the connection never went disconnected";
            disconnect_after =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - frozen);
            RecordProperty("disconnect_ms", std::to_string(disconnect_after.count()));
        }
        else
        {
            std::this_thread::sleep_until(frozen + disconnect_after + offset);
        }

        terminateAndVerify();

        EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
        EXPECT_EQ(connection.mObserver->mRenegotiationNeeded.count(), 0);
        ASSERT_FALSE(HasFailure());
    }
}

//
// Server going away
//

TEST_F(LLWebRTCShutdownTest, TerminateAfterServerVanished)
{
    // The server is gone altogether, not just silent: its sockets are closed,
    // so what llwebrtc sends to it now bounces.  On Windows a UDP send to a
    // closed port makes a later receive on that socket fail with
    // WSAECONNRESET, a classic source of trouble in teardown code.  Terminate
    // straight away, and again once some consent checks have bounced.
    const std::chrono::milliseconds delays[] = { std::chrono::milliseconds(0), std::chrono::milliseconds(2500) };

    for (size_t cycle = 0; cycle < std::size(delays); ++cycle)
    {
        SCOPED_TRACE("delay " + std::to_string(delays[cycle].count()) + " ms");
        if (cycle > 0)
        {
            start();
        }

        ShutdownConnection connection = openLiveConnection();
        ASSERT_TRUE(mConnected);

        auto server = std::find_if(mServers.begin(), mServers.end(),
                                   [&connection](const std::unique_ptr<TestServer>& s) { return s.get() == connection.mServer; });
        ASSERT_NE(server, mServers.end());
        mServers.erase(server);
        connection.mServer = nullptr;

        std::this_thread::sleep_for(delays[cycle]);
        terminateAndVerify();

        EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
        ASSERT_FALSE(HasFailure());
    }
}

//
// terminate() running out of budget
//

TEST_F(LLWebRTCShutdownTimeoutTest, TerminateTimesOut)
{
    // Something holds up the shutdown -- here a viewer callback that doesn't
    // return -- so terminate() gives up after its budget, abandons the
    // shutdown to a detached thread and leaks the instance.  It must return
    // on time, and the abandoned shutdown must still finish cleanly once
    // whatever was holding it up lets go.
    terminateAndTimeOut();
    ASSERT_FALSE(HasFailure());

    releaseAndWaitForAbandonedShutdown();

    // Past the budget the usual promise can't be kept: the abandoned shutdown
    // goes on closing connections, and their observers hear about it late.
    // But the log sink is unhooked before terminate() returns, so no logging
    // reaches the viewer.
    for (const std::string& call : mGate->lateCalls())
    {
        EXPECT_EQ(call, "OnPeerConnectionClosed");
    }
}

TEST_F(LLWebRTCShutdownTimeoutTest, InitAfterTimedOutTerminate)
{
    // After a timed-out terminate(), init() starts a second instance while
    // the first is still shutting down on its detached thread.  The two must
    // coexist: the old one finishing must not disturb a call on the new one,
    // and the new one must then shut down normally.
    terminateAndTimeOut();
    ASSERT_FALSE(HasFailure());

    start();
    ShutdownConnection connection = openLiveConnection();
    ASSERT_TRUE(mConnected);

    releaseAndWaitForAbandonedShutdown();

    connection.mServer->peer()->setEchoData(true);
    connection.mObserver->mDataInterface.load()->sendData("still here");
    EXPECT_TRUE(connection.mDataObserver->mReceived.waitFor(1)) << "the call on the new instance stopped working";

    terminateAndVerify();

    EXPECT_EQ(connection.mObserver->mClosed.count(), 1);
}

//
// Leaks
//

TEST_F(LLWebRTCShutdownTest, NoThreadsLeakedAcrossCycles)
{
    // Each instance runs network, worker and signaling threads, plus whatever
    // the factory and device module start.  terminate() must stop them all,
    // or a session that keeps restarting voice slowly piles up threads.
    if (processThreadCount() < 0)
    {
        GTEST_SKIP() << "can't count this process's threads on this platform";
    }

    // Some threads start once per process on first use, and OS thread pools
    // grow and shrink on their own, so measure only after a couple of cycles
    // and allow a little slack.  A leak of even one thread per cycle still
    // shows up over the measured cycles.
    constexpr int WARMUP_CYCLES   = 2;
    constexpr int MEASURED_CYCLES = 8;
    constexpr int SLACK           = 4;

    int baseline = -1;
    for (int cycle = 0; cycle < WARMUP_CYCLES + MEASURED_CYCLES; ++cycle)
    {
        SCOPED_TRACE("cycle " + std::to_string(cycle));
        if (cycle > 0)
        {
            start();
        }

        ShutdownConnection connection = openLiveConnection();
        ASSERT_TRUE(mConnected);
        terminateAndVerify();
        mServers.clear(); // each test server runs threads of its own
        ASSERT_FALSE(HasFailure());

        if (cycle == WARMUP_CYCLES - 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            baseline = processThreadCount();
        }
    }

    int last = -1;
    EXPECT_TRUE(waitForThreadCountAtMost(baseline + SLACK, std::chrono::seconds(5), last))
        << "thread count grew from " << baseline << " to " << last << " over " << MEASURED_CYCLES
        << " init/terminate cycles";
    RecordProperty("baseline_threads", std::to_string(baseline));
    RecordProperty("final_threads", std::to_string(last));
}
