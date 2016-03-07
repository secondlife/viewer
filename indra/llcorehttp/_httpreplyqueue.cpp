/**
 * @file _httpreplyqueue.cpp
 * @brief Internal definitions for the operation reply queue
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

#include "_httpreplyqueue.h"


#include "_mutex.h"
#include "_thread.h"
#include "_httpoperation.h"

using namespace LLCoreInt;


namespace LLCore
{


HttpReplyQueue::HttpReplyQueue()
	: RefCounted(true)
{
}


HttpReplyQueue::~HttpReplyQueue()
{
	while (! mQueue.empty())
	{
		HttpOperation * op = mQueue.back();
		mQueue.pop_back();
		op->release();
	}
}


void HttpReplyQueue::addOp(HttpOperation * op)
{
	{
		HttpScopedLock lock(mQueueMutex);

		mQueue.push_back(op);
	}
	mQueueCV.notify_all();
}


HttpOperation * HttpReplyQueue::fetchOp()
{
	HttpOperation * result(NULL);

	{
		HttpScopedLock lock(mQueueMutex);

		if (mQueue.empty())
			return NULL;

		result = mQueue.front();
		mQueue.erase(mQueue.begin());
	}

	// Caller also acquires the reference count
	return result;
}


void HttpReplyQueue::fetchAll(OpContainer & ops)
{
	// Not valid putting something back on the queue...
	llassert_always(ops.empty());

	{
		HttpScopedLock lock(mQueueMutex);

		if (! mQueue.empty())
		{
			mQueue.swap(ops);
		}
	}

	// Caller also acquires the reference counts on each op.
	return;
}


}  // end namespace LLCore
