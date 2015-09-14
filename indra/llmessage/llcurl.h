/** 
 * @file llcurl.h
 * @author Zero / Donovan
 * @date 2006-10-15
 * @brief A wrapper around libcurl.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
 
#ifndef LL_LLCURL_H
#define LL_LLCURL_H

#include "linden_common.h"

#include <sstream>
#include <string>
#include <vector>

#include <boost/intrusive_ptr.hpp>
#include <curl/curl.h> // TODO: remove dependency

#include "llbuffer.h"
#include "llhttpconstants.h"
#include "lliopipe.h"
#include "llsd.h"
#include "llqueuedthread.h"
#include "llframetimer.h"
#include "llpointer.h"
#include "llsingleton.h"

class LLMutex;
//class LLCurlThread;

// For whatever reason, this is not typedef'd in curl.h
typedef size_t (*curl_header_callback)(void *ptr, size_t size, size_t nmemb, void *stream);

class LLCurl
{
	LOG_CLASS(LLCurl);
	
public:

	/**
	 * @ brief Set certificate authority file used to verify HTTPS certs.
	 */
	static void setCAFile(const std::string& file);

	/**
	 * @ brief Set certificate authority path used to verify HTTPS certs.
	 */
	static void setCAPath(const std::string& path);

	/**
	 * @ brief Return human-readable string describing libcurl version.
	 */
	static std::string getVersionString();
	
	/**
	 * @ brief Get certificate authority file used to verify HTTPS certs.
	 */
	static const std::string& getCAFile() { return sCAFile; }

	/**
	 * @ brief Get certificate authority path used to verify HTTPS certs.
	 */
	static const std::string& getCAPath() { return sCAPath; }

	/**
	 * @ brief Initialize LLCurl class
	 */
	static void initClass(F32 curl_reuest_timeout = 120.f, S32 max_number_handles = 256, bool multi_threaded = false);

	/**
	 * @ brief Cleanup LLCurl class
	 */
	static void cleanupClass();

	/**
	 * @ brief curl error code -> string
	 */
	static std::string strerror(CURLcode errorcode);
	
	// For OpenSSL callbacks
	static std::vector<LLMutex*> sSSLMutex;

	// OpenSSL callbacks
	static void ssl_locking_callback(int mode, int type, const char *file, int line);
	static unsigned long ssl_thread_id(void);

//	static LLCurlThread* getCurlThread() { return sCurlThread ;}

	static CURLM* newMultiHandle() ;
	static CURLMcode deleteMultiHandle(CURLM* handle) ;
	static CURL*  newEasyHandle() ;
	static void   deleteEasyHandle(CURL* handle) ;

	static CURL*	createStandardCurlHandle();

private:
	static std::string sCAPath;
	static std::string sCAFile;
	static const unsigned int MAX_REDIRECTS;
    //	static LLCurlThread* sCurlThread;
//    static LLCurlThread* sCurlThread;

	static LLMutex* sHandleMutexp ;
	static S32      sTotalHandles ;
	static S32      sMaxHandles;
	static CURL*	sCurlTemplateStandardHandle;
public:
	static bool     sNotQuitting;
	static F32      sCurlRequestTimeOut;	
};


// Provide access to LLCurl free functions outside of llcurl.cpp without polluting the global namespace.
namespace LLCurlFF
{
	void check_easy_code(CURLcode code);
	//void check_multi_code(CURLMcode code);
}

#endif // LL_LLCURL_H
