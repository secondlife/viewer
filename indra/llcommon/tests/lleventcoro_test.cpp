/**
 * @file   coroutine_test.cpp
 * @author Nat Goodspeed
 * @date   2009-04-22
 * @brief  Test for coroutine.
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

#define BOOST_RESULT_OF_USE_TR1 1
#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "linden_common.h"

#include <iostream>
#include <string>

#include "../test/lltut.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "../test/debug.h"

using namespace llcoro;

/*****************************************************************************
*   Test helpers
*****************************************************************************/
/// Simulate an event API whose response is immediate: sent on receipt of the
/// initial request, rather than after some delay. This is the case that
/// distinguishes postAndSuspend() from calling post(), then calling
/// suspendUntilEventOn().
class ImmediateAPI
{
public:
    ImmediateAPI():
        mPump("immediate", true)
    {
        mPump.listen("API", boost::bind(&ImmediateAPI::operator(), this, _1));
    }

    LLEventPump& getPump() { return mPump; }

    // Invoke this with an LLSD map containing:
    // ["value"]: Integer value. We will reply with ["value"] + 1.
    // ["reply"]: Name of LLEventPump on which to send success response.
    // ["error"]: Name of LLEventPump on which to send error response.
    // ["fail"]: Presence of this key selects ["error"], else ["success"] as
    // the name of the pump on which to send the response.
    bool operator()(const LLSD& event) const
    {
        LLSD::Integer value(event["value"]);
        LLSD::String replyPumpName(event.has("fail")? "error" : "reply");
        LLEventPumps::instance().obtain(event[replyPumpName]).post(value + 1);
        // give listener a chance to process
        llcoro::suspend();
        return false;
    }

private:
    LLEventStream mPump;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct coroutine_data {};
    typedef test_group<coroutine_data> coroutine_group;
    typedef coroutine_group::object object;
    coroutine_group coroutinegrp("coroutine");

    // use static data so we can intersperse coroutine functions with the
    // tests that engage them
    ImmediateAPI immediateAPI;
    std::string replyName, errorName, threw, stringdata;
    LLSD result, errordata;
    int which;

    // reinit vars at the start of each test
    void clear()
    {
        replyName.clear();
        errorName.clear();
        threw.clear();
        stringdata.clear();
        result = LLSD();
        errordata = LLSD();
        which = 0;
    }

    void explicit_wait(boost::shared_ptr<LLCoros::Promise<std::string>>& cbp)
    {
        BEGIN
        {
            // The point of this test is to verify / illustrate suspending a
            // coroutine for something other than an LLEventPump. In other
            // words, this shows how to adapt to any async operation that
            // provides a callback-style notification (and prove that it
            // works).

            // Perhaps we would send a request to a remote server and arrange
            // for cbp->set_value() to be called on response.
            // For test purposes, instead of handing 'callback' (or an
            // adapter) off to some I/O subsystem, we'll just pass it back to
            // our caller.
            cbp = boost::make_shared<LLCoros::Promise<std::string>>();
            LLCoros::Future<std::string> future = LLCoros::getFuture(*cbp);

            // calling get() on the future causes us to suspend
            debug("about to suspend");
            stringdata = future.get();
            ensure_equals("Got it", stringdata, "received");
        }
        END
    }

    template<> template<>
    void object::test<1>()
    {
        clear();
        set_test_name("explicit_wait");
        DEBUG;

        // Construct the coroutine instance that will run explicit_wait.
        boost::shared_ptr<LLCoros::Promise<std::string>> respond;
        LLCoros::instance().launch("test<1>",
                                   boost::bind(explicit_wait, boost::ref(respond)));
        // When the coroutine waits for the future, it returns here.
        debug("about to respond");
        // Now we're the I/O subsystem delivering a result. This should make
        // the coroutine ready.
        respond->set_value("received");
        // but give it a chance to wake up
        llcoro::suspend();
        // ensure the coroutine ran and woke up again with the intended result
        ensure_equals(stringdata, "received");
    }

    void waitForEventOn1()
    {
        BEGIN
        {
            result = suspendUntilEventOn("source");
        }
        END
    }

    template<> template<>
    void object::test<2>()
    {
        clear();
        set_test_name("waitForEventOn1");
        DEBUG;
        LLCoros::instance().launch("test<2>", waitForEventOn1);
        debug("about to send");
        LLEventPumps::instance().obtain("source").post("received");
        // give waitForEventOn1() a chance to run
        llcoro::suspend();
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void coroPump()
    {
        BEGIN
        {
            LLCoroEventPump waiter;
            replyName = waiter.getName();
            result = waiter.suspend();
        }
        END
    }

    template<> template<>
    void object::test<3>()
    {
        clear();
        set_test_name("coroPump");
        DEBUG;
        LLCoros::instance().launch("test<3>", coroPump);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        // give coroPump() a chance to run
        llcoro::suspend();
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void postAndWait1()
    {
        BEGIN
        {
            result = postAndSuspend(LLSDMap("value", 17),       // request event
                                 immediateAPI.getPump(),     // requestPump
                                 "reply1",                   // replyPump
                                 "reply");                   // request["reply"] = name
        }
        END
    }

    template<> template<>
    void object::test<4>()
    {
        clear();
        set_test_name("postAndWait1");
        DEBUG;
        LLCoros::instance().launch("test<4>", postAndWait1);
        // give postAndWait1() a chance to run
        llcoro::suspend();
        ensure_equals(result.asInteger(), 18);
    }

    void coroPumpPost()
    {
        BEGIN
        {
            LLCoroEventPump waiter;
            result = waiter.postAndSuspend(LLSDMap("value", 17),
                                        immediateAPI.getPump(), "reply");
        }
        END
    }

    template<> template<>
    void object::test<5>()
    {
        clear();
        set_test_name("coroPumpPost");
        DEBUG;
        LLCoros::instance().launch("test<5>", coroPumpPost);
        // give coroPumpPost() a chance to run
        llcoro::suspend();
        ensure_equals(result.asInteger(), 18);
    }
}
