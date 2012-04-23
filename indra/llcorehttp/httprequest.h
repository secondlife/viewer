/**
 * @file httprequest.h
 * @brief Public-facing declarations for HttpRequest class
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

#ifndef	_LLCORE_HTTP_REQUEST_H_
#define	_LLCORE_HTTP_REQUEST_H_


#include "httpcommon.h"
#include "httphandler.h"


namespace LLCore
{

class HttpRequestQueue;
class HttpReplyQueue;
class HttpService;
class HttpOptions;
class HttpHeaders;
class HttpOperation;

/// HttpRequest supplies the entry into the HTTP transport
/// services in the LLCore libraries.  Services provided include:
///
/// - Some, but not all, global initialization of libcurl.
/// - Starting asynchronous, threaded HTTP requests.
/// - Definition of policy classes affect request handling.
/// - Utilities to control request options and headers
///
/// Requests
///
/// The class supports the current HTTP request operations:
///
/// - requestGetByteRange:  GET with Range header for a single range of bytes
///
/// Policy Classes
///
/// <TBD>
///
/// Usage
///
/// <TBD>
///
/// Threading:  An instance may only be used by one application/
/// consumer thread.  But a thread may have as many instances of
/// this as it likes.
///
/// Allocation:  Not refcounted, may be stack allocated though that
/// hasn't been tested.  Queued requests can still run and any
/// queued replies will keep refcounts to the reply queue leading
/// to memory leaks.
///
/// @pre Before using this class (static or instances), some global
/// initialization is required.  See @see httpcommon.h for more information.
///
/// @nosubgrouping
///

class HttpRequest
{
public:
	HttpRequest();
	virtual ~HttpRequest();

private:
	HttpRequest(const HttpRequest &);			// Disallowed
	void operator=(const HttpRequest &);		// Disallowed		

public:
	/// @name PolicyMethods
	/// @{

	/// Represents a default, catch-all policy class that guarantees
	/// eventual service for any HTTP request.
	static const int DEFAULT_POLICY_ID = 0;
	
	enum EGlobalPolicy
	{
		/// Maximum number of connections the library will use to
		/// perform operations.  This is somewhat soft as the underlying
		/// transport will cache some connections (up to 5).
		GLOBAL_CONNECTION_LIMIT
	};

	/// Set a parameter on a global policy option.  Calls
	/// made after the start of the servicing thread are
	/// not honored and return an error status.
	///
	/// @param opt		Enum of option to be set.
	/// @param value	Desired value of option.
	/// @return			Standard status code.
	HttpStatus setPolicyGlobalOption(EGlobalPolicy opt, long value);

	/// Create a new policy class into which requests can be made.
	///
	/// @return			If positive, the policy_id used to reference
	///					the class in other methods.  If -1, an error
	///					occurred and @see getStatus() may provide more
	///					detail on the reason.
	unsigned int createPolicyClass();

	enum EClassPolicy
	{
		/// Limits the number of connections used for the class.
		CLASS_CONNECTION_LIMIT,

		/// Limits the number of connections used for a single
		/// literal address/port pair within the class.
		PER_HOST_CONNECTION_LIMIT,

		/// Suitable requests are allowed to pipeline on their
		/// connections when they ask for it.
		ENABLE_PIPELINING
	};
	
	/// Set a parameter on a class-based policy option.  Calls
	/// made after the start of the servicing thread are
	/// not honored and return an error status.
	///
	/// @param policy_id		ID of class as returned by @see createPolicyClass().
	/// @param opt				Enum of option to be set.
	/// @param value			Desired value of option.
	/// @return					Standard status code.
	HttpStatus setPolicyClassOption(unsigned int policy_id,
									EClassPolicy opt,
									long value);

	/// @}

	/// @name RequestMethods
	///
	/// @{

	/// Some calls expect to succeed as the normal part of operation and so
	/// return a useful value rather than a status.  When they do fail, the
	/// status is saved and can be fetched with this method.
	///
	/// @return			Status of the failing method invocation.  If the
	///					preceding call succeeded or other HttpStatus
	///					returning calls immediately preceded this method,
	///					the returned value may not be reliable.
	///
	HttpStatus getStatus() const;

	/// Queue a full HTTP GET request to be issued with a 'Range' header.
	/// The request is queued and serviced by the working thread and
	/// notification of completion delivered to the optional HttpHandler
	/// argument during @see update() calls.
	///
	/// With a valid handle returned, it can be used to reference the
	/// request in other requests (like cancellation) and will be an
	/// argument when any HttpHandler object is invoked.
	///
	/// @param	policy_id		Default or user-defined policy class under
	///							which this request is to be serviced.
	/// @param	priority		Standard priority scheme inherited from
	///							Indra code base.
	/// @param	url
	/// @param	offset
	/// @param	len
	/// @param	options			(optional)
	/// @param	headers			(optional)
	/// @param	handler			(optional)
	/// @return					The handle of the request if successfully
	///							queued or LLCORE_HTTP_HANDLE_INVALID if the
	///							request could not be queued.  In the latter
	///							case, @see getStatus() will return more info.
	///
	HttpHandle requestGetByteRange(unsigned int policy_id,
								   float priority,
								   const std::string & url,
								   size_t offset,
								   size_t len,
								   HttpOptions * options,
								   HttpHeaders * headers,
								   HttpHandler * handler);


	/// Queue a NoOp request.
	/// The request is queued and serviced by the working thread which
	/// immediately processes it and returns the request to the reply
	/// queue.
	///
	/// @param	handler			(optional)
	/// @return					The handle of the request if successfully
	///							queued or LLCORE_HTTP_HANDLE_INVALID if the
	///							request could not be queued.  In the latter
	///							case, @see getStatus() will return more info.
	///
	HttpHandle requestNoOp(HttpHandler * handler);

	/// While all the heavy work is done by the worker thread, notifications
	/// must be performed in the context of the application thread.  These
	/// are done synchronously during calls to this method which gives the
	/// library control so notification can be performed.  Application handlers
	/// are expected to return 'quickly' and do any significant processing
	/// outside of the notification callback to onCompleted().
	///
	/// @param	millis			Maximum number of wallclock milliseconds to
	///							spend in the call.  As hinted at above, this
	///							is partly a function of application code so it's
	///							a soft limit.  (And not currently implemented.)
	///
	/// @return					Standard status code.
	HttpStatus update(long millis);

	/// @}
	
	/// @name RequestMgmtMethods
	///
	/// @{
	
	HttpHandle requestCancel(HttpHandle request, HttpHandler *);

	/// @}

	/// @name UtilityMethods
	///
	/// @{

	/// Initialization method that needs to be called before queueing any
	/// requests.  Doesn't start the worker thread and may be called befoer
	/// or after policy setup.
	static HttpStatus createService();

	/// Mostly clean shutdown of services prior to exit.  Caller is expected
	/// to have stopped a running worker thread before calling this.
	static HttpStatus destroyService();

	/// Called once after @see createService() to start the worker thread.
	/// Stopping the thread is achieved by requesting it via @see requestStopThread().
	/// May be called before or after requests are issued.
	static HttpStatus startThread();

	/// Queues a request to the worker thread to have it stop processing
	/// and exit (without exiting the program).  When the operation is
	/// picked up by the worker thread, it immediately processes it and
	/// begins detaching from refcounted resources like request and
	/// reply queues and then returns to the host OS.  It *does* queue a
	/// reply to give the calling application thread a notification that
	/// the operation has been performed.
	///
	/// @param	handler			(optional)
	/// @return					The handle of the request if successfully
	///							queued or LLCORE_HTTP_HANDLE_INVALID if the
	///							request could not be queued.  In the latter
	///							case, @see getStatus() will return more info.
	///							As the request cannot be cancelled, the handle
	///							is generally not useful.
	///
	HttpHandle requestStopThread(HttpHandler * handler);

	/// @}

protected:
	void generateNotification(HttpOperation * op);

	class InternalHandler;
	friend class InternalHandler;

private:
	/// @name InstanceData
	///
	/// @{
	HttpStatus			mLastReqStatus;
	HttpReplyQueue *	mReplyQueue;
	HttpRequestQueue *	mRequestQueue;
	InternalHandler *	mSelfHandler;
	
	/// @}
	
	// ====================================
	/// @name GlobalState
	///
	/// @{
	///
	/// Must be established before any threading is allowed to
	/// start.
	///
	static unsigned int		sNextPolicyID;
	
	/// @}
	// End Global State
	// ====================================
	
};  // end class HttpRequest


}  // end namespace LLCore



#endif	// _LLCORE_HTTP_REQUEST_H_
