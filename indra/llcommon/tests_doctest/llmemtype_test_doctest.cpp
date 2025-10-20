/**
 * @file llmemtype_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL memtype
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
// Auto-generated from llmemtype_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
// #include "../llmemtype.h"  // not available on Windows
// #include "../llallocator.h"  // not available on Windows
#include <stack>

TUT_SUITE("llcommon")
{
    TUT_CASE("llmemtype_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llmemtype_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         ensure("Simplest test ever", true);
        //     }
    }

    TUT_CASE("llmemtype_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llmemtype_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         {
        //             LLMemType m1(LLMemType::MTYPE_INIT);
        //         }
        //         ensure("Test that you can construct and destruct the mem type");
        //     }
    }

    TUT_CASE("llmemtype_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert llmemtype_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<3>()
        //     {
        //         {
        //             ensure("Test that creation and destruction properly inc/dec the stack");
        //             ensure_equals(memTypeStack.size(), 0);
        //             {
        //                 LLMemType m1(LLMemType::MTYPE_INIT);
        //                 ensure_equals(memTypeStack.size(), 1);
        //                 LLMemType m2(LLMemType::MTYPE_STARTUP);
        //                 ensure_equals(memTypeStack.size(), 2);
        //             }
        //             ensure_equals(memTypeStack.size(), 0);
        //         }
        //     }
    }

    TUT_CASE("llmemtype_test::object_test_4")
    {
        DOCTEST_FAIL("TODO: convert llmemtype_test.cpp::object::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<4>()
        //     {
        //         // catch the begining and end
        //         std::string test_name = LLMemType::getNameFromID(LLMemType::MTYPE_INIT.mID);
        //         ensure_equals("Init name", test_name, "Init");

        //         std::string test_name2 = LLMemType::getNameFromID(LLMemType::MTYPE_VOLUME.mID);
        //         ensure_equals("Volume name", test_name2, "Volume");

        //         std::string test_name3 = LLMemType::getNameFromID(LLMemType::MTYPE_OTHER.mID);
        //         ensure_equals("Other name", test_name3, "Other");

        //         std::string test_name4 = LLMemType::getNameFromID(-1);
        //         ensure_equals("Invalid name", test_name4, "INVALID");
        //     }
    }

}

