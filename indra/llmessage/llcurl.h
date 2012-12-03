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
#include "llthread.h"
#include "llqueuedthread.h"
#include "llframetimer.h"
#include "llpointer.h"


class LLMutex;
class LLCurlThread;

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
	
	class Responder : public LLThreadSafeRefCount
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

	private:
		std::string mURL;
	};
	typedef LLPointer<Responder>	ResponderPtr;


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

	static LLCurlThread* getCurlThread() { return sCurlThread ;}

	static CURLM* newMultiHandle() ;
	static CURLMcode deleteMultiHandle(CURLM* handle) ;
	static CURL*  newEasyHandle() ;
	static void   deleteEasyHandle(CURL* handle) ;

private:
	static std::string sCAPath;
	static std::string sCAFile;
	static const unsigned int MAX_REDIRECTS;
	static LLCurlThread* sCurlThread;

	static LLMutex* sHandleMutexp ;
	static S32      sTotalHandles ;
	static S32      sMaxHandles;
public:
	static bool     sNotQuitting;
	static F32      sCurlRequestTimeOut;	
};

class LLCurl::Easy
{
	LOG_CLASS(Easy);

private:
	Easy();

public:
	static Easy* getEasy();
	~Easy();

	CURL* getCurlHandle() const { return mCurlEasyHandle; }

	void setErrorBuffer();
	void setCA();

	void setopt(CURLoption option, S32 value);
	// These assume the setter does not free value!
	void setopt(CURLoption option, void* value);
	void setopt(CURLoption option, char* value);
	// Copies the string so that it is guaranteed to stick around
	void setoptString(CURLoption option, const std::string& value);

	void slist_append(const char* str);
	void setHeaders();

	U32 report(CURLcode);
	void getTransferInfo(LLCurl::TransferInfo* info);

	void prepRequest(const std::string& url, const std::vector<std::string>& headers, LLCurl::ResponderPtr, S32 time_out = 0, bool post = false);

	const char* getErrorBuffer();

	std::stringstream& getInput() { return mInput; }
	std::stringstream& getHeaderOutput() { return mHeaderOutput; }
	LLIOPipe::buffer_ptr_t& getOutput() { return mOutput; }
	const LLChannelDescriptors& getChannels() { return mChannels; }

	void resetState();

	static CURL* allocEasyHandle();
	static void releaseEasyHandle(CURL* handle);

private:
	friend class LLCurl;
	friend class LLCurl::Multi;

	CURL*				mCurlEasyHandle;
	struct curl_slist*	mHeaders;

	std::stringstream	mRequest;
	LLChannelDescriptors mChannels;
	LLIOPipe::buffer_ptr_t mOutput;
	std::stringstream	mInput;
	std::stringstream	mHeaderOutput;
	char				mErrorBuffer[CURL_ERROR_SIZE];

	// Note: char*'s not strings since we pass pointers to curl
	std::vector<char*>	mStrings;

	LLCurl::ResponderPtr		mResponder;

	static std::set<CURL*> sFreeHandles;
	static std::set<CURL*> sActiveHandles;
	static LLMutex*        sHandleMutexp ;
};

class LLCurl::Multi
{
	LOG_CLASS(Multi);

	friend class LLCurlThread ;

private:
	~Multi();

	void markDead() ;
	bool doPerform();

public:

	typedef enum
	{
		STATE_READY=0,
		STATE_PERFORMING=1,
		STATE_COMPLETED=2
	} ePerformState;

	Multi(F32 idle_time_out = 0.f);	

	LLCurl::Easy* allocEasy();
	bool addEasy(LLCurl::Easy* easy);	
	void removeEasy(LLCurl::Easy* easy);
	
	void lock() ;
	void unlock() ;

	void setState(ePerformState state) ;
	ePerformState getState() ;
	
	bool isCompleted() ;
	bool isValid() {return mCurlMultiHandle != NULL && mValid;}
	bool isDead() {return mDead;}

	bool waitToComplete() ;

	S32 process();
	
	CURLMsg* info_read(S32* msgs_in_queue);

	S32 mQueued;
	S32 mErrorCount;
	
private:
	void easyFree(LLCurl::Easy*);
	void cleanup(bool deleted = false) ;
	
	CURLM* mCurlMultiHandle;

	typedef std::set<LLCurl::Easy*> easy_active_list_t;
	easy_active_list_t mEasyActiveList;
	typedef std::map<CURL*, LLCurl::Easy*> easy_active_map_t;
	easy_active_map_t mEasyActiveMap;
	typedef std::set<LLCurl::Easy*> easy_free_list_t;
	easy_free_list_t mEasyFreeList;

