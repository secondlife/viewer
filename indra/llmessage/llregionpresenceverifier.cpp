/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=internal$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * The following source code is PROPRIETARY AND CONFIDENTIAL. Use of
 * this source code is governed by the Linden Lab Source Code Disclosure
 * Agreement ("Agreement") previously entered between you and Linden
 * Lab. By accessing, using, copying, modifying or distributing this
 * software, you acknowledge that you have been informed of your
 * obligations under the Agreement and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llregionpresenceverifier.h"
#include "llhttpclientinterface.h"
#include <sstream>
#include "net.h"
#include "message.h"


LLRegionPresenceVerifier::RegionResponder::RegionResponder(ResponsePtr data) : mSharedData(data)
{
}


void LLRegionPresenceVerifier::RegionResponder::result(const LLSD& content)
{
	std::string host = content["private_host"].asString();
	U32 port = content["private_port"].asInteger();
	LLHost destination(host, port);
	LLUUID id = content["region_id"];

	llinfos << "Verifying " << destination.getString() << " is region " << id << llendl;

	std::stringstream uri;
	uri << "http://" << destination.getString() << "/state/basic";
	mSharedData->getHttpClient().get(uri.str(), new VerifiedDestinationResponder(mSharedData, content));
}

void LLRegionPresenceVerifier::RegionResponder::completed(
	U32 status,
	const std::string& reason,
	const LLSD& content)
{
	LLHTTPClient::Responder::completed(status, reason, content);
	
	mSharedData->onCompletedRegionRequest();
}


LLRegionPresenceVerifier::VerifiedDestinationResponder::VerifiedDestinationResponder(ResponsePtr data, const LLSD& content) : mSharedData(data), mContent(content)
{
}




void LLRegionPresenceVerifier::VerifiedDestinationResponder::result(const LLSD& content)
{
	LLUUID actual_region_id = content["region_id"];
	LLUUID expected_region_id = mContent["region_id"];

	if (mSharedData->checkValidity(content))
	{
		mSharedData->onRegionVerified(mContent);
	}
	else if ((mSharedData->shouldRetry()) && (actual_region_id != expected_region_id)) // If the region is correct, then it means we've deliberately changed the data
	{
		LLSD headers;
		headers["Cache-Control"] = "no-cache, max-age=0";
		llinfos << "Requesting region information, get uncached for region " << mSharedData->getRegionUri() << llendl;
		mSharedData->decrementRetries();
		mSharedData->getHttpClient().get(mSharedData->getRegionUri(), new RegionResponder(mSharedData), headers);
	}
	else
	{
		llwarns << "Could not correctly look up region from region presence service. Region: " << mSharedData->getRegionUri() << llendl;
	}
}
