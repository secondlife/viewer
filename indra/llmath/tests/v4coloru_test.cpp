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


#include "linden_common.h"
#include "../test/lldoctest.h"

#include "llsd.h"

#include "../v4coloru.h"


TEST_SUITE("v4coloru_h") {

TEST_CASE("test_1")
{

        LLColor4U llcolor4u;
        CHECK_MESSAGE(((0 == llcolor4u.mV[VRED], "1:LLColor4u:Fail to initialize ") && (0 == llcolor4u.mV[VGREEN]) && (0 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4u1(r,g,b);
        CHECK_MESSAGE(((r == llcolor4u1.mV[VRED], "2:LLColor4u:Fail to initialize ") && (g == llcolor4u1.mV[VGREEN]) && (b == llcolor4u1.mV[VBLUE])&& (255 == llcolor4u1.mV[VALPHA])));

        LLColor4U llcolor4u2(r,g,b,a);
        CHECK_MESSAGE(((r == llcolor4u2.mV[VRED], "3:LLColor4u:Fail to initialize ") && (g == llcolor4u2.mV[VGREEN]) && (b == llcolor4u2.mV[VBLUE])&& (a == llcolor4u2.mV[VALPHA])));

        const U8 vec[4] = {0x12,0xFF,0xAF,0x23
}

TEST_CASE("test_2")
{

        LLColor4U llcolor4ua(1, 2, 3, 4);
        LLSD sd = llcolor4ua.getValue();
        LLColor4U llcolor4u;
        llcolor4u.setValue(sd);
        CHECK_MESSAGE(llcolor4u == llcolor4ua, "setValue(LLSD)/getValue Failed ");
    
}

TEST_CASE("test_3")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4u(r,g,b,a);
        llcolor4u.setToBlack();
        CHECK_MESSAGE(((0 == llcolor4u.mV[VRED], "setToBlack:Fail to set black ") && (0 == llcolor4u.mV[VGREEN]) && (0 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));

        llcolor4u.setToWhite();
        CHECK_MESSAGE(((255 == llcolor4u.mV[VRED], "setToWhite:Fail to white ") && (255 == llcolor4u.mV[VGREEN]) && (255 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));
    
}

TEST_CASE("test_4")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4ua(r,g,b,a);
        LLSD sd = llcolor4ua.getValue();
        LLColor4U llcolor4u = (LLColor4U)sd;
        CHECK_MESSAGE(llcolor4u == llcolor4ua, "Operator=(LLSD) Failed ");
    
}

TEST_CASE("test_5")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
        LLColor4U llcolor4u;
        llcolor4u.setVec(r,g,b,a);
        CHECK_MESSAGE(((r == llcolor4u.mV[VRED], "1:setVec:Fail to set the values ") && (g == llcolor4u.mV[VGREEN]) && (b == llcolor4u.mV[VBLUE])&& (a == llcolor4u.mV[VALPHA])));

        llcolor4u.setToBlack();
        llcolor4u.setVec(r,g,b);
        CHECK_MESSAGE(((r == llcolor4u.mV[VRED], "2:setVec:Fail to set the values ") && (g == llcolor4u.mV[VGREEN]) && (b == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));

        LLColor4U llcolor4u1;
        llcolor4u1.setVec(llcolor4u);
        CHECK_MESSAGE(llcolor4u1 == llcolor4u, "3:setVec:Fail to set the values ");

        const U8 vec[4] = {0x12,0xFF,0xAF,0x23
}

TEST_CASE("test_6")
{

        U8 alpha = 0x12;
        LLColor4U llcolor4u;
        llcolor4u.setAlpha(alpha);
        CHECK_MESSAGE((alpha == llcolor4u.mV[VALPHA], "setAlpha:Fail to set alpha value "));
    
}

TEST_CASE("test_7")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF;
        LLColor4U llcolor4u(r,g,b);
        CHECK_MESSAGE(is_approx_equal(llcolor4u.magVecSquared(, "magVecSquared:Fail "), (F32)(r*r + g*g + b*b)));
        CHECK_MESSAGE(is_approx_equal(llcolor4u.magVec(, "magVec:Fail "), (F32) sqrt((F32) (r*r + g*g + b*b))));
    
}

TEST_CASE("test_8")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF;
        std::ostringstream stream1, stream2;
        LLColor4U llcolor4u1(r,g,b),llcolor4u2;
        stream1 << llcolor4u1;
        llcolor4u2.setVec(r,g,b);
        stream2 << llcolor4u2;
        CHECK_MESSAGE((stream1.str(, "operator << failed ") == stream2.str()));
    
}

TEST_CASE("test_9")
{

        U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
        U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
        LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
        llcolor4u3 = llcolor4u1 + llcolor4u2;
        CHECK_MESSAGE(llcolor4u3.mV[VRED] == (U8, "1a.operator+:Fail to Add the values ")(r1+r2));
        CHECK_MESSAGE(llcolor4u3.mV[VGREEN] == (U8, "1b.operator+:Fail to Add the values ")(g1+g2));
        CHECK_MESSAGE(llcolor4u3.mV[VBLUE] == (U8, "1c.operator+:Fail to Add the values ")(b1+b2));

        llcolor4u2 += llcolor4u1;
        CHECK_MESSAGE(llcolor4u2.mV[VRED] == (U8, "2a.operator+=:Fail to Add the values ")(r1+r2));
        CHECK_MESSAGE(llcolor4u2.mV[VGREEN] == (U8, "2b.operator+=:Fail to Add the values ")(g1+g2));
        CHECK_MESSAGE(llcolor4u2.mV[VBLUE] == (U8, "2c.operator+=:Fail to Add the values ")(b1+b2));
    
}

TEST_CASE("test_10")
{

        U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
        U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
        LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
        llcolor4u3 = llcolor4u1 - llcolor4u2;
        CHECK_MESSAGE(llcolor4u3.mV[VRED] == (U8, "1a. operator-:Fail to Add the values ")(r1-r2));
        CHECK_MESSAGE(llcolor4u3.mV[VGREEN] == (U8, "1b. operator-:Fail to Add the values ")(g1-g2));
        CHECK_MESSAGE(llcolor4u3.mV[VBLUE] == (U8, "1c. operator-:Fail to Add the values ")(b1-b2));

        llcolor4u1 -= llcolor4u2;
        CHECK_MESSAGE(llcolor4u1.mV[VRED] == (U8, "2a. operator-=:Fail to Add the values ")(r1-r2));
        CHECK_MESSAGE(llcolor4u1.mV[VGREEN] == (U8, "2b. operator-=:Fail to Add the values ")(g1-g2));
        CHECK_MESSAGE(llcolor4u1.mV[VBLUE] == (U8, "2c. operator-=:Fail to Add the values ")(b1-b2));
    
}

TEST_CASE("test_11")
{

        U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
        U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
        LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
        llcolor4u3 = llcolor4u1 * llcolor4u2;
        CHECK_MESSAGE(llcolor4u3.mV[VRED] == (U8, "1a. operator*:Fail to multiply the values")(r1*r2));
        CHECK_MESSAGE(llcolor4u3.mV[VGREEN] == (U8, "1b. operator*:Fail to multiply the values")(g1*g2));
        CHECK_MESSAGE(llcolor4u3.mV[VBLUE] == (U8, "1c. operator*:Fail to multiply the values")(b1*b2));

        U8 mulVal = 123;
        llcolor4u1 *= mulVal;
        CHECK_MESSAGE(llcolor4u1.mV[VRED] == (U8, "2a. operator*=:Fail to multiply the values")(r1*mulVal));
        CHECK_MESSAGE(llcolor4u1.mV[VGREEN] == (U8, "2b. operator*=:Fail to multiply the values")(g1*mulVal));
        CHECK_MESSAGE(llcolor4u1.mV[VBLUE] == (U8, "2c. operator*=:Fail to multiply the values")(b1*mulVal));
    
}

TEST_CASE("test_12")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF;
        LLColor4U llcolor4u(r,g,b),llcolor4u1;
        llcolor4u1 = llcolor4u;
        CHECK_MESSAGE((llcolor4u1 == llcolor4u, "operator== failed to ensure the equality "));
        llcolor4u1.setToBlack();
        CHECK_MESSAGE((llcolor4u1 != llcolor4u, "operator!= failed to ensure the equality "));
    
}

TEST_CASE("test_13")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 12;
        LLColor4U llcolor4u(r,g,b,a);
        U8 modVal = 45;
        llcolor4u %= modVal;
        CHECK_MESSAGE(llcolor4u.mV[VALPHA] == (U8, "operator%=:Fail ")(a * modVal));
    
}

TEST_CASE("test_14")
{

        U8 r = 0x12, g = 0xFF, b = 0xAF, a = 12;
        LLColor4U llcolor4u1(r,g,b,a);
        std::string color("12, 23, 132, 50");
        LLColor4U::parseColor4U(color, &llcolor4u1);
        CHECK_MESSAGE(((12 == llcolor4u1.mV[VRED], "parseColor4U() failed to parse the color value ") && (23 == llcolor4u1.mV[VGREEN]) && (132 == llcolor4u1.mV[VBLUE])&& (50 == llcolor4u1.mV[VALPHA])));

        color = "12, 23, 132";
        CHECK_MESSAGE((false == LLColor4U::parseColor4U(color, &llcolor4u1, "2:parseColor4U() failed to parse the color value ")));

        color = "12";
        CHECK_MESSAGE((false == LLColor4U::parseColor4U(color, &llcolor4u1, "2:parseColor4U() failed to parse the color value ")));
    
}

TEST_CASE("test_15")
{

        U8 r = 12, g = 123, b = 3, a = 2;
        LLColor4U llcolor4u(r,g,b,a),llcolor4u1;
        const F32 fVal = 3.f;
        llcolor4u1 = llcolor4u.multAll(fVal);
        CHECK_MESSAGE((((U8, "multAll:Fail to multiply ")ll_round(r * fVal) == llcolor4u1.mV[VRED]) && (U8)ll_round(g * fVal) == llcolor4u1.mV[VGREEN]
                                            && ((U8)ll_round(b * fVal) == llcolor4u1.mV[VBLUE])&& ((U8)ll_round(a * fVal) == llcolor4u1.mV[VALPHA])));
    
}

TEST_CASE("test_16")
{

        U8 r1 = 12, g1 = 123, b1 = 3, a1 = 2;
        U8 r2 = 23, g2 = 230, b2 = 124, a2 = 255;
        LLColor4U llcolor4u(r1,g1,b1,a1),llcolor4u1(r2,g2,b2,a2);
        llcolor4u1 = llcolor4u1.addClampMax(llcolor4u);
        CHECK_MESSAGE(((r1+r2 == llcolor4u1.mV[VRED], "1:addClampMax():Fail to add the value ") && (255 == llcolor4u1.mV[VGREEN]) && (b1+b2 == llcolor4u1.mV[VBLUE])&& (255 == llcolor4u1.mV[VALPHA])));

        r1 = 132, g1 = 3, b1 = 3, a1 = 2;
        r2 = 123, g2 = 230, b2 = 154, a2 = 25;
        LLColor4U llcolor4u2(r1,g1,b1,a1),llcolor4u3(r2,g2,b2,a2);
        llcolor4u3 = llcolor4u3.addClampMax(llcolor4u2);
        CHECK_MESSAGE(((255 == llcolor4u3.mV[VRED], "2:addClampMax():Fail to add the value ") && (g1+g2 == llcolor4u3.mV[VGREEN]) && (b1+b2 == llcolor4u3.mV[VBLUE])&& (a1+a2 == llcolor4u3.mV[VALPHA])));
    
}

TEST_CASE("test_17")
{

        F32 r = 23.f, g = 12.32f, b = -12.3f;
        LLColor3 color3(r,g,b);
        LLColor4U llcolor4u;
        llcolor4u.setVecScaleClamp(color3);
        const S32 MAX_COLOR = 255;
        F32 color_scale_factor = MAX_COLOR/r;
        S32 r2 = ll_round(r * color_scale_factor);
        S32 g2 = ll_round(g * color_scale_factor);
        CHECK_MESSAGE(((r2 == llcolor4u.mV[VRED], "setVecScaleClamp():Fail to add the value ") && (g2 == llcolor4u.mV[VGREEN]) && (0 == llcolor4u.mV[VBLUE])&& (255 == llcolor4u.mV[VALPHA])));
    
}

} // TEST_SUITE

