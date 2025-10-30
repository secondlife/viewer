/**
 * @file llnearbyvoicemoderation.cpp
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llagent.h"
#include "llnotificationsutil.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoiceclient.h"
#include "llviewerobjectlist.h"

#include "llnearbyvoicemoderation.h"

LLNearbyVoiceModeration::LLNearbyVoiceModeration()
{
    // TODO: default to false, when appropriate info cap is added
    mIsNearbyChatModerator = true;
    mParcelCallbackConnection = gAgent.addParcelChangedCallback([this]() { updateModeratorStatus(); });
}

LLNearbyVoiceModeration::~LLNearbyVoiceModeration()
{
    if (mParcelCallbackConnection.connected())
    {
        mParcelCallbackConnection.disconnect();
    }
}

LLVOAvatar* LLNearbyVoiceModeration::getVOAvatarFromId(const LLUUID& agent_id)
{
    LLViewerObject *obj = gObjectList.findObject(agent_id);
    while (obj && obj->isAttachment())
    {
        obj = (LLViewerObject*)obj->getParent();
    }

    if (obj && obj->isAvatar())
    {
        return (LLVOAvatar*)obj;
    }
    else
    {
        return NULL;
    }
}

const std::string LLNearbyVoiceModeration::getCapUrlFromRegion(LLViewerRegion* region)
{
    if (! region || ! region->capabilitiesReceived())
    {
        return std::string();
    }

    std::string url = region->getCapability("SpatialVoiceModerationRequest");
    if (url.empty())
    {
        LL_INFOS() << "Capability URL for region " << region->getName() << " is empty" << LL_ENDL;
        return std::string();
    }
    LL_INFOS() << "Capability URL for region " << region->getName() << " is " << url << LL_ENDL;

    return url;
}

void LLNearbyVoiceModeration::requestMuteIndividual(const LLUUID& agent_id, bool mute)
{
    LLVOAvatar* avatar = getVOAvatarFromId(agent_id);
    if (avatar)
    {
        const std::string cap_url = getCapUrlFromRegion(avatar->getRegion());
        if (cap_url.length())
        {
            const std::string operand = mute ? "mute" : "unmute";

            LLSD body;
            body["operand"] = operand;
            body["agent_id"] = agent_id;
            body["moderator_id"] = gAgent.getID();

            const std::string agent_name = avatar->getFullname();
            LL_INFOS() << "Resident " << agent_name
                       << " (" << agent_id << ")" << " applying " << operand << LL_ENDL;

            std::string success_msg =
                STRINGIZE("Resident " << agent_name
                          << " (" << agent_id << ")" << " nearby voice was set to " << operand);

            std::string failure_msg =
                STRINGIZE("Unable to change voice muting for resident "
                          << agent_name << " (" << agent_id << ")");

            LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(
                cap_url,
                body,
                success_msg,
                failure_msg);
        }
    }
}

void LLNearbyVoiceModeration::requestMuteAll(bool mute)
{
    // Use our own avatar to get the region name
    LLViewerRegion* region = gAgent.getRegion();

    const std::string cap_url = getCapUrlFromRegion(region);
    if (cap_url.length())
    {
        const std::string operand = mute ? "mute_all" : "unmute_all";

        LLSD body;
        body["operand"] = operand;
        body["moderator_id"] = gAgent.getID();

        LL_INFOS() << "For all residents in this region, applying: " << operand << LL_ENDL;

        std::string success_msg =
            STRINGIZE("Nearby voice for all residents was set to: " << operand);

        std::string failure_msg =
            STRINGIZE("Unable to set nearby voice for all residents to: " << operand);

        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(
            cap_url,
            body,
            success_msg,
            failure_msg);
    }
}

void LLNearbyVoiceModeration::setMutedInfo(const std::string& channelID, bool mute)
{
    auto it = mChannelMuteMap.find(channelID);
    if (it == mChannelMuteMap.end())
    {
        if (mute)
        {
            // Channel is new and being muted
            showMutedNotification(true);
        }
        mChannelMuteMap[channelID] = mute;
    }
    else
    {
        if (it->second != mute)
        {
            // Flag changed
            showMutedNotification(mute);
            it->second = mute;
        }
    }
}

void LLNearbyVoiceModeration::showNotificationIfNeeded()
{
    if (LLVoiceClient::getInstance()->inProximalChannel() &&
        LLVoiceClient::getInstance()->getIsModeratorMuted(gAgentID))
    {
        showMutedNotification(true);
    }
}

void LLNearbyVoiceModeration::showMutedNotification(bool is_muted)
{
    // Check if the current voice channel is nearby chat
    if (LLVoiceClient::getInstance()->inProximalChannel())
    {
        LLNotificationsUtil::add(is_muted ? "NearbyVoiceMutedByModerator" : "NearbyVoiceUnmutedByModerator");
    }
}

void LLNearbyVoiceModeration::updateModeratorStatus()
{
    LL_WARNS() << "Request moderator status info" << LL_ENDL;
    // TODO: Uncomment and set correct capability name, when appropriate info cap is added
    /*
    if (LLViewerRegion* region = gAgent.getRegion())
    {

        std::string url = region->getCapability("SpatialVoiceModerationInfoRequest");
        if (!url.empty())
        {
            LLCoros::instance().launch("getModeratorStatusCoro", [url]() { getModeratorStatusCoro(url); });
        }
    }
    */
}

void LLNearbyVoiceModeration::getModeratorStatusCoro(std::string cap_url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getModeratorStatusCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOpts->setFollowRedirects(true);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, cap_url, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS() << "Failed to get nearby voice moderator info" << LL_ENDL;
        return;
    }
    else if (!result["success"].asBoolean())
    {
        LL_WARNS() << "Failed to get nearby voice moderator info: " << result["message"] << LL_ENDL;
        return;
    }

    // TODO: update the field, when appropriate info cap is added
    bool is_moderator = result["moderator"].asBoolean();
    LLNearbyVoiceModeration::getInstance()->setNearbyChatModerator(is_moderator);
}

