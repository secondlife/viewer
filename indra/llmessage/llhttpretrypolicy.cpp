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

#include "linden_common.h"
#include "llhttpretrypolicy.h"

void LLAdaptiveRetryPolicy::onFailure(S32 status, const LLSD& headers)
{
	if (mRetryCount > 0)
	{
		mDelay = llclamp(mDelay*mBackoffFactor,mMinDelay,mMaxDelay);
	}
	// Honor server Retry-After header.
	// Status 503 may ask us to wait for a certain amount of time before retrying.
	F32 wait_time = mDelay;
	F32 retry_header_time;
	if (headers.has(HTTP_IN_HEADER_RETRY_AFTER)
		&& getSecondsUntilRetryAfter(headers[HTTP_IN_HEADER_RETRY_AFTER].asStringRef(), retry_header_time))
	{
		wait_time = retry_header_time;
	}

	if (mRetryCount>=mMaxRetries)
	{
		llinfos << "Too many retries " << mRetryCount << ", will not retry" << llendl;
		mShouldRetry = false;
	}
	if (!isHttpServerErrorStatus(status))
	{
		llinfos << "Non-server error " << status << ", will not retry" << llendl;
		mShouldRetry = false;
	}
	if (mShouldRetry)
	{
		llinfos << "Retry count " << mRetryCount << " should retry after " << wait_time << llendl;
		mRetryTimer.reset();
		mRetryTimer.setTimerExpirySec(wait_time);
	}
	mRetryCount++;
}
	

bool LLAdaptiveRetryPolicy::shouldRetry(F32& seconds_to_wait) const
{
	llassert(mRetryCount>0); // have to call onFailure() before shouldRetry()
	seconds_to_wait = mShouldRetry ? mRetryTimer.getRemainingTimeF32() : F32_MAX;
	return mShouldRetry;
}
