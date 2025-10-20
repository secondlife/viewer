/**
 * @file lazyeventapi_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for lazyeventapi
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
// Auto-generated from lazyeventapi_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "lazyeventapi.h"
#include "llevents.h"
#include "llsdutil.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("lazyeventapi_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert lazyeventapi_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("LazyEventAPI");
        //         // this is where the magic (should) happen
        //         // 'register' still a keyword until C++17
        //         MyRegistrar regster;
        //         LLEventPumps::instance().obtain("Test").post(llsd::map("op", "set", "data", "hey"));
        //         ensure_equals("failed to set data", data.asString(), "hey");
        //     }
    }

    TUT_CASE("lazyeventapi_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert lazyeventapi_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("No LazyEventAPI");
        //         // Because the MyRegistrar declaration in test<1>() is local, because
        //         // it has been destroyed, we fully expect NOT to reach a MyListener
        //         // instance with this post.
        //         LLEventPumps::instance().obtain("Test").post(llsd::map("op", "set", "data", "moot"));
        //         ensure("accidentally set data", ! data.isDefined());
        //     }
    }

    TUT_CASE("lazyeventapi_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert lazyeventapi_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         set_test_name("LazyEventAPI metadata");
        //         MyRegistrar regster;
        //         // Of course we have 'regster' in hand; we don't need to search for
        //         // it. But this next test verifies that we can find (all) LazyEventAPI
        //         // instances using LazyEventAPIBase::instance_snapshot. Normally we
        //         // wouldn't search; normally we'd just look at each instance in the
        //         // loop body.
        //         const MyRegistrar* found = nullptr;
        //         for (const auto& registrar : LL::LazyEventAPIBase::instance_snapshot())
        //             if ((found = dynamic_cast<const MyRegistrar*>(&registrar)))
        //                 break;
        //         ensure("Failed to find MyRegistrar via LLInstanceTracker", found);

        //         ensure_equals("wrong API name", found->getName(), "Test");
        //         ensure_contains("wrong API desc", found->getDesc(), "test LLEventAPI");
        //         ensure_equals("wrong API field", found->getDispatchKey(), "op");
        //         // Normally we'd just iterate over *found. But for test purposes,
        //         // actually capture the range of NameDesc pairs in a vector.
        //         std::vector<LL::LazyEventAPIBase::NameDesc> ops{ found->begin(), found->end() };
        //         ensure_equals("failed to find operations", ops.size(), 1);
        //         ensure_equals("wrong operation name", ops[0].first, "set");
        //         ensure_contains("wrong operation desc", ops[0].second, "set operation");
        //         LLSD metadata{ found->getMetadata(ops[0].first) };
        //         ensure_equals("bad metadata name", metadata["name"].asString(), ops[0].first);
        //         ensure_equals("bad metadata desc", metadata["desc"].asString(), ops[0].second);
        //     }
    }

}

