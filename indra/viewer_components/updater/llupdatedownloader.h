/** 
 * @file llupdatedownloader.h
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

#ifndef LL_UPDATE_DOWNLOADER_H
#define LL_UPDATE_DOWNLOADER_H


#include <stdexcept>
#include <string>
#include <boost/shared_ptr.hpp>
#include "lluri.h"


//
// An asynchronous download service for fetching updates.
//
class LLUpdateDownloader
{
public:
	class BusyError;
	class Client;
	class Implementation;
	
	LLUpdateDownloader(Client & client);
	
	// Cancel any in progress download; a no op if none is in progress.
	void cancel(void);
	
	// Start a new download.
	//
	// This method will throw a BusyException instance if a download is already
	// in progress.
	void download(LLURI const & uri);
	
	// Returns true if a download is in progress.
	bool isDownloading(void);
	
private:
	boost::shared_ptr<Implementation> mImplementation;
};


//
// An interface to be implemented by clients initiating a update download.
//
class LLUpdateDownloader::Client {
public:
	
	// The download has completed successfully.
	virtual void downloadComplete(void) = 0;
	
	// The download failed.
	virtual void downloadError(std::string const & message) = 0;
};


#endif