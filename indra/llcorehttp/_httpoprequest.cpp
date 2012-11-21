/**
 * @file _httpoprequest.cpp
 * @brief Definitions for internal class HttpOpRequest
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

#include "llhttpstatuscodes.h"
#include "llproxy.h"

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


// Take data from libcurl's CURLOPT_DEBUGFUNCTION callback and
// escape and format it for a tracing line in logging.  Absolutely
// anything including NULs can be in the data.  If @scrub is true,
// non-printing or non-ascii characters are replaced with spaces
// otherwise a %XX form of escaping is used.
void escape_libcurl_debug_data(char * buffer, size_t len, bool scrub,
							   std::string & safe_line);


// OS-neutral string comparisons of various types
int os_strncasecmp(const char *s1, const char *s2, size_t n);
int os_strcasecmp(const char *s1, const char *s2);
char * os_strtok_r(char *str, const char *delim, char **saveptr);


static const char * const hdr_whitespace(" \t");
static const char * const hdr_separator(": \t");

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
	  mReqHeaders(NULL),
	  mReqOptions(NULL),
	  mCurlActive(false),
	  mCurlHandle(NULL),
	  mCurlService(NULL),
	  mCurlHeaders(NULL),
	  mCurlBodyPos(0),
	  mReplyBody(NULL),
	  mReplyOffset(0),
	  mReplyLength(0),
	  mReplyFullLength(0),
	  mReplyHeaders(NULL),
	  mPolicyRetries(0),
	  mPolicyRetryAt(HttpTime(0)),
	  mPolicyRetryLimit(HTTP_RETRY_COUNT_DEFAULT)
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
	
	if (mReqOptions)
	{
		mReqOptions->release();
		mReqOptions = NULL;
	}

	if (mReqHeaders)
	{
		mReqHeaders->release();
		mReqHeaders = NULL;
	}

	if (mCurlHandle)
	{
		curl_easy_cleanup(mCurlHandle);
		mCurlHandle = NULL;
	}

	mCurlService = NULL;

	if (mCurlHeaders)
	{
		curl_slist_free_all(mCurlHeaders);
		mCurlHeaders = NULL;
	}

	if (mReplyBody)
	{
		mReplyBody->release();
		mReplyBody = NULL;
	}

	if (mReplyHeaders)
	{
		mReplyHeaders->release();
		mReplyHeaders = NULL;
	}
}


void HttpOpRequest::stageFromRequest(HttpService * service)
{
	addRef();
	service->getPolicy().addOp(this);			// transfers refcount
}


void HttpOpRequest::stageFromReady(HttpService * service)
{
	addRef();
	service->getTransport().addOp(this);		// transfers refcount
}


void HttpOpRequest::stageFromActive(HttpService * service)
{
	if (mReplyLength)
	{
		// If non-zero, we received and processed a Content-Range
		// header with the response.  Verify that what it says
		// is consistent with the received data.
		if (mReplyLength != mReplyBody->size())
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
		if (mReplyOffset || mReplyLength)
		{
			// Got an explicit offset/length in response
			response->setRange(mReplyOffset, mReplyLength, mReplyFullLength);
		}
		response->setContentType(mReplyConType);
		
		mUserHandler->onCompleted(static_cast<HttpHandle>(this), response);

		response->release();
	}
}


HttpStatus HttpOpRequest::cancel()
{
	mStatus = HttpStatus(HttpStatus::LLCORE, HE_OP_CANCELED);

	addAsReply();

	return HttpStatus();
}


HttpStatus HttpOpRequest::setupGet(HttpRequest::policy_t policy_id,
								   HttpRequest::priority_t priority,
								   const std::string & url,
								   HttpOptions * options,
								   HttpHeaders * headers)
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
											HttpOptions * options,
											HttpHeaders * headers)
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
									HttpOptions * options,
									HttpHeaders * headers)
{
	setupCommon(policy_id, priority, url, body, options, headers);
	mReqMethod = HOR_POST;
	
	return HttpStatus();
}


HttpStatus HttpOpRequest::setupPut(HttpRequest::policy_t policy_id,
								   HttpRequest::priority_t priority,
								   const std::string & url,
								   BufferArray * body,
								   HttpOptions * options,
								   HttpHeaders * headers)
{
	setupCommon(policy_id, priority, url, body, options, headers);
	mReqMethod = HOR_PUT;
	
	return HttpStatus();
}


void HttpOpRequest::setupCommon(HttpRequest::policy_t policy_id,
								HttpRequest::priority_t priority,
								const std::string & url,
								BufferArray * body,
								HttpOptions * options,
								HttpHeaders * headers)
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
		headers->addRef();
		mReqHeaders = headers;
	}
	if (options && ! mReqOptions)
	{
		options->addRef();
		mReqOptions = options;
		if (options->getWantHeaders())
		{
			mProcFlags |= PF_SAVE_HEADERS;
		}
		mPolicyRetryLimit = options->getRetries();
		mPolicyRetryLimit = llclamp(mPolicyRetryLimit, HTTP_RETRY_COUNT_MIN, HTTP_RETRY_COUNT_MAX);
		mTracing = (std::max)(mTracing, llclamp(options->getTrace(), HTTP_TRACE_MIN, HTTP_TRACE_MAX));
	}
}


// Sets all libcurl options and data for a request.
//
// Used both for initial requests and to 'reload' for
// a retry, generally with a different CURL handle.
// Junk may be left around from a failed request and that
// needs to be cleaned out.
//
HttpStatus HttpOpRequest::prepareRequest(HttpService * service)
{
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
	if (mReplyHeaders)
	{
		mReplyHeaders->release();
		mReplyHeaders = NULL;
	}
	mReplyConType.clear();
	
	// *FIXME:  better error handling later
	HttpStatus status;

	// Get policy options
	HttpPolicyGlobal & policy(service->getPolicy().getGlobalOptions());
	
	mCurlHandle = curl_easy_init();
	curl_easy_setopt(mCurlHandle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	curl_easy_setopt(mCurlHandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_URL, mReqURL.c_str());
	curl_easy_setopt(mCurlHandle, CURLOPT_PRIVATE, this);
	curl_easy_setopt(mCurlHandle, CURLOPT_ENCODING, "");

	if (HTTP_ENABLE_LINKSYS_WRT54G_V5_DNS_FIX)
	{
		// The Linksys WRT54G V5 router has an issue with frequent
		// DNS lookups from LAN machines.  If they happen too often,
		// like for every HTTP request, the router gets annoyed after
		// about 700 or so requests and starts issuing TCP RSTs to
		// new connections.  Reuse the DNS lookups for even a few
		// seconds and no RSTs.
		curl_easy_setopt(mCurlHandle, CURLOPT_DNS_CACHE_TIMEOUT, 15);
	}
	else
	{
		// *TODO:  Revisit this old DNS timeout setting - may no longer be valid
		// I don't think this is valid anymore, the Multi shared DNS
		// cache is working well.  For the case of naked easy handles,
		// consider using a shared DNS object.
		curl_easy_setopt(mCurlHandle, CURLOPT_DNS_CACHE_TIMEOUT, 0);
	}
	curl_easy_setopt(mCurlHandle, CURLOPT_AUTOREFERER, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_MAXREDIRS, HTTP_REDIRECTS_DEFAULT);
	curl_easy_setopt(mCurlHandle, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(mCurlHandle, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(mCurlHandle, CURLOPT_READFUNCTION, readCallback);
	curl_easy_setopt(mCurlHandle, CURLOPT_READDATA, this);
	curl_easy_setopt(mCurlHandle, CURLOPT_SSL_VERIFYPEER, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_SSL_VERIFYHOST, 0);

	const std::string * opt_value(NULL);
	long opt_long(0L);
	policy.get(HttpRequest::GP_LLPROXY, &opt_long);
	if (opt_long)
	{
		// Use the viewer-based thread-safe API which has a
		// fast/safe check for proxy enable.  Would like to
		// encapsulate this someway...
		LLProxy::getInstance()->applyProxySettings(mCurlHandle);
	}
	else if (policy.get(HttpRequest::GP_HTTP_PROXY, &opt_value))
	{
		// *TODO:  This is fine for now but get fuller socks5/
		// authentication thing going later....
		curl_easy_setopt(mCurlHandle, CURLOPT_PROXY, opt_value->c_str());
		curl_easy_setopt(mCurlHandle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
	}
	if (policy.get(HttpRequest::GP_CA_PATH, &opt_value))
	{
		curl_easy_setopt(mCurlHandle, CURLOPT_CAPATH, opt_value->c_str());
	}
	if (policy.get(HttpRequest::GP_CA_FILE, &opt_value))
	{
		curl_easy_setopt(mCurlHandle, CURLOPT_CAINFO, opt_value->c_str());
	}
	
	switch (mReqMethod)
	{
	case HOR_GET:
		curl_easy_setopt(mCurlHandle, CURLOPT_HTTPGET, 1);
		mCurlHeaders = curl_slist_append(mCurlHeaders, "Connection: keep-alive");
		mCurlHeaders = curl_slist_append(mCurlHeaders, "Keep-alive: 300");
		break;
		
	case HOR_POST:
		{
			curl_easy_setopt(mCurlHandle, CURLOPT_POST, 1);
			curl_easy_setopt(mCurlHandle, CURLOPT_ENCODING, "");
			long data_size(0);
			if (mReqBody)
			{
				data_size = mReqBody->size();
			}
			curl_easy_setopt(mCurlHandle, CURLOPT_POSTFIELDS, static_cast<void *>(NULL));
			curl_easy_setopt(mCurlHandle, CURLOPT_POSTFIELDSIZE, data_size);
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Expect:");
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Connection: keep-alive");
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Keep-alive: 300");
		}
		break;
		
	case HOR_PUT:
		{
			curl_easy_setopt(mCurlHandle, CURLOPT_UPLOAD, 1);
			long data_size(0);
			if (mReqBody)
			{
				data_size = mReqBody->size();
			}
			curl_easy_setopt(mCurlHandle, CURLOPT_INFILESIZE, data_size);
			curl_easy_setopt(mCurlHandle, CURLOPT_POSTFIELDS, (void *) NULL);
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Expect:");
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Connection: keep-alive");
			mCurlHeaders = curl_slist_append(mCurlHeaders, "Keep-alive: 300");
		}
		break;
		
	default:
		LL_ERRS("CoreHttp") << "Invalid HTTP method in request:  "
							<< int(mReqMethod)  << ".  Can't recover."
							<< LL_ENDL;
		break;
	}

	// Tracing
	if (mTracing >= HTTP_TRACE_CURL_HEADERS)
	{
		curl_easy_setopt(mCurlHandle, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(mCurlHandle, CURLOPT_DEBUGDATA, this);
		curl_easy_setopt(mCurlHandle, CURLOPT_DEBUGFUNCTION, debugCallback);
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
		snprintf(range_line, sizeof(range_line),
				 (mReqLength ? fmt1 : fmt2),
				 (unsigned long) mReqOffset, (unsigned long) (mReqOffset + mReqLength - 1));
#endif // LL_WINDOWS
		range_line[sizeof(range_line) - 1] = '\0';
		mCurlHeaders = curl_slist_append(mCurlHeaders, range_line);
	}

	mCurlHeaders = curl_slist_append(mCurlHeaders, "Pragma:");

	// Request options
	long timeout(HTTP_REQUEST_TIMEOUT_DEFAULT);
	if (mReqOptions)
	{
		timeout = mReqOptions->getTimeout();
		timeout = llclamp(timeout, HTTP_REQUEST_TIMEOUT_MIN, HTTP_REQUEST_TIMEOUT_MAX);
	}
	curl_easy_setopt(mCurlHandle, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(mCurlHandle, CURLOPT_CONNECTTIMEOUT, timeout);

	// Request headers
	if (mReqHeaders)
	{
		// Caller's headers last to override
		mCurlHeaders = append_headers_to_slist(mReqHeaders, mCurlHeaders);
	}
	curl_easy_setopt(mCurlHandle, CURLOPT_HTTPHEADER, mCurlHeaders);

	if (mProcFlags & (PF_SCAN_RANGE_HEADER | PF_SAVE_HEADERS))
	{
		curl_easy_setopt(mCurlHandle, CURLOPT_HEADERFUNCTION, headerCallback);
		curl_easy_setopt(mCurlHandle, CURLOPT_HEADERDATA, this);
	}
	
	if (status)
	{
		mCurlService = service;
	}
	return status;
}


size_t HttpOpRequest::writeCallback(void * data, size_t size, size_t nmemb, void * userdata)
{
	HttpOpRequest * op(static_cast<HttpOpRequest *>(userdata));

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
	HttpOpRequest * op(static_cast<HttpOpRequest *>(userdata));

	if (! op->mReqBody)
	{
		return 0;
	}
	const size_t req_size(size * nmemb);
	const size_t body_size(op->mReqBody->size());
	if (body_size <= op->mCurlBodyPos)
	{
		LL_WARNS("HttpCore") << "Request body position beyond body size.  Aborting request."
							 << LL_ENDL;
		return 0;
	}

	const size_t do_size((std::min)(req_size, body_size - op->mCurlBodyPos));
	const size_t read_size(op->mReqBody->read(op->mCurlBodyPos, static_cast<char *>(data), do_size));
	op->mCurlBodyPos += read_size;
	return read_size;
}

		
size_t HttpOpRequest::headerCallback(void * data, size_t size, size_t nmemb, void * userdata)
{
	static const char status_line[] = "HTTP/";
	static const size_t status_line_len = sizeof(status_line) - 1;

	static const char con_ran_line[] = "content-range:";
	static const size_t con_ran_line_len = sizeof(con_ran_line) - 1;

	HttpOpRequest * op(static_cast<HttpOpRequest *>(userdata));

	const size_t hdr_size(size * nmemb);
	const char * hdr_data(static_cast<const char *>(data));		// Not null terminated

	if (hdr_size >= status_line_len && ! strncmp(status_line, hdr_data, status_line_len))
	{
		// One of possibly several status lines.  Reset what we know and start over
		// taking results from the last header stanza we receive.
		op->mReplyOffset = 0;
		op->mReplyLength = 0;
		op->mReplyFullLength = 0;
		op->mStatus = HttpStatus();
		if (op->mReplyHeaders)
		{
			op->mReplyHeaders->mHeaders.clear();
		}
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
	
	// Save header if caller wants them in the response
	if (op->mProcFlags & PF_SAVE_HEADERS)
	{
		// Save headers in response
		if (! op->mReplyHeaders)
		{
			op->mReplyHeaders = new HttpHeaders;
		}
		op->mReplyHeaders->mHeaders.push_back(std::string(hdr_data, wanted_hdr_size));
	}

	// Detect and parse 'Content-Range' headers
	if (op->mProcFlags & PF_SCAN_RANGE_HEADER)
	{
		char hdr_buffer[128];			// Enough for a reasonable header
		size_t frag_size((std::min)(wanted_hdr_size, sizeof(hdr_buffer) - 1));
		
		memcpy(hdr_buffer, hdr_data, frag_size);
		hdr_buffer[frag_size] = '\0';
		if (frag_size > con_ran_line_len &&
			! os_strncasecmp(hdr_buffer, con_ran_line, con_ran_line_len))
		{
			unsigned int first(0), last(0), length(0);
			int status;

			if (! (status = parse_content_range_header(hdr_buffer, &first, &last, &length)))
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
				LL_INFOS_ONCE("CoreHttp") << "Problem parsing odd Content-Range header:  '"
										  << std::string(hdr_data, frag_size)
										  << "'.  Ignoring."
										  << LL_ENDL;
			}
		}
	}

	return hdr_size;
}


int HttpOpRequest::debugCallback(CURL * handle, curl_infotype info, char * buffer, size_t len, void * userdata)
{
	HttpOpRequest * op(static_cast<HttpOpRequest *>(userdata));

	std::string safe_line;
	std::string tag;
	bool logit(false);
	len = (std::min)(len, size_t(256));					// Keep things reasonable in all cases
	
	switch (info)
	{
	case CURLINFO_TEXT:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "TEXT";
			escape_libcurl_debug_data(buffer, len, true, safe_line);
			logit = true;
		}
		break;
			
	case CURLINFO_HEADER_IN:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "HEADERIN";
			escape_libcurl_debug_data(buffer, len, true, safe_line);
			logit = true;
		}
		break;
			
	case CURLINFO_HEADER_OUT:
		if (op->mTracing >= HTTP_TRACE_CURL_HEADERS)
		{
			tag = "HEADEROUT";
			escape_libcurl_debug_data(buffer, 2 * len, true, safe_line);		// Goes out as one line
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
				escape_libcurl_debug_data(buffer, len, false, safe_line);
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
				escape_libcurl_debug_data(buffer, len, false, safe_line);
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
		LL_INFOS("CoreHttp") << "TRACE, LibcurlDebug, Handle:  "
							 << static_cast<HttpHandle>(op)
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
	char * tok_state(NULL), * tok(NULL);
	bool match(true);
			
	if (! os_strtok_r(buffer, hdr_separator, &tok_state))
		match = false;
	if (match && (tok = os_strtok_r(NULL, hdr_whitespace, &tok_state)))
		match = 0 == os_strcasecmp("bytes", tok);
	if (match && ! (tok = os_strtok_r(NULL, " \t", &tok_state)))
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


int os_strncasecmp(const char *s1, const char *s2, size_t n)
{
#if LL_WINDOWS
	return _strnicmp(s1, s2, n);
#else
	return strncasecmp(s1, s2, n);
#endif	// LL_WINDOWS
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


}  // end anonymous namespace

		
