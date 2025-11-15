/**
 * @file lleventcoro_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL eventcoro
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
// Auto-generated from lleventcoro_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include <boost/bind.hpp>
#include <boost/range.hpp>
#include <boost/utility.hpp>
#include "linden_common.h"
#include <iostream>
#include <string>
#include <typeinfo>
#include "../test/lltestapp.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llevents.h"
#include "llcoros.h"
#include "lleventfilter.h"
#include "lleventcoro.h"
#include "../test/debug.h"
#include "../test/sync.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("lleventcoro_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert lleventcoro_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("explicit_wait");
        //         DEBUG;

        //         // Construct the coroutine instance that will run explicit_wait.
        //         std::shared_ptr<LLCoros::Promise<std::string>> respond;
        //         LLCoros::instance().launch("test<1>",
        //                                    [this, &respond](){ explicit_wait(respond); });
        //         mSync.bump();
        //         // When the coroutine waits for the future, it returns here.
        //         debug("about to respond");
        //         // Now we're the I/O subsystem delivering a result. This should make
        //         // the coroutine ready.
        //         respond->set_value("received");
        //         // but give it a chance to wake up
        //         mSync.yield();
        //         // ensure the coroutine ran and woke up again with the intended result
        //         ensure_equals(stringdata, "received");
        //     }
    }

    TUT_CASE("lleventcoro_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert lleventcoro_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("waitForEventOn1");
        //         DEBUG;
        //         LLCoros::instance().launch("test<2>", [this](){ waitForEventOn1(); });
        //         mSync.bump();
        //         debug("about to send");
        //         LLEventPumps::instance().obtain("source").post("received");
        //         // give waitForEventOn1() a chance to run
        //         mSync.yield();
        //         debug("back from send");
        //         ensure_equals(result.asString(), "received");
        //     }
    }

    TUT_CASE("lleventcoro_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert lleventcoro_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         set_test_name("coroPump");
        //         DEBUG;
        //         LLCoros::instance().launch("test<3>", [this](){ coroPump(); });
        //         mSync.bump();
        //         debug("about to send");
        //         LLEventPumps::instance().obtain(replyName).post("received");
        //         // give coroPump() a chance to run
        //         mSync.yield();
        //         debug("back from send");
        //         ensure_equals(result.asString(), "received");
        //     }
    }

    TUT_CASE("lleventcoro_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert lleventcoro_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         set_test_name("postAndWait1");
        //         DEBUG;
        //         LLCoros::instance().launch("test<4>", [this](){ postAndWait1(); });
        //         ensure_equals(result.asInteger(), 18);
        //     }
    }

    TUT_CASE("lleventcoro_test::object_test_5")
    {
        DOCTEST_FAIL("TODO: convert lleventcoro_test.cpp::object::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<5>()
        //     {
        //         set_test_name("coroPumpPost");
        //         DEBUG;
        //         LLCoros::instance().launch("test<5>", [this](){ coroPumpPost(); });
        //         ensure_equals(result.asInteger(), 18);
        //     }
    }

    TUT_CASE("lleventcoro_test::object_test_6")
    {
        DOCTEST_FAIL("TODO: convert lleventcoro_test.cpp::object::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<6>()
        //     {
        //         set_test_name("LLEventMailDrop");
        //         tut::test<LLEventMailDrop>();
        //     }
    }

    TUT_CASE("lleventcoro_test::object_test_7")
    {
        DOCTEST_FAIL("TODO: convert lleventcoro_test.cpp::object::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<7>()
        //     {
        //         set_test_name("LLEventLogProxyFor<LLEventMailDrop>");
        //         tut::test< LLEventLogProxyFor<LLEventMailDrop> >();
        //     }
    }

}

