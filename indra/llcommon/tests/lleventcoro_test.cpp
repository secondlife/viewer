/**
 * @file   coroutine_test.cpp
 * @author Nat Goodspeed
 * @date   2009-04-22
 * @brief  Test for coroutine.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
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
#include <boost/coroutine/coroutine.hpp>
// Normally, lleventcoro.h obviates future.hpp. We only include this because
// we implement a "by hand" test of future functionality.
#include <boost/coroutine/future.hpp>
#include <boost/bind.hpp>
#include <boost/range.hpp>

#include "linden_common.h"

#include <iostream>
#include <string>

#include "../test/lltut.h"
#include "llsd.h"
#include "llevents.h"
#include "tests/wrapllerrs.h"
#include "stringize.h"
#include "lleventcoro.h"

/*****************************************************************************
*   Debugging stuff
*****************************************************************************/
// This class is intended to illuminate entry to a given block, exit from the
// same block and checkpoints along the way. It also provides a convenient
// place to turn std::cout output on and off.
class Debug
{
public:
    Debug(const std::string& block):
        mBlock(block)
    {
        (*this)("entry");
    }

    ~Debug()
    {
        (*this)("exit");
    }

    void operator()(const std::string& status)
    {
//      std::cout << mBlock << ' ' << status << std::endl;
    }

private:
    const std::string mBlock;
};

// It's often convenient to use the name of the enclosing function as the name
// of the Debug block.
#define DEBUG Debug debug(__FUNCTION__)

// These BEGIN/END macros are specifically for debugging output -- please
// don't assume you must use such for coroutines in general! They only help to
// make control flow (as well as exception exits) explicit.
#define BEGIN                                   \
{                                               \
    DEBUG;                                      \
    try

#define END                                             \
    catch (...)                                         \
    {                                                   \
/*      std::cout << "*** exceptional " << std::flush; */    \
        throw;                                          \
    }                                                   \
}

/*****************************************************************************
*   from the banana.cpp example program borrowed for test<1>()
*****************************************************************************/
namespace coroutines = boost::coroutines;
using coroutines::coroutine;

