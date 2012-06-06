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

#include "_httppolicy.h"

#include "_httpoprequest.h"
#include "_httpservice.h"
#include "_httplibcurl.h"


namespace LLCore
{

HttpPolicy::HttpPolicy(HttpService * service)
	: mService(service)
{
	for (int policy_class(0); policy_class < HttpRequest::POLICY_CLASS_LIMIT; ++policy_class)
	{
		mReadyInClass[policy_class] = 0;
	}
}


HttpPolicy::~HttpPolicy()
{
	for (int policy_class(0); policy_class < HttpRequest::POLICY_CLASS_LIMIT; ++policy_class)
	{
		HttpReadyQueue & readyq(mReadyQueue[policy_class]);
		
		while (! readyq.empty())
		{
			HttpOpRequest * op(readyq.top());
		
			op->cancel();
			op->release();
			mReadyInClass[policy_class]--;
			readyq.pop();
		}
	}
	mService = NULL;
}


void HttpPolicy::addOp(HttpOpRequest * op)
{
	const int policy_class(op->mReqPolicy);
	
	mReadyQueue[policy_class].push(op);
	++mReadyInClass[policy_class];
}


HttpService::ELoopSpeed HttpPolicy::processReadyQueue()
{
	HttpService::ELoopSpeed result(HttpService::REQUEST_SLEEP);
	HttpLibcurl & transport(mService->getTransport());
	
	for (int policy_class(0); policy_class < HttpRequest::POLICY_CLASS_LIMIT; ++policy_class)
	{
		HttpReadyQueue & readyq(mReadyQueue[policy_class]);
		int active(transport.getActiveCountInClass(policy_class));
		int needed(8 - active);

		if (needed > 0 && mReadyInClass[policy_class] > 0)
		{
			// Scan ready queue for requests that match policy

			while (! readyq.empty() && needed > 0 && mReadyInClass[policy_class] > 0)
			{
				HttpOpRequest * op(readyq.top());
				readyq.pop();

				op->stageFromReady(mService);
				op->release();
					
				--mReadyInClass[policy_class];
				--needed;
			}
		}

		if (! readyq.empty())
		{
			// If anything is ready, continue looping...
			result = (std::min)(result, HttpService::NORMAL);
		}
	}

	return result;
}


bool HttpPolicy::changePriority(HttpHandle handle, HttpRequest::priority_t priority)
{
	for (int policy_class(0); policy_class < HttpRequest::POLICY_CLASS_LIMIT; ++policy_class)
	{
		HttpReadyQueue::container_type & c(mReadyQueue[policy_class].get_container());
	
		// Scan ready queue for requests that match policy
		for (HttpReadyQueue::container_type::iterator iter(c.begin()); c.end() != iter;)
		{
			HttpReadyQueue::container_type::iterator cur(iter++);

			if (static_cast<HttpHandle>(*cur) == handle)
			{
				HttpOpRequest * op(*cur);
				c.erase(cur);							// All iterators are now invalidated
				op->mReqPriority = priority;
				mReadyQueue[policy_class].push(op);		// Re-insert using adapter class
				return true;
			}
		}
	}
	
	return false;
}


}  // end namespace LLCore
