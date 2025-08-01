/**
 * @file v3color_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v3color test cases.
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

#include "../v3color.h"


TEST_SUITE("v3color_h") {

TEST_CASE("test_1")
{

        LLColor3 llcolor3;
        CHECK_MESSAGE((0.0f == llcolor3.mV[0], "1:LLColor3:Fail to default-initialize ") && (0.0f == llcolor3.mV[1]) && (0.0f == llcolor3.mV[2]));
        F32 r = 2.0f, g = 3.2f, b = 1.f;
        F32 v1,v2,v3;
        LLColor3 llcolor3a(r,g,b);
        ensure("2:LLColor3:Fail to initialize " ,(2.0f == llcolor3a.mV[0]) && (3.2f == llcolor3a.mV[1]) && (1.f == llcolor3a.mV[2]));

        const F32 vec[3] = {2.0f, 3.2f,1.f
}

TEST_CASE("test_2")
{

        LLColor3 llcolor3;
        llcolor3.setToBlack();
        CHECK_MESSAGE(((llcolor3.mV[0] == 0.f, "setToBlack:Fail to set black ") && (llcolor3.mV[1] == 0.f) && (llcolor3.mV[2] == 0.f)));
        llcolor3.setToWhite();
        CHECK_MESSAGE(((llcolor3.mV[0] == 1.f, "setToWhite:Fail to set white  ") && (llcolor3.mV[1] == 1.f) && (llcolor3.mV[2] == 1.f)));
    
}

TEST_CASE("test_3")
{

        F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
        LLColor3 llcolor3, llcolor3a;
        llcolor3.setVec(r,g,b);
        CHECK_MESSAGE(((r == llcolor3.mV[0], "1:setVec(r,g,b) Fail ") && (g == llcolor3.mV[1]) && (b == llcolor3.mV[2])));
        llcolor3a.setVec(llcolor3);
        CHECK_MESSAGE(llcolor3 == llcolor3a, "2:setVec(LLColor3) Fail ");
        F32 vec[3] = {1.2324f, 2.45634f, .234563f
}

TEST_CASE("test_4")
{

        F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
        LLColor3 llcolor3(r,g,b);
        CHECK_MESSAGE(is_approx_equal(llcolor3.magVecSquared(, "magVecSquared:Fail "), (r*r + g*g + b*b)));
        CHECK_MESSAGE(is_approx_equal(llcolor3.magVec(, "magVec:Fail "), (F32) sqrt(r*r + g*g + b*b)));
    
}

TEST_CASE("test_5")
{

        F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
        F32 val1, val2,val3;
        LLColor3 llcolor3(r,g,b);
        F32 vecMag = llcolor3.normVec();
        F32 mag = (F32) sqrt(r*r + g*g + b*b);
        F32 oomag = 1.f / mag;
        val1 = r * oomag;
        val2 = g * oomag;
        val3 = b * oomag;
        CHECK_MESSAGE((is_approx_equal(val1, llcolor3.mV[0], "1:normVec failed ") && is_approx_equal(val2, llcolor3.mV[1]) && is_approx_equal(val3, llcolor3.mV[2]) && is_approx_equal(vecMag, mag)));
        r = .000000000f, g = 0.f, b = 0.0f;
        llcolor3.setVec(r,g,b);
        vecMag = llcolor3.normVec();
        CHECK_MESSAGE((0. == llcolor3.mV[0] && 0. == llcolor3.mV[1] && 0. == llcolor3.mV[2] && vecMag == 0., "2:normVec failed should be 0. "));
    
}

TEST_CASE("test_6")
{

        F32 r = 2.3436212f, g = -1231.f, b = .7849321232f;
        std::ostringstream stream1, stream2;
        LLColor3 llcolor3(r,g,b),llcolor3a;
        stream1 << llcolor3;
        llcolor3a.setVec(r,g,b);
        stream2 << llcolor3a;
        CHECK_MESSAGE((stream1.str(, "operator << failed ") == stream2.str()));
    
}

TEST_CASE("test_7")
{

        F32 r = 2.3436212f, g = -1231.f, b = .7849321232f;
        LLColor3 llcolor3(r,g,b),llcolor3a;
        llcolor3a = llcolor3;
        CHECK_MESSAGE((llcolor3a == llcolor3, "operator == failed "));
    
}

TEST_CASE("test_8")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2),llcolor3b;
        llcolor3b = llcolor3 + llcolor3a ;
        CHECK_MESSAGE(is_approx_equal(r1+r2 ,llcolor3b.mV[0], "1:operator+ failed") && is_approx_equal(g1+g2,llcolor3b.mV[1])&& is_approx_equal(b1+b2,llcolor3b.mV[2]));
        r1 = -.235f, g1 = -24.32f, b1 = 2.13f,  r2 = -2.3f, g2 = 1.f, b2 = 34.21f;
        llcolor3.setVec(r1,g1,b1);
        llcolor3a.setVec(r2,g2,b2);
        llcolor3b = llcolor3 + llcolor3a;
        CHECK_MESSAGE(is_approx_equal(r1+r2 ,llcolor3b.mV[0], "2:operator+ failed") && is_approx_equal(g1+g2,llcolor3b.mV[1])&& is_approx_equal(b1+b2,llcolor3b.mV[2]));
    
}

TEST_CASE("test_9")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2),llcolor3b;
        llcolor3b = llcolor3 - llcolor3a ;
        CHECK_MESSAGE(is_approx_equal(r1-r2 ,llcolor3b.mV[0], "1:operator- failed") && is_approx_equal(g1-g2,llcolor3b.mV[1])&& is_approx_equal(b1-b2,llcolor3b.mV[2]));
        r1 = -.235f, g1 = -24.32f, b1 = 2.13f,  r2 = -2.3f, g2 = 1.f, b2 = 34.21f;
        llcolor3.setVec(r1,g1,b1);
        llcolor3a.setVec(r2,g2,b2);
        llcolor3b = llcolor3 - llcolor3a;
        CHECK_MESSAGE(is_approx_equal(r1-r2 ,llcolor3b.mV[0], "2:operator- failed") && is_approx_equal(g1-g2,llcolor3b.mV[1])&& is_approx_equal(b1-b2,llcolor3b.mV[2]));
    
}

TEST_CASE("test_10")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2),llcolor3b;
        llcolor3b = llcolor3 * llcolor3a;
        CHECK_MESSAGE(is_approx_equal(r1*r2 ,llcolor3b.mV[0], "1:operator* failed") && is_approx_equal(g1*g2,llcolor3b.mV[1])&& is_approx_equal(b1*b2,llcolor3b.mV[2]));
        llcolor3a.setToBlack();
        F32 mulVal = 4.332f;
        llcolor3a = llcolor3 * mulVal;
        CHECK_MESSAGE(is_approx_equal(r1*mulVal ,llcolor3a.mV[0], "2:operator* failed") && is_approx_equal(g1*mulVal,llcolor3a.mV[1])&& is_approx_equal(b1*mulVal,llcolor3a.mV[2]));
        llcolor3a.setToBlack();
        llcolor3a = mulVal * llcolor3;
        CHECK_MESSAGE(is_approx_equal(r1*mulVal ,llcolor3a.mV[0], "3:operator* failed") && is_approx_equal(g1*mulVal,llcolor3a.mV[1])&& is_approx_equal(b1*mulVal,llcolor3a.mV[2]));
    
}

TEST_CASE("test_11")
{

        F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
        LLColor3 llcolor3(r,g,b),llcolor3a;
        llcolor3a = -llcolor3;
        CHECK_MESSAGE((-llcolor3a == llcolor3, "operator- failed "));
    
}

TEST_CASE("test_12")
{

        F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
        LLColor3 llcolor3(r,g,b),llcolor3a(r,g,b);
        CHECK_MESSAGE(llcolor3a == llcolor3, "1:operator== failed");
        r = 13.3436212f, g = -11.f, b = .7849321232f;
        llcolor3.setVec(r,g,b);
        llcolor3a.setVec(r,g,b);
        CHECK_MESSAGE(llcolor3a == llcolor3, "2:operator== failed");
    
}

TEST_CASE("test_13")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
        CHECK_MESSAGE((llcolor3 != llcolor3a, "1:operator!= failed"));
        llcolor3.setToBlack();
        llcolor3a.setVec(llcolor3);
        CHECK_MESSAGE(( false == (llcolor3a != llcolor3, "2:operator!= failed")));
    
}

TEST_CASE("test_14")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
        llcolor3a += llcolor3;
        CHECK_MESSAGE(is_approx_equal(r1+r2 ,llcolor3a.mV[0], "1:operator+= failed") && is_approx_equal(g1+g2,llcolor3a.mV[1])&& is_approx_equal(b1+b2,llcolor3a.mV[2]));
        llcolor3.setVec(r1,g1,b1);
        llcolor3a.setVec(r2,g2,b2);
        llcolor3a += llcolor3;
        CHECK_MESSAGE(is_approx_equal(r1+r2 ,llcolor3a.mV[0], "2:operator+= failed") && is_approx_equal(g1+g2,llcolor3a.mV[1])&& is_approx_equal(b1+b2,llcolor3a.mV[2]));
    
}

TEST_CASE("test_15")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
        llcolor3a -= llcolor3;
        CHECK_MESSAGE(is_approx_equal(r2-r1, llcolor3a.mV[0], "1:operator-= failed"));
        CHECK_MESSAGE(is_approx_equal(g2-g1, llcolor3a.mV[1], "2:operator-= failed"));
        CHECK_MESSAGE(is_approx_equal(b2-b1, llcolor3a.mV[2], "3:operator-= failed"));
        llcolor3.setVec(r1,g1,b1);
        llcolor3a.setVec(r2,g2,b2);
        llcolor3a -= llcolor3;
        CHECK_MESSAGE(is_approx_equal(r2-r1, llcolor3a.mV[0], "4:operator-= failed"));
        CHECK_MESSAGE(is_approx_equal(g2-g1, llcolor3a.mV[1], "5:operator-= failed"));
        CHECK_MESSAGE(is_approx_equal(b2-b1, llcolor3a.mV[2], "6:operator-= failed"));
    
}

TEST_CASE("test_16")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
        llcolor3a *= llcolor3;
        CHECK_MESSAGE(is_approx_equal(r1*r2 ,llcolor3a.mV[0], "1:operator*= failed") && is_approx_equal(g1*g2,llcolor3a.mV[1])&& is_approx_equal(b1*b2,llcolor3a.mV[2]));
        F32 mulVal = 4.332f;
        llcolor3 *=mulVal;
        CHECK_MESSAGE(is_approx_equal(r1*mulVal ,llcolor3.mV[0], "2:operator*= failed") && is_approx_equal(g1*mulVal,llcolor3.mV[1])&& is_approx_equal(b1*mulVal,llcolor3.mV[2]));
    
}

TEST_CASE("test_17")
{

        F32 r = 2.3436212f, g = -1231.f, b = .7849321232f;
        LLColor3 llcolor3(r,g,b);
        llcolor3.clamp();
        ensure("1:clamp:Fail to clamp " ,(1.0f == llcolor3.mV[0]) && (0.f == llcolor3.mV[1]) && (b == llcolor3.mV[2]));
        r = -2.3436212f, g = -1231.f, b = 67.7849321232f;
        llcolor3.setVec(r,g,b);
        llcolor3.clamp();
        ensure("2:clamp:Fail to clamp " ,(0.f == llcolor3.mV[0]) && (0.f == llcolor3.mV[1]) && (1.f == llcolor3.mV[2]));
    
}

TEST_CASE("test_18")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        F32 val = 2.3f,val1,val2,val3;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
        val1 = r1 + (r2 - r1)* val;
        val2 = g1 + (g2 - g1)* val;
        val3 = b1 + (b2 - b1)* val;
        LLColor3 llcolor3b = lerp(llcolor3,llcolor3a,val);
        CHECK_MESSAGE(((val1 ==llcolor3b.mV[0], "lerp failed ")&& (val2 ==llcolor3b.mV[1]) && (val3 ==llcolor3b.mV[2])));
    
}

TEST_CASE("test_19")
{

        F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
        LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
        F32 val = distVec(llcolor3,llcolor3a);
        CHECK_MESSAGE(is_approx_equal((F32, "distVec failed ") sqrt((r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2)) ,val));

        F32 val1 = distVec_squared(llcolor3,llcolor3a);
        CHECK_MESSAGE(is_approx_equal(((r1-r2, "distVec_squared failed ")*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2)) ,val1));
    
}

TEST_CASE("test_20")
{

        F32 r1 = 1.02223f, g1 = 22222.212f, b1 = 122222.00002f;
        LLColor3 llcolor31(r1,g1,b1);

        LLSD sd = llcolor31.getValue();
        LLColor3 llcolor32;
        llcolor32.setValue(sd);
        CHECK_MESSAGE(llcolor31 == llcolor32, "LLColor3::setValue/getValue failed");

        LLColor3 llcolor33(sd);
        CHECK_MESSAGE(llcolor31 == llcolor33, "LLColor3(LLSD) failed");
    
}

} // TEST_SUITE

