/**
 * @file _httpoprequest.cpp
 * @brief Definitions for internal class HttpOpRequest
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2014, Linden Research, Inc.
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

#include "_httpoprequest.h"

#include <cstdio>
#include <algorithm>

#include "httpcommon.h"
#include "httphandler.h"
#include "httpresponse.h"
#include "bufferarray.h"
#include "httpheaders.h"
#include "httpoptions.h"

#include "_httprequestqueue.h"
#include "_httpreplyqueue.h"
#include "_httpservice.h"
#include "_httppolicy.h"
#include "_httppolicyglobal.h"
#include "_httplibcurl.h"
#include "_httpinternal.h"

#include "llhttpconstants.h"
#include "llproxy.h"

// *DEBUG:  "[curl:bugs] #1420" problem and testing.
//
// A pipelining problem, https://sourceforge.net/p/curl/bugs/1420/,
// was a source of Core_9 failures.  Code related to this can be
// identified and tested by:
// * Looking for '[curl:bugs]' strings in source and following
//   instructions there.
// * Set 'QAModeHttpTrace' to 2 or 3 in settings.xml and look for
//   'timed out' events in the log.
// * Enable the HttpRangeRequestsDisable debug setting which causes
//   full asset fetches.  These slow the pipelines down a bit.
//

namespace
{

// Attempts to parse a 'Content-Range:' header.  Caller must already
// have verified that the header tag is present.  The 'buffer' argument
// will be processed by strtok_r calls which will modify the buffer.
//
// @return		-1 if invalid and response should be dropped, 0 if valid an
//				correct, 1 if couldn't be parsed.  If 0, the first, last,
//				and length arguments are also written.  'length' may be
//				0 if the length wasn't available to the server.
//
int parse_content_range_header(char * buffer,
							   unsigned int * first,
							   unsigned int * last,
							   unsigned int * length);

// Similar for Retry-After headers.  Only parses the delta form
// of the header, HTTP time formats aren't interesting for client
// purposes.
//
// @return		0 if successfully parsed and seconds time delta
//				returned in time argument.
//
int parse_retry_after_header(char * buffer, int * time);


// Take data from libcurl's CURLOPT_DEBUGFUNCTION callback and
// escape and format it for a tracing line in logging.  Absolutely
// anything including NULs can be in the data.  If @scrub is true,
// non-printing or non-ascii characters are replaced with spaces
// otherwise a %XX form of escaping is used.
void escape_libcurl_debug_data(char * buffer, size_t len, bool scrub,
							   std::string & safe_line);


// OS-neutral string comparisons of various types.
int os_strcasecmp(const char * s1, const char * s2);
char * os_strtok_r(char * str, const char * delim, char ** saveptr);
char * os_strtrim(char * str);
char * os_strltrim(char * str);
void os_strlower(char * str);

// Error testing and reporting for libcurl status codes
void check_curl_easy_code(CURLcode code, int curl_setopt_option);

static const char * const LOG_CORE("CoreHttp");

} // end anonymous namespace


namespace LLCore
{


HttpOpRequest::HttpOpRequest()
	: HttpOperation(),
	  mProcFlags(0U),
	  mReqMethod(HOR_GET),
	  mReqBody(NULL),
	  mReqOffset(0),
	  mReqLength(0),
	  mReqHeaders(),
	  mReqOptions(),
	  mCurlActive(false),
	  mCurlHandle(NULL),
	  mCurlService(NULL),
	  mCurlHeaders(NULL),
	  mCurlBodyPos(0),
	  mCurlTemp(NULL),
	  mCurlTempLen(0),
	  mReplyBody(NULL),
	  mReplyOffset(0),
	  mReplyLength(0),
	  mReplyFullLength(0),
	  mReplyHeaders(),
	  mPolicyRetries(0),
	  mPolicy503Retries(0),
	  mPolicyRetryAt(HttpTime(0)),
	  mPolicyRetryLimit(HTTP_RETRY_COUNT_DEFAULT),
	  mPolicyMinRetryBackoff(HttpTime(HTTP_RETRY_BACKOFF_MIN_DEFAULT)),
	  mPolicyMaxRetryBackoff(HttpTime(HTTP_RETRY_BACKOFF_MAX_DEFAULT)),
	  mCallbackSSLVerify(NULL)
{
	// *NOTE:  As members are added, retry initialization/cleanup
	// may need to be extended in @see prepareRequest().
}



HttpOpRequest::~HttpOpRequest()
{
	if (mReqBody)
	{
		mReqBody->release();
		mReqBody = NULL;
	}
	
	if (mCurlHandle)
	{
		// Uncertain of thread context so free using
		// safest method.
		curl_easy_cleanup(mCurlHandle);
		mCurlHandle = NULL;
	}

	mCurlService = NULL;

	if (mCurlHeaders)
	{
		curl_slist_free_all(mCurlHeaders);
		mCurlHeaders = NULL;
	}

	delete [] mCurlTemp;
	mCurlTemp = NULL;
	mCurlTempLen = 0;
	
	if (mReplyBody)
	{
		mReplyBody->release();
		mReplyBody = NULL;
	}

}


void HttpOpRequest::stageFromRequest(HttpService * service)
{
    HttpOpRequest::ptr_t self(boost::dynamic_pointer_cast<HttpOpRequest>(shared_from_this()));
    service->getPolicy().addOp(self);			// transfers refcount
}


void HttpOpRequest::stageFromReady(HttpService * service)
{
    HttpOpRequest::ptr_t self(boost::dynamic_pointer_cast<HttpOpRequest>(shared_from_this()));
    service->getTransport().addOp(self);		// transfers refcount
}


void HttpOpRequest::stageFromActive(HttpService * service)
{
	if (mReplyLength)
	{
		// If non-zero, we received and processed a Content-Range
		// header with the response.  If there is received data
		// (and there may not be due to protocol violations,
		// HEAD requests, etc., see BUG-2295) Verify that what it
		// says is consistent with the received data.
		if (mReplyBody && mReplyBody->size() && mReplyLength != mReplyBody->size())
		{
			// Not as expected, fail the request
			mStatus = HttpStatus(HttpStatus::LLCORE, HE_INV_CONTENT_RANGE_HDR);
		}
	}
	
	if (mCurlHeaders)
	{
		// We take these headers out of the request now as they were
		// allocated originally in this thread and the notifier doesn't
		// need them.  This eliminates one source of heap moving across
		// threads.

		curl_slist_free_all(mCurlHeaders);
		mCurlHeaders = NULL;
	}

	// Also not needed on the other side
	delete [] mCurlTemp;
	mCurlTemp = NULL;
	mCurlTempLen = 0;
	
	addAsReply();
}


void HttpOpRequest::visitNotifier(HttpRequest * request)
{
	if (mUserHandler)
	{
		HttpResponse * response = new HttpResponse();
		response->setStatus(mStatus);
		response->setBody(mReplyBody);
		response->setHeaders(mReplyHeaders);
        response->setRequestURL(mReqURL);

        if (mReplyOffset || mReplyLength)
		{
			// Got an explicit offset/length in response
			response->setRange(mReplyOffset, mReplyLength, mReplyFullLength);
		}
		response->setContentType(mReplyConType);
		response->setRetries(mPolicyRetries, mPolicy503Retries);
		
		HttpResponse::TransferStats::ptr_t stats = HttpResponse::TransferStats::ptr_t(new HttpResponse::TransferStats);

		curl_easy_getinfo(mCurlHandle, CURLINFO_SIZE_DOWNLOAD, &stats->mSizeDownload);
		curl_easy_getinfo(mCurlHandle, CURLINFO_TOTAL_TIME, &stats->mTotalTime);
		curl_easy_getinfo(mCurlHandle, CURLINFO_SPEED_DOWNLOAD, &stats->mSpeedDownload);

		response->setTransferStats(stats);

		mUserHandler->onCompleted(this->getHandle(), response);

		response->release();
	}
}

// /*static*/
// HttpOpRequest::ptr_t HttpOpRequest::fromHandle(HttpHandle handle)
// {
// 
//     return boost::dynamic_pointer_cast<HttpOpRequest>((static_cast<HttpOpRequest *>(handle))->shared_from_this());
// }


