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

/*****************************************************************************/
//  test<1>() is cloned from a Boost.Coroutine example program whose copyright
//  info is reproduced here:
/*---------------------------------------------------------------------------*/
//  Copyright (c) 2006, Giovanni P. Deretta
//
//  This code may be used under either of the following two licences:
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy 
//  of this software and associated documentation files (the "Software"), to deal 
//  in the Software without restriction, including without limitation the rights 
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
//  copies of the Software, and to permit persons to whom the Software is 
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in 
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
//  THE SOFTWARE. OF SUCH DAMAGE.
//
//  Or:
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
/*****************************************************************************/

// On some platforms, Boost.Coroutine must #define magic symbols before
// #including platform-API headers. Naturally, that's ineffective unless the
// Boost.Coroutine #include is the *first* #include of the platform header.
// That means that client code must generally #include Boost.Coroutine headers
// before anything else.
#include <boost/dcoroutine/coroutine.hpp>
// Normally, lleventcoro.h obviates future.hpp. We only include this because
// we implement a "by hand" test of future functionality.
#include <boost/dcoroutine/future.hpp>
#include <boost/bind.hpp>
#include <boost/range.hpp>

#include "linden_common.h"

#include <iostream>
#include <string>

#include "../test/lltut.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "tests/wrapllerrs.h"
#include "stringize.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "../test/debug.h"

/*****************************************************************************
*   from the banana.cpp example program borrowed for test<1>()
*****************************************************************************/
namespace coroutines = boost::dcoroutines;
using coroutines::coroutine;

template<typename Iter>
bool match(Iter first, Iter last, std::string match) {
  std::string::iterator i = match.begin();
  for(; (first != last) && (i != match.end()); ++i) {
    if (*first != *i)
      return false;
    ++first;
  }
  return i == match.end();
}

template<typename BidirectionalIterator> 
BidirectionalIterator 
match_substring(BidirectionalIterator begin, 
		BidirectionalIterator end, 
		std::string xmatch,
		BOOST_DEDUCED_TYPENAME coroutine<BidirectionalIterator(void)>::self& self) { 
//BidirectionalIterator begin_ = begin;
  for(; begin != end; ++begin) 
    if(match(begin, end, xmatch)) {
      self.yield(begin);
    }
  return end;
} 

typedef coroutine<std::string::iterator(void)> match_coroutine_type;

