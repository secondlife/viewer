/**
 * @file llvoicewebrtc_test.cpp
 * @brief Tests for llvoicewebrtc.cpp against a fake llwebrtc.
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

// The voice client is driven the way the viewer drives it -- the work queue
// is serviced and the session state machines run once per tick() -- while
// the tests play llwebrtc (through the fake peer connections) and the
// simulator (through the HTTP posts in the TestWorld).

#include "llviewerprecompiledheaders.h"

#include "llvoicewebrtc_test_support.h"
#include "llvoicewebrtc_fake_llwebrtc.h"

#include "llmutelist.h"
#include "llparcel.h"

#include <chrono>
#include <thread>

using namespace llvoicewebrtc_test;
using fake_llwebrtc::FakeLLWebRTC;
using fake_llwebrtc::FakePeerConnection;
using llwebrtc::LLWebRTCSignalingObserver;
using llwebrtc::LLWebRTCVoiceDevice;
using llwebrtc::LLWebRTCVoiceDeviceList;
using testing::ElementsAre;

namespace
{

const LLUUID     AGENT_ID("a9e1d000-0000-4000-8000-000000000001");
const LLUUID     PEER_ID("a9e1d000-0000-4000-8000-000000000002");
const LLUUID     OTHER_PEER_ID("a9e1d000-0000-4000-8000-000000000003");
const LLUUID     REGION_ID("4e910000-0000-4000-8000-000000000001");
const LLUUID     NEIGHBOR_ID("4e910000-0000-4000-8000-000000000002");
const LLUUID     VIVOX_NEIGHBOR_ID("4e910000-0000-4000-8000-000000000003");
const LLUUID     VIEWER_SESSION("5e551000-0000-4000-8000-000000000001");
const LLVector3d REGION_ORIGIN(256000.0, 256000.0, 0.0);
const std::string ADHOC_CHANNEL = "adhoc-channel";

[[maybe_unused]] const testing::Environment* const sEnvironment = testing::AddGlobalTestEnvironment(new VoiceTestEnvironment);

TestWorld&    world() { return TestWorld::get(); }
FakeLLWebRTC& webrtc() { return FakeLLWebRTC::get(); }

class LLVoiceWebRTCTest : public testing::Test
{
protected:
    void SetUp() override
    {
        world().reset();
        webrtc().reset();
        resetVoiceSettings();

        gAgentID = AGENT_ID;
        world().addRegion(REGION_ID, REGION_ORIGIN);
        world().setAgentRegion(REGION_ID);
        world().mHttpHandler = [](const std::string& url, const LLSD& body) -> std::optional<LLSD>
        {
            // Answer provisioning requests; everything else just succeeds.
            if (body.has("jsep"))
            {
                return provisionAnswer(VIEWER_SESSION);
            }
            return httpSuccess();
        };

        if (!LLVoiceClient::instanceExists())
        {
            LLVoiceClient::initParamSingleton(nullptr);
        }
        mClient = LLWebRTCVoiceClient::getInstance();
        mClient->init(nullptr);
        mClient->processChannels(true);
        mClient->addObserver(&mStatus);
        mClient->addObserver(&mParticipants);
    }

    void TearDown() override
    {
        mClient->removeObserver(&mStatus);
        mClient->removeObserver(&mParticipants);
        mClient->terminate();

        // Let the voice loop coroutine see that it's shutting down, and
        // the HTTP coroutines unwind, before the client is destroyed.
        if (mVoiceLoopStarted)
        {
            pumpCoroutinesFor(std::chrono::milliseconds(300));
        }
        for (auto& post : world().pendingPosts())
        {
            TestWorld::completePost(post, httpFailure(HTTP_SERVICE_UNAVAILABLE));
        }
        pumpCoroutines();
        LLWebRTCVoiceClient::deleteSingleton();
        pumpCoroutines();
        world().reset();
        webrtc().reset();
    }

    static void pumpCoroutinesFor(std::chrono::milliseconds duration)
    {
        auto until = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < until)
        {
            pumpCoroutines();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // One turn of the viewer's voice processing: coroutines run, queued
    // llwebrtc callbacks are delivered, and the session state machines
    // advance.
    void tick(int count = 1)
    {
        for (int i = 0; i < count; ++i)
        {
            pumpCoroutines();
            Access::workQueue(*mClient)->runPending();
            Access::processSessionStates();
            pumpCoroutines();
            Access::workQueue(*mClient)->runPending();
        }
    }

    template <typename PRED>
    bool tickUntil(PRED pred, int max_ticks = 50)
    {
        for (int i = 0; i < max_ticks && !pred(); ++i)
        {
            tick();
        }
        return pred();
    }

    static LLSD adhocChannelInfo(const std::string& channel = ADHOC_CHANNEL)
    {
        LLSD info;
        info["channel_uri"]         = channel;
        info["channel_credentials"] = "channel-credentials";
        info["voice_server_type"]   = "webrtc";
        return info;
    }

    // Starts an ad-hoc session and returns its peer connection, initialized
    // and waiting for an offer.
    FakePeerConnection* startAdHoc(const std::string& channel = ADHOC_CHANNEL, bool notify_on_first_join = false,
                                   bool hangup_on_last_leave = false)
    {
        size_t before = webrtc().mConnections.size();
        Access::startAdHoc(*mClient, adhocChannelInfo(channel), notify_on_first_join, hangup_on_last_leave);
        tick();
        return webrtc().mConnections.size() > before ? webrtc().latest() : nullptr;
    }

    // Takes a connection the rest of the way: offer, provisioning, audio,
    // and the data channel, after which it has sent its join.
    testing::AssertionResult bringUp(FakePeerConnection* connection)
    {
        if (!connection)
        {
            return testing::AssertionFailure() << "no peer connection";
        }
        connection->offer();
        if (!tickUntil([&] { return !connection->mAnswers.empty(); }))
        {
            return testing::AssertionFailure() << "the answer never reached the peer connection";
        }
        connection->establishAudio();
        tick();
        connection->dataChannelReady();
        if (!tickUntil([&] { return !connection->mData.mSent.empty(); }))
        {
            return testing::AssertionFailure() << "no join was sent over the data channel";
        }
        return testing::AssertionSuccess();
    }

    FakePeerConnection* connectAdHoc(const std::string& channel = ADHOC_CHANNEL, bool notify_on_first_join = false,
                                     bool hangup_on_last_leave = false)
    {
        FakePeerConnection* connection = startAdHoc(channel, notify_on_first_join, hangup_on_last_leave);
        EXPECT_TRUE(bringUp(connection));
        return connection;
    }

    void enableVoice()
    {
        mClient->setVoiceEnabled(true);
        mVoiceLoopStarted = true;
    }

    // Data the voice client sent on a connection, parsed, after 'from'.
    static std::vector<boost::json::value> sentSince(FakePeerConnection* connection, size_t from)
    {
        std::vector<boost::json::value> result;
        for (size_t i = from; i < connection->mData.mSent.size(); ++i)
        {
            result.push_back(json(connection->mData.mSent[i]));
        }
        return result;
    }

    static std::string participantUpdate(const LLUUID& id, const std::string& update)
    {
        return "{\"" + id.asString() + "\":" + update + "}";
    }

    LLWebRTCVoiceClient*         mClient{ nullptr };
    RecordingStatusObserver      mStatus;
    RecordingParticipantObserver mParticipants;
    bool                         mVoiceLoopStarted{ false };
};

} // namespace

//============================================================================
// Startup and devices

TEST_F(LLVoiceWebRTCTest, InitStartsWebRTCAndAsksForDevices)
{
    EXPECT_EQ(webrtc().mInitCalls, 1);
    EXPECT_EQ(webrtc().mLogCallback, static_cast<llwebrtc::LLWebRTCLogCallback*>(mClient));
    ASSERT_TRUE(webrtc().mDevices);
    EXPECT_EQ(webrtc().mDevices->mObserver, static_cast<llwebrtc::LLWebRTCDevicesObserver*>(mClient));
    EXPECT_GE(webrtc().mDevices->mRefreshes, 1);
    EXPECT_FALSE(webrtc().mDevices->mStopCaptureOnMute);
}

TEST_F(LLVoiceWebRTCTest, StopMicCaptureWhenMutedAppliesAsSoonAsItChanges)
{
    gSavedSettings.setS32("StopMicCaptureWhenMuted", 1);
    EXPECT_TRUE(webrtc().mDevices->mStopCaptureOnMute);
    gSavedSettings.setS32("StopMicCaptureWhenMuted", 0);
    EXPECT_FALSE(webrtc().mDevices->mStopCaptureOnMute);
}

TEST_F(LLVoiceWebRTCTest, DeviceListsComeFromWebRTCAndSelectTheConfiguredDevices)
{
    gSavedSettings.setString("VoiceInputAudioDevice", "mic-2");
    gSavedSettings.setString("VoiceOutputAudioDevice", "speakers-1");
    LLWebRTCVoiceDeviceList render{ LLWebRTCVoiceDevice("Speakers", "speakers-1") };
    LLWebRTCVoiceDeviceList capture{ LLWebRTCVoiceDevice("Mic 1", "mic-1"), LLWebRTCVoiceDevice("Mic 2", "mic-2") };

    mClient->OnDevicesChanged(render, capture); // from a webrtc thread: queued
    EXPECT_FALSE(mClient->deviceSettingsAvailable());
    tick();

    ASSERT_EQ(mClient->getRenderDevices().size(), 1u);
    EXPECT_EQ(mClient->getRenderDevices().front().display_name, "Speakers");
    EXPECT_EQ(mClient->getRenderDevices().front().full_name, "speakers-1");
    ASSERT_EQ(mClient->getCaptureDevices().size(), 2u);
    EXPECT_EQ(mClient->getCaptureDevices().back().full_name, "mic-2");
    EXPECT_EQ(webrtc().mDevices->mRenderDevice, "speakers-1");
    EXPECT_EQ(webrtc().mDevices->mCaptureDevice, "mic-2");
    EXPECT_TRUE(mClient->deviceSettingsAvailable());

    // The device panel polls for updates; each is reported once.
    EXPECT_TRUE(mClient->deviceSettingsUpdated());
    EXPECT_FALSE(mClient->deviceSettingsUpdated());
}

TEST_F(LLVoiceWebRTCTest, UnchangedDeviceListsDontReselectDevices)
{
    LLWebRTCVoiceDeviceList render{ LLWebRTCVoiceDevice("Speakers", "speakers-1") };
    LLWebRTCVoiceDeviceList capture{ LLWebRTCVoiceDevice("Mic 1", "mic-1") };
    mClient->OnDevicesChanged(render, capture);
    tick();
    mClient->OnDevicesChanged(render, capture);
    tick();
    EXPECT_EQ(webrtc().mDevices->mRenderDeviceSets, 1);
    EXPECT_EQ(webrtc().mDevices->mCaptureDeviceSets, 1);

    capture.emplace_back("Mic 2", "mic-2");
    mClient->OnDevicesChanged(render, capture);
    tick();
    EXPECT_EQ(webrtc().mDevices->mRenderDeviceSets, 1);
    EXPECT_EQ(webrtc().mDevices->mCaptureDeviceSets, 2);
}

TEST_F(LLVoiceWebRTCTest, DeviceSettingsUnavailableWithoutBothKindsOfDevice)
{
    mClient->OnDevicesChanged(LLWebRTCVoiceDeviceList{ LLWebRTCVoiceDevice("Speakers", "speakers-1") }, {});
    tick();
    EXPECT_FALSE(mClient->deviceSettingsAvailable());
}

TEST_F(LLVoiceWebRTCTest, AudioProcessingConfigIsAppliedOnlyWhenItChanges)
{
    enableVoice();
    mClient->updateSettings();
    auto& configs = webrtc().mDevices->mAudioConfigs;
    ASSERT_EQ(configs.size(), 1u);
    EXPECT_TRUE(configs.back().mEchoCancellation);
    EXPECT_TRUE(configs.back().mAGC);
    EXPECT_EQ(configs.back().mNoiseSuppressionLevel,
              llwebrtc::LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_VERY_HIGH);

    mClient->updateSettings();
    EXPECT_EQ(configs.size(), 1u);

    gSavedSettings.setBOOL("VoiceEchoCancellation", false);
    mClient->updateSettings();
    ASSERT_EQ(configs.size(), 2u);
    EXPECT_FALSE(configs.back().mEchoCancellation);
    EXPECT_TRUE(configs.back().mAGC);

    gSavedSettings.setU32("VoiceNoiseSuppressionLevel", 1);
    mClient->updateSettings();
    ASSERT_EQ(configs.size(), 3u);
    EXPECT_EQ(configs.back().mNoiseSuppressionLevel,
              llwebrtc::LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_LOW);
}

TEST_F(LLVoiceWebRTCTest, AudioProcessingConfigIsReappliedAfterWebRTCRestarts)
{
    enableVoice();
    mClient->updateSettings();
    ASSERT_EQ(webrtc().mDevices->mAudioConfigs.size(), 1u);

    mClient->terminate();
    mClient->userAuthorized("", AGENT_ID); // restarts webrtc
    ASSERT_EQ(webrtc().mInitCalls, 2);
    ASSERT_TRUE(webrtc().mDevices->mAudioConfigs.empty());

    mClient->updateSettings();
    EXPECT_EQ(webrtc().mDevices->mAudioConfigs.size(), 1u);
}

TEST_F(LLVoiceWebRTCTest, TuningModeAndLevels)
{
    EXPECT_FALSE(mClient->inTuningMode());
    mClient->tuningStart();
    EXPECT_TRUE(mClient->inTuningMode());
    EXPECT_TRUE(webrtc().mDevices->mTuningMode);

    mClient->tuningSetMicVolume(0.25f);
    EXPECT_FLOAT_EQ(webrtc().mDevices->mTuningMicGain, 0.25f);

    webrtc().mDevices->mTuningAudioLevel = 10.f;
    EXPECT_FLOAT_EQ(mClient->tuningGetEnergy(), 0.8f - 0.01f * 10.f);

    mClient->tuningStop();
    EXPECT_FALSE(mClient->inTuningMode());
    EXPECT_FALSE(webrtc().mDevices->mTuningMode);
}

TEST_F(LLVoiceWebRTCTest, MicGainIsPassedOnWhenItChanges)
{
    mClient->setMicGain(0.7f);
    EXPECT_FLOAT_EQ(webrtc().mDevices->mMicGain, 0.7f);
}

//============================================================================
// Microphone mute

TEST_F(LLVoiceWebRTCTest, MutingFadesOutButUnmutingIsImmediate)
{
    mClient->setMuteMic(true);
    mClient->setMuteMic(false);
    auto& calls = webrtc().mDevices->mMuteCalls;
    ASSERT_EQ(calls.size(), 2u);
    EXPECT_TRUE(calls[0].mMute);
    EXPECT_GT(calls[0].mDelayMs, 0);
    EXPECT_FALSE(calls[1].mMute);
    EXPECT_EQ(calls[1].mDelayMs, 0);
}

TEST_F(LLVoiceWebRTCTest, MuteInTuningModeOnlyReachesTheDeviceWhenCaptureStopsOnMute)
{
    FakePeerConnection* connection = connectAdHoc();
    size_t audio_mutes = connection->mAudio.mMuteCalls.size();
    mClient->tuningStart();

    mClient->setMuteMic(true);
    EXPECT_TRUE(webrtc().mDevices->mMuteCalls.empty());

    gSavedSettings.setS32("StopMicCaptureWhenMuted", 1);
    mClient->setMuteMic(true);
    EXPECT_EQ(webrtc().mDevices->mMuteCalls.size(), 1u);

    // Either way, tuning leaves the connections alone.
    EXPECT_EQ(connection->mAudio.mMuteCalls.size(), audio_mutes);
}

TEST_F(LLVoiceWebRTCTest, AdHocConnectionFollowsTheMic)
{
    FakePeerConnection* connection = connectAdHoc();
    // Muted while being set up, then the mic state is applied.
    EXPECT_THAT(connection->mAudio.mMuteCalls, ElementsAre(true, false));

    mClient->setMuteMic(true);
    EXPECT_TRUE(connection->mAudio.mMuted);
    mClient->setMuteMic(false);
    EXPECT_FALSE(connection->mAudio.mMuted);
}

TEST_F(LLVoiceWebRTCTest, ConnectionComesUpMutedIfTheMicIsMuted)
{
    mClient->setMuteMic(true);
    FakePeerConnection* connection = connectAdHoc();
    EXPECT_TRUE(connection->mAudio.mMuted);
}

TEST_F(LLVoiceWebRTCTest, SpeakerVolumeReachesConnections)
{
    mClient->setVoiceVolume(0.4f);
    FakePeerConnection* connection = connectAdHoc();
    EXPECT_FLOAT_EQ(connection->mAudio.mReceiveVolume, 0.4f);

    mClient->setVoiceVolume(0.9f);
    EXPECT_FLOAT_EQ(connection->mAudio.mReceiveVolume, 0.9f);
}

//============================================================================
// Bringing up a connection

TEST_F(LLVoiceWebRTCTest, NewSessionInitializesAPeerConnectionWithTheGridsStunServers)
{
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    EXPECT_EQ(connection->mInitializeCalls, 1);
    EXPECT_NE(connection->mObserver, nullptr);
    ASSERT_EQ(connection->mOptions.mServers.size(), 1u);
    EXPECT_THAT(connection->mOptions.mServers[0].mUrls,
                ElementsAre("stun:stun1.aditi.secondlife.io:3478", "stun:stun2.aditi.secondlife.io:3478"));
}

TEST_F(LLVoiceWebRTCTest, AgniHasThreeStunServers)
{
    world().mGridLoginID = "Agni";
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    ASSERT_EQ(connection->mOptions.mServers.size(), 1u);
    EXPECT_THAT(connection->mOptions.mServers[0].mUrls,
                ElementsAre("stun:stun1.agni.secondlife.io:3478", "stun:stun2.agni.secondlife.io:3478",
                            "stun:stun3.agni.secondlife.io:3478"));
}

TEST_F(LLVoiceWebRTCTest, AdHocOfferIsSentToTheRegionsProvisioningCapability)
{
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    EXPECT_TRUE(world().mPosts.empty()) << "nothing to send before there's an offer";

    connection->offer("v=0 the offer");
    ASSERT_TRUE(tickUntil([] { return !world().mPosts.empty(); }));

    auto posts = world().postsTo(TestWorld::provisionUrl(REGION_ID));
    ASSERT_EQ(posts.size(), 1u);
    const LLSD& body = posts[0]->mBody;
    EXPECT_EQ(body["jsep"]["type"].asString(), "offer");
    EXPECT_EQ(body["jsep"]["sdp"].asString(), "v=0 the offer");
    EXPECT_EQ(body["channel_type"].asString(), "multiagent");
    EXPECT_EQ(body["channel"].asString(), ADHOC_CHANNEL);
    EXPECT_EQ(body["credentials"].asString(), "channel-credentials");
    EXPECT_EQ(body["voice_server_type"].asString(), "webrtc");
}

TEST_F(LLVoiceWebRTCTest, ParcelOfferAsksForTheLocalChannelOfTheParcel)
{
    Access::startParcel(*mClient, REGION_ID.asString() + "-12", 12);
    tick();
    FakePeerConnection* connection = webrtc().latest();
    ASSERT_NE(connection, nullptr);
    connection->offer();
    ASSERT_TRUE(tickUntil([] { return !world().mPosts.empty(); }));

    const LLSD& body = world().mPosts[0]->mBody;
    EXPECT_EQ(body["channel_type"].asString(), "local");
    EXPECT_EQ(body["parcel_local_id"].asInteger(), 12);
    EXPECT_FALSE(body.has("credentials"));
}

TEST_F(LLVoiceWebRTCTest, TheSimulatorsAnswerIsHandedToWebRTC)
{
    world().mHttpHandler = [](const std::string&, const LLSD&) -> std::optional<LLSD>
    { return provisionAnswer(VIEWER_SESSION, "v=0 the answer"); };
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    connection->offer();
    ASSERT_TRUE(tickUntil([&] { return !connection->mAnswers.empty(); }));
    EXPECT_THAT(connection->mAnswers, ElementsAre("v=0 the answer"));
}

TEST_F(LLVoiceWebRTCTest, NoRequestIsMadeUntilTheRegionHasCapabilities)
{
    world().findRegion(REGION_ID)->mCapabilitiesReceived = false;
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    connection->offer();
    tick(5);
    EXPECT_TRUE(world().mPosts.empty());

    world().findRegion(REGION_ID)->mCapabilitiesReceived = true;
    EXPECT_TRUE(tickUntil([&] { return !connection->mAnswers.empty(); }));
}

TEST_F(LLVoiceWebRTCTest, UnusableProvisioningAnswerDropsTheConnection)
{
    world().mHttpHandler = [](const std::string&, const LLSD& body) -> std::optional<LLSD>
    { return httpSuccess(LLSD::emptyMap()); }; // no jsep, no viewer_session
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    connection->offer();
    EXPECT_TRUE(tickUntil([&] { return connection->mShutdownCalls > 0; }));
    EXPECT_TRUE(connection->mAnswers.empty());
}

TEST_F(LLVoiceWebRTCTest, EstablishedSessionReportsLoggedInAndJoined)
{
    FakePeerConnection* connection = connectAdHoc();
    EXPECT_EQ(Access::currentChannel(*mClient), ADHOC_CHANNEL);
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_LOGGED_IN));
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_JOINED));
    ASSERT_FALSE(mStatus.mChannelInfos.empty());
    EXPECT_EQ(mStatus.mChannelInfos.back()["channel_uri"].asString(), ADHOC_CHANNEL);
    EXPECT_EQ(mStatus.mChannelInfos.back()["voice_server_type"].asString(), "webrtc");

    // The agent is a participant of its own session.
    EXPECT_TRUE(mClient->isParticipant(AGENT_ID));
    EXPECT_TRUE(world().mNameRequests.count(AGENT_ID));
}

TEST_F(LLVoiceWebRTCTest, AdHocConnectionJoinsAsPrimary)
{
    FakePeerConnection* connection = connectAdHoc();
    ASSERT_EQ(connection->mData.mSent.size(), 1u);
    EXPECT_EQ(json(connection->mData.mSent[0]), json(R"({"j":{"p":true}})"));
}

//============================================================================
// Data channel: participants and their state

TEST_F(LLVoiceWebRTCTest, ServerAnnouncedParticipantsJoinTheSession)
{
    FakePeerConnection* connection = connectAdHoc();
    int changes = mParticipants.mChanges;

    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    EXPECT_TRUE(mClient->isParticipant(PEER_ID));
    EXPECT_GT(mParticipants.mChanges, changes);

    std::set<LLUUID> participants;
    mClient->getParticipantList(participants);
    EXPECT_EQ(participants, (std::set<LLUUID>{ AGENT_ID, PEER_ID }));

    world().resolveName(PEER_ID, "Peer Resident");
    EXPECT_EQ(mClient->getDisplayName(PEER_ID), "Peer Resident");
}

TEST_F(LLVoiceWebRTCTest, ParticipantsLeaveWhenTheServerSaysSo)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    connection->receive(participantUpdate(PEER_ID, R"({"l":true})"));
    tick();
    EXPECT_FALSE(mClient->isParticipant(PEER_ID));
}

TEST_F(LLVoiceWebRTCTest, TheAgentIsNeverRemovedByALeave)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(AGENT_ID, R"({"l":true})"));
    tick();
    EXPECT_TRUE(mClient->isParticipant(AGENT_ID));
}

TEST_F(LLVoiceWebRTCTest, PowerAndVoiceActivityUpdates)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();

    // Power is sent as level * 128.
    connection->receive(participantUpdate(PEER_ID, R"({"p":64,"v":true})"));
    tick();
    EXPECT_TRUE(mClient->getIsSpeaking(PEER_ID));
    EXPECT_FLOAT_EQ(mClient->getCurrentPower(PEER_ID), 0.5f);

    // Power only counts while speaking.
    connection->receive(participantUpdate(PEER_ID, R"({"v":false})"));
    tick();
    EXPECT_FALSE(mClient->getIsSpeaking(PEER_ID));
    EXPECT_FLOAT_EQ(mClient->getCurrentPower(PEER_ID), 0.f);
}

TEST_F(LLVoiceWebRTCTest, ModeratorMuteOfAnAdHocParticipant)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    connection->receive(participantUpdate(PEER_ID, R"({"m":true})"));
    tick();
    EXPECT_TRUE(mClient->getIsModeratorMuted(PEER_ID));
    connection->receive(participantUpdate(PEER_ID, R"({"m":false})"));
    tick();
    EXPECT_FALSE(mClient->getIsModeratorMuted(PEER_ID));
}

TEST_F(LLVoiceWebRTCTest, UpdatesForUnknownParticipantsDontAddThem)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"p":100,"v":true})"));
    tick();
    EXPECT_FALSE(mClient->isParticipant(PEER_ID));
}

TEST_F(LLVoiceWebRTCTest, MalformedDataIsIgnored)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"), true); // binary
    connection->receive("not json");
    connection->receive("[1,2,3]");
    connection->receive("{\"not-a-uuid\":{\"j\":{}}}");
    connection->receive(participantUpdate(LLUUID::null, R"({"j":{}})"));
    connection->receive(participantUpdate(OTHER_PEER_ID, "42"));
    connection->receive(participantUpdate(PEER_ID, R"({"j":7,"p":"loud","v":1,"m":"yes"})"));
    tick();

    std::set<LLUUID> participants;
    mClient->getParticipantList(participants);
    EXPECT_EQ(participants, std::set<LLUUID>{ AGENT_ID });

    // Still working afterwards.
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    EXPECT_TRUE(mClient->isParticipant(PEER_ID));
}

TEST_F(LLVoiceWebRTCTest, JoiningParticipantsGetTheAgentsMutesAndVolumes)
{
    world().mVoiceMuted.insert(PEER_ID);
    world().mSpeakerVolumes[OTHER_PEER_ID] = 0.3f;
    FakePeerConnection* connection = connectAdHoc();
    size_t sent = connection->mData.mSent.size();

    connection->receive("{\"" + PEER_ID.asString() + "\":{\"j\":{}},\"" + OTHER_PEER_ID.asString() + "\":{\"j\":{}}}");
    tick();

    auto messages = sentSince(connection, sent);
    ASSERT_EQ(messages.size(), 1u);
    boost::json::object expected;
    expected["m"]  = boost::json::object{ { PEER_ID.asString(), true } };
    expected["ug"] = boost::json::object{ { OTHER_PEER_ID.asString(), 60 } }; // stored volume * 200
    EXPECT_EQ(messages[0], boost::json::value(expected));

    // The participant's volume comes from storage too.
    EXPECT_FLOAT_EQ(mClient->getUserVolume(OTHER_PEER_ID), 0.3f);
}

TEST_F(LLVoiceWebRTCTest, ServerVersionIsReportedPerSessionType)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(AGENT_ID, R"({"V":"janus-1.2.3"})"));
    tick();
    EXPECT_EQ(mClient->getVersion().serverVersion, "janus-1.2.3");
    EXPECT_EQ(mClient->getVersion().mBuildVersion, "ad-hoc");

    // Version strings about anyone else are ignored.
    connection->receive(participantUpdate(PEER_ID, R"({"V":"other"})"));
    tick();
    EXPECT_EQ(mClient->getVersion().serverVersion, "janus-1.2.3");
}

TEST_F(LLVoiceWebRTCTest, P2PCallIsJoinedWhenThePeerJoinsAndEndsWhenThePeerLeaves)
{
    FakePeerConnection* connection = connectAdHoc(ADHOC_CHANNEL, true, true);
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_LOGGED_IN));
    EXPECT_FALSE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_JOINED));

    connection->receive(participantUpdate(AGENT_ID, R"({"V":"v"})"));
    tick();
    EXPECT_EQ(mClient->getVersion().mBuildVersion, "p2p");

    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_JOINED));

    connection->receive(participantUpdate(PEER_ID, R"({"l":true})"));
    tick();
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL));
}

//============================================================================
// Per-participant volume and mute

TEST_F(LLVoiceWebRTCTest, UserVolumeIsStoredAndSentToTheServer)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    size_t sent = connection->mData.mSent.size();

    mClient->setUserVolume(PEER_ID, 0.8f);
    EXPECT_FLOAT_EQ(mClient->getUserVolume(PEER_ID), 0.8f);
    ASSERT_TRUE(world().mSpeakerVolumes.count(PEER_ID));
    EXPECT_FLOAT_EQ(world().mSpeakerVolumes[PEER_ID], 0.8f);

    auto messages = sentSince(connection, sent);
    ASSERT_EQ(messages.size(), 1u);
    boost::json::object expected;
    expected["ug"] = boost::json::object{ { PEER_ID.asString(), (uint32_t)(0.8f * 220) } };
    EXPECT_EQ(messages[0], boost::json::value(expected));

    // Back to the default, the stored setting is dropped.
    mClient->setUserVolume(PEER_ID, LLVoiceClient::VOLUME_DEFAULT);
    EXPECT_FALSE(world().mSpeakerVolumes.count(PEER_ID));
}

TEST_F(LLVoiceWebRTCTest, UserVolumeIsClamped)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    mClient->setUserVolume(PEER_ID, 5.f);
    EXPECT_FLOAT_EQ(mClient->getUserVolume(PEER_ID), LLVoiceClient::VOLUME_MAX);
}

TEST_F(LLVoiceWebRTCTest, UserVolumeIsNotSentForStrangers)
{
    FakePeerConnection* connection = connectAdHoc();
    size_t sent = connection->mData.mSent.size();
    mClient->setUserVolume(PEER_ID, 0.8f);
    EXPECT_EQ(connection->mData.mSent.size(), sent);
    EXPECT_FLOAT_EQ(mClient->getUserVolume(PEER_ID), LLVoiceClient::VOLUME_MIN);
}

TEST_F(LLVoiceWebRTCTest, MuteListChangesAreSentToTheServer)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    size_t sent = connection->mData.mSent.size();

    // Mute flags are inverted: a set flag means "don't mute this".
    mClient->onChangeDetailed(LLMute(PEER_ID, "Peer", LLMute::AGENT, 0));
    mClient->onChangeDetailed(LLMute(PEER_ID, "Peer", LLMute::AGENT, LLMute::flagVoiceChat));
    mClient->onChangeDetailed(LLMute(PEER_ID, "Peer", LLMute::OBJECT, 0)); // not an agent: ignored

    auto messages = sentSince(connection, sent);
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0], json("{\"m\":{\"" + PEER_ID.asString() + "\":true}}"));
    EXPECT_EQ(messages[1], json("{\"m\":{\"" + PEER_ID.asString() + "\":false}}"));
}

//============================================================================
// Failures and recovery

TEST_F(LLVoiceWebRTCTest, RenegotiationRestartsTheConnection)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->renegotiationNeeded();

    // After a short randomized wait the failure is reported, the server is
    // told the session is over, and the peer connection is shut down...
    ASSERT_TRUE(tickUntil([&] { return connection->mShutdownCalls > 0; }));
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::ERROR_UNKNOWN));
    auto posts = world().postsTo(TestWorld::provisionUrl(REGION_ID));
    ASSERT_EQ(posts.size(), 2u);
    EXPECT_TRUE(posts[1]->mBody["logout"].asBoolean());
    EXPECT_EQ(posts[1]->mBody["viewer_session"].asUUID(), VIEWER_SESSION);

    // ...then the same peer connection starts over.
    EXPECT_TRUE(tickUntil([&] { return connection->mInitializeCalls == 2; }));
    EXPECT_FALSE(connection->mFreed);
    EXPECT_TRUE(bringUp(connection));
}

TEST_F(LLVoiceWebRTCTest, FullChannelIsReportedToObservers)
{
    world().mHttpHandler = [](const std::string&, const LLSD&) -> std::optional<LLSD> { return httpFailure(HTTP_CONFLICT); };
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    connection->offer();
    EXPECT_TRUE(tickUntil([&] { return mStatus.saw(LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL); }));
}

TEST_F(LLVoiceWebRTCTest, LockedChannelIsReportedToObservers)
{
    world().mHttpHandler = [](const std::string&, const LLSD&) -> std::optional<LLSD> { return httpFailure(HTTP_UNAUTHORIZED); };
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    connection->offer();
    EXPECT_TRUE(tickUntil([&] { return mStatus.saw(LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED); }));
}

TEST_F(LLVoiceWebRTCTest, FailedProvisioningIsNotRetriedInATightLoop)
{
    world().mHttpHandler = [](const std::string&, const LLSD&) -> std::optional<LLSD> { return httpFailure(HTTP_INTERNAL_SERVER_ERROR); };
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);

    // Play webrtc: every (re)initialization produces a fresh offer.
    for (int i = 0; i < 20; ++i)
    {
        if (connection->mInitializeCalls > (int)world().mPosts.size())
        {
            connection->offer();
        }
        tick();
    }
    // 20 ticks is 2s of the voice loop; retries back off from 0.5-1.5s.
    EXPECT_LE(world().mPosts.size(), 4u);
}

TEST_F(LLVoiceWebRTCTest, MissingProvisioningCapabilityIsAFailure)
{
    world().findRegion(REGION_ID)->mCapabilities.erase("ProvisionVoiceAccountRequest");
    FakePeerConnection* connection = startAdHoc();
    ASSERT_NE(connection, nullptr);
    connection->offer();
    EXPECT_TRUE(tickUntil([&] { return mStatus.mStatuses.size() > 0; }));
    EXPECT_TRUE(world().mPosts.empty());
    EXPECT_TRUE(connection->mAnswers.empty());
}

//============================================================================
// ICE trickling

TEST_F(LLVoiceWebRTCTest, IceCandidatesAreTrickledToTheSignalingCapability)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->iceCandidate("candidate:1 1 udp 2122260223 192.0.2.1 54321 typ host", "0", 0);
    connection->iceCandidate("candidate:2 1 udp 2122260223 192.0.2.2 54322 typ host", "0", 0);
    ASSERT_TRUE(tickUntil([] { return !world().postsTo(TestWorld::signalingUrl(REGION_ID)).empty(); }));

    auto posts = world().postsTo(TestWorld::signalingUrl(REGION_ID));
    ASSERT_EQ(posts.size(), 1u);
    const LLSD& body = posts[0]->mBody;
    ASSERT_EQ(body["candidates"].size(), 2);
    EXPECT_EQ(body["candidates"][0]["candidate"].asString(), "candidate:1 1 udp 2122260223 192.0.2.1 54321 typ host");
    EXPECT_EQ(body["candidates"][0]["sdpMid"].asString(), "0");
    EXPECT_EQ(body["candidates"][0]["sdpMLineIndex"].asInteger(), 0);
    EXPECT_EQ(body["viewer_session"].asUUID(), VIEWER_SESSION);
    EXPECT_EQ(body["voice_server_type"].asString(), "webrtc");

    connection->iceGatheringState(LLWebRTCSignalingObserver::ICE_GATHERING_COMPLETE);
    ASSERT_TRUE(tickUntil([] { return world().postsTo(TestWorld::signalingUrl(REGION_ID)).size() == 2; }));
    EXPECT_TRUE(world().postsTo(TestWorld::signalingUrl(REGION_ID))[1]->mBody["candidate"]["completed"].asBoolean());
}

TEST_F(LLVoiceWebRTCTest, FailedIceTrickleRestartsTheSession)
{
    FakePeerConnection* connection = connectAdHoc();
    world().mHttpHandler = [](const std::string& url, const LLSD&) -> std::optional<LLSD>
    { return url == TestWorld::signalingUrl(REGION_ID) ? httpFailure(HTTP_INTERNAL_SERVER_ERROR) : httpSuccess(); };
    connection->iceCandidate("candidate:1 1 udp 2122260223 192.0.2.1 54321 typ host");
    EXPECT_TRUE(tickUntil([&] { return connection->mShutdownCalls > 0; }));
}

//============================================================================
// Leaving

TEST_F(LLVoiceWebRTCTest, LeavingLogsOutAndReleasesThePeerConnection)
{
    FakePeerConnection* connection = connectAdHoc();
    mClient->leaveChannel(false);
    EXPECT_TRUE(Access::currentChannel(*mClient).empty());

    ASSERT_TRUE(tickUntil([&] { return connection->mFreed; }));
    auto posts = world().postsTo(TestWorld::provisionUrl(REGION_ID));
    ASSERT_EQ(posts.size(), 2u);
    EXPECT_TRUE(posts[1]->mBody["logout"].asBoolean());
    EXPECT_EQ(posts[1]->mBody["viewer_session"].asUUID(), VIEWER_SESSION);
    EXPECT_EQ(connection->mShutdownCalls, 1);
    EXPECT_FALSE(connection->mObserverSetWhenFreed) << "freed with its observer still registered";
    EXPECT_EQ(connection->mData.mObserver, nullptr);
    EXPECT_TRUE(tickUntil([] { return !Access::hasSession(ADHOC_CHANNEL); }));
    EXPECT_FALSE(mClient->isParticipant(AGENT_ID));
}

TEST_F(LLVoiceWebRTCTest, ConnectionWaitsForTheLogoutBeforeShuttingDown)
{
    FakePeerConnection* connection = connectAdHoc();
    world().mHttpHandler = nullptr; // hold the logout
    mClient->leaveChannel(false);
    tick(5);
    ASSERT_EQ(world().pendingPosts().size(), 1u);
    EXPECT_EQ(connection->mShutdownCalls, 0);

    TestWorld::completePost(world().pendingPosts()[0], httpSuccess());
    EXPECT_TRUE(tickUntil([&] { return connection->mFreed; }));
}

TEST_F(LLVoiceWebRTCTest, LeavingStopsPushToTalkWhenAsked)
{
    world().mUserPTTState = true;
    connectAdHoc();
    mClient->leaveChannel(false);
    EXPECT_TRUE(world().mUserPTTState);
    mClient->leaveChannel(true);
    EXPECT_FALSE(world().mUserPTTState);
}

TEST_F(LLVoiceWebRTCTest, StartingANewSessionLeavesTheOldOne)
{
    FakePeerConnection* first = connectAdHoc("first-channel");
    FakePeerConnection* second = connectAdHoc("second-channel");
    EXPECT_EQ(Access::currentChannel(*mClient), "second-channel");
    EXPECT_TRUE(tickUntil([&] { return first->mFreed; }));
    EXPECT_FALSE(second->mFreed);
}

//============================================================================
// Channel queries

TEST_F(LLVoiceWebRTCTest, ChannelInfoAndComparisons)
{
    EXPECT_TRUE(Access::audioSessionChannelInfo(*mClient).isUndefined());
    connectAdHoc();

    EXPECT_TRUE(mClient->isCurrentChannel(adhocChannelInfo()));
    EXPECT_FALSE(mClient->isCurrentChannel(adhocChannelInfo("another-channel")));
    LLSD vivox = adhocChannelInfo();
    vivox["voice_server_type"] = "vivox";
    EXPECT_FALSE(mClient->isCurrentChannel(vivox));

    LLSD a, b;
    a["voice_server_type"] = "webrtc";
    a["sip_uri"]           = "sip:x";
    b                      = a;
    EXPECT_TRUE(mClient->compareChannels(a, b));
    b["sip_uri"] = "sip:y";
    EXPECT_FALSE(mClient->compareChannels(a, b));

    mClient->processChannels(false);
    EXPECT_FALSE(mClient->isCurrentChannel(adhocChannelInfo()));
}

TEST_F(LLVoiceWebRTCTest, OnlyP2PCallsCanBeCalledBack)
{
    connectAdHoc(PEER_ID.asString(), true, true);
    EXPECT_TRUE(mClient->isSessionCallBackPossible(PEER_ID));
    EXPECT_FALSE(mClient->isSessionCallBackPossible(OTHER_PEER_ID));
}

//============================================================================
// Spatial voice

class LLVoiceWebRTCSpatialTest : public LLVoiceWebRTCTest
{
protected:
    void SetUp() override
    {
        LLVoiceWebRTCTest::SetUp();
        world().addRegion(NEIGHBOR_ID, REGION_ORIGIN + LLVector3d(256.0, 0.0, 0.0));
        world().addRegion(VIVOX_NEIGHBOR_ID, REGION_ORIGIN + LLVector3d(0.0, 256.0, 0.0), false);
        Access::setNeighboringRegions(*mClient, { REGION_ID, NEIGHBOR_ID, VIVOX_NEIGHBOR_ID });
    }

    // Starts estate voice; returns the connections to the agent's region
    // and the webrtc neighbor.
    std::pair<FakePeerConnection*, FakePeerConnection*> startEstate()
    {
        Access::startEstate(*mClient);
        tick();
        EXPECT_EQ(webrtc().mConnections.size(), 2u);
        return { webrtc().connection(0), webrtc().connection(1) };
    }

    std::pair<FakePeerConnection*, FakePeerConnection*> connectEstate()
    {
        auto [here, neighbor] = startEstate();
        EXPECT_TRUE(bringUp(here));
        EXPECT_TRUE(bringUp(neighbor));
        tick(); // spatial connections settle which is primary once up
        return { here, neighbor };
    }
};

TEST_F(LLVoiceWebRTCSpatialTest, EstateVoiceConnectsToTheRegionAndItsWebRTCNeighbors)
{
    auto [here, neighbor] = startEstate();
    ASSERT_NE(here, nullptr);
    ASSERT_NE(neighbor, nullptr);
    here->offer();
    neighbor->offer();
    ASSERT_TRUE(tickUntil([] { return world().mPosts.size() == 2; }));
    EXPECT_EQ(world().postsTo(TestWorld::provisionUrl(REGION_ID)).size(), 1u);
    EXPECT_EQ(world().postsTo(TestWorld::provisionUrl(NEIGHBOR_ID)).size(), 1u);
    EXPECT_EQ(world().postsTo(TestWorld::provisionUrl(REGION_ID))[0]->mBody["channel_type"].asString(), "local");
    EXPECT_FALSE(world().postsTo(TestWorld::provisionUrl(REGION_ID))[0]->mBody.has("parcel_local_id"));
}

TEST_F(LLVoiceWebRTCSpatialTest, OnlyTheAgentsRegionIsPrimary)
{
    auto [here, neighbor] = connectEstate();
    auto sent_to = [](FakePeerConnection* connection, const char* message)
    {
        auto messages = sentSince(connection, 0);
        return std::find(messages.begin(), messages.end(), json(message)) != messages.end();
    };
    EXPECT_TRUE(sent_to(here, R"({"j":{"p":true}})"));
    EXPECT_TRUE(sent_to(neighbor, R"({"j":{}})"));
    EXPECT_FALSE(sent_to(neighbor, R"({"j":{"p":true}})"));

    // Only the agent's own region hears the mic.
    mClient->setMuteMic(false);
    EXPECT_FALSE(here->mAudio.mMuted);
    EXPECT_TRUE(neighbor->mAudio.mMuted);
}

TEST_F(LLVoiceWebRTCSpatialTest, CrossingIntoTheNeighborMakesItPrimary)
{
    auto [here, neighbor] = connectEstate();
    mClient->setMuteMic(false);
    size_t neighbor_sent = neighbor->mData.mSent.size();

    world().setAgentRegion(NEIGHBOR_ID);
    tick();
    auto messages = sentSince(neighbor, neighbor_sent);
    EXPECT_NE(std::find(messages.begin(), messages.end(), json(R"({"j":{"p":true}})")), messages.end());
    EXPECT_FALSE(neighbor->mAudio.mMuted);
    EXPECT_TRUE(here->mAudio.mMuted);
}

TEST_F(LLVoiceWebRTCSpatialTest, NeighborsOutOfRangeAreDisconnected)
{
    auto [here, neighbor] = connectEstate();
    Access::setNeighboringRegions(*mClient, { REGION_ID });
    EXPECT_TRUE(tickUntil([&] { return neighbor->mFreed; }));
    EXPECT_FALSE(here->mFreed);
    EXPECT_EQ(here->mShutdownCalls, 0);
}

TEST_F(LLVoiceWebRTCSpatialTest, NeighborsThatStopSupportingWebRTCAreDisconnected)
{
    auto [here, neighbor] = connectEstate();
    world().findRegion(NEIGHBOR_ID)->mFeatures["VoiceServerType"] = "vivox";
    EXPECT_TRUE(tickUntil([&] { return neighbor->mFreed; }));
    EXPECT_FALSE(here->mFreed);
}

TEST_F(LLVoiceWebRTCSpatialTest, NewNeighborsAreConnected)
{
    connectEstate();
    const LLUUID new_neighbor("4e910000-0000-4000-8000-000000000004");
    world().addRegion(new_neighbor, REGION_ORIGIN - LLVector3d(256.0, 0.0, 0.0));
    Access::setNeighboringRegions(*mClient, { REGION_ID, NEIGHBOR_ID, new_neighbor });
    tick();
    EXPECT_EQ(webrtc().mConnections.size(), 3u);
}

TEST_F(LLVoiceWebRTCSpatialTest, ParticipantsJoinOnlyThroughTheirPrimaryServer)
{
    auto [here, neighbor] = connectEstate();
    neighbor->receive(participantUpdate(PEER_ID, R"({"j":{}})"));
    tick();
    EXPECT_FALSE(mClient->isParticipant(PEER_ID));

    neighbor->receive(participantUpdate(PEER_ID, R"({"j":{"p":true}})"));
    tick();
    EXPECT_TRUE(mClient->isParticipant(PEER_ID));
}

TEST_F(LLVoiceWebRTCSpatialTest, ModeratorMuteOfTheAgentIsReportedFromThePrimaryRegionOnly)
{
    auto [here, neighbor] = connectEstate();
    neighbor->receive(participantUpdate(AGENT_ID, R"({"m":true})"));
    tick();
    EXPECT_TRUE(world().mModerationMutedInfo.empty());
    EXPECT_FALSE(mClient->getIsModeratorMuted(AGENT_ID));

    here->receive(participantUpdate(AGENT_ID, R"({"m":true})"));
    tick();
    ASSERT_EQ(world().mModerationMutedInfo.size(), 1u);
    EXPECT_EQ(world().mModerationMutedInfo[0], std::make_pair(std::string("Estate"), true));
    EXPECT_TRUE(mClient->getIsModeratorMuted(AGENT_ID));
}

TEST_F(LLVoiceWebRTCSpatialTest, PositionIsSentToSpatialConnections)
{
    auto [here, neighbor] = connectEstate();
    size_t sent = here->mData.mSent.size();

    LLVector3d avatar = REGION_ORIGIN + LLVector3d(10.0, 20.0, 30.0);
    Access::setAvatarPosition(*mClient, avatar, LLQuaternion());
    Access::setListenerPosition(*mClient, avatar, LLQuaternion());
    mClient->sendPositionUpdate(false);

    auto messages = sentSince(here, sent);
    ASSERT_EQ(messages.size(), 1u);
    const boost::json::object& update = messages[0].as_object();
    EXPECT_EQ(update.at("sp").at("x").as_int64(), (int64_t)(avatar.mdV[VX] * 100));
    EXPECT_EQ(update.at("sp").at("y").as_int64(), (int64_t)(avatar.mdV[VY] * 100));
    EXPECT_EQ(update.at("sp").at("z").as_int64(), (int64_t)(avatar.mdV[VZ] * 100));
    EXPECT_EQ(update.at("lp"), update.at("sp"));
    EXPECT_EQ(update.at("sh").at("w").as_int64(), 100);
    EXPECT_FALSE(sentSince(neighbor, 0).empty());

    // Nothing more until something changes, unless forced.
    mClient->sendPositionUpdate(false);
    EXPECT_EQ(sentSince(here, sent).size(), 1u);
    mClient->sendPositionUpdate(true);
    EXPECT_EQ(sentSince(here, sent).size(), 2u);
}

TEST_F(LLVoiceWebRTCSpatialTest, ListenerIsTetheredToWithin50mOfTheAvatar)
{
    auto [here, neighbor] = connectEstate();
    size_t sent = here->mData.mSent.size();

    LLVector3d avatar = REGION_ORIGIN + LLVector3d(100.0, 100.0, 20.0);
    Access::setAvatarPosition(*mClient, avatar, LLQuaternion());
    Access::setListenerPosition(*mClient, avatar + LLVector3d(200.0, 0.0, 0.0), LLQuaternion());
    mClient->sendPositionUpdate(false);

    auto messages = sentSince(here, sent);
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].at("lp").at("x").as_int64(), (int64_t)((avatar.mdV[VX] + 50.0) * 100));
    EXPECT_EQ(messages[0].at("lp").at("y").as_int64(), (int64_t)(avatar.mdV[VY] * 100));
}

TEST_F(LLVoiceWebRTCSpatialTest, AdHocSessionsGetNoPositionUpdates)
{
    FakePeerConnection* connection = connectAdHoc();
    size_t sent = connection->mData.mSent.size();
    mClient->sendPositionUpdate(true);
    EXPECT_EQ(connection->mData.mSent.size(), sent);
}

TEST_F(LLVoiceWebRTCSpatialTest, HidingMutesTheMicAndUnhidingRestoresIt)
{
    auto [here, neighbor] = connectEstate();
    mClient->setMuteMic(false);
    size_t device_mutes = webrtc().mDevices->mMuteCalls.size();

    mClient->setHidden(true);
    ASSERT_EQ(webrtc().mDevices->mMuteCalls.size(), device_mutes + 1);
    EXPECT_TRUE(webrtc().mDevices->mMuteCalls.back().mMute);
    EXPECT_TRUE(here->mAudio.mMuted);

    size_t sent = here->mData.mSent.size();
    mClient->setHidden(false);
    EXPECT_FALSE(webrtc().mDevices->mMuteCalls.back().mMute);
    EXPECT_FALSE(here->mAudio.mMuted);
    EXPECT_EQ(sentSince(here, sent).size(), 1u) << "position is resent on unhiding";
}

TEST_F(LLVoiceWebRTCSpatialTest, SpatialChannelInfoSetsParcelVoiceFlags)
{
    LLParcel parcel;
    world().mAgentParcel = &parcel;
    LLSD info;
    info["channel_uri"] = REGION_ID.asString();
    EXPECT_TRUE(mClient->setSpatialChannel(info));
    EXPECT_TRUE(parcel.getParcelFlagAllowVoice());
    EXPECT_TRUE(parcel.getParcelFlagUseEstateVoiceChannel());

    info["channel_uri"] = "";
    mClient->setSpatialChannel(info);
    EXPECT_FALSE(parcel.getParcelFlagAllowVoice());
    EXPECT_FALSE(parcel.getParcelFlagUseEstateVoiceChannel());
}

TEST_F(LLVoiceWebRTCSpatialTest, SpatialChannelInfoWithoutAParcelSetsRegionVoice)
{
    LLSD info;
    info["channel_uri"] = "";
    mClient->setSpatialChannel(info);
    EXPECT_FALSE(world().agentRegion()->isVoiceEnabled());
    info["channel_uri"] = REGION_ID.asString();
    mClient->setSpatialChannel(info);
    EXPECT_TRUE(world().agentRegion()->isVoiceEnabled());
}

TEST_F(LLVoiceWebRTCSpatialTest, EstateVersionIsReportedAsEstate)
{
    auto [here, neighbor] = connectEstate();
    here->receive(participantUpdate(AGENT_ID, R"({"V":"janus-9"})"));
    tick();
    EXPECT_EQ(mClient->getVersion().serverVersion, "janus-9");
    EXPECT_EQ(mClient->getVersion().mBuildVersion, "estate");
}

//============================================================================
// Enabling, disabling and shutting down

TEST_F(LLVoiceWebRTCTest, EnablingVoiceTurnsOnTheDevicesAndActivatesTheChannel)
{
    enableVoice();
    EXPECT_TRUE(webrtc().mDevices->mVoiceEnabled);
    EXPECT_EQ(world().mVoiceChannelActivations, 1);
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED));
    EXPECT_TRUE(mClient->isVoiceWorking());
}

TEST_F(LLVoiceWebRTCTest, DisablingVoiceLeavesSessionsAndTurnsOffTheDevices)
{
    enableVoice();
    FakePeerConnection* connection = connectAdHoc();
    mClient->setVoiceEnabled(false);
    EXPECT_FALSE(webrtc().mDevices->mVoiceEnabled);
    EXPECT_EQ(world().mVoiceChannelDeactivations, 1);
    EXPECT_TRUE(mStatus.saw(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED));
    EXPECT_TRUE(Access::currentChannel(*mClient).empty());
    EXPECT_TRUE(tickUntil([&] { return connection->mFreed; }));
}

TEST_F(LLVoiceWebRTCTest, TerminateClosesConnectionsBeforeTerminatingWebRTC)
{
    FakePeerConnection* connection = connectAdHoc();
    size_t posts = world().mPosts.size();
    mClient->terminate();

    EXPECT_EQ(webrtc().mTerminateCalls, 1);
    EXPECT_EQ(connection->mShutdownCalls, 1);
    EXPECT_EQ(webrtc().mUnclosedConnectionsAtTerminate, 0u);
    EXPECT_EQ(world().mPosts.size(), posts) << "no logout round trip while quitting";
    EXPECT_TRUE(LLWebRTCVoiceClient::isShuttingDown());
    EXPECT_TRUE(LLWebRTCVoiceClient::isWebRTCTerminated());
}

TEST_F(LLVoiceWebRTCTest, TerminateDrainsEveryConnection)
{
    FakePeerConnection* adhoc = connectAdHoc();
    world().addRegion(NEIGHBOR_ID, REGION_ORIGIN + LLVector3d(256.0, 0.0, 0.0));
    Access::setNeighboringRegions(*mClient, { REGION_ID, NEIGHBOR_ID });
    Access::startEstate(*mClient); // leaves the ad-hoc session, which keeps closing
    tick();
    ASSERT_EQ(webrtc().mConnections.size(), 3u);

    mClient->terminate();
    EXPECT_EQ(webrtc().mUnclosedConnectionsAtTerminate, 0u);
    for (auto& connection : webrtc().mConnections)
    {
        EXPECT_GE(connection->mShutdownCalls, 1);
    }
    (void)adhoc;
}

TEST_F(LLVoiceWebRTCTest, TerminateGivesUpOnAConnectionThatWontClose)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->mCloseOnShutdown = false;

    auto start = std::chrono::steady_clock::now();
    mClient->terminate();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_EQ(webrtc().mTerminateCalls, 1);
    EXPECT_EQ(connection->mShutdownCalls, 1);
    EXPECT_GE(elapsed, std::chrono::milliseconds(2900));
    EXPECT_LT(elapsed, std::chrono::seconds(6));
}

TEST_F(LLVoiceWebRTCTest, ConnectionsOutlivingWebRTCDontTouchIt)
{
    FakePeerConnection* connection = connectAdHoc();
    connection->mCloseOnShutdown = false; // still open when webrtc terminates
    mClient->terminate();
    LLWebRTCVoiceClient::deleteSingleton(); // destroys the sessions and connections
    EXPECT_EQ(webrtc().mFreeCallsAfterTerminate, 0);
    mClient = LLWebRTCVoiceClient::getInstance();
    mClient->init(nullptr);
}

TEST_F(LLVoiceWebRTCTest, SettingVoiceWhileShuttingDownIsIgnored)
{
    mClient->terminate();
    size_t statuses = mStatus.mStatuses.size();
    mClient->setVoiceEnabled(true);
    EXPECT_EQ(mStatus.mStatuses.size(), statuses);
    EXPECT_EQ(world().mVoiceChannelActivations, 0);
}

TEST_F(LLVoiceWebRTCTest, SessionStartedWithoutWebRTCHasNoConnection)
{
    mClient->terminate();
    size_t connections = webrtc().mConnections.size();
    Access::startAdHoc(*mClient, adhocChannelInfo(), false, false);
    tick(3);
    EXPECT_EQ(webrtc().mConnections.size(), connections);
}

TEST_F(LLVoiceWebRTCTest, VoiceWorksAgainAfterARestart)
{
    connectAdHoc();
    mClient->terminate();
    mClient->userAuthorized("", AGENT_ID);
    EXPECT_EQ(webrtc().mInitCalls, 2);
    EXPECT_EQ(webrtc().mDevices->mObserver, static_cast<llwebrtc::LLWebRTCDevicesObserver*>(mClient));
    connectAdHoc("after-restart");
    EXPECT_EQ(Access::currentChannel(*mClient), "after-restart");
}

//============================================================================
// Stats

TEST_F(LLVoiceWebRTCTest, ConnectionStatsAreAccepted)
{
    FakePeerConnection* connection = connectAdHoc();
    llwebrtc::LLWebRTCStatsMap stats;
    stats["candidate-pair"]["currentRoundTripTime"]     = "0.042";
    stats["candidate-pair"]["availableOutgoingBitrate"] = "64000";
    stats["outbound"]["mediaSourceId"]                  = "m1";
    stats["outbound"]["packetsSent"]                    = "100";
    stats["remote-inbound"]["localId"]                  = "l1";
    stats["remote-inbound"]["packetsLost"]              = "-3";
    stats["remote-inbound"]["jitter"]                   = "0.002";
    stats["inbound"]["jitterBufferDelay"]               = "1.5";
    stats["inbound"]["jitterBufferEmittedCount"]        = "0";
    stats["inbound"]["packetsReceived"]                 = "90";
    stats["inbound"]["packetsLost"]                     = "2";
    connection->stats(stats);
    tick();
    SUCCEED();
}