HttpStatus HttpOpRequest::cancel()
{
	mStatus = HttpStatus(HttpStatus::LLCORE, HE_OP_CANCELED);

	addAsReply();

	return HttpStatus();
}


HttpStatus HttpOpRequest::setupGet(HttpRequest::policy_t policy_id,
								   HttpRequest::priority_t priority,
								   const std::string & url,
                                   const HttpOptions::ptr_t & options,
								   const HttpHeaders::ptr_t & headers)
{
	setupCommon(policy_id, priority, url, NULL, options, headers);
	mReqMethod = HOR_GET;
	
	return HttpStatus();
}


HttpStatus HttpOpRequest::setupGetByteRange(HttpRequest::policy_t policy_id,
											HttpRequest::priority_t priority,
											const std::string & url,
											size_t offset,
											size_t len,
                                            const HttpOptions::ptr_t & options,
                                            const HttpHeaders::ptr_t & headers)
{
	setupCommon(policy_id, priority, url, NULL, options, headers);
	mReqMethod = HOR_GET;
	mReqOffset = offset;
	mReqLength = len;
	if (offset || len)
	{
		mProcFlags |= PF_SCAN_RANGE_HEADER;
	}
	
	return HttpStatus();
}


HttpStatus HttpOpRequest::setupPost(HttpRequest::policy_t policy_id,
									HttpRequest::priority_t priority,
									const std::string & url,
									BufferArray * body,
                                    const HttpOptions::ptr_t & options,
                                    const HttpHeaders::ptr_t & headers)
{
	setupCommon(policy_id, priority, url, body, options, headers);
	mReqMethod = HOR_POST;
	
	return HttpStatus();
}


HttpStatus HttpOpRequest::setupPut(HttpRequest::policy_t policy_id,
								   HttpRequest::priority_t priority,
								   const std::string & url,
								   BufferArray * body,
                                   const HttpOptions::ptr_t & options,
								   const HttpHeaders::ptr_t & headers)
{
	setupCommon(policy_id, priority, url, body, options, headers);
	mReqMethod = HOR_PUT;
	
	return HttpStatus();
}


HttpStatus HttpOpRequest::setupDelete(HttpRequest::policy_t policy_id,
    HttpRequest::priority_t priority,
    const std::string & url,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t & headers)
{
    setupCommon(policy_id, priority, url, NULL, options, headers);
    mReqMethod = HOR_DELETE;

    return HttpStatus();
}


HttpStatus HttpOpRequest::setupPatch(HttpRequest::policy_t policy_id,
    HttpRequest::priority_t priority,
    const std::string & url,
    BufferArray * body,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t & headers)
{
    setupCommon(policy_id, priority, url, body, options, headers);
    mReqMethod = HOR_PATCH;

    return HttpStatus();
}


