/**
 * @file v3color_glm_equivalence_test.cpp
 * @brief Differential tests for LLColor3 ↔ glm::vec3 (used as RGB).
 *
 * Unlike LLVector4, LLColor3 has correctly behaving arithmetic
 * operators that touch all three components. This test confirms.
 *
 * GLM has no native color type — we use glm::vec3 as the canonical
 * RGB representation.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llmath.h"
#include "../v3color.h"

#include <glm/glm.hpp>

#include <cmath>

namespace tut
{
    struct v3color_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        static glm::vec3 to_glm(const LLColor3& c)
        {
            return glm::vec3(c.mV[0], c.mV[1], c.mV[2]);
        }

        static LLColor3 to_ll(const glm::vec3& c)
        {
            return LLColor3(c.x, c.y, c.z);
        }

        static bool color_near(const LLColor3& a, const glm::vec3& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[0] - b.x) <= eps
                && std::fabs(a.mV[1] - b.y) <= eps
                && std::fabs(a.mV[2] - b.z) <= eps;
        }
    };

    using v3color_glm_equiv_test = test_group<v3color_glm_equiv_data>;
    using v3color_glm_equiv_object = v3color_glm_equiv_test::object;
    tut::v3color_glm_equiv_test v3color_glm_equiv_testcase("v3color_glm_equivalence");

    template<> template<>
    void v3color_glm_equiv_object::test<1>()
    {
        LLColor3 ll(0.5f, 0.7f, 0.3f);
        glm::vec3 gm(0.5f, 0.7f, 0.3f);
        ensure("rgb construction matches", color_near(ll, gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<2>()
    {
        // Color addition (e.g., adding two light contributions).
        LLColor3 a_ll(0.2f, 0.3f, 0.4f);
        LLColor3 b_ll(0.5f, 0.6f, 0.7f);
        LLColor3 sum_ll = a_ll + b_ll;

        glm::vec3 a_gm(0.2f, 0.3f, 0.4f);
        glm::vec3 b_gm(0.5f, 0.6f, 0.7f);
        glm::vec3 sum_gm = a_gm + b_gm;

        ensure("color addition matches", color_near(sum_ll, sum_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<3>()
    {
        // Scalar multiply (e.g., dimming a light).
        LLColor3 ll(0.4f, 0.6f, 0.8f);
        LLColor3 dimmed_ll = ll * 0.5f;

        glm::vec3 gm(0.4f, 0.6f, 0.8f);
        glm::vec3 dimmed_gm = gm * 0.5f;

        ensure("scalar multiply (dim) matches", color_near(dimmed_ll, dimmed_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<4>()
    {
        // Component-wise multiply (e.g., light * material). LLColor3
        // uses operator*= for this.
        LLColor3 light_ll(1.0f, 0.8f, 0.6f);
        LLColor3 mat_ll(0.5f, 0.5f, 0.5f);
        LLColor3 result_ll = light_ll;
        result_ll *= mat_ll;

        glm::vec3 light_gm(1.0f, 0.8f, 0.6f);
        glm::vec3 mat_gm(0.5f, 0.5f, 0.5f);
        glm::vec3 result_gm = light_gm * mat_gm;

        ensure("component-wise color multiply matches", color_near(result_ll, result_gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<5>()
    {
        // Default constructor produces zero (black) in LL.
        LLColor3 ll;
        glm::vec3 gm(0.0f);
        ensure("default ctor is black", color_near(ll, gm));
    }

    template<> template<>
    void v3color_glm_equiv_object::test<6>()
    {
        // Round-trip through glm preserves all three components.
        LLColor3 original(0.123f, 0.456f, 0.789f);
        LLColor3 round = to_ll(to_glm(original));
        ensure("round-trip preserves rgb",
               std::fabs(round.mV[0] - original.mV[0]) < kEps
               && std::fabs(round.mV[1] - original.mV[1]) < kEps
               && std::fabs(round.mV[2] - original.mV[2]) < kEps);
    }

    template<> template<>
    void v3color_glm_equiv_object::test<7>()
    {
        // length() / "brightness" — sum of squared components.
        LLColor3 ll(0.6f, 0.8f, 0.0f);  // length 1.0
        F32 len_ll = ll.length();

        glm::vec3 gm(0.6f, 0.8f, 0.0f);
        F32 len_gm = glm::length(gm);

        ensure_approximately_equals("color length matches", len_ll, len_gm, 16);
    }
}
