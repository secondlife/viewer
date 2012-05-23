/**
 * @file _httprequestqueue.h
 * @brief Internal declaration for the operation request queue
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

#ifndef	_LLCORE_HTTP_REQUEST_QUEUE_H_
#define	_LLCORE_HTTP_REQUEST_QUEUE_H_


#include <vector>

#include "_refcounted.h"
#include "_mutex.h"


namespace LLCore
{


class HttpOperation;


/// Thread-safe queue of HttpOperation objects.  Just
/// a simple queue that handles the transfer of operation
/// requests from all HttpRequest instances into the
/// singleton HttpService instance.

class HttpRequestQueue : public LLCoreInt::RefCounted
{
protected:
	/// Caller acquires a Refcount on construction
	HttpRequestQueue();
	virtual ~HttpRequestQueue();

private:
	HttpRequestQueue(const HttpRequestQueue &);			// Not defined
	void operator=(const HttpRequestQueue &);			// Not defined

public:
	static void init();
	static void term();
	
	/// Threading:  callable by any thread once inited.
	inline static HttpRequestQueue * instanceOf()
		{
			return sInstance;
		}
	
public:
	typedef std::vector<HttpOperation *> OpContainer;

	/// Insert an object at the back of the reply queue.
	///
	/// Caller my provide one refcount to the Library which takes
	/// possession of the count.
	///
	/// Threading:  callable by any thread.
	void addOp(HttpOperation * op);

	/// Caller acquires reference count on returned operation
	///
	/// Threading:  callable by any thread.
	HttpOperation * fetchOp(bool wait);

	/// Caller acquires reference count on each returned operation
	///
	/// Threading:  callable by any thread.
	void fetchAll(bool wait, OpContainer & ops);
	
protected:
	static HttpRequestQueue *			sInstance;
	
protected:
	OpContainer							mQueue;
	LLCoreInt::HttpMutex				mQueueMutex;
	LLCoreInt::HttpConditionVariable	mQueueCV;
	
}; // end class HttpRequestQueue

}  // end namespace LLCore


#endif	// _LLCORE_HTTP_REQUEST_QUEUE_H_
