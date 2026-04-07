/**
 * @file v2math_glm_equivalence_test.cpp
 * @brief Regression tests for the glm::vec2 free function helpers in v2math.h.
 *
 * This file used to be a differential test that compared LLVector2 against
 * glm::vec2 across construction, arithmetic, dot, cross, length, normalize,
 * scaling, distance, and the helper functions (angle_between etc.). After
 * the LLVector2 → glm::vec2 migration completed and LLVector2 itself was
 * deleted, the differential side disappeared.
 *
 * What remains here:
 *   1. **glm::vec2 helper regression tests** — exercise the free functions
 *      in v2math.h (angle_between, signed_angle_between, are_parallel,
 *      dist_vec, dist_vec_squared, lerp) and pin their expected outputs.
 *   2. **lerpVec2 ↔ glm::mix equivalence tests** — historic regression net
 *      proving that the deleted LLSettingsBase::lerpVec2 helper was
 *      mathematically identical to glm::mix.
 *
 * The file name is kept for git history continuity. A future cleanup pass
 * could rename it to something like `v2math_helpers_test.cpp` if desired.
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

        static bool gm_near(const glm::vec2& a, const glm::vec2& b, F32 eps = kEps)
        {
            return std::fabs(a.x - b.x) <= eps
                && std::fabs(a.y - b.y) <= eps;
        }
    };

    using v2math_glm_equiv_test = test_group<v2math_glm_equiv_data>;
    using v2math_glm_equiv_object = v2math_glm_equiv_test::object;
    tut::v2math_glm_equiv_test v2math_glm_equiv_testcase("v2math_glm_equivalence");

    // ---------- v2math.h helper regression tests ----------

    template<> template<>
    void v2math_glm_equiv_object::test<1>()
    {
        // angle_between(perpendicular vectors) == PI/2
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, 1.0f);
        ensure_approximately_equals("perp angle is PI/2",
                                    angle_between(a, b), F_PI_BY_TWO, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<2>()
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
    void v2math_glm_equiv_object::test<3>()
    {
        // angle_between(antiparallel vectors) == PI
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(-1.0f, 0.0f);
        ensure_approximately_equals("antiparallel angle is PI",
                                    angle_between(a, b), F_PI, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<4>()
    {
        // signed_angle_between is positive when b is CCW from a
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, 1.0f);
        ensure_approximately_equals("CCW signed angle is +PI/2",
                                    signed_angle_between(a, b), F_PI_BY_TWO, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<5>()
    {
        // signed_angle_between is negative when b is CW from a
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, -1.0f);
        ensure_approximately_equals("CW signed angle is -PI/2",
                                    signed_angle_between(a, b), -F_PI_BY_TWO, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<6>()
    {
        // are_parallel detects parallel vectors
        const glm::vec2 a(2.0f, 4.0f);
        const glm::vec2 b(1.0f, 2.0f);
        ensure("parallel detected", are_parallel(a, b, 1e-5f));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<7>()
    {
        // are_parallel detects anti-parallel vectors (1 - |dot| ≈ 0 for both)
        const glm::vec2 a(2.0f, 4.0f);
        const glm::vec2 b(-1.0f, -2.0f);
        ensure("antiparallel detected as parallel", are_parallel(a, b, 1e-5f));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<8>()
    {
        // are_parallel rejects non-parallel
        const glm::vec2 a(1.0f, 0.0f);
        const glm::vec2 b(0.0f, 1.0f);
        ensure("perpendicular not parallel", !are_parallel(a, b, 1e-5f));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<9>()
    {
        // dist_vec computes Euclidean distance
        const glm::vec2 a(1.0f, 2.0f);
        const glm::vec2 b(4.0f, 6.0f);  // distance is 5
        ensure_approximately_equals("3-4-5 distance",
                                    dist_vec(a, b), 5.0f, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<10>()
    {
        // dist_vec_squared returns the squared distance
        const glm::vec2 a(1.0f, 2.0f);
        const glm::vec2 b(4.0f, 6.0f);  // distance² == 25
        ensure_approximately_equals("3-4-5 distance squared",
                                    dist_vec_squared(a, b), 25.0f, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<11>()
    {
        // dist_vec to self is zero
        const glm::vec2 a(7.0f, -3.0f);
        ensure_approximately_equals("self distance is zero",
                                    dist_vec(a, a), 0.0f, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<12>()
    {
        // lerp midpoint
        const glm::vec2 a(0.0f, 0.0f);
        const glm::vec2 b(10.0f, 20.0f);
        const glm::vec2 mid = lerp(a, b, 0.5f);
        ensure("midpoint matches", gm_near(mid, glm::vec2(5.0f, 10.0f)));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<13>()
    {
        // lerp at u=0 returns a, at u=1 returns b
        const glm::vec2 a(3.0f, 7.0f);
        const glm::vec2 b(11.0f, 13.0f);
        ensure("u=0 returns a", gm_near(lerp(a, b, 0.0f), a));
        ensure("u=1 returns b", gm_near(lerp(a, b, 1.0f), b));
    }

    // ---------- glm::mix equivalence (formerly LLSettingsBase::lerpVec2) ----------
    //
    // LLSettingsBase used to have a static `lerpVec2(glm::vec2& a, const glm::vec2& b, F32 mix)`
    // helper that did `a.x = lerp(a.x, b.x, mix); a.y = lerp(a.y, b.y, mix);`
    // per component. These tests originally proved equivalence with `glm::mix`
    // so we could safely replace the helper with `a = glm::mix(a, b, mix)` at
    // the three caller sites in llsettingssky.cpp / llsettingswater.cpp.
    // The helper has been deleted; the tests stay as a regression net.
    //
    // The `inline_lerpVec2` here is the OLD lerpVec2 body, kept inline so this
    // test file doesn't need to drag in llsettingsbase.h. If glm::mix ever
    // diverges from per-component scalar lerp these tests will fail loudly.

    static void inline_lerpVec2(glm::vec2& a, const glm::vec2& b, F32 mix)
    {
        // Mirrors the old LLSettingsBase::lerpVec2 body exactly.
        a.x = lerp(a.x, b.x, mix);
        a.y = lerp(a.y, b.y, mix);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<14>()
    {
        // mix=0: result should equal a (no movement toward b)
        glm::vec2 a_ll(3.0f, 7.0f);
        glm::vec2 b(10.0f, 20.0f);
        glm::vec2 a_gm = a_ll;
        inline_lerpVec2(a_ll, b, 0.0f);
        a_gm = glm::mix(a_gm, b, 0.0f);
        ensure("mix=0 lerpVec2 matches glm::mix", gm_near(a_ll, a_gm));
        ensure_approximately_equals("mix=0 keeps a.x", a_ll.x, 3.0f, 16);
        ensure_approximately_equals("mix=0 keeps a.y", a_ll.y, 7.0f, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<15>()
    {
        // mix=1: result should equal b (full movement to b)
        glm::vec2 a_ll(3.0f, 7.0f);
        glm::vec2 b(10.0f, 20.0f);
        glm::vec2 a_gm = a_ll;
        inline_lerpVec2(a_ll, b, 1.0f);
        a_gm = glm::mix(a_gm, b, 1.0f);
        ensure("mix=1 lerpVec2 matches glm::mix", gm_near(a_ll, a_gm));
        ensure_approximately_equals("mix=1 reaches b.x", a_ll.x, 10.0f, 16);
        ensure_approximately_equals("mix=1 reaches b.y", a_ll.y, 20.0f, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<16>()
    {
        // mix=0.5: result should be the midpoint
        glm::vec2 a_ll(2.0f, 4.0f);
        glm::vec2 b(10.0f, 20.0f);
        glm::vec2 a_gm = a_ll;
        inline_lerpVec2(a_ll, b, 0.5f);
        a_gm = glm::mix(a_gm, b, 0.5f);
        ensure("mix=0.5 lerpVec2 matches glm::mix", gm_near(a_ll, a_gm));
        ensure_approximately_equals("midpoint x", a_ll.x, 6.0f, 16);
        ensure_approximately_equals("midpoint y", a_ll.y, 12.0f, 16);
    }

    template<> template<>
    void v2math_glm_equiv_object::test<17>()
    {
        // arbitrary mix value, arbitrary inputs (the realistic settings case)
        glm::vec2 a_ll(0.123f, -4.56f);
        glm::vec2 b(7.89f, 0.001f);
        const F32 mix = 0.371f;
        glm::vec2 a_gm = a_ll;
        inline_lerpVec2(a_ll, b, mix);
        a_gm = glm::mix(a_gm, b, mix);
        ensure("arbitrary lerpVec2 matches glm::mix", gm_near(a_ll, a_gm));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<18>()
    {
        // Negative direction (b less than a)
        glm::vec2 a_ll(10.0f, 20.0f);
        glm::vec2 b(-5.0f, -10.0f);
        const F32 mix = 0.25f;
        glm::vec2 a_gm = a_ll;
        inline_lerpVec2(a_ll, b, mix);
        a_gm = glm::mix(a_gm, b, mix);
        ensure("negative direction matches", gm_near(a_ll, a_gm));
    }

    template<> template<>
    void v2math_glm_equiv_object::test<19>()
    {
        // Zero vectors on both sides
        glm::vec2 a_ll(0.0f, 0.0f);
        glm::vec2 b(0.0f, 0.0f);
        glm::vec2 a_gm = a_ll;
        inline_lerpVec2(a_ll, b, 0.5f);
        a_gm = glm::mix(a_gm, b, 0.5f);
        ensure("zero vectors match", gm_near(a_ll, a_gm));
        ensure_approximately_equals("zero stays zero", a_ll.x, 0.0f, 16);
        ensure_approximately_equals("zero stays zero", a_ll.y, 0.0f, 16);
    }
}