	LLQueuedThread::handle_t mHandle ;
	ePerformState mState;

	BOOL mDead ;
	BOOL mValid ;
	LLMutex* mMutexp ;
	LLMutex* mDeletionMutexp ;
	LLMutex* mEasyMutexp ;
	LLFrameTimer mIdleTimer ;
	F32 mIdleTimeOut;
};

class LLCurlThread : public LLQueuedThread
{
public:

	class CurlRequest : public LLQueuedThread::QueuedRequest
	{
	protected:
		virtual ~CurlRequest(); // use deleteRequest()
		
	public:
		CurlRequest(handle_t handle, LLCurl::Multi* multi, LLCurlThread* curl_thread);

		/*virtual*/ bool processRequest();
		/*virtual*/ void finishRequest(bool completed);

	private:
		// input
		LLCurl::Multi* mMulti;
		LLCurlThread*  mCurlThread;
	};
	friend class CurlRequest;

public:
	LLCurlThread(bool threaded = true) ;
	virtual ~LLCurlThread() ;

	S32 update(F32 max_time_ms);

	void addMulti(LLCurl::Multi* multi) ;
	void killMulti(LLCurl::Multi* multi) ;

private:
	bool doMultiPerform(LLCurl::Multi* multi) ;
	void deleteMulti(LLCurl::Multi* multi) ;
	void cleanupMulti(LLCurl::Multi* multi) ;
} ;


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
};

//for texture fetch only
class LLCurlTextureRequest : public LLCurlRequest
{
public:
	LLCurlTextureRequest(S32 concurrency);
	~LLCurlTextureRequest();

	U32 getByteRange(const std::string& url, const headers_t& headers, S32 offset, S32 length, U32 pri, LLCurl::ResponderPtr responder, F32 delay_time = -1.f);
	void nextRequests();
	void completeRequest(S32 received_bytes);

	void updatePriority(U32 handle, U32 pri);
	void removeRequest(U32 handle);

	U32 getTotalReceivedBits();
	U32 getTotalIssuedRequests();
	S32 getNumRequests();
	bool isWaiting(U32 handle);
	
private:
	LLMutex mMutex;
	S32 mConcurrency;
	S32 mInQueue; //request currently in queue.
	U32 mHandleCounter;
	U32 mTotalIssuedRequests;
	U32 mTotalReceivedBits;

	typedef struct _request_t
	{
		_request_t(U32 handle, const std::string& url, const headers_t& headers, S32 offset, S32 length, U32 pri, LLCurl::ResponderPtr responder) :
				mHandle(handle), mUrl(url), mHeaders(headers), mOffset(offset), mLength(length), mPriority(pri), mResponder(responder), mStartTime(0.f)
				{}

		U32  mHandle;
		std::string mUrl;
		LLCurlRequest::headers_t mHeaders;
		S32 mOffset;
		S32 mLength;
		LLCurl::ResponderPtr mResponder;
		U32 mPriority;
		F32 mStartTime; //start time to issue this request
	} request_t;

	struct request_compare
	{
		bool operator()(const request_t* lhs, const request_t* rhs) const
		{
			if(lhs->mPriority != rhs->mPriority)
			{
				return lhs->mPriority > rhs->mPriority; // higher priority in front of queue (set)
			}
			else
			{
				return (U32)lhs < (U32)rhs;
			}
		}
	};

	typedef std::set<request_t*, request_compare> req_queue_t;
	req_queue_t mCachedRequests;
	std::map<S32, request_t*> mRequestMap;

	LLFrameTimer mGlobalTimer;
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
	bool getResult(CURLcode* result, LLCurl::TransferInfo* info = NULL);
	std::string getErrorString();
	bool isCompleted() {return mMulti->isCompleted() ;}
	bool wait() { return mMulti->waitToComplete(); }
	bool isValid() {return mMulti && mMulti->isValid(); }

	LLCurl::Easy* getEasy() const { return mEasy; }

private:
	CURLMsg* info_read(S32* queue, LLCurl::TransferInfo* info);
	
private:
	LLCurl::Multi* mMulti;
	LLCurl::Easy* mEasy;
	bool mRequestSent;
	bool mResultReturned;
};

// Provide access to LLCurl free functions outside of llcurl.cpp without polluting the global namespace.
namespace LLCurlFF
{
	void check_easy_code(CURLcode code);
	void check_multi_code(CURLMcode code);
}

#endif // LL_LLCURL_H
