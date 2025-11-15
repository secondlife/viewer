/**
 * @file stringize_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for stringize
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
// Auto-generated from stringize_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include <iomanip>
#include "linden_common.h"
#include "../stringize.h"
#include "../llsd.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("stringize_test::stringize_object_test_1")
    {
        DOCTEST_FAIL("TODO: convert stringize_test.cpp::stringize_object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void stringize_object::test<1>()
        //     {
        //         ensure_equals(stringize(c),    "c");
        //         ensure_equals(stringize(s),    "17");
        //         ensure_equals(stringize(i),    "34");
        //         ensure_equals(stringize(l),    "68");
        //         ensure_equals(stringize(f),    "3.14159");
        //         ensure_equals(stringize(d),    "3.14159");
        //         ensure_equals(stringize(abc),  "abc def");
        //         ensure_equals(stringize(def),  "def ghi"); //Will generate LL_WARNS() due to narrowing.
        //         ensure_equals(stringize(llsd), "{'abc':'abc def','d':r3.14159,'i':i34}");
        //     }
    }

    TUT_CASE("stringize_test::stringize_object_test_2")
    {
        DOCTEST_FAIL("TODO: convert stringize_test.cpp::stringize_object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void stringize_object::test<2>()
        //     {
        //         ensure_equals(STRINGIZE("c is " << c), "c is c");
        //         ensure_equals(STRINGIZE(std::setprecision(4) << d), "3.142");
        //     }
    }

    TUT_CASE("stringize_test::stringize_object_test_3")
    {
        DOCTEST_FAIL("TODO: convert stringize_test.cpp::stringize_object::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void stringize_object::test<3>()
        //     {
        //         //Tests rely on validity of wstring_to_utf8str()
        //         ensure_equals(wstring_to_utf8str(wstringize(c)),    wstring_to_utf8str(L"c"));
        //         ensure_equals(wstring_to_utf8str(wstringize(s)),    wstring_to_utf8str(L"17"));
        //         ensure_equals(wstring_to_utf8str(wstringize(i)),    wstring_to_utf8str(L"34"));
        //         ensure_equals(wstring_to_utf8str(wstringize(l)),    wstring_to_utf8str(L"68"));
        //         ensure_equals(wstring_to_utf8str(wstringize(f)),    wstring_to_utf8str(L"3.14159"));
        //         ensure_equals(wstring_to_utf8str(wstringize(d)),    wstring_to_utf8str(L"3.14159"));
        //         ensure_equals(wstring_to_utf8str(wstringize(abc)),  wstring_to_utf8str(L"abc def"));
        //         ensure_equals(wstring_to_utf8str(wstringize(abc)),  wstring_to_utf8str(wstringize(abc.c_str())));
        //         ensure_equals(wstring_to_utf8str(wstringize(def)),  wstring_to_utf8str(L"def ghi"));
        //  //       ensure_equals(wstring_to_utf8str(wstringize(llsd)), wstring_to_utf8str(L"{'abc':'abc def','d':r3.14159,'i':i34}"));
        //     }
    }

}

