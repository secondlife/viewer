/** 
 * @file llupdatechecker.h
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

#ifndef LL_UPDATERCHECKER_H
#define LL_UPDATERCHECKER_H


#include <boost/shared_ptr.hpp>

#include "llmd5.h"
#include "llhttpclient.h"

//
// Implements asynchronous checking for updates.
//
class LLUpdateChecker {
public:
	class Client;
	class Implementation: public LLHTTPClient::Responder
	{
	public:
		Implementation(Client & client);
		~Implementation();
		void checkVersion(std::string const & urlBase, 
						  std::string const & channel,
						  std::string const & version,
						  std::string const & platform,
						  std::string const & platform_version,
						  unsigned char       uniqueid[MD5HEX_STR_SIZE],
						  bool                willing_to_test
						  );
	
    protected:
		// Responder:
		virtual void httpCompleted();
		virtual void httpFailure();
	
	private:	
		static const char * sLegacyProtocolVersion;
		static const char * sProtocolVersion;
		const char* mProtocol;
		
		Client & mClient;
		LLHTTPClient mHttpClient;
		bool         mInProgress;
		std::string   mVersion;
		std::string   mUrlBase;
		std::string   mChannel;
		std::string   mPlatform;
		std::string   mPlatformVersion;
		unsigned char mUniqueId[MD5HEX_STR_SIZE];
		bool          mWillingToTest;
		
		std::string buildUrl(std::string const & urlBase, 
							 std::string const & channel,
							 std::string const & version,
							 std::string const & platform,
							 std::string const & platform_version,
							 unsigned char       uniqueid[MD5HEX_STR_SIZE],
							 bool                willing_to_test);

		LOG_CLASS(LLUpdateChecker::Implementation);
	};

	
	// An exception that may be raised on check errors.
	class CheckError;
	
	LLUpdateChecker(Client & client);
	
	// Check status of current app on the given host for the channel and version provided.
	void checkVersion(std::string const & urlBase, 
					  std::string const & channel,
					  std::string const & version,
					  std::string const & platform,
					  std::string const & platform_version,
					  unsigned char       uniqueid[MD5HEX_STR_SIZE],
					  bool                willing_to_test);
	
private:
	LLPointer<Implementation> mImplementation;
};


class LLURI; // From lluri.h


//
// The client interface implemented by a requestor checking for an update.
//
class LLUpdateChecker::Client
{
public:
	// An error occurred while checking for an update.
	virtual void error(std::string const & message) = 0;
	
	// A successful response was received from the viewer version manager
	virtual void response(LLSD const & content) = 0;
};


#endif
