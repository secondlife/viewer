/**
 * @file llbase64_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for LL BASE64
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
// Auto-generated from llbase64_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include <string>
#include "linden_common.h"
#include "../llbase64.h"
#include "../lluuid.h"

TUT_SUITE("llcommon")
{
    TUT_CASE("llbase64_test::base64_object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llbase64_test.cpp::base64_object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void base64_object::test<1>()
        //     {
        //         std::string result;

        //         result = LLBase64::encode(NULL, 0);
        //         ensure("encode nothing", (result == "") );

        //         LLUUID nothing;
        //         result = LLBase64::encode(&nothing.mData[0], UUID_BYTES);
        //         ensure("encode blank uuid",
        //                 (result == "AAAAAAAAAAAAAAAAAAAAAA==") );

        //         LLUUID id("526a1e07-a19d-baed-84c4-ff08a488d15e");
        //         result = LLBase64::encode(&id.mData[0], UUID_BYTES);
        //         ensure("encode random uuid",
        //                 (result == "UmoeB6Gduu2ExP8IpIjRXg==") );

        //     }
    }

    TUT_CASE("llbase64_test::base64_object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llbase64_test.cpp::base64_object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void base64_object::test<2>()
        //     {
        //         std::string result;

        //         U8 blob[40] = { 115, 223, 172, 255, 140, 70, 49, 125, 236, 155, 45, 199, 101, 17, 164, 131, 230, 19, 80, 64, 112, 53, 135, 98, 237, 12, 26, 72, 126, 14, 145, 143, 118, 196, 11, 177, 132, 169, 195, 134 };
        //         result = LLBase64::encode(&blob[0], 40);
        //         ensure("encode 40 bytes",
        //                 (result == "c9+s/4xGMX3smy3HZRGkg+YTUEBwNYdi7QwaSH4OkY92xAuxhKnDhg==") );
        //     }
    }

}

