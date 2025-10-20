/**
 * @file llcond_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL cond
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
// Auto-generated from llcond_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llcond.h"
#include "llcoros.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llcond_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llcond_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("Immediate gratification");
        //         cond.set_one(1);
        //         ensure("wait_for_equal() failed",
        //                cond.wait_for_equal(F32Milliseconds(1), 1));
        //         ensure("wait_for_unequal() should have failed",
        //                ! cond.wait_for_unequal(F32Milliseconds(1), 1));
        //     }
    }

    TUT_CASE("llcond_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llcond_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("Simple two-coroutine test");
        //         LLCoros::instance().launch(
        //             "test<2>",
        //             [this]()
        //             {
        //                 // Lambda immediately entered -- control comes here first.
        //                 ensure_equals(cond.get(), 0);
        //                 cond.set_all(1);
        //                 cond.wait_equal(2);
        //                 ensure_equals(cond.get(), 2);
        //                 cond.set_all(3);
        //             });
        //         // Main coroutine is resumed only when the lambda waits.
        //         ensure_equals(cond.get(), 1);
        //         cond.set_all(2);
        //         cond.wait_equal(3);
        //     }
    }

}

