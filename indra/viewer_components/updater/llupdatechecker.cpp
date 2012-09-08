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


class LLUpdateChecker::Implementation:
	public LLHTTPClient::Responder
{
public:
	Implementation(Client & client);
	~Implementation();
	void checkVersion(std::string const & protocolVersion, std::string const & hostUrl, 
			   std::string const & servicePath, std::string channel, std::string version);
	
	// Responder:
	virtual void completed(U32 status,
						   const std::string & reason,
						   const LLSD& content);
	virtual void error(U32 status, const std::string & reason);
	
private:	
	static const char * sProtocolVersion;
	
	Client & mClient;
	LLHTTPClient mHttpClient;
	bool mInProgress;
	std::string mVersion;
	
	std::string buildUrl(std::string const & protocolVersion, std::string const & hostUrl, 
						 std::string const & servicePath, std::string channel, std::string version);

	LOG_CLASS(LLUpdateChecker::Implementation);
};



// LLUpdateChecker
//-----------------------------------------------------------------------------


LLUpdateChecker::LLUpdateChecker(LLUpdateChecker::Client & client):
	mImplementation(new LLUpdateChecker::Implementation(client))
{
	; // No op.
}


void LLUpdateChecker::checkVersion(std::string const & protocolVersion, std::string const & hostUrl, 
							std::string const & servicePath, std::string channel, std::string version)
{
	mImplementation->checkVersion(protocolVersion, hostUrl, servicePath, channel, version);
}



// LLUpdateChecker::Implementation
//-----------------------------------------------------------------------------


const char * LLUpdateChecker::Implementation::sProtocolVersion = "v1.0";


LLUpdateChecker::Implementation::Implementation(LLUpdateChecker::Client & client):
	mClient(client),
	mInProgress(false)
{
	; // No op.
}


LLUpdateChecker::Implementation::~Implementation()
{
	; // No op.
}


void LLUpdateChecker::Implementation::checkVersion(std::string const & protocolVersion, std::string const & hostUrl, 
											std::string const & servicePath, std::string channel, std::string version)
{
	llassert(!mInProgress);
	
	if(protocolVersion != sProtocolVersion) throw CheckError("unsupported protocol");
		
	mInProgress = true;
	mVersion = version;
	std::string checkUrl = buildUrl(protocolVersion, hostUrl, servicePath, channel, version);
	LL_INFOS("UpdateCheck") << "checking for updates at " << checkUrl << llendl;
	
	// The HTTP client will wrap a raw pointer in a boost::intrusive_ptr causing the
	// passed object to be silently and automatically deleted.  We pass a self-
	// referential intrusive pointer to which we add a reference to keep the
	// client from deleting the update checker implementation instance.
	LLHTTPClient::ResponderPtr temporaryPtr(this);
	boost::intrusive_ptr_add_ref(temporaryPtr.get());
	mHttpClient.get(checkUrl, temporaryPtr);
}

void LLUpdateChecker::Implementation::completed(U32 status,
							  const std::string & reason,
							  const LLSD & content)
{
	mInProgress = false;	
	
	if(status != 200) {
		LL_WARNS("UpdateCheck") << "html error " << status << " (" << reason << ")" << llendl;
		mClient.error(reason);
	} else if(!content.asBoolean()) {
		LL_INFOS("UpdateCheck") << "up to date" << llendl;
		mClient.upToDate();
	} else if(content["required"].asBoolean()) {
		LL_INFOS("UpdateCheck") << "version invalid" << llendl;
		LLURI uri(content["url"].asString());
		mClient.requiredUpdate(content["version"].asString(), uri, content["hash"].asString());
	} else {
		LL_INFOS("UpdateCheck") << "newer version " << content["version"].asString() << " available" << llendl;
		LLURI uri(content["url"].asString());
		mClient.optionalUpdate(content["version"].asString(), uri, content["hash"].asString());
	}
}


void LLUpdateChecker::Implementation::error(U32 status, const std::string & reason)
{
	mInProgress = false;
	LL_WARNS("UpdateCheck") << "update check failed; " << reason << llendl;
	mClient.error(reason);
}


std::string LLUpdateChecker::Implementation::buildUrl(std::string const & protocolVersion, std::string const & hostUrl, 
													  std::string const & servicePath, std::string channel, std::string version)
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
#else
	static const char * platform = "lnx";
#endif
	
	LLSD path;
	path.append(servicePath);
	path.append(protocolVersion);
	path.append(channel);
	path.append(version);
	path.append(platform);
	return LLURI::buildHTTP(hostUrl, path).asString();
}
