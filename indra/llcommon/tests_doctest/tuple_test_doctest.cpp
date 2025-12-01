/**
 * @file tuple_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for tuple
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
// Auto-generated from tuple_test.cpp at 2025-10-16T18:47:17Z
// ---------------------------------------------------------------------------

#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"

#include "linden_common.h"
#include "tuple.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("tuple_test::object_test_1")
    {
        TUT_SET_TEST_NAME("tuple");

        std::tuple<std::string, int> tup{ "abc", 17 };
        std::tuple<int, std::string, int> ptup{ tuple_cons(34, tup) };

        std::tuple<std::string, int> tup2;
        int i = 0;

        std::tie(i, tup2) = tuple_split(ptup);

        TUT_ENSURE_EQ("tuple_car() fail", i, 34);
        TUT_ENSURE_EQ("tuple_cdr() (0) fail", std::get<0>(tup2), "abc");
        TUT_ENSURE_EQ("tuple_cdr() (1) fail", std::get<1>(tup2), 17);
    }
}
