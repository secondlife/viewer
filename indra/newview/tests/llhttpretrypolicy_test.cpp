/** 
 * @file llhttpretrypolicy_test.cpp
 * @brief Header tests to exercise the LLHTTPRetryPolicy classes.
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

#include "../llviewerprecompiledheaders.h"
#include "../llhttpretrypolicy.h"
#include "lltut.h"

namespace tut
{
struct TestData
{
};

typedef test_group<TestData>    RetryPolicyTestGroup;
typedef RetryPolicyTestGroup::object        RetryPolicyTestObject;
RetryPolicyTestGroup retryPolicyTestGroup("retry_policy");

template<> template<>
void RetryPolicyTestObject::test<1>()
{
    LLAdaptiveRetryPolicy never_retry(1.0,1.0,1.0,0);
    LLSD headers;
    F32 wait_seconds;
    
    // No retry until we've failed a try.
    ensure("never retry 0", !never_retry.shouldRetry(wait_seconds));

    // 0 retries max.
    never_retry.onFailure(500,headers);
    ensure("never retry 1", !never_retry.shouldRetry(wait_seconds)); 
}

template<> template<>
void RetryPolicyTestObject::test<2>()
{
    LLSD headers;
    F32 wait_seconds;

    // Normally only retry on server error (5xx)
    LLAdaptiveRetryPolicy noRetry404(1.0,2.0,3.0,10);
    noRetry404.onFailure(404,headers);
    ensure("no retry on 404", !noRetry404.shouldRetry(wait_seconds)); 

    // Can retry on 4xx errors if enabled by flag.
    bool do_retry_4xx = true;
    LLAdaptiveRetryPolicy doRetry404(1.0,2.0,3.0,10,do_retry_4xx);
    doRetry404.onFailure(404,headers);
    ensure("do retry on 404", doRetry404.shouldRetry(wait_seconds)); 
}

template<> template<>
void RetryPolicyTestObject::test<3>()
{
    // Should retry after 1.0, 2.0, 3.0, 3.0 seconds.
    LLAdaptiveRetryPolicy basic_retry(1.0,3.0,2.0,4);
    LLSD headers;
    F32 wait_seconds;
    bool should_retry;
    U32 frac_bits = 6;

    // No retry until we've failed a try.
    ensure("basic_retry 0", !basic_retry.shouldRetry(wait_seconds));

    // Starting wait 1.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 1", should_retry);
    ensure_approximately_equals("basic_retry 1", wait_seconds, 1.0F, frac_bits);

    // Double wait to 2.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 2", should_retry);
    ensure_approximately_equals("basic_retry 2", wait_seconds, 2.0F, frac_bits);

    // Hit max wait of 3.0 (4.0 clamped to max 3)
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 3", should_retry);
    ensure_approximately_equals("basic_retry 3", wait_seconds, 3.0F, frac_bits);

    // At max wait, should stay at 3.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 4", should_retry);
    ensure_approximately_equals("basic_retry 4", wait_seconds, 3.0F, frac_bits);

    // Max retries, should fail now.
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 5", !should_retry);

    // Max retries, should fail now.
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 5", !should_retry);

    // After a success, should reset to the starting state.
    basic_retry.onSuccess();

    // No retry until we've failed a try.
    ensure("basic_retry 6", !basic_retry.shouldRetry(wait_seconds));

    // Starting wait 1.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 7", should_retry);
    ensure_approximately_equals("basic_retry 7", wait_seconds, 1.0F, frac_bits);

    // Double wait to 2.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    ensure("basic_retry 8", should_retry);
    ensure_approximately_equals("basic_retry 8", wait_seconds, 2.0F, frac_bits);
}

// Retries should stop as soon as a non-5xx error is received.
template<> template<>
void RetryPolicyTestObject::test<4>()
{
    // Should retry after 1.0, 2.0, 3.0, 3.0 seconds.
    LLAdaptiveRetryPolicy killer404(1.0,3.0,2.0,4);
    LLSD headers;
    F32 wait_seconds;
    bool should_retry;
    U32 frac_bits = 6;

    // Starting wait 1.0
    killer404.onFailure(500,headers);
    should_retry = killer404.shouldRetry(wait_seconds);
    ensure("killer404 1", should_retry);
    ensure_approximately_equals("killer404 1", wait_seconds, 1.0F, frac_bits);

    // Double wait to 2.0
    killer404.onFailure(500,headers);
    should_retry = killer404.shouldRetry(wait_seconds);
    ensure("killer404 2", should_retry);
    ensure_approximately_equals("killer404 2", wait_seconds, 2.0F, frac_bits);

    // Should fail on non-5xx
    killer404.onFailure(404,headers);
    should_retry = killer404.shouldRetry(wait_seconds);
    ensure("killer404 3", !should_retry);

    // After a non-5xx, should keep failing.
    killer404.onFailure(500,headers);
    should_retry = killer404.shouldRetry(wait_seconds);
    ensure("killer404 4", !should_retry);
}

// Test handling of "retry-after" header. If present, this header
// value overrides the computed delay, but does not affect the
// progression of delay values.  For example, if the normal
// progression of delays would be 1,2,4,8..., but the 2nd and 3rd calls
// get a retry header of 33, the pattern would become 1,33,33,8...
template<> template<>
void RetryPolicyTestObject::test<5>()
{
    LLAdaptiveRetryPolicy policy(1.0,25.0,2.0,6);
    LLSD headers_with_retry;
    headers_with_retry[HTTP_IN_HEADER_RETRY_AFTER] = "666";
    LLSD headers_without_retry;
    F32 wait_seconds;
    bool should_retry;
    U32 frac_bits = 6;

    policy.onFailure(500,headers_without_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    ensure("retry header 1", should_retry);
    ensure_approximately_equals("retry header 1", wait_seconds, 1.0F, frac_bits);

    policy.onFailure(500,headers_without_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    ensure("retry header 2", should_retry);
    ensure_approximately_equals("retry header 2", wait_seconds, 2.0F, frac_bits);

    policy.onFailure(500,headers_with_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    ensure("retry header 3", should_retry);
    // 4.0 overrides by header -> 666.0
    ensure_approximately_equals("retry header 3", wait_seconds, 666.0F, frac_bits);

    policy.onFailure(500,headers_with_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    ensure("retry header 4", should_retry);
    // 8.0 overrides by header -> 666.0
    ensure_approximately_equals("retry header 4", wait_seconds, 666.0F, frac_bits);

    policy.onFailure(500,headers_without_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    ensure("retry header 5", should_retry);
    ensure_approximately_equals("retry header 5", wait_seconds, 16.0F, frac_bits);

    policy.onFailure(500,headers_without_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    ensure("retry header 6", should_retry);
    ensure_approximately_equals("retry header 6", wait_seconds, 25.0F, frac_bits);

    policy.onFailure(500,headers_with_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    ensure("retry header 7", !should_retry);
}

// Test getSecondsUntilRetryAfter(const std::string& retry_after, F32& seconds_to_wait),
// used by header parsing of the retry policy.
template<> template<>
void RetryPolicyTestObject::test<6>()
{
    F32 seconds_to_wait;
    bool success;

    std::string str1("0");
    seconds_to_wait = F32_MAX;
    success = LLAdaptiveRetryPolicy::getSecondsUntilRetryAfter(str1, seconds_to_wait);
    ensure("parse 1", success);
    ensure_equals("parse 1", seconds_to_wait, 0.0);

    std::string str2("999.9");
    seconds_to_wait = F32_MAX;
    success = LLAdaptiveRetryPolicy::getSecondsUntilRetryAfter(str2, seconds_to_wait);
    ensure("parse 2", success);
    ensure_approximately_equals("parse 2", seconds_to_wait, 999.9F, 8);

    time_t nowseconds;
    time(&nowseconds);
    std::string str3 = LLDate((F64)(nowseconds+44)).asRFC1123();
    seconds_to_wait = F32_MAX;
    success = LLAdaptiveRetryPolicy::getSecondsUntilRetryAfter(str3, seconds_to_wait);
    std::cerr << " str3 [" << str3 << "]" << std::endl;
    ensure("parse 3", success);
    ensure_approximately_equals_range("parse 3", seconds_to_wait, 44.0F, 2.0F);
}

// Test retry-after field in both llmessage and CoreHttp headers.
template<> template<>
void RetryPolicyTestObject::test<7>()
{
    std::cerr << "7 starts" << std::endl;
    
    LLSD sd_headers;
    time_t nowseconds;
    time(&nowseconds);
    LLAdaptiveRetryPolicy policy(17.0,644.0,3.0,5);
    F32 seconds_to_wait;
    bool should_retry;

    // No retry until we've failed a try.
    ensure("header 0", !policy.shouldRetry(seconds_to_wait));
    
    // no retry header, use default.
    policy.onFailure(500,LLSD());
    should_retry = policy.shouldRetry(seconds_to_wait);
    ensure("header 1", should_retry);
    ensure_approximately_equals("header 1", seconds_to_wait, 17.0F, 6);

    // retry header should override, give delay of 0
    std::string date_string = LLDate((F64)(nowseconds+7)).asRFC1123();
    sd_headers[HTTP_IN_HEADER_RETRY_AFTER] = date_string;
    policy.onFailure(503,sd_headers);
    should_retry = policy.shouldRetry(seconds_to_wait);
    ensure("header 2", should_retry);
    ensure_approximately_equals_range("header 2", seconds_to_wait, 7.0F, 2.0F);

    LLCore::HttpResponse *response;
    LLCore::HttpHeaders::ptr_t headers;

    response = new LLCore::HttpResponse();
    headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());
    response->setStatus(503);
    response->setHeaders(headers);
    headers->append(HTTP_IN_HEADER_RETRY_AFTER, std::string("600"));
    policy.onFailure(response);
    should_retry = policy.shouldRetry(seconds_to_wait);
    ensure("header 3",should_retry);
    ensure_approximately_equals("header 3", seconds_to_wait, 600.0F, 6);
    response->release();

    response = new LLCore::HttpResponse();
    headers = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());
    response->setStatus(503);
    response->setHeaders(headers);
    time(&nowseconds);
    date_string = LLDate((F64)(nowseconds+77)).asRFC1123();
    std::cerr << "date_string [" << date_string << "]" << std::endl;
    headers->append(HTTP_IN_HEADER_RETRY_AFTER,date_string);
    policy.onFailure(response);
    should_retry = policy.shouldRetry(seconds_to_wait);
    ensure("header 4",should_retry);
    ensure_approximately_equals_range("header 4", seconds_to_wait, 77.0F, 2.0F);
    response->release();

    // Timeout should be clamped at max.
    policy.onFailure(500,LLSD());
    should_retry = policy.shouldRetry(seconds_to_wait);
    ensure("header 5", should_retry);
    ensure_approximately_equals("header 5", seconds_to_wait, 644.0F, 6);

    // No more retries.
    policy.onFailure(500,LLSD());
    should_retry = policy.shouldRetry(seconds_to_wait);
    ensure("header 6", !should_retry);
}

}

