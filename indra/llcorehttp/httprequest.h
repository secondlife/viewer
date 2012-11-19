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
class BufferArray;

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
	typedef unsigned int policy_t;
	typedef unsigned int priority_t;
	
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

		/// A long value setting the maximum number of connections
		/// allowed over all policy classes.  Note that this will be
		/// a somewhat soft value.  There may be an additional five
		/// connections per policy class depending upon runtime
		/// behavior.
		GP_CONNECTION_LIMIT,

		/// String containing a system-appropriate directory name
		/// where SSL certs are stored.
		GP_CA_PATH,

		/// String giving a full path to a file containing SSL certs.
		GP_CA_FILE,

		/// String of host/port to use as simple HTTP proxy.  This is
		/// going to change in the future into something more elaborate
		/// that may support richer schemes.
		GP_HTTP_PROXY,

		/// Long value that if non-zero enables the use of the
		/// traditional LLProxy code for http/socks5 support.  If
		/// enabled, has priority over GP_HTTP_PROXY.
		GP_LLPROXY,

		/// Long value setting the logging trace level for the
		/// library.  Possible values are:
		/// 0 - No tracing (default)
		/// 1 - Basic tracing of request start, stop and major events.
		/// 2 - Connection, header and payload size information from
		///     HTTP transactions.
		/// 3 - Partial logging of payload itself.
		///
		/// These values are also used in the trace modes for
		/// individual requests in HttpOptions.  Also be aware that
		/// tracing tends to impact performance of the viewer.
		GP_TRACE
	};

	/// Set a parameter on a global policy option.  Calls
	/// made after the start of the servicing thread are
	/// not honored and return an error status.
	///
	/// @param opt		Enum of option to be set.
	/// @param value	Desired value of option.
	/// @return			Standard status code.
	static HttpStatus setPolicyGlobalOption(EGlobalPolicy opt, long value);
	static HttpStatus setPolicyGlobalOption(EGlobalPolicy opt, const std::string & value);

	/// Create a new policy class into which requests can be made.
	///
	/// @return			If positive, the policy_id used to reference
	///					the class in other methods.  If 0, an error
	///					occurred and @see getStatus() may provide more
	///					detail on the reason.
	static policy_t createPolicyClass();

	enum EClassPolicy
	{
		/// Limits the number of connections used for the class.
		CP_CONNECTION_LIMIT,

		/// Limits the number of connections used for a single
		/// literal address/port pair within the class.
		CP_PER_HOST_CONNECTION_LIMIT,

		/// Suitable requests are allowed to pipeline on their
		/// connections when they ask for it.
		CP_ENABLE_PIPELINING
	};
	
	/// Set a parameter on a class-based policy option.  Calls
	/// made after the start of the servicing thread are
	/// not honored and return an error status.
	///
	/// @param policy_id		ID of class as returned by @see createPolicyClass().
	/// @param opt				Enum of option to be set.
	/// @param value			Desired value of option.
	/// @return					Standard status code.
	static HttpStatus setPolicyClassOption(policy_t policy_id, EClassPolicy opt, long value);

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

	/// Queue a full HTTP GET request to be issued for entire entity.
	/// The request is queued and serviced by the working thread and
	/// notification of completion delivered to the optional HttpHandler
	/// argument during @see update() calls.
	///
	/// With a valid handle returned, it can be used to reference the
	/// request in other requests (like cancellation) and will be an
	/// argument when any HttpHandler object is invoked.
	///
	/// Headers supplied by default:
	/// - Connection: keep-alive
	/// - Accept: */*
	/// - Accept-Encoding: deflate, gzip
	/// - Keep-alive: 300
	/// - Host: <stuff>
	///
	/// Some headers excluded by default:
	/// - Pragma:
	/// - Cache-control:
	/// - Range:
	/// - Transfer-Encoding:
	/// - Referer:
	///
	/// @param	policy_id		Default or user-defined policy class under
	///							which this request is to be serviced.
	/// @param	priority		Standard priority scheme inherited from
	///							Indra code base (U32-type scheme).
	/// @param	url				URL with any encoded query parameters to
	///							be accessed.
	/// @param	options			Optional instance of an HttpOptions object
	///							to provide additional controls over the request
	///							function for this request only.  Any such
	///							object then becomes shared-read across threads
	///							and no code should modify the HttpOptions
	///							instance.
	/// @param	headers			Optional instance of an HttpHeaders object
	///							to provide additional and/or overridden
	///							headers for the request.  As with options,
	///							the instance becomes shared-read across threads
	///							and no code should modify the HttpHeaders
	///							instance.
	/// @param	handler			Optional pointer to an HttpHandler instance
	///							whose onCompleted() method will be invoked
	///							during calls to update().  This is a non-
	///							reference-counted object which would be a
	///							problem for shutdown and other edge cases but
	///							the pointer is only dereferenced during
	///							calls to update().
	///
	/// @return					The handle of the request if successfully
	///							queued or LLCORE_HTTP_HANDLE_INVALID if the
	///							request could not be queued.  In the latter
	///							case, @see getStatus() will return more info.
	///
	HttpHandle requestGet(policy_t policy_id,
						  priority_t priority,
						  const std::string & url,
						  HttpOptions * options,
						  HttpHeaders * headers,
						  HttpHandler * handler);


	/// Queue a full HTTP GET request to be issued with a 'Range' header.
	/// The request is queued and serviced by the working thread and
	/// notification of completion delivered to the optional HttpHandler
	/// argument during @see update() calls.
	///
	/// With a valid handle returned, it can be used to reference the
	/// request in other requests (like cancellation) and will be an
	/// argument when any HttpHandler object is invoked.
	///
	/// Headers supplied by default:
	/// - Connection: keep-alive
	/// - Accept: */*
	/// - Accept-Encoding: deflate, gzip
	/// - Keep-alive: 300
	/// - Host: <stuff>
	/// - Range: <stuff>  (will be omitted if offset == 0 and len == 0)
	///
	/// Some headers excluded by default:
	/// - Pragma:
	/// - Cache-control:
	/// - Transfer-Encoding:
	/// - Referer:
	///
	/// @param	policy_id		@see requestGet()
	/// @param	priority		"
	/// @param	url				"
	/// @param	offset			Offset of first byte into resource to be returned.
	/// @param	len				Count of bytes to be returned
	/// @param	options			@see requestGet()
	/// @param	headers			"
	/// @param	handler			"
	/// @return					"
	///
	HttpHandle requestGetByteRange(policy_t policy_id,
								   priority_t priority,
								   const std::string & url,
								   size_t offset,
								   size_t len,
								   HttpOptions * options,
								   HttpHeaders * headers,
								   HttpHandler * handler);


	/// Queue a full HTTP POST.  Query arguments and body may
	/// be provided.  Caller is responsible for escaping and
	/// encoding and communicating the content types.
	///
	/// Headers supplied by default:
	/// - Connection: keep-alive
	/// - Accept: */*
	/// - Accept-Encoding: deflate, gzip
	/// - Keep-Alive: 300
	/// - Host: <stuff>
	/// - Content-Length: <digits>
	/// - Content-Type: application/x-www-form-urlencoded
	///
	/// Some headers excluded by default:
	/// - Pragma:
	/// - Cache-Control:
	/// - Transfer-Encoding: ... chunked ...
	/// - Referer:
	/// - Content-Encoding:
	/// - Expect:
	///
	/// @param	policy_id		@see requestGet()
	/// @param	priority		"
	/// @param	url				"
	/// @param	body			Byte stream to be sent as the body.  No
	///							further encoding or escaping will be done
	///							to the content.
	/// @param	options			@see requestGet()K(optional)
	/// @param	headers			"
	/// @param	handler			"
	/// @return					"
	///
	HttpHandle requestPost(policy_t policy_id,
						   priority_t priority,
						   const std::string & url,
						   BufferArray * body,
						   HttpOptions * options,
						   HttpHeaders * headers,
						   HttpHandler * handler);


	/// Queue a full HTTP PUT.  Query arguments and body may
	/// be provided.  Caller is responsible for escaping and
	/// encoding and communicating the content types.
	///
	/// Headers supplied by default:
	/// - Connection: keep-alive
	/// - Accept: */*
	/// - Accept-Encoding: deflate, gzip
	/// - Keep-Alive: 300
	/// - Host: <stuff>
	/// - Content-Length: <digits>
	///
	/// Some headers excluded by default:
	/// - Pragma:
	/// - Cache-Control:
	/// - Transfer-Encoding: ... chunked ...
	/// - Referer:
	/// - Content-Encoding:
	/// - Expect:
	/// - Content-Type:
	///
	/// @param	policy_id		@see requestGet()
	/// @param	priority		"
	/// @param	url				"
	/// @param	body			Byte stream to be sent as the body.  No
	///							further encoding or escaping will be done
	///							to the content.
	/// @param	options			@see requestGet()K(optional)
	/// @param	headers			"
	/// @param	handler			"
	/// @return					"
	///
	HttpHandle requestPut(policy_t policy_id,
						  priority_t priority,
						  const std::string & url,
						  BufferArray * body,
						  HttpOptions * options,
						  HttpHeaders * headers,
						  HttpHandler * handler);


	/// Queue a NoOp request.
	/// The request is queued and serviced by the working thread which
	/// immediately processes it and returns the request to the reply
	/// queue.
	///
	/// @param	handler			@see requestGet()
	/// @return					"
	///
	HttpHandle requestNoOp(HttpHandler * handler);

	/// While all the heavy work is done by the worker thread, notifications
	/// must be performed in the context of the application thread.  These
	/// are done synchronously during calls to this method which gives the
	/// library control so notification can be performed.  Application handlers
	/// are expected to return 'quickly' and do any significant processing
	/// outside of the notification callback to onCompleted().
	///
	/// @param	usecs			Maximum number of wallclock microseconds to
	///							spend in the call.  As hinted at above, this
	///							is partly a function of application code so it's
	///							a soft limit.  A '0' value will run without
	///							time limit until everything queued has been
	///							delivered.
	///
	/// @return					Standard status code.
	HttpStatus update(long usecs);

	/// @}
	
	/// @name RequestMgmtMethods
	///
	/// @{
	
	HttpHandle requestCancel(HttpHandle request, HttpHandler *);

	/// Request that a previously-issued request be reprioritized.
	/// The status of whether the change itself succeeded arrives
	/// via notification.  
	///
	/// @param	request			Handle of previously-issued request to
	///							be changed.
	/// @param	priority		New priority value.
	/// @param	handler			@see requestGet()
	/// @return					"
	///
	HttpHandle requestSetPriority(HttpHandle request, priority_t priority, HttpHandler * handler);

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

	/// Queue a Spin request.
	/// DEBUG/TESTING ONLY.  This puts the worker into a CPU spin for
	/// test purposes.
	///
	/// @param	mode			0 for hard spin, 1 for soft spin
	/// @return					Standard handle return cases.
	///
	HttpHandle requestSpin(int mode);

	/// @}
	
	/// @name DynamicPolicyMethods
	///
	/// @{

	/// Request that a running transport pick up a new proxy setting.
	/// An empty string will indicate no proxy is to be used.
	HttpHandle requestSetHttpProxy(const std::string & proxy, HttpHandler * handler);

    /// @}

protected:
	void generateNotification(HttpOperation * op);

private:
	/// @name InstanceData
	///
	/// @{
	HttpStatus			mLastReqStatus;
	HttpReplyQueue *	mReplyQueue;
	HttpRequestQueue *	mRequestQueue;
	
	/// @}
	
	// ====================================
	/// @name GlobalState
	///
	/// @{
	///
	/// Must be established before any threading is allowed to
	/// start.
	///
	static policy_t		sNextPolicyID;
	
	/// @}
	// End Global State
	// ====================================
	
};  // end class HttpRequest


}  // end namespace LLCore



#endif	// _LLCORE_HTTP_REQUEST_H_
