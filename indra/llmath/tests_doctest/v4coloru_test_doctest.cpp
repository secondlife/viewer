// doctest translation of v4coloru_test.cpp
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"

#include "llsd.h"

#include "../v4coloru.h"

/**
 * @file v4coloru_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v4coloru test cases.
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

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::ensure_not;
    using tut_compat::ensure_throws;

    struct v4coloru_data
    {
    };
} // namespace tut

TUT_SUITE("v4coloru_test")
{
    TUT_CASE("v4coloru_test::v4coloru_object_test_1")
    {
        using namespace tut;

        LLColor4U llcolor4u;
        TUT_ENSURE("1:LLColor4u:Fail to initialize ", ((0 == llcolor4u.mV[VRED]) && (0 == llcolor4u.mV[VGREEN]) && (0 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4u1(r,g,b);
        TUT_ENSURE("2:LLColor4u:Fail to initialize ", ((r == llcolor4u1.mV[VRED]) && (g == llcolor4u1.mV[VGREEN]) && (b == llcolor4u1.mV[VBLUE])&& (255 == llcolor4u1.mV[VALPHA])));

        LLColor4U llcolor4u2(r,g,b,a);
        TUT_ENSURE("3:LLColor4u:Fail to initialize ", ((r == llcolor4u2.mV[VRED]) && (g == llcolor4u2.mV[VGREEN]) && (b == llcolor4u2.mV[VBLUE])&& (a == llcolor4u2.mV[VALPHA])));

        const U8 vec[4] = {0x12,0xFF,0xAF,0x23};
        LLColor4U llcolor4u3(vec);
        TUT_ENSURE("4:LLColor4u:Fail to initialize ", ((vec[0] == llcolor4u3.mV[VRED]) && (vec[1] == llcolor4u3.mV[VGREEN]) && (vec[2] == llcolor4u3.mV[VBLUE])&& (vec[3] == llcolor4u3.mV[VALPHA])));

        LLSD sd = llcolor4u3.getValue();
        LLColor4U llcolor4u4(sd);
        TUT_ENSURE_EQ("5:LLColor4u (LLSD) Failed ", llcolor4u4, llcolor4u3);
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_2")
    {
        using namespace tut;

        LLColor4U llcolor4ua(1, 2, 3, 4);
        LLSD sd = llcolor4ua.getValue();
        LLColor4U llcolor4u;
        llcolor4u.setValue(sd);
        TUT_ENSURE_EQ("setValue(LLSD)/getValue Failed ", llcolor4u, llcolor4ua);
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_3")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4u(r,g,b,a);
        llcolor4u.setToBlack();
        TUT_ENSURE("setToBlack:Fail to set black ", ((0 == llcolor4u.mV[VRED]) && (0 == llcolor4u.mV[VGREEN]) && (0 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));

        llcolor4u.setToWhite();
        TUT_ENSURE("setToWhite:Fail to white ", ((255 == llcolor4u.mV[VRED]) && (255 == llcolor4u.mV[VGREEN]) && (255 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_4")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4ua(r,g,b,a);
        LLSD sd = llcolor4ua.getValue();
        LLColor4U llcolor4u = (LLColor4U)sd;
        TUT_ENSURE_EQ("Operator=(LLSD) Failed ",  llcolor4u, llcolor4ua);
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_5")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4u;
        llcolor4u.setVec(r,g,b,a);
        TUT_ENSURE("1:setVec:Fail to set the values ", ((r == llcolor4u.mV[VRED]) && (g == llcolor4u.mV[VGREEN]) && (b == llcolor4u.mV[VBLUE])&& (a == llcolor4u.mV[VALPHA])));

        llcolor4u.setToBlack();
        llcolor4u.setVec(r,g,b);
        TUT_ENSURE("2:setVec:Fail to set the values ", ((r == llcolor4u.mV[VRED]) && (g == llcolor4u.mV[VGREEN]) && (b == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));

        LLColor4U llcolor4u1;
        llcolor4u1.setVec(llcolor4u);
        TUT_ENSURE_EQ("3:setVec:Fail to set the values ", llcolor4u1,llcolor4u);

        const U8 vec[4] = {0x12,0xFF,0xAF,0x23};
        LLColor4U llcolor4u2;
        llcolor4u2.setVec(vec);
        TUT_ENSURE("4:setVec:Fail to set the values ", ((vec[0] == llcolor4u2.mV[VRED]) && (vec[1] == llcolor4u2.mV[VGREEN]) && (vec[2] == llcolor4u2.mV[VBLUE])&& (vec[3] == llcolor4u2.mV[VALPHA])));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_6")
    {
        using namespace tut;

        U8 alpha = 0x12;
        LLColor4U llcolor4u;
        llcolor4u.setAlpha(alpha);
        TUT_ENSURE("setAlpha:Fail to set alpha value ", (alpha == llcolor4u.mV[VALPHA]));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_7")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF;
        LLColor4U llcolor4u(r,g,b);
        TUT_ENSURE("magVecSquared:Fail ", is_approx_equal(llcolor4u.magVecSquared(), (F32)(r*r + g*g + b*b)));
        TUT_ENSURE("magVec:Fail ", is_approx_equal(llcolor4u.magVec(), (F32) sqrt((F32) (r*r + g*g + b*b))));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_8")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF;
        std::ostringstream stream1, stream2;
        LLColor4U llcolor4u1(r,g,b),llcolor4u2;
        stream1 << llcolor4u1;
        llcolor4u2.setVec(r,g,b);
        stream2 << llcolor4u2;
        TUT_ENSURE("operator << failed ", (stream1.str() == stream2.str()));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_9")
    {
        using namespace tut;

        U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
        U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
        LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
        llcolor4u3 = llcolor4u1 + llcolor4u2;
        TUT_ENSURE_EQ("1a.operator+:Fail to Add the values ", llcolor4u3.mV[VRED], (U8)(r1+r2));
        TUT_ENSURE_EQ("1b.operator+:Fail to Add the values ", llcolor4u3.mV[VGREEN], (U8)(g1+g2));
        TUT_ENSURE_EQ("1c.operator+:Fail to Add the values ", llcolor4u3.mV[VBLUE], (U8)(b1+b2));

        llcolor4u2 += llcolor4u1;
        TUT_ENSURE_EQ("2a.operator+=:Fail to Add the values ", llcolor4u2.mV[VRED], (U8)(r1+r2));
        TUT_ENSURE_EQ("2b.operator+=:Fail to Add the values ", llcolor4u2.mV[VGREEN], (U8)(g1+g2));
        TUT_ENSURE_EQ("2c.operator+=:Fail to Add the values ", llcolor4u2.mV[VBLUE], (U8)(b1+b2));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_10")
    {
        using namespace tut;

        U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
        U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
        LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
        llcolor4u3 = llcolor4u1 - llcolor4u2;
        TUT_ENSURE_EQ("1a. operator-:Fail to Add the values ", llcolor4u3.mV[VRED], (U8)(r1-r2));
        TUT_ENSURE_EQ("1b. operator-:Fail to Add the values ", llcolor4u3.mV[VGREEN], (U8)(g1-g2));
        TUT_ENSURE_EQ("1c. operator-:Fail to Add the values ", llcolor4u3.mV[VBLUE], (U8)(b1-b2));

        llcolor4u1 -= llcolor4u2;
        TUT_ENSURE_EQ("2a. operator-=:Fail to Add the values ", llcolor4u1.mV[VRED], (U8)(r1-r2));
        TUT_ENSURE_EQ("2b. operator-=:Fail to Add the values ", llcolor4u1.mV[VGREEN], (U8)(g1-g2));
        TUT_ENSURE_EQ("2c. operator-=:Fail to Add the values ", llcolor4u1.mV[VBLUE], (U8)(b1-b2));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_11")
    {
        using namespace tut;

        U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
        U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
        LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
        llcolor4u3 = llcolor4u1 * llcolor4u2;
        TUT_ENSURE_EQ("1a. operator*:Fail to multiply the values", llcolor4u3.mV[VRED], (U8)(r1*r2));
        TUT_ENSURE_EQ("1b. operator*:Fail to multiply the values", llcolor4u3.mV[VGREEN], (U8)(g1*g2));
        TUT_ENSURE_EQ("1c. operator*:Fail to multiply the values", llcolor4u3.mV[VBLUE], (U8)(b1*b2));

        U8 mulVal = 123;
        llcolor4u1 *= mulVal;
        TUT_ENSURE_EQ("2a. operator*=:Fail to multiply the values", llcolor4u1.mV[VRED], (U8)(r1*mulVal));
        TUT_ENSURE_EQ("2b. operator*=:Fail to multiply the values", llcolor4u1.mV[VGREEN], (U8)(g1*mulVal));
        TUT_ENSURE_EQ("2c. operator*=:Fail to multiply the values", llcolor4u1.mV[VBLUE], (U8)(b1*mulVal));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_12")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF;
        LLColor4U llcolor4u(r,g,b),llcolor4u1;
        llcolor4u1 = llcolor4u;
        TUT_ENSURE("operator== failed to ensure the equality ", (llcolor4u1 == llcolor4u));
        llcolor4u1.setToBlack();
        TUT_ENSURE("operator!= failed to ensure the equality ", (llcolor4u1 != llcolor4u));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_13")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 12;
        LLColor4U llcolor4u(r,g,b,a);
        U8 modVal = 45;
        llcolor4u %= modVal;
        TUT_ENSURE_EQ("operator%=:Fail ", llcolor4u.mV[VALPHA], (U8)(a * modVal));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_14")
    {
        using namespace tut;

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 12;
        LLColor4U llcolor4u1(r,g,b,a);
        std::string color("12, 23, 132, 50");
        LLColor4U::parseColor4U(color, &llcolor4u1);
        TUT_ENSURE("parseColor4U() failed to parse the color value ", ((12 == llcolor4u1.mV[VRED]) && (23 == llcolor4u1.mV[VGREEN]) && (132 == llcolor4u1.mV[VBLUE])&& (50 == llcolor4u1.mV[VALPHA])));

        color = "12, 23, 132";
        TUT_ENSURE("2:parseColor4U() failed to parse the color value ",  (false == LLColor4U::parseColor4U(color, &llcolor4u1)));

        color = "12";
        TUT_ENSURE("2:parseColor4U() failed to parse the color value ",  (false == LLColor4U::parseColor4U(color, &llcolor4u1)));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_15")
    {
        using namespace tut;

        U8 r = 12, g = 123, b = 3, a = 2;
        LLColor4U llcolor4u(r,g,b,a),llcolor4u1;
        const F32 fVal = 3.f;
        llcolor4u1 = llcolor4u.multAll(fVal);
        TUT_ENSURE("multAll:Fail to multiply ", (((U8)ll_round(r * fVal) == llcolor4u1.mV[VRED]) && (U8)ll_round(g * fVal) == llcolor4u1.mV[VGREEN]
                                            && ((U8)ll_round(b * fVal) == llcolor4u1.mV[VBLUE])&& ((U8)ll_round(a * fVal) == llcolor4u1.mV[VALPHA])));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_16")
    {
        using namespace tut;

        U8 r1 = 12, g1 = 123, b1 = 3, a1 = 2;
        U8 r2 = 23, g2 = 230, b2 = 124, a2 = 255;
        LLColor4U llcolor4u(r1,g1,b1,a1),llcolor4u1(r2,g2,b2,a2);
        llcolor4u1 = llcolor4u1.addClampMax(llcolor4u);
        TUT_ENSURE("1:addClampMax():Fail to add the value ",  ((r1+r2 == llcolor4u1.mV[VRED]) && (255 == llcolor4u1.mV[VGREEN]) && (b1+b2 == llcolor4u1.mV[VBLUE])&& (255 == llcolor4u1.mV[VALPHA])));

        r1 = 132, g1 = 3, b1 = 3, a1 = 2;
        r2 = 123, g2 = 230, b2 = 154, a2 = 25;
        LLColor4U llcolor4u2(r1,g1,b1,a1),llcolor4u3(r2,g2,b2,a2);
        llcolor4u3 = llcolor4u3.addClampMax(llcolor4u2);
        TUT_ENSURE("2:addClampMax():Fail to add the value ",  ((255 == llcolor4u3.mV[VRED]) && (g1+g2 == llcolor4u3.mV[VGREEN]) && (b1+b2 == llcolor4u3.mV[VBLUE])&& (a1+a2 == llcolor4u3.mV[VALPHA])));
    }

    TUT_CASE("v4coloru_test::v4coloru_object_test_17")
    {
        using namespace tut;

        F32 r = 23.f, g = 12.32f, b = -12.3f;
        LLColor3 color3(r,g,b);
        LLColor4U llcolor4u;
        llcolor4u.setVecScaleClamp(color3);
        const S32 MAX_COLOR = 255;
        F32 color_scale_factor = MAX_COLOR/r;
        S32 r2 = ll_round(r * color_scale_factor);
        S32 g2 = ll_round(g * color_scale_factor);
        TUT_ENSURE("setVecScaleClamp():Fail to add the value ",  ((r2 == llcolor4u.mV[VRED]) && (g2 == llcolor4u.mV[VGREEN]) && (0 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));
    }
}
