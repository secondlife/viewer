/** 
 * @file llwlhandlers.cpp
 * @brief Various classes which handle Windlight-related messaging
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

	if (unvalidated_content[0]["regionID"].asUUID() != gAgent.getRegion()->getRegionID())
	{
		LL_WARNS("WindlightCaps") << "Not in the region from where this data was received (wanting "
			<< gAgent.getRegion()->getRegionID() << " but got " << unvalidated_content[0]["regionID"].asUUID()
			<< ") - ignoring..." << LL_ENDL;
		return;
	}

	LLEnvManager::getInstance()->processIncomingMessage(unvalidated_content, LLEnvKey::SCOPE_REGION);
}
/*virtual*/ void LLEnvironmentRequestResponder::error(U32 status, const std::string& reason)
{
	LL_INFOS("WindlightCaps") << "Got an error, not using region windlight..." << LL_ENDL;
	// notify manager that region settings are undefined
	LLEnvManager::getInstance()->processIncomingMessage(LLSD(), LLEnvKey::SCOPE_REGION);
}


/****
 * LLEnvironmentApplyResponder
 ****/
clock_t LLEnvironmentApplyResponder::UPDATE_WAIT_SECONDS = clock_t(3.f);
clock_t LLEnvironmentApplyResponder::sLastUpdate = clock_t(0.f);

bool LLEnvironmentApplyResponder::initiateRequest(const LLSD& content)
{
	clock_t current = clock();
	if(current >= sLastUpdate + (UPDATE_WAIT_SECONDS * CLOCKS_PER_SEC))
	{
		sLastUpdate = current;
	}
	else
	{
		LLSD args(LLSD::emptyMap());
		args["WAIT"] = (F64)UPDATE_WAIT_SECONDS;
		LLNotificationsUtil::add("EnvUpdateRate", args);
		return false;
	}

	std::string url = gAgent.getRegion()->getCapability("EnvironmentSettings");
	if (!url.empty())
	{
		LL_INFOS("WindlightCaps") << "Sending windlight settings to " << url << LL_ENDL;
		LL_DEBUGS("WindlightCaps") << "content: " << content << LL_ENDL;
		LLHTTPClient::post(url, content, new LLEnvironmentApplyResponder());
		return true;
	}
	else
	{
		LL_WARNS("WindlightCaps") << "Applying windlight settings not supported" << LL_ENDL;
	}
	return false;
}
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
	}
	else
	{
		LL_WARNS("WindlightCaps") << "Region couldn't apply windlight settings!  Reason from sim: " << content["fail_reason"].asString() << LL_ENDL;
		LLSD args(LLSD::emptyMap());
		args["FAIL_REASON"] = content["fail_reason"].asString();
		LLNotificationsUtil::add("WLRegionApplyFail", args);
	}

	LLEnvManager::getInstance()->commitSettingsFinished(LLEnvKey::SCOPE_REGION);
}
/*virtual*/ void LLEnvironmentApplyResponder::error(U32 status, const std::string& reason)
{
	std::stringstream msg;
	msg << reason << " (Code " << status << ")";

	LL_WARNS("WindlightCaps") << "Couldn't apply windlight settings to region!  Reason: " << msg << LL_ENDL;

	LLSD args(LLSD::emptyMap());
	args["FAIL_REASON"] = msg.str();
	LLNotificationsUtil::add("WLRegionApplyFail", args);

	LLEnvManager::getInstance()->commitSettingsFinished(LLEnvKey::SCOPE_REGION);
}
