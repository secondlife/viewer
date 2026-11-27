/**
 * @file llvoicewebrtc_e2e_test.cpp
 * @brief Tests for llvoicewebrtc.cpp over real llwebrtc connections.
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

// The voice client over the real llwebrtc library, connecting to the test
// peer from indra/llwebrtc/gtests standing in for the voice server.  The
// simulator is still the TestWorld: provisioning requests are answered by
// handing the offer to a test server and returning its answer, as the
// simulator does with the real voice server.
//
// These run real connections on fixed ports, so they hold the 'llwebrtc'
// resource lock, and each runs in its own process.

#include "llviewerprecompiledheaders.h"

#include "llvoicewebrtc_test_support.h"

#include "llwebrtc_test_support.h"

#include <chrono>
#include <map>
#include <memory>
#include <thread>

using namespace llvoicewebrtc_test;
using llwebrtc_test::TestServer;
using Peer = llwebrtc_testpeer::LLWebRTCTestPeerObserver;

namespace
{

const LLUUID      AGENT_ID("a9e1d000-0000-4000-8000-000000000001");
const LLUUID      PEER_ID("a9e1d000-0000-4000-8000-000000000002");
const LLUUID      REGION_ID("4e910000-0000-4000-8000-000000000001");
const LLUUID      VIEWER_SESSION("5e551000-0000-4000-8000-000000000001");
const std::string ADHOC_CHANNEL = "adhoc-channel";

class E2EEnvironment : public VoiceTestEnvironment
{
public:
    void SetUp() override
    {
        VoiceTestEnvironment::SetUp();
        ASSERT_TRUE(llwebrtc_test::startNetworking());
    }
    void TearDown() override
    {
        llwebrtc_test::stopNetworking();
        VoiceTestEnvironment::TearDown();
    }
};
[[maybe_unused]] const testing::Environment* const sEnvironment = testing::AddGlobalTestEnvironment(new E2EEnvironment);

TestWorld& world() { return TestWorld::get(); }

class LLVoiceWebRTCE2ETest : public testing::Test
{
protected:
    void SetUp() override
    {
        world().reset();
        resetVoiceSettings();
        gAgentID = AGENT_ID;
        world().addRegion(REGION_ID);
        world().setAgentRegion(REGION_ID);
        world().mHttpHandler = [this](const std::string& url, const LLSD& body) { return simulator(url, body); };

        if (!LLVoiceClient::instanceExists())
        {
            LLVoiceClient::initParamSingleton(nullptr);
        }
        mClient = LLWebRTCVoiceClient::getInstance();
        mClient->init(nullptr);
        mClient->processChannels(true);
        mClient->addObserver(&mStatus);
    }

    void TearDown() override
    {
        mClient->removeObserver(&mStatus);
        mClient->terminate();
        for (auto& post : world().pendingPosts())
        {
            TestWorld::completePost(post, httpFailure(HTTP_SERVICE_UNAVAILABLE));
        }
        pumpCoroutines();
        LLWebRTCVoiceClient::deleteSingleton();
        pumpCoroutines();
        mServers.clear();
        world().reset();
    }

    // The simulator: provisioning requests get a fresh voice server, which
    // answers the offer.  Trickled candidates go to the newest server.
    std::optional<LLSD> simulator(const std::string& url, const LLSD& body)
    {
        if (body.has("jsep"))
        {
            auto server = std::make_unique<TestServer>();
            if (!server->peer())
            {
                return httpFailure(HTTP_INTERNAL_SERVER_ERROR);
            }
            server->peer()->answerOffer(body["jsep"]["sdp"].asString());
            std::string answer;
            if (!server->waitForAnswer(answer))
            {
                return httpFailure(HTTP_INTERNAL_SERVER_ERROR);
            }
            mServers.push_back(std::move(server));
            return provisionAnswer(VIEWER_SESSION, answer);
        }
        if (body.has("candidates") && !mServers.empty())
        {
            for (const LLSD& candidate : llsd::inArray(body["candidates"]))
            {
                mServers.back()->peer()->addRemoteCandidate(candidate["sdpMid"].asString(),
                                                            candidate["sdpMLineIndex"].asInteger(),
                                                            candidate["candidate"].asString());
            }
        }
        return httpSuccess();
    }

    void tick()
    {
        pumpCoroutines();
        Access::workQueue(*mClient)->runPending();
        Access::processSessionStates();
        pumpCoroutines();
        Access::workQueue(*mClient)->runPending();
    }

    // Runs the voice client in real time, as the viewer's voice loop does,
    // until 'pred' holds.
    template <typename PRED>
    bool runUntil(PRED pred, std::chrono::milliseconds timeout = llgtest::DEFAULT_TIMEOUT)
    {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!pred() && std::chrono::steady_clock::now() < deadline)
        {
            tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return pred();
    }

    static LLSD adhocChannelInfo()
    {
        LLSD info;
        info["channel_uri"]         = ADHOC_CHANNEL;
        info["channel_credentials"] = "channel-credentials";
        info["voice_server_type"]   = "webrtc";
        return info;
    }

    static bool received(TestServer& server, const std::string& message)
    {
        for (auto& received : server.messages())
        {
            if (json(received.mData) == json(message))
            {
                return true;
            }
        }
        return false;
    }

    // Starts an ad-hoc session and runs it until the server has its join.
    testing::AssertionResult connectAdHoc()
    {
        Access::startAdHoc(*mClient, adhocChannelInfo(), false, false);
        if (!runUntil([this] { return !mServers.empty(); }))
        {
            return testing::AssertionFailure() << "no provisioning request reached the simulator";
        }
        if (!runUntil([this] { return received(*mServers.back(), R"({"j":{"p":true}})"); }))
        {
            return testing::AssertionFailure() << "the voice server never got a join";
        }
        return testing::AssertionSuccess();
    }

    LLWebRTCVoiceClient*                     mClient{ nullptr };
    RecordingStatusObserver                  mStatus;
    std::vector<std::unique_ptr<TestServer>> mServers;
};

} // namespace

TEST_F(LLVoiceWebRTCE2ETest, AdHocSessionConnectsToTheVoiceServer)
{
    ASSERT_TRUE(connectAdHoc());
    EXPECT_TRUE(mServers.back()->waitForState(Peer::CONNECTION_CONNECTED));
    EXPECT_TRUE(runUntil([this] { return mStatus.saw(LLVoiceClientStatusObserver::STATUS_JOINED); }));
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_LOGGED_IN));
    EXPECT_TRUE(mClient->isParticipant(AGENT_ID));
}

TEST_F(LLVoiceWebRTCE2ETest, ServerParticipantUpdatesReachTheClient)
{
    ASSERT_TRUE(connectAdHoc());
    mServers.back()->peer()->sendData("{\"" + PEER_ID.asString() + "\":{\"j\":{}}}", false);
    EXPECT_TRUE(runUntil([this] { return mClient->isParticipant(PEER_ID); }));

    mServers.back()->peer()->sendData("{\"" + PEER_ID.asString() + "\":{\"p\":64,\"v\":true}}", false);
    EXPECT_TRUE(runUntil([this] { return mClient->getIsSpeaking(PEER_ID); }));
    EXPECT_FLOAT_EQ(mClient->getCurrentPower(PEER_ID), 0.5f);

    mServers.back()->peer()->sendData("{\"" + PEER_ID.asString() + "\":{\"l\":true}}", false);
    EXPECT_TRUE(runUntil([this] { return !mClient->isParticipant(PEER_ID); }));
}

TEST_F(LLVoiceWebRTCE2ETest, UserVolumeReachesTheServer)
{
    ASSERT_TRUE(connectAdHoc());
    mServers.back()->peer()->sendData("{\"" + PEER_ID.asString() + "\":{\"j\":{}}}", false);
    ASSERT_TRUE(runUntil([this] { return mClient->isParticipant(PEER_ID); }));

    mClient->setUserVolume(PEER_ID, 0.8f);
    std::string expected = "{\"ug\":{\"" + PEER_ID.asString() + "\":" + std::to_string((uint32_t)(0.8f * 220)) + "}}";
    EXPECT_TRUE(runUntil([&] { return received(*mServers.back(), expected); }));
}

TEST_F(LLVoiceWebRTCE2ETest, LeavingLogsOutAndClosesTheSession)
{
    ASSERT_TRUE(connectAdHoc());
    mClient->leaveChannel(false);
    EXPECT_TRUE(runUntil([] { return !Access::hasSession(ADHOC_CHANNEL); }));
    auto posts = world().postsTo(TestWorld::provisionUrl(REGION_ID));
    ASSERT_EQ(posts.size(), 2u);
    EXPECT_TRUE(posts[1]->mBody["logout"].asBoolean());
    EXPECT_EQ(posts[1]->mBody["viewer_session"].asUUID(), VIEWER_SESSION);
}

TEST_F(LLVoiceWebRTCE2ETest, QuittingWithALiveSessionIsPrompt)
{
    ASSERT_TRUE(connectAdHoc());
    auto start = std::chrono::steady_clock::now();
    mClient->terminate();
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(elapsed, std::chrono::seconds(3)) << "the connection should close well inside the drain timeout";
    EXPECT_TRUE(Access::allSessionsClosed());
}

// Slow (~20s): waits out webrtc's disconnect detection and llwebrtc's
// renegotiation delay.
TEST_F(LLVoiceWebRTCE2ETest, ServerThatStopsRespondingIsReplaced)
{
    ASSERT_TRUE(connectAdHoc());
    mServers.back()->peer()->freeze();

    EXPECT_TRUE(runUntil([this] { return mServers.size() >= 2; }, std::chrono::seconds(40)))
        << "the client never asked the simulator for a new connection";
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::ERROR_UNKNOWN));
    EXPECT_TRUE(runUntil([this] { return received(*mServers.back(), R"({"j":{"p":true}})"); }));
}
