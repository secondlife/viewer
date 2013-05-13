/**
 * @file _httprequestqueue.cpp
 * @brief 
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

#include "_httprequestqueue.h"

#include "_httpoperation.h"
#include "_mutex.h"


using namespace LLCoreInt;

namespace LLCore
{

HttpRequestQueue * HttpRequestQueue::sInstance(NULL);


HttpRequestQueue::HttpRequestQueue()
	: RefCounted(true),
	  mQueueStopped(false)
{
}


HttpRequestQueue::~HttpRequestQueue()
{
	while (! mQueue.empty())
	{
		HttpOperation * op = mQueue.back();
		mQueue.pop_back();
		op->release();
	}
}


void HttpRequestQueue::init()
{
	llassert_always(! sInstance);
	sInstance = new HttpRequestQueue();
}


void HttpRequestQueue::term()
{
	if (sInstance)
	{
		sInstance->release();
		sInstance = NULL;
	}
}


HttpStatus HttpRequestQueue::addOp(HttpOperation * op)
{
	bool wake(false);
	{
		HttpScopedLock lock(mQueueMutex);

		if (mQueueStopped)
		{
			// Return op and error to caller
			return HttpStatus(HttpStatus::LLCORE, HE_SHUTTING_DOWN);
		}
		wake = mQueue.empty();
		mQueue.push_back(op);
	}
	if (wake)
	{
		mQueueCV.notify_all();
	}
	return HttpStatus();
}


HttpOperation * HttpRequestQueue::fetchOp(bool wait)
{
	HttpOperation * result(NULL);

	{
		HttpScopedLock lock(mQueueMutex);

		while (mQueue.empty())
		{
			if (! wait || mQueueStopped)
				return NULL;
			mQueueCV.wait(lock);
		}

		result = mQueue.front();
		mQueue.erase(mQueue.begin());
	}

	// Caller also acquires the reference count
	return result;
}


void HttpRequestQueue::fetchAll(bool wait, OpContainer & ops)
{
	// Not valid putting something back on the queue...
	llassert_always(ops.empty());

	{
		HttpScopedLock lock(mQueueMutex);

		while (mQueue.empty())
		{
			if (! wait || mQueueStopped)
				return;
			mQueueCV.wait(lock);
		}

		mQueue.swap(ops);
	}

	// Caller also acquires the reference counts on each op.
	return;
}


void HttpRequestQueue::wakeAll()
{
	mQueueCV.notify_all();
}


void HttpRequestQueue::stopQueue()
{
	{
		HttpScopedLock lock(mQueueMutex);

		mQueueStopped = true;
		wakeAll();
	}
}


} // end namespace LLCore
