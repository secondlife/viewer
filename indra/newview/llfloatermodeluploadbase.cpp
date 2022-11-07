/**
 * @file llfloatermodeluploadbase.cpp
 * @brief LLFloaterUploadModelBase class definition
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloatermodeluploadbase.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llnotificationsutil.h"
#include "llcorehttputil.h"

LLFloaterModelUploadBase::LLFloaterModelUploadBase(const LLSD& key)
:LLFloater(key),
 mHasUploadPerm(false)
{
}

void LLFloaterModelUploadBase::requestAgentUploadPermissions()
{
    std::string capability = "MeshUploadFlag";
    std::string url = gAgent.getRegionCapability(capability);

    if (!url.empty())
    {
        LL_INFOS()<< typeid(*this).name()
                  << "::requestAgentUploadPermissions() requesting for upload model permissions from: "
                  << url << LL_ENDL;
        LLCoros::instance().launch("LLFloaterModelUploadBase::requestAgentUploadPermissionsCoro",
            boost::bind(&LLFloaterModelUploadBase::requestAgentUploadPermissionsCoro, this, url, getPermObserverHandle()));
    }
    else
    {
        LLSD args;
        args["CAPABILITY"] = capability;
        LLNotificationsUtil::add("RegionCapabilityRequestError", args);
        // BAP HACK avoid being blocked by broken server side stuff
        mHasUploadPerm = true;
    }
}

void LLFloaterModelUploadBase::requestAgentUploadPermissionsCoro(std::string url,
    LLHandle<LLUploadPermissionsObserver> observerHandle)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("MeshUploadFlag", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);


    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLUploadPermissionsObserver* observer = observerHandle.get();

    if (!observer)
    { 
        LL_WARNS("MeshUploadFlag") << "Unable to get observer after call to '" << url << "' aborting." << LL_ENDL;
        return;
    }

    if (!status)
    {
        observer->setPermissonsErrorStatus(status.getStatus(), status.getMessage());
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    observer->onPermissionsReceived(result);
}
