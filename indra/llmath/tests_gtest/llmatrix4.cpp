/**
 * @file llmatrix4.cpp
 * @brief GoogleTest unit tests for LLMatrix4.
 *
 * LLMatrix4 had zero unit-test coverage in the legacy TUT suite. These tests
 * cover identity, translation, rotation, scale, transpose, inverse, and
 * matrix-vector multiplication — i.e. the basic invariants we'll need
 * intact during the GLM migration.
 */

#include "linden_common.h"

#include "m4math.h"
#include "v3math.h"
#include "v4math.h"
#include "llquaternion.h"

#include <gtest/gtest.h>

#include <cmath>

namespace
{
    constexpr F32 kEps = 1e-5f;

    ::testing::AssertionResult MatricesNear(const LLMatrix4& actual,
                                            const LLMatrix4& expected,
                                            F32 eps = kEps)
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                const F32 diff = std::fabs(actual.mMatrix[i][j] - expected.mMatrix[i][j]);
                if (diff > eps)
                {
                    return ::testing::AssertionFailure()
                        << "matrices differ at [" << i << "][" << j << "]: "
                        << "actual=" << actual.mMatrix[i][j]
                        << " expected=" << expected.mMatrix[i][j]
                        << " diff=" << diff;
                }
            }
        }
        return ::testing::AssertionSuccess();
    }

    ::testing::AssertionResult VectorsNear(const LLVector3& actual,
                                           const LLVector3& expected,
                                           F32 eps = kEps)
    {
        for (int i = 0; i < 3; ++i)
        {
            const F32 diff = std::fabs(actual.mV[i] - expected.mV[i]);
            if (diff > eps)
            {
                return ::testing::AssertionFailure()
                    << "vectors differ at [" << i << "]: "
                    << "actual=" << actual.mV[i]
                    << " expected=" << expected.mV[i];
            }
        }
        return ::testing::AssertionSuccess();
    }
}

// ---------- Construction & identity ----------

TEST(LLMatrix4Test, DefaultConstructorIsIdentity)
{
    LLMatrix4 m;
    EXPECT_TRUE(m.isIdentity());
}

TEST(LLMatrix4Test, SetIdentityIsIdentity)
{
    LLMatrix4 m;
    m.setZero();
    EXPECT_FALSE(m.isIdentity());
    m.setIdentity();
    EXPECT_TRUE(m.isIdentity());
}

TEST(LLMatrix4Test, IdentityHasOnesOnDiagonal)
{
    LLMatrix4 m;
    m.setIdentity();
    EXPECT_FLOAT_EQ(m.mMatrix[0][0], 1.0f);
    EXPECT_FLOAT_EQ(m.mMatrix[1][1], 1.0f);
    EXPECT_FLOAT_EQ(m.mMatrix[2][2], 1.0f);
    EXPECT_FLOAT_EQ(m.mMatrix[3][3], 1.0f);
}

TEST(LLMatrix4Test, ZeroMatrixIsAllZeros)
{
    LLMatrix4 m;
    m.setZero();
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            EXPECT_FLOAT_EQ(m.mMatrix[i][j], 0.0f);
        }
    }
}

// ---------- Translation ----------

TEST(LLMatrix4Test, SetTranslationStoresInRowThree)
{
    LLMatrix4 m;
    m.setTranslation(1.0f, 2.0f, 3.0f);
    // LLMatrix4 stores translation in row 3 (mMatrix[3][0..2])
    EXPECT_FLOAT_EQ(m.mMatrix[3][0], 1.0f);
    EXPECT_FLOAT_EQ(m.mMatrix[3][1], 2.0f);
    EXPECT_FLOAT_EQ(m.mMatrix[3][2], 3.0f);
    EXPECT_FLOAT_EQ(m.mMatrix[3][3], 1.0f);
}

TEST(LLMatrix4Test, TranslateAffectsPoint)
{
    LLMatrix4 m;
    m.setTranslation(10.0f, 20.0f, 30.0f);

    const LLVector3 origin(0.0f, 0.0f, 0.0f);
    // operator*(LLVector3, LLMatrix4) is the row-vector * matrix convention used here.
    const LLVector3 result = origin * m;
    EXPECT_TRUE(VectorsNear(result, LLVector3(10.0f, 20.0f, 30.0f)));
}