HttpStatus HttpOpRequest::setupCopy(HttpRequest::policy_t policy_id,
    HttpRequest::priority_t priority,
    const std::string & url,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t &headers)
{
    setupCommon(policy_id, priority, url, NULL, options, headers);
    mReqMethod = HOR_COPY;

    return HttpStatus();
}


HttpStatus HttpOpRequest::setupMove(HttpRequest::policy_t policy_id,
    HttpRequest::priority_t priority,
    const std::string & url,
    const HttpOptions::ptr_t & options,
    const HttpHeaders::ptr_t &headers)
{
    setupCommon(policy_id, priority, url, NULL, options, headers);
    mReqMethod = HOR_MOVE;

    return HttpStatus();
}


void HttpOpRequest::setupCommon(HttpRequest::policy_t policy_id,
								HttpRequest::priority_t priority,
								const std::string & url,
								BufferArray * body,
                                const HttpOptions::ptr_t & options,
								const HttpHeaders::ptr_t & headers)
{
	mProcFlags = 0U;
	mReqPolicy = policy_id;
	mReqPriority = priority;
	mReqURL = url;
	if (body)
	{
		body->addRef();
		mReqBody = body;
	}
	if (headers && ! mReqHeaders)
	{
		mReqHeaders = headers;
	}
	if (options && !mReqOptions)
	{
		mReqOptions = options;
		if (options->getWantHeaders())
		{
			mProcFlags |= PF_SAVE_HEADERS;
		}
		if (options->getUseRetryAfter())
		{
			mProcFlags |= PF_USE_RETRY_AFTER;
		}
		mPolicyRetryLimit = options->getRetries();
		mPolicyRetryLimit = llclamp(mPolicyRetryLimit, HTTP_RETRY_COUNT_MIN, HTTP_RETRY_COUNT_MAX);
		mTracing = (std::max)(mTracing, llclamp(options->getTrace(), HTTP_TRACE_MIN, HTTP_TRACE_MAX));

		mPolicyMinRetryBackoff = llclamp(options->getMinBackoff(), HttpTime(0), HTTP_RETRY_BACKOFF_MAX);
		mPolicyMaxRetryBackoff = llclamp(options->getMaxBackoff(), mPolicyMinRetryBackoff, HTTP_RETRY_BACKOFF_MAX);
	}
}

