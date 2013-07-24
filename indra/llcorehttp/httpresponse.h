/**
 * @file httpresponse.h
 * @brief Public-facing declarations for the HttpResponse class
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

#ifndef	_LLCORE_HTTP_RESPONSE_H_
#define	_LLCORE_HTTP_RESPONSE_H_


#include <string>

#include "httpcommon.h"

#include "_refcounted.h"


namespace LLCore
{

class BufferArray;
class HttpHeaders;

/// HttpResponse is instantiated by the library and handed to
/// the caller during callbacks to the handler.  It supplies
/// all the status, header and HTTP body data the caller is
/// interested in.  Methods provide simple getters to return
/// individual pieces of the response.
///
/// Typical usage will have the caller interrogate the object
/// and return from the handler callback.  Instances are refcounted
/// and callers can bump the count and retain the object as needed.
///
/// Threading:  Not intrinsically thread-safe.
///
/// Allocation:  Refcounted, heap only.  Caller of the constructor
/// is given a refcount.
///
class HttpResponse : public LLCoreInt::RefCounted
{
public:
	HttpResponse();

protected:
	virtual ~HttpResponse();							// Use release()
	
	HttpResponse(const HttpResponse &);					// Not defined
	void operator=(const HttpResponse &);				// Not defined
	
public:
	/// Returns the final status of the requested operation.
	///
	HttpStatus getStatus() const
		{
			return mStatus;
		}

	void setStatus(const HttpStatus & status)
		{
			mStatus = status;
		}

	/// Simple getter for the response body returned as a scatter/gather
	/// buffer.  If the operation doesn't produce data (such as the Null
	/// or StopThread operations), this may be NULL.
	///
	/// Caller can hold onto the response by incrementing the reference
	/// count of the returned object.
	BufferArray * getBody() const
		{
			return mBufferArray;
		}

	/// Set the response data in the instance.  Will drop the reference
	/// count to any existing data and increment the count of that passed
	/// in.  It is legal to set the data to NULL.
	void setBody(BufferArray * ba);
	
	/// And a getter for the headers.  And as with @see getResponse(),
	/// if headers aren't available because the operation doesn't produce
	/// any or delivery of headers wasn't requested in the options, this
	/// will be NULL.
	///
	/// Caller can hold onto the headers by incrementing the reference
	/// count of the returned object.
	HttpHeaders * getHeaders() const
		{
			return mHeaders;
		}

	/// Behaves like @see setResponse() but for header data.
	void setHeaders(HttpHeaders * headers);

	/// If a 'Range:' header was used, these methods are involved
	/// in setting and returning data about the actual response.
	/// If both @offset and @length are returned as 0, we probably
	/// didn't get a Content-Range header in the response.  This
	/// occurs with various Capabilities-based services and the
	/// caller is going to have to make assumptions on receipt of
	/// a 206 status.  The @full value may also be zero in cases of
	/// parsing problems or a wild-carded length response.
	void getRange(unsigned int * offset, unsigned int * length, unsigned int * full) const
		{
			*offset = mReplyOffset;
			*length = mReplyLength;
			*full = mReplyFullLength;
		}

	void setRange(unsigned int offset, unsigned int length, unsigned int full_length)
		{
			mReplyOffset = offset;
			mReplyLength = length;
			mReplyFullLength = full_length;
		}

	///
	const std::string & getContentType() const
		{
			return mContentType;
		}

	void setContentType(const std::string & con_type)
		{
			mContentType = con_type;
		}

protected:
	// Response data here
	HttpStatus			mStatus;
	unsigned int		mReplyOffset;
	unsigned int		mReplyLength;
	unsigned int		mReplyFullLength;
	BufferArray *		mBufferArray;
	HttpHeaders *		mHeaders;
	std::string			mContentType;
};


}   // end namespace LLCore

#endif	// _LLCORE_HTTP_RESPONSE_H_
