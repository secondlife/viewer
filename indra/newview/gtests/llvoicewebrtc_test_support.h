/**
 * @file llvoicewebrtc_test_support.h
 * @brief Shared helpers for the llvoicewebrtc.cpp GoogleTest suites.
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

#ifndef LLVOICEWEBRTC_TEST_SUPPORT_H
#define LLVOICEWEBRTC_TEST_SUPPORT_H

#include "llgtest.h"
#include "llvoicewebrtc_test_world.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "llvoicewebrtc.h"

#include "boost/json.hpp"
#include "httprequest.h"
#include "llapp.h"
#include "llerrorcontrol.h"
#include "lleventcoro.h"

#include <set>
#include <string>
#include <vector>

// llvoicewebrtc.h declares this class a friend of LLWebRTCVoiceClient but
// nothing in the viewer defines it, so the tests use it as their way in to
// the client's internals: starting sessions directly and running one pass
// of the voice loop, which needs an avatar the tests don't have.
class LLWebRTCVoiceClientCapResponder
{
public:
    using Client = LLWebRTCVoiceClient;

    // One pass of what voiceConnectionCoro() queues every 100ms.
    static void processSessionStates() { Client::sessionState::processSessionStates(); }

    static bool   hasSession(const std::string& channel_id) { return Client::sessionState::hasSession(channel_id); }
    static size_t sessionCount()
    {
        size_t count = 0;
        Client::sessionState::for_each([&count](const Client::sessionState::ptr_t&) { ++count; });
        return count;
    }
    static bool allSessionsClosed() { return Client::sessionState::allSessionsClosed(); }

    static void startEstate(Client& client) { client.startEstateSession(); }
    static void startParcel(Client& client, const std::string& channel_id, S32 parcel_local_id)
    {
        client.startParcelSession(channel_id, parcel_local_id);
    }
    static void startAdHoc(Client& client, const LLSD& channel_info, bool notify_on_first_join, bool hangup_on_last_leave)
    {
        client.startAdHocSession(channel_info, notify_on_first_join, hangup_on_last_leave);
    }

    static std::string currentChannel(Client& client) { return client.mSession ? client.mSession->mChannelID : std::string(); }
    static std::string nextChannel(Client& client) { return client.mNextSession ? client.mNextSession->mChannelID : std::string(); }

    static LLSD audioSessionChannelInfo(Client& client) { return client.getAudioSessionChannelInfo(); }

    static LL::WorkQueue::ptr_t workQueue(Client& client) { return client.mVoiceWorkQueue; }

    // updatePosition() fills these in from the avatar and camera.
    static void setNeighboringRegions(Client& client, const std::set<LLUUID>& regions) { client.mNeighboringRegions = regions; }
    static void setAvatarPosition(Client& client, const LLVector3d& position, const LLQuaternion& rotation)
    {
        client.setAvatarPosition(position, LLVector3::zero, rotation);
    }
    static void setListenerPosition(Client& client, const LLVector3d& position, const LLQuaternion& rotation)
    {
        client.setListenerPosition(position, LLVector3::zero, rotation);
        client.enforceTether();
    }
};

namespace llvoicewebrtc_test
{
using Access = LLWebRTCVoiceClientCapResponder;
using Status = LLVoiceClientStatusObserver::EStatusType;

// Process-wide setup for a test executable; register it once with
// testing::AddGlobalTestEnvironment().
class VoiceTestEnvironment : public testing::Environment
{
    // Coroutines stop at their next suspend unless the app is running, as
    // they do when the viewer quits.
    class RunningApp : public LLApp
    {
    public:
        RunningApp() { setStatus(APP_STATUS_RUNNING); }
        bool init() override { return true; }
        bool cleanup() override { return true; }
        bool frame() override { return true; }
    };

public:
    void SetUp() override
    {
        mApp = std::make_unique<RunningApp>();

        // Viewer log output, warnings and up, goes to stderr alongside the
        // test output.
        LLError::initForApplication(".", ".", true);
        LLError::setDefaultLevel(LLError::LEVEL_WARN);

        // The provisioning coroutines construct LLCore::HttpRequests (the
        // posts themselves go to the TestWorld), which need the service's
        // request queue.  Its worker thread is never started.
        LLCore::HttpRequest::createService();
    }

    void TearDown() override
    {
        LLCore::HttpRequest::destroyService();
        mApp.reset();
    }

private:
    std::unique_ptr<RunningApp> mApp;
};

class RecordingStatusObserver final : public LLVoiceClientStatusObserver
{
public:
    void onChange(EStatusType status, const LLSD& channelInfo, bool proximal) override
    {
        mStatuses.push_back(status);
        mChannelInfos.push_back(channelInfo);
    }
    bool saw(EStatusType status) const { return std::find(mStatuses.begin(), mStatuses.end(), status) != mStatuses.end(); }
    size_t count(EStatusType status) const { return std::count(mStatuses.begin(), mStatuses.end(), status); }

    std::vector<EStatusType> mStatuses;
    std::vector<LLSD>        mChannelInfos;
};

class RecordingParticipantObserver final : public LLVoiceClientParticipantObserver
{
public:
    void onParticipantsChanged() override { ++mChanges; }
    int  mChanges{ 0 };
};

// Parses a message sent over the data channel.
inline boost::json::value json(const std::string& text)
{
    boost::system::error_code ec;
    boost::json::value        value = boost::json::parse(text, ec);
    return ec ? boost::json::value() : value;
}

// The provisioning answer the simulator gives.
inline LLSD provisionAnswer(const LLUUID& viewer_session, const std::string& sdp = "v=0 fake answer")
{
    LLSD body;
    body["jsep"]["type"]     = "answer";
    body["jsep"]["sdp"]      = sdp;
    body["viewer_session"]   = viewer_session;
    return httpSuccess(body);
}

inline void pumpCoroutines()
{
    // Each suspend lets every ready coroutine run up to its next suspend.
    for (int i = 0; i < 4; ++i)
    {
        llcoro::suspend();
    }
}

// Settings the voice client reads, at their viewer defaults.
inline void resetVoiceSettings()
{
    auto declare_s32 = [](const char* name, S32 value)
    {
        if (!gSavedSettings.controlExists(name))
        {
            gSavedSettings.declareS32(name, value, "test");
        }
        gSavedSettings.setS32(name, value);
    };
    auto declare_u32 = [](const char* name, U32 value)
    {
        if (!gSavedSettings.controlExists(name))
        {
            gSavedSettings.declareU32(name, value, "test");
        }
        gSavedSettings.setU32(name, value);
    };
    auto declare_bool = [](const char* name, bool value)
    {
        if (!gSavedSettings.controlExists(name))
        {
            gSavedSettings.declareBOOL(name, value, "test");
        }
        gSavedSettings.setBOOL(name, value);
    };
    auto declare_f32 = [](const char* name, F32 value)
    {
        if (!gSavedSettings.controlExists(name))
        {
            gSavedSettings.declareF32(name, value, "test");
        }
        gSavedSettings.setF32(name, value);
    };
    auto declare_string = [](const char* name, const std::string& value)
    {
        if (!gSavedSettings.controlExists(name))
        {
            gSavedSettings.declareString(name, value, "test");
        }
        gSavedSettings.setString(name, value);
    };
    declare_s32("StopMicCaptureWhenMuted", 0);
    declare_s32("VoiceEarLocation", 0);
    declare_f32("AudioLevelMic", 1.0f);
    declare_string("VoiceInputAudioDevice", "Default");
    declare_string("VoiceOutputAudioDevice", "Default");
    declare_bool("VoiceEchoCancellation", true);
    declare_bool("VoiceAutomaticGainControl", true);
    declare_u32("VoiceNoiseSuppressionLevel", 4);
}

} // namespace llvoicewebrtc_test

#endif // LLVOICEWEBRTC_TEST_SUPPORT_H
