/**
 * @file httpoptions.h
 * @brief Public-facing declarations for the HTTPOptions class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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
class HttpOptions : private boost::noncopyable
{
public:
	HttpOptions();

	typedef boost::shared_ptr<HttpOptions> ptr_t;

    virtual ~HttpOptions();						// Use release()

protected:
	
	HttpOptions(const HttpOptions &);			// Not defined
	void operator=(const HttpOptions &);		// Not defined

public:

	// Default:   false
	void				setWantHeaders(bool wanted);
	bool				getWantHeaders() const
	{
		return mWantHeaders;
	}

	// Default:  0
	void				setTrace(int long);
	int					getTrace() const
	{
		return mTracing;
	}

	// Default:  30
	void				setTimeout(unsigned int timeout);
	unsigned int		getTimeout() const
	{
		return mTimeout;
	}

	// Default:  0
	void				setTransferTimeout(unsigned int timeout);
	unsigned int		getTransferTimeout() const
	{
		return mTransferTimeout;
	}

    /// Sets the number of retries on an LLCore::HTTPRequest before the 
    /// request fails.
	// Default:  8
	void				setRetries(unsigned int retries);
	unsigned int		getRetries() const
	{
		return mRetries;
	}

	// Default:  true
	void				setUseRetryAfter(bool use_retry);
	bool				getUseRetryAfter() const
	{
		return mUseRetryAfter;
	}

    /// Instructs the LLCore::HTTPRequest to follow redirects 
	/// Default: false
	void				setFollowRedirects(bool follow_redirect);
	bool				getFollowRedirects() const
	{
		return mFollowRedirects;
	}

    /// Instructs the LLCore::HTTPRequest to verify that the exchanged security
    /// certificate is authentic. 
    /// Default: false
    void				setSSLVerifyPeer(bool verify);
	bool				getSSLVerifyPeer() const
	{
		return mVerifyPeer;
	}

    /// Instructs the LLCore::HTTPRequest to verify that the name in the 
    /// security certificate matches the name of the host contacted.
    /// Default: false
    void				setSSLVerifyHost(bool verify);
	bool	        	getSSLVerifyHost() const
	{
		return mVerifyHost;
	}

    /// Sets the time for DNS name caching in seconds.  Setting this value
    /// to 0 will disable name caching.  Setting this value to -1 causes the 
    /// name cache to never time out.
    /// Default: -1
	void				setDNSCacheTimeout(int timeout);
	int					getDNSCacheTimeout() const
	{
		return mDNSCacheTimeout;
	}

    /// Retrieve only the headers and status from the request. Setting this 
    /// to true implies setWantHeaders(true) as well.
    /// Default: false
    void                setHeadersOnly(bool nobody);
    bool                getHeadersOnly() const
    {
        return mNoBody;
    }
	
protected:
	bool				mWantHeaders;
	int					mTracing;
	unsigned int		mTimeout;
	unsigned int		mTransferTimeout;
	unsigned int		mRetries;
	bool				mUseRetryAfter;
	bool				mFollowRedirects;
	bool				mVerifyPeer;
	bool        		mVerifyHost;
	int					mDNSCacheTimeout;
    bool                mNoBody;
}; // end class HttpOptions


}  // end namespace HttpOptions

#endif	// _LLCORE_HTTP_OPTIONS_H_
