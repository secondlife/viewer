/**
 * @file llvoicewebrtc_test_world.cpp
 * @brief The viewer state seen by llvoicewebrtc.cpp under test.
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

#include "llvoicewebrtc_test_world.h"
#include "llvoicewebrtc_test_world_internal.h"

#include "llavatarname.h"
#include "llcorehttputil.h"
#include "llhost.h"
#include "llviewerregion.h"

namespace llvoicewebrtc_test
{

LLSD httpSuccess(const LLSD& body)
{
    LLSD result = body.isMap() ? body : LLSD::emptyMap();
    LLSD status;
    status[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_SUCCESS] = true;
    status[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_TYPE]    = LLSD::Integer(LLCore::HttpStatus::LLCORE);
    status[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_STATUS]  = LLSD::Integer(LLCore::HE_SUCCESS);
    result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]         = status;
    return result;
}

LLSD httpFailure(int http_status)
{
    LLCore::HttpStatus failure(http_status);
    LLSD result = LLSD::emptyMap();
    LLSD status;
    status[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_SUCCESS] = false;
    status[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_TYPE]    = LLSD::Integer(failure.getType());
    status[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_STATUS]  = LLSD::Integer(failure.getStatus());
    result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS]         = status;
    return result;
}

TestWorld& TestWorld::get()
{
    static TestWorld sWorld;
    return sWorld;
}

void TestWorld::reset()
{
    mAgentRegion = nullptr;
    for (auto& region : mRegions)
    {
        delete region->mRegion;
    }
    mRegions.clear();
    mAgentParcel = nullptr;

    // Posts still pending belong to coroutines that may yet run; complete
    // them with a failure so they unwind rather than wait forever.
    for (auto& post : mPosts)
    {
        if (!post->mCompleted)
        {
            completePost(post, httpFailure(HTTP_SERVICE_UNAVAILABLE));
        }
    }
    mPosts.clear();
    mHttpHandler = nullptr;

    mVoiceEnabled = true;
    mUserPTTState = false;
    mGridLoginID  = "aditi";
    mVoiceMuted.clear();
    mSpeakerVolumes.clear();
    mMuteListObservers         = 0;
    mVoiceChannelActivations   = 0;
    mVoiceChannelDeactivations = 0;
    mModerationMutedInfo.clear();
    mNameRequests.clear();
    mNameCallbacks.clear();
}

LLViewerRegion* TestWorld::addRegion(const LLUUID& id, const LLVector3d& origin, bool webrtc)
{
    auto region     = std::make_unique<FakeRegion>();
    region->mID     = id;
    region->mOrigin = origin;
    region->mCapabilities["ProvisionVoiceAccountRequest"] = provisionUrl(id);
    region->mCapabilities["VoiceSignalingRequest"]        = signalingUrl(id);
    region->mFeatures["VoiceServerType"]                  = webrtc ? "webrtc" : "vivox";
    // The stubbed constructor builds an empty region; everything the voice
    // client asks of it is answered from the FakeRegion.
    region->mRegion = new LLViewerRegion(0, LLHost(), 0, 0, 256.f);
    region->mRegion->setRegionFlag(REGION_FLAGS_ALLOW_VOICE, true);
    LLViewerRegion* result = region->mRegion;
    mRegions.push_back(std::move(region));
    return result;
}

FakeRegion* TestWorld::findRegion(const LLUUID& id)
{
    for (auto& region : mRegions)
    {
        if (region->mID == id)
        {
            return region.get();
        }
    }
    return nullptr;
}

FakeRegion* TestWorld::findRegion(const LLViewerRegion* viewer_region)
{
    for (auto& region : mRegions)
    {
        if (region->mRegion == viewer_region)
        {
            return region.get();
        }
    }
    return nullptr;
}

LLViewerRegion* TestWorld::regionAt(const LLVector3d& pos)
{
    for (auto& region : mRegions)
    {
        if (pos.mdV[VX] >= region->mOrigin.mdV[VX] && pos.mdV[VX] < region->mOrigin.mdV[VX] + 256.0 &&
            pos.mdV[VY] >= region->mOrigin.mdV[VY] && pos.mdV[VY] < region->mOrigin.mdV[VY] + 256.0)
        {
            return region->mRegion;
        }
    }
    return nullptr;
}

void TestWorld::setAgentRegion(const LLUUID& id)
{
    FakeRegion* region = findRegion(id);
    mAgentRegion       = region ? region->mRegion : nullptr;
}

std::string TestWorld::provisionUrl(const LLUUID& region_id)
{
    return "https://sim.test/" + region_id.asString() + "/ProvisionVoiceAccountRequest";
}

std::string TestWorld::signalingUrl(const LLUUID& region_id)
{
    return "https://sim.test/" + region_id.asString() + "/VoiceSignalingRequest";
}

std::vector<HttpPostPtr> TestWorld::postsTo(const std::string& url) const
{
    std::vector<HttpPostPtr> result;
    for (auto& post : mPosts)
    {
        if (post->mUrl == url)
        {
            result.push_back(post);
        }
    }
    return result;
}

std::vector<HttpPostPtr> TestWorld::pendingPosts() const
{
    std::vector<HttpPostPtr> result;
    for (auto& post : mPosts)
    {
        if (!post->mCompleted)
        {
            result.push_back(post);
        }
    }
    return result;
}

void TestWorld::completePost(const HttpPostPtr& post, const LLSD& result)
{
    post->mResult    = result;
    post->mCompleted = true;
}

void TestWorld::resolveName(const LLUUID& id, const std::string& display_name)
{
    mNameRequests.erase(id);
    auto found = mNameCallbacks.find(id);
    if (found == mNameCallbacks.end())
    {
        return;
    }
    std::shared_ptr<NameCallbacks> callbacks = found->second;
    mNameCallbacks.erase(found);

    LLAvatarName name;
    name.fromString(display_name); // stubbed to set just the display name
    callbacks->mSignal(id, name);
}

} // namespace llvoicewebrtc_test
