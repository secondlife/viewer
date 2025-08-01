/**
 * @file v4color_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v4color test cases.
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

#include "../v4coloru.h"
#include "llsd.h"
#include "../v3color.h"
#include "../v4color.h"


TEST_SUITE("v4color_h") {

TEST_CASE("test_1")
{

        LLColor4 llcolor4;
        CHECK_MESSAGE(((0 == llcolor4.mV[VRED], "1:LLColor4:Fail to initialize ") && (0 == llcolor4.mV[VGREEN]) && (0 == llcolor4.mV[VBLUE])&& (1.0f == llcolor4.mV[VALPHA])));

        F32 r = 0x20, g = 0xFFFF, b = 0xFF, a = 0xAF;
        LLColor4 llcolor4a(r,g,b);
        CHECK_MESSAGE(((r == llcolor4a.mV[VRED], "2:LLColor4:Fail to initialize ") && (g == llcolor4a.mV[VGREEN]) && (b == llcolor4a.mV[VBLUE])&& (1.0f == llcolor4a.mV[VALPHA])));

        LLColor4 llcolor4b(r,g,b,a);
        CHECK_MESSAGE(((r == llcolor4b.mV[VRED], "3:LLColor4:Fail to initialize ") && (g == llcolor4b.mV[VGREEN]) && (b == llcolor4b.mV[VBLUE])&& (a == llcolor4b.mV[VALPHA])));

        const F32 vec[4] = {.112f ,23.2f, -4.2f, -.0001f
}

TEST_CASE("test_2")
{

        LLColor4 llcolor(1.0, 2.0, 3.0, 4.0);
        LLSD llsd = llcolor.getValue();
        LLColor4 llcolor4(llsd), llcolor4a;
        llcolor4a.setValue(llsd);
        CHECK_MESSAGE((llcolor4 == llcolor4a, "setValue: failed"));
        LLSD sd = llcolor4a.getValue();
        LLColor4 llcolor4b(sd);
        CHECK_MESSAGE((llcolor4b == llcolor4a, "getValue: Failed "));
    
}

TEST_CASE("test_3")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 0xAF;
        LLColor4 llcolor4(r,g,b,a);
        llcolor4.setToBlack();
        CHECK_MESSAGE(((0 == llcolor4.mV[VRED], "setToBlack:Fail to set the black ") && (0 == llcolor4.mV[VGREEN]) && (0 == llcolor4.mV[VBLUE])&& (1.0f == llcolor4.mV[VALPHA])));

        llcolor4.setToWhite();
        CHECK_MESSAGE(((1.f == llcolor4.mV[VRED], "setToWhite:Fail to set the white ") && (1.f == llcolor4.mV[VGREEN]) && (1.f == llcolor4.mV[VBLUE])&& (1.0f == llcolor4.mV[VALPHA])));
    
}

TEST_CASE("test_4")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF, a = 0xAF;
        LLColor4 llcolor4;
        llcolor4.setVec(r,g,b);
        CHECK_MESSAGE(((r == llcolor4.mV[VRED], "1:setVec:Fail to set the values ") && (g == llcolor4.mV[VGREEN]) && (b == llcolor4.mV[VBLUE])&& (1.f == llcolor4.mV[VALPHA])));

        llcolor4.setVec(r,g,b,a);
        CHECK_MESSAGE(((r == llcolor4.mV[VRED], "2:setVec:Fail to set the values ") && (g == llcolor4.mV[VGREEN]) && (b == llcolor4.mV[VBLUE])&& (a == llcolor4.mV[VALPHA])));

        LLColor4 llcolor4a;
        llcolor4a.setVec(llcolor4);
        CHECK_MESSAGE(llcolor4a == llcolor4, "3:setVec:Fail to set the values ");

        LLColor3 llcolor3(-2.23f,1.01f,42.3f);
        llcolor4a.setVec(llcolor3);
        CHECK_MESSAGE(((llcolor3.mV[VRED] == llcolor4a.mV[VRED], "4:setVec:Fail to set the values ") && (llcolor3.mV[VGREEN] == llcolor4a.mV[VGREEN]) && (llcolor3.mV[VBLUE] == llcolor4a.mV[VBLUE])));

        F32 val = -.33f;
        llcolor4a.setVec(llcolor3,val);
        CHECK_MESSAGE(((llcolor3.mV[VRED] == llcolor4a.mV[VRED], "4:setVec:Fail to set the values ") && (llcolor3.mV[VGREEN] == llcolor4a.mV[VGREEN]) && (llcolor3.mV[VBLUE] == llcolor4a.mV[VBLUE]) && (val == llcolor4a.mV[VALPHA])));

        const F32 vec[4] = {.112f ,23.2f, -4.2f, -.0001f
}

TEST_CASE("test_5")
{

        F32 alpha = 0xAF;
        LLColor4 llcolor4;
        llcolor4.setAlpha(alpha);
        CHECK_MESSAGE((alpha == llcolor4.mV[VALPHA], "setAlpha:Fail to initialize "));
    
}

TEST_CASE("test_6")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF;
        LLColor4 llcolor4(r,g,b);
        CHECK_MESSAGE(is_approx_equal(llcolor4.magVecSquared(, "magVecSquared:Fail "), (r*r + g*g + b*b)));
        CHECK_MESSAGE(is_approx_equal(llcolor4.magVec(, "magVec:Fail "), (F32) sqrt(r*r + g*g + b*b)));
    
}

TEST_CASE("test_7")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF;
        LLColor4 llcolor4(r,g,b);
        F32 vecMag = llcolor4.normVec();
        F32 mag = (F32) sqrt(r*r + g*g + b*b);
        F32 oomag = 1.f / mag;
        F32 val1 = r * oomag, val2 = g * oomag, val3 = b * oomag;
        CHECK_MESSAGE((is_approx_equal(val1, llcolor4.mV[0], "1:normVec failed ") && is_approx_equal(val2, llcolor4.mV[1]) && is_approx_equal(val3, llcolor4.mV[2]) && is_approx_equal(vecMag, mag)));
    
}

TEST_CASE("test_8")
{

        LLColor4 llcolor4;
        CHECK_MESSAGE((1 == llcolor4.isOpaque(, "1:isOpaque failed ")));
        F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 1.f;
        llcolor4.setVec(r,g,b,a);
        CHECK_MESSAGE((1 == llcolor4.isOpaque(, "2:isOpaque failed ")));
        a = 2.f;
        llcolor4.setVec(r,g,b,a);
        CHECK_MESSAGE((0 == llcolor4.isOpaque(, "3:isOpaque failed ")));
    
}

TEST_CASE("test_9")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF;
        LLColor4 llcolor4(r,g,b);
        CHECK_MESSAGE(( r ==  llcolor4[0], "1:operator [] failed"));
        CHECK_MESSAGE(( g ==  llcolor4[1], "2:operator [] failed"));
        CHECK_MESSAGE(( b ==  llcolor4[2], "3:operator [] failed"));

        r = 0xA20, g = 0xFBFF, b = 0xFFF;
        llcolor4.setVec(r,g,b);
        F32 &ref1 = llcolor4[0];
        CHECK_MESSAGE(( ref1 ==  llcolor4[0], "4:operator [] failed"));
        F32 &ref2 = llcolor4[1];
        CHECK_MESSAGE(( ref2 ==  llcolor4[1], "5:operator [] failed"));
        F32 &ref3 = llcolor4[2];
        CHECK_MESSAGE(( ref3 ==  llcolor4[2], "6:operator [] failed"));
    
}

TEST_CASE("test_10")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF;
        LLColor3 llcolor3(r,g,b);
        LLColor4 llcolor4a,llcolor4b;
        llcolor4a = llcolor3;
        CHECK_MESSAGE(((llcolor3.mV[0] == llcolor4a.mV[VRED], "Operator=:Fail to initialize ") && (llcolor3.mV[1] == llcolor4a.mV[VGREEN]) && (llcolor3.mV[2] == llcolor4a.mV[VBLUE])));
        LLSD sd = llcolor4a.getValue();
        llcolor4b = LLColor4(sd);
        CHECK_MESSAGE(llcolor4a == llcolor4b, "Operator= LLSD:Fail ");
    
}

TEST_CASE("test_11")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF;
        std::ostringstream stream1, stream2;
        LLColor4 llcolor4a(r,g,b),llcolor4b;
        stream1 << llcolor4a;
        llcolor4b.setVec(r,g,b);
        stream2 << llcolor4b;
        CHECK_MESSAGE((stream1.str(, "operator << failed ") == stream2.str()));
    
}

TEST_CASE("test_12")
{

        F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF;
        F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
        LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
        llcolor4c = llcolor4b + llcolor4a;
        CHECK_MESSAGE((is_approx_equal(r1+r2,llcolor4c.mV[VRED], "operator+:Fail to Add the values ") && is_approx_equal(g1+g2,llcolor4c.mV[VGREEN]) && is_approx_equal(b1+b2,llcolor4c.mV[VBLUE])));

        llcolor4b += llcolor4a;
        CHECK_MESSAGE((is_approx_equal(r1+r2,llcolor4b.mV[VRED], "operator+=:Fail to Add the values ") && is_approx_equal(g1+g2,llcolor4b.mV[VGREEN]) && is_approx_equal(b1+b2,llcolor4b.mV[VBLUE])));
    
}

TEST_CASE("test_13")
{

        F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF;
        F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
        LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
        llcolor4c = llcolor4a - llcolor4b;
        CHECK_MESSAGE((is_approx_equal(r1-r2,llcolor4c.mV[VRED], "operator-:Fail to subtract the values ") && is_approx_equal(g1-g2,llcolor4c.mV[VGREEN]) && is_approx_equal(b1-b2,llcolor4c.mV[VBLUE])));

        llcolor4a -= llcolor4b;
        CHECK_MESSAGE((is_approx_equal(r1-r2,llcolor4a.mV[VRED], "operator-=:Fail to subtract the values ") && is_approx_equal(g1-g2,llcolor4a.mV[VGREEN]) && is_approx_equal(b1-b2,llcolor4a.mV[VBLUE])));
    
}

TEST_CASE("test_14")
{

        F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF;
        F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
        LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
        llcolor4c = llcolor4a * llcolor4b;
        CHECK_MESSAGE((is_approx_equal(r1*r2,llcolor4c.mV[VRED], "1:operator*:Fail to multiply the values") && is_approx_equal(g1*g2,llcolor4c.mV[VGREEN]) && is_approx_equal(b1*b2,llcolor4c.mV[VBLUE])));

        F32 mulVal = 3.33f;
        llcolor4c = llcolor4a * mulVal;
        CHECK_MESSAGE((is_approx_equal(r1*mulVal,llcolor4c.mV[VRED], "2:operator*:Fail ") && is_approx_equal(g1*mulVal,llcolor4c.mV[VGREEN]) && is_approx_equal(b1*mulVal,llcolor4c.mV[VBLUE])));
        llcolor4c = mulVal * llcolor4a;
        CHECK_MESSAGE((is_approx_equal(r1*mulVal,llcolor4c.mV[VRED], "3:operator*:Fail to multiply the values") && is_approx_equal(g1*mulVal,llcolor4c.mV[VGREEN]) && is_approx_equal(b1*mulVal,llcolor4c.mV[VBLUE])));

        llcolor4a *= mulVal;
        CHECK_MESSAGE((is_approx_equal(r1*mulVal,llcolor4a.mV[VRED], "4:operator*=:Fail to multiply the values ") && is_approx_equal(g1*mulVal,llcolor4a.mV[VGREEN]) && is_approx_equal(b1*mulVal,llcolor4a.mV[VBLUE])));

        LLColor4 llcolor4d(r1,g1,b1),llcolor4e(r2,g2,b2);
        llcolor4e *= llcolor4d;
        CHECK_MESSAGE((is_approx_equal(r1*r2,llcolor4e.mV[VRED], "5:operator*=:Fail to multiply the values ") && is_approx_equal(g1*g2,llcolor4e.mV[VGREEN]) && is_approx_equal(b1*b2,llcolor4e.mV[VBLUE])));
    
}

TEST_CASE("test_15")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 0x30;
        F32 div = 12.345f;
        LLColor4 llcolor4a(r,g,b,a),llcolor4b;
        llcolor4b = llcolor4a % div;//chnage only alpha value nor r,g,b;
        CHECK_MESSAGE((is_approx_equal(r,llcolor4b.mV[VRED], "1operator%:Fail ") && is_approx_equal(g,llcolor4b.mV[VGREEN]) && is_approx_equal(b,llcolor4b.mV[VBLUE])&& is_approx_equal(div*a,llcolor4b.mV[VALPHA])));

        llcolor4b = div % llcolor4a;
        CHECK_MESSAGE((is_approx_equal(r,llcolor4b.mV[VRED], "2operator%:Fail ") && is_approx_equal(g,llcolor4b.mV[VGREEN]) && is_approx_equal(b,llcolor4b.mV[VBLUE])&& is_approx_equal(div*a,llcolor4b.mV[VALPHA])));

        llcolor4a %= div;
        CHECK_MESSAGE((is_approx_equal(a*div,llcolor4a.mV[VALPHA], "operator%=:Fail ")));
    
}

TEST_CASE("test_16")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 0x30;
        LLColor4 llcolor4a(r,g,b,a),llcolor4b;
        llcolor4b = llcolor4a;
        CHECK_MESSAGE((llcolor4b == llcolor4a, "1:operator== failed to ensure the equality "));
        F32 r1 = 0x2, g1 = 0xFF, b1 = 0xFA;
        LLColor3 llcolor3(r1,g1,b1);
        llcolor4b = llcolor3;
        CHECK_MESSAGE((llcolor4b == llcolor3, "2:operator== failed to ensure the equality "));
        CHECK_MESSAGE((llcolor4a != llcolor3, "2:operator!= failed to ensure the equality "));
    
}

TEST_CASE("test_17")
{

        F32 r = 0x20, g = 0xFFFF, b = 0xFF;
        LLColor4 llcolor4a(r,g,b),llcolor4b;
        LLColor3 llcolor3 = vec4to3(llcolor4a);
        CHECK_MESSAGE((is_approx_equal(llcolor3.mV[VRED],llcolor4a.mV[VRED], "vec4to3:Fail to convert vec4 to vec3 ") && is_approx_equal(llcolor3.mV[VGREEN],llcolor4a.mV[VGREEN]) && is_approx_equal(llcolor3.mV[VBLUE],llcolor4a.mV[VBLUE])));
        llcolor4b = vec3to4(llcolor3);
        CHECK_MESSAGE(llcolor4b == llcolor4a, "vec3to4:Fail to convert vec3 to vec4 ");
    
}

TEST_CASE("test_18")
{

        F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF, val = 0x20;
        F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
        LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
        llcolor4c = lerp(llcolor4a,llcolor4b,val);
        CHECK_MESSAGE((is_approx_equal(r1 + (r2 - r1, "lerp:Fail ")* val,llcolor4c.mV[VRED]) && is_approx_equal(g1 + (g2 - g1)* val,llcolor4c.mV[VGREEN]) && is_approx_equal(b1 + (b2 - b1)* val,llcolor4c.mV[VBLUE])));
    
}

TEST_CASE("test_19")
{

        F32 r = 12.0f, g = -2.3f, b = 1.32f, a = 5.0f;
        LLColor4 llcolor4a(r,g,b,a),llcolor4b;
        std::string color("red");
        LLColor4::parseColor(color, &llcolor4b);
        CHECK_MESSAGE(llcolor4b == LLColor4::red, "1:parseColor() failed to parse the color value ");

        color = "12.0, -2.3, 1.32, 5.0";
        LLColor4::parseColor(color, &llcolor4b);
        llcolor4a = llcolor4a * (1.f / 255.f);
        CHECK_MESSAGE(llcolor4a == llcolor4b, "2:parseColor() failed to parse the color value ");

        color = "yellow5";
        llcolor4a.setVec(r,g,b);
        LLColor4::parseColor(color, &llcolor4a);
        CHECK_MESSAGE(llcolor4a == LLColor4::yellow5, "3:parseColor() failed to parse the color value ");
    
}

TEST_CASE("test_20")
{

        F32 r = 12.0f, g = -2.3f, b = 1.32f, a = 5.0f;
        LLColor4 llcolor4a(r,g,b,a),llcolor4b;
        std::string color("12.0, -2.3, 1.32, 5.0");
        LLColor4::parseColor4(color, &llcolor4b);
        CHECK_MESSAGE(llcolor4a == llcolor4b, "parseColor4() failed to parse the color value ");
    
}

} // TEST_SUITE

