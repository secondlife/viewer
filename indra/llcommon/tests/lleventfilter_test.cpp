/**
 * @file   lleventfilter_test.cpp
 * @author Nat Goodspeed
 * @date   2009-03-06
 * @brief  Test for lleventfilter.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventfilter.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "stringize.h"
#include "llsdutil.h"
#include "listener.h"
#include "tests/wrapllerrs.h"

#include <typeinfo>

/*****************************************************************************
*   Test classes
*****************************************************************************/
// Strictly speaking, we're testing LLEventTimeoutBase rather than the
// production LLEventTimeout (using LLTimer) because we don't want every test
// run to pause for some number of seconds until we reach a real timeout. But
// as we've carefully put all functionality except actual LLTimer calls into
// LLEventTimeoutBase, that should suffice. We're not not not trying to test
// LLTimer here.
class TestEventTimeout: public LLEventTimeoutBase
{
public:
    TestEventTimeout():
        mElapsed(true)
    {}
    TestEventTimeout(LLEventPump& source):
        LLEventTimeoutBase(source),
        mElapsed(true)
    {}

    // test hook
    void forceTimeout(bool timeout=true) { mElapsed = timeout; }

protected:
    virtual void setCountdown(F32 seconds) { mElapsed = false; }
    virtual bool countdownElapsed() const { return mElapsed; }

private:
    bool mElapsed;
};

// Similar remarks about LLEventThrottle: we're actually testing the logic in
// LLEventThrottleBase, dummying out the LLTimer and LLEventTimeout used by
// the production LLEventThrottle class.
class TestEventThrottle: public LLEventThrottleBase
{
public:
    TestEventThrottle(F32 interval):
        LLEventThrottleBase(interval),
        mAlarmRemaining(-1),
        mTimerRemaining(-1)
    {}
    TestEventThrottle(LLEventPump& source, F32 interval):
        LLEventThrottleBase(source, interval),
        mAlarmRemaining(-1),
        mTimerRemaining(-1)
    {}

    /*----- implementation of LLEventThrottleBase timing functionality -----*/
    virtual void alarmActionAfter(F32 interval, const LLEventTimeoutBase::Action& action) /*override*/
    {
        mAlarmRemaining = interval;
        mAlarmAction = action;
    }

    virtual bool alarmRunning() const /*override*/
    {
        // decrementing to exactly 0 should mean the alarm fires
        return mAlarmRemaining > 0;
    }

    virtual void alarmCancel() /*override*/
    {
        mAlarmRemaining = -1;
    }

    virtual void timerSet(F32 interval) /*override*/
    {
        mTimerRemaining = interval;
    }

    virtual F32  timerGetRemaining() const /*override*/
    {
        // LLTimer.getRemainingTimeF32() never returns negative; 0.0 means expired
        return (mTimerRemaining > 0.0)? mTimerRemaining : 0.0;
    }

    /*------------------- methods for manipulating time --------------------*/
    void alarmAdvance(F32 delta)
    {
        bool wasRunning = alarmRunning();
        mAlarmRemaining -= delta;
        if (wasRunning && ! alarmRunning())
        {
            mAlarmAction();
        }
    }

    void timerAdvance(F32 delta)
    {
        // This simple implementation, like alarmAdvance(), completely ignores
        // HOW negative mTimerRemaining might go. All that matters is whether
        // it's negative. We trust that no test method in this source will
        // drive it beyond the capacity of an F32. Seems like a safe assumption.
        mTimerRemaining -= delta;
    }

    void advance(F32 delta)
    {
        // Advance the timer first because it has no side effects.
        // alarmAdvance() might call flush(), which will need to see the
        // change in the timer.
        timerAdvance(delta);
        alarmAdvance(delta);
    }

