/**
 * @file _httplibcurl.cpp
 * @brief Internal definitions of the Http libcurl thread
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

#include "_httplibcurl.h"

#include "httpheaders.h"
#include "_httpoprequest.h"
#include "_httpservice.h"


namespace LLCore
{


HttpLibcurl::HttpLibcurl(HttpService * service)
	: mService(service)
{
	mMultiHandles[0] = curl_multi_init();
}


HttpLibcurl::~HttpLibcurl()
{
	// *FIXME:  need to cancel requests in this class, not in op class.
	while (! mActiveOps.empty())
	{
		active_set_t::iterator item(mActiveOps.begin());

		(*item)->cancel();
		(*item)->release();
	}

	if (mMultiHandles[0])
	{
		// *FIXME:  Do some multi cleanup here first

		
		curl_multi_cleanup(mMultiHandles[0]);
		mMultiHandles[0] = NULL;
	}

	mService = NULL;
}
	

void HttpLibcurl::init()
{}


void HttpLibcurl::term()
{}


void HttpLibcurl::processTransport()
{
	if (mMultiHandles[0])
	{
		// Give libcurl some cycles to do I/O & callbacks
		int running(0);
		CURLMcode status(CURLM_CALL_MULTI_PERFORM);
		do
		{
			running = 0;
			status = curl_multi_perform(mMultiHandles[0], &running);
		}
		while (0 != running && CURLM_CALL_MULTI_PERFORM == status);

		// Run completion on anything done
		CURLMsg * msg(NULL);
		int msgs_in_queue(0);
		while ((msg = curl_multi_info_read(mMultiHandles[0], &msgs_in_queue)))
		{
			if (CURLMSG_DONE == msg->msg)
			{
				CURL * handle(msg->easy_handle);
				CURLcode result(msg->data.result);

				completeRequest(mMultiHandles[0], handle, result);
				handle = NULL;			// No longer valid on return
			}
			else if (CURLMSG_NONE == msg->msg)
			{
				// Ignore this... it shouldn't mean anything.
				;
			}
			else
			{
				// *FIXME:  Issue a logging event for this.
				;
			}
			msgs_in_queue = 0;
		}
	}
}


void HttpLibcurl::addOp(HttpOpRequest * op)
{
	// Create standard handle
	if (! op->prepareForGet(mService))
	{
		// Couldn't issue request, fail with notification
		// *FIXME:  Need failure path
		return;
	}

	// Make the request live
	curl_multi_add_handle(mMultiHandles[0], op->mCurlHandle);
	op->mCurlActive = true;
	
	// On success, make operation active
	mActiveOps.insert(op);
}


void HttpLibcurl::completeRequest(CURLM * multi_handle, CURL * handle, CURLcode status)
{
	HttpOpRequest * op(NULL);
	curl_easy_getinfo(handle, CURLINFO_PRIVATE, &op);
	// *FIXME:  check the pointer

	if (handle != op->mCurlHandle || ! op->mCurlActive)
	{
		// *FIXME:  This is a sanity check that needs validation/termination.
		;
	}

	active_set_t::iterator it(mActiveOps.find(op));
	if (mActiveOps.end() == it)
	{
		// *FIXME:  Fatal condition.  This must be here.
		;
	}
	else
	{
		mActiveOps.erase(it);
	}

	// Deactivate request
	op->mCurlActive = false;

	// Set final status of request
	if (op->mStatus)
	{
		// Only set if it hasn't failed by other mechanisms yet
		op->mStatus = HttpStatus(HttpStatus::EXT_CURL_EASY, status);
	}
	if (op->mStatus)
	{
		int http_status(200);

		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_status);
		op->mStatus = LLCore::HttpStatus(http_status,
										 (http_status >= 200 && http_status <= 299
										  ? HE_SUCCESS
										  : HE_REPLY_ERROR));
	}

	// Detach from multi and recycle handle
	curl_multi_remove_handle(multi_handle, handle);
	curl_easy_cleanup(handle);
	op->mCurlHandle = NULL;
	
	// Deliver to reply queue and release
	op->stageFromActive(mService);
	op->release();
}


int HttpLibcurl::activeCount() const
{
	return mActiveOps.size();
}


// ---------------------------------------
// Free functions
// ---------------------------------------


struct curl_slist * append_headers_to_slist(const HttpHeaders * headers, struct curl_slist * slist)
{
	for (HttpHeaders::container_t::const_iterator it(headers->mHeaders.begin());

		headers->mHeaders.end() != it;
		 ++it)
	{
		slist = curl_slist_append(slist, (*it).c_str());
	}
	return slist;
}


}  // end namespace LLCore
