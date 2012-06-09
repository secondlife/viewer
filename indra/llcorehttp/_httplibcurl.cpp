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


namespace LLCore
{


HttpLibcurl::HttpLibcurl(HttpService * service)
	: mService(service)
{
	for (int policy_class(0); policy_class < HttpRequest::POLICY_CLASS_LIMIT; ++policy_class)
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

	for (int policy_class(0); policy_class < HttpRequest::POLICY_CLASS_LIMIT; ++policy_class)
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
	for (int policy_class(0); policy_class < HttpRequest::POLICY_CLASS_LIMIT; ++policy_class)
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

				HttpService::ELoopSpeed	speed(completeRequest(mMultiHandles[policy_class], handle, result));
				ret = (std::min)(ret, speed);
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
	llassert_always(op->mReqPolicy < HttpRequest::POLICY_CLASS_LIMIT);
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
	
	// On success, make operation active
	mActiveOps.insert(op);
}


HttpService::ELoopSpeed HttpLibcurl::completeRequest(CURLM * multi_handle, CURL * handle, CURLcode status)
{
	static const HttpStatus cant_connect(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
	static const HttpStatus cant_res_proxy(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_PROXY);
	static const HttpStatus cant_res_host(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_HOST);

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
		op->mStatus = LLCore::HttpStatus(http_status);
	}

	// Detach from multi and recycle handle
	curl_multi_remove_handle(multi_handle, handle);
	curl_easy_cleanup(handle);
	op->mCurlHandle = NULL;
	
	// Retry or finalize
	if (! op->mStatus)
	{
		// If this failed, we might want to retry.  Have to inspect
		// the status a little more deeply for those reasons worth retrying...
		if (op->mPolicyRetries < op->mPolicyRetryLimit &&
			((op->mStatus.isHttpStatus() && op->mStatus.mType >= 499 && op->mStatus.mType <= 599) ||
			 cant_connect == op->mStatus ||
			 cant_res_proxy == op->mStatus ||
			 cant_res_host == op->mStatus))
		{
			// Okay, worth a retry.  We include 499 in this test as
			// it's the old 'who knows?' error from many grid services...
			HttpPolicy & policy(mService->getPolicy());
		
			policy.retryOp(op);
			return HttpService::NORMAL;			// Having pushed to retry, keep things running
		}
	}

	// This op is done, finalize it delivering it to the reply queue...
	if (! op->mStatus)
	{
		LL_WARNS("CoreHttp") << "URL op failed after " << op->mPolicyRetries
							 << " retries.  Reason:  " << op->mStatus.toString()
							 << LL_ENDL;
	}
	else if (op->mPolicyRetries)
	{
		LL_WARNS("CoreHttp") << "URL op succeeded after " << op->mPolicyRetries << " retries."
							 << LL_ENDL;
	}
	
	op->stageFromActive(mService);
	op->release();
	return HttpService::REQUEST_SLEEP;
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