// Sets all libcurl options and data for a request.
//
// Used both for initial requests and to 'reload' for
// a retry, generally with a different CURL handle.
// Junk may be left around from a failed request and that
// needs to be cleaned out.
//
// *TODO:  Move this to _httplibcurl where it belongs.
HttpStatus HttpOpRequest::prepareRequest(HttpService * service)
{
	CURLcode code;
	
	// Scrub transport and result data for retried op case
	mCurlActive = false;
	mCurlHandle = NULL;
	mCurlService = NULL;
	if (mCurlHeaders)
	{
		curl_slist_free_all(mCurlHeaders);
		mCurlHeaders = NULL;
	}
	mCurlBodyPos = 0;

	if (mReplyBody)
	{
		mReplyBody->release();
		mReplyBody = NULL;
	}
	mReplyOffset = 0;
	mReplyLength = 0;
	mReplyFullLength = 0;
    mReplyHeaders.reset();
	mReplyConType.clear();
	
	// *FIXME:  better error handling later
	HttpStatus status;

	// Get global and class policy options
	HttpPolicyGlobal & gpolicy(service->getPolicy().getGlobalOptions());
	HttpPolicyClass & cpolicy(service->getPolicy().getClassOptions(mReqPolicy));
	
	mCurlHandle = service->getTransport().getHandle();
	if (! mCurlHandle)
	{
		// We're in trouble.  We'll continue but it won't go well.
		LL_WARNS(LOG_CORE) << "Failed to allocate libcurl easy handle.  Continuing."
						   << LL_ENDL;
		return HttpStatus(HttpStatus::LLCORE, HE_BAD_ALLOC);
	}

	code = curl_easy_setopt(mCurlHandle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	check_curl_easy_code(code, CURLOPT_IPRESOLVE);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_NOSIGNAL, 1);
	check_curl_easy_code(code, CURLOPT_NOSIGNAL);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_NOPROGRESS, 1);
	check_curl_easy_code(code, CURLOPT_NOPROGRESS);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_URL, mReqURL.c_str());
	check_curl_easy_code(code, CURLOPT_URL);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_PRIVATE, getHandle());
	check_curl_easy_code(code, CURLOPT_PRIVATE);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_ENCODING, "");
	check_curl_easy_code(code, CURLOPT_ENCODING);

	code = curl_easy_setopt(mCurlHandle, CURLOPT_AUTOREFERER, 1);
	check_curl_easy_code(code, CURLOPT_AUTOREFERER);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_MAXREDIRS, HTTP_REDIRECTS_DEFAULT);
	check_curl_easy_code(code, CURLOPT_MAXREDIRS);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_WRITEFUNCTION, writeCallback);
	check_curl_easy_code(code, CURLOPT_WRITEFUNCTION);
    code = curl_easy_setopt(mCurlHandle, CURLOPT_WRITEDATA, getHandle());
	check_curl_easy_code(code, CURLOPT_WRITEDATA);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_READFUNCTION, readCallback);
	check_curl_easy_code(code, CURLOPT_READFUNCTION);
    code = curl_easy_setopt(mCurlHandle, CURLOPT_READDATA, getHandle());
	check_curl_easy_code(code, CURLOPT_READDATA);
    code = curl_easy_setopt(mCurlHandle, CURLOPT_SEEKFUNCTION, seekCallback);
    check_curl_easy_code(code, CURLOPT_SEEKFUNCTION);
    code = curl_easy_setopt(mCurlHandle, CURLOPT_SEEKDATA, getHandle());
    check_curl_easy_code(code, CURLOPT_SEEKDATA);

	code = curl_easy_setopt(mCurlHandle, CURLOPT_COOKIEFILE, "");
	check_curl_easy_code(code, CURLOPT_COOKIEFILE);

	if (gpolicy.mSslCtxCallback)
	{
		code = curl_easy_setopt(mCurlHandle, CURLOPT_SSL_CTX_FUNCTION, curlSslCtxCallback);
		check_curl_easy_code(code, CURLOPT_SSL_CTX_FUNCTION);
        code = curl_easy_setopt(mCurlHandle, CURLOPT_SSL_CTX_DATA, getHandle());
		check_curl_easy_code(code, CURLOPT_SSL_CTX_DATA);
		mCallbackSSLVerify = gpolicy.mSslCtxCallback;
	}

	long follow_redirect(1L);
	long sslPeerV(0L);
	long sslHostV(0L);
    long dnsCacheTimeout(-1L);
    long nobody(0L);

	if (mReqOptions)
	{
		follow_redirect = mReqOptions->getFollowRedirects() ? 1L : 0L;
		sslPeerV = mReqOptions->getSSLVerifyPeer() ? 1L : 0L;
		sslHostV = mReqOptions->getSSLVerifyHost() ? 2L : 0L;
		dnsCacheTimeout = mReqOptions->getDNSCacheTimeout();
        nobody = mReqOptions->getHeadersOnly() ? 1L : 0L;
	}
	code = curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, follow_redirect);
	check_curl_easy_code(code, CURLOPT_FOLLOWLOCATION);

	code = curl_easy_setopt(mCurlHandle, CURLOPT_SSL_VERIFYPEER, sslPeerV);
	check_curl_easy_code(code, CURLOPT_SSL_VERIFYPEER);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_SSL_VERIFYHOST, sslHostV);
	check_curl_easy_code(code, CURLOPT_SSL_VERIFYHOST);

    code = curl_easy_setopt(mCurlHandle, CURLOPT_NOBODY, nobody);
    check_curl_easy_code(code, CURLOPT_NOBODY);

	// The Linksys WRT54G V5 router has an issue with frequent
	// DNS lookups from LAN machines.  If they happen too often,
	// like for every HTTP request, the router gets annoyed after
	// about 700 or so requests and starts issuing TCP RSTs to
	// new connections.  Reuse the DNS lookups for even a few
	// seconds and no RSTs.
	code = curl_easy_setopt(mCurlHandle, CURLOPT_DNS_CACHE_TIMEOUT, dnsCacheTimeout);
	check_curl_easy_code(code, CURLOPT_DNS_CACHE_TIMEOUT);

	if (gpolicy.mUseLLProxy)
	{
		// Use the viewer-based thread-safe API which has a
		// fast/safe check for proxy enable.  Would like to
		// encapsulate this someway...
		if (LLProxy::instanceExists())
		{
			// Make sure proxy won't be initialized from here,
			// it might conflict with LLStartUp::startLLProxy()
			LLProxy::getInstance()->applyProxySettings(mCurlHandle);
		}
		else
		{
			LL_WARNS() << "Proxy is not initialized!" << LL_ENDL;
		}

	}
	else if (gpolicy.mHttpProxy.size())
	{
		// *TODO:  This is fine for now but get fuller socks5/
		// authentication thing going later....
		code = curl_easy_setopt(mCurlHandle, CURLOPT_PROXY, gpolicy.mHttpProxy.c_str());
		check_curl_easy_code(code, CURLOPT_PROXY);
		code = curl_easy_setopt(mCurlHandle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
		check_curl_easy_code(code, CURLOPT_PROXYTYPE);
	}
	if (gpolicy.mCAPath.size())
	{
		code = curl_easy_setopt(mCurlHandle, CURLOPT_CAPATH, gpolicy.mCAPath.c_str());
		check_curl_easy_code(code, CURLOPT_CAPATH);
	}
	if (gpolicy.mCAFile.size())
	{
		code = curl_easy_setopt(mCurlHandle, CURLOPT_CAINFO, gpolicy.mCAFile.c_str());
		check_curl_easy_code(code, CURLOPT_CAINFO);
	}
	
	switch (mReqMethod)
	{
	case HOR_GET:
        if (nobody == 0)
            code = curl_easy_setopt(mCurlHandle, CURLOPT_HTTPGET, 1);
		check_curl_easy_code(code, CURLOPT_HTTPGET);
		break;
		
	case HOR_POST:
		{
			code = curl_easy_setopt(mCurlHandle, CURLOPT_POST, 1);
			check_curl_easy_code(code, CURLOPT_POST);
			code = curl_easy_setopt(mCurlHandle, CURLOPT_ENCODING, "");
			check_curl_easy_code(code, CURLOPT_ENCODING);
			long data_size(0);
			if (mReqBody)
			{
				data_size = mReqBody->size();
			}
			code = curl_easy_setopt(mCurlHandle, CURLOPT_POSTFIELDS, static_cast<void *>(NULL));
			check_curl_easy_code(code, CURLOPT_POSTFIELDS);
			code = curl_easy_setopt(mCurlHandle, CURLOPT_POSTFIELDSIZE, data_size);
			check_curl_easy_code(code, CURLOPT_POSTFIELDSIZE);
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Expect:");
		}
		break;
		
    case HOR_PATCH:
        code = curl_easy_setopt(mCurlHandle, CURLOPT_CUSTOMREQUEST, "PATCH");
        check_curl_easy_code(code, CURLOPT_CUSTOMREQUEST);
        // fall through.  The rest is the same as PUT
    case HOR_PUT:
		{
			code = curl_easy_setopt(mCurlHandle, CURLOPT_UPLOAD, 1);
			check_curl_easy_code(code, CURLOPT_UPLOAD);
			long data_size(0);
			if (mReqBody)
			{
				data_size = mReqBody->size();
			}
			code = curl_easy_setopt(mCurlHandle, CURLOPT_INFILESIZE, data_size);
			check_curl_easy_code(code, CURLOPT_INFILESIZE);
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Expect:");
		}
		break;
		
    case HOR_DELETE:
        code = curl_easy_setopt(mCurlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
        check_curl_easy_code(code, CURLOPT_CUSTOMREQUEST);
        break;

    case HOR_COPY:
        code = curl_easy_setopt(mCurlHandle, CURLOPT_CUSTOMREQUEST, "COPY");
        check_curl_easy_code(code, CURLOPT_CUSTOMREQUEST);
        break;

    case HOR_MOVE:
        code = curl_easy_setopt(mCurlHandle, CURLOPT_CUSTOMREQUEST, "MOVE");
        check_curl_easy_code(code, CURLOPT_CUSTOMREQUEST);
        break;

	default:
		LL_ERRS(LOG_CORE) << "Invalid HTTP method in request:  "
						  << int(mReqMethod)  << ".  Can't recover."
						  << LL_ENDL;
		break;
	}


    // *TODO: Should this be 'Keep-Alive' ?
    mCurlHeaders = curl_slist_append(mCurlHeaders, "Connection: keep-alive");
    mCurlHeaders = curl_slist_append(mCurlHeaders, "Keep-alive: 300");

	// Tracing
	if (mTracing >= HTTP_TRACE_CURL_HEADERS)
	{
		code = curl_easy_setopt(mCurlHandle, CURLOPT_VERBOSE, 1);
		check_curl_easy_code(code, CURLOPT_VERBOSE);
		code = curl_easy_setopt(mCurlHandle, CURLOPT_DEBUGDATA, this);
		check_curl_easy_code(code, CURLOPT_DEBUGDATA);
		code = curl_easy_setopt(mCurlHandle, CURLOPT_DEBUGFUNCTION, debugCallback);
		check_curl_easy_code(code, CURLOPT_DEBUGFUNCTION);
	}
	
	// There's a CURLOPT for this now...
	if ((mReqOffset || mReqLength) && HOR_GET == mReqMethod)
	{
		static const char * const fmt1("Range: bytes=%lu-%lu");
		static const char * const fmt2("Range: bytes=%lu-");

		char range_line[64];

#if LL_WINDOWS
		_snprintf_s(range_line, sizeof(range_line), sizeof(range_line) - 1,
					(mReqLength ? fmt1 : fmt2),
					(unsigned long) mReqOffset, (unsigned long) (mReqOffset + mReqLength - 1));
#else
		if ( mReqLength )
		{
			snprintf(range_line, sizeof(range_line),
					 fmt1,
					 (unsigned long) mReqOffset, (unsigned long) (mReqOffset + mReqLength - 1));
		}
		else
		{
			snprintf(range_line, sizeof(range_line),
					 fmt2,
					 (unsigned long) mReqOffset);
		}
#endif // LL_WINDOWS
		range_line[sizeof(range_line) - 1] = '\0';
		mCurlHeaders = curl_slist_append(mCurlHeaders, range_line);
	}

	mCurlHeaders = curl_slist_append(mCurlHeaders, "Pragma:");

	// Request options
	long timeout(HTTP_REQUEST_TIMEOUT_DEFAULT);
	long xfer_timeout(HTTP_REQUEST_XFER_TIMEOUT_DEFAULT);
	if (mReqOptions)
 	{
		timeout = mReqOptions->getTimeout();
		timeout = llclamp(timeout, HTTP_REQUEST_TIMEOUT_MIN, HTTP_REQUEST_TIMEOUT_MAX);
		xfer_timeout = mReqOptions->getTransferTimeout();
		xfer_timeout = llclamp(xfer_timeout, HTTP_REQUEST_TIMEOUT_MIN, HTTP_REQUEST_TIMEOUT_MAX);
	}
	if (xfer_timeout == 0L)
	{
		xfer_timeout = timeout;
	}
	if (cpolicy.mPipelining > 1L)
	{
		// Pipelining affects both connection and transfer timeout values.
		// Requests that are added to a pipeling immediately have completed
		// their connection so the connection delay tends to be less than
		// the non-pipelined value.  Transfers are the opposite.  Transfer
		// timeout starts once the connection is established and completion
		// can be delayed due to the pipelined requests ahead.  So, it's
		// a handwave but bump the transfer timeout up by the pipelining
		// depth to give some room.
		//
		// BUG-7698, BUG-7688, BUG-7694 (others).  Scylla and Charybdis
		// situation.  Operating against a CDN having service issues may
		// lead to requests stalling for an arbitrarily long time with only
		// the CURLOPT_TIMEOUT value leading to a closed connection.  Sadly
		// for pipelining, libcurl (7.39.0 and earlier, at minimum) starts
		// the clock on this value as soon as a request is started down
		// the wire.  We want a short value to recover and retry from the
		// CDN.  We need a long value to safely deal with a succession of
		// piled-up pipelined requests.
		//
		// *TODO:  Find a better scheme than timeouts to guarantee liveness.
		// Progress on the connection is what we really want, not timeouts.
		// But we don't have access to that and the request progress indicators
		// (various libcurl callbacks) have the same problem TIMEOUT does.
		//
		// xfer_timeout *= cpolicy.mPipelining;
		xfer_timeout *= 2L;
	}
	// *DEBUG:  Enable following override for timeout handling and "[curl:bugs] #1420" tests
    //if (cpolicy.mPipelining)
    //{
    //    xfer_timeout = 1L;
    //    timeout = 1L;
    //}
	code = curl_easy_setopt(mCurlHandle, CURLOPT_TIMEOUT, xfer_timeout);
	check_curl_easy_code(code, CURLOPT_TIMEOUT);
	code = curl_easy_setopt(mCurlHandle, CURLOPT_CONNECTTIMEOUT, timeout);
	check_curl_easy_code(code, CURLOPT_CONNECTTIMEOUT);

	// Request headers
	if (mReqHeaders)
	{
		// Caller's headers last to override
		mCurlHeaders = append_headers_to_slist(mReqHeaders, mCurlHeaders);
	}
	code = curl_easy_setopt(mCurlHandle, CURLOPT_HTTPHEADER, mCurlHeaders);
	check_curl_easy_code(code, CURLOPT_HTTPHEADER);

	if (mProcFlags & (PF_SCAN_RANGE_HEADER | PF_SAVE_HEADERS | PF_USE_RETRY_AFTER))
	{
		code = curl_easy_setopt(mCurlHandle, CURLOPT_HEADERFUNCTION, headerCallback);
		check_curl_easy_code(code, CURLOPT_HEADERFUNCTION);
		code = curl_easy_setopt(mCurlHandle, CURLOPT_HEADERDATA, this);
		check_curl_easy_code(code, CURLOPT_HEADERDATA);
	}
	
	if (status)
	{
		mCurlService = service;
	}
	return status;
}


size_t HttpOpRequest::writeCallback(void * data, size_t size, size_t nmemb, void * userdata)
{
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(userdata));

	if (! op->mReplyBody)
	{
		op->mReplyBody = new BufferArray();
	}
	const size_t req_size(size * nmemb);
	const size_t write_size(op->mReplyBody->append(static_cast<char *>(data), req_size));
	return write_size;
}

		
size_t HttpOpRequest::readCallback(void * data, size_t size, size_t nmemb, void * userdata)
{
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(userdata));

	if (! op->mReqBody)
	{
		return 0;
	}
	const size_t req_size(size * nmemb);
	const size_t body_size(op->mReqBody->size());
	if (body_size <= op->mCurlBodyPos)
	{
		if (body_size < op->mCurlBodyPos)
		{
			// Warn but continue if the read position moves beyond end-of-body
			// for some reason.
			LL_WARNS(LOG_CORE) << "Request body position beyond body size.  Truncating request body."
							   << LL_ENDL;
		}
		return 0;
	}

	const size_t do_size((std::min)(req_size, body_size - op->mCurlBodyPos));
	const size_t read_size(op->mReqBody->read(op->mCurlBodyPos, static_cast<char *>(data), do_size));
	op->mCurlBodyPos += read_size;
	return read_size;
}


