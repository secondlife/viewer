/** 
 * @file llhttpretrypolicy.h
 * @brief Header for a retry policy class intended for use with http responders.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llhttpretrypolicy.h"

namespace
{
    // Moved from httpconstants.h... only used in this file.
    bool isHttpServerErrorStatus(S32 status)
    {
        // Status 499 is sometimes used for re-interpreted status 2xx errors.
        // Allow retry of these, since we don't have enough information in this
        // context to know if this will always fail.
        if (HTTP_INTERNAL_ERROR == status) return true;

        // Check for status 5xx.
        return((500 <= status) && (status < 600));
    }
}

LLAdaptiveRetryPolicy::LLAdaptiveRetryPolicy(F32 min_delay, F32 max_delay, F32 backoff_factor, U32 max_retries, bool retry_on_4xx):
	mMinDelay(min_delay),
	mMaxDelay(max_delay),
	mBackoffFactor(backoff_factor),
	mMaxRetries(max_retries),
	mRetryOn4xx(retry_on_4xx)
{
	init();
}

void LLAdaptiveRetryPolicy::init()
{
	mDelay = mMinDelay;
	mRetryCount = 0;
	mShouldRetry = true;
}

void LLAdaptiveRetryPolicy::reset()
{
	init();
}

bool LLAdaptiveRetryPolicy::getRetryAfter(const LLSD& headers, F32& retry_header_time)
{
	return (headers.has(HTTP_IN_HEADER_RETRY_AFTER)
			&& getSecondsUntilRetryAfter(headers[HTTP_IN_HEADER_RETRY_AFTER].asStringRef(), retry_header_time));
}

bool LLAdaptiveRetryPolicy::getRetryAfter(const LLCore::HttpHeaders::ptr_t &headers, F32& retry_header_time)
{
	if (headers)
	{
		const std::string *retry_value = headers->find(HTTP_IN_HEADER_RETRY_AFTER.c_str()); 
		if (retry_value && 
			getSecondsUntilRetryAfter(*retry_value, retry_header_time))
		{
			return true;
		}
	}
	return false;
}

void LLAdaptiveRetryPolicy::onSuccess()
{
	init();
}

void LLAdaptiveRetryPolicy::onFailure(S32 status, const LLSD& headers)
{
	F32 retry_header_time;
	bool has_retry_header_time = getRetryAfter(headers,retry_header_time);
	onFailureCommon(status, has_retry_header_time, retry_header_time);
}
  
void LLAdaptiveRetryPolicy::onFailure(const LLCore::HttpResponse *response)
{
	F32 retry_header_time;
	const LLCore::HttpHeaders::ptr_t headers = response->getHeaders();
	bool has_retry_header_time = getRetryAfter(headers,retry_header_time);
	onFailureCommon(response->getStatus().getType(), has_retry_header_time, retry_header_time);
}

void LLAdaptiveRetryPolicy::onFailureCommon(S32 status, bool has_retry_header_time, F32 retry_header_time)
{
	if (!mShouldRetry)
	{
		LL_INFOS() << "keep on failing" << LL_ENDL;
		return;
	}
	if (mRetryCount > 0)
	{
		mDelay = llclamp(mDelay*mBackoffFactor,mMinDelay,mMaxDelay);
	}
	// Honor server Retry-After header.
	// Status 503 may ask us to wait for a certain amount of time before retrying.
	F32 wait_time = mDelay;
	if (has_retry_header_time)
	{
		wait_time = retry_header_time;
	}

	if (mRetryCount>=mMaxRetries)
	{
		LL_INFOS() << "Too many retries " << mRetryCount << ", will not retry" << LL_ENDL;
		mShouldRetry = false;
	}
	if (!mRetryOn4xx && !isHttpServerErrorStatus(status))
	{
		LL_INFOS() << "Non-server error " << status << ", will not retry" << LL_ENDL;
		mShouldRetry = false;
	}
	if (mShouldRetry)
	{
		LL_INFOS() << "Retry count " << mRetryCount << " should retry after " << wait_time << LL_ENDL;
		mRetryTimer.reset();
		mRetryTimer.setTimerExpirySec(wait_time);
	}
	mRetryCount++;
}
	

bool LLAdaptiveRetryPolicy::shouldRetry(F32& seconds_to_wait) const
{
	if (mRetryCount == 0)
	{
		// Called shouldRetry before any failure.
		seconds_to_wait = F32_MAX;
		return false;
	}
	seconds_to_wait = mShouldRetry ? (F32) mRetryTimer.getRemainingTimeF32() : F32_MAX;
	return mShouldRetry;
}

// Moved from httpconstants.  Only used by this file.
// Parses 'Retry-After' header contents and returns seconds until retry should occur.
/*static*/
bool LLAdaptiveRetryPolicy::getSecondsUntilRetryAfter(const std::string& retry_after, F32& seconds_to_wait)
{
    // *TODO:  This needs testing!   Not in use yet.
    // Examples of Retry-After headers:
    // Retry-After: Fri, 31 Dec 1999 23:59:59 GMT
    // Retry-After: 120

    // Check for number of seconds version, first:
    char* end = 0;
    // Parse as double
    double seconds = std::strtod(retry_after.c_str(), &end);
    if (end != 0 && *end == 0)
    {
        // Successful parse
        seconds_to_wait = (F32)seconds;
        return true;
    }

    // Parse rfc1123 date.
    time_t date = curl_getdate(retry_after.c_str(), NULL);
    if (-1 == date) return false;

    seconds_to_wait = (F64)date - LLTimer::getTotalSeconds();

    return true;
}

