/**
 * @file _httppolicy.cpp
 * @brief Internal definitions of the Http policy thread
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

#include "linden_common.h"

#include "_httppolicy.h"

#include "_httpoprequest.h"
#include "_httpservice.h"
#include "_httplibcurl.h"

#include "lltimer.h"


namespace LLCore
{

HttpPolicy::HttpPolicy(HttpService * service)
	: mService(service)
{}


HttpPolicy::~HttpPolicy()
{
	for (int policy_class(0); policy_class < LL_ARRAY_SIZE(mState); ++policy_class)
	{
		HttpRetryQueue & retryq(mState[policy_class].mRetryQueue);
		while (! retryq.empty())
		{
			HttpOpRequest * op(retryq.top());
		
			op->cancel();
			op->release();
			retryq.pop();
		}

		HttpReadyQueue & readyq(mState[policy_class].mReadyQueue);
		while (! readyq.empty())
		{
			HttpOpRequest * op(readyq.top());
		
			op->cancel();
			op->release();
			readyq.pop();
		}
	}
	mService = NULL;
}


void HttpPolicy::addOp(HttpOpRequest * op)
{
	const int policy_class(op->mReqPolicy);
	
	op->mPolicyRetries = 0;
	mState[policy_class].mReadyQueue.push(op);
}


void HttpPolicy::retryOp(HttpOpRequest * op)
{
	static const HttpTime retry_deltas[] =
		{
			 250000,			// 1st retry in 0.25 S, etc...
			 500000,
			1000000,
			2000000,
			5000000				// ... to every 5.0 S.
		};
	static const int delta_max(int(LL_ARRAY_SIZE(retry_deltas)) - 1);
	
	const HttpTime now(totalTime());
	const int policy_class(op->mReqPolicy);
	
	const HttpTime delta(retry_deltas[llclamp(op->mPolicyRetries, 0, delta_max)]);
	op->mPolicyRetryAt = now + delta;
	++op->mPolicyRetries;
	LL_WARNS("CoreHttp") << "URL op retry #" << op->mPolicyRetries
						 << " being scheduled for " << delta << " uSecs from now."
						 << LL_ENDL;
	mState[policy_class].mRetryQueue.push(op);
}


HttpService::ELoopSpeed HttpPolicy::processReadyQueue()
{
	const HttpTime now(totalTime());
	HttpService::ELoopSpeed result(HttpService::REQUEST_SLEEP);
	HttpLibcurl & transport(mService->getTransport());
	
	for (int policy_class(0); policy_class < LL_ARRAY_SIZE(mState); ++policy_class)
	{
		int active(transport.getActiveCountInClass(policy_class));
		int needed(8 - active);

		HttpRetryQueue & retryq(mState[policy_class].mRetryQueue);
		HttpReadyQueue & readyq(mState[policy_class].mReadyQueue);
		
		if (needed > 0)
		{
			// First see if we have any retries...
			while (needed > 0 && ! retryq.empty())
			{
				HttpOpRequest * op(retryq.top());
				if (op->mPolicyRetryAt > now)
					break;
			
				retryq.pop();
				
				op->stageFromReady(mService);
				op->release();
					
				--needed;
			}
		
			// Now go on to the new requests...
			while (needed > 0 && ! readyq.empty())
			{
				HttpOpRequest * op(readyq.top());
				readyq.pop();

				op->stageFromReady(mService);
				op->release();
					
				--needed;
			}
		}
				
		if (! readyq.empty() || ! retryq.empty())
		{
			// If anything is ready, continue looping...
			result = (std::min)(result, HttpService::NORMAL);
		}
	} // end foreach policy_class

	return result;
}


bool HttpPolicy::changePriority(HttpHandle handle, HttpRequest::priority_t priority)
{
	for (int policy_class(0); policy_class < LL_ARRAY_SIZE(mState); ++policy_class)
	{
		HttpReadyQueue::container_type & c(mState[policy_class].mReadyQueue.get_container());
	
		// Scan ready queue for requests that match policy
		for (HttpReadyQueue::container_type::iterator iter(c.begin()); c.end() != iter;)
		{
			HttpReadyQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c.erase(cur);									// All iterators are now invalidated
				op->mReqPriority = priority;
				mState[policy_class].mReadyQueue.push(op);		// Re-insert using adapter class
				return true;
			}
		}
	}
	
	return false;
}

bool HttpPolicy::stageAfterCompletion(HttpOpRequest * op)
{
	static const HttpStatus cant_connect(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
	static const HttpStatus cant_res_proxy(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_PROXY);
	static const HttpStatus cant_res_host(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_RESOLVE_HOST);

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
			retryOp(op);
			return true;				// still active/ready
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
	return false;						// not active
}


}  // end namespace LLCore
