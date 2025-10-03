/**
 * @file _httppolicyclass.cpp
 * @brief Definitions for internal class defining class policy option.
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

#include "_httppolicyclass.h"

#include "_httpinternal.h"


namespace LLCore
{


HttpPolicyClass::HttpPolicyClass()
    : mConnectionLimit(HTTP_CONNECTION_LIMIT_DEFAULT),
      mPerHostConnectionLimit(HTTP_CONNECTION_LIMIT_DEFAULT),
      mPipelining(HTTP_PIPELINING_DEFAULT),
      mThrottleRate(HTTP_THROTTLE_RATE_DEFAULT)
{}


HttpPolicyClass::~HttpPolicyClass()
{}


HttpPolicyClass & HttpPolicyClass::operator=(const HttpPolicyClass & other)
{
    if (this != &other)
    {
        mConnectionLimit = other.mConnectionLimit;
        mPerHostConnectionLimit = other.mPerHostConnectionLimit;
        mPipelining = other.mPipelining;
        mThrottleRate = other.mThrottleRate;
    }
    return *this;
}


HttpPolicyClass::HttpPolicyClass(const HttpPolicyClass & other)
    : mConnectionLimit(other.mConnectionLimit),
      mPerHostConnectionLimit(other.mPerHostConnectionLimit),
      mPipelining(other.mPipelining),
      mThrottleRate(other.mThrottleRate)
{}


HttpStatus HttpPolicyClass::set(HttpRequest::EPolicyOption opt, long value)
{
    switch (opt)
    {
    case HttpRequest::PO_CONNECTION_LIMIT:
        mConnectionLimit = llclamp(value, long(HTTP_CONNECTION_LIMIT_MIN), long(HTTP_CONNECTION_LIMIT_MAX));
        break;

    case HttpRequest::PO_PER_HOST_CONNECTION_LIMIT:
        mPerHostConnectionLimit = llclamp(value, long(HTTP_CONNECTION_LIMIT_MIN), mConnectionLimit);
        break;

    case HttpRequest::PO_PIPELINING_DEPTH:
        mPipelining = llclamp(value, 0L, HTTP_PIPELINING_MAX);
        break;

    case HttpRequest::PO_THROTTLE_RATE:
        mThrottleRate = llclamp(value, 0L, 1000000L);
        break;

    default:
        return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
    }

    return HttpStatus();
}


HttpStatus HttpPolicyClass::get(HttpRequest::EPolicyOption opt, long * value) const
{
    switch (opt)
    {
    case HttpRequest::PO_CONNECTION_LIMIT:
        *value = mConnectionLimit;
        break;

    case HttpRequest::PO_PER_HOST_CONNECTION_LIMIT:
        *value = mPerHostConnectionLimit;
        break;

    case HttpRequest::PO_PIPELINING_DEPTH:
        *value = mPipelining;
        break;

    case HttpRequest::PO_THROTTLE_RATE:
        *value = mThrottleRate;
        break;

    default:
        return HttpStatus(HttpStatus::LLCORE, HE_INVALID_ARG);
    }

    return HttpStatus();
}


}  // end namespace LLCore
