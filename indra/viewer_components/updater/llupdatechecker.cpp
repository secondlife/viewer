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


void LLUpdateChecker::checkVersion(std::string const & hostUrl, 
								   std::string const & servicePath,
								   std::string const & channel,
								   std::string const & version,
								   std::string const & platform_version,
								   unsigned char       uniqueid[MD5HEX_STR_SIZE],
								   bool                willing_to_test)
{
	mImplementation->checkVersion(hostUrl, servicePath, channel, version, platform_version, uniqueid, willing_to_test);
}



// LLUpdateChecker::Implementation
//-----------------------------------------------------------------------------


const char * LLUpdateChecker::Implementation::sLegacyProtocolVersion = "v1.0";
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


void LLUpdateChecker::Implementation::checkVersion(std::string const & hostUrl, 
												   std::string const & servicePath,
												   std::string const & channel,
												   std::string const & version,
												   std::string const & platform_version,
												   unsigned char       uniqueid[MD5HEX_STR_SIZE],
												   bool                willing_to_test)
{
	llassert(!mInProgress);
	
	mInProgress = true;

	mHostUrl     	 = hostUrl;
	mServicePath 	 = servicePath;
	mChannel     	 = channel;
	mVersion     	 = version;
	mPlatformVersion = platform_version;
	memcpy(mUniqueId, uniqueid, MD5HEX_STR_SIZE);
	mWillingToTest   = willing_to_test;
	
	mProtocol = sProtocolVersion;

	std::string checkUrl = buildUrl(hostUrl, servicePath, channel, version, platform_version, uniqueid, willing_to_test);
	LL_INFOS("UpdaterService") << "checking for updates at " << checkUrl << LL_ENDL;
	
	mHttpClient.get(checkUrl, this);
}

void LLUpdateChecker::Implementation::completed(U32 status,
												const std::string & reason,
												const LLSD & content)
{
	mInProgress = false;	
	
	if(status != 200)
	{
		if (status == 404)
		{
			if (mProtocol == sProtocolVersion)
			{
				mProtocol = sLegacyProtocolVersion;
				std::string retryUrl = buildUrl(mHostUrl, mServicePath, mChannel, mVersion, mPlatformVersion, mUniqueId, mWillingToTest);

				LL_WARNS("UpdaterService")
					<< "update response using " << sProtocolVersion
					<< " was 404... retry with legacy protocol" << mProtocol
					<< "\n at " << retryUrl
					<< LL_ENDL;
	
				mHttpClient.get(retryUrl, this);
			}
			else
			{
				LL_WARNS("UpdaterService")
					<< "update response using " << sLegacyProtocolVersion
					<< " was 404; request failed"
					<< LL_ENDL;
				mClient.error(reason);
			}
		}
		else
		{
			LL_WARNS("UpdaterService") << "response error " << status << " (" << reason << ")" << LL_ENDL;
			mClient.error(reason);
		}
	}
	else
	{
		mClient.response(content);
	}
}


void LLUpdateChecker::Implementation::error(U32 status, const std::string & reason)
{
	mInProgress = false;
	LL_WARNS("UpdaterService") << "update check failed; " << reason << LL_ENDL;
	mClient.error(reason);
}


std::string LLUpdateChecker::Implementation::buildUrl(std::string const & hostUrl, 
													  std::string const & servicePath,
													  std::string const & channel,
													  std::string const & version,
													  std::string const & platform_version,
													  unsigned char       uniqueid[MD5HEX_STR_SIZE],
													  bool                willing_to_test)
{	
#ifdef LL_WINDOWS
	static const char * platform = "win";
#elif LL_DARWIN
    long versMin;
    Gestalt(gestaltSystemVersionMinor, &versMin);
    
    static const char *platform;
    if (versMin == 5) //OS 10.5
    {
        platform = "mac_legacy";
    }
    else 
    {
        platform = "mac";
    }
#elif LL_LINUX
	static const char * platform = "lnx";
#else
#   error "unsupported platform"
#endif
	
	LLSD path;
	path.append(servicePath);
	path.append(mProtocol);
	path.append(channel);
	path.append(version);
	path.append(platform);
	if (mProtocol != sLegacyProtocolVersion)
	{
		path.append(platform_version);
		path.append(willing_to_test ? "testok" : "testno");
		path.append((char*)uniqueid);
	}
	return LLURI::buildHTTP(hostUrl, path).asString();
}
