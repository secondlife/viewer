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

	// Shadows HttpService's method
	bool changePriority(HttpHandle handle, unsigned int priority);
	
protected:
	int					mReadyInClass[HttpRequest::POLICY_CLASS_LIMIT];
	HttpReadyQueue		mReadyQueue[HttpRequest::POLICY_CLASS_LIMIT];
	HttpService *		mService;				// Naked pointer, not refcounted, not owner
	
};  // end class HttpPolicy

}  // end namespace LLCore

#endif // _LLCORE_HTTP_POLICY_H_
