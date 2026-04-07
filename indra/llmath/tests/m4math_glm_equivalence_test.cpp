/**
 * @file m4math_glm_equivalence_test.cpp
 * @brief Differential tests proving LLMatrix4 and glm::mat4 produce identical
 *        results for the same inputs/operations.
 *
 * This is the safety net for the GLM migration. Any divergence between
 * LL math and GLM math should be caught here before it reaches the
 * rendering pipeline. If you're touching LLMatrix4 or its callers, run
 * this test.
 *
 * Conventions:
 *   LLMatrix4 — row-major storage (mMatrix[row][col]), row-vector × matrix
 *               (i.e. v_world = v_local * M_local_to_world).
 *   glm::mat4 — column-major storage (m[col][row]), matrix × column-vector
 *               (i.e. v_world = M_local_to_world * v_local).
 *
 * The two layouts happen to be bit-equivalent in memory for the same
 * underlying transformation, so we can convert with a flat memcpy. The
 * MULTIPLICATION ORDER is reversed: LL `A * B` corresponds to GLM `B * A`.
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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>

namespace tut
{
    struct m4math_glm_equiv_data
    {
        // Tolerance for float comparisons. GLM uses the same SSE precision
        // as LL, so element-wise differences should be tiny.
        static constexpr F32 kEps = 1e-5f;

        // Convert LLMatrix4 → glm::mat4. The two have identical memory
        // layout: row-major × row-vector matches column-major × column-vector
        // for the same transformation.
        static glm::mat4 to_glm(const LLMatrix4& m)
        {
            glm::mat4 result;
            std::memcpy(glm::value_ptr(result), &m.mMatrix[0][0], sizeof(F32) * 16);
            return result;
        }

        // Convert back the same way.
        static LLMatrix4 to_ll(const glm::mat4& m)
        {
            LLMatrix4 result;
            std::memcpy(&result.mMatrix[0][0], glm::value_ptr(m), sizeof(F32) * 16);
            return result;
        }

        // Element-wise comparison of LL and GLM matrices.
        static bool equivalent(const LLMatrix4& ll, const glm::mat4& gm, F32 eps = kEps)
        {
            const F32* ll_data = &ll.mMatrix[0][0];
            const F32* gm_data = glm::value_ptr(gm);
            for (int i = 0; i < 16; ++i)
            {
                if (std::fabs(ll_data[i] - gm_data[i]) > eps)
                {
                    return false;
                }
            }
            return true;
        }

        // Round-trip stability: ll → glm → ll should be identity.
        static bool round_trip_equal(const LLMatrix4& original, F32 eps = kEps)
        {
            const LLMatrix4 round = to_ll(to_glm(original));
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    if (std::fabs(original.mMatrix[i][j] - round.mMatrix[i][j]) > eps)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        // Vector comparison helpers.
        static bool vectors_equal(const LLVector3& ll, const glm::vec3& gm, F32 eps = kEps)
        {
            return std::fabs(ll.mV[0] - gm.x) <= eps
                && std::fabs(ll.mV[1] - gm.y) <= eps
                && std::fabs(ll.mV[2] - gm.z) <= eps;
        }
    };

    using m4math_glm_equiv_test = test_group<m4math_glm_equiv_data>;
    using m4math_glm_equiv_object = m4math_glm_equiv_test::object;
    tut::m4math_glm_equiv_test m4math_glm_equiv_testcase("m4math_glm_equivalence");

    // ---------- Layout sanity ----------

    template<> template<>
    void m4math_glm_equiv_object::test<1>()
    {
        // Identity matrices in both libraries should be element-wise equal.
        LLMatrix4 ll_ident;
        ll_ident.setIdentity();
        const glm::mat4 gm_ident(1.0f);
        ensure("identity matrices match", equivalent(ll_ident, gm_ident));
    }

    template<> template<>
    void m4math_glm_equiv_object::test<2>()
    {
        // Round-trip ll → glm → ll preserves identity.
        LLMatrix4 m;
        m.setIdentity();
        ensure("identity round-trips", round_trip_equal(m));
    }

    template<> template<>
    void m4math_glm_equiv_object::test<3>()
    {
        // Round-trip preserves an arbitrary matrix.
        LLMatrix4 m;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                m.mMatrix[i][j] = static_cast<F32>(i * 4 + j) + 0.5f;
        ensure("arbitrary round-trip", round_trip_equal(m));
    }

    // ---------- Translation ----------

    template<> template<>
    void m4math_glm_equiv_object::test<4>()
    {
        // setTranslation in LL must match glm::translate(identity) in GLM.
        LLMatrix4 ll;
        ll.setTranslation(1.0f, 2.0f, 3.0f);

        const glm::mat4 gm = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));

        ensure("translation matrices match", equivalent(ll, gm));
    }

    template<> template<>
    void m4math_glm_equiv_object::test<5>()
    {
        // Translating a point: LL uses v * M, GLM uses M * v. Same result.
        LLMatrix4 ll;
        ll.setTranslation(10.0f, 20.0f, 30.0f);

        const glm::mat4 gm = to_glm(ll);

        const LLVector3 ll_origin(0.0f, 0.0f, 0.0f);
        const LLVector3 ll_result = ll_origin * ll;

        const glm::vec3 gm_result = glm::vec3(gm * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

        ensure("translated point matches", vectors_equal(ll_result, gm_result));
    }

    // ---------- Multiplication ----------

    template<> template<>
    void m4math_glm_equiv_object::test<6>()
    {
        // Multiplication order is REVERSED between LL and GLM.
        //   LL:  result = A * B   means "apply A then B"
        //   GLM: result = B * A   means "apply A then B"
        LLMatrix4 a; a.setTranslation(1.0f, 2.0f, 3.0f);
        LLMatrix4 b; b.setTranslation(10.0f, 20.0f, 30.0f);

        LLMatrix4 ll_result = a;
        ll_result *= b;  // LL composition: apply a then b

        // Same composition in GLM (reversed order).
        const glm::mat4 gm_result = to_glm(b) * to_glm(a);

        ensure("composition order matches", equivalent(ll_result, gm_result));
    }

    template<> template<>
    void m4math_glm_equiv_object::test<7>()
    {
        // Translation composition adds offsets — verify both ways agree.
        LLMatrix4 a; a.setTranslation(1.0f, 2.0f, 3.0f);
        LLMatrix4 b; b.setTranslation(10.0f, 20.0f, 30.0f);

        LLMatrix4 ll_result = a;
        ll_result *= b;

        const glm::mat4 gm_expected = glm::translate(glm::mat4(1.0f), glm::vec3(11.0f, 22.0f, 33.0f));

        ensure("composed translation == 11,22,33", equivalent(ll_result, gm_expected));
    }

    // ---------- Transpose ----------

    template<> template<>
    void m4math_glm_equiv_object::test<8>()
    {
        // Transpose with non-trivial input. Note: a true element-wise
        // transpose flips i<->j, but because LL and GLM use opposite
        // storage orders, LL::transpose() in memory is GLM::transpose()
        // in memory (both swap row-major rows for cols).
        LLMatrix4 ll;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                ll.mMatrix[i][j] = static_cast<F32>(i * 4 + j);

        const glm::mat4 gm_before = to_glm(ll);
        ll.transpose();
        const glm::mat4 gm_after = glm::transpose(gm_before);

        ensure("transpose matches", equivalent(ll, gm_after));
    }

    // ---------- Inverse ----------

    template<> template<>
    void m4math_glm_equiv_object::test<9>()
    {
        // Inverse of identity == identity in both.
        LLMatrix4 ll;
        ll.setIdentity();
        ll.invert();

        const glm::mat4 gm = glm::inverse(glm::mat4(1.0f));

        ensure("inverse of identity matches", equivalent(ll, gm));
    }

    template<> template<>
    void m4math_glm_equiv_object::test<10>()
    {
        // Inverse of a rotation+translation matrix.
        LLMatrix4 ll;
        ll.initRotTrans(0.5f, LLVector3(0.0f, 0.0f, 1.0f), LLVector3(3.0f, 4.0f, 5.0f));

        const glm::mat4 gm_before = to_glm(ll);

        ll.invert();
        const glm::mat4 gm_after = glm::inverse(gm_before);

        // Inverse can lose a few bits of precision; allow looser epsilon.
        ensure("inverse of rot+trans matches", equivalent(ll, gm_after, 1e-4f));
    }

    // ---------- Determinant ----------

    template<> template<>
    void m4math_glm_equiv_object::test<11>()
    {
        LLMatrix4 ll;
        ll.setIdentity();
        const glm::mat4 gm = to_glm(ll);
        ensure_approximately_equals("det(identity)", ll.determinant(), glm::determinant(gm), 16);
    }

    template<> template<>
    void m4math_glm_equiv_object::test<12>()
    {
        LLMatrix4 ll;
        ll.setTranslation(7.0f, -3.0f, 11.0f);
        const glm::mat4 gm = to_glm(ll);
        ensure_approximately_equals("det(translation)", ll.determinant(), glm::determinant(gm), 16);
    }

    template<> template<>
    void m4math_glm_equiv_object::test<13>()
    {
        // Determinant of an arbitrary matrix.
        LLMatrix4 ll;
        ll.initRotTrans(0.7f, LLVector3(1.0f, 2.0f, 3.0f), LLVector3(4.0f, 5.0f, 6.0f));
        const glm::mat4 gm = to_glm(ll);
        // Rotation+translation matrices should have determinant 1.
        ensure_approximately_equals("det(rot+trans)", ll.determinant(), glm::determinant(gm), 16);
    }

    // ---------- Rotation ----------

    template<> template<>
    void m4math_glm_equiv_object::test<14>()
    {
        // 90 degrees around Z. LL initRotation takes radians + axis vector.
        LLMatrix4 ll;
        ll.initRotation(F_PI_BY_TWO, LLVector4(0.0f, 0.0f, 1.0f, 0.0f));

        const glm::mat4 gm = glm::rotate(glm::mat4(1.0f), F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));

        ensure("Z 90deg rotation matches", equivalent(ll, gm, 1e-5f));
    }

    template<> template<>
    void m4math_glm_equiv_object::test<15>()
    {
        // Arbitrary rotation around an axis.
        LLMatrix4 ll;
        const LLVector3 axis_ll(1.0f, 2.0f, 3.0f);
        // Normalize the axis (LL initRotation needs a normalized axis vector
        // as LLVector4).
        LLVector3 normalized_ll = axis_ll;
        normalized_ll.normalize();
        ll.initRotation(0.7f, LLVector4(normalized_ll.mV[0], normalized_ll.mV[1], normalized_ll.mV[2], 0.0f));

        const glm::vec3 axis_gm = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
        const glm::mat4 gm = glm::rotate(glm::mat4(1.0f), 0.7f, axis_gm);

        ensure("arbitrary rotation matches", equivalent(ll, gm, 1e-5f));
    }

    // ---------- Vector transform ----------

    template<> template<>
    void m4math_glm_equiv_object::test<16>()
    {
        // Transform a point through a rotation+translation. Both libs
        // should produce the same world-space point.
        LLMatrix4 ll;
        ll.initRotTrans(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f), LLVector3(10.0f, 20.0f, 30.0f));
        const glm::mat4 gm = to_glm(ll);

        const LLVector3 v_local(1.0f, 0.0f, 0.0f);
        const LLVector3 ll_world = v_local * ll;

        const glm::vec3 gm_world = glm::vec3(gm * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

        ensure("transformed point matches", vectors_equal(ll_world, gm_world, 1e-5f));
    }

    // ---------- Composition stress test ----------

    template<> template<>
    void m4math_glm_equiv_object::test<17>()
    {
        // Build a TRS chain (translate * rotate * scale-like) the LL way
        // and the GLM way, then compare. This is the kind of operation
        // that drives the rendering pipeline's modelview matrix.
        LLMatrix4 ll_t; ll_t.setTranslation(5.0f, 10.0f, 15.0f);
        LLMatrix4 ll_r; ll_r.initRotation(0.3f, LLVector4(0.0f, 1.0f, 0.0f, 0.0f));

        // LL convention: scale * rotate * translate (apply scale first, then rotate, then translate)
        // means in LL code: scaled_rotated = scale; scaled_rotated *= rotate; result = scaled_rotated; result *= translate
        LLMatrix4 ll_result = ll_r;
        ll_result *= ll_t;

        // GLM equivalent: translate * rotate (reversed multiplication order)
        const glm::mat4 gm_t = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 10.0f, 15.0f));
        const glm::mat4 gm_r = glm::rotate(glm::mat4(1.0f), 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 gm_result = gm_t * gm_r;

        ensure("TR composition matches", equivalent(ll_result, gm_result, 1e-5f));
    }

    template<> template<>
    void m4math_glm_equiv_object::test<18>()
    {
        // Chain of three transforms: rotate, translate, rotate.
        LLMatrix4 ll_r1; ll_r1.initRotation(0.4f, LLVector4(1.0f, 0.0f, 0.0f, 0.0f));
        LLMatrix4 ll_t;  ll_t.setTranslation(2.0f, 3.0f, 4.0f);
        LLMatrix4 ll_r2; ll_r2.initRotation(0.6f, LLVector4(0.0f, 0.0f, 1.0f, 0.0f));

        LLMatrix4 ll_result = ll_r1;
        ll_result *= ll_t;
        ll_result *= ll_r2;

        // GLM: same chain in reverse multiplication order.
        const glm::mat4 gm_r1 = glm::rotate(glm::mat4(1.0f), 0.4f, glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::mat4 gm_t  = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f));
        const glm::mat4 gm_r2 = glm::rotate(glm::mat4(1.0f), 0.6f, glm::vec3(0.0f, 0.0f, 1.0f));
        const glm::mat4 gm_result = gm_r2 * gm_t * gm_r1;

        ensure("R-T-R chain matches", equivalent(ll_result, gm_result, 1e-5f));
    }
}