int HttpOpRequest::seekCallback(void *userdata, curl_off_t offset, int origin)
{
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(userdata));

    if (!op->mReqBody)
    {
        return 0;
    }

    size_t newPos = 0;
    if (origin == SEEK_SET)
        newPos = offset;
    else if (origin == SEEK_END)
        newPos = static_cast<curl_off_t>(op->mReqBody->size()) + offset;
    else if (origin == SEEK_CUR)
        newPos = static_cast<curl_off_t>(op->mCurlBodyPos) + offset;
    else
        return 2;

    if (newPos >= op->mReqBody->size())
    {
        LL_WARNS(LOG_CORE) << "Attempt to seek to position outside post body." << LL_ENDL;
        return 2;
    }

    op->mCurlBodyPos = (size_t)newPos;

    return 0;
}

		
size_t HttpOpRequest::headerCallback(void * data, size_t size, size_t nmemb, void * userdata)
{
	static const char status_line[] = "HTTP/";
	static const size_t status_line_len = sizeof(status_line) - 1;
	static const char con_ran_line[] = "content-range";
	static const char con_retry_line[] = "retry-after";
	
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(userdata));

	const size_t hdr_size(size * nmemb);
	const char * hdr_data(static_cast<const char *>(data));		// Not null terminated
	bool is_header(true);
	
	if (hdr_size >= status_line_len && ! strncmp(status_line, hdr_data, status_line_len))
	{
		// One of possibly several status lines.  Reset what we know and start over
		// taking results from the last header stanza we receive.
		op->mReplyOffset = 0;
		op->mReplyLength = 0;
		op->mReplyFullLength = 0;
		op->mReplyRetryAfter = 0;
		op->mStatus = HttpStatus();
		if (op->mReplyHeaders)
		{
			op->mReplyHeaders->clear();
		}
		is_header = false;
	}

	// Nothing in here wants a final CR/LF combination.  Remove
	// it as much as possible.
	size_t wanted_hdr_size(hdr_size);
	if (wanted_hdr_size && '\n' == hdr_data[wanted_hdr_size - 1])
	{
		if (--wanted_hdr_size && '\r' == hdr_data[wanted_hdr_size - 1])
		{
			--wanted_hdr_size;
		}
	}

	// Copy and normalize header fragments for the following
	// stages.  Would like to modify the data in-place but that
	// may not be allowed and we need one byte extra for NUL.
	// At the end of this we will have:
	//
	// If ':' present in header:
	//   1.  name points to text to left of colon which
	//       will be ascii lower-cased and left and right
	//       trimmed of whitespace.
	//   2.  value points to text to right of colon which
	//       will be left trimmed of whitespace.
	// Otherwise:
	//   1.  name points to header which will be left
	//       trimmed of whitespace.
	//   2.  value is NULL
	// Any non-NULL pointer may point to a zero-length string.
	//
	if (wanted_hdr_size >= op->mCurlTempLen)
	{
		delete [] op->mCurlTemp;
		op->mCurlTempLen = 2 * wanted_hdr_size + 1;
		op->mCurlTemp = new char [op->mCurlTempLen];
	}
	memcpy(op->mCurlTemp, hdr_data, wanted_hdr_size);
	op->mCurlTemp[wanted_hdr_size] = '\0';
	char * name(op->mCurlTemp);
	char * value(strchr(name, ':'));
	if (value)
	{
		*value++ = '\0';
		os_strlower(name);
		name = os_strtrim(name);
		value = os_strltrim(value);
	}
	else
	{
		// Doesn't look well-formed, do minimal normalization on it
		name = os_strltrim(name);
	}

	// Normalized, now reject headers with empty names.
	if (! *name)
	{
		// No use continuing
		return hdr_size;
	}
	
	// Save header if caller wants them in the response
	if (is_header && op->mProcFlags & PF_SAVE_HEADERS)
	{
		// Save headers in response
		if (! op->mReplyHeaders)
		{
			op->mReplyHeaders = HttpHeaders::ptr_t(new HttpHeaders);
		}
		op->mReplyHeaders->append(name, value ? value : "");
	}

	// From this point, header-specific processors are free to
	// modify the header value.
	
	// Detect and parse 'Content-Range' headers
	if (is_header
		&& op->mProcFlags & PF_SCAN_RANGE_HEADER
		&& value && *value
		&& ! strcmp(name, con_ran_line))
	{
		unsigned int first(0), last(0), length(0);
		int status;

		if (! (status = parse_content_range_header(value, &first, &last, &length)))
		{
			// Success, record the fragment position
			op->mReplyOffset = first;
			op->mReplyLength = last - first + 1;
			op->mReplyFullLength = length;
		}
		else if (-1 == status)
		{
			// Response is badly formed and shouldn't be accepted
			op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_INV_CONTENT_RANGE_HDR);
		}
		else
		{
			// Ignore the unparsable.
			LL_INFOS_ONCE(LOG_CORE) << "Problem parsing odd Content-Range header:  '"
									<< std::string(hdr_data, wanted_hdr_size)
									<< "'.  Ignoring."
									<< LL_ENDL;
		}
	}

	// Detect and parse 'Retry-After' headers
	if (is_header
		&& op->mProcFlags & PF_USE_RETRY_AFTER
		&& value && *value
		&& ! strcmp(name, con_retry_line))
	{
		int time(0);
		if (! parse_retry_after_header(value, &time))
		{
			op->mReplyRetryAfter = time;
		}
	}

	return hdr_size;
}


