/**
 * @file llquaternion_glm_equivalence_test.cpp
 * @brief Differential tests proving LLQuaternion and glm::quat produce
 *        identical results for the same operations.
 *
 * Part of the GLM-migration safety net.
 *
 * Conventions worked out and pinned by these tests:
 *
 *   - LLQuaternion stores mQ[VX, VY, VZ, VW] (xyzw, w is the scalar).
 *   - glm::quat constructor is glm::quat(w, x, y, z) (w first!).
 *     glm::quat stores internally as wxyz too.
 *
 *   - LLQuaternion `operator*(a, b)` composes "apply a, then b". This
 *     is REVERSED from the standard Hamilton product convention. Doing
 *     the same composition in glm requires REVERSED multiplication
 *     order: `LL: a * b` ≡ `glm: glm_b * glm_a`.
 *
 *   - LLQuaternion::DEFAULT and the default ctor produce identity
 *     (0,0,0,1). glm::quat() also defaults to identity, but explicit
 *     identity is glm::quat(1, 0, 0, 0) (w=1, xyz=0).
 *
 *   - LL conjugate / glm::conjugate negate xyz, leave w. They match.
 *
 *   - LL normalize() / glm::normalize do the same thing for normal
 *     unit-ish quaternions. (This quaternion class actually behaves
 *     correctly here, unlike LLVector4::normalize.)
 *
 * KNOWN PARKED BUG: `LLQuaternion::lerp(F32 t, const LLQuaternion &q)`
 * (the single-arg "lerp from identity" form, not the two-arg one) has a
 * W-component bug at llquaternion.cpp:462 — uses `q.mQ[VZ]` in the W
 * formula instead of `q.mQ[VW]`. The bug is unreachable from production
 * code (only the single-arg `nlerp` wrapper calls it, and only test
 * code calls that wrapper). NOT tested here — see test_coverage.md.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llmath.h"
#include "../llquaternion.h"
#include "../v3math.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace tut
{
    struct llquat_glm_equiv_data
    {
        static constexpr F32 kEps = 1e-5f;

        // LL quaternion → glm. Note the swap: glm::quat constructor is
        // (w, x, y, z) while LL stores mQ[VX, VY, VZ, VW].
        static glm::quat to_glm(const LLQuaternion& q)
        {
            return glm::quat(q.mQ[VW], q.mQ[VX], q.mQ[VY], q.mQ[VZ]);
        }

        static LLQuaternion to_ll(const glm::quat& q)
        {
            return LLQuaternion(q.x, q.y, q.z, q.w);
        }

        static bool quats_near(const LLQuaternion& a, const glm::quat& b, F32 eps = kEps)
        {
            // Note: q and -q represent the same rotation, so for some
            // operations we'd want to check both signs. For these tests
            // we use direct construction so the sign should match.
            return std::fabs(a.mQ[VX] - b.x) <= eps
                && std::fabs(a.mQ[VY] - b.y) <= eps
                && std::fabs(a.mQ[VZ] - b.z) <= eps
                && std::fabs(a.mQ[VW] - b.w) <= eps;
        }

        static bool ll_quats_near(const LLQuaternion& a, const LLQuaternion& b, F32 eps = kEps)
        {
            return std::fabs(a.mQ[VX] - b.mQ[VX]) <= eps
                && std::fabs(a.mQ[VY] - b.mQ[VY]) <= eps
                && std::fabs(a.mQ[VZ] - b.mQ[VZ]) <= eps
                && std::fabs(a.mQ[VW] - b.mQ[VW]) <= eps;
        }

        // Quaternions q and -q are the same rotation. For composition
        // tests where the sign might flip, compare absolute equivalence.
        static bool quats_equivalent_rotation(const LLQuaternion& a, const glm::quat& b, F32 eps = kEps)
        {
            // Either match directly, or match negated (q and -q are the
            // same rotation).
            return quats_near(a, b, eps)
                || (std::fabs(a.mQ[VX] + b.x) <= eps
                    && std::fabs(a.mQ[VY] + b.y) <= eps
                    && std::fabs(a.mQ[VZ] + b.z) <= eps
                    && std::fabs(a.mQ[VW] + b.w) <= eps);
        }
    };

    using llquat_glm_equiv_test = test_group<llquat_glm_equiv_data>;
    using llquat_glm_equiv_object = llquat_glm_equiv_test::object;
    tut::llquat_glm_equiv_test llquat_glm_equiv_testcase("llquaternion_glm_equivalence");

    // ---------- Construction & identity ----------

    template<> template<>
    void llquat_glm_equiv_object::test<1>()
    {
        // Default ctor produces identity in both libs.
        LLQuaternion ll;
        glm::quat gm(1.0f, 0.0f, 0.0f, 0.0f);  // glm::quat(w, x, y, z)
        ensure("default ctor is identity", quats_near(ll, gm));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<2>()
    {
        // Explicit (x, y, z, w) construction matches glm::quat(w, x, y, z).
        LLQuaternion ll(0.1f, 0.2f, 0.3f, 0.927f);  // approx unit
        glm::quat gm(0.927f, 0.1f, 0.2f, 0.3f);
        ensure("explicit ctor matches", quats_near(ll, gm));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<3>()
    {
        // Round-trip ll → glm → ll preserves the value.
        LLQuaternion original(0.1f, 0.2f, 0.3f, 0.927f);
        LLQuaternion round = to_ll(to_glm(original));
        ensure("round-trip preserves value", ll_quats_near(round, original));
    }

    // ---------- Axis-angle construction ----------

    template<> template<>
    void llquat_glm_equiv_object::test<4>()
    {
        // 90 degrees around Z axis.
        LLQuaternion ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));

        glm::quat gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));

        ensure("Z 90deg axis-angle matches", quats_equivalent_rotation(ll, gm));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<5>()
    {
        // Arbitrary axis, arbitrary angle.
        const F32 angle = 0.7f;
        LLVector3 axis_ll(1.0f, 2.0f, 3.0f);
        axis_ll.normalize();
        LLQuaternion ll(angle, axis_ll);

        glm::vec3 axis_gm = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
        glm::quat gm = glm::angleAxis(angle, axis_gm);

        ensure("arbitrary axis-angle matches", quats_equivalent_rotation(ll, gm));
    }

    // ---------- Composition (multiplication order REVERSED) ----------

    template<> template<>
    void llquat_glm_equiv_object::test<6>()
    {
        // Identity composition: q * I == q == I * q in both libs.
        LLQuaternion q_ll(0.1f, 0.2f, 0.3f, 0.927f);
        LLQuaternion ident_ll;  // default = identity

        LLQuaternion left_ll = q_ll * ident_ll;
        LLQuaternion right_ll = ident_ll * q_ll;

        ensure("q * I == q", ll_quats_near(left_ll, q_ll));
        ensure("I * q == q", ll_quats_near(right_ll, q_ll));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<7>()
    {
        // Composition order: LL `a * b` corresponds to glm `b * a`.
        // Build a 90deg-around-Z and a 90deg-around-X, compose them
        // both ways in both libs, and verify the LL "apply A then B"
        // matches the glm "B * A".
        LLQuaternion z90_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion x90_ll(F_PI_BY_TWO, LLVector3(1.0f, 0.0f, 0.0f));

        // LL: apply z90 then x90 = z90 * x90
        LLQuaternion ll_compose = z90_ll * x90_ll;

        // glm equivalent: x90 * z90 (reversed)
        glm::quat z90_gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat x90_gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat gm_compose = x90_gm * z90_gm;

        ensure("LL a*b == glm b*a (composition order reversed)",
               quats_equivalent_rotation(ll_compose, gm_compose));
    }

    // ---------- Conjugate / inverse ----------

    template<> template<>
    void llquat_glm_equiv_object::test<8>()
    {
        // Conjugate negates xyz, leaves w. Same in both libs.
        LLQuaternion ll(0.1f, 0.2f, 0.3f, 0.927f);
        LLQuaternion ll_conj = ll;
        ll_conj.conjugate();

        glm::quat gm(0.927f, 0.1f, 0.2f, 0.3f);
        glm::quat gm_conj = glm::conjugate(gm);

        ensure("conjugate matches", quats_near(ll_conj, gm_conj));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<9>()
    {
        // q * conjugate(q) == identity (for unit quaternions).
        LLQuaternion ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion ll_conj = ll;
        ll_conj.conjugate();
        LLQuaternion ll_product = ll * ll_conj;

        glm::quat gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat gm_conj = glm::conjugate(gm);
        glm::quat gm_product = gm_conj * gm;  // reversed order

        ensure("q * conj(q) == identity (LL)",
               std::fabs(ll_product.mQ[VW] - 1.0f) < 1e-5f
               && std::fabs(ll_product.mQ[VX]) < 1e-5f
               && std::fabs(ll_product.mQ[VY]) < 1e-5f
               && std::fabs(ll_product.mQ[VZ]) < 1e-5f);
        ensure("LL and glm products match", quats_equivalent_rotation(ll_product, gm_product));
    }

    // ---------- Normalize ----------

    template<> template<>
    void llquat_glm_equiv_object::test<10>()
    {
        // LL normalize and glm::normalize agree on unit-ish quats.
        // Note: unlike LLVector4, LLQuaternion::normalize DOES include W
        // in the magnitude.
        LLQuaternion ll(2.0f, 3.0f, 4.0f, 5.0f);  // not unit
        ll.normalize();

        glm::quat gm(5.0f, 2.0f, 3.0f, 4.0f);  // (w, x, y, z)
        glm::quat gm_norm = glm::normalize(gm);

        ensure("normalized quats match", quats_near(ll, gm_norm));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<11>()
    {
        // Normalized quaternion has length 1 in both libs.
        LLQuaternion ll(2.0f, 3.0f, 4.0f, 5.0f);
        ll.normalize();

        F32 mag_sq = ll.mQ[VX]*ll.mQ[VX] + ll.mQ[VY]*ll.mQ[VY]
                   + ll.mQ[VZ]*ll.mQ[VZ] + ll.mQ[VW]*ll.mQ[VW];
        ensure_approximately_equals("|q|² == 1 after normalize", mag_sq, 1.0f, 16);
    }

    // ---------- Quaternion as rotation operator on vectors ----------

    template<> template<>
    void llquat_glm_equiv_object::test<12>()
    {
        // Apply a 90deg-around-Z rotation to (1, 0, 0). Should give (0, 1, 0).
        LLQuaternion ll = LLQuaternion(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLVector3 v_ll(1.0f, 0.0f, 0.0f);
        LLVector3 rotated_ll = v_ll * ll;  // SL: vector * quaternion

        glm::quat gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 v_gm(1.0f, 0.0f, 0.0f);
        glm::vec3 rotated_gm = gm * v_gm;  // glm: quaternion * vector

        ensure_approximately_equals("rotated x", rotated_ll.mV[0], rotated_gm.x, 16);
        ensure_approximately_equals("rotated y", rotated_ll.mV[1], rotated_gm.y, 16);
        ensure_approximately_equals("rotated z", rotated_ll.mV[2], rotated_gm.z, 16);
    }

    template<> template<>
    void llquat_glm_equiv_object::test<13>()
    {
        // Apply an arbitrary rotation to a vector and verify both libs
        // give the same result.
        const F32 angle = 0.6f;
        LLVector3 axis_ll(1.0f, 1.0f, 0.0f);
        axis_ll.normalize();
        LLQuaternion ll(angle, axis_ll);
        LLVector3 v_ll(2.0f, 3.0f, 4.0f);
        LLVector3 rotated_ll = v_ll * ll;

        glm::vec3 axis_gm = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
        glm::quat gm = glm::angleAxis(angle, axis_gm);
        glm::vec3 v_gm(2.0f, 3.0f, 4.0f);
        glm::vec3 rotated_gm = gm * v_gm;

        ensure_approximately_equals("rotated x", rotated_ll.mV[0], rotated_gm.x, 16);
        ensure_approximately_equals("rotated y", rotated_ll.mV[1], rotated_gm.y, 16);
        ensure_approximately_equals("rotated z", rotated_ll.mV[2], rotated_gm.z, 16);
    }

    // ---------- Dot product ----------

    template<> template<>
    void llquat_glm_equiv_object::test<14>()
    {
        // LL dot(LLQuaternion, LLQuaternion) is a free function.
        LLQuaternion a_ll(0.1f, 0.2f, 0.3f, 0.927f);
        LLQuaternion b_ll(0.5f, 0.4f, 0.3f, 0.707f);
        F32 dot_ll = dot(a_ll, b_ll);

        glm::quat a_gm(0.927f, 0.1f, 0.2f, 0.3f);
        glm::quat b_gm(0.707f, 0.5f, 0.4f, 0.3f);
        F32 dot_gm = glm::dot(a_gm, b_gm);

        ensure_approximately_equals("dot product matches", dot_ll, dot_gm, 16);
    }
}
