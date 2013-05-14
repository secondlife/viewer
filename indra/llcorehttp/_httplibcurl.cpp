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
	: mService(service),
	  mPolicyCount(0),
	  mMultiHandles(NULL)
{}


HttpLibcurl::~HttpLibcurl()
{
	shutdown();
	
	mService = NULL;
}


void HttpLibcurl::shutdown()
{
	while (! mActiveOps.empty())
	{
		HttpOpRequest * op(* mActiveOps.begin());
		mActiveOps.erase(mActiveOps.begin());

		cancelRequest(op);
		op->release();
	}

	if (mMultiHandles)
	{
		for (int policy_class(0); policy_class < mPolicyCount; ++policy_class)
		{
			if (mMultiHandles[policy_class])
			{
				curl_multi_cleanup(mMultiHandles[policy_class]);
				mMultiHandles[policy_class] = 0;
			}
		}

		delete [] mMultiHandles;
		mMultiHandles = NULL;
	}

	mPolicyCount = 0;
}


void HttpLibcurl::start(int policy_count)
{
	llassert_always(policy_count <= HTTP_POLICY_CLASS_LIMIT);
	llassert_always(! mMultiHandles);					// One-time call only
	
	mPolicyCount = policy_count;
	mMultiHandles = new CURLM * [mPolicyCount];
	for (int policy_class(0); policy_class < mPolicyCount; ++policy_class)
	{
		mMultiHandles[policy_class] = curl_multi_init();
	}
}


// Give libcurl some cycles, invoke it's callbacks, process
// completed requests finalizing or issuing retries as needed.
//
// If active list goes empty *and* we didn't queue any
// requests for retry, we return a request for a hard
// sleep otherwise ask for a normal polling interval.
HttpService::ELoopSpeed HttpLibcurl::processTransport()
{
	HttpService::ELoopSpeed	ret(HttpService::REQUEST_SLEEP);

	// Give libcurl some cycles to do I/O & callbacks
	for (int policy_class(0); policy_class < mPolicyCount; ++policy_class)
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
					ret = HttpService::NORMAL;
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
				LL_WARNS_ONCE("CoreHttp") << "Unexpected message from libcurl.  Msg code:  "
										  << msg->msg
										  << LL_ENDL;
			}
			msgs_in_queue = 0;
		}
	}

	if (! mActiveOps.empty())
	{
		ret = HttpService::NORMAL;
	}
	return ret;
}


// Caller has provided us with a ref count on op.
void HttpLibcurl::addOp(HttpOpRequest * op)
{
	llassert_always(op->mReqPolicy < mPolicyCount);
	llassert_always(mMultiHandles[op->mReqPolicy] != NULL);
	
	// Create standard handle
	if (! op->prepareRequest(mService))
	{
		// Couldn't issue request, fail with notification
		// *TODO:  Need failure path
		return;
	}

	// Make the request live
	curl_multi_add_handle(mMultiHandles[op->mReqPolicy], op->mCurlHandle);
	op->mCurlActive = true;
	
	if (op->mTracing > HTTP_TRACE_OFF)
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


// Implements the transport part of any cancel operation.
// See if the handle is an active operation and if so,
// use the more complicated transport-based cancelation
// method to kill the request.
bool HttpLibcurl::cancel(HttpHandle handle)
{
	HttpOpRequest * op(static_cast<HttpOpRequest *>(handle));
	active_set_t::iterator it(mActiveOps.find(op));
	if (mActiveOps.end() == it)
	{
		return false;
	}

	// Cancel request
	cancelRequest(op);

	// Drop references
	mActiveOps.erase(it);
	op->release();

	return true;
}


// *NOTE:  cancelRequest logic parallels completeRequest logic.
// Keep them synchronized as necessary.  Caller is expected to
// remove the op from the active list and release the op *after*
// calling this method.  It must be called first to deliver the
// op to the reply queue with refcount intact.
void HttpLibcurl::cancelRequest(HttpOpRequest * op)
{
	// Deactivate request
	op->mCurlActive = false;

	// Detach from multi and recycle handle
	curl_multi_remove_handle(mMultiHandles[op->mReqPolicy], op->mCurlHandle);
	curl_easy_cleanup(op->mCurlHandle);
	op->mCurlHandle = NULL;

	// Tracing
	if (op->mTracing > HTTP_TRACE_OFF)
	{
		LL_INFOS("CoreHttp") << "TRACE, RequestCanceled, Handle:  "
							 << static_cast<HttpHandle>(op)
							 << ", Status:  " << op->mStatus.toHex()
							 << LL_ENDL;
	}

	// Cancel op and deliver for notification
	op->cancel();
}


// *NOTE:  cancelRequest logic parallels completeRequest logic.
// Keep them synchronized as necessary.
bool HttpLibcurl::completeRequest(CURLM * multi_handle, CURL * handle, CURLcode status)
{
	HttpOpRequest * op(NULL);
	curl_easy_getinfo(handle, CURLINFO_PRIVATE, &op);

	if (handle != op->mCurlHandle || ! op->mCurlActive)
	{
		LL_WARNS("CoreHttp") << "libcurl handle and HttpOpRequest handle in disagreement or inactive request."
							 << "  Handle:  " << static_cast<HttpHandle>(handle)
							 << LL_ENDL;
		return false;
	}

	active_set_t::iterator it(mActiveOps.find(op));
	if (mActiveOps.end() == it)
	{
		LL_WARNS("CoreHttp") << "libcurl completion for request not on active list.  Continuing."
							 << "  Handle:  " << static_cast<HttpHandle>(handle)
							 << LL_ENDL;
		return false;
	}

	// Deactivate request
	mActiveOps.erase(it);
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
		if (http_status >= 100 && http_status <= 999)
		{
			char * cont_type(NULL);
			curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &cont_type);
			if (cont_type)
			{
				op->mReplyConType = cont_type;
			}
			op->mStatus = HttpStatus(http_status);
		}
		else
		{
			LL_WARNS("CoreHttp") << "Invalid HTTP response code ("
								 << http_status << ") received from server."
								 << LL_ENDL;
			op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_INVALID_HTTP_STATUS);
		}
	}

	// Detach from multi and recycle handle
	curl_multi_remove_handle(multi_handle, handle);
	curl_easy_cleanup(handle);
	op->mCurlHandle = NULL;

	// Tracing
	if (op->mTracing > HTTP_TRACE_OFF)
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


int HttpLibcurl::getActiveCountInClass(int policy_class) const
{
	int count(0);
	
	for (active_set_t::const_iterator iter(mActiveOps.begin());
		 mActiveOps.end() != iter;
		 ++iter)
	{
		if ((*iter)->mReqPolicy == policy_class)
		{
			++count;
		}
	}
	
	return count;
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
