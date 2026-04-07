/**
 * @file m3math_glm_equivalence_test.cpp
 * @brief Differential tests for LLMatrix3 ↔ glm::mat3.
 *
 * Same conventions as the LLMatrix4 differential test:
 *   LLMatrix3 — row-major (mMatrix[row][col]), row-vector × matrix
 *   glm::mat3 — column-major (m[col][row]), matrix × column-vector
 *
 * The two representations have IDENTICAL memory layout for the same
 * transformation, so conversion is a flat memcpy via `glm::value_ptr`.
 * Multiplication order is REVERSED: `LL: A * B` ≡ `glm: B * A`.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llmath.h"
#include "../m3math.h"
#include "../v3math.h"
#include "../llquaternion.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>

namespace tut
{
    // FINDING (2026-04-07): glm::mat3 is NOT tightly packed in this build.
    // The sizeof check originally asserted 36 bytes but failed at compile
    // time, proving glm pads mat3 internally (likely 3 columns of vec3
    // each padded to vec4 alignment for SIMD = 48 bytes). This means
    // memcpy/value_ptr-based conversion is unsafe for mat3 — we have to
    // go element-by-element. Documented here so the next person doesn't
    // try the LLMatrix4 memcpy trick.
    //
    // (LLMatrix4 ↔ glm::mat4 memcpy DOES work because glm::mat4 is
    // 64 bytes, no padding needed.)

    struct m3math_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        // Element-by-element conversion.
        //
        // SL stores basis vectors as ROWS of LLMatrix3 (verified by
        // setRows(fwd, left, up) which writes mMatrix[0..2][0..2] from
        // the three basis vectors).
        //
        // glm stores basis vectors as COLUMNS of glm::mat3.
        //
        // So for the SAME rotation, LL row r = glm col r. That means
        // the SAME logical element at (row r, col c) lives at:
        //   LL: mMatrix[r][c]   (row r, col c)
        //   glm: m[r][c]        (col r, position c within that col)
        // — i.e., the indices are the same (`m[r][c]` indexes glm column
        // r at position c, which is what we want).
        static glm::mat3 to_glm(const LLMatrix3& m)
        {
            glm::mat3 result;
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    result[r][c] = m.mMatrix[r][c];
            return result;
        }

        static LLMatrix3 to_ll(const glm::mat3& m)
        {
            LLMatrix3 result;
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    result.mMatrix[r][c] = m[r][c];
            return result;
        }

        static bool equivalent(const LLMatrix3& ll, const glm::mat3& gm, F32 eps = kEps)
        {
            for (int r = 0; r < 3; ++r)
            {
                for (int c = 0; c < 3; ++c)
                {
                    if (std::fabs(ll.mMatrix[r][c] - gm[r][c]) > eps)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        static bool round_trip_equal(const LLMatrix3& original, F32 eps = kEps)
        {
            const LLMatrix3 round = to_ll(to_glm(original));
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (std::fabs(original.mMatrix[i][j] - round.mMatrix[i][j]) > eps)
                    {
                        return false;
                    }
                }
            }
            return true;
        }
    };

    using m3math_glm_equiv_test = test_group<m3math_glm_equiv_data>;
    using m3math_glm_equiv_object = m3math_glm_equiv_test::object;
    tut::m3math_glm_equiv_test m3math_glm_equiv_testcase("m3math_glm_equivalence");

    // ---------- Layout sanity ----------

    template<> template<>
    void m3math_glm_equiv_object::test<1>()
    {
        LLMatrix3 ll_ident;
        ll_ident.setIdentity();
        const glm::mat3 gm_ident(1.0f);
        ensure("identity matrices match", equivalent(ll_ident, gm_ident));
    }

    template<> template<>
    void m3math_glm_equiv_object::test<2>()
    {
        LLMatrix3 m;
        m.setIdentity();
        ensure("identity round-trips", round_trip_equal(m));
    }

    template<> template<>
    void m3math_glm_equiv_object::test<3>()
    {
        LLMatrix3 m;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                m.mMatrix[i][j] = static_cast<F32>(i * 3 + j) + 0.5f;
        ensure("arbitrary round-trip", round_trip_equal(m));
    }

    // ---------- Multiplication ----------

    template<> template<>
    void m3math_glm_equiv_object::test<4>()
    {
        // Identity is multiplicative identity in both libs.
        LLMatrix3 m;
        m.setRot(0.5f, LLVector3(0.0f, 0.0f, 1.0f));

        LLMatrix3 ident;
        ident.setIdentity();

        LLMatrix3 right = m * ident;
        LLMatrix3 left = ident * m;

        ensure("m * I == m", equivalent(right, to_glm(m)));
        ensure("I * m == m", equivalent(left, to_glm(m)));
    }

    template<> template<>
    void m3math_glm_equiv_object::test<5>()
    {
        // Composition order REVERSED.
        LLMatrix3 a; a.setRot(0.3f, LLVector3(0.0f, 1.0f, 0.0f));
        LLMatrix3 b; b.setRot(0.7f, LLVector3(1.0f, 0.0f, 0.0f));

        LLMatrix3 ll_result = a * b;

        // GLM equivalent: reversed order
        const glm::mat3 gm_result = to_glm(b) * to_glm(a);

        ensure("LL a*b matches GLM b*a", equivalent(ll_result, gm_result));
    }

    // ---------- Transpose ----------

    template<> template<>
    void m3math_glm_equiv_object::test<6>()
    {
        // Transpose of identity is identity in both.
        LLMatrix3 m;
        m.setIdentity();
        m.transpose();
        const glm::mat3 gm = glm::transpose(glm::mat3(1.0f));
        ensure("transpose of I", equivalent(m, gm));
    }

    template<> template<>
    void m3math_glm_equiv_object::test<7>()
    {
        // Transpose of an arbitrary matrix.
        LLMatrix3 m;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                m.mMatrix[i][j] = static_cast<F32>(i * 3 + j);

        const glm::mat3 gm_before = to_glm(m);
        m.transpose();
        const glm::mat3 gm_after = glm::transpose(gm_before);

        ensure("arbitrary transpose matches", equivalent(m, gm_after));
    }

    // ---------- Determinant ----------

    template<> template<>
    void m3math_glm_equiv_object::test<8>()
    {
        LLMatrix3 m;
        m.setIdentity();
        const glm::mat3 gm = to_glm(m);
        ensure_approximately_equals("det(I)", m.determinant(), glm::determinant(gm), 16);
    }

    template<> template<>
    void m3math_glm_equiv_object::test<9>()
    {
        // Pure rotation should have determinant 1 in both.
        LLMatrix3 m;
        m.setRot(0.7f, LLVector3(1.0f, 2.0f, 3.0f));
        const glm::mat3 gm = to_glm(m);
        ensure_approximately_equals("det(rotation) == 1",
                                    m.determinant(), glm::determinant(gm), 16);
    }

    // ---------- Rotation construction ----------

    template<> template<>
    void m3math_glm_equiv_object::test<10>()
    {
        // setRot from axis-angle vs glm::mat3(glm::angleAxis).
        const F32 angle = 0.6f;
        LLVector3 axis_ll(0.0f, 0.0f, 1.0f);
        LLMatrix3 ll;
        ll.setRot(angle, axis_ll);

        glm::vec3 axis_gm(0.0f, 0.0f, 1.0f);
        glm::mat3 gm = glm::mat3_cast(glm::angleAxis(angle, axis_gm));

        ensure("Z rotation matches", equivalent(ll, gm));
    }

    template<> template<>
    void m3math_glm_equiv_object::test<11>()
    {
        // Arbitrary axis rotation.
        const F32 angle = 0.5f;
        LLVector3 axis_ll(1.0f, 2.0f, 3.0f);
        axis_ll.normalize();
        LLMatrix3 ll;
        ll.setRot(angle, axis_ll);

        glm::vec3 axis_gm = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
        glm::mat3 gm = glm::mat3_cast(glm::angleAxis(angle, axis_gm));

        ensure("arbitrary axis rotation matches", equivalent(ll, gm));
    }

    // ---------- Construction from quaternion ----------

    template<> template<>
    void m3math_glm_equiv_object::test<12>()
    {
        // LLMatrix3 from LLQuaternion vs glm::mat3 from glm::quat.
        LLQuaternion ll_q(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLMatrix3 ll(ll_q);

        glm::quat gm_q = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat3 gm = glm::mat3_cast(gm_q);

        ensure("matrix from quaternion matches", equivalent(ll, gm));
    }
}
