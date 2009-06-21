/** 
 * @file 
 * @brief 
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
	uri << "http://" << destination.getString() << "/state/basic/";
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

	lldebugs << "Actual region: " << content << llendl;
	lldebugs << "Expected region: " << mContent << llendl;

	if (mSharedData->checkValidity(content) &&
		(actual_region_id == expected_region_id))
	{
		mSharedData->onRegionVerified(mContent);
	}
	else if (mSharedData->shouldRetry())
	{
		retry();
	}
	else
	{
		llwarns << "Could not correctly look up region from region presence service. Region: " << mSharedData->getRegionUri() << llendl;
	}
}

void LLRegionPresenceVerifier::VerifiedDestinationResponder::retry()
{
	LLSD headers;
	headers["Cache-Control"] = "no-cache, max-age=0";
	llinfos << "Requesting region information, get uncached for region " << mSharedData->getRegionUri() << llendl;
	mSharedData->decrementRetries();
	mSharedData->getHttpClient().get(mSharedData->getRegionUri(), new RegionResponder(mSharedData), headers);
}

void LLRegionPresenceVerifier::VerifiedDestinationResponder::error(U32 status, const std::string& reason)
{
	retry();
}

