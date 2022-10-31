/** 
 * @file llwlhandlers.cpp
 * @brief Various classes which handle Windlight-related messaging
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llwlhandlers.h"

#include "llagent.h"
#include "llviewerregion.h"
#include "llnotificationsutil.h"
#include "llcorehttputil.h"

#include "llparcel.h"
/****
 * LLEnvironmentRequest
 ****/
// static
bool LLEnvironmentRequest::initiate(LLEnvironment::environment_apply_fn cb)
{
    LLViewerRegion* cur_region = gAgent.getRegion();

    if (!cur_region)
    {
        LL_WARNS("WindlightCaps") << "Viewer region not set yet, skipping env. settings request" << LL_ENDL;
        return false;
    }

    if (!cur_region->capabilitiesReceived())
    {
        LL_INFOS("WindlightCaps") << "Deferring windlight settings request until we've got region caps" << LL_ENDL;
        cur_region->setCapabilitiesReceivedCallback([cb](const LLUUID &region_id, LLViewerRegion* regionp) { LLEnvironmentRequest::onRegionCapsReceived(region_id, cb); });
        return false;
    }

    return doRequest(cb);
}

// static
void LLEnvironmentRequest::onRegionCapsReceived(const LLUUID& region_id, LLEnvironment::environment_apply_fn cb)
{
    if (region_id != gAgent.getRegion()->getRegionID())
    {
        LL_INFOS("WindlightCaps") << "Got caps for a non-current region" << LL_ENDL;
        return;
    }

    LL_DEBUGS("WindlightCaps") << "Received region capabilities" << LL_ENDL;
    doRequest(cb);
}

// static
bool LLEnvironmentRequest::doRequest(LLEnvironment::environment_apply_fn cb)
{
    std::string url = gAgent.getRegionCapability("EnvironmentSettings");
    if (url.empty())
    {
        LL_INFOS("WindlightCaps") << "Skipping windlight setting request - we don't have this capability" << LL_ENDL;
        // region is apparently not capable of this; don't respond at all
        // (there shouldn't be any regions where this is the case... but
        LL_INFOS("ENVIRONMENT") << "No legacy windlight caps... just set the region to be the default day." << LL_ENDL;
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_REGION, LLSettingsDay::GetDefaultAssetId());
        return false;
    }

    std::string coroname =
        LLCoros::instance().launch("LLEnvironmentRequest::environmentRequestCoro",
        [url, cb]() { LLEnvironmentRequest::environmentRequestCoro(url, cb); });

    LL_INFOS("WindlightCaps") << "Requesting region windlight settings via " << url << LL_ENDL;
    return true;
}

S32 LLEnvironmentRequest::sLastRequest = 0;

//static 
void LLEnvironmentRequest::environmentRequestCoro(std::string url, LLEnvironment::environment_apply_fn cb)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    S32 requestId = ++LLEnvironmentRequest::sLastRequest;
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t 
            httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("EnvironmentRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LL_WARNS("WindlightCaps") << "Using legacy Windlight caps." << LL_ENDL;

    if (requestId != LLEnvironmentRequest::sLastRequest)
    {
        LL_INFOS("WindlightCaps") << "Got superseded by another responder; ignoring..." << LL_ENDL;
        return;
    }

    LLSD httpResults = result["http_result"];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        LL_WARNS("WindlightCaps") << "Got an error, not using region windlight... " << LL_ENDL;
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_REGION, LLSettingsDay::GetDefaultAssetId());

        return;
    }
    result = result["content"];
    LL_INFOS("WindlightCaps") << "Received region legacy  windlight settings" << LL_ENDL;

    LLUUID regionId;
    if (gAgent.getRegion())
    {
        regionId = gAgent.getRegion()->getRegionID();
    }

    if ((result[0]["regionID"].asUUID() != regionId) && regionId.notNull())
    {
        LL_WARNS("WindlightCaps") << "Not in the region from where this data was received (wanting "
            << regionId << " but got " << result[0]["regionID"].asUUID()
            << ") - ignoring..." << LL_ENDL;
        return;
    }

    if (cb)
    {
        LLEnvironment::EnvironmentInfo::ptr_t pinfo = LLEnvironment::EnvironmentInfo::extractLegacy(result);

        cb(INVALID_PARCEL_ID, pinfo);
    }
}


