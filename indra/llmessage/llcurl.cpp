/**
 * @file llcurl.cpp
 * @author Zero / Donovan
 * @date 2006-10-15
 * @brief Implementation of wrapper around libcurl.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010-2013, Linden Research, Inc.
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

#if LL_WINDOWS
#define SAFE_SSL 1
#elif LL_DARWIN
#define SAFE_SSL 1
#else
#define SAFE_SSL 1
#endif

#include "linden_common.h"

#include "llcurl.h"

#include <algorithm>
#include <iomanip>
#include <curl/curl.h>
#if SAFE_SSL
#include <openssl/crypto.h>
#endif

#include "llbufferstream.h"
#include "llproxy.h"
#include "llsdserialize.h"
#include "llstl.h"
#include "llstring.h"
#include "llthread.h"
#include "lltimer.h"

//////////////////////////////////////////////////////////////////////////////
/*
	The trick to getting curl to do keep-alives is to reuse the
	same easy handle for the requests.  It appears that curl
	keeps a pool of connections alive for each easy handle, but
	doesn't share them between easy handles.  Therefore it is
	important to keep a pool of easy handles and reuse them,
	rather than create and destroy them with each request.  This
	code does this.

	Furthermore, it would behoove us to keep track of which
	hosts an easy handle was used for and pick an easy handle
	that matches the next request.  This code does not current
	do this.
 */

// *TODO: TSN remove the commented code from this file
//////////////////////////////////////////////////////////////////////////////

//static const S32 MAX_ACTIVE_REQUEST_COUNT = 100;

// DEBUG //
S32 gCurlEasyCount = 0;
S32 gCurlMultiCount = 0;

//////////////////////////////////////////////////////////////////////////////

//static
std::vector<LLMutex*> LLCurl::sSSLMutex;
std::string LLCurl::sCAPath;
std::string LLCurl::sCAFile;
//LLCurlThread* LLCurl::sCurlThread = NULL ;
LLMutex* LLCurl::sHandleMutexp = NULL ;
S32      LLCurl::sTotalHandles = 0 ;
bool     LLCurl::sNotQuitting = true;
F32      LLCurl::sCurlRequestTimeOut = 120.f; //seonds
S32      LLCurl::sMaxHandles = 256; //max number of handles, (multi handles and easy handles combined).
CURL*	 LLCurl::sCurlTemplateStandardHandle = NULL;

void check_curl_code(CURLcode code)
{
	if (code != CURLE_OK)
	{
		// linux appears to throw a curl error once per session for a bad initialization
		// at a pretty random time (when enabling cookies).
		LL_WARNS("curl") << "curl error detected: " << curl_easy_strerror(code) << LL_ENDL;
	}
}

void check_curl_multi_code(CURLMcode code) 
{
	if (code != CURLM_OK)
	{
		// linux appears to throw a curl error once per session for a bad initialization
		// at a pretty random time (when enabling cookies).
		LL_WARNS("curl") << "curl multi error detected: " << curl_multi_strerror(code) << LL_ENDL;
	}
}

//static
void LLCurl::setCAPath(const std::string& path)
{
	sCAPath = path;
}

//static
void LLCurl::setCAFile(const std::string& file)
{
	sCAFile = file;
}

//static
std::string LLCurl::getVersionString()
{
	return std::string(curl_version());
}


//static
std::string LLCurl::strerror(CURLcode errorcode)
{
	return std::string(curl_easy_strerror(errorcode));
}


////////////////////////////////////////////////////////////////////////////

#if SAFE_SSL
//static
void LLCurl::ssl_locking_callback(int mode, int type, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
	{
		LLCurl::sSSLMutex[type]->lock();
	}
	else
	{
		LLCurl::sSSLMutex[type]->unlock();
	}
}

//static
unsigned long LLCurl::ssl_thread_id(void)
{
	return LLThread::currentID();
}
#endif

void LLCurl::initClass(F32 curl_reuest_timeout, S32 max_number_handles, bool multi_threaded)
{
	sCurlRequestTimeOut = curl_reuest_timeout ; //seconds
	sMaxHandles = max_number_handles ; //max number of handles, (multi handles and easy handles combined).

	// Do not change this "unless you are familiar with and mean to control 
	// internal operations of libcurl"
	// - http://curl.haxx.se/libcurl/c/curl_global_init.html
	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);

	check_curl_code(code);
	
#if SAFE_SSL
	S32 mutex_count = CRYPTO_num_locks();
	for (S32 i=0; i<mutex_count; i++)
	{
		sSSLMutex.push_back(new LLMutex(NULL));
	}
	CRYPTO_set_id_callback(&LLCurl::ssl_thread_id);
	CRYPTO_set_locking_callback(&LLCurl::ssl_locking_callback);
#endif

//	sCurlThread = new LLCurlThread(multi_threaded) ;
	if(multi_threaded)
	{
		sHandleMutexp = new LLMutex(NULL) ;
//		Easy::sHandleMutexp = new LLMutex(NULL) ;
	}
}

