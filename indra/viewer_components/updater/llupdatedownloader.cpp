/** 
 * @file llupdatedownloader.cpp
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
#include "lldir.h"
#include "llfile.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llthread.h"
#include "llupdatedownloader.h"


class LLUpdateDownloader::Implementation:
	public LLThread
{
public:
	Implementation(LLUpdateDownloader::Client & client);
	void cancel(void);
	void download(LLURI const & uri);
	bool isDownloading(void);

private:
	static const char * sSecondLifeUpdateRecord;
	
	LLUpdateDownloader::Client & mClient;
	std::string mDownloadRecordPath;
	
	void resumeDownloading(LLSD const & downloadData);
	void run(void);
	bool shouldResumeOngoingDownload(LLURI const & uri, LLSD & downloadData);
	void startDownloading(LLURI const & uri);
};



// LLUpdateDownloader
//-----------------------------------------------------------------------------


LLUpdateDownloader::LLUpdateDownloader(Client & client):
	mImplementation(new LLUpdateDownloader::Implementation(client))
{
	; // No op.
}


void LLUpdateDownloader::cancel(void)
{
	mImplementation->cancel();
}


void LLUpdateDownloader::download(LLURI const & uri)
{
	mImplementation->download(uri);
}


bool LLUpdateDownloader::isDownloading(void)
{
	return mImplementation->isDownloading();
}



// LLUpdateDownloader::Implementation
//-----------------------------------------------------------------------------


const char * LLUpdateDownloader::Implementation::sSecondLifeUpdateRecord =
	"SecondLifeUpdateDownload.xml";


LLUpdateDownloader::Implementation::Implementation(LLUpdateDownloader::Client & client):
	LLThread("LLUpdateDownloader"),
	mClient(client),
	mDownloadRecordPath(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, sSecondLifeUpdateRecord))
{
	; // No op.
}


void LLUpdateDownloader::Implementation::cancel(void)
{
}


void LLUpdateDownloader::Implementation::download(LLURI const & uri)
{
	LLSD downloadData;
	if(shouldResumeOngoingDownload(uri, downloadData)){
		
	} else {
					
	}
}


bool LLUpdateDownloader::Implementation::isDownloading(void)
{
	return false;
}


void resumeDownloading(LLSD const & downloadData)
{
}


bool LLUpdateDownloader::Implementation::shouldResumeOngoingDownload(LLURI const & uri, LLSD & downloadData)
{
	if(!LLFile::isfile(mDownloadRecordPath)) return false;
	
	llifstream dataStream(mDownloadRecordPath);
	LLSDSerialize parser;
	parser.fromXMLDocument(downloadData, dataStream);
	
	if(downloadData["url"].asString() != uri.asString()) return false;
	
	std::string downloadedFilePath = downloadData["path"].asString();
	if(LLFile::isfile(downloadedFilePath)) {
		llstat fileStatus;
		LLFile::stat(downloadedFilePath, &fileStatus);
		downloadData["bytes_downloaded"] = LLSD(LLSD::Integer(fileStatus.st_size)); 
		return true;
	} else {
		return false;
	}

	return true;
}


void LLUpdateDownloader::Implementation::startDownloading(LLURI const & uri)
{
	LLSD downloadData;
	downloadData["url"] = uri.asString();
	LLSD path = uri.pathArray();
	std::string fileName = path[path.size() - 1].asString();
	std::string filePath = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, fileName);
	llofstream dataStream(mDownloadRecordPath);
	LLSDSerialize parser;
	parser.toPrettyXML(downloadData, dataStream);
	
	llofstream downloadStream(filePath);
}
