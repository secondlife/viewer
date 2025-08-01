/**
 * @file m3math_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief Test cases of m3math.h
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

#include "../m3math.h"
#include "../v3math.h"
#include "../v4math.h"
#include "../m4math.h"
#include "../llquaternion.h"
#include "../v3dmath.h"

#include "../test/lldoctest.h"

#if LL_WINDOWS
// disable unreachable code warnings caused by usage of skip.
#pragma warning(disable: 4702)
#endif

#if LL_WINDOWS
// disable unreachable code warnings caused by usage of skip.
#pragma warning(disable: 4702)
#endif

TEST_SUITE("m3math_h") {

TEST_CASE("test_1")
{

        LLMatrix3 llmat3_obj;
        llmat3_obj.setIdentity();
        CHECK_MESSAGE(1.f == llmat3_obj.mMatrix[0][0] &&
                    0.f == llmat3_obj.mMatrix[0][1] &&
                    0.f == llmat3_obj.mMatrix[0][2] &&
                    0.f == llmat3_obj.mMatrix[1][0] &&
                    1.f == llmat3_obj.mMatrix[1][1] &&
                    0.f == llmat3_obj.mMatrix[1][2] &&
                    0.f == llmat3_obj.mMatrix[2][0] &&
                    0.f == llmat3_obj.mMatrix[2][1] &&
                    1.f == llmat3_obj.mMatrix[2][2], "LLMatrix3::setIdentity failed");
    
}

TEST_CASE("test_2")
{

        LLMatrix3 llmat3_obj;
        llmat3_obj.setZero();

        CHECK_MESSAGE(0.f == llmat3_obj.setZero(, "LLMatrix3::setZero failed").mMatrix[0][0] &&
                    0.f == llmat3_obj.setZero().mMatrix[0][1] &&
                    0.f == llmat3_obj.setZero().mMatrix[0][2] &&
                    0.f == llmat3_obj.setZero().mMatrix[1][0] &&
                    0.f == llmat3_obj.setZero().mMatrix[1][1] &&
                    0.f == llmat3_obj.setZero().mMatrix[1][2] &&
                    0.f == llmat3_obj.setZero().mMatrix[2][0] &&
                    0.f == llmat3_obj.setZero().mMatrix[2][1] &&
                    0.f == llmat3_obj.setZero().mMatrix[2][2]);
    
}

TEST_CASE("test_3")
{

        LLMatrix3 llmat3_obj;
        LLVector3 vect1(2, 1, 4);
        LLVector3 vect2(3, 5, 7);
        LLVector3 vect3(6, 9, 7);
        llmat3_obj.setRows(vect1, vect2, vect3);
        CHECK_MESSAGE(2 == llmat3_obj.mMatrix[0][0] &&
                        1 == llmat3_obj.mMatrix[0][1] &&
                        4 == llmat3_obj.mMatrix[0][2] &&
                        3 == llmat3_obj.mMatrix[1][0] &&
                        5 == llmat3_obj.mMatrix[1][1] &&
                        7 == llmat3_obj.mMatrix[1][2] &&
                        6 == llmat3_obj.mMatrix[2][0] &&
                        9 == llmat3_obj.mMatrix[2][1] &&
                        7 == llmat3_obj.mMatrix[2][2], "LLVector3::setRows failed ");
    
}

TEST_CASE("test_4")
{

        LLMatrix3 llmat3_obj;
        LLVector3 vect1(2, 1, 4);
        LLVector3 vect2(3, 5, 7);
        LLVector3 vect3(6, 9, 7);
        llmat3_obj.setRows(vect1, vect2, vect3);

        CHECK_MESSAGE(vect1 == llmat3_obj.getFwdRow(, "LLVector3::getFwdRow failed "));
        CHECK_MESSAGE(vect2 == llmat3_obj.getLeftRow(, "LLVector3::getLeftRow failed "));
        CHECK_MESSAGE(vect3 == llmat3_obj.getUpRow(, "LLVector3::getUpRow failed "));
    
}

TEST_CASE("test_5")
{

        LLMatrix3 llmat_obj1;
        LLMatrix3 llmat_obj2;
        LLMatrix3 llmat_obj3;

        LLVector3 llvec1(1, 3, 5);
        LLVector3 llvec2(3, 6, 1);
        LLVector3 llvec3(4, 6, 9);

        LLVector3 llvec4(1, 1, 5);
        LLVector3 llvec5(3, 6, 8);
        LLVector3 llvec6(8, 6, 2);

        LLVector3 llvec7(0, 0, 0);
        LLVector3 llvec8(0, 0, 0);
        LLVector3 llvec9(0, 0, 0);

        llmat_obj1.setRows(llvec1, llvec2, llvec3);
        llmat_obj2.setRows(llvec4, llvec5, llvec6);
        llmat_obj3.setRows(llvec7, llvec8, llvec9);
        llmat_obj3 = llmat_obj1 * llmat_obj2;
        CHECK_MESSAGE(50 == llmat_obj3.mMatrix[0][0] &&
                        49 == llmat_obj3.mMatrix[0][1] &&
                        39 == llmat_obj3.mMatrix[0][2] &&
                        29 == llmat_obj3.mMatrix[1][0] &&
                        45 == llmat_obj3.mMatrix[1][1] &&
                        65 == llmat_obj3.mMatrix[1][2] &&
                        94 == llmat_obj3.mMatrix[2][0] &&
                        94 == llmat_obj3.mMatrix[2][1] &&
                        86 == llmat_obj3.mMatrix[2][2], "LLMatrix3::operator*(const LLMatrix3 &a, const LLMatrix3 &b) failed");
    
}

TEST_CASE("test_6")
{


        LLMatrix3 llmat_obj1;

        LLVector3 llvec(1, 3, 5);
        LLVector3 res_vec(0, 0, 0);
        LLVector3 llvec1(1, 3, 5);
        LLVector3 llvec2(3, 6, 1);
        LLVector3 llvec3(4, 6, 9);

        llmat_obj1.setRows(llvec1, llvec2, llvec3);
        res_vec = llvec * llmat_obj1;

        LLVector3 expected_result(30, 51, 53);

        CHECK_MESSAGE(res_vec == expected_result, "LLMatrix3::operator*(const LLVector3 &a, const LLMatrix3 &b) failed");
    
}

TEST_CASE("test_7")
{

        LLMatrix3 llmat_obj1;
        LLVector3d llvec3d1;
        LLVector3d llvec3d2(0, 3, 4);

        LLVector3 llvec1(1, 3, 5);
        LLVector3 llvec2(3, 2, 1);
        LLVector3 llvec3(4, 6, 0);

        llmat_obj1.setRows(llvec1, llvec2, llvec3);
        llvec3d1 = llvec3d2 * llmat_obj1;

        LLVector3d expected_result(25, 30, 3);

        CHECK_MESSAGE(llvec3d1 == expected_result, "LLMatrix3::operator*(const LLVector3 &a, const LLMatrix3 &b) failed");
    
}

TEST_CASE("test_8")
{

        LLMatrix3 llmat_obj1;
        LLMatrix3 llmat_obj2;

        LLVector3 llvec1(1, 3, 5);
        LLVector3 llvec2(3, 6, 1);
        LLVector3 llvec3(4, 6, 9);

        llmat_obj1.setRows(llvec1, llvec2, llvec3);
        llmat_obj2.setRows(llvec1, llvec2, llvec3);
        CHECK_MESSAGE(llmat_obj1 == llmat_obj2, "LLMatrix3::operator==(const LLMatrix3 &a, const LLMatrix3 &b) failed");

        llmat_obj2.setRows(llvec2, llvec2, llvec3);
        CHECK_MESSAGE(llmat_obj1 != llmat_obj2, "LLMatrix3::operator!=(const LLMatrix3 &a, const LLMatrix3 &b) failed");
    
}

TEST_CASE("test_9")
{

        LLMatrix3 llmat_obj1;
        LLQuaternion llmat_quat;

        LLVector3 llmat1(2.0f, 1.0f, 6.0f);
        LLVector3 llmat2(1.0f, 1.0f, 3.0f);
        LLVector3 llmat3(1.0f, 7.0f, 5.0f);

        llmat_obj1.setRows(llmat1, llmat2, llmat3);
        llmat_quat = llmat_obj1.quaternion();
        CHECK_MESSAGE(is_approx_equal(-0.66666669f, llmat_quat.mQ[0], "LLMatrix3::quaternion failed ") &&
                        is_approx_equal(-0.83333337f, llmat_quat.mQ[1]) &&
                        is_approx_equal(0.0f, llmat_quat.mQ[2]) &&
                        is_approx_equal(1.5f, llmat_quat.mQ[3]));
    
}

TEST_CASE("test_10")
{

        LLMatrix3 llmat_obj;

        LLVector3 llvec1(1, 2, 3);
        LLVector3 llvec2(3, 2, 1);
        LLVector3 llvec3(2, 2, 2);

        llmat_obj.setRows(llvec1, llvec2, llvec3);
        llmat_obj.transpose();

        LLVector3 resllvec1(1, 3, 2);
        LLVector3 resllvec2(2, 2, 2);
        LLVector3 resllvec3(3, 1, 2);
        LLMatrix3 expectedllmat_obj;
        expectedllmat_obj.setRows(resllvec1, resllvec2, resllvec3);

        CHECK_MESSAGE(llmat_obj == expectedllmat_obj, "LLMatrix3::transpose failed ");
    
}

TEST_CASE("test_11")
{

        LLMatrix3 llmat_obj1;

        LLVector3 llvec1(1, 2, 3);
        LLVector3 llvec2(3, 2, 1);
        LLVector3 llvec3(2, 2, 2);
        llmat_obj1.setRows(llvec1, llvec2, llvec3);
        CHECK_MESSAGE(0.0f == llmat_obj1.determinant(, "LLMatrix3::determinant failed "));
    
}

TEST_CASE("test_12")
{

        LLMatrix3 llmat_obj;

        LLVector3 llvec1(1, 4, 3);
        LLVector3 llvec2(1, 2, 0);
        LLVector3 llvec3(2, 4, 2);

        skip("This test fails depending on architecture. Need to fix comparison operation, is_approx_equal, to work on more than one platform.");

        llmat_obj.setRows(llvec1, llvec2, llvec3);
        llmat_obj.orthogonalize();

        CHECK_MESSAGE(is_approx_equal(0.19611614f, llmat_obj.mMatrix[0][0], "LLMatrix3::orthogonalize failed ") &&
               is_approx_equal(0.78446454f, llmat_obj.mMatrix[0][1]) &&
               is_approx_equal(0.58834841f, llmat_obj.mMatrix[0][2]) &&
               is_approx_equal(0.47628204f, llmat_obj.mMatrix[1][0]) &&
               is_approx_equal(0.44826545f, llmat_obj.mMatrix[1][1]) &&
               is_approx_equal(-0.75644795f, llmat_obj.mMatrix[1][2]) &&
               is_approx_equal(-0.85714286f, llmat_obj.mMatrix[2][0]) &&
               is_approx_equal(0.42857143f, llmat_obj.mMatrix[2][1]) &&
               is_approx_equal(-0.28571429f, llmat_obj.mMatrix[2][2]));
    
}

TEST_CASE("test_13")
{

        LLMatrix3 llmat_obj;

        LLVector3 llvec1(3, 2, 1);
        LLVector3 llvec2(6, 2, 1);
        LLVector3 llvec3(3, 6, 8);

        llmat_obj.setRows(llvec1, llvec2, llvec3);
        llmat_obj.adjointTranspose();

        CHECK_MESSAGE(10 == llmat_obj.mMatrix[0][0] &&
                        -45 == llmat_obj.mMatrix[1][0] &&
                        30 == llmat_obj.mMatrix[2][0] &&
                        -10 == llmat_obj.mMatrix[0][1] &&
                        21 == llmat_obj.mMatrix[1][1] &&
                        -12 == llmat_obj.mMatrix[2][1] &&
                        0  == llmat_obj.mMatrix[0][2] &&
                        3 == llmat_obj.mMatrix[1][2] &&
                        -6 == llmat_obj.mMatrix[2][2], "LLMatrix3::adjointTranspose failed ");
    
}

} // TEST_SUITE

