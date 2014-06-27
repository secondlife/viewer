/** 
 * @file llexperienceassociationresponder.cpp
 * @brief llexperienceassociationresponder implementation. This class combines 
 * a lookup for a script association and an experience details request. The first
 * is always async, but the second may be cached locally.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#include "llexperienceassociationresponder.h"
#include "llexperiencecache.h"
#include "llviewerregion.h"
#include "llagent.h"

ExperienceAssociationResponder::ExperienceAssociationResponder(ExperienceAssociationResponder::callback_t callback):mCallback(callback)
{
    ref();
}

void ExperienceAssociationResponder::fetchAssociatedExperience( const LLUUID& object_id, const LLUUID& item_id, callback_t callback )
{
    LLSD request;
    request["object-id"]=object_id;
    request["item-id"]=item_id;
    fetchAssociatedExperience(request, callback);
}

void ExperienceAssociationResponder::fetchAssociatedExperience(LLSD& request, callback_t callback)
{
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        std::string lookup_url=region->getCapability("GetMetadata"); 
        if(!lookup_url.empty())
        {
            LLSD fields;
            fields.append("experience");
            request["fields"] = fields;
            LLHTTPClient::post(lookup_url, request, new ExperienceAssociationResponder(callback));
        }
    }
}

void ExperienceAssociationResponder::httpFailure()
{
    LLSD msg;
    msg["error"]=(LLSD::Integer)getStatus();
    msg["message"]=getReason();
    LL_INFOS("ExperienceAssociation") << "Failed to look up associated experience: " << getStatus() << ": " << getReason() << LL_ENDL;

    sendResult(msg);
  
}
void ExperienceAssociationResponder::httpSuccess()
{
    if(!getContent().has("experience"))
    {

        LLSD msg;
        msg["message"]="no experience";
        msg["error"]=-1;
        sendResult(msg);
        return;
    }

    LLExperienceCache::get(getContent()["experience"].asUUID(), boost::bind(&ExperienceAssociationResponder::sendResult, this, _1));

}

void ExperienceAssociationResponder::sendResult( const LLSD& experience )
{
    mCallback(experience);
    unref();
}