    F32 mAlarmRemaining, mTimerRemaining;
    LLEventTimeoutBase::Action mAlarmAction;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct filter_data
    {
        // The resemblance between this test data and that in llevents_tut.cpp
        // is not coincidental.
        filter_data():
            pumps(LLEventPumps::instance()),
            mainloop(pumps.obtain("mainloop")),
            listener0("first"),
            listener1("second")
        {}
        LLEventPumps& pumps;
        LLEventPump& mainloop;
        Listener listener0;
        Listener listener1;

        void check_listener(const std::string& desc, const Listener& listener, const LLSD& got)
        {
            ensure_equals(STRINGIZE(listener << ' ' << desc),
                          listener.getLastEvent(), got);
        }
    };
    typedef test_group<filter_data> filter_group;
    typedef filter_group::object filter_object;
    filter_group filtergrp("lleventfilter");

    template<> template<>
    void filter_object::test<1>()
    {
        set_test_name("LLEventMatching");
        LLEventPump& driver(pumps.obtain("driver"));
        listener0.reset(0);
        // Listener isn't derived from LLEventTrackable specifically to test
        // various connection-management mechanisms. But that means we have a
        // couple of transient Listener objects, one of which is listening to
        // a persistent LLEventPump. Capture those connections in local
        // LLTempBoundListener instances so they'll disconnect
        // on destruction.
        LLTempBoundListener temp1(
            listener0.listenTo(driver));
        // Construct a pattern LLSD: desired Event must have a key "foo"
        // containing string "bar"
        LLSD pattern;
        pattern.insert("foo", "bar");
        LLEventMatching filter(driver, pattern);
        listener1.reset(0);
        LLTempBoundListener temp2(
            listener1.listenTo(filter));
        driver.post(1);
        check_listener("direct", listener0, LLSD(1));
        check_listener("filtered", listener1, LLSD(0));
        // Okay, construct an LLSD map matching the pattern
        LLSD data;
        data["foo"] = "bar";
        data["random"] = 17;
        driver.post(data);
        check_listener("direct", listener0, data);
        check_listener("filtered", listener1, data);
    }

    template<> template<>
    void filter_object::test<2>()
    {
        set_test_name("LLEventTimeout::actionAfter()");
        LLEventPump& driver(pumps.obtain("driver"));
        TestEventTimeout filter(driver);
        listener0.reset(0);
        LLTempBoundListener temp1(
            listener0.listenTo(filter));
        // Use listener1.call() as the Action for actionAfter(), since it
        // already provides a way to sense the call
        listener1.reset(0);
        // driver --> filter --> listener0
        filter.actionAfter(20,
                           boost::bind(&Listener::call, boost::ref(listener1), LLSD("timeout")));
        // Okay, (fake) timer is ticking. 'filter' can only sense the timer
        // when we pump mainloop. Do that right now to take the logic path
        // before either the anticipated event arrives or the timer expires.
        mainloop.post(17);
        check_listener("no timeout 1", listener1, LLSD(0));
        // Expected event arrives...
        driver.post(1);
        check_listener("event passed thru", listener0, LLSD(1));
        // Should have canceled the timer. Verify that by asserting that the
        // time has expired, then pumping mainloop again.
        filter.forceTimeout();
        mainloop.post(17);
        check_listener("no timeout 2", listener1, LLSD(0));
        // Verify chained actionAfter() calls, that is, that a second
        // actionAfter() resets the timer established by the first
        // actionAfter().
        filter.actionAfter(20,
                           boost::bind(&Listener::call, boost::ref(listener1), LLSD("timeout")));
        // Since our TestEventTimeout class isn't actually manipulating time
        // (quantities of seconds), only a bool "elapsed" flag, sense that by
        // forcing the flag between actionAfter() calls.
        filter.forceTimeout();
        // Pumping mainloop here would result in a timeout (as we'll verify
        // below). This state simulates a ticking timer that has not yet timed
        // out. But now, before a mainloop event lets 'filter' recognize
        // timeout on the previous actionAfter() call, pretend we're pushing
        // that timeout farther into the future.
        filter.actionAfter(20,
                           boost::bind(&Listener::call, boost::ref(listener1), LLSD("timeout")));
        // Look ma, no timeout!
        mainloop.post(17);
        check_listener("no timeout 3", listener1, LLSD(0));
        // Now let the updated actionAfter() timer expire.
        filter.forceTimeout();
        // Notice the timeout.
        mainloop.post(17);
        check_listener("timeout", listener1, LLSD("timeout"));
        // Timing out cancels the timer. Verify that.
        listener1.reset(0);
        filter.forceTimeout();
        mainloop.post(17);
        check_listener("no timeout 4", listener1, LLSD(0));
        // Reset the timer and then cancel() it.
        filter.actionAfter(20,
                           boost::bind(&Listener::call, boost::ref(listener1), LLSD("timeout")));
        // neither expired nor satisified
        mainloop.post(17);
        check_listener("no timeout 5", listener1, LLSD(0));
        // cancel
        filter.cancel();
        // timeout!
        filter.forceTimeout();
        mainloop.post(17);
        check_listener("no timeout 6", listener1, LLSD(0));
    }

