/**
 * @file v2math_helpers_test.cpp
 * @brief Unit tests for the glm::vec2 free function helpers in v2math.h.
 *
 * The remaining helpers (`angle_between`, `signed_angle_between`,
 * `are_parallel`) are composite operations on glm::vec2 with no direct
 * one-call glm primitive — they wrap normalize + dot + cross-sign +
 * epsilon checks. These tests pin their behavior so that any future
 * cleanup of the helpers (e.g. tightening an epsilon, swapping a clamp)
 * surfaces immediately.
 *
 * History: this file used to test LLVector2 ↔ glm::vec2 differential
 * equivalence during the migration. After LLVector2 was deleted, the
 * file was repurposed to test the surviving free function helpers in
 * v2math.cpp. The thin wrappers `dist_vec`, `dist_vec_squared`, and
 * `lerp` were also removed in 2026-04 (every caller now uses the glm
 * primitive directly), and the LLSettingsBase::lerpVec2 historic
 * regression net was retired since the helper it protected has been
 * gone for months.
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
    struct v2math_helpers_data
    {
        static constexpr F32 kEps = 1e-5f;
    };

    using v2math_helpers_test_t = test_group<v2math_helpers_data>;
    using v2math_helpers_object = v2math_helpers_test_t::object;
    tut::v2math_helpers_test_t v2math_helpers_testcase("v2math_helpers");

    // ---------- angle_between ----------

    template<> template<>
    void v2math_helpers_object::test<1>()
    {
        // angle_between(perpendicular vectors) == PI/2
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, 1.0f);
        ensure_approximately_equals("perp angle is PI/2",
                                    angle_between(a, b), F_PI_BY_TWO, 16);
    }

    template<> template<>
    void v2math_helpers_object::test<2>()
    {
        // angle_between(parallel vectors) ≈ 0. The acos of a near-1 dot
        // is numerically unstable, so the result drifts a few mrad
        // (acos(1 - epsilon) ≈ sqrt(2*epsilon)). Use a coarser bound.
        const glm::vec2 a(2.0f, 4.0f);
        const glm::vec2 b(1.0f, 2.0f);
        const F32 ang = angle_between(a, b);
        ensure("parallel angle is approximately 0", std::fabs(ang) < 1e-3f);
    }

    template<> template<>
    void v2math_helpers_object::test<3>()
    {
        // angle_between(antiparallel vectors) == PI
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(-1.0f, 0.0f);
        ensure_approximately_equals("antiparallel angle is PI",
                                    angle_between(a, b), F_PI, 16);
    }

    // ---------- signed_angle_between ----------

    template<> template<>
    void v2math_helpers_object::test<4>()
    {
        // signed_angle_between is positive when b is CCW from a
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, 1.0f);
        ensure_approximately_equals("CCW signed angle is +PI/2",
                                    signed_angle_between(a, b), F_PI_BY_TWO, 16);
    }

    template<> template<>
    void v2math_helpers_object::test<5>()
    {
        // signed_angle_between is negative when b is CW from a
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, -1.0f);
        ensure_approximately_equals("CW signed angle is -PI/2",
                                    signed_angle_between(a, b), -F_PI_BY_TWO, 16);
    }

    // ---------- are_parallel ----------

    template<> template<>
    void v2math_helpers_object::test<6>()
    {
        // are_parallel detects parallel vectors
        const glm::vec2 a(2.0f, 4.0f);
        const glm::vec2 b(1.0f, 2.0f);
        ensure("parallel detected", are_parallel(a, b, 1e-5f));
    }

    template<> template<>
    void v2math_helpers_object::test<7>()
    {
        // are_parallel detects anti-parallel vectors (1 - |dot| ≈ 0 for both)
        const glm::vec2 a(2.0f, 4.0f);
        const glm::vec2 b(-1.0f, -2.0f);
        ensure("antiparallel detected as parallel", are_parallel(a, b, 1e-5f));
    }

    template<> template<>
    void v2math_helpers_object::test<8>()
    {
        // are_parallel rejects non-parallel
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, 1.0f);
        ensure("perpendicular not parallel", !are_parallel(a, b, 1e-5f));
    }
}
