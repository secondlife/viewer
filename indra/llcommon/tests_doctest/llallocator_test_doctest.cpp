/**
 * @file llallocator_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL allocator
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
// Auto-generated from llallocator_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
// #include "../llallocator.h"  // not available on Windows

TUT_SUITE("llcommon")
{
    TUT_CASE("llallocator_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llallocator_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         llallocator.setProfilingEnabled(false);
        //         ensure("Profiler disable", !llallocator.isProfiling());
        //     }
    }

    TUT_CASE("llallocator_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llallocator_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         llallocator.setProfilingEnabled(true);
        //         ensure("Profiler enable", llallocator.isProfiling());
        //     }
    }

    TUT_CASE("llallocator_test::object_test_3")
    {
        DOCTEST_FAIL("TODO: convert llallocator_test.cpp::object::test<3> from TUT to doctest");
        // Original snippet:
        // template <> template <>
        //     void object::test<3>()
        //     {
        //         llallocator.setProfilingEnabled(true);

        //         char * test_alloc = new char[1024];

        //         llallocator.getProfile();

        //         delete [] test_alloc;

        //         llallocator.getProfile();

        //         // *NOTE - this test isn't ensuring anything right now other than no
        //         // exceptions are thrown.
        //     }
    }

}

