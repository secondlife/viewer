/**
 * @file llheteromap_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL heteromap
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
// Auto-generated from llheteromap_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llheteromap.h"
#include <set>

TUT_SUITE("llcommon")
{
    TUT_CASE("llheteromap_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llheteromap_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("create, get, delete");

        //         {
        //             LLHeteroMap map;

        //             {
        //                 // create each instance
        //                 Chalk& chalk = map.obtain<Chalk>();
        //                 chalk.name = "Chalk";

        //                 Cheese& cheese = map.obtain<Cheese>();
        //                 cheese.name = "Cheese";

        //                 Chowdah& chowdah = map.obtain<Chowdah>();
        //                 chowdah.name = "Chowdah";
        //             } // refs go out of scope

        //             {
        //                 // verify each instance
        //                 Chalk& chalk = map.obtain<Chalk>();
        //                 ensure_equals(chalk.name, "Chalk");

        //                 Cheese& cheese = map.obtain<Cheese>();
        //                 ensure_equals(cheese.name, "Cheese");

        //                 Chowdah& chowdah = map.obtain<Chowdah>();
        //                 ensure_equals(chowdah.name, "Chowdah");
        //             }
        //         } // destroy map

        //         // Chalk, Cheese and Chowdah should have been created in specific order
        //         ensure_equals(clog, "aeo");

        //         // We don't care what order they're destroyed in, as long as each is
        //         // appropriately destroyed.
        //         std::set<std::string> dtorset;
        //         for (const char* cp = "aeo"; *cp; ++cp)
        //             dtorset.insert(std::string(1, *cp));
        //         ensure_equals(dlog, dtorset);
        //     }
    }

}

