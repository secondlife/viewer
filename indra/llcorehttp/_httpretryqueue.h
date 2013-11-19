/**
 * @file _httpretryqueue.h
 * @brief Internal declaration for the operation retry queue
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

#ifndef	_LLCORE_HTTP_RETRY_QUEUE_H_
#define	_LLCORE_HTTP_RETRY_QUEUE_H_


#include <queue>

#include "_httpoprequest.h"


namespace LLCore
{

/// HttpRetryQueue provides a simple priority queue for HttpOpRequest objects.
///
/// This uses the priority_queue adaptor class to provide the queue
/// as well as the ordering scheme while allowing us access to the
/// raw container if we follow a few simple rules.  One of the more
/// important of those rules is that any iterator becomes invalid
/// on element erasure.  So pay attention.
///
/// Threading:  not thread-safe.  Expected to be used entirely by
/// a single thread, typically a worker thread of some sort.

struct HttpOpRetryCompare
{
	bool operator()(const HttpOpRequest * lhs, const HttpOpRequest * rhs)
		{
			return lhs->mPolicyRetryAt < rhs->mPolicyRetryAt;
		}
};

	
typedef std::priority_queue<HttpOpRequest *,
							std::deque<HttpOpRequest *>,
							LLCore::HttpOpRetryCompare> HttpRetryQueueBase;

class HttpRetryQueue : public HttpRetryQueueBase
{
public:
	HttpRetryQueue()
		: HttpRetryQueueBase()
		{}
	
	~HttpRetryQueue()
		{}
	
protected:
	HttpRetryQueue(const HttpRetryQueue &);		// Not defined
	void operator=(const HttpRetryQueue &);		// Not defined

public:
	const container_type & get_container() const
		{
			return c;
		}

	container_type & get_container()
		{
			return c;
		}

}; // end class HttpRetryQueue


}  // end namespace LLCore


#endif	// _LLCORE_HTTP_RETRY_QUEUE_H_
