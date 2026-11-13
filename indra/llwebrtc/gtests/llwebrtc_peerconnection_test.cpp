/**
 * @file llwebrtc_peerconnection_test.cpp
 * @brief Tests for llwebrtc peer connection lifecycle and offer generation.
 *
 * No remote peer is involved: these cover everything up to producing the
 * local SDP offer, and shutting connections down cleanly.
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

#include <memory>
#include <optional>
#include <string>
#include <deque>

using llwebrtc::LLWebRTCPeerConnectionInterface;
using llwebrtc_test::RecordingSignalingObserver;
using testing::ContainsRegex;
using testing::HasSubstr;
using testing::StartsWith;

namespace
{

struct TestConnection
{
    LLWebRTCPeerConnectionInterface*            mConnection{ nullptr };
    std::unique_ptr<RecordingSignalingObserver> mObserver;
};

class LLWebRTCPeerConnectionTest : public testing::Test
{
protected:
    void TearDown() override
    {
        // Clean up anything a failed test left open.
        for (auto& connection : mConnections)
        {
            close(connection);
        }
        mConnections.clear();
    }

    TestConnection& open()
    {
        TestConnection connection;
        connection.mObserver   = std::make_unique<RecordingSignalingObserver>();
        connection.mConnection = llwebrtc::newPeerConnection();
        if (connection.mConnection)
        {
            connection.mConnection->setSignalingObserver(connection.mObserver.get());
        }
        mConnections.push_back(std::move(connection));
        return mConnections.back();
    }

    // Shuts the connection down the same way the viewer does: shut down,
    // wait for OnPeerConnectionClosed, unregister, then free.
    static void close(TestConnection& connection)
    {
        if (!connection.mConnection)
        {
            return;
        }

        const int closed_before = connection.mObserver->mClosed.count();
        connection.mConnection->shutdownConnection();
        EXPECT_TRUE(connection.mObserver->mClosed.waitFor(closed_before + 1))
            << "OnPeerConnectionClosed was not called after shutdownConnection()";

        connection.mConnection->unsetSignalingObserver(connection.mObserver.get());
        llwebrtc::freePeerConnection(connection.mConnection);
        connection.mConnection = nullptr;
    }

    // deque, not vector: open() hands out references that must survive later open() calls.
    std::deque<TestConnection> mConnections;
};

} // namespace

TEST_F(LLWebRTCPeerConnectionTest, NewConnectionCanBeFreedWithoutInitializing)
{
    LLWebRTCPeerConnectionInterface* connection = llwebrtc::newPeerConnection();
    ASSERT_NE(connection, nullptr);
    llwebrtc::freePeerConnection(connection);
}

TEST_F(LLWebRTCPeerConnectionTest, ShutdownWithoutInitializeStillNotifiesClosed)
{
    // The viewer's connection state machine waits for OnPeerConnectionClosed
    // with no timeout of its own, so it must arrive even if the connection
    // never got as far as initializing.
    TestConnection& connection = open();
    ASSERT_NE(connection.mConnection, nullptr);

    EXPECT_TRUE(connection.mConnection->shutdownConnection());
    EXPECT_TRUE(connection.mObserver->mClosed.waitFor(1));

    connection.mConnection->unsetSignalingObserver(connection.mObserver.get());
    llwebrtc::freePeerConnection(connection.mConnection);
    connection.mConnection = nullptr;
}

TEST_F(LLWebRTCPeerConnectionTest, InitializeProducesAudioAndDataOffer)
{
    TestConnection& connection = open();
    ASSERT_NE(connection.mConnection, nullptr);

    ASSERT_TRUE(connection.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));

    std::optional<std::string> offer = connection.mObserver->mOffer.wait();
    ASSERT_TRUE(offer.has_value()) << "OnOfferAvailable was not called after initializeConnection()";

    EXPECT_THAT(*offer, StartsWith("v=0"));
    EXPECT_THAT(*offer, HasSubstr("m=audio "));
    // The "SLData" data channel is negotiated in the same offer.
    EXPECT_THAT(*offer, HasSubstr("m=application "));

    EXPECT_EQ(connection.mObserver->mRenegotiationNeeded.count(), 0);
}

TEST_F(LLWebRTCPeerConnectionTest, OfferRequestsStereoOpusAt48k)
{
    // llwebrtc rewrites the generated SDP to force 48kHz stereo Opus, which
    // is what the Second Life voice servers expect.
    TestConnection& connection = open();
    ASSERT_NE(connection.mConnection, nullptr);
    ASSERT_TRUE(connection.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));

    std::optional<std::string> offer = connection.mObserver->mOffer.wait();
    ASSERT_TRUE(offer.has_value());

#ifdef GTEST_USES_SIMPLE_RE
    EXPECT_THAT(*offer, ContainsRegex("a=rtpmap:\\d+ opus/48000/2"));
#else
    EXPECT_THAT(*offer, ContainsRegex("a=rtpmap:[0-9]+ opus/48000/2"));
#endif
    EXPECT_THAT(*offer, HasSubstr("stereo=1;sprop-stereo=1"));
    EXPECT_THAT(*offer, HasSubstr("maxplaybackrate=48000"));
}

TEST_F(LLWebRTCPeerConnectionTest, ConcurrentConnectionsEachProduceAnOffer)
{
    TestConnection& first  = open();
    TestConnection& second = open();
    ASSERT_NE(first.mConnection, nullptr);
    ASSERT_NE(second.mConnection, nullptr);
    ASSERT_NE(first.mConnection, second.mConnection);

    ASSERT_TRUE(first.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));
    ASSERT_TRUE(second.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));

    std::optional<std::string> first_offer  = first.mObserver->mOffer.wait();
    std::optional<std::string> second_offer = second.mObserver->mOffer.wait();
    ASSERT_TRUE(first_offer.has_value());
    ASSERT_TRUE(second_offer.has_value());

    // Each connection has its own ICE credentials.
    EXPECT_NE(*first_offer, *second_offer);
}

TEST_F(LLWebRTCPeerConnectionTest, InitializedConnectionShutsDownCleanly)
{
    TestConnection& connection = open();
    ASSERT_NE(connection.mConnection, nullptr);
    ASSERT_TRUE(connection.mConnection->initializeConnection(LLWebRTCPeerConnectionInterface::InitOptions()));
    ASSERT_TRUE(connection.mObserver->mOffer.wait().has_value());

    close(connection);

    // No audio or data channel can come up without a remote answer.
    EXPECT_EQ(connection.mObserver->mAudioEstablished.count(), 0);
    EXPECT_EQ(connection.mObserver->mDataChannelReady.count(), 0);
}

TEST_F(LLWebRTCPeerConnectionTest, GatherStatsBeforeInitializeIsHarmless)
{
    TestConnection& connection = open();
    ASSERT_NE(connection.mConnection, nullptr);

    // Stats are only collected for connected peers; asking early must be a
    // no-op rather than a crash.
    connection.mConnection->gatherConnectionStats();
    SUCCEED();
}
