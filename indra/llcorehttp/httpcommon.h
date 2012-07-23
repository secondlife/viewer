/**
 * @file httpcommon.h
 * @brief Public-facing declarations and definitions of common types
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

#ifndef	_LLCORE_HTTP_COMMON_H_
#define	_LLCORE_HTTP_COMMON_H_

/// @package LLCore::HTTP
///
/// This library implements a high-level, Indra-code-free client interface to
/// HTTP services based on actual patterns found in the viewer and simulator.
/// Interfaces are similar to those supplied by the legacy classes
/// LLCurlRequest and LLHTTPClient.  To that is added a policy scheme that
/// allows an application to specify connection behaviors:  limits on
/// connections, HTTP keepalive, HTTP pipelining, retry-on-error limits, etc.
///
/// Features of the library include:
/// - Single, private working thread where all transport and processing occurs.
/// - Support for multiple consumers running in multiple threads.
/// - Scatter/gather (a.k.a. buffer array) model for bulk data movement.
/// - Reference counting used for many object instance lifetimes.
/// - Minimal data sharing across threads for correctness and low latency.
/// 
/// The public interface is declared in a few key header files:
/// - "llcorehttp/bufferarray.h"
/// - "llcorehttp/httpcommon.h"
/// - "llcorehttp/httphandler.h"
/// - "llcorehttp/httpheaders.h"
/// - "llcorehttp/httpoptions.h"
/// - "llcorehttp/httprequest.h"
/// - "llcorehttp/httpresponse.h"
///
/// The library is still under early development and particular users
/// may need access to internal implementation details that are found
/// in the _*.h header files.  But this is a crutch to be avoided if at
/// all possible and probably indicates some interface work is neeeded.
///
/// Using the library is fairly easy.  Global setup needs a few
/// steps:
///
/// - libcurl initialization including thread-safely callbacks for SSL:
///   .  curl_global_init(...)
///   .  CRYPTO_set_locking_callback(...)
///   .  CRYPTO_set_id_callback(...)
/// - HttpRequest::createService() called to instantiate singletons
///   and support objects.
///
/// An HTTP consumer in an application, and an application may have many
/// consumers, does a few things:
///
/// - Instantiate and retain an object based on HttpRequest.  This
///   object becomes the portal into runtime services for the consumer.
/// - Derive or mixin the HttpHandler class if you want notification
///   when requests succeed or fail.  This object's onCompleted()
///   method is invoked and an instance can be shared across
///   requests.
///
/// Issuing a request is straightforward:
/// - Construct a suitable URL.
/// - Configure HTTP options for the request.  (optional)
/// - Build a list of additional headers.  (optional)
/// - Invoke one of the requestXXXX() methods (requestGetByteRange,
///   requestPost, etc.) on the HttpRequest instance supplying the
///   above along with a policy class, a priority and an optional
///   pointer to an HttpHandler instance.  Work is then queued to
///   the worker thread and occurs asynchronously.
/// - Periodically invoke the update() method on the HttpRequest
///   instance which performs completion notification to HttpHandler
///   objects.
/// - Do completion processing in your onCompletion() method.
///
/// Code fragments:
/// Rather than a poorly-maintained example in comments, look in the
/// example subdirectory which is a minimal yet functional tool to do
/// GET request performance testing.  With four calls:
///
///   	init_curl();
///     LLCore::HttpRequest::createService();
///     LLCore::HttpRequest::startThread();
///     LLCore::HttpRequest * hr = new LLCore::HttpRequest();
///
/// the program is basically ready to issue requests.
///


#include "linden_common.h"		// Modifies curl/curl.h interfaces

#include <string>


namespace LLCore
{


/// All queued requests are represented by an HttpHandle value.
/// The invalid value is returned when a request failed to queue.
/// The actual status for these failures is then fetched with
/// HttpRequest::getStatus().
///
/// The handle is valid only for the life of a request.  On
/// return from any HttpHandler notification, the handle immediately
/// becomes invalid and may be recycled for other queued requests.

typedef void * HttpHandle;
#define LLCORE_HTTP_HANDLE_INVALID		(NULL)

/// For internal scheduling and metrics, we use a microsecond
/// timebase compatible with the environment.
typedef U64 HttpTime;

/// Error codes defined by the library itself as distinct from
/// libcurl (or any other transport provider).
enum HttpError
{
	// Successful value compatible with the libcurl codes.
	HE_SUCCESS = 0,

	// Intended for HTTP reply codes 100-999, indicates that
	// the reply should be considered an error by the application.
	HE_REPLY_ERROR = 1,
	
	// Service is shutting down and requested operation will
	// not be queued or performed.
	HE_SHUTTING_DOWN = 2,
	
	// Operation was canceled by request.
	HE_OP_CANCELED = 3,
	
	// Invalid content range header received.
	HE_INV_CONTENT_RANGE_HDR = 4,
	
	// Request handle not found
	HE_HANDLE_NOT_FOUND = 5,

	// Invalid datatype for option/setting
	HE_INVALID_ARG = 6,

	// Option hasn't been explicitly set
	HE_OPT_NOT_SET = 7,
	
	// Option not dynamic, must be set during init phase
	HE_OPT_NOT_DYNAMIC = 8,
	
	// Invalid HTTP status code returned by server
	HE_INVALID_HTTP_STATUS = 9
	
}; // end enum HttpError


/// HttpStatus encapsulates errors from libcurl (easy, multi), HTTP
/// reply status codes and internal errors as well.  The encapsulation
/// isn't expected to completely isolate the caller from libcurl but
/// basic operational tests (success or failure) are provided.
///
/// Non-HTTP status are encoded as (type, status) with type being
/// one of:  EXT_CURL_EASY, EXT_CURL_MULTI or LLCORE and status
/// being the success/error code from that domain.  HTTP status
/// is encoded as (status, error_flag).  Status should be in the
/// range [100, 999] and error_flag is either HE_SUCCESS or
/// HE_REPLY_ERROR to indicate whether this should be treated as
/// a successful status or an error.  The application is responsible
/// for making that determination and a range like [200, 299] isn't
/// automatically assumed to be definitive.
///
/// Examples:
///
/// 1.  Construct a default, successful status code:
///				HttpStatus();
///
/// 2.  Construct a successful, HTTP 200 status code:
///				HttpStatus(200);
///
/// 3.  Construct a failed, HTTP 404 not-found status code:
///				HttpStatus(404);
///
/// 4.  Construct a failed libcurl couldn't connect status code:
///				HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
///
/// 5.  Construct an HTTP 301 status code to be treated as success:
///				HttpStatus(301, HE_SUCCESS);
///

struct HttpStatus
{
	typedef unsigned short type_enum_t;
	
	HttpStatus()
		: mType(LLCORE),
		  mStatus(HE_SUCCESS)
		{}

	HttpStatus(type_enum_t type, short status)
		: mType(type),
		  mStatus(status)
		{}
	
	HttpStatus(int http_status)
		: mType(http_status),
		  mStatus(http_status >= 200 && http_status <= 299
				  ? HE_SUCCESS
				  : HE_REPLY_ERROR)
		{
			llassert(http_status >= 100 && http_status <= 999);
		}
	
	HttpStatus(const HttpStatus & rhs)
		: mType(rhs.mType),
		  mStatus(rhs.mStatus)
		{}

	HttpStatus & operator=(const HttpStatus & rhs)
		{
			// Don't care if lhs & rhs are the same object

			mType = rhs.mType;
			mStatus = rhs.mStatus;
			return *this;
		}
	
	static const type_enum_t EXT_CURL_EASY = 0;
	static const type_enum_t EXT_CURL_MULTI = 1;
	static const type_enum_t LLCORE = 2;
	
	type_enum_t			mType;
	short				mStatus;

	/// Test for successful status in the code regardless
	/// of error source (internal, libcurl).
	///
	/// @return			'true' when status is successful.
	///
	operator bool() const
	{
		return 0 == mStatus;
	}

	/// Inverse of previous operator.
	///
	/// @return			'true' on any error condition
	bool operator !() const
	{
		return 0 != mStatus;
	}

	/// Equality and inequality tests to bypass bool conversion
	/// which will do the wrong thing in conditional expressions.
	bool operator==(const HttpStatus & rhs) const
	{
		return mType == rhs.mType && mStatus == rhs.mStatus;
	}

	bool operator!=(const HttpStatus & rhs) const
	{
		return ! operator==(rhs);
	}

	/// Convert to single numeric representation.  Mainly
	/// for logging or other informal purposes.  Also
	/// creates an ambiguous second path to integer conversion
	/// which tends to find programming errors such as formatting
	/// the status to a stream (operator<<).
	operator unsigned long() const;
	unsigned long toULong() const
	{
		return operator unsigned long();
	}

	/// And to convert to a hex string.
	std::string toHex() const;
	
	/// Convert status to a string representation.  For
	/// success, returns an empty string.  For failure
	/// statuses, a string as appropriate for the source of
	/// the error code (libcurl easy, libcurl multi, or
	/// LLCore itself).
	std::string toString() const;

	/// Returns true if the status value represents an
	/// HTTP response status (100 - 999).
	bool isHttpStatus() const
	{
		return 	mType >= type_enum_t(100) && mType <= type_enum_t(999);
	}
	
}; // end struct HttpStatus

}  // end namespace LLCore

#endif	// _LLCORE_HTTP_COMMON_H_
