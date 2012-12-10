/**
 * @file _httpoprequest.h
 * @brief Internal declarations for the HttpOpRequest subclass
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef	_LLCORE_HTTP_OPREQUEST_H_
#define	_LLCORE_HTTP_OPREQUEST_H_


#include "linden_common.h"		// Modifies curl/curl.h interfaces

#include <string>
#include <curl/curl.h>

#include "httpcommon.h"
#include "httprequest.h"
#include "_httpoperation.h"
#include "_refcounted.h"


namespace LLCore
{


class BufferArray;
class HttpHeaders;
class HttpOptions;


/// HttpOpRequest requests a supported HTTP method invocation with
/// option and header overrides.
///
/// Essentially an RPC to get an HTTP GET, POST or PUT executed
/// asynchronously with options to override behaviors and HTTP
/// headers.
///
/// Constructor creates a raw object incapable of useful work.
/// A subsequent call to one of the setupXXX() methods provides
/// the information needed to make a working request which can
/// then be enqueued to a request queue.
///

class HttpOpRequest : public HttpOperation
{
public:
	HttpOpRequest();

protected:
	virtual ~HttpOpRequest();							// Use release()

private:
	HttpOpRequest(const HttpOpRequest &);				// Not defined
	void operator=(const HttpOpRequest &);				// Not defined

public:
	enum EMethod
	{
		HOR_GET,
		HOR_POST,
		HOR_PUT
	};
	
	virtual void stageFromRequest(HttpService *);
	virtual void stageFromReady(HttpService *);
	virtual void stageFromActive(HttpService *);

	virtual void visitNotifier(HttpRequest * request);
			
public:
	/// Setup Methods
	///
	/// Basically an RPC setup for each type of HTTP method
	/// invocation with one per method type.  These are
	/// generally invoked right after construction.
	///
	/// Threading:  called by application thread
	///
	HttpStatus setupGet(HttpRequest::policy_t policy_id,
						HttpRequest::priority_t priority,
						const std::string & url,
						HttpOptions * options,
						HttpHeaders * headers);
	
	HttpStatus setupGetByteRange(HttpRequest::policy_t policy_id,
								 HttpRequest::priority_t priority,
								 const std::string & url,
								 size_t offset,
								 size_t len,
								 HttpOptions * options,
								 HttpHeaders * headers);
	
	HttpStatus setupPost(HttpRequest::policy_t policy_id,
						 HttpRequest::priority_t priority,
						 const std::string & url,
						 BufferArray * body,
						 HttpOptions * options,
						 HttpHeaders * headers);
	
	HttpStatus setupPut(HttpRequest::policy_t policy_id,
						HttpRequest::priority_t priority,
						const std::string & url,
						BufferArray * body,
						HttpOptions * options,
						HttpHeaders * headers);

	// Internal method used to setup the libcurl options for a request.
	// Does all the libcurl handle setup in one place.
	//
	// Threading:  called by worker thread
	//
	HttpStatus prepareRequest(HttpService * service);
	
	virtual HttpStatus cancel();

protected:
	// Common setup for all the request methods.
	//
	// Threading:  called by application thread
	//
	void setupCommon(HttpRequest::policy_t policy_id,
					 HttpRequest::priority_t priority,
					 const std::string & url,
					 BufferArray * body,
					 HttpOptions * options,
					 HttpHeaders * headers);

	// libcurl operational callbacks
	//
	// Threading:  called by worker thread
	//
	static size_t writeCallback(void * data, size_t size, size_t nmemb, void * userdata);
	static size_t readCallback(void * data, size_t size, size_t nmemb, void * userdata);
	static size_t headerCallback(void * data, size_t size, size_t nmemb, void * userdata);
	static int debugCallback(CURL *, curl_infotype info, char * buffer, size_t len, void * userdata);

protected:
	unsigned int		mProcFlags;
	static const unsigned int	PF_SCAN_RANGE_HEADER = 0x00000001U;
	static const unsigned int	PF_SAVE_HEADERS = 0x00000002U;

public:
	// Request data
	EMethod				mReqMethod;
	std::string			mReqURL;
	BufferArray *		mReqBody;
	off_t				mReqOffset;
	size_t				mReqLength;
	HttpHeaders *		mReqHeaders;
	HttpOptions *		mReqOptions;

	// Transport data
	bool				mCurlActive;
	CURL *				mCurlHandle;
	HttpService *		mCurlService;
	curl_slist *		mCurlHeaders;
	size_t				mCurlBodyPos;
	
	// Result data
	HttpStatus			mStatus;
	BufferArray *		mReplyBody;
	off_t				mReplyOffset;
	size_t				mReplyLength;
	size_t				mReplyFullLength;
	HttpHeaders *		mReplyHeaders;
	std::string			mReplyConType;

	// Policy data
	int					mPolicyRetries;
	HttpTime			mPolicyRetryAt;
	int					mPolicyRetryLimit;
};  // end class HttpOpRequest


/// HttpOpRequestCompare isn't an operation but a uniform comparison
/// functor for STL containers that order by priority.  Mainly
/// used for the ready queue container but defined here.
class HttpOpRequestCompare
{
public:
	bool operator()(const HttpOpRequest * lhs, const HttpOpRequest * rhs)
		{
			return lhs->mReqPriority > rhs->mReqPriority;
		}
};  // end class HttpOpRequestCompare


// ---------------------------------------
// Free functions
// ---------------------------------------

// Internal function to append the contents of an HttpHeaders
// instance to a curl_slist object.
curl_slist * append_headers_to_slist(const HttpHeaders *, curl_slist * slist);

}   // end namespace LLCore

#endif	// _LLCORE_HTTP_OPREQUEST_H_

