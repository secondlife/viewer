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
#include <boost/lexical_cast.hpp>
#include <curl/curl.h>
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
	~Implementation();
	void cancel(void);
	void download(LLURI const & uri);
	bool isDownloading(void);
	void onHeader(void * header, size_t size);
	void onBody(void * header, size_t size);
private:
	static const char * sSecondLifeUpdateRecord;
	
	LLUpdateDownloader::Client & mClient;
	CURL * mCurl;
	llofstream mDownloadStream;
	std::string mDownloadRecordPath;
	
	void initializeCurlGet(std::string const & url);
	void resumeDownloading(LLSD const & downloadData);
	void run(void);
	bool shouldResumeOngoingDownload(LLURI const & uri, LLSD & downloadData);
	void startDownloading(LLURI const & uri);

	LOG_CLASS(LLUpdateDownloader::Implementation);
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


namespace {
	size_t write_function(void * data, size_t blockSize, size_t blocks, void * downloader)
	{
		size_t bytes = blockSize * blocks;
		reinterpret_cast<LLUpdateDownloader::Implementation *>(downloader)->onBody(data, bytes);
		return bytes;
	}

	size_t header_function(void * data, size_t blockSize, size_t blocks, void * downloader)
	{
		size_t bytes = blockSize * blocks;
		reinterpret_cast<LLUpdateDownloader::Implementation *>(downloader)->onHeader(data, bytes);
		return bytes;
	}
}


const char * LLUpdateDownloader::Implementation::sSecondLifeUpdateRecord =
	"SecondLifeUpdateDownload.xml";


LLUpdateDownloader::Implementation::Implementation(LLUpdateDownloader::Client & client):
	LLThread("LLUpdateDownloader"),
	mClient(client),
	mCurl(0),
	mDownloadRecordPath(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, sSecondLifeUpdateRecord))
{
	CURLcode code = curl_global_init(CURL_GLOBAL_ALL); // Just in case.
	llassert(code = CURLE_OK); // TODO: real error handling here. 
}


LLUpdateDownloader::Implementation::~Implementation()
{
	if(mCurl) curl_easy_cleanup(mCurl);
}


void LLUpdateDownloader::Implementation::cancel(void)
{
	llassert(!"not implemented");
}
	

void LLUpdateDownloader::Implementation::download(LLURI const & uri)
{
	LLSD downloadData;
	if(shouldResumeOngoingDownload(uri, downloadData)){
		startDownloading(uri); // TODO: Implement resume.
	} else {
		startDownloading(uri);
	}
}


bool LLUpdateDownloader::Implementation::isDownloading(void)
{
	return !isStopped();
}

void LLUpdateDownloader::Implementation::onHeader(void * buffer, size_t size)
{
	char const * headerPtr = reinterpret_cast<const char *> (buffer);
	std::string header(headerPtr, headerPtr + size);
	size_t colonPosition = header.find(':');
	if(colonPosition == std::string::npos) return; // HTML response; ignore.
	
	if(header.substr(0, colonPosition) == "Content-Length") {
		try {
			size_t firstDigitPos = header.find_first_of("0123456789", colonPosition);
			size_t lastDigitPos = header.find_last_of("0123456789");
			std::string contentLength = header.substr(firstDigitPos, lastDigitPos - firstDigitPos + 1);
			size_t size = boost::lexical_cast<size_t>(contentLength);
			LL_INFOS("UpdateDownload") << "download size is " << size << LL_ENDL;
			
			LLSD downloadData;
			llifstream idataStream(mDownloadRecordPath);
			LLSDSerialize parser;
			parser.fromXMLDocument(downloadData, idataStream);
			idataStream.close();
			downloadData["size"] = LLSD(LLSD::Integer(size));
			llofstream odataStream(mDownloadRecordPath);
			parser.toPrettyXML(downloadData, odataStream);
		} catch (std::exception const & e) {
			LL_WARNS("UpdateDownload") << "unable to read content length (" 
				<< e.what() << ")" << LL_ENDL;
		}
	} else {
		; // No op.
	}
}


void LLUpdateDownloader::Implementation::onBody(void * buffer, size_t size)
{
	mDownloadStream.write(reinterpret_cast<const char *>(buffer), size);
}


void LLUpdateDownloader::Implementation::run(void)
{
	CURLcode code = curl_easy_perform(mCurl);
	if(code == CURLE_OK) {
		LL_INFOS("UpdateDownload") << "download successful" << LL_ENDL;
		mClient.downloadComplete();
	} else {
		LL_WARNS("UpdateDownload") << "download failed with error " << code << LL_ENDL;
		mClient.downloadError("curl error");
	}
}


void LLUpdateDownloader::Implementation::initializeCurlGet(std::string const & url)
{
	if(mCurl == 0) {
		mCurl = curl_easy_init();
	} else {
		curl_easy_reset(mCurl);
	}
	
	llassert(mCurl != 0); // TODO: real error handling here.
	
	CURLcode code;
	code = curl_easy_setopt(mCurl, CURLOPT_NOSIGNAL, true);
	code = curl_easy_setopt(mCurl, CURLOPT_FOLLOWLOCATION, true);
	code = curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, &write_function);
	code = curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, this);
	code = curl_easy_setopt(mCurl, CURLOPT_HEADERFUNCTION, &header_function);
	code = curl_easy_setopt(mCurl, CURLOPT_HEADERDATA, this);
	code = curl_easy_setopt(mCurl, CURLOPT_HTTPGET, true);
	code = curl_easy_setopt(mCurl, CURLOPT_URL, url.c_str());
}


void LLUpdateDownloader::Implementation::resumeDownloading(LLSD const & downloadData)
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
	LL_INFOS("UpdateDownload") << "downloading " << filePath << LL_ENDL;
	LL_INFOS("UpdateDownload") << "from " << uri.asString() << LL_ENDL;
	downloadData["path"] = filePath;
	llofstream dataStream(mDownloadRecordPath);
	LLSDSerialize parser;
	parser.toPrettyXML(downloadData, dataStream);
	
	mDownloadStream.open(filePath, std::ios_base::out | std::ios_base::binary);
	initializeCurlGet(uri.asString());
	start();
}