// ---------- Multiplication ----------

TEST(LLMatrix4Test, IdentityIsMultiplicativeIdentity)
{
    LLMatrix4 m;
    m.setTranslation(5.0f, -2.0f, 7.0f);
    m.mMatrix[0][1] = 0.5f;  // throw in some non-trivial values
    m.mMatrix[2][0] = -1.5f;

    LLMatrix4 ident;
    ident.setIdentity();

    LLMatrix4 a = m;
    a *= ident;
    LLMatrix4 b = ident;
    b *= m;

    EXPECT_TRUE(MatricesNear(a, m));
    EXPECT_TRUE(MatricesNear(b, m));
}

TEST(LLMatrix4Test, TranslationCompositionAdds)
{
    LLMatrix4 a; a.setTranslation(1.0f, 2.0f, 3.0f);
    LLMatrix4 b; b.setTranslation(10.0f, 20.0f, 30.0f);
    LLMatrix4 c = a;
    c *= b;

    LLMatrix4 expected;
    expected.setTranslation(11.0f, 22.0f, 33.0f);
    EXPECT_TRUE(MatricesNear(c, expected));
}

// ---------- Transpose ----------

TEST(LLMatrix4Test, TransposeOfIdentityIsIdentity)
{
    LLMatrix4 m;
    m.setIdentity();
    m.transpose();
    EXPECT_TRUE(m.isIdentity());
}

TEST(LLMatrix4Test, TransposeIsInvolution)
{
    LLMatrix4 m;
    // Fill with distinct values
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m.mMatrix[i][j] = static_cast<F32>(i * 4 + j);

    LLMatrix4 original = m;
    m.transpose();
    m.transpose();
    EXPECT_TRUE(MatricesNear(m, original));
}

TEST(LLMatrix4Test, TransposeSwapsOffDiagonal)
{
    LLMatrix4 m;
    m.setIdentity();
    m.mMatrix[0][1] = 7.0f;
    m.transpose();
    EXPECT_FLOAT_EQ(m.mMatrix[1][0], 7.0f);
    EXPECT_FLOAT_EQ(m.mMatrix[0][1], 0.0f);
}

// ---------- Inverse ----------

TEST(LLMatrix4Test, InverseOfIdentityIsIdentity)
{
    LLMatrix4 m;
    m.setIdentity();
    m.invert();
    EXPECT_TRUE(m.isIdentity());
}

TEST(LLMatrix4Test, InverseTimesOriginalIsIdentity)
{
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
    EXPECT_TRUE(MatricesNear(forward, ident, 1e-4f));
    EXPECT_TRUE(MatricesNear(backward, ident, 1e-4f));
}

// ---------- Rotation ----------

TEST(LLMatrix4Test, RotationAroundZ90DegreesMapsXToY)
{
    LLMatrix4 m;
    m.initRotation(F_PI_BY_TWO, LLVector4(0.0f, 0.0f, 1.0f, 0.0f));

    const LLVector3 x_axis(1.0f, 0.0f, 0.0f);
    const LLVector3 result = x_axis * m;

    EXPECT_TRUE(VectorsNear(result, LLVector3(0.0f, 1.0f, 0.0f), 1e-5f));
}

TEST(LLMatrix4Test, IdentityRotationLeavesVectorsAlone)
{
    LLMatrix4 m;
    m.initRotation(0.0f, LLVector4(0.0f, 0.0f, 1.0f, 0.0f));

    const LLVector3 v(2.0f, 3.0f, 4.0f);
    EXPECT_TRUE(VectorsNear(v * m, v));
}

// ---------- Determinant ----------

TEST(LLMatrix4Test, IdentityDeterminantIsOne)
{
    LLMatrix4 m;
    m.setIdentity();
    EXPECT_NEAR(m.determinant(), 1.0f, kEps);
}

TEST(LLMatrix4Test, ZeroMatrixDeterminantIsZero)
{
    LLMatrix4 m;
    m.setZero();
    EXPECT_NEAR(m.determinant(), 0.0f, kEps);
}

TEST(LLMatrix4Test, TranslationDeterminantIsOne)
{
    LLMatrix4 m;
    m.setTranslation(7.0f, -3.0f, 11.0f);
    EXPECT_NEAR(m.determinant(), 1.0f, kEps);
}
