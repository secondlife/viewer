/**
 * @file llframetimer_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL frametimer
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

// ---------------------------------------------------------------------------
// Auto-generated from llframetimer_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "../llframetimer.h"
#include "../llsd.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llframetimer_test::frametimer_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert llframetimer_test.cpp::frametimer_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void frametimer_object_t::test<1>()
        //     {
        //         F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        //         LLFrameTimer timer;
        //         timer.setExpiryAt(seconds_since_epoch);
        //         F64 expires_at = timer.expiresAt();
        //         ensure_distance(
        //             "set expiry matches get expiry",
        //             expires_at,
        //             seconds_since_epoch,
        //             0.001);
        //     }
    }

    TUT_CASE("llframetimer_test::frametimer_object_t_test_2")
    {
        DOCTEST_FAIL("TODO: convert llframetimer_test.cpp::frametimer_object_t::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void frametimer_object_t::test<2>()
        //     {
        //         F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        //         seconds_since_epoch += 10.0;
        //         LLFrameTimer timer;
        //         timer.setExpiryAt(seconds_since_epoch);
        //         F64 expires_at = timer.expiresAt();
        //         ensure_distance(
        //             "set expiry matches get expiry 1",
        //             expires_at,
        //             seconds_since_epoch,
        //             0.001);
        //         seconds_since_epoch += 10.0;
        //         timer.setExpiryAt(seconds_since_epoch);
        //         expires_at = timer.expiresAt();
        //         ensure_distance(
        //             "set expiry matches get expiry 2",
        //             expires_at,
        //             seconds_since_epoch,
        //             0.001);
        //     }
    }

    TUT_CASE("llframetimer_test::frametimer_object_t_test_3")
    {
        DOCTEST_FAIL("TODO: convert llframetimer_test.cpp::frametimer_object_t::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void frametimer_object_t::test<3>()
        //     {
        //         clock_t t1 = clock();
        //         ms_sleep(200);
        //         clock_t t2 = clock();
        //         clock_t elapsed = t2 - t1 + 1;
        //         std::cout << "Note: using clock(), ms_sleep() actually took " << (long)elapsed << "ms" << std::endl;

        //         F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        //         seconds_since_epoch += 2.0;
        //         LLFrameTimer timer;
        //         timer.setExpiryAt(seconds_since_epoch);
        //         /*
        //          * Note that the ms_sleep(200) below is only guaranteed to return
        //          * in 200ms _or_more_, so it should be true that by the 10th
        //          * iteration we've gotten to the 2 seconds requested above
        //          * and the timer should expire, but it can expire in fewer iterations
        //          * if one or more of the ms_sleep calls takes longer.
        //          * (as it did when we moved to Mac OS X 10.10)
        //          */
        //         int iterations_until_expiration = 0;
        //         while ( !timer.hasExpired() )
        //         {
        //             ms_sleep(200);
        //             LLFrameTimer::updateFrameTime();
        //             iterations_until_expiration++;
        //         }
        //         ensure("timer took too long to expire", iterations_until_expiration <= 10);
        //     }
    }

    TUT_CASE("llframetimer_test::frametimer_object_t_test_4")
    {
        DOCTEST_FAIL("TODO: convert llframetimer_test.cpp::frametimer_object_t::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void frametimer_object_t::test<4>()
        //     {
        //     }
    }

}