void LLCurl::cleanupClass()
{
	sNotQuitting = false; //set quitting

	//shut down curl thread
// 	while(1)
// 	{
// 		if(!sCurlThread->update(1)) //finish all tasks
// 		{
// 			break ;
// 		}
// 	}
	LL_CHECK_MEMORY
//	sCurlThread->shutdown() ;
	LL_CHECK_MEMORY
//	delete sCurlThread ;
//	sCurlThread = NULL ;
	LL_CHECK_MEMORY

#if SAFE_SSL
	CRYPTO_set_locking_callback(NULL);
	for_each(sSSLMutex.begin(), sSSLMutex.end(), DeletePointer());
	sSSLMutex.clear();
#endif
	
    LL_CHECK_MEMORY

	// Free the template easy handle
	curl_easy_cleanup(sCurlTemplateStandardHandle);
	sCurlTemplateStandardHandle = NULL;
	LL_CHECK_MEMORY


	delete sHandleMutexp ;
	sHandleMutexp = NULL ;

	LL_CHECK_MEMORY

	// removed as per https://jira.secondlife.com/browse/SH-3115
	//llassert(Easy::sActiveHandles.empty());
}

//static 
CURLM* LLCurl::newMultiHandle()
{
	llassert(sNotQuitting);

	LLMutexLock lock(sHandleMutexp) ;

	if(sTotalHandles + 1 > sMaxHandles)
	{
		LL_WARNS() << "no more handles available." << LL_ENDL ;
		return NULL ; //failed
	}
	sTotalHandles++;

	CURLM* ret = curl_multi_init() ;
	if(!ret)
	{
		LL_WARNS() << "curl_multi_init failed." << LL_ENDL ;
	}

	return ret ;
}

//static 
CURLMcode  LLCurl::deleteMultiHandle(CURLM* handle)
{
	if(handle)
	{
		LLMutexLock lock(sHandleMutexp) ;		
		sTotalHandles-- ;
		return curl_multi_cleanup(handle) ;
	}
	return CURLM_OK ;
}

//static 
CURL*  LLCurl::newEasyHandle()
{
	llassert(sNotQuitting);
	LLMutexLock lock(sHandleMutexp) ;

	if(sTotalHandles + 1 > sMaxHandles)
	{
		LL_WARNS() << "no more handles available." << LL_ENDL ;
		return NULL ; //failed
	}
	sTotalHandles++;

	CURL* ret = createStandardCurlHandle();
	if(!ret)
	{
		LL_WARNS() << "failed to create curl handle." << LL_ENDL ;
	}

	return ret ;
}

//static 
void  LLCurl::deleteEasyHandle(CURL* handle)
{
	if(handle)
	{
		LLMutexLock lock(sHandleMutexp) ;
		LL_CHECK_MEMORY
		curl_easy_cleanup(handle) ;
		LL_CHECK_MEMORY
		sTotalHandles-- ;
	}
}

const unsigned int LLCurl::MAX_REDIRECTS = 5;

// Provide access to LLCurl free functions outside of llcurl.cpp without polluting the global namespace.
void LLCurlFF::check_easy_code(CURLcode code)
{
	check_curl_code(code);
}
// void LLCurlFF::check_multi_code(CURLMcode code)
// {
// 	check_curl_multi_code(code);
// }


// Static
CURL* LLCurl::createStandardCurlHandle()
{
	if (sCurlTemplateStandardHandle == NULL)
	{	// Late creation of the template curl handle
		sCurlTemplateStandardHandle = curl_easy_init();
		if (sCurlTemplateStandardHandle == NULL)
		{
			LL_WARNS() << "curl error calling curl_easy_init()" << LL_ENDL;
		}
		else
		{
			CURLcode result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
			check_curl_code(result);
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_NOSIGNAL, 1);
			check_curl_code(result);
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_NOPROGRESS, 1);
			check_curl_code(result);
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_ENCODING, "");	
			check_curl_code(result);
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_AUTOREFERER, 1);
			check_curl_code(result);
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_FOLLOWLOCATION, 1);	
			check_curl_code(result);
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_SSL_VERIFYPEER, 1);
			check_curl_code(result);
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_SSL_VERIFYHOST, 0);
			check_curl_code(result);

			// The Linksys WRT54G V5 router has an issue with frequent
			// DNS lookups from LAN machines.  If they happen too often,
			// like for every HTTP request, the router gets annoyed after
			// about 700 or so requests and starts issuing TCP RSTs to
			// new connections.  Reuse the DNS lookups for even a few
			// seconds and no RSTs.
			result = curl_easy_setopt(sCurlTemplateStandardHandle, CURLOPT_DNS_CACHE_TIMEOUT, 15);
			check_curl_code(result);
		}
	}

	return curl_easy_duphandle(sCurlTemplateStandardHandle);
}
