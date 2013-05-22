/**
 * @file httpoptions.cpp
 * @brief Implementation of the HTTPOptions class
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

#include "httpoptions.h"

#include "_httpinternal.h"


namespace LLCore
{


HttpOptions::HttpOptions()
	: RefCounted(true),
	  mWantHeaders(false),
	  mTracing(HTTP_TRACE_OFF),
	  mTimeout(HTTP_REQUEST_TIMEOUT_DEFAULT),
	  mRetries(HTTP_RETRY_COUNT_DEFAULT)
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


void HttpOptions::setRetries(unsigned int retries)
{
	mRetries = retries;
}


}   // end namespace LLCore
