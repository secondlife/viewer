/**
 * @file llquaternion_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief Test cases of llquaternion.h
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

#include "../v4math.h"
#include "../v3math.h"
#include "../v3dmath.h"
#include "../m4math.h"
#include "../m3math.h"
#include "../llquaternion.h"

TEST_SUITE("LLQuaternion") {

TEST_CASE("test_1")
{

        LLQuaternion llquat;
        CHECK_MESSAGE(0.f == llquat.mQ[0] &&
                                            0.f == llquat.mQ[1] &&
                                            0.f == llquat.mQ[2] &&
                                            1.f == llquat.mQ[3], "LLQuaternion::LLQuaternion() failed");
    
}

TEST_CASE("test_2")
{

        LLMatrix4 llmat;
        LLVector4 vector1(2.0f, 1.0f, 3.0f, 6.0f);
        LLVector4 vector2(5.0f, 6.0f, 0.0f, 1.0f);
        LLVector4 vector3(2.0f, 1.0f, 2.0f, 9.0f);
        LLVector4 vector4(3.0f, 8.0f, 1.0f, 5.0f);

        llmat.initRows(vector1, vector2, vector3, vector4);
        CHECK_MESSAGE(2.0f == llmat.mMatrix[0][0] &&
                                            1.0f == llmat.mMatrix[0][1] &&
                                            3.0f == llmat.mMatrix[0][2] &&
                                            6.0f == llmat.mMatrix[0][3] &&
                                            5.0f == llmat.mMatrix[1][0] &&
                                            6.0f == llmat.mMatrix[1][1] &&
                                            0.0f == llmat.mMatrix[1][2] &&
                                            1.0f == llmat.mMatrix[1][3] &&
                                            2.0f == llmat.mMatrix[2][0] &&
                                            1.0f == llmat.mMatrix[2][1] &&
                                            2.0f == llmat.mMatrix[2][2] &&
                                            9.0f == llmat.mMatrix[2][3] &&
                                            3.0f == llmat.mMatrix[3][0] &&
                                            8.0f == llmat.mMatrix[3][1] &&
                                            1.0f == llmat.mMatrix[3][2] &&
                                            5.0f == llmat.mMatrix[3][3], "explicit LLQuaternion(const LLMatrix4 &mat) failed");
    
}

TEST_CASE("test_3")
{

        LLMatrix3 llmat;

        LLVector3 vect1(3.4028234660000000f , 234.56f, 4234.442234f);
        LLVector3 vect2(741.434f, 23.00034f, 6567.223423f);
        LLVector3 vect3(566.003034f, 12.98705f, 234.764423f);
        llmat.setRows(vect1, vect2, vect3);

        CHECK_MESSAGE(3.4028234660000000f == llmat.mMatrix[0][0] &&
                                        234.56f == llmat.mMatrix[0][1] &&
                                        4234.442234f == llmat.mMatrix[0][2] &&
                                        741.434f == llmat.mMatrix[1][0] &&
                                        23.00034f == llmat.mMatrix[1][1] &&
                                        6567.223423f == llmat.mMatrix[1][2] &&
                                        566.003034f == llmat.mMatrix[2][0] &&
                                        12.98705f == llmat.mMatrix[2][1] &&
                                        234.764423f == llmat.mMatrix[2][2], "LLMatrix3::setRows fn failed.");
    
}

TEST_CASE("test_4")
{

        F32 x_val = 3.0f;
        F32 y_val = 2.0f;
        F32 z_val = 6.0f;
        F32 w_val = 1.0f;

        LLQuaternion res_quat;
        res_quat.setQuatInit(x_val, y_val, z_val, w_val);
        res_quat.normQuat();

        CHECK_MESSAGE(is_approx_equal(0.42426407f, res_quat.mQ[0], "LLQuaternion::normQuat() fn failed") &&
                            is_approx_equal(0.28284273f, res_quat.mQ[1]) &&
                            is_approx_equal(0.84852815f, res_quat.mQ[2]) &&
                            is_approx_equal(0.14142136f, res_quat.mQ[3]));

        x_val = 0.0f;
        y_val = 0.0f;
        z_val = 0.0f;
        w_val = 0.0f;

        res_quat.setQuatInit(x_val, y_val, z_val, w_val);
        res_quat.normQuat();

        CHECK_MESSAGE(is_approx_equal(0.0f, res_quat.mQ[0], "LLQuaternion::normQuat() fn. failed.") &&
                            is_approx_equal(0.0f, res_quat.mQ[1]) &&
                            is_approx_equal(0.0f, res_quat.mQ[2]) &&
                            is_approx_equal(1.0f, res_quat.mQ[3]));


        CHECK_MESSAGE(is_approx_equal(0.0f, res_quat.mQ[0], "LLQuaternion::normQuat() fn. failed.") &&
                            is_approx_equal(0.0f, res_quat.mQ[1]) &&
                            is_approx_equal(0.0f, res_quat.mQ[2]) &&
                            is_approx_equal(1.0f, res_quat.mQ[3]));
    
}

TEST_CASE("test_5")
{

        F32 x_val = 3.0f;
        F32 y_val = 2.0f;
        F32 z_val = 6.0f;
        F32 w_val = 1.0f;

        LLQuaternion res_quat;
        LLQuaternion result, result1;
        result1 = result = res_quat.setQuatInit(x_val, y_val, z_val, w_val);

        result.conjQuat();
        result1.transQuat();

        CHECK_MESSAGE(is_approx_equal(result1.mQ[0], result.mQ[0], "LLQuaternion::conjQuat and LLQuaternion::transQuat failed ") &&
                                is_approx_equal(result1.mQ[1], result.mQ[1]) &&
                                is_approx_equal(result1.mQ[2], result.mQ[2]));

    
}

TEST_CASE("test_6")
{

        LLQuaternion quat1(3.0f, 2.0f, 6.0f, 0.0f), quat2(1.0f, 1.0f, 1.0f, 1.0f);
        CHECK_MESSAGE(ll_round(12.000000f, 2, "1. The two values are different") == ll_round(dot(quat1, quat2), 2));

        LLQuaternion quat0(3.0f, 9.334f, 34.5f, 23.0f), quat(34.5f, 23.23f, 2.0f, 45.5f);
        CHECK_MESSAGE(ll_round(1435.828807f, 2, "2. The two values are different") == ll_round(dot(quat0, quat), 2));
    
}

TEST_CASE("test_7")
{

        F32 radian = 60.0f;
        LLQuaternion quat(3.0f, 2.0f, 6.0f, 0.0f);
        LLQuaternion quat1;
        quat1 = quat.constrain(radian);
        CHECK_MESSAGE(is_approx_equal_fraction(-0.423442f, quat1.mQ[0], 8, "1. LLQuaternion::constrain(F32 radians) failed") &&
                                    is_approx_equal_fraction(-0.282295f, quat1.mQ[1], 8) &&
                                    is_approx_equal_fraction(-0.846884f, quat1.mQ[2], 8) &&
                                    is_approx_equal_fraction(0.154251f, quat1.mQ[3], 8));


        radian = 30.0f;
        LLQuaternion quat0(37.50f, 12.0f, 86.023f, 40.32f);
        quat1 = quat0.constrain(radian);

        CHECK_MESSAGE(is_approx_equal_fraction(37.500000f, quat1.mQ[0], 8, "2. LLQuaternion::constrain(F32 radians) failed") &&
                                    is_approx_equal_fraction(12.0000f, quat1.mQ[1], 8) &&
                                    is_approx_equal_fraction(86.0230f, quat1.mQ[2], 8) &&
                                    is_approx_equal_fraction(40.320000f, quat1.mQ[3], 8));
    
}

TEST_CASE("test_8")
{

        F32 value1 = 15.0f;
        LLQuaternion quat1(1.0f, 2.0f, 4.0f, 1.0f);
        LLQuaternion quat2(4.0f, 3.0f, 6.5f, 9.7f);
        LLQuaternion res_lerp, res_slerp, res_nlerp;

        //test case for lerp(F32 t, const LLQuaternion &q) fn.
        res_lerp = lerp(value1, quat1);
        CHECK_MESSAGE(is_approx_equal_fraction(0.181355f, res_lerp.mQ[0], 16, "1. LLQuaternion lerp(F32 t, const LLQuaternion &q) failed") &&
                                        is_approx_equal_fraction(0.362711f, res_lerp.mQ[1], 16) &&
                                        is_approx_equal_fraction(0.725423f, res_lerp.mQ[2], 16) &&
                                        is_approx_equal_fraction(0.556158f, res_lerp.mQ[3], 16));

        //test case for lerp(F32 t, const LLQuaternion &p, const LLQuaternion &q) fn.
        res_lerp = lerp(value1, quat1, quat2);
        CHECK_MESSAGE(is_approx_equal_fraction(0.314306f, res_lerp.mQ[0], 16, "2. LLQuaternion lerp(F32 t, const LLQuaternion &p, const LLQuaternion &q) failed") &&
                                        is_approx_equal_fraction(0.116156f, res_lerp.mQ[1], 16) &&
                                        is_approx_equal_fraction(0.283559f, res_lerp.mQ[2], 16) &&
                                        is_approx_equal_fraction(0.898506f, res_lerp.mQ[3], 16));

        //test case for slerp( F32 u, const LLQuaternion &a, const LLQuaternion &b ) fn.
        res_slerp = slerp(value1, quat1, quat2);
        CHECK_MESSAGE(is_approx_equal_fraction(46.000f, res_slerp.mQ[0], 16, "3. LLQuaternion slerp( F32 u, const LLQuaternion &a, const LLQuaternion &b) failed") &&
                                        is_approx_equal_fraction(17.00f, res_slerp.mQ[1], 16) &&
                                        is_approx_equal_fraction(41.5f, res_slerp.mQ[2], 16) &&
                                        is_approx_equal_fraction(131.5f, res_slerp.mQ[3], 16));

        //test case for nlerp(F32 t, const LLQuaternion &a, const LLQuaternion &b) fn.
        res_nlerp = nlerp(value1, quat1, quat2);
        CHECK_MESSAGE(is_approx_equal_fraction(0.314306f, res_nlerp.mQ[0], 16, "4. LLQuaternion nlerp(F32 t, const LLQuaternion &a, const LLQuaternion &b) failed") &&
                                        is_approx_equal_fraction(0.116157f, res_nlerp.mQ[1], 16) &&
                                        is_approx_equal_fraction(0.283559f, res_nlerp.mQ[2], 16) &&
                                        is_approx_equal_fraction(0.898506f, res_nlerp.mQ[3], 16));

        //test case for nlerp(F32 t, const LLQuaternion &q) fn.
        res_slerp = slerp(value1, quat1);
        CHECK_MESSAGE(is_approx_equal_fraction(1.0f, res_slerp.mQ[0], 16, "5. LLQuaternion slerp(F32 t, const LLQuaternion &q) failed") &&
                                        is_approx_equal_fraction(2.0f, res_slerp.mQ[1], 16) &&
                                        is_approx_equal_fraction(4.0000f, res_slerp.mQ[2], 16) &&
                                        is_approx_equal_fraction(1.000f, res_slerp.mQ[3], 16));

        LLQuaternion quat3(2.0f, 1.0f, 5.5f, 10.5f);
        LLQuaternion res_nlerp1;
        value1 = 100.0f;
        res_nlerp1 = nlerp(value1, quat3);
        CHECK_MESSAGE(is_approx_equal_fraction(0.268245f, res_nlerp1.mQ[0], 16, "6. LLQuaternion nlerp(F32 t, const LLQuaternion &q)  failed") &&                                        is_approx_equal_fraction(0.134122f, res_nlerp1.mQ[1], 2) &&
                                        is_approx_equal_fraction(0.737673f, res_nlerp1.mQ[2], 16) &&
                                        is_approx_equal_fraction(0.604892f, res_nlerp1.mQ[3], 16));

        //test case for lerp(F32 t, const LLQuaternion &q) fn.
        res_lerp = lerp(value1, quat2);
        CHECK_MESSAGE(is_approx_equal_fraction(0.404867f, res_lerp.mQ[0], 16, "7. LLQuaternion lerp(F32 t, const LLQuaternion &q) failed") &&
                                        is_approx_equal_fraction(0.303650f, res_lerp.mQ[1], 16) &&
                                        is_approx_equal_fraction(0.657909f, res_lerp.mQ[2], 16) &&
                                        is_approx_equal_fraction(0.557704f, res_lerp.mQ[3], 16));

    
}

TEST_CASE("test_9")
{

        //test case for LLQuaternion operator*(const LLQuaternion &a, const LLQuaternion &b) fn
        LLQuaternion quat1(1.0f, 2.5f, 3.5f, 5.5f);
        LLQuaternion quat2(4.0f, 3.0f, 5.0f, 1.0f);
        LLQuaternion result = quat1 *  quat2;
        CHECK_MESSAGE((21.0f == result.mQ[0], "1. LLQuaternion Operator* failed") &&
                                            (10.0f == result.mQ[1]) &&
                                            (38.0f == result.mQ[2]) &&
                                            (-23.5f == result.mQ[3]));

        LLQuaternion quat3(2341.340f, 2352.345f, 233.25f, 7645.5f);
        LLQuaternion quat4(674.067f, 893.0897f, 578.0f, 231.0f);
        result = quat3 * quat4;
        CHECK_MESSAGE((4543086.5f == result.mQ[0], "2. LLQuaternion Operator* failed") &&
                                            (8567578.0f == result.mQ[1]) &&
                                            (3967591.25f == result.mQ[2]) &&
                                            is_approx_equal(-2047783.25f, result.mQ[3]));

        //inline LLQuaternion operator+(const LLQuaternion &a, const LLQuaternion &b)fn.
        result = quat1 + quat2;
        CHECK_MESSAGE((5.0f == result.mQ[0], "3. LLQuaternion operator+ failed") &&
                                            (5.5f == result.mQ[1]) &&
                                            (8.5f == result.mQ[2]) &&
                                            (6.5f == result.mQ[3]));

        result = quat3 + quat4;
        CHECK_MESSAGE(is_approx_equal(3015.407227f, result.mQ[0], "4. LLQuaternion operator+ failed") &&
            is_approx_equal(3245.434570f, result.mQ[1]) &&
            (811.25f == result.mQ[2]) &&
            (7876.5f == result.mQ[3]));

        //inline LLQuaternion operator-(const LLQuaternion &a, const LLQuaternion &b) fn
        result = quat1 - quat2;
        CHECK_MESSAGE((-3.0f == result.mQ[0], "5. LLQuaternion operator-(const LLQuaternion &a, const LLQuaternion &b) failed") &&
            (-0.5f == result.mQ[1]) &&
            (-1.5f == result.mQ[2]) &&
            (4.5f == result.mQ[3]));

        result = quat3 - quat4;
        CHECK_MESSAGE(is_approx_equal(1667.273071f, result.mQ[0], "6. LLQuaternion operator-(const LLQuaternion &a, const LLQuaternion &b) failed") &&
            is_approx_equal(1459.255249f, result.mQ[1]) &&
            (-344.75f == result.mQ[2]) &&
            (7414.50f == result.mQ[3]));
    
}

TEST_CASE("test_10")
{

        LLVector4 vect(12.0f, 5.0f, 60.0f, 75.1f);
        LLQuaternion quat(2323.034f, 23.5f, 673.23f, 57667.5f);
        LLVector4 result = vect * quat;
        CHECK_MESSAGE(is_approx_equal(39928406016.0f, result.mV[0], "1. LLVector4 operator*(const LLVector4 &a, const LLQuaternion &rot) failed") &&
            // gcc on x86 actually gives us more precision than we were expecting, verified with -ffloat-store - we forgive this
            (1457802240.0f >= result.mV[1]) && // gcc+x86+linux
            (1457800960.0f <= result.mV[1]) && // elsewhere
            is_approx_equal(200580612096.0f, result.mV[2]) &&
            (75.099998f == result.mV[3]));

        LLVector4 vect1(22.0f, 45.0f, 40.0f, 78.1f);
        LLQuaternion quat1(2.034f, 45.5f, 37.23f, 7.5f);
        result = vect1 * quat1;
        CHECK_MESSAGE(is_approx_equal(-58153.5390f, result.mV[0], "2. LLVector4 operator*(const LLVector4 &a, const LLQuaternion &rot) failed") &&
            (183787.8125f == result.mV[1]) &&
            (116864.164063f == result.mV[2]) &&
            (78.099998f == result.mV[3]));
    
}

TEST_CASE("test_11")
{

        LLVector3 vect(12.0f, 5.0f, 60.0f);
        LLQuaternion quat(23.5f, 6.5f, 3.23f, 56.5f);
        LLVector3 result = vect * quat;
        CHECK_MESSAGE(is_approx_equal(97182.953125f,result.mV[0], "1. LLVector3 operator*(const LLVector3 &a, const LLQuaternion &rot) failed") &&
            is_approx_equal(-135405.640625f, result.mV[1]) &&
            is_approx_equal(162986.140f, result.mV[2]));

        LLVector3 vect1(5.0f, 40.0f, 78.1f);
        LLQuaternion quat1(2.034f, 45.5f, 37.23f, 7.5f);
        result = vect1 * quat1;
        CHECK_MESSAGE(is_approx_equal(33217.703f, result.mV[0], "2. LLVector3 operator*(const LLVector3 &a, const LLQuaternion &rot) failed") &&
            is_approx_equal(295383.8125f, result.mV[1]) &&
            is_approx_equal(84718.140f, result.mV[2]));
    
}

TEST_CASE("test_12")
{

        LLVector3d vect(-2.0f, 5.0f, -6.0f);
        LLQuaternion quat(-3.5f, 4.5f, 3.5f, 6.5f);
        LLVector3d result = vect * quat;
        CHECK_MESSAGE((-633.0f == result.mdV[0], "1. LLVector3d operator*(const LLVector3d &a, const LLQuaternion &rot) failed ") &&
            (-300.0f == result.mdV[1]) &&
            (-36.0f == result.mdV[2]));

        LLVector3d vect1(5.0f, -4.5f, 8.21f);
        LLQuaternion quat1(2.0f, 4.5f, -7.2f, 9.5f);
        result = vect1 * quat1;
        CHECK_MESSAGE(is_approx_equal_fraction(-120.29f, (F32, "2. LLVector3d operator*(const LLVector3d &a, const LLQuaternion &rot) failed") result.mdV[0], 8) &&
            is_approx_equal_fraction(-1683.958f, (F32) result.mdV[1], 8) &&
            is_approx_equal_fraction(516.56f, (F32) result.mdV[2], 8));

        LLVector3d vect2(2.0f, 3.5f, 1.1f);
        LLQuaternion quat2(1.0f, 4.0f, 2.0f, 5.0f);
        result = vect2 * quat2;
        CHECK_MESSAGE(is_approx_equal_fraction(18.400001f, (F32, "3. LLVector3d operator*(const LLVector3d &a, const LLQuaternion &rot) failed") result.mdV[0], 8) &&
            is_approx_equal_fraction(188.6f, (F32) result.mdV[1], 8) &&
            is_approx_equal_fraction(32.20f, (F32) result.mdV[2], 8));
    
}

TEST_CASE("test_13")
{

        LLQuaternion quat(23.5f, 34.5f, 16723.4f, 324.7f);
        LLQuaternion result = -quat;
        CHECK_MESSAGE((-23.5f == result.mQ[0], "1. LLQuaternion operator-(const LLQuaternion &a) failed") &&
            (-34.5f == result.mQ[1]) &&
            (-16723.4f == result.mQ[2]) &&
            (-324.7f == result.mQ[3]));

        LLQuaternion quat1(-3.5f, -34.5f, -16.4f, -154.7f);
        result = -quat1;
        CHECK_MESSAGE((3.5f == result.mQ[0], "2. LLQuaternion operator-(const LLQuaternion &a) failed.") &&
            (34.5f == result.mQ[1]) &&
            (16.4f == result.mQ[2]) &&
            (154.7f == result.mQ[3]));
    
}

TEST_CASE("test_14")
{

        LLQuaternion quat_value(9.0f, 8.0f, 7.0f, 6.0f);
        F32 a =3.5f;
        LLQuaternion result = a * quat_value;
        LLQuaternion result1 = quat_value * a;

        CHECK_MESSAGE((result.mQ[0] == result1.mQ[0], "1. LLQuaternion operator* failed") &&
            (result.mQ[1] == result1.mQ[1]) &&
            (result.mQ[2] == result1.mQ[2]) &&
            (result.mQ[3] == result1.mQ[3]));


        LLQuaternion quat_val(9454.0f, 43568.3450f, 456343247.0343f, 2346.03434f);
        a =-3324.3445f;
        result = a * quat_val;
        result1 = quat_val * a;

        CHECK_MESSAGE((result.mQ[0] == result1.mQ[0], "2. LLQuaternion operator* failed") &&
            (result.mQ[1] == result1.mQ[1]) &&
            (result.mQ[2] == result1.mQ[2]) &&
            (result.mQ[3] == result1.mQ[3]));
    
}

TEST_CASE("test_15")
{

        // test cases for inline LLQuaternion operator~(const LLQuaternion &a)
        LLQuaternion quat_val(2323.634f, -43535.4f, 3455.88f, -32232.45f);
        LLQuaternion result = ~quat_val;
        CHECK_MESSAGE((-2323.634f == result.mQ[0], "1. LLQuaternion operator~(const LLQuaternion &a) failed ") &&
            (43535.4f == result.mQ[1]) &&
            (-3455.88f == result.mQ[2]) &&
            (-32232.45f == result.mQ[3]));

        //test case for inline bool LLQuaternion::operator==(const LLQuaternion &b) const
        LLQuaternion quat_val1(2323.634f, -43535.4f, 3455.88f, -32232.45f);
        LLQuaternion quat_val2(2323.634f, -43535.4f, 3455.88f, -32232.45f);
        CHECK_MESSAGE(quat_val1 == quat_val2, "2. LLQuaternion::operator==(const LLQuaternion &b) failed");
    
}

TEST_CASE("test_16")
{

        //test case for inline bool LLQuaternion::operator!=(const LLQuaternion &b) const
        LLQuaternion quat_val1(2323.634f, -43535.4f, 3455.88f, -32232.45f);
        LLQuaternion quat_val2(0, -43535.4f, 3455.88f, -32232.45f);
        CHECK_MESSAGE(quat_val1 != quat_val2, "LLQuaternion::operator!=(const LLQuaternion &b) failed");
    
}

TEST_CASE("test_17")
{

        //test case for LLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order)
        F32 x = 2.0f;
        F32 y = 1.0f;
        F32 z = 3.0f;

        LLQuaternion result = mayaQ(x, y, z, LLQuaternion::XYZ);
        CHECK_MESSAGE(is_approx_equal_fraction(0.0172174f, result.mQ[0], 16, "1. LLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order) failed for XYZ") &&
            is_approx_equal_fraction(0.009179f, result.mQ[1], 16) &&
            is_approx_equal_fraction(0.026020f, result.mQ[2], 16) &&
            is_approx_equal_fraction(0.999471f, result.mQ[3], 16));

        LLQuaternion result1 = mayaQ(x, y, z, LLQuaternion::YZX);
        CHECK_MESSAGE(is_approx_equal_fraction(0.017217f, result1.mQ[0], 16, "2. LLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order) failed for XYZ") &&
            is_approx_equal_fraction(0.008265f, result1.mQ[1], 16) &&
            is_approx_equal_fraction(0.026324f, result1.mQ[2], 16) &&
            is_approx_equal_fraction(0.999471f, result1.mQ[3], 16));

        LLQuaternion result2 = mayaQ(x, y, z, LLQuaternion::ZXY);
        CHECK_MESSAGE(is_approx_equal_fraction(0.017674f, result2.mQ[0], 16, "3. LLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order) failed for ZXY") &&
            is_approx_equal_fraction(0.008265f, result2.mQ[1], 16) &&
            is_approx_equal_fraction(0.026020f, result2.mQ[2], 16) &&
            is_approx_equal_fraction(0.999471f, result2.mQ[3], 16));

        LLQuaternion result3 = mayaQ(x, y, z, LLQuaternion::XZY);
        CHECK_MESSAGE(is_approx_equal_fraction(0.017674f, result3.mQ[0], 16, "4. TLLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order) failed for XZY") &&
            is_approx_equal_fraction(0.009179f, result3.mQ[1], 16) &&
            is_approx_equal_fraction(0.026020f, result3.mQ[2], 16) &&
            is_approx_equal_fraction(0.999463f, result3.mQ[3], 16));

        LLQuaternion result4 = mayaQ(x, y, z, LLQuaternion::YXZ);
        CHECK_MESSAGE(is_approx_equal_fraction(0.017217f, result4.mQ[0], 16, "5. LLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order) failed for YXZ") &&
            is_approx_equal_fraction(0.009179f, result4.mQ[1], 16) &&
            is_approx_equal_fraction(0.026324f, result4.mQ[2], 16) &&
            is_approx_equal_fraction(0.999463f, result4.mQ[3], 16));

        LLQuaternion result5 = mayaQ(x, y, z, LLQuaternion::ZYX);
        CHECK_MESSAGE(is_approx_equal_fraction(0.017674f, result5.mQ[0], 16, "6. LLQuaternion mayaQ(F32 xRot, F32 yRot, F32 zRot, LLQuaternion::Order order) failed for ZYX") &&
            is_approx_equal_fraction(0.008265f, result5.mQ[1], 16) &&
            is_approx_equal_fraction(0.026324f, result5.mQ[2], 16) &&
            is_approx_equal_fraction(0.999463f, result5.mQ[3], 16));
    
}

TEST_CASE("test_18")
{

        // test case for friend std::ostream& operator<<(std::ostream &s, const LLQuaternion &a) fn
        LLQuaternion a(1.0f, 1.0f, 1.0f, 1.0f);
        std::ostringstream result_value;
        result_value << a;
        ensure_equals("1. Operator << failed", result_value.str(), "{ 1, 1, 1, 1 
}

TEST_CASE("test_19")
{

        //test case for const char *OrderToString( const LLQuaternion::Order order ) fn
        const char* result = OrderToString(LLQuaternion::XYZ);
        CHECK_MESSAGE((0 == strcmp("XYZ", result, "1. OrderToString failed for XYZ")));

        result = OrderToString(LLQuaternion::YZX);
        CHECK_MESSAGE((0 == strcmp("YZX", result, "2. OrderToString failed for YZX")));

        result = OrderToString(LLQuaternion::ZXY);
        CHECK_MESSAGE((0 == strcmp("ZXY", result, "3. OrderToString failed for ZXY")) &&
            (0 != strcmp("XYZ", result)) &&
            (0 != strcmp("YXZ", result)) &&
            (0 != strcmp("ZYX", result)) &&
            (0 != strcmp("XYZ", result)));

        result = OrderToString(LLQuaternion::XZY);
        CHECK_MESSAGE((0 == strcmp("XZY", result, "4. OrderToString failed for XZY")));

        result = OrderToString(LLQuaternion::ZYX);
        CHECK_MESSAGE((0 == strcmp("ZYX", result, "5. OrderToString failed for ZYX")));

        result = OrderToString(LLQuaternion::YXZ);
        CHECK_MESSAGE((0 == strcmp("YXZ", result, "6.OrderToString failed for YXZ")));
    
}

TEST_CASE("test_20")
{

        //test case for LLQuaternion::Order StringToOrder( const char *str ) fn
        int result = StringToOrder("XYZ");
        CHECK_MESSAGE(0 == result, "1. LLQuaternion::Order StringToOrder(const char *str ) failed for XYZ");

        result = StringToOrder("YZX");
        CHECK_MESSAGE(1 == result, "2. LLQuaternion::Order StringToOrder(const char *str) failed for YZX");

        result = StringToOrder("ZXY");
        CHECK_MESSAGE(2 == result, "3. LLQuaternion::Order StringToOrder(const char *str) failed for ZXY");

        result = StringToOrder("XZY");
        CHECK_MESSAGE(3 == result, "4. LLQuaternion::Order StringToOrder(const char *str) failed for XZY");

        result = StringToOrder("YXZ");
        CHECK_MESSAGE(4 == result, "5. LLQuaternion::Order StringToOrder(const char *str) failed for YXZ");

        result = StringToOrder("ZYX");
        CHECK_MESSAGE(5 == result, "6. LLQuaternion::Order StringToOrder(const char *str) failed for  ZYX");

    
}

TEST_CASE("test_21")
{

        //void LLQuaternion::getAngleAxis(F32* angle, LLVector3 &vec) const fn
        F32 angle_value = 90.0f;
        LLVector3 vect(12.0f, 4.0f, 1.0f);
        LLQuaternion llquat(angle_value, vect);
        llquat.getAngleAxis(&angle_value, vect);
        CHECK_MESSAGE(is_approx_equal_fraction(2.035406f, angle_value, 16, "LLQuaternion::getAngleAxis(F32* angle, LLVector3 &vec) failed") &&
            is_approx_equal_fraction(0.315244f, vect.mV[1], 16) &&
            is_approx_equal_fraction(0.078811f, vect.mV[2], 16) &&
            is_approx_equal_fraction(0.945733f, vect.mV[0], 16));
    
}

TEST_CASE("test_22")
{

        //test case for void LLQuaternion::getEulerAngles(F32 *roll, F32 *pitch, F32 *yaw) const fn
        F32 roll = -12.0f;
        F32 pitch = -22.43f;
        F32 yaw = 11.0f;

        LLQuaternion llquat;
        llquat.getEulerAngles(&roll, &pitch, &yaw);
        CHECK_MESSAGE(is_approx_equal(0.000f, llquat.mQ[0], "LLQuaternion::getEulerAngles(F32 *roll, F32 *pitch, F32 *yaw) failed") &&
            is_approx_equal(0.000f, llquat.mQ[1]) &&
            is_approx_equal(0.000f, llquat.mQ[2]) &&
            is_approx_equal(1.000f, llquat.mQ[3]));
    
}

} // TEST_SUITE

