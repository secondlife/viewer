/**
 * @file _httplibcurl.h
 * @brief Declarations for internal class providing libcurl transport.
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

#ifndef	_LLCORE_HTTP_LIBCURL_H_
#define	_LLCORE_HTTP_LIBCURL_H_

#include "linden_common.h"		// Modifies curl/curl.h interfaces

#include <curl/curl.h>
#include <curl/multi.h>

#include <set>

#include "httprequest.h"
#include "_httpservice.h"
#include "_httpinternal.h"


namespace LLCore
{


class HttpPolicy;
class HttpOpRequest;
class HttpHeaders;


/// Implements libcurl-based transport for an HttpService instance.
///
/// Threading:  Single-threaded.  Other than for construction/destruction,
/// all methods are expected to be invoked in a single thread, typically
/// a worker thread of some sort.

class HttpLibcurl
{
public:
	HttpLibcurl(HttpService * service);
	virtual ~HttpLibcurl();

private:
	HttpLibcurl(const HttpLibcurl &);			// Not defined
	void operator=(const HttpLibcurl &);		// Not defined

public:
	/// Give cycles to libcurl to run active requests.  Completed
	/// operations (successful or failed) will be retried or handed
	/// over to the reply queue as final responses.
	///
	/// @return			Indication of how long this method is
	///					willing to wait for next service call.
	HttpService::ELoopSpeed processTransport();

	/// Add request to the active list.  Caller is expected to have
	/// provided us with a reference count on the op to hold the
	/// request.  (No additional references will be added.)
	void addOp(HttpOpRequest * op);

	/// One-time call to set the number of policy classes to be
	/// serviced and to create the resources for each.  Value
	/// must agree with HttpPolicy::setPolicies() call.
	void start(int policy_count);

	/// Synchronously stop libcurl operations.  All active requests
	/// are canceled and removed from libcurl's handling.  Easy
	/// handles are detached from their multi handles and released.
	/// Multi handles are also released.  Canceled requests are
	/// completed with canceled status and made available on their
	/// respective reply queues.
	///
	/// Can be restarted with a start() call.
	void shutdown();

	/// Return global and per-class counts of active requests.
	int getActiveCount() const;
	int getActiveCountInClass(int policy_class) const;

	/// Attempt to cancel a request identified by handle.
	///
	/// Interface shadows HttpService's method.
	///
	/// @return			True if handle was found and operation canceled.
	///
	bool cancel(HttpHandle handle);

protected:
	/// Invoked when libcurl has indicated a request has been processed
	/// to completion and we need to move the request to a new state.
	bool completeRequest(CURLM * multi_handle, CURL * handle, CURLcode status);

	/// Invoked to cancel an active request, mainly during shutdown
	/// and destroy.
	void cancelRequest(HttpOpRequest * op);
	
protected:
	typedef std::set<HttpOpRequest *> active_set_t;
	
protected:
	HttpService *		mService;				// Simple reference, not owner
	active_set_t		mActiveOps;
	int					mPolicyCount;
	CURLM **			mMultiHandles;
}; // end class HttpLibcurl

}  // end namespace LLCore

#endif // _LLCORE_HTTP_LIBCURL_H_