    template<> template<>
    void filter_object::test<3>()
    {
        set_test_name("LLEventTimeout::eventAfter()");
        LLEventPump& driver(pumps.obtain("driver"));
        TestEventTimeout filter(driver);
        listener0.reset(0);
        LLTempBoundListener temp1(
            listener0.listenTo(filter));
        filter.eventAfter(20, LLSD("timeout"));
        // Okay, (fake) timer is ticking. 'filter' can only sense the timer
        // when we pump mainloop. Do that right now to take the logic path
        // before either the anticipated event arrives or the timer expires.
        mainloop.post(17);
        check_listener("no timeout 1", listener0, LLSD(0));
        // Expected event arrives...
        driver.post(1);
        check_listener("event passed thru", listener0, LLSD(1));
        // Should have canceled the timer. Verify that by asserting that the
        // time has expired, then pumping mainloop again.
        filter.forceTimeout();
        mainloop.post(17);
        check_listener("no timeout 2", listener0, LLSD(1));
        // Set timer again.
        filter.eventAfter(20, LLSD("timeout"));
        // Now let the timer expire.
        filter.forceTimeout();
        // Notice the timeout.
        mainloop.post(17);
        check_listener("timeout", listener0, LLSD("timeout"));
        // Timing out cancels the timer. Verify that.
        listener0.reset(0);
        filter.forceTimeout();
        mainloop.post(17);
        check_listener("no timeout 3", listener0, LLSD(0));
    }

    template<> template<>
    void filter_object::test<4>()
    {
        set_test_name("LLEventTimeout::errorAfter()");
        WrapLLErrs capture;
        LLEventPump& driver(pumps.obtain("driver"));
        TestEventTimeout filter(driver);
        listener0.reset(0);
        LLTempBoundListener temp1(
            listener0.listenTo(filter));
        filter.errorAfter(20, "timeout");
        // Okay, (fake) timer is ticking. 'filter' can only sense the timer
        // when we pump mainloop. Do that right now to take the logic path
        // before either the anticipated event arrives or the timer expires.
        mainloop.post(17);
        check_listener("no timeout 1", listener0, LLSD(0));
        // Expected event arrives...
        driver.post(1);
        check_listener("event passed thru", listener0, LLSD(1));
        // Should have canceled the timer. Verify that by asserting that the
        // time has expired, then pumping mainloop again.
        filter.forceTimeout();
        mainloop.post(17);
        check_listener("no timeout 2", listener0, LLSD(1));
        // Set timer again.
        filter.errorAfter(20, "timeout");
        // Now let the timer expire.
        filter.forceTimeout();
        // Notice the timeout.
        std::string threw = capture.catch_llerrs([this](){
                mainloop.post(17);
            });
        ensure_contains("errorAfter() timeout exception", threw, "timeout");
        // Timing out cancels the timer. Verify that.
        listener0.reset(0);
        filter.forceTimeout();
        mainloop.post(17);
        check_listener("no timeout 3", listener0, LLSD(0));
    }