/*****************************************************************************
*   Test helpers
*****************************************************************************/
/// Simulate an event API whose response is immediate: sent on receipt of the
/// initial request, rather than after some delay. This is the case that
/// distinguishes postAndWait() from calling post(), then calling
/// waitForEventOn().
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
    struct coroutine_data
    {
        // Define coroutine bodies as methods here so they can use ensure*()

        void explicit_wait(boost::dcoroutines::coroutine<void()>::self& self)
        {
            BEGIN
            {
                // ... do whatever preliminary stuff must happen ...

                // declare the future
                boost::dcoroutines::future<LLSD> future(self);
                // tell the future what to wait for
                LLTempBoundListener connection(
                    LLEventPumps::instance().obtain("source").listen("coro", voidlistener(boost::dcoroutines::make_callback(future))));
                ensure("Not yet", ! future);
                // attempting to dereference ("resolve") the future causes the calling
                // coroutine to wait for it
                debug("about to wait");
                result = *future;
                ensure("Got it", future);
            }
            END
        }

        void waitForEventOn1()
        {
            BEGIN
            {
                result = waitForEventOn("source");
            }
            END
        }

        void waitForEventOn2()
        {
            BEGIN
            {
                LLEventWithID pair = waitForEventOn("reply", "error");
                result = pair.first;
                which  = pair.second;
                debug(STRINGIZE("result = " << result << ", which = " << which));
            }
            END
        }

        void postAndWait1()
        {
            BEGIN
            {
                result = postAndWait(LLSDMap("value", 17),       // request event
                                     immediateAPI.getPump(),     // requestPump
                                     "reply1",                   // replyPump
                                     "reply");                   // request["reply"] = name
            }
            END
        }

        void postAndWait2()
        {
            BEGIN
            {
                LLEventWithID pair = ::postAndWait2(LLSDMap("value", 18),
                                                    immediateAPI.getPump(),
                                                    "reply2",
                                                    "error2",
                                                    "reply",
                                                    "error");
                result = pair.first;
                which  = pair.second;
                debug(STRINGIZE("result = " << result << ", which = " << which));
            }
            END
        }

        void postAndWait2_1()
        {
            BEGIN
            {
                LLEventWithID pair = ::postAndWait2(LLSDMap("value", 18)("fail", LLSD()),
                                                    immediateAPI.getPump(),
                                                    "reply2",
                                                    "error2",
                                                    "reply",
                                                    "error");
                result = pair.first;
                which  = pair.second;
                debug(STRINGIZE("result = " << result << ", which = " << which));
            }
            END
        }

        void coroPump()
        {
            BEGIN
            {
                LLCoroEventPump waiter;
                replyName = waiter.getName();
                result = waiter.wait();
            }
            END
        }

        void coroPumpPost()
        {
            BEGIN
            {
                LLCoroEventPump waiter;
                result = waiter.postAndWait(LLSDMap("value", 17),
                                            immediateAPI.getPump(), "reply");
            }
            END
        }

        void coroPumps()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                LLEventWithID pair(waiter.wait());
                result = pair.first;
                which  = pair.second;
            }
            END
        }

        void coroPumpsNoEx()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                result = waiter.waitWithException();
            }
            END
        }

        void coroPumpsEx()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                try
                {
                    result = waiter.waitWithException();
                    debug("no exception");
                }
                catch (const LLErrorEvent& e)
                {
                    debug(STRINGIZE("exception " << e.what()));
                    errordata = e.getData();
                }
            }
            END
        }

        void coroPumpsNoLog()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                result = waiter.waitWithLog();
            }
            END
        }

        void coroPumpsLog()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                WrapLLErrs capture;
                try
                {
                    result = waiter.waitWithLog();
                    debug("no exception");
                }
                catch (const WrapLLErrs::FatalException& e)
                {
                    debug(STRINGIZE("exception " << e.what()));
                    threw = e.what();
                }
            }
            END
        }

        void coroPumpsPost()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                LLEventWithID pair(waiter.postAndWait(LLSDMap("value", 23),
                                                      immediateAPI.getPump(), "reply", "error"));
                result = pair.first;
                which  = pair.second;
            }
            END
        }

        void coroPumpsPost_1()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                LLEventWithID pair(
                    waiter.postAndWait(LLSDMap("value", 23)("fail", LLSD()),
                                       immediateAPI.getPump(), "reply", "error"));
                result = pair.first;
                which  = pair.second;
            }
            END
        }

        void coroPumpsPostNoEx()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                result = waiter.postAndWaitWithException(LLSDMap("value", 8),
                                                         immediateAPI.getPump(), "reply", "error");
            }
            END
        }

        void coroPumpsPostEx()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                try
                {
                    result = waiter.postAndWaitWithException(
                        LLSDMap("value", 9)("fail", LLSD()),
                        immediateAPI.getPump(), "reply", "error");
                    debug("no exception");
                }
                catch (const LLErrorEvent& e)
                {
                    debug(STRINGIZE("exception " << e.what()));
                    errordata = e.getData();
                }
            }
            END
        }

        void coroPumpsPostNoLog()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                result = waiter.postAndWaitWithLog(LLSDMap("value", 30),
                                                   immediateAPI.getPump(), "reply", "error");
            }
            END
        }

        void coroPumpsPostLog()
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                WrapLLErrs capture;
                try
                {
                    result = waiter.postAndWaitWithLog(
                        LLSDMap("value", 31)("fail", LLSD()),
                        immediateAPI.getPump(), "reply", "error");
                    debug("no exception");
                }
                catch (const WrapLLErrs::FatalException& e)
                {
                    debug(STRINGIZE("exception " << e.what()));
                    threw = e.what();
                }
            }
            END
        }

        ImmediateAPI immediateAPI;
        std::string replyName, errorName, threw;
        LLSD result, errordata;
        int which;
    };
    typedef test_group<coroutine_data> coroutine_group;
    typedef coroutine_group::object object;
    coroutine_group coroutinegrp("coroutine");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("From banana.cpp example program in Boost.Coroutine distro");
        std::string buffer = "banananana"; 
        std::string match = "nana"; 
        std::string::iterator begin = buffer.begin();
        std::string::iterator end = buffer.end();