CURLcode HttpOpRequest::curlSslCtxCallback(CURL *curl, void *sslctx, void *userdata)
{
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(userdata));

	if (op->mCallbackSSLVerify)
	{
		SSL_CTX * ctx = (SSL_CTX *)sslctx;
		// disable any default verification for server certs
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
		// set the verification callback.
		SSL_CTX_set_cert_verify_callback(ctx, sslCertVerifyCallback, userdata);
		// the calls are void
	}

	return CURLE_OK;
}

int HttpOpRequest::sslCertVerifyCallback(X509_STORE_CTX *ctx, void *param)
{
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(param));

	if (op->mCallbackSSLVerify)
	{
		op->mStatus = op->mCallbackSSLVerify(op->mReqURL, op->mUserHandler, ctx);
	}

	return (op->mStatus) ? 1 : 0;
}

int HttpOpRequest::debugCallback(CURL * handle, curl_infotype info, char * buffer, size_t len, void * userdata)
{
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(userdata));

	std::string safe_line;
	std::string tag;
	bool logit(false);
	const size_t log_len((std::min)(len, size_t(256)));		// Keep things reasonable in all cases
	
	switch (info)
	{
	case CURLINFO_TEXT:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "TEXT";
			escape_libcurl_debug_data(buffer, log_len, true, safe_line);
			logit = true;
		}
		break;
			
	case CURLINFO_HEADER_IN:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "HEADERIN";
			escape_libcurl_debug_data(buffer, log_len, true, safe_line);
			logit = true;
		}
		break;
			
	case CURLINFO_HEADER_OUT:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "HEADEROUT";
			escape_libcurl_debug_data(buffer, log_len, true, safe_line);	// Goes out as one line unlike header_in
			logit = true;
		}
		break;
			
	case CURLINFO_DATA_IN:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "DATAIN";
			logit = true;
			if (op->mTracing >= HTTP_TRACE_CURL_BODIES)
			{
				escape_libcurl_debug_data(buffer, log_len, false, safe_line);
			}
			else
			{
				std::ostringstream out;
				out << len << " Bytes";
				safe_line = out.str();
			}
		}
		break;
			
	case CURLINFO_DATA_OUT:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "DATAOUT";
			logit = true;
			if (op->mTracing >= HTTP_TRACE_CURL_BODIES)
			{
				escape_libcurl_debug_data(buffer, log_len, false, safe_line);
			}
			else
			{
				std::ostringstream out;
				out << len << " Bytes";
				safe_line = out.str();
			}
		}
		break;
			
	default:
		logit = false;
		break;
	}

	if (logit)
	{
		LL_INFOS(LOG_CORE) << "TRACE, LibcurlDebug, Handle:  "
						   << op->getHandle()
						   << ", Type:  " << tag
						   << ", Data:  " << safe_line
						   << LL_ENDL;
	}
		
	return 0;
}


}   // end namespace LLCore


