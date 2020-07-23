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
#include <typeinfo>

#include "../test/lltut.h"
#include "../test/lltestapp.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventfilter.h"
#include "lleventcoro.h"
#include "../test/debug.h"
#include "../test/sync.h"

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
    ImmediateAPI(Sync& sync):
        mPump("immediate", true),
        mSync(sync)
    {
        mPump.listen("API", boost::bind(&ImmediateAPI::operator(), this, _1));
    }

    LLEventPump& getPump() { return mPump; }

    // Invoke this with an LLSD map containing:
    // ["value"]: Integer value. We will reply with ["value"] + 1.
    // ["reply"]: Name of LLEventPump on which to send response.
    bool operator()(const LLSD& event) const
    {
        mSync.bump();
        LLSD::Integer value(event["value"]);
        LLEventPumps::instance().obtain(event["reply"]).post(value + 1);
        return false;
    }

private:
    LLEventStream mPump;
    Sync& mSync;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct test_data
    {
        Sync mSync;
        ImmediateAPI immediateAPI{mSync};
        std::string replyName, errorName, threw, stringdata;
        LLSD result, errordata;
        int which;
        LLTestApp testApp;

        void explicit_wait(boost::shared_ptr<LLCoros::Promise<std::string>>& cbp);
        void waitForEventOn1();
        void coroPump();
        void postAndWait1();
        void coroPumpPost();
    };
    typedef test_group<test_data> coroutine_group;
    typedef coroutine_group::object object;
    coroutine_group coroutinegrp("coroutine");

    void test_data::explicit_wait(boost::shared_ptr<LLCoros::Promise<std::string>>& cbp)
    {
        BEGIN
        {
            mSync.bump();
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
            mSync.bump();
            ensure_equals("Got it", stringdata, "received");
        }
        END
    }

    template<> template<>
    void object::test<1>()
    {
        set_test_name("explicit_wait");
        DEBUG;

        // Construct the coroutine instance that will run explicit_wait.
        boost::shared_ptr<LLCoros::Promise<std::string>> respond;
        LLCoros::instance().launch("test<1>",
                                   [this, &respond](){ explicit_wait(respond); });
        mSync.bump();
        // When the coroutine waits for the future, it returns here.
        debug("about to respond");
        // Now we're the I/O subsystem delivering a result. This should make
        // the coroutine ready.
        respond->set_value("received");
        // but give it a chance to wake up
        mSync.yield();
        // ensure the coroutine ran and woke up again with the intended result
        ensure_equals(stringdata, "received");
    }

    void test_data::waitForEventOn1()
    {
        BEGIN
        {
            mSync.bump();
            result = suspendUntilEventOn("source");
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("waitForEventOn1");
        DEBUG;
        LLCoros::instance().launch("test<2>", [this](){ waitForEventOn1(); });
        mSync.bump();
        debug("about to send");
        LLEventPumps::instance().obtain("source").post("received");
        // give waitForEventOn1() a chance to run
        mSync.yield();
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void test_data::coroPump()
    {
        BEGIN
        {
            mSync.bump();
            LLCoroEventPump waiter;
            replyName = waiter.getName();
            result = waiter.suspend();
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("coroPump");
        DEBUG;
        LLCoros::instance().launch("test<3>", [this](){ coroPump(); });
        mSync.bump();
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        // give coroPump() a chance to run
        mSync.yield();
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    void test_data::postAndWait1()
    {
        BEGIN
        {
            mSync.bump();
            result = postAndSuspend(LLSDMap("value", 17),       // request event
                                 immediateAPI.getPump(),     // requestPump
                                 "reply1",                   // replyPump
                                 "reply");                   // request["reply"] = name
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("postAndWait1");
        DEBUG;
        LLCoros::instance().launch("test<4>", [this](){ postAndWait1(); });
        ensure_equals(result.asInteger(), 18);
    }

    void test_data::coroPumpPost()
    {
        BEGIN
        {
            mSync.bump();
            LLCoroEventPump waiter;
            result = waiter.postAndSuspend(LLSDMap("value", 17),
                                        immediateAPI.getPump(), "reply");
            mSync.bump();
        }
        END
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("coroPumpPost");
        DEBUG;
        LLCoros::instance().launch("test<5>", [this](){ coroPumpPost(); });
        ensure_equals(result.asInteger(), 18);
    }

    template <class PUMP>
    void test()
    {
        PUMP pump(typeid(PUMP).name());
        bool running{false};
        LLSD data{LLSD::emptyArray()};
        // start things off by posting once before even starting the listener
        // coro
        LL_DEBUGS() << "test() posting first" << LL_ENDL;
        LLSD first{LLSDMap("desc", "first")("value", 0)};
        bool consumed = pump.post(first);
        ensure("should not have consumed first", ! consumed);
        // now launch the coro
        LL_DEBUGS() << "test() launching listener coro" << LL_ENDL;
        running = true;
        LLCoros::instance().launch(
            "listener",
            [&pump, &running, &data](){
                // important for this test that we consume posted values
                LLCoros::instance().set_consuming(true);
                // should immediately retrieve 'first' without waiting
                LL_DEBUGS() << "listener coro waiting for first" << LL_ENDL;
                data.append(llcoro::suspendUntilEventOnWithTimeout(pump, 0.1, LLSD()));
                // Don't use ensure() from within the coro -- ensure() failure
                // throws tut::fail, which won't propagate out to the main
                // test driver, which will result in an odd failure.
                // Wait for 'second' because it's not already pending.
                LL_DEBUGS() << "listener coro waiting for second" << LL_ENDL;
                data.append(llcoro::suspendUntilEventOnWithTimeout(pump, 0.1, LLSD()));
                // and wait for 'third', which should involve no further waiting
                LL_DEBUGS() << "listener coro waiting for third" << LL_ENDL;
                data.append(llcoro::suspendUntilEventOnWithTimeout(pump, 0.1, LLSD()));
                LL_DEBUGS() << "listener coro done" << LL_ENDL;
                running = false;
            });
        // back from coro at the point where it's waiting for 'second'
        LL_DEBUGS() << "test() posting second" << LL_ENDL;
        LLSD second{llsd::map("desc", "second", "value", 1)};
        consumed = pump.post(second);
        ensure("should have consumed second", consumed);
        // This is a key point: even though we've post()ed the value for which
        // the coroutine is waiting, it's actually still suspended until we
        // pause for some other reason. The coroutine will only pick up one
        // value at a time from our 'pump'. It's important to exercise the
        // case when we post() two values before it picks up either.
        LL_DEBUGS() << "test() posting third" << LL_ENDL;
        LLSD third{llsd::map("desc", "third", "value", 2)};
        consumed = pump.post(third);
        ensure("should NOT yet have consumed third", ! consumed);
        // now just wait for coro to finish -- which it eventually will, given
        // that all its suspend calls have short timeouts.
        while (running)
        {
            LL_DEBUGS() << "test() waiting for coro done" << LL_ENDL;
            llcoro::suspendUntilTimeout(0.1);
        }
        // okay, verify expected results
        ensure_equals("should have received three values", data,
                      llsd::array(first, second, third));
        LL_DEBUGS() << "test() done" << LL_ENDL;
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("LLEventMailDrop");
        tut::test<LLEventMailDrop>();
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("LLEventLogProxyFor<LLEventMailDrop>");
        tut::test< LLEventLogProxyFor<LLEventMailDrop> >();
    }
}
