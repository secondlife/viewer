/**
 * @file llvoicewebrtc_stubs.cpp
 * @brief The parts of the viewer llvoicewebrtc.cpp uses, stubbed for tests.
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

// Definitions for everything llvoicewebrtc.cpp links against from the rest
// of newview and from llmessage, answered from the TestWorld.  The real
// headers are used, so a signature change in the viewer breaks this file at
// compile time rather than silently diverging.
//
// LLWorld, LLGridManager and LLAppViewer are never constructed here: the
// voice client reaches them through instance() accessors, and the stubbed
// members below don't touch 'this'.

#include "llviewerprecompiledheaders.h"

#include "llvoicewebrtc_test_world_internal.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llavatarname.h"
#include "llavatarnamecache.h"
#include "llcorehttputil.h"
#include "llfirstuse.h"
#include "llmutelist.h"
#include "llnearbyvoicemoderation.h"
#include "llparcel.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvoavatarself.h"
#include "llvoicechannel.h"
#include "llvoiceclient.h"
#include "llwind.h"
#include "llworld.h"
#include "lleventcoro.h"

using llvoicewebrtc_test::TestWorld;

//----------------------------------------------------------------------------
// Globals

LLControlGroup            gSavedSettings("Global");
LLUUID                    gAgentID;
LLAgent                   gAgent;
LLPointer<LLVOAvatarSelf> gAgentAvatarp;
LLAppViewer*              LLAppViewer::sInstance = nullptr;

// Without an avatar the voice client skips position tracking; tests set
// positions directly instead.
bool isAgentAvatarValid()
{
    return false;
}

LLQuaternion LLJoint::getWorldRotation()
{
    return LLQuaternion();
}

namespace LLStatViewer
{
LLTrace::SampleStatHandle<F32> WEBRTC_JITTER_BUFFER("test_webrtc_jitter_buffer");
LLTrace::SampleStatHandle<F32> WEBRTC_JITTER_IN("test_webrtc_jitter_in");
LLTrace::SampleStatHandle<F32> WEBRTC_JITTER_OUT("test_webrtc_jitter_out");
LLTrace::SampleStatHandle<F32> WEBRTC_LATENCY("test_webrtc_latency");
LLTrace::SampleStatHandle<F32> WEBRTC_UPLOAD_BANDWIDTH("test_webrtc_upload_bandwidth");
LLTrace::SampleStatHandle<U32> WEBRTC_PACKETS_IN_LOST("test_webrtc_packets_in_lost");
LLTrace::SampleStatHandle<U32> WEBRTC_PACKETS_IN_RECEIVED("test_webrtc_packets_in_received");
LLTrace::SampleStatHandle<U32> WEBRTC_PACKETS_OUT_LOST("test_webrtc_packets_out_lost");
LLTrace::SampleStatHandle<U32> WEBRTC_PACKETS_OUT_SENT("test_webrtc_packets_out_sent");
}

//----------------------------------------------------------------------------
// LLAgent

LLAgent::LLAgent()
{
}

LLAgent::~LLAgent()
{
}

LLViewerRegion* LLAgent::getRegion() const
{
    return TestWorld::get().agentRegion();
}

//----------------------------------------------------------------------------
// LLViewerRegion, answered from the matching FakeRegion

LLViewerRegion::LLViewerRegion(const U64& handle, const LLHost& host, const U32 surface_grid_width,
                               const U32 patch_grid_width, const F32 region_width_meters) :
    mImpl(nullptr),
    mRegionFlags(0)
{
}

LLViewerRegion::~LLViewerRegion()
{
}

const LLUUID& LLViewerRegion::getRegionID() const
{
    static const LLUUID sNull;
    auto region = TestWorld::get().findRegion(this);
    return region ? region->mID : sNull;
}

bool LLViewerRegion::capabilitiesReceived() const
{
    auto region = TestWorld::get().findRegion(this);
    return region && region->mCapabilitiesReceived;
}

std::string LLViewerRegion::getCapability(std::string_view name) const
{
    auto region = TestWorld::get().findRegion(this);
    if (!region)
    {
        return std::string();
    }
    auto found = region->mCapabilities.find(std::string(name));
    return found == region->mCapabilities.end() ? std::string() : found->second;
}

void LLViewerRegion::getSimulatorFeatures(LLSD& info) const
{
    auto region = TestWorld::get().findRegion(this);
    info = region ? region->mFeatures : LLSD();
}

bool LLViewerRegion::isVoiceEnabled() const
{
    return getRegionFlag(REGION_FLAGS_ALLOW_VOICE);
}

LLVector3d LLViewerRegion::getPosGlobalFromRegion(const LLVector3& offset) const
{
    auto region = TestWorld::get().findRegion(this);
    LLVector3d result(offset);
    if (region)
    {
        result += region->mOrigin;
    }
    return result;
}

const LLHost& LLViewerRegion::getHost() const
{
    static const LLHost sHost;
    return sHost;
}

std::string LLViewerRegion::getDescription() const
{
    return std::string();
}

// A member of LLViewerRegion.
LLWind::LLWind()
{
}

LLWind::~LLWind()
{
}

//----------------------------------------------------------------------------
// LLWorld

LLViewerRegion* LLWorld::getRegionFromID(const LLUUID& region_id)
{
    auto region = TestWorld::get().findRegion(region_id);
    return region ? region->mRegion : nullptr;
}

LLViewerRegion* LLWorld::getRegionFromPosGlobal(const LLVector3d& pos)
{
    return TestWorld::get().regionAt(pos);
}

//----------------------------------------------------------------------------
// Parcels.  LLParcel itself is the real one, from llinventory.

LLViewerParcelMgr::LLViewerParcelMgr()
{
}

LLViewerParcelMgr::~LLViewerParcelMgr()
{
}

LLParcel* LLViewerParcelMgr::getAgentParcel() const
{
    return TestWorld::get().mAgentParcel;
}

//----------------------------------------------------------------------------
// Grid

std::string LLGridManager::getGridLoginID()
{
    return TestWorld::get().mGridLoginID;
}

//----------------------------------------------------------------------------
// LLAppViewer / first use

void LLAppViewer::postToMainCoro(const LL::WorkQueue::Work& work)
{
    // The real one runs this on the main coroutine; nothing here depends on
    // that, and the only caller just records a first-use hint.
}

void LLFirstUse::speak(bool enable)
{
}

//----------------------------------------------------------------------------
// LLMuteList / LLMute

LLMuteList::LLMuteList()
{
}

LLMuteList::~LLMuteList()
{
}

void LLMuteList::cleanupSingleton()
{
}

void LLMuteList::addObserver(LLMuteListObserver* observer)
{
    ++TestWorld::get().mMuteListObservers;
}

bool LLMuteList::isMuted(const LLUUID& id, const std::string& name, U32 flags) const
{
    // Only voice mutes matter to the voice client.
    return (flags & LLMute::flagVoiceChat) && TestWorld::get().mVoiceMuted.count(id);
}

LLMute::LLMute(const LLUUID& id, const std::string& name, EType type, U32 flags) :
    mID(id),
    mName(name),
    mType(type),
    mFlags(flags)
{
}

//----------------------------------------------------------------------------
// LLNearbyVoiceModeration

LLNearbyVoiceModeration::LLNearbyVoiceModeration()
{
}

LLNearbyVoiceModeration::~LLNearbyVoiceModeration()
{
}

void LLNearbyVoiceModeration::setMutedInfo(const std::string& channelID, bool mute)
{
    TestWorld::get().mModerationMutedInfo.emplace_back(channelID, mute);
}

//----------------------------------------------------------------------------
// LLVoiceClient and friends, from llvoiceclient.cpp

const F32 LLVoiceClient::VOLUME_MIN     = 0.f;
const F32 LLVoiceClient::VOLUME_DEFAULT = 0.5f;
const F32 LLVoiceClient::VOLUME_MAX     = 1.0f;

LLVoiceClient::LLVoiceClient(LLPumpIO* pump) :
    mVoiceEffectEnabled(gSavedSettings, "VoiceMorphingEnabled", true),
    mVoiceEffectDefault(gSavedSettings, "VoiceEffectDefault", "")
{
}

LLVoiceClient::~LLVoiceClient()
{
}

bool LLVoiceClient::voiceEnabled()
{
    return TestWorld::get().mVoiceEnabled;
}

bool LLVoiceClient::getUserPTTState()
{
    return TestWorld::get().mUserPTTState;
}

void LLVoiceClient::setUserPTTState(bool ptt)
{
    TestWorld::get().mUserPTTState = ptt;
}

std::string LLVoiceClientStatusObserver::status2string(LLVoiceClientStatusObserver::EStatusType inStatus)
{
    return std::to_string((int)inStatus);
}

LLSpeakerVolumeStorage::LLSpeakerVolumeStorage()
{
}

LLSpeakerVolumeStorage::~LLSpeakerVolumeStorage()
{
}

void LLSpeakerVolumeStorage::cleanupSingleton()
{
}

void LLSpeakerVolumeStorage::storeSpeakerVolume(const LLUUID& speaker_id, F32 volume)
{
    TestWorld::get().mSpeakerVolumes[speaker_id] = volume;
}

bool LLSpeakerVolumeStorage::getSpeakerVolume(const LLUUID& speaker_id, F32& volume)
{
    auto& volumes = TestWorld::get().mSpeakerVolumes;
    auto  found   = volumes.find(speaker_id);
    if (found == volumes.end())
    {
        return false;
    }
    volume = found->second;
    return true;
}

void LLSpeakerVolumeStorage::removeSpeakerVolume(const LLUUID& speaker_id)
{
    TestWorld::get().mSpeakerVolumes.erase(speaker_id);
}

//----------------------------------------------------------------------------
// LLVoiceChannel: one current channel that counts (de)activations.

LLVoiceChannel::LLVoiceChannel(const LLUUID& session_id, const std::string& session_name) :
    mCallDirection(OUTGOING_CALL),
    mSessionID(session_id),
    mState(STATE_NO_CHANNEL_INFO),
    mSessionName(session_name),
    mCallEndedByAgent(false),
    mIgnoreNextSessionLeave(false)
{
}

LLVoiceChannel::~LLVoiceChannel()
{
}

LLVoiceChannel* LLVoiceChannel::getCurrentVoiceChannel()
{
    static LLVoiceChannel sChannel(LLUUID::null, "test");
    return &sChannel;
}

void LLVoiceChannel::activate()
{
    ++TestWorld::get().mVoiceChannelActivations;
}

void LLVoiceChannel::deactivate()
{
    ++TestWorld::get().mVoiceChannelDeactivations;
}

void LLVoiceChannel::onChange(EStatusType status, const LLSD& channelInfo, bool proximal) {}
void LLVoiceChannel::handleStatusChange(EStatusType status) {}
void LLVoiceChannel::handleError(EStatusType status) {}
void LLVoiceChannel::setChannelInfo(const LLSD& channelInfo) {}
void LLVoiceChannel::resetChannelInfo() {}
void LLVoiceChannel::requestChannelInfo() {}
bool LLVoiceChannel::isActive() const { return false; }
bool LLVoiceChannel::callStarted() const { return false; }
void LLVoiceChannel::setState(EState state) { mState = state; }

//----------------------------------------------------------------------------
// LLAvatarNameCache: requests are answered by TestWorld::resolveName().

LLAvatarName::LLAvatarName() :
    mExpires(0.0),
    mNextUpdate(0.0),
    mIsDisplayNameDefault(false),
    mIsTemporaryName(false)
{
}

void LLAvatarName::fromString(const std::string& full_name)
{
    mDisplayName = full_name;
}

std::string LLAvatarName::getDisplayName(bool force_use_display_name) const
{
    return mDisplayName;
}

LLAvatarNameCache::callback_connection_t LLAvatarNameCache::get(const LLUUID& agent_id, callback_slot_t slot)
{
    TestWorld& world = TestWorld::get();
    world.mNameRequests.insert(agent_id);
    auto& callbacks = world.mNameCallbacks[agent_id];
    if (!callbacks)
    {
        callbacks = std::make_shared<TestWorld::NameCallbacks>();
    }
    return callbacks->mSignal.connect(slot);
}

//----------------------------------------------------------------------------
// LLCoreHttpUtil::HttpCoroutineAdapter: posts go to the TestWorld.

const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS("http_result");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_SUCCESS("success");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_TYPE("type");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_STATUS("status");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_MESSAGE("message");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_URL("url");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS("headers");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT("content");
const std::string LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW("raw");

LLCoreHttpUtil::HttpCoroutineAdapter::HttpCoroutineAdapter(std::string name, LLCore::HttpRequest::policy_t policyId) :
    mAdapterName(name),
    mPolicyId(policyId)
{
}

LLCoreHttpUtil::HttpCoroutineAdapter::~HttpCoroutineAdapter()
{
}

LLSD LLCoreHttpUtil::HttpCoroutineAdapter::postAndSuspend(LLCore::HttpRequest::ptr_t request,
                                                         const std::string& url, const LLSD& body,
                                                         LLCore::HttpOptions::ptr_t options,
                                                         LLCore::HttpHeaders::ptr_t headers)
{
    TestWorld& world = TestWorld::get();
    auto post        = std::make_shared<llvoicewebrtc_test::HttpPost>();
    post->mUrl       = url;
    post->mBody      = body;
    world.mPosts.push_back(post);
    if (world.mHttpHandler)
    {
        if (std::optional<LLSD> result = world.mHttpHandler(url, body))
        {
            TestWorld::completePost(post, *result);
        }
    }
    do
    {
        llcoro::suspend();
    } while (!post->mCompleted);
    return post->mResult;
}

LLCore::HttpStatus LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(const LLSD& httpResults)
{
    LLCore::HttpStatus::type_enum_t type = static_cast<LLCore::HttpStatus::type_enum_t>(httpResults[HTTP_RESULTS_TYPE].asInteger());
    short code = static_cast<short>(httpResults[HTTP_RESULTS_STATUS].asInteger());
    return LLCore::HttpStatus(type, code);
}