// =======================================
// Anonymous Namespace
// =======================================

namespace
{

int parse_content_range_header(char * buffer,
							   unsigned int * first,
							   unsigned int * last,
							   unsigned int * length)
{
	static const char * const hdr_whitespace(" \t");

	char * tok_state(NULL), * tok(NULL);
	bool match(true);
			
	if (! (tok = os_strtok_r(buffer, hdr_whitespace, &tok_state)))
		match = false;
	else
		match = (0 == os_strcasecmp("bytes", tok));
	if (match && ! (tok = os_strtok_r(NULL, hdr_whitespace, &tok_state)))
		match = false;
	if (match)
	{
		unsigned int lcl_first(0), lcl_last(0), lcl_len(0);

#if LL_WINDOWS
		if (3 == sscanf_s(tok, "%u-%u/%u", &lcl_first, &lcl_last, &lcl_len))
#else
		if (3 == sscanf(tok, "%u-%u/%u", &lcl_first, &lcl_last, &lcl_len))
#endif // LL_WINDOWS
		{
			if (lcl_first > lcl_last || lcl_last >= lcl_len)
				return -1;
			*first = lcl_first;
			*last = lcl_last;
			*length = lcl_len;
			return 0;
		}
#if LL_WINDOWS
		if (2 == sscanf_s(tok, "%u-%u/*", &lcl_first, &lcl_last))
#else
		if (2 == sscanf(tok, "%u-%u/*", &lcl_first, &lcl_last))
#endif	// LL_WINDOWS
		{
			if (lcl_first > lcl_last)
				return -1;
			*first = lcl_first;
			*last = lcl_last;
			*length = 0;
			return 0;
		}
	}

	// Header is there but badly/unexpectedly formed, try to ignore it.
	return 1;
}


int parse_retry_after_header(char * buffer, int * time)
{
	char * endptr(buffer);
	long lcl_time(strtol(buffer, &endptr, 10));
	if (*endptr == '\0' && endptr != buffer && lcl_time > 0)
	{
		*time = lcl_time;
		return 0;
	}

	// Could attempt to parse HTTP time here but we're not really
	// interested in it.  Scheduling based on wallclock time on
	// user hardware will lead to tears.
	
	// Header is there but badly/unexpectedly formed, try to ignore it.
	return 1;
}


void escape_libcurl_debug_data(char * buffer, size_t len, bool scrub, std::string & safe_line)
{
	std::string out;
	len = (std::min)(len, size_t(200));
	out.reserve(3 * len);
	for (int i(0); i < len; ++i)
	{
		unsigned char uc(static_cast<unsigned char>(buffer[i]));

		if (uc < 32 || uc > 126)
		{
			if (scrub)
			{
				out.append(1, ' ');
			}
			else
			{
				static const char hex[] = "0123456789ABCDEF";
				char convert[4];

				convert[0] = '%';
				convert[1] = hex[(uc >> 4) % 16];
				convert[2] = hex[uc % 16];
				convert[3] = '\0';
				out.append(convert);
			}
		}
		else
		{
			out.append(1, buffer[i]);
		}
	}
	safe_line.swap(out);
}



int os_strcasecmp(const char *s1, const char *s2)
{
#if LL_WINDOWS
	return _stricmp(s1, s2);
#else
	return strcasecmp(s1, s2);
#endif // LL_WINDOWS
}


char * os_strtok_r(char *str, const char *delim, char ** savestate)
{
#if LL_WINDOWS
	return strtok_s(str, delim, savestate);
#else
	return strtok_r(str, delim, savestate);
#endif
}


void os_strlower(char * str)
{
	for (char c(0); (c = *str); ++str)
	{
		*str = tolower(c);
	}
}


char * os_strtrim(char * lstr)
{
	while (' ' == *lstr || '\t' == *lstr)
	{
		++lstr;
	}
	if (*lstr)
	{
		char * rstr(lstr + strlen(lstr));
		while (lstr < rstr && *--rstr)
		{
			if (' ' == *rstr || '\t' == *rstr)
			{
				*rstr = '\0';
			}
		}
		llassert(lstr <= rstr);
	}
	return lstr;
}


char * os_strltrim(char * lstr)
{
	while (' ' == *lstr || '\t' == *lstr)
	{
		++lstr;
	}
	return lstr;
}


void check_curl_easy_code(CURLcode code, int curl_setopt_option)
{
	if (CURLE_OK != code)
	{
		// Comment from old llcurl code which may no longer apply:
		//
		// linux appears to throw a curl error once per session for a bad initialization
		// at a pretty random time (when enabling cookies).
		LL_WARNS(LOG_CORE) << "libcurl error detected:  " << curl_easy_strerror(code)
						   << ", curl_easy_setopt option:  " << curl_setopt_option
						   << LL_ENDL;
	}
}

}  // end anonymous namespace
