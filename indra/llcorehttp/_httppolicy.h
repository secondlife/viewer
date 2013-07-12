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
#include "_httppolicyclass.h"
#include "_httpinternal.h"


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
	/// Cancel all ready and retry requests sending them to
	/// their notification queues.  Release state resources
	/// making further request handling impossible.
	///
	/// Threading:  called by worker thread
	void shutdown();

	/// Deliver policy definitions and enable handling of
	/// requests.  One-time call invoked before starting
	/// the worker thread.
	///
	/// Threading:  called by application thread
	void start(const HttpPolicyGlobal & global,
			   const std::vector<HttpPolicyClass> & classes);

	/// Give the policy layer some cycles to scan the ready
	/// queue promoting higher-priority requests to active
	/// as permited.
	///
	/// @return			Indication of how soon this method
	///					should be called again.
	///
	/// Threading:  called by worker thread
	HttpService::ELoopSpeed processReadyQueue();

	/// Add request to a ready queue.  Caller is expected to have
	/// provided us with a reference count to hold the request.  (No
	/// additional references will be added.)
	///
	/// OpRequest is owned by the request queue after this call
	/// and should not be modified by anyone until retrieved
	/// from queue.
	///
	/// Threading:  called by any thread
	void addOp(HttpOpRequest *);

	/// Similar to addOp, used when a caller wants to retry a
	/// request that has failed.  It's placed on a special retry
	/// queue but ordered by retry time not priority.  Otherwise,
	/// handling is the same and retried operations are considered
	/// before new ones but that doesn't guarantee completion
	/// order.
	///
	/// Threading:  called by worker thread
	void retryOp(HttpOpRequest *);

	/// Attempt to change the priority of an earlier request.
	/// Request that Shadows HttpService's method
	///
	/// Threading:  called by worker thread
	bool changePriority(HttpHandle handle, HttpRequest::priority_t priority);

	/// Attempt to cancel a previous request.
	/// Shadows HttpService's method as well
	///
	/// Threading:  called by worker thread
	bool cancel(HttpHandle handle);

	/// When transport is finished with an op and takes it off the
	/// active queue, it is delivered here for dispatch.  Policy
	/// may send it back to the ready/retry queues if it needs another
	/// go or we may finalize it and send it on to the reply queue.
	///
	/// @return			Returns true of the request is still active
	///					or ready after staging, false if has been
	///					sent on to the reply queue.
	///
	/// Threading:  called by worker thread
	bool stageAfterCompletion(HttpOpRequest * op);
	
	// Get pointer to global policy options.  Caller is expected
	// to do context checks like no setting once running.
	///
	/// Threading:  called by any thread *but* the object may
	/// only be modified by the worker thread once running.
	///
	HttpPolicyGlobal & getGlobalOptions()
		{
			return mGlobalOptions;
		}

	/// Get ready counts for a particular policy class
	///
	/// Threading:  called by worker thread
	int getReadyCount(HttpRequest::policy_t policy_class) const;
	
protected:
	struct State;

	int									mActiveClasses;
	State *								mState;
	HttpService *						mService;				// Naked pointer, not refcounted, not owner
	HttpPolicyGlobal					mGlobalOptions;
	
};  // end class HttpPolicy

}  // end namespace LLCore

#endif // _LLCORE_HTTP_POLICY_H_
