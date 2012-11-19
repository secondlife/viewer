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
#include "_httppolicyclass.h"

#include "lltimer.h"


namespace LLCore
{


// Per-policy-class data for a running system.
// Collection of queues, parameters, history, metrics, etc.
// for a single policy class.
//
// Threading:  accessed only by worker thread
struct HttpPolicy::State
{
public:
	State()
		: mConnMax(HTTP_CONNECTION_LIMIT_DEFAULT),
		  mConnAt(HTTP_CONNECTION_LIMIT_DEFAULT),
		  mConnMin(1),
		  mNextSample(0),
		  mErrorCount(0),
		  mErrorFactor(0)
		{}
	
	HttpReadyQueue		mReadyQueue;
	HttpRetryQueue		mRetryQueue;

	HttpPolicyClass		mOptions;

	long				mConnMax;
	long				mConnAt;
	long				mConnMin;

	HttpTime			mNextSample;
	unsigned long		mErrorCount;
	unsigned long		mErrorFactor;
};


HttpPolicy::HttpPolicy(HttpService * service)
	: mActiveClasses(0),
	  mState(NULL),
	  mService(service)
{}


HttpPolicy::~HttpPolicy()
{
	shutdown();
	
	mService = NULL;
}


void HttpPolicy::shutdown()
{
	for (int policy_class(0); policy_class < mActiveClasses; ++policy_class)
	{
		HttpRetryQueue & retryq(mState[policy_class].mRetryQueue);
		while (! retryq.empty())
		{
			HttpOpRequest * op(retryq.top());
			retryq.pop();
		
			op->cancel();
			op->release();
		}

		HttpReadyQueue & readyq(mState[policy_class].mReadyQueue);
		while (! readyq.empty())
		{
			HttpOpRequest * op(readyq.top());
			readyq.pop();
		
			op->cancel();
			op->release();
		}
	}
	delete [] mState;
	mState = NULL;
	mActiveClasses = 0;
}


void HttpPolicy::start(const HttpPolicyGlobal & global,
					   const std::vector<HttpPolicyClass> & classes)
{
	llassert_always(! mState);

	mGlobalOptions = global;
	mActiveClasses = classes.size();
	mState = new State [mActiveClasses];
	for (int i(0); i < mActiveClasses; ++i)
	{
		mState[i].mOptions = classes[i];
		mState[i].mConnMax = classes[i].mConnectionLimit;
		mState[i].mConnAt = mState[i].mConnMax;
		mState[i].mConnMin = 2;
	}
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
	LL_WARNS("CoreHttp") << "HTTP request " << static_cast<HttpHandle>(op)
						 << " retry " << op->mPolicyRetries
						 << " scheduled for +" << (delta / HttpTime(1000))
						 << " mS.  Status:  " << op->mStatus.toHex()
						 << LL_ENDL;
	if (op->mTracing > 0)
	{
		LL_INFOS("CoreHttp") << "TRACE, ToRetryQueue, Handle:  "
							 << static_cast<HttpHandle>(op)
							 << LL_ENDL;
	}
	mState[policy_class].mRetryQueue.push(op);
}


// Attempt to deliver requests to the transport layer.
//
// Tries to find HTTP requests for each policy class with
// available capacity.  Starts with the retry queue first
// looking for requests that have waited long enough then
// moves on to the ready queue.
//
// If all queues are empty, will return an indication that
// the worker thread may sleep hard otherwise will ask for
// normal polling frequency.
//
HttpService::ELoopSpeed HttpPolicy::processReadyQueue()
{
	const HttpTime now(totalTime());
	HttpService::ELoopSpeed result(HttpService::REQUEST_SLEEP);
	HttpLibcurl & transport(mService->getTransport());
	
	for (int policy_class(0); policy_class < mActiveClasses; ++policy_class)
	{
		State & state(mState[policy_class]);
		int active(transport.getActiveCountInClass(policy_class));
		int needed(state.mConnAt - active);		// Expect negatives here

		HttpRetryQueue & retryq(state.mRetryQueue);
		HttpReadyQueue & readyq(state.mReadyQueue);
		
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
			result = HttpService::NORMAL;
		}
	} // end foreach policy_class

