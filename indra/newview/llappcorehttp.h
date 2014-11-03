/**
 * @file llappcorehttp.h
 * @brief Singleton initialization/shutdown class for llcorehttp library
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2014, Linden Research, Inc.
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

#ifndef	_LL_APP_COREHTTP_H_
#define	_LL_APP_COREHTTP_H_


#include "httprequest.h"
#include "httphandler.h"
#include "httpresponse.h"


// This class manages the lifecyle of the core http library.
// Slightly different style than traditional code but reflects
// the use of handler classes and light-weight interface
// object instances of the new libraries.  To be used
// as a singleton and static construction is fine.
class LLAppCoreHttp : public LLCore::HttpHandler
{
public:
	static const long			PIPELINING_DEPTH;

	typedef LLCore::HttpRequest::policy_t policy_t;

	enum EAppPolicy
	{
		/// Catchall policy class.  Not used yet
		/// but will have a generous concurrency
		/// limit.  Deep queueing possible by having
		/// a chatty HTTP user.
		///
		/// Destination:     anywhere
		/// Protocol:        http: or https:
		/// Transfer size:   KB-MB
		/// Long poll:       no
		/// Concurrency:     high 
		/// Request rate:    unknown
		/// Pipelined:       no
		AP_DEFAULT,

		/// Texture fetching policy class.  Used to
		/// download textures via capability or SSA
		/// baking service.  Deep queueing of requests.
		/// Do not share.  GET requests only.
		///
		/// Destination:     simhost:12046 & {bake-texture,cdn}:80
		/// Protocol:        http:
		/// Transfer size:   KB-MB
		/// Long poll:       no
		/// Concurrency:     high
		/// Request rate:    high
		/// Pipelined:       yes
		AP_TEXTURE,

		/// Legacy mesh fetching policy class.  Used to
		/// download textures via 'GetMesh' capability.
		/// To be deprecated.  Do not share.
		///
		/// Destination:     simhost:12046
		/// Protocol:        http:
		/// Transfer size:   KB-MB
		/// Long poll:       no
		/// Concurrency:     dangerously high
		/// Request rate:    high
		/// Pipelined:       no
		AP_MESH1,

		/// New mesh fetching policy class.  Used to
		/// download textures via 'GetMesh2' capability.
		/// Used when fetch request (typically one LOD)
		/// is 'small', currently defined as 2MB.
		/// Very deeply queued.  Do not share.  GET
		/// requests only.
		///
		/// Destination:     simhost:12046 & cdn:80
		/// Protocol:        http:
		/// Transfer size:   KB-MB
		/// Long poll:       no
		/// Concurrency:     high
		/// Request rate:    high
		/// Pipelined:       yes
		AP_MESH2,

		/// Large mesh fetching policy class.  Used to
		/// download textures via 'GetMesh' or 'GetMesh2'
		/// capability.  Used when fetch request
		/// is not small to avoid head-of-line problem
		/// when large requests block a sequence of small,
		/// fast requests.  Can be shared with similar
		/// traffic that can wait for longish stalls
		/// (default timeout 600S).
		///
		/// Destination:     simhost:12046 & cdn:80
		/// Protocol:        http:
		/// Transfer size:   MB
		/// Long poll:       no
		/// Concurrency:     low
		/// Request rate:    low
		/// Pipelined:       no
		AP_LARGE_MESH,

		/// Asset upload policy class.  Used to store
		/// assets (mesh only at the moment) via
		/// changeable URL.  Responses may take some
		/// time (default timeout 240S).
		///
		/// Destination:     simhost:12043
		/// Protocol:        https:
		/// Transfer size:   KB-MB
		/// Long poll:       no
		/// Concurrency:     low
		/// Request rate:    low
		/// Pipelined:       no
		AP_UPLOADS,

		/// Long-poll-type HTTP requests.  Not
		/// bound by a connection limit.  Requests
		/// will typically hang around for a long
		/// time (~30S).  Only shareable with other
		/// long-poll requests.
		///
		/// Destination:     simhost:12043
		/// Protocol:        https:
		/// Transfer size:   KB
		/// Long poll:       yes
		/// Concurrency:     unlimited but low in practice
		/// Request rate:    low
		/// Pipelined:       no
		AP_LONG_POLL,

		/// Inventory operations (really Capabilities-
		/// related operations).  Mix of high-priority
		/// and low-priority operations.
		///
		/// Destination:     simhost:12043
		/// Protocol:        https:
		/// Transfer size:   KB-MB
		/// Long poll:       no
		/// Concurrency:     high
		/// Request rate:    high
		/// Pipelined:       no
		AP_INVENTORY,
		AP_REPORTING = AP_INVENTORY,	// Piggy-back on inventory
		
		AP_COUNT						// Must be last
	};
	
public:
	LLAppCoreHttp();
	~LLAppCoreHttp();
	
	// Initialize the LLCore::HTTP library creating service classes
	// and starting the servicing thread.  Caller is expected to do
	// other initializations (SSL mutex, thread hash function) appropriate
	// for the application.
	void init();

	// Request that the servicing thread stop servicing requests,
	// release resource references and stop.  Request is asynchronous
	// and @see cleanup() will perform a limited wait loop for this
	// request to stop the thread.
	void requestStop();
	
	// Terminate LLCore::HTTP library services.  Caller is expected
	// to have made a best-effort to shutdown the servicing thread
	// by issuing a requestThreadStop() and waiting for completion
	// notification that the stop has completed.
	void cleanup();

	// Notification when the stop request is complete.
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

	// Retrieve a policy class identifier for desired
	// application function.
	policy_t getPolicy(EAppPolicy policy) const
		{
			return mHttpClasses[policy].mPolicy;
		}

	// Return whether a policy is using pipelined operations.
	bool isPipelined(EAppPolicy policy) const
		{
			return mHttpClasses[policy].mPipelined;
		}

	// Apply initial or new settings from the environment.
	void refreshSettings(bool initial);
	
private:
	static const F64			MAX_THREAD_WAIT_TIME;
	
private:

	// PODish container for per-class settings and state.
	struct HttpClass
	{
	public:
		HttpClass();

	public:
		policy_t					mPolicy;			// Policy class id for the class
		U32							mConnLimit;
		bool						mPipelined;
		boost::signals2::connection mSettingsSignal;	// Signal to global setting that affect this class (if any)
	};
		
	LLCore::HttpRequest *		mRequest;				// Request queue to issue shutdowns
	LLCore::HttpHandle			mStopHandle;
	F64							mStopRequested;
	bool						mStopped;
	HttpClass					mHttpClasses[AP_COUNT];
	bool						mPipelined;				// Global setting
	boost::signals2::connection mPipelinedSignal;		// Signal for 'HttpPipelining' setting
};


#endif	// _LL_APP_COREHTTP_H_
