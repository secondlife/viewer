/**
 * @file _httpopsetget.cpp
 * @brief Definitions for internal class HttpOpSetGet
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "_httpopsetget.h"

#include "httpcommon.h"

#include "_httpservice.h"
#include "_httppolicy.h"


namespace LLCore
{


// ==================================
// HttpOpSetget
// ==================================


HttpOpSetGet::HttpOpSetGet()
	: HttpOperation(),
	  mIsGlobal(false),
	  mDoSet(false),
	  mSetting(-1),				// Nothing requested
	  mLongValue(0L)
{}


HttpOpSetGet::~HttpOpSetGet()
{}


void HttpOpSetGet::setupGet(HttpRequest::EGlobalPolicy setting)
{
	mIsGlobal = true;
	mSetting = setting;
}


void HttpOpSetGet::setupSet(HttpRequest::EGlobalPolicy setting, const std::string & value)
{
	mIsGlobal = true;
	mDoSet = true;
	mSetting = setting;
	mStrValue = value;
}


void HttpOpSetGet::stageFromRequest(HttpService * service)
{
	HttpPolicyGlobal & pol_opt(service->getPolicy().getGlobalOptions());
	HttpRequest::EGlobalPolicy setting(static_cast<HttpRequest::EGlobalPolicy>(mSetting));
	
	if (mDoSet)
	{
		mStatus = pol_opt.set(setting, mStrValue);
	}
	if (mStatus)
	{
		const std::string * value(NULL);
		if ((mStatus = pol_opt.get(setting, &value)))
		{
			mStrValue = *value;
		}
	}
	
	addAsReply();
}


}   // end namespace LLCore

		
