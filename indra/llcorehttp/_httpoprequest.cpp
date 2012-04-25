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
#include "_httplibcurl.h"


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

#if defined(WIN32)

// Not available on windows where the legacy strtok interface
// is thread-safe.
char *strtok_r(char *str, const char *delim, char **saveptr);

#endif

}


namespace LLCore
{


// ==================================
// HttpOpRequest
// ==================================


HttpOpRequest::HttpOpRequest()
	: HttpOperation(),
	  mProcFlags(0U),
	  mReqMethod(HOR_GET),
	  mReqBody(NULL),
	  mReqOffset(0),
	  mReqLen(0),
	  mReqHeaders(NULL),
	  mReqOptions(NULL),
	  mCurlActive(false),
	  mCurlHandle(NULL),
	  mCurlHeaders(NULL),
	  mCurlService(NULL),
	  mReplyStatus(200),
	  mReplyBody(NULL),
	  mReplyOffset(0),
	  mReplyLen(0),
	  mReplyHeaders(NULL)
{}


HttpOpRequest::~HttpOpRequest()
{
	if (mCurlHandle)
	{
		curl_easy_cleanup(mCurlHandle);
		mCurlHandle = NULL;
	}

	if (mCurlHeaders)
	{
		curl_slist_free_all(mCurlHeaders);
		mCurlHeaders = NULL;
	}

	mCurlService = NULL;
	
	if (mReqBody)
	{
		mReqBody->release();
		mReqBody = NULL;
	}
	
	if (mReqHeaders)
	{
		curl_slist_free_all(mReqHeaders);
		mReqHeaders = NULL;
	}

	if (mReqOptions)
	{
		mReqOptions->release();
		mReqOptions = NULL;
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
	service->getPolicy()->addOp(this);			// transfers refcount
}


void HttpOpRequest::stageFromReady(HttpService * service)
{
	addRef();
	service->getTransport()->addOp(this);		// transfers refcount
}


void HttpOpRequest::stageFromActive(HttpService * service)
{
	if (mReplyLen)
	{
		// If non-zero, we received and processed a Content-Range
		// header with the response.  Verify that what it says
		// is consistent with the received data.
		if (mReplyLen != mReplyBody->size())
		{
			// Not as expected, fail the request
			mStatus = HttpStatus(HttpStatus::LLCORE, HE_INV_CONTENT_RANGE_HDR);
		}
	}
	
	if (mReqHeaders)
	{
		// We take these headers out of the request now as they were
		// allocated originally in this thread and the notifier doesn't
		// need them.  This eliminates one source of heap moving across
		// threads.

		curl_slist_free_all(mReqHeaders);
		mReqHeaders = NULL;
	}
	addAsReply();
}


void HttpOpRequest::visitNotifier(HttpRequest * request)
{
	if (mLibraryHandler)
	{
		HttpResponse * response = new HttpResponse();

		// *FIXME:  add http status, offset, length
		response->setStatus(mStatus);
		response->setReplyStatus(mReplyStatus);
		response->setBody(mReplyBody);
		response->setHeaders(mReplyHeaders);
		mLibraryHandler->onCompleted(static_cast<HttpHandle>(this), response);

		response->release();
	}
}


HttpStatus HttpOpRequest::cancel()
{
	mStatus = HttpStatus(HttpStatus::LLCORE, HE_OP_CANCELED);

	addAsReply();

	return HttpStatus();
}


HttpStatus HttpOpRequest::setupGetByteRange(unsigned int policy_id,
											float priority,
											const std::string & url,
											size_t offset,
											size_t len,
											HttpOptions * options,
											HttpHeaders * headers)
{
	HttpStatus status;

	mProcFlags = 0;
	mReqPolicy = policy_id;
	mReqPriority = priority;
	mReqMethod = HOR_GET;
	mReqURL = url;
	mReqOffset = offset;
	mReqLen = len;
	if (offset || len)
	{
		mProcFlags |= PF_SCAN_RANGE_HEADER;
	}
	if (headers && ! mReqHeaders)
	{
		mReqHeaders = append_headers_to_slist(headers, mReqHeaders);
	}
	if (options && ! mReqOptions)
	{
		mReqOptions = new HttpOptions(*options);
	}
	
	return status;
}


HttpStatus HttpOpRequest::prepareForGet(HttpService * service)
{
	// *FIXME:  better error handling later
	HttpStatus status;
	
	mCurlHandle = curl_easy_init();
	curl_easy_setopt(mCurlHandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_URL, mReqURL.c_str());
	curl_easy_setopt(mCurlHandle, CURLOPT_PRIVATE, this);
	curl_easy_setopt(mCurlHandle, CURLOPT_ENCODING, "");
	// curl_easy_setopt(handle, CURLOPT_PROXY, "");

	mCurlHeaders = curl_slist_append(mCurlHeaders, "Pragma:");
	curl_easy_setopt(mCurlHandle, CURLOPT_DNS_CACHE_TIMEOUT, 0);
	curl_easy_setopt(mCurlHandle, CURLOPT_HTTPHEADER, mCurlHeaders);
	curl_easy_setopt(mCurlHandle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(mCurlHandle, CURLOPT_WRITEDATA, mCurlHandle);

	if (mReqOffset || mReqLen)
	{
		static const char * fmt1("Range: bytes=%d-%d");
		static const char * fmt2("Range: bytes=%d-");

		char range_line[64];

#if defined(WIN32)
		_snprintf_s(range_line, sizeof(range_line), sizeof(range_line) - 1,
					(mReqLen ? fmt1 : fmt2),
					mReqOffset, mReqOffset + mReqLen - 1);
#else
		snprintf(range_line, sizeof(range_line),
				 (mReqLen ? fmt1 : fmt2),
				 mReqOffset, mReqOffset + mReqLen - 1);
#endif // defined(WIN32)
		range_line[sizeof(range_line) - 1] = '\0';
		mCurlHeaders = curl_slist_append(mCurlHeaders, range_line);
	}
	
	if (mProcFlags & (PF_SCAN_RANGE_HEADER | PF_SAVE_HEADERS))
	{
		curl_easy_setopt(mCurlHandle, CURLOPT_HEADERFUNCTION, headerCallback);
		curl_easy_setopt(mCurlHandle, CURLOPT_HEADERDATA, mCurlHandle);
	}
	
	if (status)
	{
		mCurlService = service;
	}
	return status;
}


size_t HttpOpRequest::writeCallback(void * data, size_t size, size_t nmemb, void * userdata)
{
	CURL * handle(static_cast<CURL *>(userdata));
	HttpOpRequest * op(NULL);
	curl_easy_getinfo(handle, CURLINFO_PRIVATE, &op);
	// *FIXME:  check the pointer

	if (! op->mReplyBody)
	{
		op->mReplyBody = new BufferArray();
	}
	const size_t req_size(size * nmemb);
	char * lump(op->mReplyBody->appendBufferAlloc(req_size));
	memcpy(lump, data, req_size);

	return req_size;
}

		
size_t HttpOpRequest::headerCallback(void * data, size_t size, size_t nmemb, void * userdata)
{
	static const char status_line[] = "HTTP/";
	static const size_t status_line_len = sizeof(status_line) - 1;

	static const char con_ran_line[] = "content-range:";
	static const size_t con_ran_line_len = sizeof(con_ran_line) - 1;
	
	CURL * handle(static_cast<CURL *>(userdata));
	HttpOpRequest * op(NULL);
	curl_easy_getinfo(handle, CURLINFO_PRIVATE, &op);
	// *FIXME:  check the pointer

	const size_t hdr_size(size * nmemb);
	const char * hdr_data(static_cast<const char *>(data));		// Not null terminated
	
	if (hdr_size >= status_line_len && ! strncmp(status_line, hdr_data, status_line_len))
	{
		// One of possibly several status lines.  Reset what we know and start over
		op->mReplyOffset = 0;
		op->mReplyLen = 0;
		op->mStatus = HttpStatus();
	}
	else if (op->mProcFlags & PF_SCAN_RANGE_HEADER)
	{
		char hdr_buffer[128];
		size_t frag_size((std::min)(hdr_size, sizeof(hdr_buffer) - 1));
		
		memcpy(hdr_buffer, hdr_data, frag_size);
		hdr_buffer[frag_size] = '\0';
#if defined(WIN32)
		if (! _strnicmp(hdr_buffer, con_ran_line, (std::min)(frag_size, con_ran_line_len)))
#else
		if (! strncasecmp(hdr_buffer, con_ran_line, (std::min)(frag_size, con_ran_line_len)))
#endif
		{
			unsigned int first(0), last(0), length(0);
			int status;

			if (! (status = parse_content_range_header(hdr_buffer, &first, &last, &length)))
			{
				// Success, record the fragment position
				op->mReplyOffset = first;
				op->mReplyLen = last - first + 1;
			}
			else if (-1 == status)
			{
				// Response is badly formed and shouldn't be accepted
				op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_INV_CONTENT_RANGE_HDR);
			}
			else
			{
				// Ignore the unparsable.
				// *FIXME:  Maybe issue a warning into the log here
				;
			}
		}
	}

	if (op->mProcFlags & PF_SAVE_HEADERS)
	{
		// Save headers in response
		;
		
	}

	return hdr_size;
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
			
	if (! strtok_r(buffer, ": \t", &tok_state))
		match = false;
	if (match && (tok = strtok_r(NULL, " \t", &tok_state)))
#if defined(WIN32)
		match = 0 == _stricmp("bytes", tok);
#else
		match = 0 == strcasecmp("bytes", tok);
#endif
	if (match && ! (tok = strtok_r(NULL, " \t", &tok_state)))
		match = false;
	if (match)
	{
		unsigned int lcl_first(0), lcl_last(0), lcl_len(0);

#if defined(WIN32)
		if (3 == sscanf_s(tok, "%u-%u/%u", &lcl_first, &lcl_last, &lcl_len))
#else
		if (3 == sscanf(tok, "%u-%u/%u", &lcl_first, &lcl_last, &lcl_len))
#endif
		{
			if (lcl_first > lcl_last || lcl_last >= lcl_len)
				return -1;
			*first = lcl_first;
			*last = lcl_last;
			*length = lcl_len;
			return 0;
		}
#if defined(WIN32)
		if (2 == sscanf_s(tok, "%u-%u/*", &lcl_first, &lcl_last))
#else
		if (2 == sscanf(tok, "%u-%u/*", &lcl_first, &lcl_last))
#endif
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

#if defined(WIN32)

char *strtok_r(char *str, const char *delim, char ** savestate)
{
	return strtok_s(str, delim, savestate);
}

#endif

}  // end anonymous namespace

		