#if defined(BOOST_CORO_POSIX_IMPL)
//      std::cout << "Using Boost.Coroutine " << BOOST_CORO_POSIX_IMPL << '\n';
#else
//      std::cout << "Using non-Posix Boost.Coroutine implementation" << std::endl;
#endif

        typedef std::string::iterator signature(std::string::iterator, 
                                                std::string::iterator, 
                                                std::string,
                                                match_coroutine_type::self&);

        coroutine<std::string::iterator(void)> matcher
            (boost::bind(static_cast<signature*>(match_substring), 
                         begin, 
                         end, 
                         match, 
                         _1)); 

        std::string::iterator i = matcher();
/*==========================================================================*|
        while(matcher && i != buffer.end()) {
            std::cout <<"Match at: "<< std::distance(buffer.begin(), i)<<'\n'; 
            i = matcher();
        }
|*==========================================================================*/
        size_t matches[] = { 2, 4, 6 };
        for (size_t *mi(boost::begin(matches)), *mend(boost::end(matches));
             mi != mend; ++mi, i = matcher())
        {
            ensure("more", matcher);
            ensure("found", i != buffer.end());
            ensure_equals("value", std::distance(buffer.begin(), i), *mi);
        }
        ensure("done", ! matcher);
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("explicit_wait");
        DEBUG;

        // Construct the coroutine instance that will run explicit_wait.
        // Pass the ctor a callable that accepts the coroutine_type::self
        // param passed by the library.
        boost::dcoroutines::coroutine<void()>
        coro(boost::bind(&coroutine_data::explicit_wait, this, _1));
        // Start the coroutine
        coro(std::nothrow);
        // When the coroutine waits for the event pump, it returns here.
        debug("about to send");
        // Satisfy the wait.
        LLEventPumps::instance().obtain("source").post("received");
        // Now wait for the coroutine to complete.
        ensure("coroutine complete", ! coro);
        // ensure the coroutine ran and woke up again with the intended result
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("waitForEventOn1");
        DEBUG;
        LLCoros::instance().launch("test<3>",
                                   boost::bind(&coroutine_data::waitForEventOn1, this));
        debug("about to send");
        LLEventPumps::instance().obtain("source").post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("waitForEventOn2 reply");
        {
        DEBUG;
        LLCoros::instance().launch("test<4>", boost::bind(&coroutine_data::waitForEventOn2, this));
        debug("about to send");
        LLEventPumps::instance().obtain("reply").post("received");
        debug("back from send");
        }
        ensure_equals(result.asString(), "received");
        ensure_equals("which pump", which, 0);
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("waitForEventOn2 error");
        DEBUG;
        LLCoros::instance().launch("test<5>", boost::bind(&coroutine_data::waitForEventOn2, this));
        debug("about to send");
        LLEventPumps::instance().obtain("error").post("badness");
        debug("back from send");
        ensure_equals(result.asString(), "badness");
        ensure_equals("which pump", which, 1);
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("coroPump");
        DEBUG;
        LLCoros::instance().launch("test<6>", boost::bind(&coroutine_data::coroPump, this));
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("coroPumps reply");
        DEBUG;
        LLCoros::instance().launch("test<7>", boost::bind(&coroutine_data::coroPumps, this));
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
        ensure_equals("which pump", which, 0);
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("coroPumps error");
        DEBUG;
        LLCoros::instance().launch("test<8>", boost::bind(&coroutine_data::coroPumps, this));
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure_equals(result.asString(), "badness");
        ensure_equals("which pump", which, 1);
    }

    template<> template<>
    void object::test<9>()
    {
        set_test_name("coroPumpsNoEx");
        DEBUG;
        LLCoros::instance().launch("test<9>", boost::bind(&coroutine_data::coroPumpsNoEx, this));
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<10>()
    {
        set_test_name("coroPumpsEx");
        DEBUG;
        LLCoros::instance().launch("test<10>", boost::bind(&coroutine_data::coroPumpsEx, this));
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure("no result", result.isUndefined());
        ensure_equals("got error", errordata.asString(), "badness");
    }

    template<> template<>
    void object::test<11>()
    {
        set_test_name("coroPumpsNoLog");
        DEBUG;
        LLCoros::instance().launch("test<11>", boost::bind(&coroutine_data::coroPumpsNoLog, this));
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<12>()
    {
        set_test_name("coroPumpsLog");
        DEBUG;
        LLCoros::instance().launch("test<12>", boost::bind(&coroutine_data::coroPumpsLog, this));
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure("no result", result.isUndefined());
        ensure_contains("got error", threw, "badness");
    }

    template<> template<>
    void object::test<13>()
    {
        set_test_name("postAndWait1");
        DEBUG;
        LLCoros::instance().launch("test<13>", boost::bind(&coroutine_data::postAndWait1, this));
        ensure_equals(result.asInteger(), 18);
    }

    template<> template<>
    void object::test<14>()
    {
        set_test_name("postAndWait2");
        DEBUG;
        LLCoros::instance().launch("test<14>", boost::bind(&coroutine_data::postAndWait2, this));
        ensure_equals(result.asInteger(), 19);
        ensure_equals(which, 0);
    }

    template<> template<>
    void object::test<15>()
    {
        set_test_name("postAndWait2_1");
        DEBUG;
        LLCoros::instance().launch("test<15>", boost::bind(&coroutine_data::postAndWait2_1, this));
        ensure_equals(result.asInteger(), 19);
        ensure_equals(which, 1);
    }

    template<> template<>
    void object::test<16>()
    {
        set_test_name("coroPumpPost");
        DEBUG;
        LLCoros::instance().launch("test<16>", boost::bind(&coroutine_data::coroPumpPost, this));
        ensure_equals(result.asInteger(), 18);
    }

    template<> template<>
    void object::test<17>()
    {
        set_test_name("coroPumpsPost reply");
        DEBUG;
        LLCoros::instance().launch("test<17>", boost::bind(&coroutine_data::coroPumpsPost, this));
        ensure_equals(result.asInteger(), 24);
        ensure_equals("which pump", which, 0);
    }

    template<> template<>
    void object::test<18>()
    {
        set_test_name("coroPumpsPost error");
        DEBUG;
        LLCoros::instance().launch("test<18>", boost::bind(&coroutine_data::coroPumpsPost_1, this));
        ensure_equals(result.asInteger(), 24);
        ensure_equals("which pump", which, 1);
    }

    template<> template<>
    void object::test<19>()
    {
        set_test_name("coroPumpsPostNoEx");
        DEBUG;
        LLCoros::instance().launch("test<19>",
                                   boost::bind(&coroutine_data::coroPumpsPostNoEx, this));
        ensure_equals(result.asInteger(), 9);
    }

    template<> template<>
    void object::test<20>()
    {
        set_test_name("coroPumpsPostEx");
        DEBUG;
        LLCoros::instance().launch("test<20>", boost::bind(&coroutine_data::coroPumpsPostEx, this));
        ensure("no result", result.isUndefined());
        ensure_equals("got error", errordata.asInteger(), 10);
    }

    template<> template<>
    void object::test<21>()
    {
        set_test_name("coroPumpsPostNoLog");
        DEBUG;
        LLCoros::instance().launch("test<21>",
                                   boost::bind(&coroutine_data::coroPumpsPostNoLog, this));
        ensure_equals(result.asInteger(), 31);
    }

    template<> template<>
    void object::test<22>()
    {
        set_test_name("coroPumpsPostLog");
        DEBUG;
        LLCoros::instance().launch("test<22>", boost::bind(&coroutine_data::coroPumpsPostLog, this));
        ensure("no result", result.isUndefined());
        ensure_contains("got error", threw, "32");
    }
}

/*==========================================================================*|
#include <boost/context/guarded_stack_allocator.hpp>

namespace tut
{
    template<> template<>
    void object::test<23>()
    {
        set_test_name("stacksize");
        std::cout << "default_stacksize: " << boost::context::guarded_stack_allocator::default_stacksize() << '\n';
    }
} // namespace tut
|*==========================================================================*/
