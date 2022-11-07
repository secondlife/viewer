/** 
 * @file file llhttpretrypolicy.h
 * @brief declarations for http retry policy class.
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
                                        
#ifndef LL_RETRYPOLICY_H
#define LL_RETRYPOLICY_H

#include "lltimer.h"
#include "llthread.h"

#include "llhttpconstants.h"

// For compatibility with new core http lib.
#include "httpresponse.h"
#include "httpheaders.h"

// This is intended for use with HTTP Clients/Responders, but is not
// specifically coupled with those classes.
class LLHTTPRetryPolicy: public LLThreadSafeRefCount
{
public:
    LLHTTPRetryPolicy() {}

    virtual ~LLHTTPRetryPolicy() {}
    // Call after a sucess to reset retry state.

    virtual void onSuccess() = 0;
    // Call once after an HTTP failure to update state.
    virtual void onFailure(S32 status, const LLSD& headers) = 0;

    virtual void onFailure(const LLCore::HttpResponse *response) = 0;

    virtual bool shouldRetry(F32& seconds_to_wait) const = 0;

    virtual void reset() = 0;
};

// Very general policy with geometric back-off after failures,
// up to a maximum delay, and maximum number of retries.
class LLAdaptiveRetryPolicy: public LLHTTPRetryPolicy
{
public:
    LLAdaptiveRetryPolicy(F32 min_delay, F32 max_delay, F32 backoff_factor, U32 max_retries, bool retry_on_4xx = false);

    // virtual
    void onSuccess();

    void reset();
    
    // virtual
    void onFailure(S32 status, const LLSD& headers);
    // virtual
    void onFailure(const LLCore::HttpResponse *response);
    // virtual
    bool shouldRetry(F32& seconds_to_wait) const;

    static bool getSecondsUntilRetryAfter(const std::string& retry_after, F32& seconds_to_wait);

protected:
    void init();
    bool getRetryAfter(const LLSD& headers, F32& retry_header_time);
    bool getRetryAfter(const LLCore::HttpHeaders::ptr_t &headers, F32& retry_header_time);
    void onFailureCommon(S32 status, bool has_retry_header_time, F32 retry_header_time);

private:

    const F32 mMinDelay; // delay never less than this value
    const F32 mMaxDelay; // delay never exceeds this value
    const F32 mBackoffFactor; // delay increases by this factor after each retry, up to mMaxDelay.
    const U32 mMaxRetries; // maximum number of times shouldRetry will return true.
    F32 mDelay; // current default delay.
    U32 mRetryCount; // number of times shouldRetry has been called.
    LLTimer mRetryTimer; // time until next retry.
    bool mShouldRetry; // Becomes false after too many retries, or the wrong sort of status received, etc.
    bool mRetryOn4xx; // Normally only retry on 5xx server errors.
};

#endif
