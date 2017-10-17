/**
 * @file httpoptions.cpp
 * @brief Implementation of the HTTPOptions class
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

#include "httpoptions.h"
#include "lldefs.h"
#include "_httpinternal.h"


namespace LLCore
{


HttpOptions::HttpOptions() :
    mWantHeaders(false),
    mTracing(HTTP_TRACE_OFF),
    mTimeout(HTTP_REQUEST_TIMEOUT_DEFAULT),
    mTransferTimeout(HTTP_REQUEST_XFER_TIMEOUT_DEFAULT),
    mRetries(HTTP_RETRY_COUNT_DEFAULT),
    mMinRetryBackoff(HTTP_RETRY_BACKOFF_MIN_DEFAULT),
    mMaxRetryBackoff(HTTP_RETRY_BACKOFF_MAX_DEFAULT),
    mUseRetryAfter(HTTP_USE_RETRY_AFTER_DEFAULT),
    mFollowRedirects(true),
    mVerifyPeer(false),
    mVerifyHost(false),
    mDNSCacheTimeout(-1L),
    mNoBody(false)
{}


HttpOptions::~HttpOptions()
{}


void HttpOptions::setWantHeaders(bool wanted)
{
	mWantHeaders = wanted;
}


void HttpOptions::setTrace(long level)
{
	mTracing = int(level);
}


void HttpOptions::setTimeout(unsigned int timeout)
{
	mTimeout = timeout;
}


void HttpOptions::setTransferTimeout(unsigned int timeout)
{
	mTransferTimeout = timeout;
}


void HttpOptions::setRetries(unsigned int retries)
{
	mRetries = retries;
}

void HttpOptions::setMinBackoff(HttpTime delay)
{
	mMinRetryBackoff = delay;
}

void HttpOptions::setMaxBackoff(HttpTime delay)
{
	mMaxRetryBackoff = delay;
}

void HttpOptions::setUseRetryAfter(bool use_retry)
{
	mUseRetryAfter = use_retry;
}

void HttpOptions::setFollowRedirects(bool follow_redirect)
{
	mFollowRedirects = follow_redirect;
}

void HttpOptions::setSSLVerifyPeer(bool verify)
{
	mVerifyPeer = verify;
}

void HttpOptions::setSSLVerifyHost(bool verify)
{
	mVerifyHost = verify;
}

void HttpOptions::setDNSCacheTimeout(int timeout)
{
	mDNSCacheTimeout = timeout;
}

void HttpOptions::setHeadersOnly(bool nobody)
{
    mNoBody = nobody;
    if (mNoBody)
        setWantHeaders(true);
}

}   // end namespace LLCore
