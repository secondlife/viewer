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
#include "../test/lldoctest.h"

TEST_SUITE("UnknownSuite") {

TEST_CASE("test_1")
{

    LLAdaptiveRetryPolicy never_retry(1.0,1.0,1.0,0);
    LLSD headers;
    F32 wait_seconds;

    // No retry until we've failed a try.
    CHECK_MESSAGE(!never_retry.shouldRetry(wait_seconds, "never retry 0"));

    // 0 retries max.
    never_retry.onFailure(500,headers);
    CHECK_MESSAGE(!never_retry.shouldRetry(wait_seconds, "never retry 1"));

}

TEST_CASE("test_2")
{

    LLSD headers;
    F32 wait_seconds;

    // Normally only retry on server error (5xx)
    LLAdaptiveRetryPolicy noRetry404(1.0,2.0,3.0,10);
    noRetry404.onFailure(404,headers);
    CHECK_MESSAGE(!noRetry404.shouldRetry(wait_seconds, "no retry on 404"));

    // Can retry on 4xx errors if enabled by flag.
    bool do_retry_4xx = true;
    LLAdaptiveRetryPolicy doRetry404(1.0,2.0,3.0,10,do_retry_4xx);
    doRetry404.onFailure(404,headers);
    CHECK_MESSAGE(doRetry404.shouldRetry(wait_seconds, "do retry on 404"));

}

TEST_CASE("test_3")
{

    // Should retry after 1.0, 2.0, 3.0, 3.0 seconds.
    LLAdaptiveRetryPolicy basic_retry(1.0,3.0,2.0,4);
    LLSD headers;
    F32 wait_seconds;
    bool should_retry;
    U32 frac_bits = 6;

    // No retry until we've failed a try.
    CHECK_MESSAGE(!basic_retry.shouldRetry(wait_seconds, "basic_retry 0"));

    // Starting wait 1.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "basic_retry 1");
    ensure_approximately_equals("basic_retry 1", wait_seconds, 1.0F, frac_bits);

    // Double wait to 2.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "basic_retry 2");
    ensure_approximately_equals("basic_retry 2", wait_seconds, 2.0F, frac_bits);

    // Hit max wait of 3.0 (4.0 clamped to max 3)
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "basic_retry 3");
    ensure_approximately_equals("basic_retry 3", wait_seconds, 3.0F, frac_bits);

    // At max wait, should stay at 3.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "basic_retry 4");
    ensure_approximately_equals("basic_retry 4", wait_seconds, 3.0F, frac_bits);

    // Max retries, should fail now.
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(!should_retry, "basic_retry 5");

    // Max retries, should fail now.
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(!should_retry, "basic_retry 5");

    // After a success, should reset to the starting state.
    basic_retry.onSuccess();

    // No retry until we've failed a try.
    CHECK_MESSAGE(!basic_retry.shouldRetry(wait_seconds, "basic_retry 6"));

    // Starting wait 1.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "basic_retry 7");
    ensure_approximately_equals("basic_retry 7", wait_seconds, 1.0F, frac_bits);

    // Double wait to 2.0
    basic_retry.onFailure(500,headers);
    should_retry = basic_retry.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "basic_retry 8");
    ensure_approximately_equals("basic_retry 8", wait_seconds, 2.0F, frac_bits);

}

TEST_CASE("test_4")
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
    CHECK_MESSAGE(should_retry, "killer404 1");
    ensure_approximately_equals("killer404 1", wait_seconds, 1.0F, frac_bits);

    // Double wait to 2.0
    killer404.onFailure(500,headers);
    should_retry = killer404.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "killer404 2");
    ensure_approximately_equals("killer404 2", wait_seconds, 2.0F, frac_bits);

    // Should fail on non-5xx
    killer404.onFailure(404,headers);
    should_retry = killer404.shouldRetry(wait_seconds);
    CHECK_MESSAGE(!should_retry, "killer404 3");

    // After a non-5xx, should keep failing.
    killer404.onFailure(500,headers);
    should_retry = killer404.shouldRetry(wait_seconds);
    CHECK_MESSAGE(!should_retry, "killer404 4");

}

TEST_CASE("test_5")
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
    CHECK_MESSAGE(should_retry, "retry header 1");
    ensure_approximately_equals("retry header 1", wait_seconds, 1.0F, frac_bits);

    policy.onFailure(500,headers_without_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "retry header 2");
    ensure_approximately_equals("retry header 2", wait_seconds, 2.0F, frac_bits);

    policy.onFailure(500,headers_with_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "retry header 3");
    // 4.0 overrides by header -> 666.0
    ensure_approximately_equals("retry header 3", wait_seconds, 666.0F, frac_bits);

    policy.onFailure(500,headers_with_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "retry header 4");
    // 8.0 overrides by header -> 666.0
    ensure_approximately_equals("retry header 4", wait_seconds, 666.0F, frac_bits);

    policy.onFailure(500,headers_without_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "retry header 5");
    ensure_approximately_equals("retry header 5", wait_seconds, 16.0F, frac_bits);

    policy.onFailure(500,headers_without_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    CHECK_MESSAGE(should_retry, "retry header 6");
    ensure_approximately_equals("retry header 6", wait_seconds, 25.0F, frac_bits);

    policy.onFailure(500,headers_with_retry);
    should_retry = policy.shouldRetry(wait_seconds);
    CHECK_MESSAGE(!should_retry, "retry header 7");

}

} // TEST_SUITE
