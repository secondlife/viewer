/**
 * @file _httplibcurl.cpp
 * @brief Internal definitions of the Http libcurl thread
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

#include "_httplibcurl.h"

#include "httpheaders.h"
#include "bufferarray.h"
#include "_httpoprequest.h"
#include "_httppolicy.h"

#include "llhttpconstants.h"

namespace
{

// Error testing and reporting for libcurl status codes
void check_curl_multi_code(CURLMcode code);
void check_curl_multi_code(CURLMcode code, int curl_setopt_option);

// This is a template because different 'option' values require different
// types for 'ARG'. Just pass them through unchanged (by value).
template <typename ARG>
void check_curl_multi_setopt(CURLM* handle, CURLMoption option, ARG argument)
{
    CURLMcode code = curl_multi_setopt(handle, option, argument);
    check_curl_multi_code(code, option);
}

static const char * const LOG_CORE("CoreHttp");

} // end anonymous namespace


namespace LLCore
{


HttpLibcurl::HttpLibcurl(HttpService * service)
	: mService(service),
	  mHandleCache(),
	  mPolicyCount(0),
	  mMultiHandles(NULL),
	  mActiveHandles(NULL),
	  mDirtyPolicy(NULL)
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
		HttpOpRequest::ptr_t op(* mActiveOps.begin());
		mActiveOps.erase(mActiveOps.begin());

		cancelRequest(op);
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

		delete [] mActiveHandles;
		mActiveHandles = NULL;

		delete [] mDirtyPolicy;
		mDirtyPolicy = NULL;
	}

	mPolicyCount = 0;
}


void HttpLibcurl::start(int policy_count)
{
	llassert_always(policy_count <= HTTP_POLICY_CLASS_LIMIT);
	llassert_always(! mMultiHandles);					// One-time call only
	
	mPolicyCount = policy_count;
	mMultiHandles = new CURLM * [mPolicyCount];
	mActiveHandles = new int [mPolicyCount];
	mDirtyPolicy = new bool [mPolicyCount];
	
	for (int policy_class(0); policy_class < mPolicyCount; ++policy_class)
	{
		if (NULL == (mMultiHandles[policy_class] = curl_multi_init()))
		{
			LL_ERRS(LOG_CORE) << "Failed to allocate multi handle in libcurl."
							  << LL_ENDL;
		}
		mActiveHandles[policy_class] = 0;
		mDirtyPolicy[policy_class] = false;
		policyUpdated(policy_class);
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
		{
			// No handle, nothing to do.
			continue;
		}
		if (! mActiveHandles[policy_class])
		{
			// If we've gone quiet and there's a dirty update, apply it,
			// otherwise we're done.
			if (mDirtyPolicy[policy_class])
			{
				policyUpdated(policy_class);
			}
			continue;
		}
		
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

				completeRequest(mMultiHandles[policy_class], handle, result);
				handle = NULL;					// No longer valid on return
				ret = HttpService::NORMAL;		// If anything completes, we may have a free slot.
												// Turning around quickly reduces connection gap by 7-10mS.
			}
			else if (CURLMSG_NONE == msg->msg)
			{
				// Ignore this... it shouldn't mean anything.
				;
			}
			else
			{
				LL_WARNS_ONCE(LOG_CORE) << "Unexpected message from libcurl.  Msg code:  "
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
void HttpLibcurl::addOp(const HttpOpRequest::ptr_t &op)
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
	CURLMcode code;
	code = curl_multi_add_handle(mMultiHandles[op->mReqPolicy], op->mCurlHandle);
	if (CURLM_OK != code)
	{
		// *TODO:  Better cleanup and recovery but not much we can do here.
		check_curl_multi_code(code);
		return;
	}
	op->mCurlActive = true;
	mActiveOps.insert(op);
	++mActiveHandles[op->mReqPolicy];
	
	if (op->mTracing > HTTP_TRACE_OFF)
	{
		HttpPolicy & policy(mService->getPolicy());
		
		LL_INFOS(LOG_CORE) << "TRACE, ToActiveQueue, Handle:  "
                            << op->getHandle()
						    << ", Actives:  " << mActiveOps.size()
						    << ", Readies:  " << policy.getReadyCount(op->mReqPolicy)
						    << LL_ENDL;
	}
}


// Implements the transport part of any cancel operation.
// See if the handle is an active operation and if so,
// use the more complicated transport-based cancellation
// method to kill the request.
bool HttpLibcurl::cancel(HttpHandle handle)
{
    HttpOpRequest::ptr_t op = HttpOpRequest::fromHandle<HttpOpRequest>(handle);
	active_set_t::iterator it(mActiveOps.find(op));
	if (mActiveOps.end() == it)
	{
		return false;
	}

	// Cancel request
	cancelRequest(op);

	// Drop references
	mActiveOps.erase(it);
	--mActiveHandles[op->mReqPolicy];

	return true;
}


// *NOTE:  cancelRequest logic parallels completeRequest logic.
// Keep them synchronized as necessary.  Caller is expected to
// remove the op from the active list and release the op *after*
// calling this method.  It must be called first to deliver the
// op to the reply queue with refcount intact.
void HttpLibcurl::cancelRequest(const HttpOpRequest::ptr_t &op)
{
	// Deactivate request
	op->mCurlActive = false;

	// Detach from multi and recycle handle
	curl_multi_remove_handle(mMultiHandles[op->mReqPolicy], op->mCurlHandle);
	mHandleCache.freeHandle(op->mCurlHandle);
	op->mCurlHandle = NULL;

	// Tracing
	if (op->mTracing > HTTP_TRACE_OFF)
	{
		LL_INFOS(LOG_CORE) << "TRACE, RequestCanceled, Handle:  "
						   << op->getHandle()
						   << ", Status:  " << op->mStatus.toTerseString()
						   << LL_ENDL;
	}

	// Cancel op and deliver for notification
	op->cancel();
}


// *NOTE:  cancelRequest logic parallels completeRequest logic.
// Keep them synchronized as necessary.
bool HttpLibcurl::completeRequest(CURLM * multi_handle, CURL * handle, CURLcode status)
{
    HttpHandle ophandle(NULL);

    CURLcode ccode(CURLE_OK);

    ccode =	curl_easy_getinfo(handle, CURLINFO_PRIVATE, &ophandle);
    if (ccode)
    {
        LL_WARNS(LOG_CORE) << "libcurl error: " << ccode << " Unable to retrieve operation handle from CURL handle" << LL_ENDL;
        return false;
    }
    HttpOpRequest::ptr_t op(HttpOpRequest::fromHandle<HttpOpRequest>(ophandle));
    
    if (!op)
    {
        LL_WARNS() << "Unable to locate operation by handle. May have expired!" << LL_ENDL;
        return false;
    }

	if (handle != op->mCurlHandle || ! op->mCurlActive)
	{
		LL_WARNS(LOG_CORE) << "libcurl handle and HttpOpRequest handle in disagreement or inactive request."
						   << "  Handle:  " << static_cast<HttpHandle>(handle)
						   << LL_ENDL;
		return false;
	}

	active_set_t::iterator it(mActiveOps.find(op));
	if (mActiveOps.end() == it)
	{
		LL_WARNS(LOG_CORE) << "libcurl completion for request not on active list.  Continuing."
						   << "  Handle:  " << static_cast<HttpHandle>(handle)
						   << LL_ENDL;
		return false;
	}

	// Deactivate request
	mActiveOps.erase(it);
	--mActiveHandles[op->mReqPolicy];
	op->mCurlActive = false;

	// Set final status of request if it hasn't failed by other mechanisms yet
	if (op->mStatus)
	{
		op->mStatus = HttpStatus(HttpStatus::EXT_CURL_EASY, status);
	}
    if (op->mStatus)
    {
        int http_status(HTTP_OK);

        if (handle)
        {
            ccode = curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_status);
            if (ccode == CURLE_OK)
            {
                if (http_status >= 100 && http_status <= 999)
                {
                    char * cont_type(NULL);
                    ccode = curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &cont_type);
                    if (ccode == CURLE_OK)
                    {
                        if (cont_type)
                        {
                            op->mReplyConType = cont_type;
                        }
                    }
                    else
                    {
                        LL_WARNS(LOG_CORE) << "CURL error:" << ccode << " Attempting to get content type." << LL_ENDL;
                    }
                    op->mStatus = HttpStatus(http_status);
                }
                else
                {
                    LL_WARNS(LOG_CORE) << "Invalid HTTP response code ("
                        << http_status << ") received from server."
                        << LL_ENDL;
                    op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_INVALID_HTTP_STATUS);
                }
            }
            else
            {
                op->mStatus = HttpStatus(HttpStatus::LLCORE, HE_INVALID_HTTP_STATUS);
            }
        }
        else
        {
            LL_WARNS(LOG_CORE) << "Attempt to retrieve status from NULL handle!" << LL_ENDL;
        }
	}

    if (multi_handle && handle)
    {
        // Detach from multi and recycle handle
        curl_multi_remove_handle(multi_handle, handle);
        mHandleCache.freeHandle(op->mCurlHandle);
    }
    else
    {
        LL_WARNS(LOG_CORE) << "Curl multi_handle or handle is NULL on remove! multi:" 
            << std::hex << multi_handle << " h:" << std::hex << handle << std::dec << LL_ENDL;
    }

    op->mCurlHandle = NULL;

	// Tracing
	if (op->mTracing > HTTP_TRACE_OFF)
	{
		LL_INFOS(LOG_CORE) << "TRACE, RequestComplete, Handle:  "
                            << op->getHandle()
						    << ", Status:  " << op->mStatus.toTerseString()
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
	llassert_always(policy_class < mPolicyCount);

	return mActiveHandles ? mActiveHandles[policy_class] : 0;
}

void HttpLibcurl::policyUpdated(int policy_class)
{
	if (policy_class < 0 || policy_class >= mPolicyCount || ! mMultiHandles)
	{
		return;
	}
	
	HttpPolicy & policy(mService->getPolicy());
	
	if (! mActiveHandles[policy_class])
	{
		// Clear to set options.  As of libcurl 7.37.0, if a pipelining
		// multi handle has active requests and you try to set the
		// multi handle to non-pipelining, the library gets very angry
		// and goes off the rails corrupting memory.  A clue that you're
		// about to crash is that you'll get a missing server response
		// error (curl code 9).  So, if options are to be set, we let
		// the multi handle run out of requests, then set options, and
		// re-enable request processing.
		//
		// All of this stall mechanism exists for this reason.  If
		// libcurl becomes more resilient later, it should be possible
		// to remove all of this.  The connection limit settings are fine,
		// it's just that pipelined-to-non-pipelined transition that
		// is fatal at the moment.
		
		HttpPolicyClass & options(policy.getClassOptions(policy_class));
		CURLM * multi_handle(mMultiHandles[policy_class]);

		// Enable policy if stalled
		policy.stallPolicy(policy_class, false);
		mDirtyPolicy[policy_class] = false;

		if (options.mPipelining > 1)
		{
			// We'll try to do pipelining on this multihandle
			check_curl_multi_setopt(multi_handle,
									 CURLMOPT_PIPELINING,
									 1L);
			check_curl_multi_setopt(multi_handle,
									 CURLMOPT_MAX_PIPELINE_LENGTH,
									 long(options.mPipelining));
			check_curl_multi_setopt(multi_handle,
									 CURLMOPT_MAX_HOST_CONNECTIONS,
									 long(options.mPerHostConnectionLimit));
			check_curl_multi_setopt(multi_handle,
									 CURLMOPT_MAX_TOTAL_CONNECTIONS,
									 long(options.mConnectionLimit));
		}
		else
		{
			check_curl_multi_setopt(multi_handle,
									 CURLMOPT_PIPELINING,
									 0L);
			check_curl_multi_setopt(multi_handle,
									 CURLMOPT_MAX_HOST_CONNECTIONS,
									 0L);
			check_curl_multi_setopt(multi_handle,
									 CURLMOPT_MAX_TOTAL_CONNECTIONS,
									 long(options.mConnectionLimit));
		}
	}
	else if (! mDirtyPolicy[policy_class])
	{
		// Mark policy dirty and request a stall in the policy.
		// When policy goes idle, we'll re-invoke this method
		// and perform the change.  Don't allow this thread to
		// sleep while we're waiting for quiescence, we'll just
		// stop processing.
		mDirtyPolicy[policy_class] = true;
		policy.stallPolicy(policy_class, true);
	}
}

// ---------------------------------------
// HttpLibcurl::HandleCache
// ---------------------------------------

HttpLibcurl::HandleCache::HandleCache()
	: mHandleTemplate(NULL)
{
	mCache.reserve(50);
}


HttpLibcurl::HandleCache::~HandleCache()
{
	if (mHandleTemplate)
	{
		curl_easy_cleanup(mHandleTemplate);
		mHandleTemplate = NULL;
	}

	for (handle_cache_t::iterator it(mCache.begin()); mCache.end() != it; ++it)
	{
		curl_easy_cleanup(*it);
	}
	mCache.clear();
}


CURL * HttpLibcurl::HandleCache::getHandle()
{
	CURL * ret(NULL);
	
	if (! mCache.empty())
	{
		// Fastest path to handle
		ret = mCache.back();
		mCache.pop_back();
	}
	else if (mHandleTemplate)
	{
		// Still fast path
		ret = curl_easy_duphandle(mHandleTemplate);
	}
	else
	{
		// When all else fails
		ret = curl_easy_init();
	}

	return ret;
}


void HttpLibcurl::HandleCache::freeHandle(CURL * handle)
{
	if (! handle)
	{
		return;
	}

	curl_easy_reset(handle);
	if (! mHandleTemplate)
	{
		// Save the first freed handle as a template.
		mHandleTemplate = handle;
	}
	else
	{
		// Otherwise add it to the cache
		if (mCache.size() >= mCache.capacity())
		{
			mCache.reserve(mCache.capacity() + 50);
		}
		mCache.push_back(handle);
	}
}


// ---------------------------------------
// Free functions
// ---------------------------------------


struct curl_slist * append_headers_to_slist(const HttpHeaders::ptr_t &headers, struct curl_slist * slist)
{
	const HttpHeaders::const_iterator end(headers->end());
	for (HttpHeaders::const_iterator it(headers->begin()); end != it; ++it)
	{
		static const char sep[] = ": ";
		std::string header;
		header.reserve((*it).first.size() + (*it).second.size() + sizeof(sep));
		header.append((*it).first);
		header.append(sep);
		header.append((*it).second);
		
		slist = curl_slist_append(slist, header.c_str());
	}
	return slist;
}


}  // end namespace LLCore


namespace 
{
	
void check_curl_multi_code(CURLMcode code, int curl_setopt_option)
{
	if (CURLM_OK != code)
	{
		LL_WARNS(LOG_CORE) << "libcurl multi error detected:  " << curl_multi_strerror(code)
						   << ", curl_multi_setopt option:  " << curl_setopt_option
						   << LL_ENDL;
	}
}


void check_curl_multi_code(CURLMcode code)
{
	if (CURLM_OK != code)
	{
		LL_WARNS(LOG_CORE) << "libcurl multi error detected:  " << curl_multi_strerror(code)
						   << LL_ENDL;
	}
}

}  // end anonymous namespace