/****
 * LLEnvironmentApply
 ****/

clock_t LLEnvironmentApply::UPDATE_WAIT_SECONDS = clock_t(3.f);
clock_t LLEnvironmentApply::sLastUpdate = clock_t(0.f);

// static
bool LLEnvironmentApply::initiateRequest(const LLSD& content, LLEnvironment::environment_apply_fn cb)
{
    clock_t current = clock();

    // Make sure we don't update too frequently.
    if (current < sLastUpdate + (UPDATE_WAIT_SECONDS * CLOCKS_PER_SEC))
    {
        LLSD args(LLSD::emptyMap());
        args["WAIT"] = (F64)UPDATE_WAIT_SECONDS;
        LLNotificationsUtil::add("EnvUpdateRate", args);
        return false;
    }

    sLastUpdate = current;

    // Send update request.
    std::string url = gAgent.getRegionCapability("ExtEnvironment");
    if (url.empty())
    {
        LL_WARNS("WindlightCaps") << "Applying windlight settings not supported" << LL_ENDL;
        return false;
    }

    LL_INFOS("WindlightCaps") << "Sending windlight settings to " << url << LL_ENDL;
    LL_DEBUGS("WindlightCaps") << "content: " << content << LL_ENDL;

    std::string coroname =
        LLCoros::instance().launch("LLEnvironmentApply::environmentApplyCoro",
        [url, content, cb]() { LLEnvironmentApply::environmentApplyCoro(url, content, cb); });
    return true;
}

void LLEnvironmentApply::environmentApplyCoro(std::string url, LLSD content, LLEnvironment::environment_apply_fn cb)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("EnvironmentApply", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, content);

    LLSD notify; // for error reporting.  If there is something to report to user this will be defined.
    /*
     * Expecting reply from sim in form of:
     * {
     *   regionID : uuid,
     *   messageID: uuid,
     *   success : true
     * }
     * or
     * {
     *   regionID : uuid,
     *   success : false,
     *   fail_reason : string
     * }
     */

    do // while false.  
    {  // Breaks from loop in the case of an error.

        LLSD httpResults = result["http_result"];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
        if (!status)
        {
            LL_WARNS("WindlightCaps") << "Couldn't apply windlight settings to region! " << LL_ENDL;

            std::stringstream msg;
            msg << status.toString() << " (Code " << status.toTerseString() << ")";
            notify = LLSD::emptyMap();
            notify["FAIL_REASON"] = msg.str();
            break;
        }

        if (!result.has("regionID"))
        {
            notify = LLSD::emptyMap();
            notify["FAIL_REASON"] = "Missing regionID, malformed response";
            break;
        } 
        else if (result["regionID"].asUUID() != gAgent.getRegion()->getRegionID())
        {
            // note that there is no report to the user in this failure case.
            LL_WARNS("WindlightCaps") << "No longer in the region where data was sent (currently "
                << gAgent.getRegion()->getRegionID() << ", reply is from " << result["regionID"].asUUID()
                << "); ignoring..." << LL_ENDL;
            break;
        }
        else if (!result["success"].asBoolean())
        {
            LL_WARNS("WindlightCaps") << "Region couldn't apply windlight settings!  " << LL_ENDL;
            notify = LLSD::emptyMap();
            notify["FAIL_REASON"] = result["fail_reason"].asString();
            break;
        }

        LL_DEBUGS("WindlightCaps") << "Success in applying windlight settings to region " << result["regionID"].asUUID() << LL_ENDL;

    } while (false);

    if (!notify.isUndefined())
    {
        LLNotificationsUtil::add("WLRegionApplyFail", notify);
    }
}
