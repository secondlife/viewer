/**
 * @file httpoptions.h
 * @brief Public-facing declarations for the HTTPOptions class
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

#ifndef	_LLCORE_HTTP_OPTIONS_H_
#define	_LLCORE_HTTP_OPTIONS_H_


#include "httpcommon.h"

#include "_refcounted.h"


namespace LLCore
{


/// Really a struct in spirit, it provides options that
/// modify HTTP requests.
///
/// Sharing instances across requests.  It's intended that
/// these be shared across requests:  caller can create one
/// of these, set it up as needed and then reference it
/// repeatedly in HTTP operations.  But see the Threading
/// note about references.
///
/// Threading:  While this class does nothing to ensure thread
/// safety, it *is* intended to be shared between the application
/// thread and the worker thread.  This means that once an instance
/// is delivered to the library in request operations, the
/// option data must not be written until all such requests
/// complete and relinquish their references.
///
/// Allocation:  Refcounted, heap only.  Caller of the constructor
/// is given a refcount.
///
class HttpOptions : public LLCoreInt::RefCounted
{
public:
	HttpOptions();

protected:
	virtual ~HttpOptions();						// Use release()
	
	HttpOptions(const HttpOptions &);			// Not defined
	void operator=(const HttpOptions &);		// Not defined

public:
	void				setWantHeaders(bool wanted);
	bool				getWantHeaders() const
		{
			return mWantHeaders;
		}
	
	void				setTrace(int long);
	int					getTrace() const
		{
			return mTracing;
		}

	void				setTimeout(unsigned int timeout);
	unsigned int		getTimeout() const
		{
			return mTimeout;
		}

	void				setRetries(unsigned int retries);
	unsigned int		getRetries() const
		{
			return mRetries;
		}
	
protected:
	bool				mWantHeaders;
	int					mTracing;
	unsigned int		mTimeout;
	unsigned int		mRetries;
	
}; // end class HttpOptions


}  // end namespace HttpOptions

#endif	// _LLCORE_HTTP_OPTIONS_H_
