/**
 * @file httpheaders.h
 * @brief Public-facing declarations for the HttpHeaders class
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

#ifndef	_LLCORE_HTTP_HEADERS_H_
#define	_LLCORE_HTTP_HEADERS_H_


#include <string>

#include "_refcounted.h"


namespace LLCore
{

///
/// Maintains an ordered list of name/value pairs representing
/// HTTP header lines.  This is used both to provide additional
/// headers when making HTTP requests and in responses when the
/// caller has asked that headers be returned (not the default
/// option).
///
/// @note
/// This is a minimally-functional placeholder at the moment
/// to fill out the class hierarchy.  The final class will be
/// something else, probably more pair-oriented.  It's also
/// an area where shared values are desirable so refcounting is
/// already specced and a copy-on-write scheme imagined.
/// Expect changes here.
///
/// Threading:  Not intrinsically thread-safe.  It *is* expected
/// that callers will build these objects and then share them
/// via reference counting with the worker thread.  The implication
/// is that once an HttpHeader instance is handed to a request,
/// the object must be treated as read-only.
///
/// Allocation:  Refcounted, heap only.  Caller of the
/// constructor is given a refcount.
///

class HttpHeaders : public LLCoreInt::RefCounted
{
public:
	/// @post In addition to the instance, caller has a refcount
	/// to the instance.  A call to @see release() will destroy
	/// the instance.
	HttpHeaders();

protected:
	virtual ~HttpHeaders();						// Use release()

	HttpHeaders(const HttpHeaders &);			// Not defined
	void operator=(const HttpHeaders &);		// Not defined

public:
	typedef std::vector<std::string> container_t;
	container_t			mHeaders;
	
}; // end class HttpHeaders

}  // end namespace LLCore


#endif	// _LLCORE_HTTP_HEADERS_H_
