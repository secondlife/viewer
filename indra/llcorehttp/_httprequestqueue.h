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

#include "httpcommon.h"
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

protected:
	virtual ~HttpRequestQueue();						// Use release()

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

	/// Insert an object at the back of the request queue.
	///
	/// Caller must provide one refcount to the queue which takes
	/// possession of the count on success.
	///
	/// @return			Standard status.  On failure, caller
	///					must dispose of the operation with
	///					an explicit release() call.
	///
	/// Threading:  callable by any thread.
	HttpStatus addOp(HttpOperation * op);

	/// Return the operation on the front of the queue.  If
	/// the queue is empty and @wait is false, call returns
	/// immediately and a NULL pointer is returned.  If true,
	/// caller will sleep until explicitly woken.  Wakeups
	/// can be spurious and callers must expect NULL pointers
	/// even if waiting is indicated.
	///
	/// Caller acquires reference count any returned operation
	///
	/// Threading:  callable by any thread.
	HttpOperation * fetchOp(bool wait);

	/// Return all queued requests to caller.  The @ops argument
	/// should be empty when called and will be swap()'d with
	/// current contents.  Handling of the @wait argument is
	/// identical to @fetchOp.
	///
	/// Caller acquires reference count on each returned operation
	///
	/// Threading:  callable by any thread.
	void fetchAll(bool wait, OpContainer & ops);

	/// Wake any sleeping threads.  Normal queuing operations
	/// won't require this but it may be necessary for termination
	/// requests.
	///
	/// Threading:  callable by any thread.
	void wakeAll();

	/// Disallow further request queuing.  Callers to @addOp will
	/// get a failure status (LLCORE, HE_SHUTTING_DOWN).  Callers
	/// to @fetchAll or @fetchOp will get requests that are on the
	/// queue but the calls will no longer wait.  Instead they'll
	/// return immediately.  Also wakes up all sleepers to send
	/// them on their way.
	///
	/// Threading:  callable by any thread.
	void stopQueue();
	
protected:
	static HttpRequestQueue *			sInstance;
	
protected:
	OpContainer							mQueue;
	LLCoreInt::HttpMutex				mQueueMutex;
	LLCoreInt::HttpConditionVariable	mQueueCV;
	bool								mQueueStopped;
	
}; // end class HttpRequestQueue

}  // end namespace LLCore


#endif	// _LLCORE_HTTP_REQUEST_QUEUE_H_
