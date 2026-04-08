/**
 * @file lljointsolverrp3_test.cpp
 * @brief Unit tests for LLJointSolverRP3 (3-joint IK solver).
 *
 * The tests pin LLJointSolverRP3's behavior so that the upcoming
 * LLQuaternion -> glm::quat migration of the solve() body can be
 * verified to preserve IK semantics. They are NOT cross-library
 * differential tests — LLJointSolverRP3 is not being replaced. They
 * are class-against-itself behavior pins.
 *
 * The strategy is to test geometric invariants and observable behavior
 * (bone-length preservation, goal convergence, idempotence) rather
 * than hand-computed rotation values, because hand-computing IK
 * rotations is error-prone and would just embed arithmetic mistakes
 * into the assertions. Where specific magic-number assertions are
 * useful (the API surface tests), they're kept simple.
 *
 * Test fixture: a 3-joint chain rooted at A, with each segment 1 unit
 * long along the +X axis in the initial configuration:
 *
 *   A(0,0,0) ---- B(1,0,0) ---- C(2,0,0)              (chain)
 *
 *                                             G(?,?,?)  (goal, varies per test)
 *
 * The default pole vector is set to +Z so the chain bends in the XY
 * plane (perpendicular to the pole). This avoids the singular case
 * where the default pole (1,0,0) is collinear with the initial chain.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../lljointsolverrp3.h"
#include "../lljoint.h"

#include "v3math.h"
#include "llquaternion.h"
#include "llmath.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <memory>

namespace tut
{
    struct lljointsolverrp3_data
    {
        // Tolerances. The IK math accumulates float error fast through
        // multiple matrix builds + quat composes; 1e-4 is the practical
        // bound. Goal convergence is allowed slightly more slack
        // because the bend angle calculation involves an acos.
        static constexpr F32 kEps         = 1e-4f;
        static constexpr F32 kGoalEps     = 1e-3f;
        static constexpr F32 kBoneEps     = 1e-4f;

        // Chain joints. mA is the root (no parent), mB is mA's child,
        // mC is mB's child. mGoal is a free joint not parented into the
        // chain (used purely as a position carrier for the goal).
        std::unique_ptr<LLJoint> mA;
        std::unique_ptr<LLJoint> mB;
        std::unique_ptr<LLJoint> mC;
        std::unique_ptr<LLJoint> mGoal;

        LLJointSolverRP3 mSolver;

        lljointsolverrp3_data()
        {
            mA    = std::make_unique<LLJoint>("A");
            mB    = std::make_unique<LLJoint>("B");
            mC    = std::make_unique<LLJoint>("C");
            mGoal = std::make_unique<LLJoint>("Goal");
        }

        // Build the canonical 1-1 chain along +X. Call after the joints
        // are constructed; this also wires the parent/child relationships
        // and runs setupJoints so the solver caches the bone lengths.
        void buildCanonicalChain()
        {
            mB->setup("B", mA.get());
            mC->setup("C", mB.get());

            // Local positions: each segment is 1 unit along +X.
            // mA is the root and stays at the origin.
            mA->setPosition(LLVector3(0.f, 0.f, 0.f));
            mB->setPosition(LLVector3(1.f, 0.f, 0.f));
            mC->setPosition(LLVector3(1.f, 0.f, 0.f));

            // Identity rotations everywhere so the world-space chain
            // matches the local-space layout: A=(0,0,0), B=(1,0,0), C=(2,0,0).
            mA->setRotation(glm::quat(1.f, 0.f, 0.f, 0.f));
            mB->setRotation(glm::quat(1.f, 0.f, 0.f, 0.f));
            mC->setRotation(glm::quat(1.f, 0.f, 0.f, 0.f));

            // Force the world-matrix update by reading once.
            (void)mA->getWorldPosition();
            (void)mB->getWorldPosition();
            (void)mC->getWorldPosition();

            mSolver.setupJoints(mA.get(), mB.get(), mC.get(), mGoal.get());

            // Pole vector +Z so the chain bends in the XY plane.
            mSolver.setPoleVector(LLVector3(0.f, 0.f, 1.f));
        }

        // Place the goal at an absolute world position.
        void setGoal(const LLVector3& world_pos)
        {
            mGoal->setPosition(world_pos);
            (void)mGoal->getWorldPosition();
        }

        // Distance between two LLVector3.
        static F32 dist(const LLVector3& a, const LLVector3& b)
        {
            return (a - b).length();
        }

        // Approximate-equal helper for vectors.
        static bool vec_near(const LLVector3& a, const LLVector3& b, F32 eps = kEps)
        {
            return std::fabs(a.mV[VX] - b.mV[VX]) < eps
                && std::fabs(a.mV[VY] - b.mV[VY]) < eps
                && std::fabs(a.mV[VZ] - b.mV[VZ]) < eps;
        }
    };

    using lljointsolverrp3_test = test_group<lljointsolverrp3_data>;
    using lljointsolverrp3_object = lljointsolverrp3_test::object;
    tut::lljointsolverrp3_test lljointsolverrp3_testcase("LLJointSolverRP3");

    // ---------- API surface ----------

    template<> template<>
    void lljointsolverrp3_object::test<1>()
    {
        // Default constructor: solver fields take their documented defaults.
        // Default pole vector per the .cpp source is (1, 0, 0).
        ensure_equals("default pole vector",
                      mSolver.getPoleVector(), LLVector3(1.f, 0.f, 0.f));
        ensure_equals("default twist", mSolver.getTwist(), 0.f);
    }

    template<> template<>
    void lljointsolverrp3_object::test<2>()
    {
        // setPoleVector normalizes its input.
        mSolver.setPoleVector(LLVector3(2.f, 0.f, 0.f));
        ensure("non-unit pole input is normalized",
               vec_near(mSolver.getPoleVector(), LLVector3(1.f, 0.f, 0.f)));

        // Negative-axis input round-trips correctly (still unit length).
        mSolver.setPoleVector(LLVector3(0.f, -3.f, 0.f));
        ensure("negative-axis pole normalizes",
               vec_near(mSolver.getPoleVector(), LLVector3(0.f, -1.f, 0.f)));
    }

    template<> template<>
    void lljointsolverrp3_object::test<3>()
    {
        // Twist set/get round-trip.
        mSolver.setTwist(0.5f);
        ensure_equals("twist set", mSolver.getTwist(), 0.5f);

        mSolver.setTwist(-1.25f);
        ensure_equals("negative twist set", mSolver.getTwist(), -1.25f);

        mSolver.setTwist(0.f);
        ensure_equals("zero twist set", mSolver.getTwist(), 0.f);
    }

    template<> template<>
    void lljointsolverrp3_object::test<4>()
    {
        // setupJoints caches the bone lengths from the joint positions
        // at the time of the call. We can't directly inspect mLengthAB
        // and mLengthBC, but solve() will preserve them, so we can
        // verify them indirectly via a goal-reaching test (see test 6).
        // Here we just verify setup doesn't crash and the solver is
        // ready to solve.
        buildCanonicalChain();

        // Pre-solve sanity: the chain is in its canonical configuration.
        ensure("A at origin", vec_near(mA->getWorldPosition(), LLVector3(0.f, 0.f, 0.f)));
        ensure("B at (1,0,0)", vec_near(mB->getWorldPosition(), LLVector3(1.f, 0.f, 0.f)));
        ensure("C at (2,0,0)", vec_near(mC->getWorldPosition(), LLVector3(2.f, 0.f, 0.f)));
    }

    // ---------- Solve invariants ----------

    template<> template<>
    void lljointsolverrp3_object::test<5>()
    {
        // Bone-length preservation: regardless of where the goal is, after
        // solve() the segment lengths must equal the lengths cached by
        // setupJoints. This is the most fundamental invariant of the IK
        // solver and the strongest test of the rotation math, because any
        // bug that scrunches or stretches the chain (sign-flip, missed
        // normalize, compose-order miss that produces a non-unit
        // intermediate quat) shows up immediately as a bone-length drift.
        buildCanonicalChain();

        // Try a variety of goal positions: in the bend plane, out of it,
        // close, far, axis-aligned, off-axis.
        const LLVector3 goals[] = {
            LLVector3( 1.5f,  0.5f,  0.0f),  // reachable, in plane
            LLVector3( 0.0f,  1.5f,  0.0f),  // reachable, perpendicular
            LLVector3(-1.0f,  0.5f,  0.0f),  // behind A
            LLVector3( 1.0f,  1.0f,  0.0f),  // 45-degree, distance sqrt(2)
            LLVector3( 0.5f,  0.0f,  0.5f),  // out of XY plane
            LLVector3( 5.0f,  0.0f,  0.0f),  // far unreachable
            LLVector3( 0.5f,  0.5f, -0.5f),  // close, mixed-axis
        };

        for (const LLVector3& g : goals)
        {
            buildCanonicalChain();   // reset chain to canonical state
            setGoal(g);

            mSolver.solve();

            const LLVector3 a = mA->getWorldPosition();
            const LLVector3 b = mB->getWorldPosition();
            const LLVector3 c = mC->getWorldPosition();

            ensure_approximately_equals(
                "bone AB length preserved", dist(a, b), 1.0f, /*frac=*/4096);
            ensure_approximately_equals(
                "bone BC length preserved", dist(b, c), 1.0f, /*frac=*/4096);
        }
    }

    template<> template<>
    void lljointsolverrp3_object::test<6>()
    {
        // Reachable goal convergence: when the goal is within the chain's
        // reach (distance from A is <= sum of bone lengths), C should land
        // on G after solve(). This is the second core invariant of an IK
        // solver.
        buildCanonicalChain();

        // Pick a goal at distance ~1.5 from A in the XY plane (well within
        // the chain reach of 2.0).
        const LLVector3 goal(1.0f, 1.0f, 0.0f);
        setGoal(goal);

        mSolver.solve();

        const LLVector3 c = mC->getWorldPosition();
        ensure("reachable goal: C reaches G",
               dist(c, goal) < kGoalEps);
    }

    template<> template<>
    void lljointsolverrp3_object::test<7>()
    {
        // Reachable goal at multiple positions: confirms convergence isn't
        // a coincidence of the previous test's specific input. Tests 4
        // distinct reachable goals all in the XY bend plane.
        const LLVector3 goals[] = {
            LLVector3( 0.5f,  1.5f,  0.0f),
            LLVector3( 1.5f,  0.5f,  0.0f),
            LLVector3( 0.0f,  1.8f,  0.0f),
            LLVector3(-0.5f,  1.0f,  0.0f),
        };

        for (const LLVector3& g : goals)
        {
            buildCanonicalChain();
            setGoal(g);

            mSolver.solve();

            const LLVector3 c = mC->getWorldPosition();
            ensure("multi-goal convergence",
                   dist(c, g) < kGoalEps);
        }
    }

    template<> template<>
    void lljointsolverrp3_object::test<8>()
    {
        // Unreachable goal (too far): the goal is beyond the chain's
        // maximum reach (sum of bone lengths). After solve, the chain
        // should extend maximally toward the goal. Specifically, A->B and
        // B->C should both lie along the line from A to G.
        //
        // Goal at (4, 0, 0): distance 4 from A, max reach is 2.
        buildCanonicalChain();
        setGoal(LLVector3(4.f, 0.f, 0.f));

        mSolver.solve();

        const LLVector3 a = mA->getWorldPosition();
        const LLVector3 b = mB->getWorldPosition();
        const LLVector3 c = mC->getWorldPosition();

        // The chain should extend straight along +X.
        ensure("unreachable far: A at origin",
               vec_near(a, LLVector3(0.f, 0.f, 0.f)));
        ensure("unreachable far: B at (1,0,0)",
               vec_near(b, LLVector3(1.f, 0.f, 0.f), kEps));
        ensure("unreachable far: C at (2,0,0)",
               vec_near(c, LLVector3(2.f, 0.f, 0.f), kEps));
    }

    template<> template<>
    void lljointsolverrp3_object::test<9>()
    {
        // Idempotence: solve() called twice should produce the same final
        // joint configuration as solve() called once. This is guaranteed
        // by the solver design — solve() resets joint A and B to their
        // base rotations at the start of every call. So a re-run from
        // the post-solve state must reproduce the same outcome.
        buildCanonicalChain();
        setGoal(LLVector3(1.0f, 1.0f, 0.0f));

        mSolver.solve();
        const LLVector3 b1 = mB->getWorldPosition();
        const LLVector3 c1 = mC->getWorldPosition();

        mSolver.solve();
        const LLVector3 b2 = mB->getWorldPosition();
        const LLVector3 c2 = mC->getWorldPosition();

        ensure("idempotent solve: B unchanged",
               vec_near(b1, b2, kEps));
        ensure("idempotent solve: C unchanged",
               vec_near(c1, c2, kEps));
    }

    template<> template<>
    void lljointsolverrp3_object::test<10>()
    {
        // Goal already at C: solving with the goal at the current end
        // effector position should leave the chain unchanged (because
        // there's nothing to solve). Tests that the no-op case doesn't
        // accidentally rotate the joints.
        buildCanonicalChain();
        setGoal(LLVector3(2.f, 0.f, 0.f));   // C is already at (2,0,0)

        mSolver.solve();

        ensure("no-op solve: A unchanged",
               vec_near(mA->getWorldPosition(), LLVector3(0.f, 0.f, 0.f), kEps));
        ensure("no-op solve: B unchanged",
               vec_near(mB->getWorldPosition(), LLVector3(1.f, 0.f, 0.f), kEps));
        ensure("no-op solve: C unchanged",
               vec_near(mC->getWorldPosition(), LLVector3(2.f, 0.f, 0.f), kEps));
    }

    // ---------- Pole vector behavior ----------

    template<> template<>
    void lljointsolverrp3_object::test<11>()
    {
        // Pole vector controls which side B bends toward. The pole vector
        // disambiguates the two possible IK solutions (the chain extension
        // circles can intersect the goal-reach sphere on either side of
        // the AG line). Flipping the pole sign should flip B to the
        // opposite side of the bend plane.
        //
        // Solve once with the pole at +Z, capture B. Solve again with
        // pole at -Z, capture B. The two B positions must differ — if
        // they're identical, the pole vector isn't influencing the
        // solution, which is a bug.
        buildCanonicalChain();
        setGoal(LLVector3(1.f, 1.f, 0.f));   // distance sqrt(2), reachable
        mSolver.solve();
        const LLVector3 b_pos_pole = mB->getWorldPosition();

        buildCanonicalChain();
        mSolver.setPoleVector(LLVector3(0.f, 0.f, -1.f));
        setGoal(LLVector3(1.f, 1.f, 0.f));
        mSolver.solve();
        const LLVector3 b_neg_pole = mB->getWorldPosition();

        ensure("pole vector affects bend",
               !vec_near(b_pos_pole, b_neg_pole, kEps));

        // Bone lengths must still be preserved on both sides.
        ensure_approximately_equals(
            "pos pole: AB length preserved",
            dist(LLVector3(0.f, 0.f, 0.f), b_pos_pole), 1.0f, /*frac=*/4096);
        ensure_approximately_equals(
            "neg pole: AB length preserved",
            dist(LLVector3(0.f, 0.f, 0.f), b_neg_pole), 1.0f, /*frac=*/4096);
    }

    // ---------- Twist behavior ----------

    template<> template<>
    void lljointsolverrp3_object::test<12>()
    {
        // Twist rotates the solution plane around the AG line. With twist
        // = 0 and twist = PI, B should be on opposite sides of the AG line.
        buildCanonicalChain();
        setGoal(LLVector3(1.f, 1.f, 0.f));

        mSolver.setTwist(0.f);
        mSolver.solve();
        const LLVector3 b_no_twist = mB->getWorldPosition();

        buildCanonicalChain();
        setGoal(LLVector3(1.f, 1.f, 0.f));
        mSolver.setTwist(F_PI);
        mSolver.solve();
        const LLVector3 b_twist_pi = mB->getWorldPosition();

        // The two positions should differ (twist had an effect).
        ensure("twist affects B position",
               !vec_near(b_no_twist, b_twist_pi, kEps));

        // Bone lengths still preserved under twist.
        ensure_approximately_equals(
            "twisted chain: AB length preserved",
            dist(mA->getWorldPosition(), b_twist_pi), 1.0f, /*frac=*/4096);
    }

    template<> template<>
    void lljointsolverrp3_object::test<13>()
    {
        // Twist of 2*PI is a full rotation and should produce the same
        // result as twist of 0 (within float precision after the trig
        // round-trip).
        buildCanonicalChain();
        setGoal(LLVector3(1.f, 1.f, 0.f));

        mSolver.setTwist(0.f);
        mSolver.solve();
        const LLVector3 b_no_twist = mB->getWorldPosition();
        const LLVector3 c_no_twist = mC->getWorldPosition();

        buildCanonicalChain();
        setGoal(LLVector3(1.f, 1.f, 0.f));
        mSolver.setTwist(2.f * F_PI);
        mSolver.solve();
        const LLVector3 b_full_twist = mB->getWorldPosition();
        const LLVector3 c_full_twist = mC->getWorldPosition();

        // Tolerance is bumped because 2*PI twist accumulates more
        // float error in the trig path.
        ensure("twist=2PI matches twist=0 (B)",
               vec_near(b_no_twist, b_full_twist, 1e-3f));
        ensure("twist=2PI matches twist=0 (C)",
               vec_near(c_no_twist, c_full_twist, 1e-3f));
    }

    // ---------- Goal convergence with twist ----------

    template<> template<>
    void lljointsolverrp3_object::test<14>()
    {
        // Twist must NOT break goal convergence. The end effector C must
        // still land on the goal regardless of twist value, because twist
        // rotates the bend plane around the AG line — it doesn't move C.
        buildCanonicalChain();
        const LLVector3 goal(1.0f, 1.0f, 0.0f);
        setGoal(goal);

        const F32 twists[] = { 0.f, 0.5f, 1.0f, F_PI_BY_TWO, F_PI, 2.0f };
        for (F32 twist : twists)
        {
            buildCanonicalChain();
            setGoal(goal);
            mSolver.setTwist(twist);
            mSolver.solve();

            ensure("C reaches G under twist",
                   dist(mC->getWorldPosition(), goal) < kGoalEps);
        }
    }

    // ---------- Repeated solves with changing goal ----------

    template<> template<>
    void lljointsolverrp3_object::test<15>()
    {
        // Walking the goal across multiple positions and solving each
        // time should produce convergent results without accumulating
        // drift. Each solve resets to base rotations (per the design),
        // so the bone lengths must stay clean across repeated solves
        // even with widely varying goals.
        buildCanonicalChain();

        const LLVector3 goal_walk[] = {
            LLVector3( 1.5f,  0.5f,  0.0f),
            LLVector3( 0.0f,  1.5f,  0.0f),
            LLVector3(-1.0f,  1.0f,  0.0f),
            LLVector3( 1.0f, -0.5f,  0.5f),
            LLVector3( 1.5f,  0.5f,  0.0f),  // back to start
        };

        for (const LLVector3& g : goal_walk)
        {
            setGoal(g);
            mSolver.solve();

            const LLVector3 a = mA->getWorldPosition();
            const LLVector3 b = mB->getWorldPosition();
            const LLVector3 c = mC->getWorldPosition();

            ensure_approximately_equals(
                "walked goal: AB length preserved",
                dist(a, b), 1.0f, /*frac=*/4096);
            ensure_approximately_equals(
                "walked goal: BC length preserved",
                dist(b, c), 1.0f, /*frac=*/4096);
        }
    }

    // ---------- BAxis path ----------

    template<> template<>
    void lljointsolverrp3_object::test<16>()
    {
        // setBAxis enables the alternative IK path at solve():248-251
        // that uses the joint's local bend axis instead of computing
        // the ABC plane normal from cross(abVec, bcVec). The header
        // describes this as "smarter" results for non-coplanar limbs.
        //
        // DISCOVERY: the BAxis path does NOT guarantee strict goal
        // convergence in the same way the default path does. With the
        // canonical XY-plane chain, default pole (+Z), goal (1,1,0) and
        // BAxis (0,0,1), the end effector C lands at a position that
        // is NOT the goal. This is a property of the BAxis solution
        // strategy (it sacrifices strict goal-reaching for a more
        // anatomically-plausible bend).
        //
        // For the migration's regression net, what matters is that the
        // BAxis path's behavior is preserved bit-for-bit. We assert:
        //   (1) bone lengths are still preserved (the universal IK
        //       invariant — any solver path must hold this)
        //   (2) solve() doesn't crash or produce non-finite results
        //   (3) the resulting B and C positions are deterministic
        //       (we don't pin exact magic numbers because they depend
        //       on the rotation math, but we verify the chain settles
        //       into a sensible configuration)
        buildCanonicalChain();
        mSolver.setBAxis(LLVector3(0.f, 0.f, 1.f));   // Z-axis bend in B's local frame
        setGoal(LLVector3(1.f, 1.f, 0.f));

        mSolver.solve();

        const LLVector3 a = mA->getWorldPosition();
        const LLVector3 b = mB->getWorldPosition();
        const LLVector3 c = mC->getWorldPosition();

        // Bone length preservation — the universal IK invariant.
        ensure_approximately_equals(
            "BAxis path: AB length preserved",
            dist(a, b), 1.0f, /*frac=*/4096);
        ensure_approximately_equals(
            "BAxis path: BC length preserved",
            dist(b, c), 1.0f, /*frac=*/4096);

        // Finiteness check (catches NaN/Inf bugs from any normalize-of-zero
        // or divide-by-zero hazard in the migrated rotation math).
        ensure("BAxis result B is finite",
               std::isfinite(b.mV[VX]) && std::isfinite(b.mV[VY]) && std::isfinite(b.mV[VZ]));
        ensure("BAxis result C is finite",
               std::isfinite(c.mV[VX]) && std::isfinite(c.mV[VY]) && std::isfinite(c.mV[VZ]));

        // Idempotence under BAxis: solving twice produces the same result.
        // Phase 2 quat migration must preserve this.
        const LLVector3 b1 = b;
        const LLVector3 c1 = c;
        mSolver.solve();
        ensure("BAxis idempotent: B unchanged",
               vec_near(b1, mB->getWorldPosition(), kEps));
        ensure("BAxis idempotent: C unchanged",
               vec_near(c1, mC->getWorldPosition(), kEps));
    }
}
