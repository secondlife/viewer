/**
 * @file llwebrtc_connection_test.cpp
 * @brief Tests for llwebrtc connections to a (test) voice server.
 *
 * Each test brings up real connections between llwebrtc and the test peer
 * (gtests/testpeer), which stands in for the Second Life voice server.
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

#include "llwebrtc_test_support.h"

#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using llwebrtc::LLWebRTCPeerConnectionInterface;
using llwebrtc_test::RecordingDataObserver;
using llwebrtc_test::RecordingSignalingObserver;
using llwebrtc_test::TestServer;

namespace
{

struct LiveConnection
{
    LLWebRTCPeerConnectionInterface*            mConnection{ nullptr };
    std::unique_ptr<RecordingSignalingObserver> mObserver;
    std::unique_ptr<RecordingDataObserver>      mDataObserver;
    std::unique_ptr<TestServer>                 mServer;
};

class LLWebRTCConnectionTest : public testing::Test
{
protected:
    void TearDown() override
    {
        for (auto& connection : mConnections)
        {
            close(connection);
        }
        mConnections.clear();
    }

    // A connection and a test server, not yet connected.
    LiveConnection& open()
    {
        LiveConnection connection;
        connection.mObserver     = std::make_unique<RecordingSignalingObserver>();
        connection.mDataObserver = std::make_unique<RecordingDataObserver>();
        connection.mServer       = std::make_unique<TestServer>();
        connection.mConnection   = llwebrtc::newPeerConnection();
        if (connection.mConnection)
        {
            connection.mConnection->setSignalingObserver(connection.mObserver.get());
        }
        mConnections.push_back(std::move(connection));
        return mConnections.back();
    }

    // A connection that has completed the handshake with its test server,
    // with a data observer registered.  Check mConnected afterwards.
    LiveConnection& connect()
    {
        LiveConnection& connection = open();
        testing::AssertionResult connected =
            llwebrtc_test::connectToServer(connection.mConnection, *connection.mObserver, *connection.mServer);
        EXPECT_TRUE(connected);
        if (connected)
        {
            connection.mObserver->mDataInterface.load()->setDataObserver(connection.mDataObserver.get());
        }
        mConnected = mConnected && connected;
        return connection;
    }

    // Closes the llwebrtc side the way the viewer does, then the server.
    static void close(LiveConnection& connection)
    {
        if (connection.mConnection)
        {
            if (llwebrtc::LLWebRTCDataInterface* data = connection.mObserver->mDataInterface)
            {
                data->unsetDataObserver(connection.mDataObserver.get());
            }
            const int closed_before = connection.mObserver->mClosed.count();
            connection.mConnection->shutdownConnection();
            EXPECT_TRUE(connection.mObserver->mClosed.waitFor(closed_before + 1))
                << "OnPeerConnectionClosed was not called after shutdownConnection()";
            connection.mConnection->unsetSignalingObserver(connection.mObserver.get());
            llwebrtc::freePeerConnection(connection.mConnection);
            connection.mConnection = nullptr;
        }
        connection.mServer.reset();
    }

    // deque: open()/connect() hand out references that must stay valid.
    std::deque<LiveConnection> mConnections;
    bool                       mConnected{ true };
};

// Slow tests (15-25s each) covering how llwebrtc copes with a server that
// stops responding: they wait out webrtc's disconnect detection (~5s) and
// llwebrtc's DISCONNECT_RENEGOTIATE_DELAY (10s).  Kept in their own suite so
// they're easy to filter out: --gtest_filter=-LLWebRTCConnectionRecoveryTest.*
class LLWebRTCConnectionRecoveryTest : public LLWebRTCConnectionTest
{
};

std::string makeMessage(size_t size)
{
    std::string message(size, '\0');
    for (size_t i = 0; i < size; ++i)
    {
        message[i] = static_cast<char>(i * 31 + 7); // every byte value, including NUL
    }
    return message;
}

} // namespace

TEST_F(LLWebRTCConnectionTest, ConnectsToServer)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);

    EXPECT_TRUE(connection.mServer->waitForState(TestServer::CONNECTION_CONNECTED));
    EXPECT_NE(connection.mObserver->mAudioInterface.load(), nullptr);
    EXPECT_NE(connection.mObserver->mDataInterface.load(), nullptr);
    EXPECT_EQ(connection.mObserver->mRenegotiationNeeded.count(), 0);
}

TEST_F(LLWebRTCConnectionTest, CandidatesAreTrickledOnceAnswered)
{
    // llwebrtc holds its candidates until the server's answer arrives, then
    // hands them to the viewer to trickle to the server.
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);

    EXPECT_GE(connection.mObserver->mIceCandidates.count(), 1);
    EXPECT_GE(connection.mObserver->mIceGatheringStateChanges.count(), 1);
}

TEST_F(LLWebRTCConnectionTest, DataFromServerIsDelivered)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);

    connection.mServer->peer()->sendData("{\"text\":\"hello viewer\"}", false);
    connection.mServer->peer()->sendData(makeMessage(256), true);
    ASSERT_TRUE(connection.mDataObserver->mReceived.waitFor(2));

    auto messages = connection.mDataObserver->messages();
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0].mData, "{\"text\":\"hello viewer\"}");
    EXPECT_FALSE(messages[0].mBinary);
    EXPECT_TRUE(messages[1].mData == makeMessage(256));
    EXPECT_TRUE(messages[1].mBinary);
}

TEST_F(LLWebRTCConnectionTest, DataToServerIsDelivered)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);

    llwebrtc::LLWebRTCDataInterface* data = connection.mObserver->mDataInterface;
    data->sendData("{\"text\":\"hello server\"}");
    data->sendData(makeMessage(256), true);
    ASSERT_TRUE(connection.mServer->mReceived.waitFor(2));

    auto messages = connection.mServer->messages();
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0].mData, "{\"text\":\"hello server\"}");
    EXPECT_FALSE(messages[0].mBinary);
    EXPECT_TRUE(messages[1].mData == makeMessage(256));
    EXPECT_TRUE(messages[1].mBinary);
}

TEST_F(LLWebRTCConnectionTest, DataRoundTripPreservesOrder)
{
    // The SLData channel is ordered and reliable.
    constexpr int MESSAGE_COUNT = 200;

    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);
    connection.mServer->peer()->setEchoData(true);

    llwebrtc::LLWebRTCDataInterface* data = connection.mObserver->mDataInterface;
    for (int i = 0; i < MESSAGE_COUNT; ++i)
    {
        data->sendData("message " + std::to_string(i));
    }
    ASSERT_TRUE(connection.mDataObserver->mReceived.waitFor(MESSAGE_COUNT));

    auto messages = connection.mDataObserver->messages();
    ASSERT_EQ(messages.size(), static_cast<size_t>(MESSAGE_COUNT));
    for (int i = 0; i < MESSAGE_COUNT; ++i)
    {
        EXPECT_EQ(messages[i].mData, "message " + std::to_string(i));
    }
}

TEST_F(LLWebRTCConnectionTest, LargeBinaryMessageRoundTrip)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);
    connection.mServer->peer()->setEchoData(true);

    const std::string message = makeMessage(60000);
    connection.mObserver->mDataInterface.load()->sendData(message, true);
    ASSERT_TRUE(connection.mDataObserver->mReceived.waitFor(1));

    auto messages = connection.mDataObserver->messages();
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_TRUE(messages[0].mBinary);
    EXPECT_EQ(messages[0].mData.size(), message.size());
    EXPECT_TRUE(messages[0].mData == message);
}

TEST_F(LLWebRTCConnectionTest, StatsAreDeliveredForLiveConnection)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);

    connection.mConnection->gatherConnectionStats();
    ASSERT_TRUE(connection.mObserver->mStatsDelivered.waitFor(1));

    llwebrtc::LLWebRTCStatsMap stats = connection.mObserver->lastStats();
    EXPECT_FALSE(stats.empty());
    for (const auto& [id, attributes] : stats)
    {
        EXPECT_FALSE(id.empty());
    }
}

TEST_F(LLWebRTCConnectionTest, AudioControlsOnLiveConnection)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);
    connection.mServer->peer()->setEchoData(true);

    llwebrtc::LLWebRTCAudioInterface* audio = connection.mObserver->mAudioInterface;
    audio->setMute(false);
    audio->setReceiveVolume(0.5f);
    audio->setSendVolume(0.25f);
    audio->setMute(true);
    audio->setReceiveVolume(0.0f);
    audio->setSendVolume(1.0f);

    // The connection is unaffected.
    connection.mObserver->mDataInterface.load()->sendData("still here");
    ASSERT_TRUE(connection.mDataObserver->mReceived.waitFor(1));
    EXPECT_EQ(connection.mObserver->mRenegotiationNeeded.count(), 0);
}

TEST_F(LLWebRTCConnectionTest, ConcurrentConnectionsStaySeparate)
{
    // The viewer holds a connection per nearby region, each to a different
    // server.
    constexpr int CONNECTION_COUNT = 3;

    std::vector<LiveConnection*> connections;
    for (int i = 0; i < CONNECTION_COUNT; ++i)
    {
        connections.push_back(&connect());
    }
    ASSERT_TRUE(mConnected);

    for (int i = 0; i < CONNECTION_COUNT; ++i)
    {
        connections[i]->mObserver->mDataInterface.load()->sendData("for server " + std::to_string(i));
    }
    for (int i = 0; i < CONNECTION_COUNT; ++i)
    {
        ASSERT_TRUE(connections[i]->mServer->mReceived.waitFor(1));
        auto messages = connections[i]->mServer->messages();
        ASSERT_EQ(messages.size(), 1u) << "server " << i;
        EXPECT_EQ(messages[0].mData, "for server " + std::to_string(i));
    }
}

TEST_F(LLWebRTCConnectionTest, UnparseableAnswerRequestsRenegotiation)
{
    LiveConnection& connection = open();
    std::string answer;
    ASSERT_TRUE(llwebrtc_test::negotiateAnswer(connection.mConnection, *connection.mObserver, *connection.mServer, answer));

    connection.mConnection->AnswerAvailable("v=0\r\nthis is not an answer\r\n");

    EXPECT_TRUE(connection.mObserver->mRenegotiationNeeded.waitFor(1));
    EXPECT_EQ(connection.mObserver->mAudioEstablished.count(), 0);
}

TEST_F(LLWebRTCConnectionTest, AnswerWithWrongCertificateRequestsRenegotiation)
{
    // An answer whose DTLS fingerprint doesn't match the server's
    // certificate: ICE connects, then the DTLS handshake must fail.
    LiveConnection& connection = open();
    std::string answer;
    ASSERT_TRUE(llwebrtc_test::negotiateAnswer(connection.mConnection, *connection.mObserver, *connection.mServer, answer));

    const std::string prefix = "a=fingerprint:sha-256 ";
    size_t pos = answer.find(prefix);
    ASSERT_NE(pos, std::string::npos) << "answer has no sha-256 fingerprint";
    char& digit = answer[pos + prefix.size()];
    digit = (digit == '0') ? '1' : '0';

    connection.mConnection->AnswerAvailable(answer);

    EXPECT_TRUE(connection.mObserver->mRenegotiationNeeded.waitFor(1));
    EXPECT_EQ(connection.mObserver->mAudioEstablished.count(), 0);
    EXPECT_EQ(connection.mObserver->mDataChannelReady.count(), 0);
}

TEST_F(LLWebRTCConnectionTest, ConnectionCanBeShutDownWhileConnecting)
{
    // shutdownConnection() during ICE/DTLS, before audio is established.
    LiveConnection& connection = open();
    std::string answer;
    ASSERT_TRUE(llwebrtc_test::negotiateAnswer(connection.mConnection, *connection.mObserver, *connection.mServer, answer));
    connection.mConnection->AnswerAvailable(answer);

    close(connection); // checks that OnPeerConnectionClosed arrives
}

TEST_F(LLWebRTCConnectionRecoveryTest, ServerThatStopsRespondingTriggersRenegotiation)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);

    connection.mServer->peer()->freeze();

    // llwebrtc gives a disconnected connection 10s to recover on its own
    // before asking for renegotiation.
    EXPECT_FALSE(connection.mObserver->mRenegotiationNeeded.waitFor(1, std::chrono::seconds(10)))
        << "renegotiation was requested before DISCONNECT_RENEGOTIATE_DELAY elapsed";
    EXPECT_TRUE(connection.mObserver->mRenegotiationNeeded.waitFor(1, std::chrono::seconds(20)));
}

TEST_F(LLWebRTCConnectionRecoveryTest, ServerClosingTheSessionTriggersRenegotiation)
{
    // llwebrtc doesn't act on the server's DTLS/SCTP close itself; it's
    // picked up as a disconnect, so this takes as long as a server that
    // stops responding (~15s).
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);

    connection.mServer->peer()->close();

    EXPECT_TRUE(connection.mObserver->mRenegotiationNeeded.waitFor(1, std::chrono::seconds(30)));
}

TEST_F(LLWebRTCConnectionRecoveryTest, BriefServerStallRecoversWithoutRenegotiation)
{
    LiveConnection& connection = connect();
    ASSERT_TRUE(mConnected);
    connection.mServer->peer()->setEchoData(true);

    // Long enough for webrtc to declare the connection disconnected (~5s),
    // well short of llwebrtc's 10s renegotiation delay.
    connection.mServer->peer()->freeze();
    std::this_thread::sleep_for(std::chrono::seconds(7));
    connection.mServer->peer()->thaw();

    // Reconnecting is reported as audio being established again, which
    // also shows the stall really did disconnect the connection.
    ASSERT_TRUE(connection.mObserver->mAudioEstablished.waitFor(2)) << "connection didn't recover after the stall";

    // Wait past when the stale renegotiation timer would have fired.
    EXPECT_FALSE(connection.mObserver->mRenegotiationNeeded.waitFor(1, std::chrono::seconds(10)));

    connection.mObserver->mDataInterface.load()->sendData("after the stall");
    ASSERT_TRUE(connection.mDataObserver->mReceived.waitFor(1));
    EXPECT_EQ(connection.mDataObserver->messages().back().mData, "after the stall");
}
