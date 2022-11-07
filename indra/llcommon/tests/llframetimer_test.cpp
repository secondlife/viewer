/** 
 * @file lltiming_test.cpp
 * @date 2006-07-23
 * @brief Tests the timers.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "../llframetimer.h"
#include "../llsd.h"

#include "../test/lltut.h"

namespace tut
{
    struct frametimer_test
    {
        frametimer_test()
        {
            LLFrameTimer::updateFrameTime();            
        }
    };
    typedef test_group<frametimer_test> frametimer_group_t;
    typedef frametimer_group_t::object frametimer_object_t;
    tut::frametimer_group_t frametimer_instance("LLFrameTimer");

    template<> template<>
    void frametimer_object_t::test<1>()
    {
        F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        LLFrameTimer timer;
        timer.setExpiryAt(seconds_since_epoch);
        F64 expires_at = timer.expiresAt();
        ensure_distance(
            "set expiry matches get expiry",
            expires_at,
            seconds_since_epoch,
            0.001);
    }

    template<> template<>
    void frametimer_object_t::test<2>()
    {
        F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        seconds_since_epoch += 10.0;
        LLFrameTimer timer;
        timer.setExpiryAt(seconds_since_epoch);
        F64 expires_at = timer.expiresAt();
        ensure_distance(
            "set expiry matches get expiry 1",
            expires_at,
            seconds_since_epoch,
            0.001);
        seconds_since_epoch += 10.0;
        timer.setExpiryAt(seconds_since_epoch);
        expires_at = timer.expiresAt();
        ensure_distance(
            "set expiry matches get expiry 2",
            expires_at,
            seconds_since_epoch,
            0.001);
    }
    template<> template<>
    void frametimer_object_t::test<3>()
    {
        clock_t t1 = clock();
        ms_sleep(200);
        clock_t t2 = clock();
        clock_t elapsed = t2 - t1 + 1;
        std::cout << "Note: using clock(), ms_sleep() actually took " << (long)elapsed << "ms" << std::endl;

        F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
        seconds_since_epoch += 2.0;
        LLFrameTimer timer;
        timer.setExpiryAt(seconds_since_epoch);
        /*
         * Note that the ms_sleep(200) below is only guaranteed to return
         * in 200ms _or_more_, so it should be true that by the 10th
         * iteration we've gotten to the 2 seconds requested above
         * and the timer should expire, but it can expire in fewer iterations
         * if one or more of the ms_sleep calls takes longer.
         * (as it did when we moved to Mac OS X 10.10)
         */
        int iterations_until_expiration = 0;
        while ( !timer.hasExpired() )
        {
            ms_sleep(200);
            LLFrameTimer::updateFrameTime();
            iterations_until_expiration++;
        }
        ensure("timer took too long to expire", iterations_until_expiration <= 10);
    }
    
/*
    template<> template<>
    void frametimer_object_t::test<4>()
    {
    }
*/
}
