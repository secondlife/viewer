/** 
 * @file llexperienceassociationresponder.cpp
 * @brief llexperienceassociationresponder implementation
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

ExperienceAssociationResponder::ExperienceAssociationResponder(ExperienceAssociationResponder::callback_t callback):mCallback(callback)
{
    ref();
}

void ExperienceAssociationResponder::error( U32 status, const std::string& reason )
{
    LLSD msg;
    msg["error"]=(LLSD::Integer)status;
    msg["message"]=reason;
    LL_INFOS("ExperienceAssociation") << "Failed to look up associated experience: " << status << ": " << reason << LL_ENDL;

    sendResult(msg);
  
}
void ExperienceAssociationResponder::result( const LLSD& content )
{
    if(!content.has("experience"))
    {

        LLSD msg;
        msg["message"]="no experience";
        msg["error"]=-1;
        sendResult(msg);
        LL_ERRS("ExperienceAssociation")  << "Associated experience missing" << LL_ENDL;
    }

    LLExperienceCache::get(content["experience"].asUUID(), boost::bind(&ExperienceAssociationResponder::sendResult, this, _1));

    /* LLLiveLSLEditor* scriptCore = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", mParent);

    if(!scriptCore)
        return;

    LLUUID id;
    if(content.has("experience"))
    {
        id=content["experience"].asUUID();
    }
    scriptCore->setAssociatedExperience(id);*/
}

void ExperienceAssociationResponder::sendResult( const LLSD& experience )
{
    mCallback(experience);
    unref();
}