    template<> template<>
    void filter_object::test<5>()
    {
        set_test_name("LLEventThrottle");
        TestEventThrottle throttle(3);
        Concat cat;
        throttle.listen("concat", boost::ref(cat));

        // (sequence taken from LLEventThrottleBase Doxygen comments)
        //  1: post(): event immediately passed to listeners, next no sooner than 4
        throttle.advance(1);
        throttle.post("1");
        ensure_equals("1", cat.result, "1"); // delivered immediately
        //  2: post(): deferred: waiting for 3 seconds to elapse
        throttle.advance(1);
        throttle.post("2");
        ensure_equals("2", cat.result, "1"); // "2" not yet delivered
        //  3: post(): deferred
        throttle.advance(1);
        throttle.post("3");
        ensure_equals("3", cat.result, "1"); // "3" not yet delivered
        //  4: no post() call, but event delivered to listeners; next no sooner than 7
        throttle.advance(1);
        ensure_equals("4", cat.result, "13"); // "3" delivered
        //  6: post(): deferred
        throttle.advance(2);
        throttle.post("6");
        ensure_equals("6", cat.result, "13"); // "6" not yet delivered
        //  7: no post() call, but event delivered; next no sooner than 10
        throttle.advance(1);
        ensure_equals("7", cat.result, "136"); // "6" delivered
        // 12: post(): immediately passed to listeners, next no sooner than 15
        throttle.advance(5);
        throttle.post(";12");
        ensure_equals("12", cat.result, "136;12"); // "12" delivered
        // 17: post(): immediately passed to listeners, next no sooner than 20
        throttle.advance(5);
        throttle.post(";17");
        ensure_equals("17", cat.result, "136;12;17"); // "17" delivered
    }

    template<class PUMP>
    void test()
    {
        PUMP pump(typeid(PUMP).name());
        LLSD data{LLSD::emptyArray()};
        bool consumed{true};
        // listener that appends to 'data'
        // but that also returns the current value of 'consumed'
        // Instantiate this separately because we're going to listen()
        // multiple times with the same lambda: LLEventMailDrop only replays
        // queued events on a new listen() call.
        auto lambda =
            [&data, &consumed](const LLSD& event)->bool
            {
                data.append(event);
                return consumed;
            };
        {
            LLTempBoundListener conn = pump.listen("lambda", lambda);
            pump.post("first");
        }
        // first post() should certainly be received by listener
        ensure_equals("first", data, llsd::array("first"));
        // the question is, since consumed was true, did it queue the value?
        data = LLSD::emptyArray();
        {
            // if it queued the value, it would be delivered on subsequent
            // listen() call
            LLTempBoundListener conn = pump.listen("lambda", lambda);
        }
        ensure_equals("empty1", data, LLSD::emptyArray());
        data = LLSD::emptyArray();
        // now let's NOT consume the posted data
        consumed = false;
        {
            LLTempBoundListener conn = pump.listen("lambda", lambda);
            pump.post("second");
            pump.post("third");
        }
        // the two events still arrive
        ensure_equals("second,third1", data, llsd::array("second", "third"));
        data = LLSD::emptyArray();
        {
            // when we reconnect, these should be delivered again
            // but this time they should be consumed
            consumed = true;
            LLTempBoundListener conn = pump.listen("lambda", lambda);
        }
        // unconsumed events were delivered again
        ensure_equals("second,third2", data, llsd::array("second", "third"));
        data = LLSD::emptyArray();
        {
            // when we reconnect this time, no more unconsumed events
            LLTempBoundListener conn = pump.listen("lambda", lambda);
        }
        ensure_equals("empty2", data, LLSD::emptyArray());
    }

    template<> template<>
    void filter_object::test<6>()
    {
        set_test_name("LLEventMailDrop");
        tut::test<LLEventMailDrop>();
    }

    template<> template<>
    void filter_object::test<7>()
    {
        set_test_name("LLEventLogProxyFor<LLEventMailDrop>");
        tut::test< LLEventLogProxyFor<LLEventMailDrop> >();
    }
} // namespace tut

/*****************************************************************************
*   Link dependencies
*****************************************************************************/
#include "llsdutil.cpp"
