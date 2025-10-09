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
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llviewerobjectlist.h"

#include "llnearbyvoicemoderation.h"

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

void LLNearbyVoiceModeration::requestMuteChange(const LLUUID& agent_id, bool mute)
{
    LLVOAvatar* avatar = getVOAvatarFromId(agent_id);
    if (avatar)
    {
        LLViewerRegion* region = avatar->getRegion();
        if (! region || ! region->capabilitiesReceived())
        {
            // TODO: Retry if fails since the capabilities may not have been received
            // if this is called early into a region entry
            LL_INFOS() << "Region or region capabilities unavailable" << LL_ENDL;
            return;
        }
        LL_INFOS() << "Region name is " << region->getName() << LL_ENDL;

        std::string url = region->getCapability("SpatialVoiceModerationRequest");
        if (url.empty())
        {
            // TODO: Retry if fails since URL may not have not be available
            // if this is called early into a region entry
            LL_INFOS() << "Capability URL is empty" << LL_ENDL;
            return;
        }
        LL_INFOS() << "Capability URL is " << url << LL_ENDL;

        const std::string agent_name = avatar->getFullname();

        const std::string operand = mute ? "mute" : "unmute";

        LLSD body;
        body["operand"] = operand;
        body["agent_id"] = agent_id;
        body["moderator_id"] = gAgent.getID();

        LL_INFOS() << "Resident " << agent_name
                   << " (" << agent_id << ")" << " applying " << operand << LL_ENDL;

        std::string success_msg =
            STRINGIZE("Resident " << agent_name
                      << " (" << agent_id << ")" << " nearby voice was set to " << operand);

        std::string failure_msg =
            STRINGIZE("Unable to change voice muting for resident "
                      << agent_name << " (" << agent_id << ")");

        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, body,
                success_msg,
                failure_msg);
    }
}
