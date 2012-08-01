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


//
// Implements asynchronous checking for updates.
//
class LLUpdateChecker {
public:
	class Client;
	class Implementation;
	
	// An exception that may be raised on check errors.
	class CheckError;
	
	LLUpdateChecker(Client & client);
	
	// Check status of current app on the given host for the channel and version provided.
	void checkVersion(std::string const & protocolVersion, std::string const & hostUrl, 
			   std::string const & servicePath, std::string channel, std::string version);
	
private:
	boost::shared_ptr<Implementation> mImplementation;
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
	
	// A newer version is available, but the current version may still be used.
	virtual void optionalUpdate(std::string const & newVersion,
								LLURI const & uri,
								std::string const & hash) = 0;
	
	// A newer version is available, and the current version is no longer valid. 
	virtual void requiredUpdate(std::string const & newVersion,
								LLURI const & uri,
								std::string const & hash) = 0;
	
	// The checked version is up to date; no newer version exists.
	virtual void upToDate(void) = 0;
};


#endif
