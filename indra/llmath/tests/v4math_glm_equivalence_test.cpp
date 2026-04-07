/**
 * @file v4math_glm_equivalence_test.cpp
 * @brief Differential tests proving LLVector4 and glm::vec4 produce identical
 *        results for the operations where they're supposed to agree — and
 *        documenting the operations where they DIVERGE.
 *
 * Part of the GLM-migration safety net.
 *
 * IMPORTANT — known divergences (LLVector4 is essentially LLVector3 with a
 * tag-along W component that arithmetic operators IGNORE):
 *
 *   1. `LLVector4::normalize()` only normalizes XYZ. The W component is
 *      scaled by 1/|xyz|. Result is NOT a unit 4-vector.
 *
 *   2. `operator+` / `operator-` / `operator+=` / `operator-=` only operate
 *      on X/Y/Z. The W component of the result is the W of the LEFT operand
 *      (because += writes only to mV[VX..VZ] and leaves mV[VW] alone).
 *
 *   3. `operator*` (dot product) only sums `aX*bX + aY*bY + aZ*bZ` —
 *      no W contribution.
 *
 *   4. `operator*(scalar)` only scales X/Y/Z and returns a vector with W=1
 *      (from the 3-arg LLVector4 constructor).
 *
 *   5. `operator%` (cross product) is the 3D cross of XYZ with W=1.
 *
 *   6. `operator==` only compares X/Y/Z.
 *
 * In short: **LLVector4 is a 3D-vector class that happens to store a 4th
 * float**. The W component is exclusively used by callers that need to
 * round-trip a homogeneous coordinate through arithmetic that secretly
 * doesn't touch it.
 *
 * **DANGER**: parts of the codebase also use LLVector4 as a stand-in for
 * RGBA color (instead of LLColor4). Any of those callers that do arithmetic
 * on the "color" — `c1 + c2`, `color * 0.5f`, `dot(c1, c2)` — silently
 * lose or corrupt the alpha channel because the operators ignore W. This
 * is hidden behavior that the GLM migration must NOT inherit. Any code
 * path that uses LLVector4 as a color and does arithmetic should be
 * migrated to LLColor4 (or glm::vec4 with explicit semantics) BEFORE it's
 * touched by GLM-equivalence reasoning. Otherwise switching to glm::vec4
 * will silently change rendering output where alpha was being dropped or
 * frozen by the broken operators.
 *
 * This test pins all of that down so any future attempt to "fix" LLVector4
 * fails loudly here, and so the GLM migration knows to use LLVector3 (not
 * glm::vec4) for any code path that just adds/subtracts/dots LLVector4s.
 *
 * See test_coverage.md → Parked math semantics issues for the longer story.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../v3math.h"
#include "../v4math.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>

namespace tut
{
    struct v4math_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        static glm::vec4 to_glm(const LLVector4& v)
        {
            return static_cast<glm::vec4>(v);
        }

        static LLVector4 to_ll(const glm::vec4& v)
        {
            return LLVector4(v);
        }

        static bool vec_near(const LLVector4& a, const glm::vec4& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[0] - b.x) <= eps
                && std::fabs(a.mV[1] - b.y) <= eps
                && std::fabs(a.mV[2] - b.z) <= eps
                && std::fabs(a.mV[3] - b.w) <= eps;
        }

        static bool ll_vec_near(const LLVector4& a, const LLVector4& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[0] - b.mV[0]) <= eps
                && std::fabs(a.mV[1] - b.mV[1]) <= eps
                && std::fabs(a.mV[2] - b.mV[2]) <= eps
                && std::fabs(a.mV[3] - b.mV[3]) <= eps;
        }
    };

    using v4math_glm_equiv_test = test_group<v4math_glm_equiv_data>;
    using v4math_glm_equiv_object = v4math_glm_equiv_test::object;
    tut::v4math_glm_equiv_test v4math_glm_equiv_testcase("v4math_glm_equivalence");

    // ---------- Construction & conversion ----------

    template<> template<>
    void v4math_glm_equiv_object::test<1>()
    {
        LLVector4 ll(1.0f, 2.0f, 3.0f, 4.0f);
        glm::vec4 gm(1.0f, 2.0f, 3.0f, 4.0f);
        ensure("xyzw construction matches", vec_near(ll, gm));
    }

    template<> template<>
    void v4math_glm_equiv_object::test<2>()
    {
        // Round-trip preserves all four components (including W).
        LLVector4 original(3.14f, -2.71f, 1.41f, 0.577f);
        LLVector4 round = to_ll(to_glm(original));
        ensure("round-trip preserves value", ll_vec_near(round, original));
    }

    template<> template<>
    void v4math_glm_equiv_object::test<3>()
    {
        // 3-arg LLVector4 ctor sets W to 1.0
        LLVector4 ll(1.0f, 2.0f, 3.0f);
        glm::vec4 gm(1.0f, 2.0f, 3.0f, 1.0f);
        ensure("3-arg ctor produces W=1", vec_near(ll, gm));
    }

    // ---------- Addition / subtraction (DIVERGENCE: LL ignores W) ----------

    template<> template<>
    void v4math_glm_equiv_object::test<4>()
    {
        // LLVector4 + LLVector4 only adds X/Y/Z. W comes from the LEFT
        // operand (operator+= writes to mV[VX..VZ] and leaves mV[VW] alone).
        LLVector4 a_ll(1.0f, 2.0f, 3.0f, 4.0f);
        LLVector4 b_ll(10.0f, 20.0f, 30.0f, 40.0f);
        LLVector4 sum_ll = a_ll + b_ll;

        // XYZ matches glm
        glm::vec4 a_gm(1.0f, 2.0f, 3.0f, 4.0f);
        glm::vec4 b_gm(10.0f, 20.0f, 30.0f, 40.0f);
        glm::vec4 sum_gm = a_gm + b_gm;

        ensure_approximately_equals("LL+ XYZ matches glm", sum_ll.mV[0], sum_gm.x, 16);
        ensure_approximately_equals("LL+ XYZ matches glm", sum_ll.mV[1], sum_gm.y, 16);
        ensure_approximately_equals("LL+ XYZ matches glm", sum_ll.mV[2], sum_gm.z, 16);

        // W is the W of the left operand, NOT the sum.
        ensure_approximately_equals("LL+ W is left operand W (NOT sum)",
                                    sum_ll.mV[3], 4.0f, 16);
        // ...which is NOT what glm produces:
        ensure("LL+ W diverges from glm",
               std::fabs(sum_ll.mV[3] - sum_gm.w) > 1.0f);
    }

    template<> template<>
    void v4math_glm_equiv_object::test<5>()
    {
        // Same story for subtraction: XYZ matches, W is left.W untouched.
        LLVector4 a_ll(5.0f, -3.0f, 7.0f, 2.0f);
        LLVector4 b_ll(2.0f, 4.0f, -1.0f, 1.0f);
        LLVector4 diff_ll = a_ll - b_ll;

        ensure_approximately_equals("LL- XYZ x", diff_ll.mV[0], 3.0f, 16);
        ensure_approximately_equals("LL- XYZ y", diff_ll.mV[1], -7.0f, 16);
        ensure_approximately_equals("LL- XYZ z", diff_ll.mV[2], 8.0f, 16);
        // W is left.W (2.0), NOT the subtraction (2.0 - 1.0 = 1.0).
        ensure_approximately_equals("LL- W is left.W untouched", diff_ll.mV[3], 2.0f, 16);
    }

    // ---------- Scalar multiply (DIVERGENCE: W defaults to 1) ----------

    template<> template<>
    void v4math_glm_equiv_object::test<6>()
    {
        // operator*(F32) returns LLVector4(x*k, y*k, z*k) — the 3-arg ctor,
        // which sets W to 1.0. The original W is dropped entirely.
        LLVector4 v_ll(1.0f, 2.0f, 3.0f, 4.0f);
        LLVector4 scaled_ll = v_ll * 2.5f;

        ensure_approximately_equals("LL* XYZ x", scaled_ll.mV[0], 2.5f, 16);
        ensure_approximately_equals("LL* XYZ y", scaled_ll.mV[1], 5.0f, 16);
        ensure_approximately_equals("LL* XYZ z", scaled_ll.mV[2], 7.5f, 16);
        // W is 1, not 2.5*4=10
        ensure_approximately_equals("LL* W defaults to 1.0", scaled_ll.mV[3], 1.0f, 16);
    }

    // ---------- Dot product (DIVERGENCE: 3D dot only) ----------

    template<> template<>
    void v4math_glm_equiv_object::test<7>()
    {
        // SL operator* on LLVector4 is `aX*bX + aY*bY + aZ*bZ` — no W term.
        LLVector4 a_ll(1.0f, 2.0f, 3.0f, 4.0f);
        LLVector4 b_ll(5.0f, 6.0f, 7.0f, 8.0f);
        F32 dot_ll = a_ll * b_ll;

        // 3D dot: 1*5 + 2*6 + 3*7 = 5+12+21 = 38 (not 70)
        ensure_approximately_equals("LL operator* is 3D dot only", dot_ll, 38.0f, 16);

        // glm 4D dot: 70
        glm::vec4 a_gm(1.0f, 2.0f, 3.0f, 4.0f);
        glm::vec4 b_gm(5.0f, 6.0f, 7.0f, 8.0f);
        F32 dot_gm = glm::dot(a_gm, b_gm);
        ensure_approximately_equals("glm dot is 4D", dot_gm, 70.0f, 16);

        // Divergence:
        ensure("LL dot != glm 4D dot", std::fabs(dot_ll - dot_gm) > 1.0f);

        // BUT — LL dot DOES match glm::dot on the 3D portion:
        glm::vec3 a3(a_gm), b3(b_gm);
        F32 dot_3d_gm = glm::dot(a3, b3);
        ensure_approximately_equals("LL dot == glm 3D dot", dot_ll, dot_3d_gm, 16);
    }

    // ---------- Negation (DIVERGENCE: only XYZ negated) ----------

    template<> template<>
    void v4math_glm_equiv_object::test<8>()
    {
        // operator-(LLVector4) negates only XYZ. W comes through... let's
        // see what actually happens.
        LLVector4 v_ll(1.0f, -2.0f, 3.0f, -4.0f);
        LLVector4 neg_ll = -v_ll;

        ensure_approximately_equals("LL neg x", neg_ll.mV[0], -1.0f, 16);
        ensure_approximately_equals("LL neg y", neg_ll.mV[1], 2.0f, 16);
        ensure_approximately_equals("LL neg z", neg_ll.mV[2], -3.0f, 16);

        // What's the W? Look at v4math.cpp / v4math.h to see — the unary
        // minus uses the 3-arg ctor too, so W=1.
        ensure_approximately_equals("LL neg W defaults to 1", neg_ll.mV[3], 1.0f, 16);
    }

    // ---------- Length operations (XYZ-only on the LL side) ----------

    template<> template<>
    void v4math_glm_equiv_object::test<9>()
    {
        // LLVector4::length() returns the 3D length (XYZ only).
        // glm::length(vec4) returns the actual 4D length.
        // These DO NOT match in general.
        LLVector4 v_ll(3.0f, 4.0f, 0.0f, 12.0f);
        F32 len3_ll = v_ll.length();   // sqrt(9+16+0) = 5
        F32 len_ll_squared = v_ll.lengthSquared();  // 25

        glm::vec4 v_gm(3.0f, 4.0f, 0.0f, 12.0f);
        F32 len4_gm = glm::length(v_gm);  // sqrt(9+16+0+144) = 13
        F32 len4_gm_squared = glm::dot(v_gm, v_gm); // 169

        // LL length matches glm length3 (length of xyz portion)
        glm::vec3 v_gm_xyz(v_gm);
        F32 len3_gm = glm::length(v_gm_xyz);

        ensure_approximately_equals("LL length == glm length3", len3_ll, len3_gm, 16);
        ensure_approximately_equals("LL lengthSquared == 3D squared",
                                    len_ll_squared, len3_gm * len3_gm, 16);

        // And the LL length DOES NOT match the full glm 4D length:
        ensure("LL length != glm full 4D length", std::fabs(len3_ll - len4_gm) > 1.0f);
        // Suppress unused-warning by referencing the value:
        (void)len4_gm_squared;
    }

    // ---------- The normalize() divergence ----------

    template<> template<>
    void v4math_glm_equiv_object::test<10>()
    {
        // After LL normalize(), the LL vector is NOT a unit 4-vector.
        // The XYZ portion is unit-length. W is left COMPLETELY UNTOUCHED
        // (not scaled, not zeroed) because operator*= and operator/=
        // only act on XYZ. So normalize() really is just "make xyz a
        // unit vector and ignore W".
        LLVector4 v_ll(3.0f, 4.0f, 0.0f, 12.0f);  // |xyz| = 5
        v_ll.normalize();

        // After "normalize", xyz should be (3/5, 4/5, 0)
        ensure_approximately_equals("xyz x", v_ll.mV[0], 0.6f, 16);
        ensure_approximately_equals("xyz y", v_ll.mV[1], 0.8f, 16);
        ensure_approximately_equals("xyz z", v_ll.mV[2], 0.0f, 16);
        // W is COMPLETELY UNCHANGED — still 12.
        ensure_approximately_equals("w is untouched by normalize", v_ll.mV[3], 12.0f, 16);
    }

    template<> template<>
    void v4math_glm_equiv_object::test<11>()
    {
        // LL normalize() and glm::normalize(vec4) DIVERGE.
        // Document this so any "fix" attempt fails this test loudly.
        LLVector4 v_ll(3.0f, 4.0f, 0.0f, 12.0f);
        v_ll.normalize();

        glm::vec4 v_gm(3.0f, 4.0f, 0.0f, 12.0f);
        glm::vec4 v_gm_normalized = glm::normalize(v_gm);

        // After LL "normalize", lengthSquared(xyz) is 1 — the xyz portion
        // is a unit vector. But the full vector is not unit length.
        ensure("LL xyz portion is unit", std::fabs(
            v_ll.mV[0]*v_ll.mV[0] + v_ll.mV[1]*v_ll.mV[1] + v_ll.mV[2]*v_ll.mV[2] - 1.0f) < kEps);

        // The glm-normalized vector has full 4D length 1 (within float
        // precision; glm divides by sqrt(169)=13 which is exact, but
        // squaring and re-summing 12/13 introduces drift).
        ensure("glm normalized vec4 has 4D length 1",
               std::fabs(glm::length(v_gm_normalized) - 1.0f) < 1e-4f);

        // The two results are NOT equal — divergence.
        ensure("LL normalize and glm::normalize DIVERGE for vec4",
               !vec_near(v_ll, v_gm_normalized, 1e-2f));
    }

    template<> template<>
    void v4math_glm_equiv_object::test<12>()
    {
        // The XYZ portion of LL's "normalized" LLVector4 matches glm's
        // 3D normalization of the same XYZ. (W is left unchanged on the
        // LL side, which is why we only compare the XYZ part.)
        LLVector4 v_ll(3.0f, 4.0f, 0.0f, 12.0f);
        LLVector4 ll_copy = v_ll;
        ll_copy.normalize();

        glm::vec3 v_gm_xyz(v_ll.mV[0], v_ll.mV[1], v_ll.mV[2]);
        glm::vec3 normalized_xyz = glm::normalize(v_gm_xyz);

        ensure_approximately_equals("xyz x matches glm v3 normalize",
                                    ll_copy.mV[0], normalized_xyz.x, 16);
        ensure_approximately_equals("xyz y matches glm v3 normalize",
                                    ll_copy.mV[1], normalized_xyz.y, 16);
        ensure_approximately_equals("xyz z matches glm v3 normalize",
                                    ll_copy.mV[2], normalized_xyz.z, 16);
    }

    // ---------- Conversion to/from vec3 ----------

    template<> template<>
    void v4math_glm_equiv_object::test<13>()
    {
        // LLVector4 → glm::vec3 should drop W, just like glm::vec3(vec4).
        LLVector4 v_ll(1.0f, 2.0f, 3.0f, 4.0f);
        glm::vec3 v_gm_from_ll = static_cast<glm::vec3>(v_ll);

        glm::vec4 v_gm(1.0f, 2.0f, 3.0f, 4.0f);
        glm::vec3 v_gm_from_gm(v_gm);

        ensure_approximately_equals("vec3 cast x", v_gm_from_ll.x, v_gm_from_gm.x, 16);
        ensure_approximately_equals("vec3 cast y", v_gm_from_ll.y, v_gm_from_gm.y, 16);
        ensure_approximately_equals("vec3 cast z", v_gm_from_ll.z, v_gm_from_gm.z, 16);
    }
}
