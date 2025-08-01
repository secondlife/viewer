/**
 * @file v3dmath_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v3dmath test cases.
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
#include "llsd.h"
#include "../test/lldoctest.h"

#include "../m3math.h"
#include "../v4math.h"
#include "../v3dmath.h"
#include "../v3dmath.h"
#include "../llquaternion.h"

TEST_SUITE("v3dmath_h") {

TEST_CASE("test_1")
{

        LLVector3d vec3D;
        CHECK_MESSAGE(((0 == vec3D.mdV[VX], "1:LLVector3d:Fail to initialize ") && (0 == vec3D.mdV[VY]) && (0 == vec3D.mdV[VZ])));
        F64 x = 2.32f, y = 1.212f, z = -.12f;
        LLVector3d vec3Da(x,y,z);
        CHECK_MESSAGE(((2.32f == vec3Da.mdV[VX], "2:LLVector3d:Fail to initialize ") && (1.212f == vec3Da.mdV[VY]) && (-.12f == vec3Da.mdV[VZ])));
        const F64 vec[3] = {1.2f ,3.2f, -4.2f
}

TEST_CASE("test_2")
{

        S32 a = -235;
        LLSD llsd(a);
        LLVector3d vec3d(llsd);
        LLSD sd = vec3d.getValue();
        LLVector3d vec3da(sd);
        CHECK_MESSAGE((vec3d == vec3da, "1:getValue:Fail "));
    
}

TEST_CASE("test_3")
{

        F64 a = 232345521.411132;
        LLSD llsd(a);
        LLVector3d vec3d;
        vec3d.setValue(llsd);
        LLSD sd = vec3d.getValue();
        LLVector3d vec3da(sd);
        CHECK_MESSAGE((vec3d == vec3da, "1:setValue:Fail to initialize "));
    
}

TEST_CASE("test_4")
{

        F64 a[3] = {222231.43222, 12345.2343, -434343.33222
}

TEST_CASE("test_5")
{

        F64 x = 2.32f, y = 1.212f, z = -.12f;
        LLVector3d vec3D(x,y,z);
        vec3D.clearVec();
        CHECK_MESSAGE(((0 == vec3D.mdV[VX], "1:clearVec:Fail to initialize ") && (0 == vec3D.mdV[VY]) && (0 == vec3D.mdV[VZ])));
        vec3D.setVec(x,y,z);
        CHECK_MESSAGE(((x == vec3D.mdV[VX], "2:setVec:Fail to initialize ") && (y == vec3D.mdV[VY]) && (z == vec3D.mdV[VZ])));
        vec3D.zeroVec();
        CHECK_MESSAGE(((0 == vec3D.mdV[VX], "3:zeroVec:Fail to initialize ") && (0 == vec3D.mdV[VY]) && (0 == vec3D.mdV[VZ])));
        vec3D.clearVec();
        LLVector3 vec3((F32)x,(F32)y,(F32)z);
        vec3D.setVec(vec3);
        CHECK_MESSAGE(((x == vec3D.mdV[VX], "4:setVec:Fail to initialize ") && (y == vec3D.mdV[VY]) && (z == vec3D.mdV[VZ])));
        vec3D.clearVec();
        const F64 vec[3] = {x,y,z
}

TEST_CASE("test_6")
{

        F64 x = -2.32, y = 1.212, z = -.12;
        LLVector3d vec3D(x,y,z);
        vec3D.abs();
        CHECK_MESSAGE(((-x == vec3D.mdV[VX], "1:abs:Fail  ") && (y == vec3D.mdV[VY]) && (-z == vec3D.mdV[VZ])));
        CHECK_MESSAGE((false == vec3D.isNull(, "2:isNull():Fail ")));
        vec3D.clearVec();
        x =.00000001, y = .000001001, z = .000001001;
        vec3D.setVec(x,y,z);
        CHECK_MESSAGE((true == vec3D.isNull(, "3:isNull():Fail ")));
        CHECK_MESSAGE((false == vec3D.isExactlyZero(, "4:isExactlyZero():Fail ")));
        x =.0000000, y = .00000000, z = .00000000;
        vec3D.setVec(x,y,z);
        CHECK_MESSAGE((true == vec3D.isExactlyZero(, "5:isExactlyZero():Fail ")));
    
}

TEST_CASE("test_7")
{

        F64 x = -2.32, y = 1.212, z = -.12;
        LLVector3d vec3D(x,y,z);

        CHECK_MESSAGE(( x ==  vec3D[0], "1:operator [] failed"));
        CHECK_MESSAGE(( y ==  vec3D[1], "2:operator [] failed"));
        CHECK_MESSAGE(( z ==  vec3D[2], "3:operator [] failed"));
        vec3D.clearVec();
        x = 23.23, y = -.2361, z = 3.25;
        vec3D.setVec(x,y,z);
        F64 &ref1 = vec3D[0];
        CHECK_MESSAGE(( ref1 ==  vec3D[0], "4:operator [] failed"));
        F64 &ref2 = vec3D[1];
        CHECK_MESSAGE(( ref2 ==  vec3D[1], "5:operator [] failed"));
        F64 &ref3 = vec3D[2];
        CHECK_MESSAGE(( ref3 ==  vec3D[2], "6:operator [] failed"));
    
}

TEST_CASE("test_8")
{

        F32 x = 1.f, y = 2.f, z = -1.f;
        LLVector4 vec4(x,y,z);
        LLVector3d vec3D;
        vec3D = vec4;
        CHECK_MESSAGE(((vec4.mV[VX] == vec3D.mdV[VX], "1:operator=:Fail to initialize ") && (vec4.mV[VY] == vec3D.mdV[VY]) && (vec4.mV[VZ] == vec3D.mdV[VZ])));
    
}

TEST_CASE("test_9")
{

        F64 x1 = 1.78787878, y1 = 232322.2121, z1 = -12121.121212;
        F64 x2 = 1.2, y2 = 2.5, z2 = 1.;
        LLVector3d vec3D(x1,y1,z1),vec3Da(x2,y2,z2),vec3Db;
        vec3Db = vec3Da+ vec3D;
        CHECK_MESSAGE(((x1+x2 == vec3Db.mdV[VX], "1:operator+:Fail to initialize ") && (y1+y2 == vec3Db.mdV[VY]) && (z1+z2 == vec3Db.mdV[VZ])));
        x1 = -2.45, y1 = 2.1, z1 = 3.0;
        vec3D.clearVec();
        vec3Da.clearVec();
        vec3D.setVec(x1,y1,z1);
        vec3Da += vec3D;
        CHECK_MESSAGE(vec3Da == vec3D, "2:operator+=: Fail to initialize");
        vec3Da += vec3D;
        CHECK_MESSAGE(((2*x1 == vec3Da.mdV[VX], "3:operator+=:Fail to initialize ") && (2*y1 == vec3Da.mdV[VY]) && (2*z1 == vec3Da.mdV[VZ])));
    
}

TEST_CASE("test_10")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1;
        F64 x2 = 1.2, y2 = 2.5, z2 = 1.;
        LLVector3d vec3D(x1,y1,z1),vec3Da(x2,y2,z2),vec3Db;
        vec3Db = vec3Da - vec3D;
        CHECK_MESSAGE(((x2-x1 == vec3Db.mdV[VX], "1:operator-:Fail to initialize ") && (y2-y1 == vec3Db.mdV[VY]) && (z2-z1 == vec3Db.mdV[VZ])));
        x1 = -2.45, y1 = 2.1, z1 = 3.0;
        vec3D.clearVec();
        vec3Da.clearVec();
        vec3D.setVec(x1,y1,z1);
        vec3Da -=vec3D;
        CHECK_MESSAGE(((2.45 == vec3Da.mdV[VX], "2:operator-=:Fail to initialize ") && (-2.1 == vec3Da.mdV[VY]) && (-3.0 == vec3Da.mdV[VZ])));
        vec3Da -= vec3D;
        CHECK_MESSAGE(((-2*x1 == vec3Da.mdV[VX], "3:operator-=:Fail to initialize ") && (-2*y1 == vec3Da.mdV[VY]) && (-2*z1 == vec3Da.mdV[VZ])));
    
}

TEST_CASE("test_11")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1;
        F64 x2 = 1.2, y2 = 2.5, z2 = 1.;
        LLVector3d vec3D(x1,y1,z1),vec3Da(x2,y2,z2);
        F64 res = vec3D * vec3Da;
        ensure_approximately_equals(
            "1:operator* failed",
            res,
            (x1*x2 + y1*y2 + z1*z2),
            8);
        vec3Da.clearVec();
        F64 mulVal = 4.2;
        vec3Da = vec3D * mulVal;
        ensure_approximately_equals(
            "2a:operator* failed",
            vec3Da.mdV[VX],
            x1*mulVal,
            8);
        ensure_approximately_equals(
            "2b:operator* failed",
            vec3Da.mdV[VY],
            y1*mulVal,
            8);
        ensure_approximately_equals(
            "2c:operator* failed",
            vec3Da.mdV[VZ],
            z1*mulVal,
            8);
        vec3Da.clearVec();
        vec3Da = mulVal * vec3D;
        ensure_approximately_equals(
            "3a:operator* failed",
            vec3Da.mdV[VX],
            x1*mulVal,
            8);
        ensure_approximately_equals(
            "3b:operator* failed",
            vec3Da.mdV[VY],
            y1*mulVal,
            8);
        ensure_approximately_equals(
            "3c:operator* failed",
            vec3Da.mdV[VZ],
            z1*mulVal,
            8);
        vec3D *= mulVal;
        ensure_approximately_equals(
            "4a:operator*= failed",
            vec3D.mdV[VX],
            x1*mulVal,
            8);
        ensure_approximately_equals(
            "4b:operator*= failed",
            vec3D.mdV[VY],
            y1*mulVal,
            8);
        ensure_approximately_equals(
            "4c:operator*= failed",
            vec3D.mdV[VZ],
            z1*mulVal,
            8);
    
}

TEST_CASE("test_12")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1;
        F64 x2 = 1.2, y2 = 2.5, z2 = 1.;
        F64 val1, val2, val3;
        LLVector3d vec3D(x1,y1,z1),vec3Da(x2,y2,z2), vec3Db;
        vec3Db = vec3D % vec3Da;
        val1 = y1*z2 - y2*z1;
        val2 = z1*x2 -z2*x1;
        val3 = x1*y2-x2*y1;
        CHECK_MESSAGE((val1 == vec3Db.mdV[VX], "1:operator% failed") && (val2 == vec3Db.mdV[VY]) && (val3 == vec3Db.mdV[VZ]));
        vec3D %= vec3Da;
        CHECK_MESSAGE(is_approx_equal(vec3D.mdV[VX],vec3Db.mdV[VX], "2:operator%= failed") &&
               is_approx_equal(vec3D.mdV[VY],vec3Db.mdV[VY]) &&
               is_approx_equal(vec3D.mdV[VZ],vec3Db.mdV[VZ]) );
    
}

TEST_CASE("test_13")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1,div = 4.2;
        F64 t = 1.f / div;
        LLVector3d vec3D(x1,y1,z1), vec3Da;
        vec3Da = vec3D/div;
        ensure_approximately_equals(
            "1a:operator/ failed",
            vec3Da.mdV[VX],
            x1*t,
            8);
        ensure_approximately_equals(
            "1b:operator/ failed",
            vec3Da.mdV[VY],
            y1*t,
            8);
        ensure_approximately_equals(
            "1c:operator/ failed",
            vec3Da.mdV[VZ],
            z1*t,
            8);
        x1 = 1.23, y1 = 4., z1 = -2.32;
        vec3D.clearVec();
        vec3Da.clearVec();
        vec3D.setVec(x1,y1,z1);
        vec3Da = vec3D/div;
        ensure_approximately_equals(
            "2a:operator/ failed",
            vec3Da.mdV[VX],
            x1*t,
            8);
        ensure_approximately_equals(
            "2b:operator/ failed",
            vec3Da.mdV[VY],
            y1*t,
            8);
        ensure_approximately_equals(
            "2c:operator/ failed",
            vec3Da.mdV[VZ],
            z1*t,
            8);
        vec3D /= div;
        ensure_approximately_equals(
            "3a:operator/= failed",
            vec3D.mdV[VX],
            x1*t,
            8);
        ensure_approximately_equals(
            "3b:operator/= failed",
            vec3D.mdV[VY],
            y1*t,
            8);
        ensure_approximately_equals(
            "3c:operator/= failed",
            vec3D.mdV[VZ],
            z1*t,
            8);
    
}

TEST_CASE("test_14")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1;
        LLVector3d vec3D(x1,y1,z1), vec3Da;
        CHECK_MESSAGE((true == (vec3D !=vec3Da, "1:operator!= failed")));
        vec3Da = vec3D;
        CHECK_MESSAGE((vec3D ==vec3Da, "2:operator== failed"));
        vec3D.clearVec();
        vec3Da.clearVec();
        x1 = .211, y1 = 21.111, z1 = 23.22;
        vec3D.setVec(x1,y1,z1);
        vec3Da.setVec(x1,y1,z1);
        CHECK_MESSAGE((vec3D ==vec3Da, "3:operator== failed"));
        CHECK_MESSAGE((false == (vec3D !=vec3Da, "4:operator!= failed")));
    
}

TEST_CASE("test_15")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1;
        LLVector3d vec3D(x1,y1,z1), vec3Da;
        std::ostringstream stream1, stream2;
        stream1 << vec3D;
        vec3Da.setVec(x1,y1,z1);
        stream2 << vec3Da;
        CHECK_MESSAGE((stream1.str(, "1:operator << failed") == stream2.str()));
    
}

TEST_CASE("test_16")
{

        F64 x1 = 1.23, y1 = 2.0, z1 = 4.;
        std::string buf("1.23 2. 4");
        LLVector3d vec3D, vec3Da(x1,y1,z1);
        LLVector3d::parseVector3d(buf, &vec3D);
        ensure_equals("1:parseVector3d: failed " , vec3D, vec3Da);
    
}

TEST_CASE("test_17")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1;
        LLVector3d vec3D(x1,y1,z1), vec3Da;
        vec3Da = -vec3D;
        CHECK_MESSAGE((vec3D == - vec3Da, "1:operator- failed"));
    
}

TEST_CASE("test_18")
{

        F64 x = 1., y = 2., z = -1.1;
        LLVector3d vec3D(x,y,z);
        F64 res = (x*x + y*y + z*z) - vec3D.magVecSquared();
        CHECK_MESSAGE(((-F_APPROXIMATELY_ZERO <= res, "1:magVecSquared:Fail ")&& (res <=F_APPROXIMATELY_ZERO)));
        res = (F32) sqrt(x*x + y*y + z*z) - vec3D.magVec();
        CHECK_MESSAGE(((-F_APPROXIMATELY_ZERO <= res, "2:magVec: Fail ")&& (res <=F_APPROXIMATELY_ZERO)));
    
}

TEST_CASE("test_19")
{

        F64 x = 1., y = 2., z = -1.1;
        LLVector3d vec3D(x,y,z);
        F64 mag = vec3D.normVec();
        mag = 1.f/ mag;
        ensure_approximately_equals(
            "1a:normVec: Fail ",
            vec3D.mdV[VX],
            x * mag,
            8);
        ensure_approximately_equals(
            "1b:normVec: Fail ",
            vec3D.mdV[VY],
            y * mag,
            8);
        ensure_approximately_equals(
            "1c:normVec: Fail ",
            vec3D.mdV[VZ],
            z * mag,
            8);
        x = 0.000000001, y = 0.000000001, z = 0.000000001;
        vec3D.clearVec();
        vec3D.setVec(x,y,z);
        mag = vec3D.normVec();
        ensure_approximately_equals(
            "2a:normVec: Fail ",
            vec3D.mdV[VX],
            x * mag,
            8);
        ensure_approximately_equals(
            "2b:normVec: Fail ",
            vec3D.mdV[VY],
            y * mag,
            8);
        ensure_approximately_equals(
            "2c:normVec: Fail ",
            vec3D.mdV[VZ],
            z * mag,
            8);
    
}

TEST_CASE("test_20")
{

        F64 x1 = 1111.232222;
        F64 y1 = 2222222222.22;
        F64 z1 = 422222222222.0;
        std::string buf("1111.232222 2222222222.22 422222222222");
        LLVector3d vec3Da, vec3Db(x1,y1,z1);
        LLVector3d::parseVector3d(buf, &vec3Da);
        CHECK_MESSAGE(vec3Da == vec3Db, "1:parseVector3 failed");
    
}

TEST_CASE("test_21")
{

        F64 x1 = 1., y1 = 2., z1 = -1.1;
        F64 x2 = 1.2, y2 = 2.5, z2 = 1.;
        F64 val = 2.3f,val1,val2,val3;
        val1 = x1 + (x2 - x1)* val;
        val2 = y1 + (y2 - y1)* val;
        val3 = z1 + (z2 - z1)* val;
        LLVector3d vec3Da(x1,y1,z1),vec3Db(x2,y2,z2);
        LLVector3d vec3d = lerp(vec3Da,vec3Db,val);
        CHECK_MESSAGE(((val1 ==vec3d.mdV[VX], "1:lerp failed")&& (val2 ==vec3d.mdV[VY]) && (val3 ==vec3d.mdV[VZ])));
    
}

TEST_CASE("test_22")
{

        F64 x = 2.32, y = 1.212, z = -.12;
        F64 min = 0.0001, max = 3.0;
        LLVector3d vec3d(x,y,z);
        CHECK_MESSAGE((true == (vec3d.clamp(min, max, "1:clamp:Fail "))));
        x = 0.000001f, z = 5.3f;
        vec3d.setVec(x,y,z);
        CHECK_MESSAGE((true == (vec3d.clamp(min, max, "2:clamp:Fail "))));
    
}

TEST_CASE("test_23")
{

        F64 x = 10., y = 20., z = -15.;
        F64 epsilon = .23425;
        LLVector3d vec3Da(x,y,z), vec3Db(x,y,z);
        CHECK_MESSAGE((true == are_parallel(vec3Da,vec3Db,epsilon, "1:are_parallel: Fail ")));
        F64 x1 = -12., y1 = -20., z1 = -100.;
        vec3Db.clearVec();
        vec3Db.setVec(x1,y1,z1);
        CHECK_MESSAGE((false == are_parallel(vec3Da,vec3Db,epsilon, "2:are_parallel: Fail ")));
    
}

TEST_CASE("test_24")
{

#if LL_WINDOWS && _MSC_VER < 1400
        skip("This fails on VS2003!");
#else
        F64 x = 10., y = 20., z = -15.;
        F64 angle1, angle2;
        LLVector3d vec3Da(x,y,z), vec3Db(x,y,z);
        angle1 = angle_between(vec3Da, vec3Db);
        CHECK_MESSAGE((0 == angle1, "1:angle_between: Fail "));
        F64 x1 = -1., y1 = -20., z1 = -1.;
        vec3Da.clearVec();
        vec3Da.setVec(x1,y1,z1);
        angle2 = angle_between(vec3Da, vec3Db);
        vec3Db.normVec();
        vec3Da.normVec();
        F64 angle = vec3Db*vec3Da;
        angle = acos(angle);
#if LL_WINDOWS && _MSC_VER > 1900
        skip("This fails on VS2017!");
#else
        CHECK_MESSAGE((angle == angle2, "2:angle_between: Fail "));
#endif

#endif
    
}

} // TEST_SUITE

