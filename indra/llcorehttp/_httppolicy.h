/**
 * @file _httppolicy.h
 * @brief Declarations for internal class enforcing policy decisions.
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

#ifndef	_LLCORE_HTTP_POLICY_H_
#define	_LLCORE_HTTP_POLICY_H_


#include "httprequest.h"
#include "_httpservice.h"
#include "_httpreadyqueue.h"
#include "_httpretryqueue.h"
#include "_httppolicyglobal.h"


namespace LLCore
{

class HttpReadyQueue;
class HttpOpRequest;


/// Implements class-based queuing policies for an HttpService instance.
///
/// Threading:  Single-threaded.  Other than for construction/destruction,
/// all methods are expected to be invoked in a single thread, typically
/// a worker thread of some sort.
class HttpPolicy
{
public:
	HttpPolicy(HttpService *);
	virtual ~HttpPolicy();

private:
	HttpPolicy(const HttpPolicy &);				// Not defined
	void operator=(const HttpPolicy &);			// Not defined

public:
	/// Give the policy layer some cycles to scan the ready
	/// queue promoting higher-priority requests to active
	/// as permited.
	HttpService::ELoopSpeed processReadyQueue();

	/// Add request to a ready queue.  Caller is expected to have
	/// provided us with a reference count to hold the request.  (No
	/// additional references will be added.)
	void addOp(HttpOpRequest *);

	/// Similar to addOp, used when a caller wants to retry a
	/// request that has failed.  It's placed on a special retry
	/// queue but ordered by retry time not priority.  Otherwise,
	/// handling is the same and retried operations are considered
	/// before new ones but that doesn't guarantee completion
	/// order.
	void retryOp(HttpOpRequest *);

	// Shadows HttpService's method
	bool changePriority(HttpHandle handle, HttpRequest::priority_t priority);

	/// When transport is finished with an op and takes it off the
	/// active queue, it is delivered here for dispatch.  Policy
	/// may send it back to the ready/retry queues if it needs another
	/// go or we may finalize it and send it on to the reply queue.
	///
	/// @return			Returns true of the request is still active
	///					or ready after staging, false if has been
	///					sent on to the reply queue.
	bool stageAfterCompletion(HttpOpRequest * op);
	
	// Get pointer to global policy options.  Caller is expected
	// to do context checks like no setting once running.
	HttpPolicyGlobal &	getGlobalOptions()
		{
			return mGlobalOptions;
		}

	void setPolicies(const HttpPolicyGlobal & global);
			
protected:
	struct State
	{
		HttpReadyQueue		mReadyQueue;
		HttpRetryQueue		mRetryQueue;
	};

	State				mState[HttpRequest::POLICY_CLASS_LIMIT];
	HttpService *		mService;				// Naked pointer, not refcounted, not owner
	HttpPolicyGlobal	mGlobalOptions;
	
};  // end class HttpPolicy

}  // end namespace LLCore

#endif // _LLCORE_HTTP_POLICY_H_
