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
#include "bufferarray.h"
#include "_httpoprequest.h"
#include "_httppolicy.h"

#include "llhttpstatuscodes.h"


namespace LLCore
{


HttpLibcurl::HttpLibcurl(HttpService * service)
	: mService(service)
{
	// *FIXME:  Use active policy class count later
	for (int policy_class(0); policy_class < LL_ARRAY_SIZE(mMultiHandles); ++policy_class)
	{
		mMultiHandles[policy_class] = 0;
	}

	// Create multi handle for default class
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
		mActiveOps.erase(item);
	}

	for (int policy_class(0); policy_class < LL_ARRAY_SIZE(mMultiHandles); ++policy_class)
	{
		if (mMultiHandles[policy_class])
		{
			// *FIXME:  Do some multi cleanup here first
		
			curl_multi_cleanup(mMultiHandles[policy_class]);
			mMultiHandles[policy_class] = 0;
		}
	}

	mService = NULL;
}
	

void HttpLibcurl::init()
{}


void HttpLibcurl::term()
{}


HttpService::ELoopSpeed HttpLibcurl::processTransport()
{
	HttpService::ELoopSpeed	ret(HttpService::REQUEST_SLEEP);

	// Give libcurl some cycles to do I/O & callbacks
	for (int policy_class(0); policy_class < LL_ARRAY_SIZE(mMultiHandles); ++policy_class)
	{
		if (! mMultiHandles[policy_class])
			continue;
		
		int running(0);
		CURLMcode status(CURLM_CALL_MULTI_PERFORM);
		do
		{
			running = 0;
			status = curl_multi_perform(mMultiHandles[policy_class], &running);
		}
		while (0 != running && CURLM_CALL_MULTI_PERFORM == status);

		// Run completion on anything done
		CURLMsg * msg(NULL);
		int msgs_in_queue(0);
		while ((msg = curl_multi_info_read(mMultiHandles[policy_class], &msgs_in_queue)))
		{
			if (CURLMSG_DONE == msg->msg)
			{
				CURL * handle(msg->easy_handle);
				CURLcode result(msg->data.result);

				if (completeRequest(mMultiHandles[policy_class], handle, result))
				{
					// Request is still active, don't get too sleepy
					ret = (std::min)(ret, HttpService::NORMAL);
				}
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

	if (! mActiveOps.empty())
	{
		ret = (std::min)(ret, HttpService::NORMAL);
	}
	return ret;
}


void HttpLibcurl::addOp(HttpOpRequest * op)
{
	llassert_always(op->mReqPolicy < POLICY_CLASS_LIMIT);
	llassert_always(mMultiHandles[op->mReqPolicy] != NULL);
	
	// Create standard handle
	if (! op->prepareRequest(mService))
	{
		// Couldn't issue request, fail with notification
		// *FIXME:  Need failure path
		return;
	}

	// Make the request live
	curl_multi_add_handle(mMultiHandles[op->mReqPolicy], op->mCurlHandle);
	op->mCurlActive = true;
	
	if (op->mTracing > TRACE_OFF)
	{
		HttpPolicy & policy(mService->getPolicy());
		
		LL_INFOS("CoreHttp") << "TRACE, ToActiveQueue, Handle:  "
							 << static_cast<HttpHandle>(op)
							 << ", Actives:  " << mActiveOps.size()
							 << ", Readies:  " << policy.getReadyCount(op->mReqPolicy)
							 << LL_ENDL;
	}
	
	// On success, make operation active
	mActiveOps.insert(op);
}


bool HttpLibcurl::completeRequest(CURLM * multi_handle, CURL * handle, CURLcode status)
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

	// Set final status of request if it hasn't failed by other mechanisms yet
	if (op->mStatus)
	{
		op->mStatus = HttpStatus(HttpStatus::EXT_CURL_EASY, status);
	}
	if (op->mStatus)
	{
		int http_status(HTTP_OK);

		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_status);
		op->mStatus = LLCore::HttpStatus(http_status);
	}

	// Detach from multi and recycle handle
	curl_multi_remove_handle(multi_handle, handle);
	curl_easy_cleanup(handle);
	op->mCurlHandle = NULL;

	// Tracing
	if (op->mTracing > TRACE_OFF)
	{
		LL_INFOS("CoreHttp") << "TRACE, RequestComplete, Handle:  "
							 << static_cast<HttpHandle>(op)
							 << ", Status:  " << op->mStatus.toHex()
							 << LL_ENDL;
	}

	// Dispatch to next stage
	HttpPolicy & policy(mService->getPolicy());
	bool still_active(policy.stageAfterCompletion(op));

	return still_active;
}


int HttpLibcurl::getActiveCount() const
{
	return mActiveOps.size();
}


int HttpLibcurl::getActiveCountInClass(int /* policy_class */) const
{
	return getActiveCount();
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
