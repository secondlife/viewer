/** 
 * @file lldeadmantimer_test.cpp
 * @brief Tests for the LLDeadmanTimer class.
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

#include "../lldeadmantimer.h"
#include "../llsd.h"
#include "../lltimer.h"

#include "../test/lltut.h"

// Convert between floating point time deltas and U64 time deltas.
// Reflects an implementation detail inside lldeadmantimer.cpp

static LLDeadmanTimer::time_type float_time_to_u64(F64 delta)
{
    return LLDeadmanTimer::time_type(delta * get_timer_info().mClockFrequency);
}

static F64 u64_time_to_float(LLDeadmanTimer::time_type delta)
{
    return delta * get_timer_info().mClockFrequencyInv;
}


namespace tut
{

struct deadmantimer_test
{
    deadmantimer_test()
        {
            // LLTimer internals updating
            get_timer_info().update();
        }
};

typedef test_group<deadmantimer_test> deadmantimer_group_t;
typedef deadmantimer_group_t::object deadmantimer_object_t;
tut::deadmantimer_group_t deadmantimer_instance("LLDeadmanTimer");

// Basic construction test and isExpired() call
template<> template<>
void deadmantimer_object_t::test<1>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(10.0, false);

        ensure_equals("WOCM isExpired() returns false after ctor()", timer.isExpired(0, started, stopped, count), false);
        ensure_approximately_equals("WOCM t1 - isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WOCM t1 - isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("WOCM t1 - isExpired() does not modify count", count, U64L(8));
    }

    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(10.0, true);

        ensure_equals("WCM isExpired() returns false after ctor()", timer.isExpired(0, started, stopped, count, user_cpu, sys_cpu), false);
        ensure_approximately_equals("WCM t1 - isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WCM t1 - isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("WCM t1 - isExpired() does not modify count", count, U64L(8));
        ensure_equals("WCM t1 - isExpired() does not modify user_cpu", user_cpu, U64L(29000));
        ensure_equals("WCM t1 - isExpired() does not modify sys_cpu", sys_cpu, U64L(57000));
    }
}


// Construct with zero horizon - not useful generally but will be useful in testing
template<> template<>
void deadmantimer_object_t::test<2>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(0.0, false);           // Zero is pre-expired
        
        ensure_equals("WOCM isExpired() still returns false with 0.0 time ctor()",
                      timer.isExpired(0, started, stopped, count), false);
    }

    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(0.0, true);            // Zero is pre-expired
        
        ensure_equals("WCM isExpired() still returns false with 0.0 time ctor()",
                      timer.isExpired(0, started, stopped, count, user_cpu, sys_cpu), false);
    }
}


// "pre-expired" timer - starting a timer with a 0.0 horizon will result in
// expiration on first test.
template<> template<>
void deadmantimer_object_t::test<3>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(0.0, false);

        timer.start(0);
        ensure_equals("WOCM isExpired() returns true with 0.0 horizon time",
                      timer.isExpired(0, started, stopped, count), true);
        ensure_approximately_equals("WOCM expired timer with no bell ringing has stopped == started", started, stopped, 8);
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(0.0, true);

        timer.start(0);
        ensure_equals("WCM isExpired() returns true with 0.0 horizon time",
                      timer.isExpired(0, started, stopped, count, user_cpu, sys_cpu), true);
        ensure_approximately_equals("WCM expired timer with no bell ringing has stopped == started", started, stopped, 8);
    }
}


// "pre-expired" timer - bell rings are ignored as we're already expired.
template<> template<>
void deadmantimer_object_t::test<4>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(0.0, false);
    
        timer.start(0);
        timer.ringBell(LLDeadmanTimer::getNow() + float_time_to_u64(1000.0), 1);
        ensure_equals("WOCM isExpired() returns true with 0.0 horizon time after bell ring",
                      timer.isExpired(0, started, stopped, count), true);
        ensure_approximately_equals("WOCM ringBell has no impact on expired timer leaving stopped == started", started, stopped, 8);
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(0.0, true);
    
        timer.start(0);
        timer.ringBell(LLDeadmanTimer::getNow() + float_time_to_u64(1000.0), 1);
        ensure_equals("WCM isExpired() returns true with 0.0 horizon time after bell ring",
                      timer.isExpired(0, started, stopped, count, user_cpu, sys_cpu), true);
        ensure_approximately_equals("WCM ringBell has no impact on expired timer leaving stopped == started", started, stopped, 8);
    }
}


// start(0) test - unexpired timer reports unexpired
template<> template<>
void deadmantimer_object_t::test<5>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(10.0, false);
    
        timer.start(0);
        ensure_equals("WOCM isExpired() returns false after starting with 10.0 horizon time",
                      timer.isExpired(0, started, stopped, count), false);
        ensure_approximately_equals("WOCM t5 - isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WOCM t5 - isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("WOCM t5 - isExpired() does not modify count", count, U64L(8));
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(10.0, true);
    
        timer.start(0);
        ensure_equals("WCM isExpired() returns false after starting with 10.0 horizon time",
                      timer.isExpired(0, started, stopped, count, user_cpu, sys_cpu), false);
        ensure_approximately_equals("WCM t5 - isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WCM t5 - isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("WCM t5 - isExpired() does not modify count", count, U64L(8));
        ensure_equals("WCM t5 - isExpired() does not modify user_cpu", user_cpu, U64L(29000));
        ensure_equals("WCM t5 - isExpired() does not modify sys_cpu", sys_cpu, U64L(57000));
    }
}


// start() test - start in the past but not beyond 1 horizon
template<> template<>
void deadmantimer_object_t::test<6>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(10.0, false);

        // Would like to do subtraction on current time but can't because
        // the implementation on Windows is zero-based.  We wrap around
        // the backside resulting in a large U64 number.
    
        LLDeadmanTimer::time_type the_past(LLDeadmanTimer::getNow());
        LLDeadmanTimer::time_type now(the_past + float_time_to_u64(5.0));
        timer.start(the_past);
        ensure_equals("WOCM t6 - isExpired() returns false with 10.0 horizon time starting 5.0 in past",
                      timer.isExpired(now, started, stopped, count), false);
        ensure_approximately_equals("WOCM t6 - isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WOCM t6 - isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("WOCM t6 - isExpired() does not modify count", count, U64L(8));
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(10.0, true);

        // Would like to do subtraction on current time but can't because
        // the implementation on Windows is zero-based.  We wrap around
        // the backside resulting in a large U64 number.
    
        LLDeadmanTimer::time_type the_past(LLDeadmanTimer::getNow());
        LLDeadmanTimer::time_type now(the_past + float_time_to_u64(5.0));
        timer.start(the_past);
        ensure_equals("WCM t6 - isExpired() returns false with 10.0 horizon time starting 5.0 in past",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), false);
        ensure_approximately_equals("WCM t6 - isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WCM t6 - isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("t6 - isExpired() does not modify count", count, U64L(8));
        ensure_equals("WCM t6 - isExpired() does not modify user_cpu", user_cpu, U64L(29000));
        ensure_equals("WCM t6 - isExpired() does not modify sys_cpu", sys_cpu, U64L(57000));
    }
}


// start() test - start in the past but well beyond 1 horizon
template<> template<>
void deadmantimer_object_t::test<7>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(10.0, false);
        
        // Would like to do subtraction on current time but can't because
        // the implementation on Windows is zero-based.  We wrap around
        // the backside resulting in a large U64 number.
    
        LLDeadmanTimer::time_type the_past(LLDeadmanTimer::getNow());
        LLDeadmanTimer::time_type now(the_past + float_time_to_u64(20.0));
        timer.start(the_past);
        ensure_equals("WOCM t7 - isExpired() returns true with 10.0 horizon time starting 20.0 in past",
                      timer.isExpired(now,started, stopped, count), true);
        ensure_approximately_equals("WOCM t7 - starting before horizon still gives equal started / stopped", started, stopped, 8);
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(10.0, true);
        
        // Would like to do subtraction on current time but can't because
        // the implementation on Windows is zero-based.  We wrap around
        // the backside resulting in a large U64 number.
    
        LLDeadmanTimer::time_type the_past(LLDeadmanTimer::getNow());
        LLDeadmanTimer::time_type now(the_past + float_time_to_u64(20.0));
        timer.start(the_past);
        ensure_equals("WCM t7 - isExpired() returns true with 10.0 horizon time starting 20.0 in past",
                      timer.isExpired(now,started, stopped, count, user_cpu, sys_cpu), true);
        ensure_approximately_equals("WOCM t7 - starting before horizon still gives equal started / stopped", started, stopped, 8);
    }
}


// isExpired() test - results are read-once.  Probes after first true are false.
template<> template<>
void deadmantimer_object_t::test<8>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(10.0, false);

        // Would like to do subtraction on current time but can't because
        // the implementation on Windows is zero-based.  We wrap around
        // the backside resulting in a large U64 number.
    
        LLDeadmanTimer::time_type the_past(LLDeadmanTimer::getNow());
        LLDeadmanTimer::time_type now(the_past + float_time_to_u64(20.0));
        timer.start(the_past);
        ensure_equals("WOCM t8 - isExpired() returns true with 10.0 horizon time starting 20.0 in past",
                      timer.isExpired(now, started, stopped, count), true);
        
        started = 42.0;
        stopped = 97.0;
        count = U64L(8);
        ensure_equals("WOCM t8 - second isExpired() returns false after true",
                      timer.isExpired(now, started, stopped, count), false);
        ensure_approximately_equals("WOCM t8 - 2nd isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WOCM t8 - 2nd isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("WOCM t8 - 2nd isExpired() does not modify count", count, U64L(8));
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(10.0, true);

        // Would like to do subtraction on current time but can't because
        // the implementation on Windows is zero-based.  We wrap around
        // the backside resulting in a large U64 number.
    
        LLDeadmanTimer::time_type the_past(LLDeadmanTimer::getNow());
        LLDeadmanTimer::time_type now(the_past + float_time_to_u64(20.0));
        timer.start(the_past);
        ensure_equals("WCM t8 - isExpired() returns true with 10.0 horizon time starting 20.0 in past",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), true);
        
        started = 42.0;
        stopped = 97.0;
        count = U64L(8);
        user_cpu = 29000;
        sys_cpu = 57000;
        ensure_equals("WCM t8 - second isExpired() returns false after true",
                      timer.isExpired(now, started, stopped, count), false);
        ensure_approximately_equals("WCM t8 - 2nd isExpired() does not modify started", started, F64(42.0), 2);
        ensure_approximately_equals("WCM t8 - 2nd isExpired() does not modify stopped", stopped, F64(97.0), 2);
        ensure_equals("WCM t8 - 2nd isExpired() does not modify count", count, U64L(8));
        ensure_equals("WCM t8 - 2nd isExpired() does not modify user_cpu", user_cpu, U64L(29000));
        ensure_equals("WCM t8 - 2nd isExpired() does not modify sys_cpu", sys_cpu, U64L(57000));
    }
}


// ringBell() test - see that we can keep a timer from expiring
template<> template<>
void deadmantimer_object_t::test<9>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(5.0, false);

        LLDeadmanTimer::time_type now(LLDeadmanTimer::getNow());
        F64 real_start(u64_time_to_float(now));
        timer.start(0);

        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        ensure_equals("WOCM t9 - 5.0 horizon timer has not timed out after 10 1-second bell rings",
                      timer.isExpired(now, started, stopped, count), false);
        F64 last_good_ring(u64_time_to_float(now));

        // Jump forward and expire
        now += float_time_to_u64(10.0);
        ensure_equals("WOCM t9 - 5.0 horizon timer expires on 10-second jump",
                      timer.isExpired(now, started, stopped, count), true);
        ensure_approximately_equals("WOCM t9 - started matches start() time", started, real_start, 4);
        ensure_approximately_equals("WOCM t9 - stopped matches last ringBell() time", stopped, last_good_ring, 4);
        ensure_equals("WOCM t9 - 10 good ringBell()s", count, U64L(10));
        ensure_equals("WOCM t9 - single read only", timer.isExpired(now, started, stopped, count), false);
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);
        LLDeadmanTimer timer(5.0, true);

        LLDeadmanTimer::time_type now(LLDeadmanTimer::getNow());
        F64 real_start(u64_time_to_float(now));
        timer.start(0);

        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        ensure_equals("WCM t9 - 5.0 horizon timer has not timed out after 10 1-second bell rings",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), false);
        F64 last_good_ring(u64_time_to_float(now));

        // Jump forward and expire
        now += float_time_to_u64(10.0);
        ensure_equals("WCM t9 - 5.0 horizon timer expires on 10-second jump",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), true);
        ensure_approximately_equals("WCM t9 - started matches start() time", started, real_start, 4);
        ensure_approximately_equals("WCM t9 - stopped matches last ringBell() time", stopped, last_good_ring, 4);
        ensure_equals("WCM t9 - 10 good ringBell()s", count, U64L(10));
        ensure_equals("WCM t9 - single read only", timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), false);
    }
}


// restart after expiration test - verify that restarts behave well
template<> template<>
void deadmantimer_object_t::test<10>()
{
    {
        // Without cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8));
        LLDeadmanTimer timer(5.0, false);

        LLDeadmanTimer::time_type now(LLDeadmanTimer::getNow());
        F64 real_start(u64_time_to_float(now));
        timer.start(0);

        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        ensure_equals("WOCM t10 - 5.0 horizon timer has not timed out after 10 1-second bell rings",
                      timer.isExpired(now, started, stopped, count), false);
        F64 last_good_ring(u64_time_to_float(now));

        // Jump forward and expire
        now += float_time_to_u64(10.0);
        ensure_equals("WOCM t10 - 5.0 horizon timer expires on 10-second jump",
                      timer.isExpired(now, started, stopped, count), true);
        ensure_approximately_equals("WOCM t10 - started matches start() time", started, real_start, 4);
        ensure_approximately_equals("WOCM t10 - stopped matches last ringBell() time", stopped, last_good_ring, 4);
        ensure_equals("WOCM t10 - 10 good ringBell()s", count, U64L(10));
        ensure_equals("WOCM t10 - single read only", timer.isExpired(now, started, stopped, count), false);
        
        // Jump forward and restart
        now += float_time_to_u64(1.0);
        real_start = u64_time_to_float(now);
        timer.start(now);
        
        // Run a modified bell ring sequence
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        ensure_equals("WOCM t10 - 5.0 horizon timer has not timed out after 8 1-second bell rings",
                      timer.isExpired(now, started, stopped, count), false);
        last_good_ring = u64_time_to_float(now);

        // Jump forward and expire
        now += float_time_to_u64(10.0);
        ensure_equals("WOCM t10 - 5.0 horizon timer expires on 8-second jump",
                      timer.isExpired(now, started, stopped, count), true);
        ensure_approximately_equals("WOCM t10 - 2nd started matches start() time", started, real_start, 4);
        ensure_approximately_equals("WOCM t10 - 2nd stopped matches last ringBell() time", stopped, last_good_ring, 4);
        ensure_equals("WOCM t10 - 8 good ringBell()s", count, U64L(8));
        ensure_equals("WOCM t10 - single read only - 2nd start",
                      timer.isExpired(now, started, stopped, count), false);
    }
    {
        // With cpu metrics
        F64 started(42.0), stopped(97.0);
        U64 count(U64L(8)), user_cpu(29000), sys_cpu(57000);

        LLDeadmanTimer timer(5.0, true);

        LLDeadmanTimer::time_type now(LLDeadmanTimer::getNow());
        F64 real_start(u64_time_to_float(now));
        timer.start(0);

        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        ensure_equals("WCM t10 - 5.0 horizon timer has not timed out after 10 1-second bell rings",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), false);
        F64 last_good_ring(u64_time_to_float(now));

        // Jump forward and expire
        now += float_time_to_u64(10.0);
        ensure_equals("WCM t10 - 5.0 horizon timer expires on 10-second jump",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), true);
        ensure_approximately_equals("WCM t10 - started matches start() time", started, real_start, 4);
        ensure_approximately_equals("WCM t10 - stopped matches last ringBell() time", stopped, last_good_ring, 4);
        ensure_equals("WCM t10 - 10 good ringBell()s", count, U64L(10));
        ensure_equals("WCM t10 - single read only", timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), false);
        
        // Jump forward and restart
        now += float_time_to_u64(1.0);
        real_start = u64_time_to_float(now);
        timer.start(now);
        
        // Run a modified bell ring sequence
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        now += float_time_to_u64(1.0);
        timer.ringBell(now, 1);
        ensure_equals("WCM t10 - 5.0 horizon timer has not timed out after 8 1-second bell rings",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), false);
        last_good_ring = u64_time_to_float(now);

        // Jump forward and expire
        now += float_time_to_u64(10.0);
        ensure_equals("WCM t10 - 5.0 horizon timer expires on 8-second jump",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), true);
        ensure_approximately_equals("WCM t10 - 2nd started matches start() time", started, real_start, 4);
        ensure_approximately_equals("WCM t10 - 2nd stopped matches last ringBell() time", stopped, last_good_ring, 4);
        ensure_equals("WCM t10 - 8 good ringBell()s", count, U64L(8));
        ensure_equals("WCM t10 - single read only - 2nd start",
                      timer.isExpired(now, started, stopped, count, user_cpu, sys_cpu), false);
    }
}



} // end namespace tut
