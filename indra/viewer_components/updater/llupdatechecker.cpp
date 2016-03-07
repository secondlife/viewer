/** 
 * @file llupdaterservice.cpp
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "linden_common.h"
#include <stdexcept>
#include <boost/format.hpp>
#include "llhttpclient.h"
#include "llsd.h"
#include "llupdatechecker.h"
#include "lluri.h"
#if LL_DARWIN
#include <CoreServices/CoreServices.h>
#endif

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif


class LLUpdateChecker::CheckError:
	public std::runtime_error
{
public:
	CheckError(const char * message):
		std::runtime_error(message)
	{
		; // No op.
	}
};


// LLUpdateChecker
//-----------------------------------------------------------------------------


LLUpdateChecker::LLUpdateChecker(LLUpdateChecker::Client & client):
	mImplementation(new LLUpdateChecker::Implementation(client))
{
	; // No op.
}


void LLUpdateChecker::checkVersion(std::string const & urlBase, 
								   std::string const & channel,
								   std::string const & version,
								   std::string const & platform,
								   std::string const & platform_version,
								   unsigned char       uniqueid[MD5HEX_STR_SIZE],
								   bool                willing_to_test)
{
	mImplementation->checkVersion(urlBase, channel, version, platform, platform_version, uniqueid, willing_to_test);
}



// LLUpdateChecker::Implementation
//-----------------------------------------------------------------------------


const char * LLUpdateChecker::Implementation::sProtocolVersion = "v1.1";


LLUpdateChecker::Implementation::Implementation(LLUpdateChecker::Client & client):
	mClient(client),
	mInProgress(false),
	mProtocol(sProtocolVersion)
{
	; // No op.
}


LLUpdateChecker::Implementation::~Implementation()
{
	; // No op.
}


void LLUpdateChecker::Implementation::checkVersion(std::string const & urlBase, 
												   std::string const & channel,
												   std::string const & version,
												   std::string const & platform,
												   std::string const & platform_version,
												   unsigned char       uniqueid[MD5HEX_STR_SIZE],
												   bool                willing_to_test)
{
	if (!mInProgress)
	{
		mInProgress = true;

		mUrlBase     	 = urlBase;
		mChannel     	 = channel;
		mVersion     	 = version;
		mPlatform        = platform;
		mPlatformVersion = platform_version;
		memcpy(mUniqueId, uniqueid, MD5HEX_STR_SIZE);
		mWillingToTest   = willing_to_test;
	
		mProtocol = sProtocolVersion;

		std::string checkUrl = buildUrl(urlBase, channel, version, platform, platform_version, uniqueid, willing_to_test);
		LL_INFOS("UpdaterService") << "checking for updates at " << checkUrl << LL_ENDL;
	
		mHttpClient.get(checkUrl, this);
	}
	else
	{
		LL_WARNS("UpdaterService") << "attempting to restart a check when one is in progress; ignored" << LL_ENDL;
	}
}

void LLUpdateChecker::Implementation::httpCompleted()
{
	mInProgress = false;	

	S32 status = getStatus();
	const LLSD& content = getContent();
	const std::string& reason = getReason();
	if(status != 200)
	{
		std::string server_error;
		if ( content.has("error_code") )
		{
			server_error += content["error_code"].asString();
		}
		if ( content.has("error_text") )
		{
			server_error += server_error.empty() ? "" : ": ";
			server_error += content["error_text"].asString();
		}

		LL_WARNS("UpdaterService") << "response error " << status
								   << " " << reason
								   << " (" << server_error << ")"
								   << LL_ENDL;
		mClient.error(reason);
	}
	else
	{
		mClient.response(content);
	}
}


void LLUpdateChecker::Implementation::httpFailure()
{
	const std::string& reason = getReason();
	mInProgress = false;
	LL_WARNS("UpdaterService") << "update check failed; " << reason << LL_ENDL;
	mClient.error(reason);
}


std::string LLUpdateChecker::Implementation::buildUrl(std::string const & urlBase, 
													  std::string const & channel,
													  std::string const & version,
													  std::string const & platform,
													  std::string const & platform_version,
													  unsigned char       uniqueid[MD5HEX_STR_SIZE],
													  bool                willing_to_test)
{
	LLSD path;
	path.append(mProtocol);
	path.append(channel);
	path.append(version);
	path.append(platform);
	path.append(platform_version);
	path.append(willing_to_test ? "testok" : "testno");
	path.append((char*)uniqueid);
	return LLURI::buildHTTP(urlBase, path).asString();
}
