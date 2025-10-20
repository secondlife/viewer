/**
 * @file lldate_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL date
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
// Auto-generated from lldate_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "../llstring.h"
#include "../lldate.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("lldate_test::date_test_object_t_test_1")
    {
        DOCTEST_FAIL("TODO: convert lldate_test.cpp::date_test_object_t::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void date_test_object_t::test<1>()
        //     {
        //         LLDate date(VALID_DATE);
        //         std::string  expected_string;
        //         bool result;
        //         expected_string = VALID_DATE;
        //         ensure_equals("Valid Date failed" , expected_string, date.asString());

        //         result = date.fromString(VALID_DATE_LEAP);
        //         expected_string = VALID_DATE_LEAP;
        //         ensure_equals("VALID_DATE_LEAP failed" , expected_string, date.asString());

        //         result = date.fromString(VALID_DATE_HOUR_BOUNDARY);
        //         expected_string = VALID_DATE_HOUR_BOUNDARY;
        //         ensure_equals("VALID_DATE_HOUR_BOUNDARY failed" , expected_string, date.asString());

        //         result = date.fromString(VALID_DATE_FRACTIONAL_SECS);
        //         expected_string = VALID_DATE_FRACTIONAL_SECS;
        //         ensure_equals("VALID_DATE_FRACTIONAL_SECS failed" , expected_string, date.asString());

        //         result = date.fromString(INVALID_DATE_MISSING_YEAR);
        //         ensure_equals("INVALID_DATE_MISSING_YEAR should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MISSING_MONTH);
        //         ensure_equals("INVALID_DATE_MISSING_MONTH should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MISSING_DATE);
        //         ensure_equals("INVALID_DATE_MISSING_DATE should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MISSING_T);
        //         ensure_equals("INVALID_DATE_MISSING_T should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MISSING_HOUR);
        //         ensure_equals("INVALID_DATE_MISSING_HOUR should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MISSING_MIN);
        //         ensure_equals("INVALID_DATE_MISSING_MIN should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MISSING_SEC);
        //         ensure_equals("INVALID_DATE_MISSING_SEC should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MISSING_Z);
        //         ensure_equals("INVALID_DATE_MISSING_Z should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_EMPTY);
        //         ensure_equals("INVALID_DATE_EMPTY should have failed" , result, false);
        //     }
    }

    TUT_CASE("lldate_test::date_test_object_t_test_2")
    {
        DOCTEST_FAIL("TODO: convert lldate_test.cpp::date_test_object_t::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void date_test_object_t::test<2>()
        //     {
        // #if LL_DATE_PARSER_CHECKS_BOUNDARY
        //         LLDate date;
        //         std::string  expected_string;
        //         bool result;

        //         result = date.fromString(INVALID_DATE_24HOUR_BOUNDARY);
        //         ensure_equals("INVALID_DATE_24HOUR_BOUNDARY should have failed" , result, false);
        //         ensure_equals("INVALID_DATE_24HOUR_BOUNDARY date still set to old value on failure!" , date.secondsSinceEpoch(), 0);

        //         result = date.fromString(INVALID_DATE_LEAP);
        //         ensure_equals("INVALID_DATE_LEAP should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_HOUR);
        //         ensure_equals("INVALID_DATE_HOUR should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MIN);
        //         ensure_equals("INVALID_DATE_MIN should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_SEC);
        //         ensure_equals("INVALID_DATE_SEC should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_YEAR);
        //         ensure_equals("INVALID_DATE_YEAR should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_MONTH);
        //         ensure_equals("INVALID_DATE_MONTH should have failed" , result, false);

        //         result = date.fromString(INVALID_DATE_DAY);
        //         ensure_equals("INVALID_DATE_DAY should have failed" , result, false);
        // #endif
        //     }
    }

    TUT_CASE("lldate_test::date_test_object_t_test_3")
    {
        DOCTEST_FAIL("TODO: convert lldate_test.cpp::date_test_object_t::test<3> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void date_test_object_t::test<3>()
        //     {
        //         LLDate date;
        //         std::istringstream stream(VALID_DATE);
        //         std::string  expected_string = VALID_DATE;
        //         date.fromStream(stream);
        //         ensure_equals("fromStream failed", date.asString(), expected_string);
        //     }
    }

    TUT_CASE("lldate_test::date_test_object_t_test_4")
    {
        DOCTEST_FAIL("TODO: convert lldate_test.cpp::date_test_object_t::test<4> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void date_test_object_t::test<4>()
        //     {
        //         LLDate date1(VALID_DATE);
        //         LLDate date2(date1);
        //         ensure_equals("LLDate(const LLDate& date) constructor failed", date1.asString(), date2.asString());
        //     }
    }

    TUT_CASE("lldate_test::date_test_object_t_test_5")
    {
        DOCTEST_FAIL("TODO: convert lldate_test.cpp::date_test_object_t::test<5> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void date_test_object_t::test<5>()
        //     {
        //         LLDate date1(VALID_DATE);
        //         LLDate date2(date1.secondsSinceEpoch());
        //         ensure_equals("secondsSinceEpoch not equal",date1.secondsSinceEpoch(), date2.secondsSinceEpoch());
        //         ensure_equals("LLDate created using secondsSinceEpoch not equal", date1.asString(), date2.asString());
        //     }
    }

    TUT_CASE("lldate_test::date_test_object_t_test_6")
    {
        DOCTEST_FAIL("TODO: convert lldate_test.cpp::date_test_object_t::test<6> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void date_test_object_t::test<6>()
        //     {
        //         LLDate date(VALID_DATE);
        //         std::ostringstream stream;
        //         stream << date;
        //         std::string expected_str = VALID_DATE;
        //         ensure_equals("ostringstream failed", expected_str, stream.str());
        //     }
    }

    TUT_CASE("lldate_test::date_test_object_t_test_7")
    {
        DOCTEST_FAIL("TODO: convert lldate_test.cpp::date_test_object_t::test<7> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void date_test_object_t::test<7>()
        //     {
        //         LLDate date;
        //         std::istringstream stream(VALID_DATE);
        //         stream >> date;
        //         std::string expected_str = VALID_DATE;
        //         std::ostringstream out_stream;
        //         out_stream << date;

        //         ensure_equals("<< failed", date.asString(),expected_str);
        //         ensure_equals("<< to >> failed", stream.str(),out_stream.str());
        //     }
    }

}

