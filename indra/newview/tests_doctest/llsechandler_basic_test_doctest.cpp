/**
 * @file llsechandler_basic_test_doctest.cpp
 * @author Roxie
 * @date 2026-03-28
 * @brief doctest wrapper for the legacy llsechandler_basic TUT cases.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "../llviewerprecompiledheaders.h"
#include "doctest.h"
#include "tut_doctest_shim.h"

#include "../tests/llsechandler_basic_test.cpp"

TUT_SUITE("LLSecHandlerBasic")
{
    TUT_CASE("LLSecHandlerBasic::test_1")
    {
        tut::sechandler_basic_test_object object;
        object.test<1>();
    }

    TUT_CASE("LLSecHandlerBasic::test_2")
    {
        tut::sechandler_basic_test_object object;
        object.test<2>();
    }

    TUT_CASE("LLSecHandlerBasic::test_3")
    {
        tut::sechandler_basic_test_object object;
        object.test<3>();
    }

    TUT_CASE("LLSecHandlerBasic::test_4")
    {
        tut::sechandler_basic_test_object object;
        object.test<4>();
    }

    TUT_CASE("LLSecHandlerBasic::test_5")
    {
        tut::sechandler_basic_test_object object;
        object.test<5>();
    }

    TUT_CASE("LLSecHandlerBasic::test_6")
    {
        tut::sechandler_basic_test_object object;
        object.test<6>();
    }

    TUT_CASE("LLSecHandlerBasic::test_7")
    {
        tut::sechandler_basic_test_object object;
        object.test<7>();
    }

    TUT_CASE("LLSecHandlerBasic::test_8")
    {
        tut::sechandler_basic_test_object object;
        object.test<8>();
    }

    TUT_CASE("LLSecHandlerBasic::test_9")
    {
        tut::sechandler_basic_test_object object;
        object.test<9>();
    }
}
