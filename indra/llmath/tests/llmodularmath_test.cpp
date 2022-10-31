/**
 * @file modularmath_test.cpp
 * @author babbage
 * @date 2008-09
 * @brief llmodularmath tests
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
 
#include "linden_common.h"

#include "../llmodularmath.h"

#include "../test/lltut.h"

namespace tut
{
    struct modularmath_data
    {
    };
    typedef test_group<modularmath_data> modularmath_test;
    typedef modularmath_test::object modularmath_object;
    tut::modularmath_test modularmath_testcase("LLModularMath");

    template<> template<>
    void modularmath_object::test<1>()
    {
        // lhs < rhs
        const U32 lhs = 0x000001;
        const U32 rhs = 0xFFFFFF;
        const U32 width = 24;
        U32 result = LLModularMath::subtract<width>(lhs, rhs); 
        ensure_equals("diff(0x000001, 0xFFFFFF, 24)", result, 2);
    }

    template<> template<>
    void modularmath_object::test<2>()
    {
        // lhs > rhs
        const U32 lhs = 0x000002;
        const U32 rhs = 0x000001;
        const U32 width = 24;
        U32 result = LLModularMath::subtract<width>(lhs, rhs); 
        ensure_equals("diff(0x000002, 0x000001, 24)", result, 1);
    }

    template<> template<>
    void modularmath_object::test<3>()
    {
        // lhs == rhs
        const U32 lhs = 0xABCDEF;
        const U32 rhs = 0xABCDEF;
        const U32 width = 24;
        U32 result = LLModularMath::subtract<width>(lhs, rhs); 
        ensure_equals("diff(0xABCDEF, 0xABCDEF, 24)", result, 0);
    }   
}
