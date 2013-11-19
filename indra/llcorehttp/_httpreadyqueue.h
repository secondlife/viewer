/**
 * @file _httpreadyqueue.h
 * @brief Internal declaration for the operation ready queue
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

#ifndef	_LLCORE_HTTP_READY_QUEUE_H_
#define	_LLCORE_HTTP_READY_QUEUE_H_


#include <queue>

#include "_httpinternal.h"
#include "_httpoprequest.h"


namespace LLCore
{

/// HttpReadyQueue provides a simple priority queue for HttpOpRequest objects.
///
/// This uses the priority_queue adaptor class to provide the queue
/// as well as the ordering scheme while allowing us access to the
/// raw container if we follow a few simple rules.  One of the more
/// important of those rules is that any iterator becomes invalid
/// on element erasure.  So pay attention.
///
/// If LLCORE_HTTP_READY_QUEUE_IGNORES_PRIORITY tests true, the class
/// implements a std::priority_queue interface but on std::deque
/// behavior to eliminate sensitivity to priority.  In the future,
/// this will likely become the only behavior or it may become
/// a run-time election.
///
/// Threading:  not thread-safe.  Expected to be used entirely by
/// a single thread, typically a worker thread of some sort.

#if LLCORE_HTTP_READY_QUEUE_IGNORES_PRIORITY

typedef std::deque<HttpOpRequest *> HttpReadyQueueBase;

#else

typedef std::priority_queue<HttpOpRequest *,
							std::deque<HttpOpRequest *>,
							LLCore::HttpOpRequestCompare> HttpReadyQueueBase;

#endif // LLCORE_HTTP_READY_QUEUE_IGNORES_PRIORITY

class HttpReadyQueue : public HttpReadyQueueBase
{
public:
	HttpReadyQueue()
		: HttpReadyQueueBase()
		{}
	
	~HttpReadyQueue()
		{}
	
protected:
	HttpReadyQueue(const HttpReadyQueue &);		// Not defined
	void operator=(const HttpReadyQueue &);		// Not defined

public:

#if LLCORE_HTTP_READY_QUEUE_IGNORES_PRIORITY
	// Types and methods needed to make a std::deque look
	// more like a std::priority_queue, at least for our
	// purposes.
	typedef HttpReadyQueueBase container_type;
	
	const_reference top() const
		{
			return front();
		}

	void pop()
		{
			pop_front();
		}

	void push(const value_type & v)
		{
			push_back(v);
		}
	
#endif // LLCORE_HTTP_READY_QUEUE_IGNORES_PRIORITY
	
	const container_type & get_container() const
		{
			return *this;
		}

	container_type & get_container()
		{
			return *this;
		}
	
}; // end class HttpReadyQueue


}  // end namespace LLCore


#endif	// _LLCORE_HTTP_READY_QUEUE_H_