	return result;
}


bool HttpPolicy::changePriority(HttpHandle handle, HttpRequest::priority_t priority)
{
	for (int policy_class(0); policy_class < mActiveClasses; ++policy_class)
	{
		State & state(mState[policy_class]);
		// We don't scan retry queue because a priority change there
		// is meaningless.  The request will be issued based on retry
		// intervals not priority value, which is now moot.
		
		// Scan ready queue for requests that match policy
		HttpReadyQueue::container_type & c(state.mReadyQueue.get_container());
		for (HttpReadyQueue::container_type::iterator iter(c.begin()); c.end() != iter;)
		{
			HttpReadyQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c.erase(cur);									// All iterators are now invalidated
				op->mReqPriority = priority;
				state.mReadyQueue.push(op);						// Re-insert using adapter class
				return true;
			}
		}
	}
	
	return false;
}


bool HttpPolicy::cancel(HttpHandle handle)
{
	for (int policy_class(0); policy_class < mActiveClasses; ++policy_class)
	{
		State & state(mState[policy_class]);

		// Scan retry queue
		HttpRetryQueue::container_type & c1(state.mRetryQueue.get_container());
		for (HttpRetryQueue::container_type::iterator iter(c1.begin()); c1.end() != iter;)
		{
			HttpRetryQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c1.erase(cur);									// All iterators are now invalidated
				op->cancel();
				op->release();
				return true;
			}
		}
		
		// Scan ready queue
		HttpReadyQueue::container_type & c2(state.mReadyQueue.get_container());
		for (HttpReadyQueue::container_type::iterator iter(c2.begin()); c2.end() != iter;)
		{
			HttpReadyQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c2.erase(cur);									// All iterators are now invalidated
				op->cancel();
				op->release();
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
	static const HttpStatus send_error(HttpStatus::EXT_CURL_EASY, CURLE_SEND_ERROR);
	static const HttpStatus recv_error(HttpStatus::EXT_CURL_EASY, CURLE_RECV_ERROR);
	static const HttpStatus upload_failed(HttpStatus::EXT_CURL_EASY, CURLE_UPLOAD_FAILED);
	static const HttpStatus op_timedout(HttpStatus::EXT_CURL_EASY, CURLE_OPERATION_TIMEDOUT);
	static const HttpStatus post_error(HttpStatus::EXT_CURL_EASY, CURLE_HTTP_POST_ERROR);

	// Retry or finalize
	if (! op->mStatus)
	{
		// If this failed, we might want to retry.  Have to inspect
		// the status a little more deeply for those reasons worth retrying...
		if (op->mPolicyRetries < op->mPolicyRetryLimit &&
			((op->mStatus.isHttpStatus() && op->mStatus.mType >= 499 && op->mStatus.mType <= 599) ||
			 cant_connect == op->mStatus ||
			 cant_res_proxy == op->mStatus ||
			 cant_res_host == op->mStatus ||
			 send_error == op->mStatus ||
			 recv_error == op->mStatus ||
			 upload_failed == op->mStatus ||
			 op_timedout == op->mStatus ||
			 post_error == op->mStatus))
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
		LL_WARNS("CoreHttp") << "HTTP request " << static_cast<HttpHandle>(op)
							 << " failed after " << op->mPolicyRetries
							 << " retries.  Reason:  " << op->mStatus.toString()
							 << " (" << op->mStatus.toHex() << ")"
							 << LL_ENDL;
	}
	else if (op->mPolicyRetries)
	{
		LL_WARNS("CoreHttp") << "HTTP request " << static_cast<HttpHandle>(op)
							 << " succeeded on retry " << op->mPolicyRetries << "."
							 << LL_ENDL;
	}

	op->stageFromActive(mService);
	op->release();
	return false;						// not active
}


int HttpPolicy::getReadyCount(HttpRequest::policy_t policy_class) const
{
	if (policy_class < mActiveClasses)
	{
		return (mState[policy_class].mReadyQueue.size()
				+ mState[policy_class].mRetryQueue.size());
	}
	return 0;
}


}  // end namespace LLCore
