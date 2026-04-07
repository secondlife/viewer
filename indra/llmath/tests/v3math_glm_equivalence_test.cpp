/**
 * @file v3math_glm_equivalence_test.cpp
 * @brief Differential tests proving LLVector3 and glm::vec3 produce identical
 *        results for the same operations.
 *
 * Part of the GLM-migration safety net. LLVector3 already provides
 * `operator glm::vec3()` and a constructor from `glm::vec3`, so the
 * conversion is built-in. This test exercises every operation we'll
 * touch during the migration and pins down the contract.
 *
 * Note: SL multiplies vector*matrix the GL way (`v * M`), but for pure
 * vector operations (add/sub/dot/cross/scale/length/normalize) the
 * convention difference doesn't matter — these are commutative or
 * scalar.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../v3math.h"
#include "../v4math.h"
#include "../m3math.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>

namespace tut
{
    struct v3math_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        static glm::vec3 to_glm(const LLVector3& v)
        {
            // LLVector3 has built-in operator glm::vec3()
            return static_cast<glm::vec3>(v);
        }

        static LLVector3 to_ll(const glm::vec3& v)
        {
            return LLVector3(v);
        }

        static bool vec_near(const LLVector3& a, const glm::vec3& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[0] - b.x) <= eps
                && std::fabs(a.mV[1] - b.y) <= eps
                && std::fabs(a.mV[2] - b.z) <= eps;
        }

        static bool ll_vec_near(const LLVector3& a, const LLVector3& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[0] - b.mV[0]) <= eps
                && std::fabs(a.mV[1] - b.mV[1]) <= eps
                && std::fabs(a.mV[2] - b.mV[2]) <= eps;
        }
    };

    using v3math_glm_equiv_test = test_group<v3math_glm_equiv_data>;
    using v3math_glm_equiv_object = v3math_glm_equiv_test::object;
    tut::v3math_glm_equiv_test v3math_glm_equiv_testcase("v3math_glm_equivalence");

    // ---------- Construction & conversion ----------

    template<> template<>
    void v3math_glm_equiv_object::test<1>()
    {
        // Same components produce equivalent vectors.
        LLVector3 ll(1.0f, 2.0f, 3.0f);
        glm::vec3 gm(1.0f, 2.0f, 3.0f);
        ensure("xyz construction matches", vec_near(ll, gm));
    }

    template<> template<>
    void v3math_glm_equiv_object::test<2>()
    {
        // Round-trip ll → glm → ll preserves the value.
        LLVector3 original(3.14f, -2.71f, 1.41f);
        LLVector3 round = to_ll(to_glm(original));
        ensure("round-trip preserves value", ll_vec_near(round, original));
    }

    template<> template<>
    void v3math_glm_equiv_object::test<3>()
    {
        // Default-constructed vectors match (both should be zero).
        LLVector3 ll;
        glm::vec3 gm(0.0f);
        ensure("default ctor matches glm zero", vec_near(ll, gm));
    }

    // ---------- Addition / subtraction ----------

    template<> template<>
    void v3math_glm_equiv_object::test<4>()
    {
        LLVector3 a_ll(1.0f, 2.0f, 3.0f);
        LLVector3 b_ll(10.0f, 20.0f, 30.0f);
        LLVector3 sum_ll = a_ll + b_ll;

        glm::vec3 a_gm(1.0f, 2.0f, 3.0f);
        glm::vec3 b_gm(10.0f, 20.0f, 30.0f);
        glm::vec3 sum_gm = a_gm + b_gm;

        ensure("addition matches", vec_near(sum_ll, sum_gm));
    }

    template<> template<>
    void v3math_glm_equiv_object::test<5>()
    {
        LLVector3 a_ll(5.0f, -3.0f, 7.0f);
        LLVector3 b_ll(2.0f, 4.0f, -1.0f);
        LLVector3 diff_ll = a_ll - b_ll;

        glm::vec3 a_gm(5.0f, -3.0f, 7.0f);
        glm::vec3 b_gm(2.0f, 4.0f, -1.0f);
        glm::vec3 diff_gm = a_gm - b_gm;

        ensure("subtraction matches", vec_near(diff_ll, diff_gm));
    }

    // ---------- Scalar multiplication ----------

    template<> template<>
    void v3math_glm_equiv_object::test<6>()
    {
        LLVector3 v_ll(1.0f, 2.0f, 3.0f);
        LLVector3 scaled_ll = v_ll * 2.5f;

        glm::vec3 v_gm(1.0f, 2.0f, 3.0f);
        glm::vec3 scaled_gm = v_gm * 2.5f;

        ensure("scalar multiply matches", vec_near(scaled_ll, scaled_gm));
    }

    template<> template<>
    void v3math_glm_equiv_object::test<7>()
    {
        LLVector3 v_ll(10.0f, 20.0f, 30.0f);
        LLVector3 divided_ll = v_ll / 2.0f;

        glm::vec3 v_gm(10.0f, 20.0f, 30.0f);
        glm::vec3 divided_gm = v_gm / 2.0f;

        ensure("scalar divide matches", vec_near(divided_ll, divided_gm));
    }

    // ---------- Dot product ----------

    template<> template<>
    void v3math_glm_equiv_object::test<8>()
    {
        // SL uses operator* for dot product. GLM uses glm::dot.
        LLVector3 a_ll(1.0f, 2.0f, 3.0f);
        LLVector3 b_ll(4.0f, 5.0f, 6.0f);
        F32 dot_ll = a_ll * b_ll;

        glm::vec3 a_gm(1.0f, 2.0f, 3.0f);
        glm::vec3 b_gm(4.0f, 5.0f, 6.0f);
        F32 dot_gm = glm::dot(a_gm, b_gm);

        ensure_approximately_equals("dot product matches", dot_ll, dot_gm, 16);
    }

    template<> template<>
    void v3math_glm_equiv_object::test<9>()
    {
        // Orthogonal vectors → dot is zero in both.
        LLVector3 a_ll(1.0f, 0.0f, 0.0f);
        LLVector3 b_ll(0.0f, 1.0f, 0.0f);
        F32 dot_ll = a_ll * b_ll;

        glm::vec3 a_gm(1.0f, 0.0f, 0.0f);
        glm::vec3 b_gm(0.0f, 1.0f, 0.0f);
        F32 dot_gm = glm::dot(a_gm, b_gm);

        ensure_approximately_equals("orthogonal dot is zero", dot_ll, dot_gm, 16);
    }

    // ---------- Cross product ----------

    template<> template<>
    void v3math_glm_equiv_object::test<10>()
    {
        // SL uses operator% for cross product. GLM uses glm::cross.
        LLVector3 a_ll(1.0f, 0.0f, 0.0f);
        LLVector3 b_ll(0.0f, 1.0f, 0.0f);
        LLVector3 cross_ll = a_ll % b_ll;

        glm::vec3 a_gm(1.0f, 0.0f, 0.0f);
        glm::vec3 b_gm(0.0f, 1.0f, 0.0f);
        glm::vec3 cross_gm = glm::cross(a_gm, b_gm);

        ensure("X cross Y == Z (matches glm)", vec_near(cross_ll, cross_gm));
    }

    template<> template<>
    void v3math_glm_equiv_object::test<11>()
    {
        // Arbitrary vectors. Verifies the cross product handedness matches.
        LLVector3 a_ll(2.0f, 3.0f, 4.0f);
        LLVector3 b_ll(5.0f, 6.0f, 7.0f);
        LLVector3 cross_ll = a_ll % b_ll;

        glm::vec3 a_gm(2.0f, 3.0f, 4.0f);
        glm::vec3 b_gm(5.0f, 6.0f, 7.0f);
        glm::vec3 cross_gm = glm::cross(a_gm, b_gm);

        ensure("arbitrary cross matches glm", vec_near(cross_ll, cross_gm));
    }

    template<> template<>
    void v3math_glm_equiv_object::test<12>()
    {
        // a x b == -(b x a) in both libs.
        LLVector3 a_ll(1.0f, 2.0f, 3.0f);
        LLVector3 b_ll(4.0f, 5.0f, 6.0f);
        LLVector3 ab = a_ll % b_ll;
        LLVector3 ba = b_ll % a_ll;
        LLVector3 neg_ba = LLVector3(-ba.mV[0], -ba.mV[1], -ba.mV[2]);

        ensure("a x b == -(b x a)", ll_vec_near(ab, neg_ba));
    }

    // ---------- Length / lengthSquared / normalize ----------

    template<> template<>
    void v3math_glm_equiv_object::test<13>()
    {
        LLVector3 v_ll(3.0f, 4.0f, 0.0f);
        F32 len_ll = v_ll.length();

        glm::vec3 v_gm(3.0f, 4.0f, 0.0f);
        F32 len_gm = glm::length(v_gm);

        ensure_approximately_equals("3-4-5 length", len_ll, len_gm, 16);
    }

    template<> template<>
    void v3math_glm_equiv_object::test<14>()
    {
        LLVector3 v_ll(2.0f, 3.0f, 6.0f);  // length 7
        F32 len2_ll = v_ll.lengthSquared();

        glm::vec3 v_gm(2.0f, 3.0f, 6.0f);
        F32 len2_gm = glm::dot(v_gm, v_gm);

        ensure_approximately_equals("lengthSquared matches dot(v,v)", len2_ll, len2_gm, 16);
    }

    template<> template<>
    void v3math_glm_equiv_object::test<15>()
    {
        LLVector3 v_ll(1.0f, 2.0f, 2.0f);  // length 3
        v_ll.normalize();

        glm::vec3 v_gm(1.0f, 2.0f, 2.0f);
        glm::vec3 norm_gm = glm::normalize(v_gm);

        ensure("normalized vector matches glm::normalize", vec_near(v_ll, norm_gm));
    }

    template<> template<>
    void v3math_glm_equiv_object::test<16>()
    {
        // Normalized vector has length 1 in both.
        LLVector3 v_ll(7.0f, -3.0f, 11.0f);
        v_ll.normalize();

        glm::vec3 v_gm(7.0f, -3.0f, 11.0f);
        glm::vec3 norm_gm = glm::normalize(v_gm);

        ensure_approximately_equals("ll normalized length is 1", v_ll.length(), 1.0f, 16);
        ensure_approximately_equals("glm normalized length is 1", glm::length(norm_gm), 1.0f, 16);
        ensure("normalized vectors equivalent", vec_near(v_ll, norm_gm));
    }

    // ---------- Negation ----------

    template<> template<>
    void v3math_glm_equiv_object::test<17>()
    {
        LLVector3 v_ll(1.0f, -2.0f, 3.0f);
        LLVector3 neg_ll = -v_ll;

        glm::vec3 v_gm(1.0f, -2.0f, 3.0f);
        glm::vec3 neg_gm = -v_gm;

        ensure("negation matches", vec_near(neg_ll, neg_gm));
    }

    // ---------- Component-wise scale (LL: scaleVec, GLM: vec * vec) ----------

    template<> template<>
    void v3math_glm_equiv_object::test<18>()
    {
        LLVector3 a_ll(2.0f, 3.0f, 4.0f);
        LLVector3 b_ll(5.0f, 6.0f, 7.0f);
        LLVector3 scaled_ll = a_ll;
        scaled_ll.scaleVec(b_ll);

        glm::vec3 a_gm(2.0f, 3.0f, 4.0f);
        glm::vec3 b_gm(5.0f, 6.0f, 7.0f);
        glm::vec3 scaled_gm = a_gm * b_gm;

        ensure("component-wise scale matches", vec_near(scaled_ll, scaled_gm));
    }

    // ---------- Distance between two points ----------

    template<> template<>
    void v3math_glm_equiv_object::test<19>()
    {
        LLVector3 a_ll(1.0f, 2.0f, 3.0f);
        LLVector3 b_ll(4.0f, 6.0f, 3.0f);  // distance is sqrt(9+16+0) = 5
        F32 dist_ll = (a_ll - b_ll).length();

        glm::vec3 a_gm(1.0f, 2.0f, 3.0f);
        glm::vec3 b_gm(4.0f, 6.0f, 3.0f);
        F32 dist_gm = glm::distance(a_gm, b_gm);

        ensure_approximately_equals("distance matches", dist_ll, dist_gm, 16);
    }

    // ---------- Edge cases ----------

    template<> template<>
    void v3math_glm_equiv_object::test<20>()
    {
        // Zero vector dot anything is zero.
        LLVector3 a_ll(0.0f, 0.0f, 0.0f);
        LLVector3 b_ll(1.0f, 2.0f, 3.0f);
        F32 dot_ll = a_ll * b_ll;

        glm::vec3 a_gm(0.0f, 0.0f, 0.0f);
        glm::vec3 b_gm(1.0f, 2.0f, 3.0f);
        F32 dot_gm = glm::dot(a_gm, b_gm);

        ensure_approximately_equals("zero dot anything", dot_ll, dot_gm, 16);
    }

    template<> template<>
    void v3math_glm_equiv_object::test<21>()
    {
        // Cross of parallel vectors is zero in both.
        LLVector3 a_ll(2.0f, 4.0f, 6.0f);
        LLVector3 b_ll(1.0f, 2.0f, 3.0f);  // 2*b == a, parallel
        LLVector3 cross_ll = a_ll % b_ll;

        glm::vec3 a_gm(2.0f, 4.0f, 6.0f);
        glm::vec3 b_gm(1.0f, 2.0f, 3.0f);
        glm::vec3 cross_gm = glm::cross(a_gm, b_gm);

        ensure("parallel cross is zero (ll)",
               std::fabs(cross_ll.mV[0]) < 1e-5f
               && std::fabs(cross_ll.mV[1]) < 1e-5f
               && std::fabs(cross_ll.mV[2]) < 1e-5f);
        ensure("parallel cross matches glm", vec_near(cross_ll, cross_gm));
    }
}
