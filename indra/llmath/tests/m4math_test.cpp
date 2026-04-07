/**
 * @file m4math_test.cpp
 * @brief LLMatrix4 test cases (TUT framework).
 *
 * LLMatrix4 had zero unit-test coverage in the legacy suite. These tests
 * cover identity, translation, rotation, scale, transpose, inverse, and
 * matrix-vector multiplication — i.e. the basic invariants we need
 * intact during the GLM migration.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../m4math.h"
#include "../v3math.h"
#include "../v4math.h"
#include "../llquaternion.h"

#include <cmath>

namespace tut
{
    struct m4math_data
    {
        // Per-test fixture (empty — tests construct their own matrices)

        // Convenience: matrix-near comparator with default epsilon.
        static bool matrices_near(const LLMatrix4& a, const LLMatrix4& b, F32 eps = 1e-5f)
        {
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    if (std::fabs(a.mMatrix[i][j] - b.mMatrix[i][j]) > eps)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        static bool vectors_near(const LLVector3& a, const LLVector3& b, F32 eps = 1e-5f)
        {
            for (int i = 0; i < 3; ++i)
            {
                if (std::fabs(a.mV[i] - b.mV[i]) > eps)
                {
                    return false;
                }
            }
            return true;
        }
    };

    using m4math_test = test_group<m4math_data>;
    using m4math_object = m4math_test::object;
    tut::m4math_test m4math_testcase("m4math_h");

    // ---------- Construction & identity ----------

    template<> template<>
    void m4math_object::test<1>()
    {
        // default constructor produces identity
        LLMatrix4 m;
        ensure("default ctor is identity", m.isIdentity());
    }

    template<> template<>
    void m4math_object::test<2>()
    {
        // setIdentity restores identity after setZero
        LLMatrix4 m;
        m.setZero();
        ensure("after setZero, not identity", !m.isIdentity());
        m.setIdentity();
        ensure("after setIdentity, identity", m.isIdentity());
    }

    template<> template<>
    void m4math_object::test<3>()
    {
        // identity has 1s on diagonal
        LLMatrix4 m;
        m.setIdentity();
        ensure_equals("[0][0]", m.mMatrix[0][0], 1.0f);
        ensure_equals("[1][1]", m.mMatrix[1][1], 1.0f);
        ensure_equals("[2][2]", m.mMatrix[2][2], 1.0f);
        ensure_equals("[3][3]", m.mMatrix[3][3], 1.0f);
    }

    template<> template<>
    void m4math_object::test<4>()
    {
        // setZero produces all zeros
        LLMatrix4 m;
        m.setZero();
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                ensure_equals("zero element", m.mMatrix[i][j], 0.0f);
            }
        }
    }

    // ---------- Translation ----------

    template<> template<>
    void m4math_object::test<5>()
    {
        // setTranslation stores in row 3
        LLMatrix4 m;
        m.setTranslation(1.0f, 2.0f, 3.0f);
        ensure_equals("translation x", m.mMatrix[3][0], 1.0f);
        ensure_equals("translation y", m.mMatrix[3][1], 2.0f);
        ensure_equals("translation z", m.mMatrix[3][2], 3.0f);
        ensure_equals("translation w", m.mMatrix[3][3], 1.0f);
    }

    template<> template<>
    void m4math_object::test<6>()
    {
        // translating origin gives translation vector
        LLMatrix4 m;
        m.setTranslation(10.0f, 20.0f, 30.0f);
        const LLVector3 origin(0.0f, 0.0f, 0.0f);
        const LLVector3 result = origin * m;
        ensure("translated origin", vectors_near(result, LLVector3(10.0f, 20.0f, 30.0f)));
    }

    // ---------- Multiplication ----------

    template<> template<>
    void m4math_object::test<7>()
    {
        // identity is multiplicative identity (left and right)
        LLMatrix4 m;
        m.setTranslation(5.0f, -2.0f, 7.0f);
        m.mMatrix[0][1] = 0.5f;
        m.mMatrix[2][0] = -1.5f;

        LLMatrix4 ident;
        ident.setIdentity();

        LLMatrix4 right = m;
        right *= ident;
        LLMatrix4 left = ident;
        left *= m;

        ensure("m * I == m", matrices_near(right, m));
        ensure("I * m == m", matrices_near(left, m));
    }

    template<> template<>
    void m4math_object::test<8>()
    {
        // composing translations adds them
        LLMatrix4 a; a.setTranslation(1.0f, 2.0f, 3.0f);
        LLMatrix4 b; b.setTranslation(10.0f, 20.0f, 30.0f);
        LLMatrix4 c = a;
        c *= b;

        LLMatrix4 expected;
        expected.setTranslation(11.0f, 22.0f, 33.0f);
        ensure("translation composition", matrices_near(c, expected));
    }

    // ---------- Transpose ----------

    template<> template<>
    void m4math_object::test<9>()
    {
        // transpose of identity is identity
        LLMatrix4 m;
        m.setIdentity();
        m.transpose();
        ensure("transpose of I is I", m.isIdentity());
    }

    template<> template<>
    void m4math_object::test<10>()
    {
        // transpose is involution (T(T(M)) == M)
        LLMatrix4 m;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m.mMatrix[i][j] = static_cast<F32>(i * 4 + j);

        LLMatrix4 original = m;
        m.transpose();
        m.transpose();
        ensure("T(T(M)) == M", matrices_near(m, original));
    }

    template<> template<>
    void m4math_object::test<11>()
    {
        // transpose swaps off-diagonal elements
        LLMatrix4 m;
        m.setIdentity();
        m.mMatrix[0][1] = 7.0f;
        m.transpose();
        ensure_equals("[1][0] after transpose", m.mMatrix[1][0], 7.0f);
        ensure_equals("[0][1] after transpose", m.mMatrix[0][1], 0.0f);
    }

    // ---------- Inverse ----------

    template<> template<>
    void m4math_object::test<12>()
    {
        // inverse of identity is identity
        LLMatrix4 m;
        m.setIdentity();
        m.invert();
        ensure("inv(I) == I", m.isIdentity());
    }

    template<> template<>
    void m4math_object::test<13>()
    {
        // M * M^-1 == I  and  M^-1 * M == I  (rotation+translation)
        LLMatrix4 original;
        original.initRotTrans(0.5f, LLVector3(0.0f, 0.0f, 1.0f), LLVector3(3.0f, 4.0f, 5.0f));

        LLMatrix4 inverse = original;
        inverse.invert();

        LLMatrix4 ident;
        ident.setIdentity();
        LLMatrix4 forward = original;
        forward *= inverse;
        LLMatrix4 backward = inverse;
        backward *= original;

        ensure("M * M^-1 == I", matrices_near(forward, ident, 1e-4f));
        ensure("M^-1 * M == I", matrices_near(backward, ident, 1e-4f));
    }

    // ---------- Rotation ----------

    template<> template<>
    void m4math_object::test<14>()
    {
        // 90deg around Z maps +X to +Y
        LLMatrix4 m;
        m.initRotation(F_PI_BY_TWO, LLVector4(0.0f, 0.0f, 1.0f, 0.0f));

        const LLVector3 x_axis(1.0f, 0.0f, 0.0f);
        const LLVector3 result = x_axis * m;

        ensure("90deg Z rotates +X to +Y", vectors_near(result, LLVector3(0.0f, 1.0f, 0.0f), 1e-5f));
    }

    template<> template<>
    void m4math_object::test<15>()
    {
        // zero-angle rotation leaves vectors alone
        LLMatrix4 m;
        m.initRotation(0.0f, LLVector4(0.0f, 0.0f, 1.0f, 0.0f));
        const LLVector3 v(2.0f, 3.0f, 4.0f);
        ensure("identity rotation preserves vector", vectors_near(v * m, v));
    }

    // ---------- Determinant ----------

    template<> template<>
    void m4math_object::test<16>()
    {
        // det(I) == 1
        LLMatrix4 m;
        m.setIdentity();
        ensure_approximately_equals("det(I) == 1", m.determinant(), 1.0f, 16);
    }

    template<> template<>
    void m4math_object::test<17>()
    {
        // det(0) == 0
        LLMatrix4 m;
        m.setZero();
        ensure_approximately_equals("det(0) == 0", m.determinant(), 0.0f, 16);
    }

    template<> template<>
    void m4math_object::test<18>()
    {
        // det of pure translation == 1 (translation is in row 3, doesn't affect 3x3 minor)
        LLMatrix4 m;
        m.setTranslation(7.0f, -3.0f, 11.0f);
        ensure_approximately_equals("det(translation) == 1", m.determinant(), 1.0f, 16);
    }
}
