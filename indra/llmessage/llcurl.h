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
#include "lliopipe.h"
#include "llsd.h"

class LLMutex;

// For whatever reason, this is not typedef'd in curl.h
typedef size_t (*curl_header_callback)(void *ptr, size_t size, size_t nmemb, void *stream);

class LLCurl
{
	LOG_CLASS(LLCurl);
	
public:
	class Easy;
	class Multi;

	struct TransferInfo
	{
		TransferInfo() : mSizeDownload(0.0), mTotalTime(0.0), mSpeedDownload(0.0) {}
		F64 mSizeDownload;
		F64 mTotalTime;
		F64 mSpeedDownload;
	};
	
	class Responder
	{
	//LOG_CLASS(Responder);
	public:

		Responder();
		virtual ~Responder();

		/**
		 * @brief return true if the status code indicates success.
		 */
		static bool isGoodStatus(U32 status)
		{
			return((200 <= status) && (status < 300));
		}
		
		virtual void errorWithContent(
			U32 status,
			const std::string& reason,
			const LLSD& content);
			//< called by completed() on bad status 

		virtual void error(U32 status, const std::string& reason);
			//< called by default error(status, reason, content)
		
		virtual void result(const LLSD& content);
			//< called by completed for good status codes.

		virtual void completedRaw(
			U32 status,
			const std::string& reason,
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer);
			/**< Override point for clients that may want to use this
			   class when the response is some other format besides LLSD
			*/

		virtual void completed(
			U32 status,
			const std::string& reason,
			const LLSD& content);
			/**< The default implemetnation calls
				either:
				* result(), or
				* error() 
			*/
			
			// Override to handle parsing of the header only.  Note: this is the only place where the contents
			// of the header can be parsed.  In the ::completed call above only the body is contained in the LLSD.
			virtual void completedHeader(U32 status, const std::string& reason, const LLSD& content);

			// Used internally to set the url for debugging later.
			void setURL(const std::string& url);

			virtual bool followRedir() 
			{
				return false;
			}

	public: /* but not really -- don't touch this */
		U32 mReferenceCount;

	private:
		std::string mURL;
	};
	typedef boost::intrusive_ptr<Responder>	ResponderPtr;


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
	static void initClass();

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

private:
	static std::string sCAPath;
	static std::string sCAFile;
	static const unsigned int MAX_REDIRECTS;
};

namespace boost
{
	void intrusive_ptr_add_ref(LLCurl::Responder* p);
	void intrusive_ptr_release(LLCurl::Responder* p);
};


class LLCurlRequest
{
public:
	typedef std::vector<std::string> headers_t;
	
	LLCurlRequest();
	~LLCurlRequest();

	void get(const std::string& url, LLCurl::ResponderPtr responder);
	bool getByteRange(const std::string& url, const headers_t& headers, S32 offset, S32 length, LLCurl::ResponderPtr responder);
	bool post(const std::string& url, const headers_t& headers, const LLSD& data, LLCurl::ResponderPtr responder, S32 time_out = 0);
	bool post(const std::string& url, const headers_t& headers, const std::string& data, LLCurl::ResponderPtr responder, S32 time_out = 0);
	
	S32  process();
	S32  getQueued();

private:
	void addMulti();
	LLCurl::Easy* allocEasy();
	bool addEasy(LLCurl::Easy* easy);
	
private:
	typedef std::set<LLCurl::Multi*> curlmulti_set_t;
	curlmulti_set_t mMultiSet;
	LLCurl::Multi* mActiveMulti;
	S32 mActiveRequestCount;
	BOOL mProcessing;
	U32 mThreadID; // debug
};

class LLCurlEasyRequest
{
public:
	LLCurlEasyRequest();
	~LLCurlEasyRequest();
	void setopt(CURLoption option, S32 value);
	void setoptString(CURLoption option, const std::string& value);
	void setPost(char* postdata, S32 size);
	void setHeaderCallback(curl_header_callback callback, void* userdata);
	void setWriteCallback(curl_write_callback callback, void* userdata);
	void setReadCallback(curl_read_callback callback, void* userdata);
	void setSSLCtxCallback(curl_ssl_ctx_callback callback, void* userdata);
	void slist_append(const char* str);
	void sendRequest(const std::string& url);
	void requestComplete();
	S32 perform();
	bool getResult(CURLcode* result, LLCurl::TransferInfo* info = NULL);
	std::string getErrorString();

private:
	CURLMsg* info_read(S32* queue, LLCurl::TransferInfo* info);
	
private:
	LLCurl::Multi* mMulti;
	LLCurl::Easy* mEasy;
	bool mRequestSent;
	bool mResultReturned;
};

#endif // LL_LLCURL_H
