/**
 * @file v2math_glm_equivalence_test.cpp
 * @brief Differential tests for LLVector2 ↔ glm::vec2.
 *
 * Smallest of the differential tests — LLVector2 has no exotic
 * conventions and the operators behave correctly. Mostly used for UI
 * coordinate math.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llmath.h"
#include "../v2math.h"

#include <glm/glm.hpp>

#include <cmath>

namespace tut
{
    struct v2math_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        static glm::vec2 to_glm(const LLVector2& v) { return glm::vec2(v.mV[0], v.mV[1]); }
        static LLVector2 to_ll(const glm::vec2& v)  { return LLVector2(v.x, v.y); }

        static bool vec_near(const LLVector2& a, const glm::vec2& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[0] - b.x) <= eps
                && std::fabs(a.mV[1] - b.y) <= eps;
        }
    };

    using v2math_glm_equiv_test = test_group<v2math_glm_equiv_data>;
    using v2math_glm_equiv_object = v2math_glm_equiv_test::object;
    tut::v2math_glm_equiv_test v2math_glm_equiv_testcase("v2math_glm_equivalence");

    template<> template<>
    void v2math_glm_equiv_object::test<1>()
    {
        LLVector2 ll(1.0f, 2.0f);
        glm::vec2 gm(1.0f, 2.0f);
        ensure("xy construction matches", vec_near(ll, gm));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<2>()
    {
        LLVector2 a_ll(1.0f, 2.0f);
        LLVector2 b_ll(10.0f, 20.0f);
        LLVector2 sum_ll = a_ll + b_ll;

        glm::vec2 a_gm(1.0f, 2.0f);
        glm::vec2 b_gm(10.0f, 20.0f);
        glm::vec2 sum_gm = a_gm + b_gm;

        ensure("addition matches", vec_near(sum_ll, sum_gm));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<3>()
    {
        LLVector2 a_ll(5.0f, -3.0f);
        LLVector2 b_ll(2.0f, 4.0f);
        LLVector2 diff_ll = a_ll - b_ll;

        glm::vec2 a_gm(5.0f, -3.0f);
        glm::vec2 b_gm(2.0f, 4.0f);
        glm::vec2 diff_gm = a_gm - b_gm;

        ensure("subtraction matches", vec_near(diff_ll, diff_gm));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<4>()
    {
        LLVector2 v_ll(3.0f, 4.0f);
        LLVector2 scaled_ll = v_ll * 2.5f;

        glm::vec2 v_gm(3.0f, 4.0f);
        glm::vec2 scaled_gm = v_gm * 2.5f;

        ensure("scalar multiply matches", vec_near(scaled_ll, scaled_gm));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<5>()
    {
        // SL uses operator* for dot product on vectors.
        LLVector2 a_ll(1.0f, 2.0f);
        LLVector2 b_ll(3.0f, 4.0f);
        F32 dot_ll = a_ll * b_ll;

        glm::vec2 a_gm(1.0f, 2.0f);
        glm::vec2 b_gm(3.0f, 4.0f);
        F32 dot_gm = glm::dot(a_gm, b_gm);

        // 1*3 + 2*4 = 11
        ensure_approximately_equals("dot matches", dot_ll, dot_gm, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<6>()
    {
        LLVector2 v_ll(3.0f, 4.0f);  // length 5
        F32 len_ll = v_ll.length();

        glm::vec2 v_gm(3.0f, 4.0f);
        F32 len_gm = glm::length(v_gm);

        ensure_approximately_equals("length matches", len_ll, len_gm, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<7>()
    {
        LLVector2 v_ll(3.0f, 4.0f);
        v_ll.normalize();

        glm::vec2 v_gm(3.0f, 4.0f);
        glm::vec2 norm_gm = glm::normalize(v_gm);

        ensure("normalize matches", vec_near(v_ll, norm_gm));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<8>()
    {
        // Distance between two points.
        LLVector2 a_ll(1.0f, 2.0f);
        LLVector2 b_ll(4.0f, 6.0f);  // distance is 5
        F32 dist_ll = (a_ll - b_ll).length();

        glm::vec2 a_gm(1.0f, 2.0f);
        glm::vec2 b_gm(4.0f, 6.0f);
        F32 dist_gm = glm::distance(a_gm, b_gm);

        ensure_approximately_equals("distance matches", dist_ll, dist_gm, 16);
    }
}
