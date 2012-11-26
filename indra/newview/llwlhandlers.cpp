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
#include "llenvmanager.h"
#include "llnotificationsutil.h"

/****
 * LLEnvironmentRequest
 ****/
// static
bool LLEnvironmentRequest::initiate()
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
		cur_region->setCapabilitiesReceivedCallback(boost::bind(&LLEnvironmentRequest::onRegionCapsReceived, _1));
		return false;
	}

	return doRequest();
}

// static
void LLEnvironmentRequest::onRegionCapsReceived(const LLUUID& region_id)
{
	if (region_id != gAgent.getRegion()->getRegionID())
	{
		LL_INFOS("WindlightCaps") << "Got caps for a non-current region" << LL_ENDL;
		return;
	}

	LL_DEBUGS("WindlightCaps") << "Received region capabilities" << LL_ENDL;
	doRequest();
}

// static
bool LLEnvironmentRequest::doRequest()
{
	std::string url = gAgent.getRegion()->getCapability("EnvironmentSettings");
	if (url.empty())
	{
		LL_INFOS("WindlightCaps") << "Skipping windlight setting request - we don't have this capability" << LL_ENDL;
		// region is apparently not capable of this; don't respond at all
		return false;
	}

	LL_INFOS("WindlightCaps") << "Requesting region windlight settings via " << url << LL_ENDL;
	LLHTTPClient::get(url, new LLEnvironmentRequestResponder());
	return true;
}

/****
 * LLEnvironmentRequestResponder
 ****/
int LLEnvironmentRequestResponder::sCount = 0; // init to 0

LLEnvironmentRequestResponder::LLEnvironmentRequestResponder()
{
	mID = ++sCount;
}
/*virtual*/ void LLEnvironmentRequestResponder::result(const LLSD& unvalidated_content)
{
	LL_INFOS("WindlightCaps") << "Received region windlight settings" << LL_ENDL;

	if (mID != sCount)
	{
		LL_INFOS("WindlightCaps") << "Got superseded by another responder; ignoring..." << LL_ENDL;
		return;
	}

	LLUUID regionId;
	if( gAgent.getRegion() )
	{
		regionId = gAgent.getRegion()->getRegionID();
	}
	
	if (unvalidated_content[0]["regionID"].asUUID() != regionId )
	{
		LL_WARNS("WindlightCaps") << "Not in the region from where this data was received (wanting "
			<< regionId << " but got " << unvalidated_content[0]["regionID"].asUUID()
			<< ") - ignoring..." << LL_ENDL;
		return;
	}

	LLEnvManagerNew::getInstance()->onRegionSettingsResponse(unvalidated_content);
}
/*virtual*/ void LLEnvironmentRequestResponder::error(U32 status, const std::string& reason)
{
	LL_INFOS("WindlightCaps") << "Got an error, not using region windlight..." << LL_ENDL;
	LLEnvManagerNew::getInstance()->onRegionSettingsResponse(LLSD());
}

/****
 * LLEnvironmentApply
 ****/

clock_t LLEnvironmentApply::UPDATE_WAIT_SECONDS = clock_t(3.f);
clock_t LLEnvironmentApply::sLastUpdate = clock_t(0.f);

// static
bool LLEnvironmentApply::initiateRequest(const LLSD& content)
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
	std::string url = gAgent.getRegion()->getCapability("EnvironmentSettings");
	if (url.empty())
	{
		LL_WARNS("WindlightCaps") << "Applying windlight settings not supported" << LL_ENDL;
		return false;
	}

	LL_INFOS("WindlightCaps") << "Sending windlight settings to " << url << LL_ENDL;
	LL_DEBUGS("WindlightCaps") << "content: " << content << LL_ENDL;
	LLHTTPClient::post(url, content, new LLEnvironmentApplyResponder());
	return true;
}

/****
 * LLEnvironmentApplyResponder
 ****/
/*virtual*/ void LLEnvironmentApplyResponder::result(const LLSD& content)
{
	if (content["regionID"].asUUID() != gAgent.getRegion()->getRegionID())
	{
		LL_WARNS("WindlightCaps") << "No longer in the region where data was sent (currently "
			<< gAgent.getRegion()->getRegionID() << ", reply is from " << content["regionID"].asUUID()
			<< "); ignoring..." << LL_ENDL;
		return;
	}
	else if (content["success"].asBoolean())
	{
		LL_DEBUGS("WindlightCaps") << "Success in applying windlight settings to region " << content["regionID"].asUUID() << LL_ENDL;
		LLEnvManagerNew::instance().onRegionSettingsApplyResponse(true);
	}
	else
	{
		LL_WARNS("WindlightCaps") << "Region couldn't apply windlight settings!  Reason from sim: " << content["fail_reason"].asString() << LL_ENDL;
		LLSD args(LLSD::emptyMap());
		args["FAIL_REASON"] = content["fail_reason"].asString();
		LLNotificationsUtil::add("WLRegionApplyFail", args);
		LLEnvManagerNew::instance().onRegionSettingsApplyResponse(false);
	}
}
/*virtual*/ void LLEnvironmentApplyResponder::error(U32 status, const std::string& reason)
{
	std::stringstream msg;
	msg << reason << " (Code " << status << ")";

	LL_WARNS("WindlightCaps") << "Couldn't apply windlight settings to region!  Reason: " << msg << LL_ENDL;

	LLSD args(LLSD::emptyMap());
	args["FAIL_REASON"] = msg.str();
	LLNotificationsUtil::add("WLRegionApplyFail", args);
}