template<typename Iter>
bool match(Iter first, Iter last, std::string match) {
  std::string::iterator i = match.begin();
  i != match.end();
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
  BidirectionalIterator begin_ = begin;
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
// I suspect this will be typical of coroutines used in Linden software
typedef boost::coroutines::coroutine<void()> coroutine_type;

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

        void explicit_wait(coroutine_type::self& self)
        {
            BEGIN
            {
                // ... do whatever preliminary stuff must happen ...

                // declare the future
                boost::coroutines::future<LLSD> future(self);
                // tell the future what to wait for
                LLTempBoundListener connection(
                    LLEventPumps::instance().obtain("source").listen("coro", voidlistener(boost::coroutines::make_callback(future))));
                ensure("Not yet", ! future);
                // attempting to dereference ("resolve") the future causes the calling
                // coroutine to wait for it
                debug("about to wait");
                result = *future;
                ensure("Got it", future);
            }
            END
        }

        void waitForEventOn1(coroutine_type::self& self)
        {
            BEGIN
            {
                result = waitForEventOn(self, "source");
            }
            END
        }

        void waitForEventOn2(coroutine_type::self& self)
        {
            BEGIN
            {
                LLEventWithID pair = waitForEventOn(self, "reply", "error");
                result = pair.first;
                which  = pair.second;
                debug(STRINGIZE("result = " << result << ", which = " << which));
            }
            END
        }

        void postAndWait1(coroutine_type::self& self)
        {
            BEGIN
            {
                result = postAndWait(self,
                                     LLSD().insert("value", 17), // request event
                                     immediateAPI.getPump(),     // requestPump
                                     "reply1",                   // replyPump
                                     "reply");                   // request["reply"] = name
            }
            END
        }

        void postAndWait2(coroutine_type::self& self)
        {
            BEGIN
            {
                LLEventWithID pair = ::postAndWait2(self,
                                                    LLSD().insert("value", 18),
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

        void postAndWait2_1(coroutine_type::self& self)
        {
            BEGIN
            {
                LLEventWithID pair = ::postAndWait2(self,
                                                    LLSD().insert("value", 18).insert("fail", LLSD()),
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

        void coroPump(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPump waiter;
                replyName = waiter.getName();
                result = waiter.wait(self);
            }
            END
        }

        void coroPumpPost(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPump waiter;
                result = waiter.postAndWait(self, LLSD().insert("value", 17),
                                            immediateAPI.getPump(), "reply");
            }
            END
        }

        void coroPumps(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                LLEventWithID pair(waiter.wait(self));
                result = pair.first;
                which  = pair.second;
            }
            END
        }

        void coroPumpsNoEx(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                result = waiter.waitWithException(self);
            }
            END
        }

        void coroPumpsEx(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                try
                {
                    result = waiter.waitWithException(self);
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

        void coroPumpsNoLog(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                result = waiter.waitWithLog(self);
            }
            END
        }

        void coroPumpsLog(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                replyName = waiter.getName0();
                errorName = waiter.getName1();
                WrapLL_ERRS capture;
                try
                {
                    result = waiter.waitWithLog(self);
                    debug("no exception");
                }
                catch (const WrapLL_ERRS::FatalException& e)
                {
                    debug(STRINGIZE("exception " << e.what()));
                    threw = e.what();
                }
            }
            END
        }

        void coroPumpsPost(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                LLEventWithID pair(waiter.postAndWait(self, LLSD().insert("value", 23),
                                                      immediateAPI.getPump(), "reply", "error"));
                result = pair.first;
                which  = pair.second;
            }
            END
        }

        void coroPumpsPost_1(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                LLEventWithID pair(
                    waiter.postAndWait(self, LLSD().insert("value", 23).insert("fail", LLSD()),
                                       immediateAPI.getPump(), "reply", "error"));
                result = pair.first;
                which  = pair.second;
            }
            END
        }

        void coroPumpsPostNoEx(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                result = waiter.postAndWaitWithException(self, LLSD().insert("value", 8),
                                                         immediateAPI.getPump(), "reply", "error");
            }
            END
        }

        void coroPumpsPostEx(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                try
                {
                    result = waiter.postAndWaitWithException(self,
                        LLSD().insert("value", 9).insert("fail", LLSD()),
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

        void coroPumpsPostNoLog(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                result = waiter.postAndWaitWithLog(self, LLSD().insert("value", 30),
                                                   immediateAPI.getPump(), "reply", "error");
            }
            END
        }

        void coroPumpsPostLog(coroutine_type::self& self)
        {
            BEGIN
            {
                LLCoroEventPumps waiter;
                WrapLL_ERRS capture;
                try
                {
                    result = waiter.postAndWaitWithLog(self,
                        LLSD().insert("value", 31).insert("fail", LLSD()),
                        immediateAPI.getPump(), "reply", "error");
                    debug("no exception");
                }
                catch (const WrapLL_ERRS::FatalException& e)
                {
                    debug(STRINGIZE("exception " << e.what()));
                    threw = e.what();
                }
            }
            END
        }

        void ensure_done(coroutine_type& coro)
        {
            ensure("coroutine complete", ! coro);
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
        coroutine_type coro(boost::bind(&coroutine_data::explicit_wait, this, _1));
        // Start the coroutine
        coro(std::nothrow);
        // When the coroutine waits for the event pump, it returns here.
        debug("about to send");
        // Satisfy the wait.
        LLEventPumps::instance().obtain("source").post("received");
        // Now wait for the coroutine to complete.
        ensure_done(coro);
        // ensure the coroutine ran and woke up again with the intended result
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("waitForEventOn1");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::waitForEventOn1, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain("source").post("received");
        debug("back from send");
        ensure_done(coro);
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("waitForEventOn2 reply");
        {
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::waitForEventOn2, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain("reply").post("received");
        debug("back from send");
        ensure_done(coro);
        }
        ensure_equals(result.asString(), "received");
        ensure_equals("which pump", which, 0);
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("waitForEventOn2 error");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::waitForEventOn2, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain("error").post("badness");
        debug("back from send");
        ensure_done(coro);
        ensure_equals(result.asString(), "badness");
        ensure_equals("which pump", which, 1);
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("coroPump");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPump, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_done(coro);
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("coroPumps reply");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumps, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_done(coro);
        ensure_equals(result.asString(), "received");
        ensure_equals("which pump", which, 0);
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("coroPumps error");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumps, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure_done(coro);
        ensure_equals(result.asString(), "badness");
        ensure_equals("which pump", which, 1);
    }

    template<> template<>
    void object::test<9>()
    {
        set_test_name("coroPumpsNoEx");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsNoEx, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_done(coro);
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<10>()
    {
        set_test_name("coroPumpsEx");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsEx, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure_done(coro);
        ensure("no result", result.isUndefined());
        ensure_equals("got error", errordata.asString(), "badness");
    }

    template<> template<>
    void object::test<11>()
    {
        set_test_name("coroPumpsNoLog");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsNoLog, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain(replyName).post("received");
        debug("back from send");
        ensure_done(coro);
        ensure_equals(result.asString(), "received");
    }

    template<> template<>
    void object::test<12>()
    {
        set_test_name("coroPumpsLog");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsLog, this, _1));
        coro(std::nothrow);
        debug("about to send");
        LLEventPumps::instance().obtain(errorName).post("badness");
        debug("back from send");
        ensure_done(coro);
        ensure("no result", result.isUndefined());
        ensure_contains("got error", threw, "badness");
    }

    template<> template<>
    void object::test<13>()
    {
        set_test_name("postAndWait1");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::postAndWait1, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 18);
    }

    template<> template<>
    void object::test<14>()
    {
        set_test_name("postAndWait2");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::postAndWait2, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 19);
        ensure_equals(which, 0);
    }

    template<> template<>
    void object::test<15>()
    {
        set_test_name("postAndWait2_1");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::postAndWait2_1, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 19);
        ensure_equals(which, 1);
    }

    template<> template<>
    void object::test<16>()
    {
        set_test_name("coroPumpPost");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpPost, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 18);
    }

    template<> template<>
    void object::test<17>()
    {
        set_test_name("coroPumpsPost reply");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsPost, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 24);
        ensure_equals("which pump", which, 0);
    }

    template<> template<>
    void object::test<18>()
    {
        set_test_name("coroPumpsPost error");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsPost_1, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 24);
        ensure_equals("which pump", which, 1);
    }

    template<> template<>
    void object::test<19>()
    {
        set_test_name("coroPumpsPostNoEx");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsPostNoEx, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 9);
    }

    template<> template<>
    void object::test<20>()
    {
        set_test_name("coroPumpsPostEx");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsPostEx, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure("no result", result.isUndefined());
        ensure_equals("got error", errordata.asInteger(), 10);
    }

    template<> template<>
    void object::test<21>()
    {
        set_test_name("coroPumpsPostNoLog");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsPostNoLog, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure_equals(result.asInteger(), 31);
    }

    template<> template<>
    void object::test<22>()
    {
        set_test_name("coroPumpsPostLog");
        DEBUG;
        coroutine_type coro(boost::bind(&coroutine_data::coroPumpsPostLog, this, _1));
        coro(std::nothrow);
        ensure_done(coro);
        ensure("no result", result.isUndefined());
        ensure_contains("got error", threw, "32");
    }
} // namespace tut
