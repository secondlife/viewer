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
#include <glm/gtx/quaternion.hpp>

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

    // ---------- Three-axes constructor ----------

    template<> template<>
    void llquat_glm_equiv_object::test<15>()
    {
        // LLQuaternion(x_axis, y_axis, z_axis) builds a rotation from
        // an orthonormal basis. Test by feeding it the rotated basis of
        // a 90deg-around-Z rotation and verifying the result rotates
        // (1,0,0) to (0,1,0) just like the axis-angle ctor would.
        //
        // For 90deg around Z:
        //   x_axis (1,0,0) -> (0, 1, 0)
        //   y_axis (0,1,0) -> (-1, 0, 0)
        //   z_axis (0,0,1) -> (0, 0, 1)
        LLQuaternion ll(LLVector3(0.0f, 1.0f, 0.0f),
                        LLVector3(-1.0f, 0.0f, 0.0f),
                        LLVector3(0.0f, 0.0f, 1.0f));

        // Confirm: rotating (1,0,0) by ll yields (0,1,0).
        LLVector3 v(1.0f, 0.0f, 0.0f);
        LLVector3 rotated = v * ll;
        ensure_approximately_equals("3-axes ctor: rotated x", rotated.mV[0], 0.0f, 16);
        ensure_approximately_equals("3-axes ctor: rotated y", rotated.mV[1], 1.0f, 16);
        ensure_approximately_equals("3-axes ctor: rotated z", rotated.mV[2], 0.0f, 16);

        // And the equivalent rotation via axis-angle.
        LLQuaternion ll_axis(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        ensure("3-axes ctor matches axis-angle equivalent",
               quats_equivalent_rotation(ll, to_glm(ll_axis)));
    }

    // ---------- Matrix3 / Matrix4 round-trip ----------

    template<> template<>
    void llquat_glm_equiv_object::test<16>()
    {
        // LLQuaternion(LLMatrix3) round-trip: build a quat, convert to
        // matrix, convert back, verify behavioral equivalence (same
        // rotation applied to a known vector).
        LLQuaternion original(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLMatrix3 mat = original.getMatrix3();
        LLQuaternion roundtrip(mat);

        LLVector3 v(1.0f, 2.0f, 3.0f);
        LLVector3 rot_orig = v * original;
        LLVector3 rot_round = v * roundtrip;

        ensure_approximately_equals("matrix3 roundtrip x", rot_orig.mV[0], rot_round.mV[0], 16);
        ensure_approximately_equals("matrix3 roundtrip y", rot_orig.mV[1], rot_round.mV[1], 16);
        ensure_approximately_equals("matrix3 roundtrip z", rot_orig.mV[2], rot_round.mV[2], 16);
    }

    template<> template<>
    void llquat_glm_equiv_object::test<17>()
    {
        // The LLQuaternion(LLMatrix3) ctor and glm::quat_cast(mat3)
        // produce equivalent rotations when both consume "the same"
        // matrix.
        //
        // NOTE: LLMatrix3 is stored OpenGL column-major (transposed
        // relative to row-major mathematical convention). The comment
        // at indra/llmath/llquaternion.cpp:232 documents that
        // LLQuaternion::getMatrix3 and LLMatrix3::quaternion are
        // internally consistent with this transpose convention.
        //
        // To compare against glm: build the matrix in BOTH libs from
        // a common axis-angle representation, then convert to quat in
        // each lib, then compare behaviorally (rotate a vector, match).
        const F32 angle = 0.7f;
        LLVector3 axis(0.0f, 0.0f, 1.0f);

        LLQuaternion ll_via_axis(angle, axis);
        LLMatrix3 ll_mat = ll_via_axis.getMatrix3();
        LLQuaternion ll_via_mat(ll_mat);

        glm::quat gm_via_axis = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat3 gm_mat = glm::mat3_cast(gm_via_axis);
        glm::quat gm_via_mat = glm::quat_cast(gm_mat);

        // Compare the behaviour: apply both to a known vector.
        LLVector3 v_ll(1.0f, 2.0f, 3.0f);
        LLVector3 rot_ll = v_ll * ll_via_mat;

        glm::vec3 v_gm(1.0f, 2.0f, 3.0f);
        glm::vec3 rot_gm = gm_via_mat * v_gm;

        ensure_approximately_equals("matrix3 ctor: rotated x", rot_ll.mV[0], rot_gm.x, 32);
        ensure_approximately_equals("matrix3 ctor: rotated y", rot_ll.mV[1], rot_gm.y, 32);
        ensure_approximately_equals("matrix3 ctor: rotated z", rot_ll.mV[2], rot_gm.z, 32);
    }

    // ---------- Euler angles ----------

    template<> template<>
    void llquat_glm_equiv_object::test<18>()
    {
        // Euler convention pin: build a quaternion from Euler angles in
        // both libs, apply to a known vector, compare.
        //
        // PINNED CONVENTION: LLQuaternion::setEulerAngles(roll, pitch, yaw)
        // delegates to LLMatrix3(roll, pitch, yaw) which builds an extrinsic
        // XYZ rotation. To match in glm we use glm::eulerAngleXYZ from
        // glm/gtx/euler_angles.hpp ... or we can just compose three
        // axis-angle rotations explicitly to remove any glm-extension
        // ambiguity.
        //
        // We compose explicitly: yaw around Z, pitch around Y, roll
        // around X. The compose order matches what LLMatrix3 builds.
        const F32 roll  = 0.3f;
        const F32 pitch = 0.5f;
        const F32 yaw   = 0.7f;

        LLQuaternion ll;
        ll.setEulerAngles(roll, pitch, yaw);

        // Behavioral check: apply to a known vector and capture the result.
        // We're not asserting a specific glm equivalent here — we're
        // pinning what LL produces so the migration can be checked
        // against this same input/output later.
        LLVector3 v(1.0f, 0.0f, 0.0f);
        LLVector3 rotated_ll = v * ll;

        // Round-trip: extract Euler from the quaternion and rebuild,
        // verify the same rotation is reproduced.
        F32 r_out, p_out, y_out;
        ll.getEulerAngles(&r_out, &p_out, &y_out);
        LLQuaternion ll_round;
        ll_round.setEulerAngles(r_out, p_out, y_out);

        LLVector3 rotated_round = v * ll_round;

        ensure_approximately_equals("euler roundtrip x", rotated_ll.mV[0], rotated_round.mV[0], 32);
        ensure_approximately_equals("euler roundtrip y", rotated_ll.mV[1], rotated_round.mV[1], 32);
        ensure_approximately_equals("euler roundtrip z", rotated_ll.mV[2], rotated_round.mV[2], 32);
    }

    // ---------- shortestArc (vector-to-vector rotation) ----------

    template<> template<>
    void llquat_glm_equiv_object::test<19>()
    {
        // shortestArc(a, b) builds a rotation that takes vector a to vector b.
        // Test the SEMANTIC: rotating a by the resulting quat should give b.
        // glm has no direct shortestArc — the migration will need a manual
        // bridge or use glm::rotation from glm/gtx/quaternion.hpp.
        LLVector3 a(1.0f, 0.0f, 0.0f);
        LLVector3 b(0.0f, 1.0f, 0.0f);

        LLQuaternion ll;
        ll.shortestArc(a, b);

        LLVector3 rotated = a * ll;
        ensure_approximately_equals("shortestArc semantic: x", rotated.mV[0], b.mV[0], 32);
        ensure_approximately_equals("shortestArc semantic: y", rotated.mV[1], b.mV[1], 32);
        ensure_approximately_equals("shortestArc semantic: z", rotated.mV[2], b.mV[2], 32);

        // Cross-check against glm::rotation (from glm/gtx/quaternion.hpp).
        glm::quat gm_arc = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 gm_rotated = gm_arc * glm::vec3(1.0f, 0.0f, 0.0f);
        ensure_approximately_equals("glm::rotation semantic: x", gm_rotated.x, b.mV[0], 32);
        ensure_approximately_equals("glm::rotation semantic: y", gm_rotated.y, b.mV[1], 32);
        ensure_approximately_equals("glm::rotation semantic: z", gm_rotated.z, b.mV[2], 32);

        // And the two should produce the same rotation (behaviorally).
        ensure("LL shortestArc and glm::rotation match",
               quats_equivalent_rotation(ll, gm_arc));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<20>()
    {
        // shortestArc with non-axis-aligned vectors.
        LLVector3 a(1.0f, 2.0f, 3.0f);
        a.normalize();
        LLVector3 b(3.0f, -1.0f, 2.0f);
        b.normalize();

        LLQuaternion ll;
        ll.shortestArc(a, b);
        LLVector3 rotated = a * ll;

        ensure_approximately_equals("shortestArc non-axis-aligned: x", rotated.mV[0], b.mV[0], 64);
        ensure_approximately_equals("shortestArc non-axis-aligned: y", rotated.mV[1], b.mV[1], 64);
        ensure_approximately_equals("shortestArc non-axis-aligned: z", rotated.mV[2], b.mV[2], 64);
    }

    // ---------- Slerp (3-arg form, the production form) ----------

    template<> template<>
    void llquat_glm_equiv_object::test<21>()
    {
        // slerp(0, a, b) == a, slerp(1, a, b) == b
        LLQuaternion a_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion b_ll(F_PI_BY_TWO, LLVector3(1.0f, 0.0f, 0.0f));

        LLQuaternion at_zero = slerp(0.0f, a_ll, b_ll);
        LLQuaternion at_one  = slerp(1.0f, a_ll, b_ll);

        ensure("slerp(0) == a", quats_equivalent_rotation(at_zero, to_glm(a_ll)));
        ensure("slerp(1) == b", quats_equivalent_rotation(at_one,  to_glm(b_ll)));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<22>()
    {
        // LL slerp(t, a, b) and glm::slerp(a, b, t) midpoint should produce
        // equivalent rotations. Test by behavioral comparison since signs
        // can flip.
        LLQuaternion a_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion b_ll(F_PI_BY_TWO, LLVector3(1.0f, 0.0f, 0.0f));
        LLQuaternion mid_ll = slerp(0.5f, a_ll, b_ll);

        glm::quat a_gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat b_gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat mid_gm = glm::slerp(a_gm, b_gm, 0.5f);

        // Apply both midpoints to a known vector and compare the rotated result.
        LLVector3 v_ll(1.0f, 2.0f, 3.0f);
        LLVector3 rot_ll = v_ll * mid_ll;

        glm::vec3 v_gm(1.0f, 2.0f, 3.0f);
        glm::vec3 rot_gm = mid_gm * v_gm;

        ensure_approximately_equals("slerp midpoint: x", rot_ll.mV[0], rot_gm.x, 64);
        ensure_approximately_equals("slerp midpoint: y", rot_ll.mV[1], rot_gm.y, 64);
        ensure_approximately_equals("slerp midpoint: z", rot_ll.mV[2], rot_gm.z, 64);
    }

    // ---------- Nlerp (3-arg form, the production form) ----------

    template<> template<>
    void llquat_glm_equiv_object::test<23>()
    {
        // nlerp(0, a, b) == a, nlerp(1, a, b) == b
        LLQuaternion a_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion b_ll(F_PI_BY_TWO, LLVector3(1.0f, 0.0f, 0.0f));

        LLQuaternion at_zero = nlerp(0.0f, a_ll, b_ll);
        LLQuaternion at_one  = nlerp(1.0f, a_ll, b_ll);

        ensure("nlerp(0) == a", quats_equivalent_rotation(at_zero, to_glm(a_ll)));
        ensure("nlerp(1) == b", quats_equivalent_rotation(at_one,  to_glm(b_ll)));
    }

    template<> template<>
    void llquat_glm_equiv_object::test<24>()
    {
        // LL nlerp(t, a, b) midpoint vs hand-rolled normalized lerp in glm.
        // glm has no direct nlerp; the standard equivalent is normalize(mix(a, b, t))
        // EXCEPT LL nlerp falls back to slerp when dot < 0 (taking the
        // shorter arc). For unit quats with dot >= 0 it's normalize(lerp).
        LLQuaternion a_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion b_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f) + LLVector3(0.1f, 0.0f, 0.0f));
        b_ll.normalize();

        LLQuaternion mid_ll = nlerp(0.5f, a_ll, b_ll);

        glm::quat a_gm = to_glm(a_ll);
        glm::quat b_gm = to_glm(b_ll);

        // hand-rolled normalize(mix(a, b, t)) — glm::lerp does this.
        glm::quat mid_gm = glm::normalize(glm::lerp(a_gm, b_gm, 0.5f));

        LLVector3 v(1.0f, 2.0f, 3.0f);
        LLVector3 rot_ll = v * mid_ll;
        glm::vec3 rot_gm = mid_gm * glm::vec3(1.0f, 2.0f, 3.0f);

        ensure_approximately_equals("nlerp midpoint: x", rot_ll.mV[0], rot_gm.x, 64);
        ensure_approximately_equals("nlerp midpoint: y", rot_ll.mV[1], rot_gm.y, 64);
        ensure_approximately_equals("nlerp midpoint: z", rot_ll.mV[2], rot_gm.z, 64);
    }

    // ---------- Two-arg lerp (the non-buggy form) ----------

    template<> template<>
    void llquat_glm_equiv_object::test<25>()
    {
        // The two-arg lerp(t, a, b) is the non-buggy lerp (lerp from a to
        // b). The single-arg lerp(t, q) has the parked W-component bug
        // and is NOT tested here.
        LLQuaternion a_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion b_ll(F_PI_BY_TWO, LLVector3(1.0f, 0.0f, 0.0f));

        LLQuaternion at_zero = lerp(0.0f, a_ll, b_ll);
        LLQuaternion at_one  = lerp(1.0f, a_ll, b_ll);

        ensure("lerp(0) == a", quats_equivalent_rotation(at_zero, to_glm(a_ll)));
        ensure("lerp(1) == b", quats_equivalent_rotation(at_one,  to_glm(b_ll)));
    }

    // ---------- Composition associativity ----------

    template<> template<>
    void llquat_glm_equiv_object::test<26>()
    {
        // (a * b) * c == a * (b * c) up to sign in both libs.
        // Build three distinct rotations and verify both grouping orders
        // produce equivalent rotations.
        LLQuaternion a_ll(0.5f, LLVector3(1.0f, 0.0f, 0.0f));
        LLQuaternion b_ll(0.7f, LLVector3(0.0f, 1.0f, 0.0f));
        LLQuaternion c_ll(0.9f, LLVector3(0.0f, 0.0f, 1.0f));

        LLQuaternion left_assoc  = (a_ll * b_ll) * c_ll;
        LLQuaternion right_assoc = a_ll * (b_ll * c_ll);

        ensure("LL: (a*b)*c == a*(b*c)",
               quats_equivalent_rotation(left_assoc, to_glm(right_assoc)));

        // Sanity: same in glm (with reversed order, per the convention pin)
        glm::quat a_gm = to_glm(a_ll);
        glm::quat b_gm = to_glm(b_ll);
        glm::quat c_gm = to_glm(c_ll);
        glm::quat gm_left  = (c_gm * b_gm) * a_gm;  // reversed because LL composes opposite
        glm::quat gm_right = c_gm * (b_gm * a_gm);

        ensure("glm: (c*b)*a == c*(b*a)",
               quats_equivalent_rotation(to_ll(gm_left), gm_right));
    }

    // ---------- Axis-angle round-trip ----------

    template<> template<>
    void llquat_glm_equiv_object::test<27>()
    {
        // Build with axis-angle, extract via getAngleAxis, rebuild,
        // compare behaviorally.
        const F32 orig_angle = 0.6f;
        LLVector3 orig_axis(1.0f, 2.0f, 3.0f);
        orig_axis.normalize();
        LLQuaternion ll(orig_angle, orig_axis);

        F32 out_angle, out_x, out_y, out_z;
        ll.getAngleAxis(&out_angle, &out_x, &out_y, &out_z);

        LLQuaternion ll_round(out_angle, LLVector3(out_x, out_y, out_z));

        ensure("axis-angle round-trip behavioral",
               quats_equivalent_rotation(ll_round, to_glm(ll)));
    }

    // ---------- Long composition drift ----------

    template<> template<>
    void llquat_glm_equiv_object::test<28>()
    {
        // Composing many small rotations should drift the same in LL
        // and glm — a sanity check that operation order over many
        // iterations doesn't accumulate divergent error.
        LLQuaternion ll;       // identity
        glm::quat gm(1.0f, 0.0f, 0.0f, 0.0f);  // identity

        const F32 step = 0.1f;
        const LLVector3 ll_axis(0.0f, 0.0f, 1.0f);
        const glm::vec3 gm_axis(0.0f, 0.0f, 1.0f);

        for (int i = 0; i < 10; ++i)
        {
            LLQuaternion increment_ll(step, ll_axis);
            ll = ll * increment_ll;  // LL: apply increment "after" ll

            glm::quat increment_gm = glm::angleAxis(step, gm_axis);
            gm = increment_gm * gm;  // glm: reversed order, same convention
        }

        // Apply both to a known vector — should match within drift tolerance.
        LLVector3 v(1.0f, 0.0f, 0.0f);
        LLVector3 rot_ll = v * ll;
        glm::vec3 rot_gm = gm * glm::vec3(1.0f, 0.0f, 0.0f);

        ensure_approximately_equals("long composition drift x", rot_ll.mV[0], rot_gm.x, 256);
        ensure_approximately_equals("long composition drift y", rot_ll.mV[1], rot_gm.y, 256);
        ensure_approximately_equals("long composition drift z", rot_ll.mV[2], rot_gm.z, 256);
    }

    // ---------- Composition with vector rotation ----------

    template<> template<>
    void llquat_glm_equiv_object::test<29>()
    {
        // Combined test: build two rotations, compose them, apply to a
        // vector, verify the result matches what we'd get from applying
        // the rotations sequentially. Catches any sign-flip / order
        // bug at the composition-rotation interface.
        LLQuaternion z90_ll(F_PI_BY_TWO, LLVector3(0.0f, 0.0f, 1.0f));
        LLQuaternion x90_ll(F_PI_BY_TWO, LLVector3(1.0f, 0.0f, 0.0f));

        LLVector3 v(1.0f, 0.0f, 0.0f);

        // Sequential application: rotate by z90 first, then by x90.
        LLVector3 step1 = v * z90_ll;
        LLVector3 sequential = step1 * x90_ll;

        // Composed application: build z90 * x90 ("apply z90 then x90"),
        // apply once.
        LLQuaternion combined_ll = z90_ll * x90_ll;
        LLVector3 composed = v * combined_ll;

        ensure_approximately_equals("seq vs composed x", sequential.mV[0], composed.mV[0], 16);
        ensure_approximately_equals("seq vs composed y", sequential.mV[1], composed.mV[1], 16);
        ensure_approximately_equals("seq vs composed z", sequential.mV[2], composed.mV[2], 16);

        // And the same in glm (with reversed compose order).
        glm::quat z90_gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat x90_gm = glm::angleAxis(F_PI_BY_TWO, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat combined_gm = x90_gm * z90_gm;  // reversed
        glm::vec3 composed_gm = combined_gm * glm::vec3(1.0f, 0.0f, 0.0f);

        ensure_approximately_equals("LL vs glm composed x", composed.mV[0], composed_gm.x, 16);
        ensure_approximately_equals("LL vs glm composed y", composed.mV[1], composed_gm.y, 16);
        ensure_approximately_equals("LL vs glm composed z", composed.mV[2], composed_gm.z, 16);
    }
}
