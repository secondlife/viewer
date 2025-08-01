/**
 * @file v3math_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief v3math test cases.
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

#include "../v3dmath.h"
#include "../m3math.h"
#include "../v4math.h"
#include "../v3math.h"
#include "../llquaternion.h"
#include "../llquantize.h"


TEST_SUITE("v3math_h") {

TEST_CASE("test_1")
{

        LLVector3 vec3;
        CHECK_MESSAGE(((0.f == vec3.mV[VX], "1:LLVector3:Fail to initialize ") && (0.f == vec3.mV[VY]) && (0.f == vec3.mV[VZ])));
        F32 x = 2.32f, y = 1.212f, z = -.12f;
        LLVector3 vec3a(x,y,z);
        CHECK_MESSAGE(((2.32f == vec3a.mV[VX], "2:LLVector3:Fail to initialize ") && (1.212f == vec3a.mV[VY]) && (-.12f == vec3a.mV[VZ])));
        const F32 vec[3] = {1.2f ,3.2f, -4.2f
}

TEST_CASE("test_2")
{

        F32 x = 2.32f, y = 1.212f, z = -.12f;
        LLVector3 vec3(x,y,z);
        LLVector3d vector3d(vec3);
        LLVector3 vec3a(vector3d);
        CHECK_MESSAGE(vec3 == vec3a, "1:LLVector3:Fail to initialize ");
        LLVector4 vector4(vec3);
        LLVector3 vec3b(vector4);
        CHECK_MESSAGE(vec3 == vec3b, "2:LLVector3:Fail to initialize ");
    
}

TEST_CASE("test_3")
{

        S32 a = 231;
        LLSD llsd(a);
        LLVector3 vec3(llsd);
        LLSD sd = vec3.getValue();
        LLVector3 vec3a(sd);
        CHECK_MESSAGE((vec3 == vec3a, "1:LLVector3:Fail to initialize "));
    
}

TEST_CASE("test_4")
{

        S32 a = 231;
        LLSD llsd(a);
        LLVector3 vec3(llsd),vec3a;
        vec3a = vec3;
        ensure("1:Operator= Fail to initialize " ,(vec3 == vec3a));
    
}

TEST_CASE("test_5")
{

        F32 x = 2.32f, y = 1.212f, z = -.12f;
        LLVector3 vec3(x,y,z);
        CHECK_MESSAGE((true == vec3.isFinite(, "1:isFinite= Fail to initialize ")));//need more test cases:
        vec3.clearVec();
        CHECK_MESSAGE(((0.f == vec3.mV[VX], "2:clearVec:Fail to set values ") && (0.f == vec3.mV[VY]) && (0.f == vec3.mV[VZ])));
        vec3.setVec(x,y,z);
        CHECK_MESSAGE(((2.32f == vec3.mV[VX], "3:setVec:Fail to set values ") && (1.212f == vec3.mV[VY]) && (-.12f == vec3.mV[VZ])));
        vec3.zeroVec();
        CHECK_MESSAGE(((0.f == vec3.mV[VX], "4:zeroVec:Fail to set values ") && (0.f == vec3.mV[VY]) && (0.f == vec3.mV[VZ])));
    
}

TEST_CASE("test_6")
{

        F32 x = 2.32f, y = 1.212f, z = -.12f;
        LLVector3 vec3(x,y,z),vec3a;
        vec3.abs();
        CHECK_MESSAGE(((x == vec3.mV[VX], "1:abs:Fail ") && (y == vec3.mV[VY]) && (-z == vec3.mV[VZ])));
        vec3a.setVec(vec3);
        CHECK_MESSAGE((vec3a == vec3, "2:setVec:Fail to initialize "));
        const F32 vec[3] = {1.2f ,3.2f, -4.2f
}

TEST_CASE("test_7")
{

        F32 x = 2.32f, y = 3.212f, z = -.12f;
        F32 min = 0.0001f, max = 3.0f;
        LLVector3 vec3(x,y,z);
        CHECK_MESSAGE(true == vec3.clamp(min, max, "1:clamp:Fail  ") && x == vec3.mV[VX] && max == vec3.mV[VY] && min == vec3.mV[VZ]);
        x = 1.f, y = 2.2f, z = 2.8f;
        vec3.setVec(x,y,z);
        CHECK_MESSAGE(false == vec3.clamp(min, max, "2:clamp:Fail  "));
    
}

TEST_CASE("test_8")
{

        F32 x = 2.32f, y = 1.212f, z = -.12f;
        LLVector3 vec3(x,y,z);
        CHECK_MESSAGE(is_approx_equal(vec3.magVecSquared(, "1:magVecSquared:Fail "), (x*x + y*y + z*z)));
        CHECK_MESSAGE(is_approx_equal(vec3.magVec(, "2:magVec:Fail "), (F32) sqrt(x*x + y*y + z*z)));
    
}

TEST_CASE("test_9")
{

        F32 x =-2.0f, y = -3.0f, z = 1.23f ;
        LLVector3 vec3(x,y,z);
        CHECK_MESSAGE((true == vec3.abs(, "1:abs():Fail ")));
        CHECK_MESSAGE((false == vec3.isNull(, "2:isNull():Fail")));    //Returns true if vector has a _very_small_ length
        x =.00000001f, y = .000001001f, z = .000001001f;
        vec3.setVec(x,y,z);
        CHECK_MESSAGE((true == vec3.isNull(, "3:isNull(): Fail ")));
    
}

TEST_CASE("test_10")
{

        F32 x =-2.0f, y = -3.0f, z = 1.f ;
        LLVector3 vec3(x,y,z),vec3a;
        CHECK_MESSAGE((true == vec3a.isExactlyZero(, "1:isExactlyZero():Fail ")));
        vec3a = vec3a.scaleVec(vec3);
        CHECK_MESSAGE(vec3a.mV[VX] == 0.f && vec3a.mV[VY] == 0.f && vec3a.mV[VZ] == 0.f, "2:scaleVec: Fail ");
        vec3a.setVec(x,y,z);
        vec3a = vec3a.scaleVec(vec3);
        CHECK_MESSAGE(((4 == vec3a.mV[VX], "3:scaleVec: Fail ") && (9 == vec3a.mV[VY]) &&(1 == vec3a.mV[VZ])));
        CHECK_MESSAGE((false == vec3.isExactlyZero(, "4:isExactlyZero():Fail ")));
    
}

TEST_CASE("test_11")
{

        F32 x =20.0f, y = 30.0f, z = 15.f ;
        F32 angle = 100.f;
        LLVector3 vec3(x,y,z),vec3a(1.f,2.f,3.f);
        vec3a = vec3a.rotVec(angle, vec3);
        LLVector3 vec3b(1.f,2.f,3.f);
        vec3b = vec3b.rotVec(angle, vec3);
        ensure_equals("rotVec():Fail" ,vec3b,vec3a);
    
}

TEST_CASE("test_12")
{

        F32 x =-2.0f, y = -3.0f, z = 1.f ;
        LLVector3 vec3(x,y,z);
        CHECK_MESSAGE(( x ==  vec3[0], "1:operator [] failed"));
        CHECK_MESSAGE(( y ==  vec3[1], "2:operator [] failed"));
        CHECK_MESSAGE(( z ==  vec3[2], "3:operator [] failed"));

        vec3.clearVec();
        x = 23.f, y = -.2361f, z = 3.25;
        vec3.setVec(x,y,z);
        F32 &ref1 = vec3[0];
        CHECK_MESSAGE(( ref1 ==  vec3[0], "4:operator [] failed"));
        F32 &ref2 = vec3[1];
        CHECK_MESSAGE(( ref2 ==  vec3[1], "5:operator [] failed"));
        F32 &ref3 = vec3[2];
        CHECK_MESSAGE(( ref3 ==  vec3[2], "6:operator [] failed"));
    
}

TEST_CASE("test_13")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
        F32 val1, val2, val3;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2), vec3b;
        vec3b = vec3 + vec3a ;
        val1 = x1+x2;
        val2 = y1+y2;
        val3 = z1+z2;
        CHECK_MESSAGE((val1 == vec3b.mV[VX], "1:operator+ failed") && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ]));

        vec3.clearVec();
        vec3a.clearVec();
        vec3b.clearVec();
        x1 = -.235f, y1 = -24.32f,z1 = 2.13f,  x2 = -2.3f, y2 = 1.f, z2 = 34.21f;
        vec3.setVec(x1,y1,z1);
        vec3a.setVec(x2,y2,z2);
        vec3b = vec3 + vec3a;
        val1 = x1+x2;
        val2 = y1+y2;
        val3 = z1+z2;
        CHECK_MESSAGE((val1 == vec3b.mV[VX], "2:operator+ failed") && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ]));
    
}

TEST_CASE("test_14")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
        F32 val1, val2, val3;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2), vec3b;
        vec3b = vec3 - vec3a ;
        val1 = x1-x2;
        val2 = y1-y2;
        val3 = z1-z2;
        CHECK_MESSAGE((val1 == vec3b.mV[VX], "1:operator- failed") && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ]));

        vec3.clearVec();
        vec3a.clearVec();
        vec3b.clearVec();
        x1 = -.235f, y1 = -24.32f,z1 = 2.13f,  x2 = -2.3f, y2 = 1.f, z2 = 34.21f;
        vec3.setVec(x1,y1,z1);
        vec3a.setVec(x2,y2,z2);
        vec3b = vec3 - vec3a;
        val1 = x1-x2;
        val2 = y1-y2;
        val3 = z1-z2;
        CHECK_MESSAGE((val1 == vec3b.mV[VX], "2:operator- failed") && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ]));
    
}

TEST_CASE("test_15")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
        F32 val1, val2, val3;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
        val1 = vec3 * vec3a;
        val2 = x1*x2 + y1*y2 + z1*z2;
        CHECK_MESSAGE(val1 == val2, "1:operator* failed");

        vec3a.clearVec();
        F32 mulVal = 4.332f;
        vec3a = vec3 * mulVal;
        val1 = x1*mulVal;
        val2 = y1*mulVal;
        val3 = z1*mulVal;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "2:operator* failed") && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
        vec3a.clearVec();
        vec3a = mulVal * vec3;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "3:operator* failed ") && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
    
}

TEST_CASE("test_16")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
        F32 val1, val2, val3;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2), vec3b;
        vec3b = vec3 % vec3a ;
        val1 = y1*z2 - y2*z1;
        val2 = z1*x2 -z2*x1;
        val3 = x1*y2-x2*y1;
        CHECK_MESSAGE((val1 == vec3b.mV[VX], "1:operator% failed") && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ]));

        vec3.clearVec();
        vec3a.clearVec();
        vec3b.clearVec();
        x1 =112.f, y1 = 22.3f,z1 = 1.2f, x2 = -2.3f, y2 = 341.11f, z2 = 1234.234f;
        vec3.setVec(x1,y1,z1);
        vec3a.setVec(x2,y2,z2);
        vec3b = vec3 % vec3a ;
        val1 = y1*z2 - y2*z1;
        val2 = z1*x2 -z2*x1;
        val3 = x1*y2-x2*y1;
        CHECK_MESSAGE((val1 == vec3b.mV[VX], "2:operator% failed ") && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ]));
    
}

TEST_CASE("test_17")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, div = 3.2f;
        F32 t = 1.f / div, val1, val2, val3;
        LLVector3 vec3(x1,y1,z1), vec3a;
        vec3a = vec3 / div;
        val1 = x1 * t;
        val2 = y1 * t;
        val3 = z1 *t;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "1:operator/ failed") && (val2 == vec3a.mV[VY]) && (val3 == vec3a.mV[VZ]));

        vec3a.clearVec();
        x1 = -.235f, y1 = -24.32f, z1 = .342f, div = -2.2f;
        t = 1.f / div;
        vec3.setVec(x1,y1,z1);
        vec3a = vec3 / div;
        val1 = x1 * t;
        val2 = y1 * t;
        val3 = z1 *t;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "2:operator/ failed") && (val2 == vec3a.mV[VY]) && (val3 == vec3a.mV[VZ]));
    
}

TEST_CASE("test_18")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f;
        LLVector3 vec3(x1,y1,z1), vec3a(x1,y1,z1);
        CHECK_MESSAGE((vec3 == vec3a, "1:operator== failed"));

        vec3a.clearVec();
        x1 = -.235f, y1 = -24.32f, z1 = .342f;
        vec3.clearVec();
        vec3a.clearVec();
        vec3.setVec(x1,y1,z1);
        vec3a.setVec(x1,y1,z1);
        CHECK_MESSAGE((vec3 == vec3a, "2:operator== failed "));
    
}

TEST_CASE("test_19")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 =112.f, y2 = 2.234f,z2 = 11.2f;;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
        CHECK_MESSAGE((vec3a != vec3, "1:operator!= failed"));

        vec3.clearVec();
        vec3.clearVec();
        vec3a.setVec(vec3);
        CHECK_MESSAGE(( false == (vec3a != vec3, "2:operator!= failed")));
    
}

TEST_CASE("test_20")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 =112.f, y2 = 2.2f,z2 = 11.2f;;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
        vec3a += vec3;
        F32 val1, val2, val3;
        val1 = x1+x2;
        val2 = y1+y2;
        val3 = z1+z2;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "1:operator+= failed") && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
    
}

TEST_CASE("test_21")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 =112.f, y2 = 2.2f,z2 = 11.2f;;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
        vec3a -= vec3;
        F32 val1, val2, val3;
        val1 = x2-x1;
        val2 = y2-y1;
        val3 = z2-z1;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "1:operator-= failed") && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
    
}

TEST_CASE("test_22")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
        F32 val1,val2,val3;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
        vec3a *= vec3;
        val1 = x1*x2;
        val2 = y1*y2;
        val3 = z1*z2;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "1:operator*= failed") && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));

        F32 mulVal = 4.332f;
        vec3 *=mulVal;
        val1 = x1*mulVal;
        val2 = y1*mulVal;
        val3 = z1*mulVal;
        CHECK_MESSAGE(is_approx_equal(val1, vec3.mV[VX], "2:operator*= failed ") && is_approx_equal(val2, vec3.mV[VY]) && is_approx_equal(val3, vec3.mV[VZ]));
    
}

TEST_CASE("test_23")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2),vec3b;
        vec3b = vec3a % vec3;
        vec3a %= vec3;
        CHECK_MESSAGE(vec3a == vec3b, "1:operator%= failed");
    
}

TEST_CASE("test_24")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, div = 3.2f;
        F32 t = 1.f / div, val1, val2, val3;
        LLVector3 vec3a(x1,y1,z1);
        vec3a /= div;
        val1 = x1 * t;
        val2 = y1 * t;
        val3 = z1 *t;
        CHECK_MESSAGE((val1 == vec3a.mV[VX], "1:operator/= failed") && (val2 == vec3a.mV[VY]) && (val3 == vec3a.mV[VZ]));
    
}

TEST_CASE("test_25")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f;
        LLVector3 vec3(x1,y1,z1), vec3a;
        vec3a = -vec3;
        CHECK_MESSAGE((-vec3a == vec3, "1:operator- failed"));
    
}

TEST_CASE("test_26")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 1.2f;
        std::ostringstream stream1, stream2;
        LLVector3 vec3(x1,y1,z1), vec3a;
        stream1 << vec3;
        vec3a.setVec(x1,y1,z1);
        stream2 << vec3a;
        CHECK_MESSAGE((stream1.str(, "1:operator << failed") == stream2.str()));
    
}

TEST_CASE("test_27")
{

        F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 1.11f, z2 = 1234.234f;
        LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
        CHECK_MESSAGE((true == (vec3 < vec3a, "1:operator< failed")));
        x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 2.f, z2 = 1234.234f;
        vec3.setVec(x1,y1,z1);
        vec3a.setVec(x2,y2,z2);
        CHECK_MESSAGE((true == (vec3 < vec3a, "2:operator< failed ")));
        x1 =2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f,
        vec3.setVec(x1,y1,z1);
        vec3a.setVec(x2,y2,z2);
        CHECK_MESSAGE((false == (vec3 < vec3a, "3:operator< failed ")));
    
}

TEST_CASE("test_28")
{

        F32 x1 =1.23f, y1 = 2.f,z1 = 4.f;
        std::string buf("1.23 2. 4");
        LLVector3 vec3, vec3a(x1,y1,z1);
        LLVector3::parseVector3(buf, &vec3);
        CHECK_MESSAGE(vec3 == vec3a, "1:parseVector3 failed");
    
}

TEST_CASE("test_29")
{

        F32 x1 =1.f, y1 = 2.f,z1 = 4.f;
        LLVector3 vec3(x1,y1,z1),vec3a,vec3b;
        vec3a.setVec(1,1,1);
        vec3a.scaleVec(vec3);
        CHECK_MESSAGE(vec3 == vec3a, "1:scaleVec failed");
        vec3a.clearVec();
        vec3a.setVec(x1,y1,z1);
        vec3a.scaleVec(vec3);
        CHECK_MESSAGE(((1.f ==vec3a.mV[VX], "2:scaleVec failed")&& (4.f ==vec3a.mV[VY]) && (16.f ==vec3a.mV[VZ])));
    
}

TEST_CASE("test_30")
{

        F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 1.11f, z2 = 1234.234f;
        F32 val = 2.3f,val1,val2,val3;
        val1 = x1 + (x2 - x1)* val;
        val2 = y1 + (y2 - y1)* val;
        val3 = z1 + (z2 - z1)* val;
        LLVector3 vec3(x1,y1,z1),vec3a(x2,y2,z2);
        LLVector3 vec3b = lerp(vec3,vec3a,val);
        CHECK_MESSAGE(((val1 ==vec3b.mV[VX], "1:lerp failed")&& (val2 ==vec3b.mV[VY]) && (val3 ==vec3b.mV[VZ])));
    
}

TEST_CASE("test_31")
{

        F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 1.f, z2 = 1.f;
        F32 val1,val2;
        LLVector3 vec3(x1,y1,z1),vec3a(x2,y2,z2);
        val1 = dist_vec(vec3,vec3a);
        val2 = (F32) sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2) + (z1 - z2)* (z1 -z2));
        CHECK_MESSAGE(val2 == val1, "1:dist_vec: Fail ");
        val1 = dist_vec_squared(vec3,vec3a);
        val2 =((x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2) + (z1 - z2)* (z1 -z2));
        CHECK_MESSAGE(val2 == val1, "2:dist_vec_squared: Fail ");
        val1 = dist_vec_squared2D(vec3, vec3a);
        val2 =(x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2);
        CHECK_MESSAGE(val2 == val1, "3:dist_vec_squared2D: Fail ");
    
}

TEST_CASE("test_32")
{

        F32 x =12.3524f, y = -342.f,z = 4.126341f;
        LLVector3 vec3(x,y,z);
        F32 mag = vec3.normVec();
        mag = 1.f/ mag;
        F32 val1 = x* mag, val2 = y* mag, val3 = z* mag;
        CHECK_MESSAGE(is_approx_equal(val1, vec3.mV[VX], "1:normVec: Fail ") && is_approx_equal(val2, vec3.mV[VY]) && is_approx_equal(val3, vec3.mV[VZ]));
        x = 0.000000001f, y = 0.f, z = 0.f;
        vec3.clearVec();
        vec3.setVec(x,y,z);
        mag = vec3.normVec();
        val1 = x* mag, val2 = y* mag, val3 = z* mag;
        CHECK_MESSAGE((mag == 0., "2:normVec: Fail ") && (0. == vec3.mV[VX]) && (0. == vec3.mV[VY])&& (0. == vec3.mV[VZ]));
    
}

TEST_CASE("test_33")
{

        F32 x = -202.23412f, y = 123.2312f, z = -89.f;
        LLVector3 vec(x,y,z);
        vec.snap(2);
        CHECK_MESSAGE(is_approx_equal(-202.23f, vec.mV[VX], "1:snap: Fail ") && is_approx_equal(123.23f, vec.mV[VY]) && is_approx_equal(-89.f, vec.mV[VZ]));
    
}

TEST_CASE("test_34")
{

        F32 x = 10.f, y = 20.f, z = -15.f;
        F32 x1, y1, z1;
        F32 lowerxy = 0.f, upperxy = 1.0f, lowerz = -1.0f, upperz = 1.f;
        LLVector3 vec3(x,y,z);
        vec3.quantize16(lowerxy,upperxy,lowerz,upperz);
        x1 = U16_to_F32(F32_to_U16(x, lowerxy, upperxy), lowerxy, upperxy);
        y1 = U16_to_F32(F32_to_U16(y, lowerxy, upperxy), lowerxy, upperxy);
        z1 = U16_to_F32(F32_to_U16(z, lowerz,  upperz),  lowerz,  upperz);
        CHECK_MESSAGE(is_approx_equal(x1, vec3.mV[VX], "1:quantize16: Fail ") && is_approx_equal(y1, vec3.mV[VY]) && is_approx_equal(z1, vec3.mV[VZ]));
        LLVector3 vec3a(x,y,z);
        vec3a.quantize8(lowerxy,upperxy,lowerz,upperz);
        x1 = U8_to_F32(F32_to_U8(x, lowerxy, upperxy), lowerxy, upperxy);
        y1 = U8_to_F32(F32_to_U8(y, lowerxy, upperxy), lowerxy, upperxy);
        z1 = U8_to_F32(F32_to_U8(z, lowerz, upperz), lowerz, upperz);
        CHECK_MESSAGE(is_approx_equal(x1, vec3a.mV[VX], "2:quantize8: Fail ") && is_approx_equal(y1, vec3a.mV[VY]) && is_approx_equal(z1, vec3a.mV[VZ]));
    
}

TEST_CASE("test_35")
{

        LLSD sd = LLSD::emptyArray();
        sd[0] = 1.f;

        LLVector3 parsed_1(sd);
        CHECK_MESSAGE(is_approx_equal(parsed_1.mV[VX], 1.f, "1:LLSD parse: Fail ") && is_approx_equal(parsed_1.mV[VY], 0.f) && is_approx_equal(parsed_1.mV[VZ], 0.f));

        sd[1] = 2.f;
        LLVector3 parsed_2(sd);
        CHECK_MESSAGE(is_approx_equal(parsed_2.mV[VX], 1.f, "2:LLSD parse: Fail ") && is_approx_equal(parsed_2.mV[VY], 2.f) && is_approx_equal(parsed_2.mV[VZ], 0.f));

        sd[2] = 3.f;
        LLVector3 parsed_3(sd);
        CHECK_MESSAGE(is_approx_equal(parsed_3.mV[VX], 1.f, "3:LLSD parse: Fail ") && is_approx_equal(parsed_3.mV[VY], 2.f) && is_approx_equal(parsed_3.mV[VZ], 3.f));
    
}

} // TEST_SUITE

