/**
 * @file llik_test.cpp
 * @author Leviathan
 * @date 2022
 * @brief LLIK unit tests
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "../llik.h"
#include "../test/lltut.h"
#include <iostream>
#include <cmath>

// HOW TO DEBUG an LLIK unit test:
// (1) Uncomment #define DEBUG_LLIK_UNIT_TESTS in llik.h
// (2) Add line like this to the particular test
//        solver.enableDebugIfPossible();
//     right before a line like:
//        solver.configureAndSolve();
// (3) Compile and link
// (5) Run the test from the CLI and pipe the output to a file:
//        ./PROJECT_llcharacter_TEST_llik.exe > /tmp/test_data
// (5) Extract the python-esq variables from the file in (4) and copy them
//     to the right place in plot_ik_test.py script
// (6) Run plot_ik_test.py to watch an animated plot of the IK solution in
//     action:
//        python plot_ik_test.py
//


namespace tut
{
	struct llik_data
	{
	};
	typedef test_group<llik_data> llik_test;
	typedef llik_test::object llik_object;
	tut::llik_test llik_testcase("LLIK");

    // SimpleCone
	template<> template<>
	void llik_object::test<1>()
	{
        LLVector3 forward_axis = LLVector3::y_axis;
        F32 cone_angle = F_PI / 4.0;

        LLIK::SimpleCone constraint(forward_axis, cone_angle);
        LLQuaternion adjusted_q;

        {
            LLQuaternion max_bend_q;
            LLQuaternion mid_bend_q;

            S32 num_pivots = 3;
            constexpr F32 ACCEPTABLE_ERROR = 1.0e-3f; // one mm
            constexpr F32 EXPANDED_SLOP = 1.0e-2f;

            for (S32 i = 0; i < num_pivots; ++i)
            {
                F32 azimuth = F_TWO_PI * F32(i) / F32(num_pivots);
                LLVector3 pivot_axis = std::cos(azimuth) * LLVector3::x_axis + std::sin(azimuth) * LLVector3::z_axis;

                // no adjustment necessary: bend
                max_bend_q.setAngleAxis(cone_angle, pivot_axis);
                adjusted_q = constraint.computeAdjustedLocalRot(max_bend_q);
                ensure("LLIK::SimpleCone should not adjust Q=max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

                F32 half_cone_angle = 0.5f * cone_angle;
                mid_bend_q.setAngleAxis(half_cone_angle, pivot_axis);
                adjusted_q = constraint.computeAdjustedLocalRot(mid_bend_q);
                ensure("LLIK::SimpleCone should not adjust Q=mid_bend", LLQuaternion::almost_equal(adjusted_q, mid_bend_q));

                // no adjustment necessary: twist
                F32 twist = azimuth + 0.1f * F_PI;
                LLQuaternion dQ;
                dQ.setAngleAxis(twist, forward_axis);
                LLQuaternion Q = dQ * mid_bend_q;
                adjusted_q = constraint.computeAdjustedLocalRot(Q);
                ensure("LLIK::SimpleCone should not constrain twist", LLQuaternion::almost_equal(adjusted_q, Q, EXPANDED_SLOP));

                // yes adjustment necessary: too much bend
                F32 del = 0.2f;
                Q.setAngleAxis(cone_angle + del, pivot_axis);
                adjusted_q = constraint.computeAdjustedLocalRot(max_bend_q);
                ensure("LLIK::SimpleCone should adjust Q for too much bend", !LLQuaternion::almost_equal(adjusted_q, Q));
                ensure("LLIK::SimpleCone should clamp to cone_angle", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

                // yes adjustment necessary: too much bend with twist
                dQ.setAngleAxis(twist, forward_axis);
                Q.setAngleAxis(cone_angle + del, pivot_axis);
                Q = dQ * Q;
                adjusted_q = constraint.computeAdjustedLocalRot(Q);
                ensure("LLIK::SimpleCone should constrain too much bend, even with some twist",
                        !LLQuaternion::almost_equal(adjusted_q, Q, EXPANDED_SLOP));
                // rather than compare rotations we'll compare how they transform forward_axis
                F32 error = dist_vec(forward_axis * max_bend_q, forward_axis * adjusted_q);
                ensure("LLIK::SimpleCone should swing forward to lie inside cone", error < ACCEPTABLE_ERROR);
            }

            // do it again but with negative bend
            for (S32 i = 0; i < num_pivots; ++i)
            {
                F32 azimuth = F_TWO_PI * F32(i) / F32(num_pivots);
                LLVector3 pivot_axis = std::cos(azimuth) * LLVector3::x_axis + std::sin(azimuth) * LLVector3::z_axis;

                // no adjustment necessary: bend
                max_bend_q.setAngleAxis(-cone_angle, pivot_axis);
                adjusted_q = constraint.computeAdjustedLocalRot(max_bend_q);
                ensure("LLIK::SimpleCone should not adjust Q=-max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

                F32 half_cone_angle = 0.5f * cone_angle;
                mid_bend_q.setAngleAxis(-half_cone_angle, pivot_axis);
                adjusted_q = constraint.computeAdjustedLocalRot(mid_bend_q);
                ensure("LLIK::SimpleCone should not adjust Q=-mid_bend", LLQuaternion::almost_equal(adjusted_q, mid_bend_q));

                // no adjustment necessary: twist
                F32 twist = azimuth + 0.1f * F_PI;
                LLQuaternion dQ;
                dQ.setAngleAxis(twist, forward_axis);
                LLQuaternion Q = dQ * mid_bend_q;
                adjusted_q = constraint.computeAdjustedLocalRot(Q);
                ensure("LLIK::SimpleCone should not constrain twist", LLQuaternion::almost_equal(adjusted_q, Q, EXPANDED_SLOP));

                // yes adjustment necessary: too much bend
                F32 del = 0.1f;
                Q.setAngleAxis(-cone_angle - del, pivot_axis);
                adjusted_q = constraint.computeAdjustedLocalRot(max_bend_q);
                ensure("LLIK::SimpleCone should adjust Q for too much negative bend", !LLQuaternion::almost_equal(adjusted_q, Q));
                ensure("LLIK::SimpleCone should clamp to -cone_angle", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

                // yes adjustment necessary: too much bend with twist
                dQ.setAngleAxis(twist, forward_axis);
                Q.setAngleAxis(-cone_angle - del, pivot_axis);
                Q = dQ * Q;
                adjusted_q = constraint.computeAdjustedLocalRot(Q);
                ensure("LLIK::SimpleCone should constrain too much bend, even with some twist",
                        !LLQuaternion::almost_equal(adjusted_q, Q, EXPANDED_SLOP));
                // rather than compare rotations we'll compare how they transform forward_axis
                F32 error = dist_vec(forward_axis * max_bend_q, forward_axis * adjusted_q);
                ensure("LLIK::SimpleCone should swing forward to lie inside cone", error < ACCEPTABLE_ERROR);
            }
        }

        // test minimizeTwist()
        {
            LLVector3 left_axis = LLVector3::x_axis;
            F32 bend_angle = cone_angle + 0.1f;
            F32 twist_angle = 1.23f;
            LLQuaternion bend;
            bend.setAngleAxis(bend_angle, left_axis);
            LLQuaternion twist;
            twist.setAngleAxis(twist_angle, forward_axis);

            LLQuaternion Q = twist * bend;
            adjusted_q = constraint.minimizeTwist(Q);
            ensure("LLIK::SimpleCone should remove twist", LLQuaternion::almost_equal(adjusted_q, bend));
        }
    }

    // KneeConstraint
	template<> template<>
	void llik_object::test<2>()
	{
        LLVector3 forward_axis = LLVector3::y_axis;
        LLVector3 pivot_axis = LLVector3::x_axis;
        F32 min_bend = -F_PI / 4.0;
        F32 max_bend = F_PI / 2.0;

        LLIK::KneeConstraint constraint(forward_axis, pivot_axis, min_bend, max_bend);

        LLQuaternion adjusted_q;

        {
            // no adjustment necessary
            LLQuaternion min_bend_q;
            min_bend_q.setAngleAxis(min_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(min_bend_q);
            ensure("LLIK::KneeConstraint should not adjust Q=min_bend", LLQuaternion::almost_equal(adjusted_q, min_bend_q));

            LLQuaternion max_bend_q;
            max_bend_q.setAngleAxis(max_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(max_bend_q);
            ensure("LLIK::KneeConstraint should not adjust Q=max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            F32 mid_bend = 0.5f * (max_bend + min_bend);
            LLQuaternion mid_bend_q;
            mid_bend_q.setAngleAxis(mid_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(mid_bend_q);
            ensure("LLIK::KneeConstraint should not adjust Q=mid_bend", LLQuaternion::almost_equal(adjusted_q, mid_bend_q));

            // yes adjustment necessary
            F32 del = 0.01f;
            LLQuaternion Q;
            Q.setAngleAxis(min_bend - del, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::KneeConstraint should adjust Q below min_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::KneeConstraint should clamp Q to min_bend", LLQuaternion::almost_equal(adjusted_q, min_bend_q));

            Q.setAngleAxis(max_bend + del, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::KneeConstraint should adjust Q above max_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::KneeConstraint should clamp Q to max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            F32 smallest_min_bend = mid_bend - F_PI + del;
            Q.setAngleAxis(smallest_min_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::KneeConstraint should adjust Q at smallest_min_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::KneeConstraint should clamp smallest_min_bend to min_bend", LLQuaternion::almost_equal(adjusted_q, min_bend_q));

            F32 largest_max_bend = mid_bend + F_PI - del;
            Q.setAngleAxis(largest_max_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::KneeConstraint should adjust Q at largest_max_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::KneeConstraint should clamp largest_max_bend to max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            // add twist
            LLQuaternion dQ;
            dQ.setAngleAxis(del, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::KneeConstraint should adjust Q with twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::KneeConstraint should clamp twist to mid_bend", LLQuaternion::almost_equal(adjusted_q, mid_bend_q));

            // swing forward out of pivot plane
            LLVector3 out = forward_axis % pivot_axis;
            out.normalize();
            dQ.setAngleAxis(del, out);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::KneeConstraint should adjust Q with swing", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::KneeConstraint should clamp swing to mid_bend", LLQuaternion::almost_equal(adjusted_q, mid_bend_q));
        }

        // test minimizeTwist()
        {
            LLVector3 off_pivot_axis = pivot_axis % forward_axis;
            F32 bend_angle = max_bend - 0.1f;
            F32 twist_angle = 1.23f;
            LLQuaternion bend;
            bend.setAngleAxis(bend_angle, off_pivot_axis);
            LLQuaternion twist;
            twist.setAngleAxis(twist_angle, forward_axis);

            LLQuaternion Q = twist * bend;
            adjusted_q = constraint.minimizeTwist(Q);

            // Note: KneeConstraint doesn't actually minimizeTwist per-se...
            // instead it assumes all bend is about mPivotAxis
            LLQuaternion expected_q;
            expected_q.setAngleAxis(bend_angle, pivot_axis);
            constexpr F32 MAX_ANGLE_ERROR = 1.0e-3f * F_PI;
            ensure("LLIK::KneeConstraint should remove twist", LLQuaternion::almost_equal(adjusted_q, expected_q, MAX_ANGLE_ERROR));
        }

        // test "fliped minimizeTwist" behavior
        {
            // KneeConstraint has an non-obvious behavior: When bend is outside
            // allowed range, it will attempt to to flip the bend in the
            // opposite direction, and if that inverted angle falls within
            // range, or is closer to the midpoint of the allowed range then it
            // "twists" to accomplish that.
            //
            // For example, if we go just below min_bend (min_bend - del) then
            // minimizeTwist() will report back -(min_bend - del) which is inside the
            // allowed bend range.
            LLVector3 off_pivot_axis = pivot_axis % forward_axis;
            F32 bend_angle = min_bend - 0.01f;
            F32 twist_angle = 1.23f;
            LLQuaternion bend;
            bend.setAngleAxis(bend_angle, off_pivot_axis);
            LLQuaternion twist;
            twist.setAngleAxis(twist_angle, forward_axis);

            LLQuaternion Q = twist * bend;
            adjusted_q = constraint.minimizeTwist(Q);

            LLQuaternion expected_q;
            expected_q.setAngleAxis(-bend_angle, pivot_axis);
            constexpr F32 MAX_ANGLE_ERROR = 1.0e-3f * F_PI;
            ensure("LLIK::KneeConstraint should prefer to remove flipped twist", LLQuaternion::almost_equal(adjusted_q, expected_q, MAX_ANGLE_ERROR));
        }
    }

    // ElbowConstraint
	template<> template<>
	void llik_object::test<3>()
	{
        LLVector3 forward_axis = LLVector3::y_axis;
        LLVector3 pivot_axis = LLVector3::x_axis;
        F32 min_bend = -0.1f;
        F32 max_bend = 0.9f * F_PI;
        F32 min_twist = -F_PI / 6.0f;
        F32 max_twist = F_PI / 5.0f;

        LLIK::ElbowConstraint constraint(forward_axis, pivot_axis, min_bend, max_bend, min_twist, max_twist);

        LLQuaternion adjusted_q;

        {
            // no adjustment necessary: bend
            LLQuaternion min_bend_q;
            min_bend_q.setAngleAxis(min_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(min_bend_q);
            ensure("LLIK::ElbowConstraint should not adjust Q=min_bend", LLQuaternion::almost_equal(adjusted_q, min_bend_q));

            LLQuaternion max_bend_q;
            max_bend_q.setAngleAxis(max_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(max_bend_q);
            ensure("LLIK::ElbowConstraint should not adjust Q=max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            F32 mid_bend = 0.5f * (max_bend + min_bend);
            LLQuaternion mid_bend_q;
            mid_bend_q.setAngleAxis(mid_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(mid_bend_q);
            ensure("LLIK::ElbowConstraint should not adjust Q=mid_bend", LLQuaternion::almost_equal(adjusted_q, mid_bend_q));

            // no adjustment necessary: twist
            LLQuaternion dQ;
            dQ.setAngleAxis(min_twist, forward_axis);
            LLQuaternion Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should not adjust Q=min_twist", LLQuaternion::almost_equal(adjusted_q, Q));

            dQ.setAngleAxis(max_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should not adjust Q=max_twist", LLQuaternion::almost_equal(adjusted_q, Q));

            F32 mid_twist = 0.5f * (max_twist + min_twist);
            dQ.setAngleAxis(mid_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should not adjust Q=mid_twist", LLQuaternion::almost_equal(adjusted_q, Q));

            // yes adjustment necessary: too much bend
            F32 del = 0.01f;
            Q.setAngleAxis(min_bend - del, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q below min_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should clamp Q to min_bend", LLQuaternion::almost_equal(adjusted_q, min_bend_q));

            Q.setAngleAxis(max_bend + del, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q above max_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should clamp Q to max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            F32 smallest_min_bend = mid_bend - F_PI + del;
            Q.setAngleAxis(smallest_min_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q at smallest_min_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should clamp smallest_min_bend to min_bend", LLQuaternion::almost_equal(adjusted_q, min_bend_q));

            F32 largest_max_bend = mid_bend + F_PI - del;
            Q.setAngleAxis(largest_max_bend, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q at largest_max_bend", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should clamp largest_max_bend to max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            // adjustment necessary: beyond min_twist
            dQ.setAngleAxis(min_twist, forward_axis);
            LLQuaternion expected_adjusted_q = dQ * mid_bend_q;
            dQ.setAngleAxis(min_twist - del, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q for below min_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should adjust Q back to min_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));

            F32 smallest_min_twist = mid_twist - F_PI + del;
            dQ.setAngleAxis(smallest_min_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q for smallest_min_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should adjust Q smallest_min_twist back to min_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));

            // adjustment necessary: beyond max_twist
            dQ.setAngleAxis(max_twist, forward_axis);
            expected_adjusted_q = dQ * mid_bend_q;
            dQ.setAngleAxis(max_twist + del, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q for above max_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should adjust Q back to max_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));

            F32 largest_max_twist = mid_twist + F_PI - del;
            dQ.setAngleAxis(largest_max_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::ElbowConstraint should adjust Q for largest_max_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::ElbowConstraint should adjust Q largest_max_twist back to max_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));
        }

        // test minimizeTwist()
        {
            LLVector3 off_pivot_axis = pivot_axis % forward_axis;
            F32 bend_angle = 1.23f;
            F32 twist_angle = 0.456f;
            LLQuaternion bend;
            bend.setAngleAxis(bend_angle, off_pivot_axis);
            LLQuaternion twist;
            twist.setAngleAxis(twist_angle, forward_axis);

            LLQuaternion Q = twist * bend;
            adjusted_q = constraint.minimizeTwist(Q);

            // Note: ElbowConstraint doesn't actually minimizeTwist per-se...
            // instead it assumes all bend is about mPivotAxis...
            // then untwists to middle of twist range
            F32 mid_twist = 0.5f * (max_twist + min_twist);
            LLQuaternion expected_twist;
            expected_twist.setAngleAxis(mid_twist, forward_axis);
            LLQuaternion expected_bend;
            expected_bend.setAngleAxis(bend_angle, pivot_axis);
            LLQuaternion expected_q = expected_twist * expected_bend;
            constexpr F32 MAX_ANGLE_ERROR = 1.0e-3f * F_PI;
            ensure("LLIK::ElbowConstraint should remove twist", LLQuaternion::almost_equal(adjusted_q, expected_q, MAX_ANGLE_ERROR));
        }

        // test "fliped minimizeTwist" behavior
        {
            // Similar to KneeConstraint the ElbowConstraint has an non-obvious
            // behavior: When bend is outside allowed range, it will attempt to
            // to flip the bend in the opposite direction, and if that inverted
            // angle falls within range, or is closer to the midpoint of the
            // allowed range then it "twists" to accomplish that.
            //
            // For example, if we go just below min_bend (min_bend - del) then
            // minimizeTwist() will report back -(min_bend - del) which is inside
            // the allowed bend range.
            LLVector3 off_pivot_axis = pivot_axis % forward_axis;
            F32 bend_angle = min_bend - 0.01f;
            F32 twist_angle = 1.23f;
            LLQuaternion bend;
            bend.setAngleAxis(bend_angle, off_pivot_axis);
            LLQuaternion twist;
            twist.setAngleAxis(twist_angle, forward_axis);

            LLQuaternion Q = twist * bend;
            adjusted_q = constraint.minimizeTwist(Q);

            F32 mid_twist = 0.5f * (max_twist + min_twist);
            LLQuaternion expected_twist;
            expected_twist.setAngleAxis(mid_twist, forward_axis);
            LLQuaternion expected_bend;
            expected_bend.setAngleAxis(-bend_angle, pivot_axis);
            LLQuaternion expected_q = expected_twist * expected_bend;
            constexpr F32 MAX_ANGLE_ERROR = 1.0e-3f * F_PI;
            ensure("LLIK::ElbowConstraint should prefer to remove flipped twist", LLQuaternion::almost_equal(adjusted_q, expected_q, MAX_ANGLE_ERROR));
        }
    }

    // TwistLimitedCone
	template<> template<>
	void llik_object::test<4>()
	{
        LLVector3 forward_axis = LLVector3::y_axis;
        F32 cone_angle = F_PI / 8.0f;
        F32 min_twist = -F_PI / 6.0f;
        F32 max_twist = F_PI / 5.0f;

        LLIK::TwistLimitedCone constraint(forward_axis, cone_angle, min_twist, max_twist);

        LLQuaternion adjusted_q;
        LLQuaternion max_bend_q;
        LLQuaternion mid_bend_q;

        S32 num_pivots = 3;
        // Note: some of these tests requires a tolerance slightly looser than default.
        constexpr F32 EXPANDED_SLOP = 1.0e-2f;
        for (S32 i = 0; i < num_pivots; ++i)
        {
            F32 azimuth = F_TWO_PI * F32(i) / F32(num_pivots);
            LLVector3 pivot_axis = std::cos(azimuth) * LLVector3::x_axis + std::sin(azimuth) * LLVector3::z_axis;

            // no adjustment necessary: bend
            max_bend_q.setAngleAxis(cone_angle, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(max_bend_q);
            ensure("LLIK::TwistLimitedCone should not adjust Q=max_bend", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            F32 half_cone_angle = 0.5f * cone_angle;
            mid_bend_q.setAngleAxis(half_cone_angle, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(mid_bend_q);
            ensure("LLIK::TwistLimitedCone should not adjust Q=mid_bend", LLQuaternion::almost_equal(adjusted_q, mid_bend_q));

            // no adjustment necessary: twist
            LLQuaternion dQ;
            dQ.setAngleAxis(min_twist, forward_axis);
            LLQuaternion Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should not adjust Q=min_twist", LLQuaternion::almost_equal(adjusted_q, Q));

            dQ.setAngleAxis(max_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should not adjust Q=max_twist", LLQuaternion::almost_equal(adjusted_q, Q));

            F32 mid_twist = 0.5f * (max_twist + min_twist);
            dQ.setAngleAxis(mid_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should not adjust Q=mid_twist", LLQuaternion::almost_equal(adjusted_q, Q));

            // yes adjustment necessary: too much bend
            F32 del = 0.01f;
            Q.setAngleAxis(cone_angle + del, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should adjust Q above cone_angle", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::TwistLimitedCone should clamp Q to cone_angle", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            Q.setAngleAxis(F_PI - 0.1f, pivot_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should adjust near flip", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::TwistLimitedCone should clamp near flip to cone_angle", LLQuaternion::almost_equal(adjusted_q, max_bend_q));

            // adjustment necessary: beyond min_twist
            dQ.setAngleAxis(min_twist, forward_axis);
            LLQuaternion expected_adjusted_q = dQ * mid_bend_q;
            dQ.setAngleAxis(min_twist - del, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should adjust Q for below min_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::TwistLimitedCone should adjust Q back to min_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q, EXPANDED_SLOP));

            F32 smallest_min_twist = mid_twist - F_PI + del;
            dQ.setAngleAxis(smallest_min_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should adjust Q for smallest_min_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::TwistLimitedCone should adjust Q smallest_min_twist back to min_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));

            // adjustment necessary: beyond max_twist
            dQ.setAngleAxis(max_twist, forward_axis);
            expected_adjusted_q = dQ * mid_bend_q;
            dQ.setAngleAxis(max_twist + del, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should adjust Q for above max_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::TwistLimitedCone should adjust Q back to max_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q, EXPANDED_SLOP));

            F32 largest_max_twist = mid_twist + F_PI - del;
            dQ.setAngleAxis(largest_max_twist, forward_axis);
            Q = dQ * mid_bend_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::TwistLimitedCone should adjust Q for largest_max_twist", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::TwistLimitedCone should adjust Q largest_max_twist back to max_twist", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q, EXPANDED_SLOP));
        }

        // test minimizeTwist()
        {
            LLVector3 pivot_axis = LLVector3::x_axis;
            F32 bend_angle = cone_angle + 0.1f;
            F32 twist_angle = max_twist + 0.1f;
            LLQuaternion bend;
            bend.setAngleAxis(bend_angle, pivot_axis);
            LLQuaternion twist;
            twist.setAngleAxis(twist_angle, forward_axis);

            LLQuaternion Q = twist * bend;
            adjusted_q = constraint.minimizeTwist(Q);

            // Note: ElbowConstraint doesn't actually minimizeTwist per-se...
            // instead it assumes all bend is about mPivotAxis...
            // then untwists to middle of twist range
            F32 mid_twist = 0.5f * (max_twist + min_twist);
            LLQuaternion expected_twist;
            expected_twist.setAngleAxis(mid_twist, forward_axis);
            LLQuaternion expected_bend;
            expected_bend.setAngleAxis(bend_angle, pivot_axis);
            LLQuaternion expected_q = expected_twist * expected_bend;
            constexpr F32 MAX_ANGLE_ERROR = 1.0e-3f * F_PI;
            ensure("LLIK::TwistLimitedCone should remove twist", LLQuaternion::almost_equal(adjusted_q, expected_q, MAX_ANGLE_ERROR));
        }
    }

    // DoubleLimitedHinge
	template<> template<>
	void llik_object::test<5>()
	{
        LLVector3 forward_axis = LLVector3::y_axis;
        LLVector3 yaw_axis = LLVector3::z_axis;
        LLVector3 pitch_axis = yaw_axis % forward_axis;
        F32 min_yaw = -0.1f;
        F32 max_yaw = 0.9f * F_PI;
        F32 min_pitch = -F_PI / 6.0f;
        F32 max_pitch = F_PI / 5.0f;
        constexpr F32 EXPANDED_SLOP = 1.0e-2f; // radians, corresponds to about 0.573 degrees

        LLIK::DoubleLimitedHinge constraint(forward_axis, yaw_axis, min_yaw, max_yaw, min_pitch, max_pitch);

        LLQuaternion adjusted_q;

        {
            // no adjustment necessary: yaw
            LLQuaternion min_yaw_q;
            min_yaw_q.setAngleAxis(min_yaw, yaw_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(min_yaw_q);
            ensure("LLIK::DoubleLimitedHinge should not adjust Q=min_yaw", LLQuaternion::almost_equal(adjusted_q, min_yaw_q));

            LLQuaternion max_yaw_q;
            max_yaw_q.setAngleAxis(max_yaw, yaw_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(max_yaw_q);
            ensure("LLIK::DoubleLimitedHinge should not adjust Q=max_yaw", LLQuaternion::almost_equal(adjusted_q, max_yaw_q));

            F32 mid_yaw = 0.5f * (max_yaw + min_yaw);
            LLQuaternion mid_yaw_q;
            mid_yaw_q.setAngleAxis(mid_yaw, yaw_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(mid_yaw_q);
            ensure("LLIK::DoubleLimitedHinge should not adjust Q=mid_yaw", LLQuaternion::almost_equal(adjusted_q, mid_yaw_q));

            // no adjustment necessary: twist
            LLQuaternion dQ;
            dQ.setAngleAxis(min_pitch, pitch_axis);
            LLQuaternion Q = dQ * mid_yaw_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should not adjust Q=min_pitch", LLQuaternion::almost_equal(adjusted_q, Q));

            dQ.setAngleAxis(max_pitch, pitch_axis);
            Q = dQ * mid_yaw_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should not adjust Q=max_pitch", LLQuaternion::almost_equal(adjusted_q, Q, EXPANDED_SLOP));

            F32 mid_pitch = 0.5f * (max_pitch + min_pitch);
            dQ.setAngleAxis(mid_pitch, pitch_axis);
            Q = dQ * mid_yaw_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should not adjust Q=mid_pitch", LLQuaternion::almost_equal(adjusted_q, Q));

            // yes adjustment necessary: too much yaw
            F32 del = 0.01f;
            Q.setAngleAxis(min_yaw - del, yaw_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q below min_yaw", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should clamp Q to min_yaw", LLQuaternion::almost_equal(adjusted_q, min_yaw_q));

            Q.setAngleAxis(max_yaw + del, yaw_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q above max_yaw", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should clamp Q to max_yaw", LLQuaternion::almost_equal(adjusted_q, max_yaw_q));

            F32 smallest_min_yaw = mid_yaw - F_PI + del;
            Q.setAngleAxis(smallest_min_yaw, yaw_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q at smallest_min_yaw", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should clamp smallest_min_yaw to min_yaw", LLQuaternion::almost_equal(adjusted_q, min_yaw_q));

            F32 largest_max_yaw = mid_yaw + F_PI - del;
            Q.setAngleAxis(largest_max_yaw, yaw_axis);
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q at largest_max_yaw", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should clamp largest_max_yaw to max_yaw", LLQuaternion::almost_equal(adjusted_q, max_yaw_q));

            // adjustment necessary: beyond min_pitch
            dQ.setAngleAxis(min_pitch, pitch_axis);
            LLQuaternion expected_adjusted_q = dQ * mid_yaw_q;
            dQ.setAngleAxis(min_pitch - del, pitch_axis);
            Q = dQ * mid_yaw_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q for below min_pitch", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should adjust Q back to min_pitch", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));

            F32 smallest_min_pitch = - 0.5f * F_PI + del;
            dQ.setAngleAxis(smallest_min_pitch, pitch_axis);
            Q = dQ * mid_yaw_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q for smallest_min_pitch", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should adjust Q smallest_min_pitch back to min_pitch",
                LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q, EXPANDED_SLOP));

            // adjustment necessary: beyond max_pitch
            dQ.setAngleAxis(max_pitch, pitch_axis);
            expected_adjusted_q = dQ * mid_yaw_q;
            dQ.setAngleAxis(max_pitch + del, pitch_axis);
            Q = dQ * mid_yaw_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q for above max_pitch", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should adjust Q back to max_pitch", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));

            F32 largest_max_pitch = 0.5f * F_PI - del;
            dQ.setAngleAxis(largest_max_pitch, pitch_axis);
            Q = dQ * mid_yaw_q;
            adjusted_q = constraint.computeAdjustedLocalRot(Q);
            ensure("LLIK::DoubleLimitedHinge should adjust Q for largest_max_pitch", !LLQuaternion::almost_equal(adjusted_q, Q));
            ensure("LLIK::DoubleLimitedHinge should adjust Q largest_max_pitch back to max_pitch", LLQuaternion::almost_equal(adjusted_q, expected_adjusted_q));
        }

        // test minimizeTwist()
        {
            LLVector3 pivot_axis = yaw_axis;
            F32 yaw_angle = max_yaw + 0.1f;
            F32 twist_angle = 1.23f;
            LLQuaternion bend;
            bend.setAngleAxis(yaw_angle, pivot_axis);
            LLQuaternion twist;
            twist.setAngleAxis(twist_angle, forward_axis);

            LLQuaternion Q = twist * bend;
            adjusted_q = constraint.minimizeTwist(Q);

            LLQuaternion expected_q = bend;
            constexpr F32 MAX_ANGLE_ERROR = 1.0e-3f * F_PI;
            ensure("LLIK::DoubleLimitedHinge should remove all twist", LLQuaternion::almost_equal(adjusted_q, expected_q, MAX_ANGLE_ERROR));
        }
    }

    // LLIKConstraintFactory
	template<> template<>
	void llik_object::test<6>()
	{
        LLIKConstraintFactory factory;
        LLIK::Constraint::ptr_t constraint;
        ensure_equals("LLIKConstraintFactory starts empty", factory.getNumConstraints(), 0);
        LLIK::Constraint::Info info;

        constraint = factory.getConstraint(info);
        ensure("LLIKConstraintFactory creates null Constraint when given bad info", ! bool(constraint));
        ensure_equals("LLIKConstraintFactory should remain empty", factory.getNumConstraints(), 0);

        info.mType = LLIK::Constraint::Info::SIMPLE_CONE_CONSTRAINT;
        info.mVectors.push_back(LLVector3::x_axis); // cone_axis
        info.mFloats.push_back(F_PI / 4.0f); // cone_angle
        constraint = factory.getConstraint(info);
        ensure("LLIKConstraintFactory creates non-null Constraint when given good info", bool(constraint));
        ensure_equals("LLIKConstraintFactory should have one Constraint", factory.getNumConstraints(), 1);

        LLIK::Constraint::ptr_t other_constraint = factory.getConstraint(info);
        ensure_equals("LLIKConstraintFactory supplies same constraint for identical info", constraint, other_constraint);

        LLIK::Constraint::Info other_info;
        other_info.mType = LLIK::Constraint::Info::ELBOW_CONSTRAINT;
        other_info.mVectors.push_back(LLVector3::x_axis); // forward_axis
        other_info.mVectors.push_back(LLVector3::y_axis); // pivot_axis
        other_info.mFloats.push_back(0.0f); // min_bend
        other_info.mFloats.push_back(F_PI / 2.0f); // max_bend
        other_info.mFloats.push_back(-F_PI / 4.0f); // min_twist
        other_info.mFloats.push_back(F_PI / 2.0f); // max_twist
        other_constraint = factory.getConstraint(other_info);
        ensure("LLIKConstraintFactory creates non-null Constraint when given good info", bool(other_constraint));
        ensure_not_equals("LLIKConstraintFactory supplies different constraint for different info", constraint, other_constraint);
        ensure_equals("LLIKConstraintFactory should have two Constraints", factory.getNumConstraints(), 2);
    }

    // Simple tests for LLIK::Solver
	template<> template<>
	void llik_object::test<7>()
	{
        LLIKConstraintFactory factory;
        S16 root_joint_id = 7;
        LLIK::Solver solver;
        constexpr F32 ACCEPTABLE_ERROR = 1.0e-3f; // one mm
        solver.setAcceptableError(ACCEPTABLE_ERROR);
        solver.setRootID(root_joint_id);
        ensure_equals("LLIK::Solver::getRootID", solver.getRootID(), root_joint_id);

        // Make a simple skeleton along the y-axis, where each child joint's local_pos
        // is a unit-vector from it's parent's origin.
        //
        //       7---8---9---10---
        //
        //         z
        //         |
        //         +-- y
        //        /
        //       x
        //

        LLIK::Constraint::Info info;
        LLIK::Constraint::ptr_t null_constraint = factory.getConstraint(info);

        constexpr F32 BONE_LENGTH = 1.234f;
        constexpr F32 POS_LENGTH = 2.345f;
        S32 num_joints = 3;
        LLVector3 local_pos = POS_LENGTH * LLVector3::y_axis;
        LLVector3 bone = BONE_LENGTH * LLVector3::y_axis;

        // Note: root joint should always have zero bone length,
        // because IK will target its "end" not its "tip".
        solver.addJoint(root_joint_id, root_joint_id-1, LLVector3::zero, LLVector3::zero, null_constraint);
        S16 joint_id = root_joint_id + 1;

        for (S32 i = 0; i < num_joints; ++i)
        {
            solver.addJoint(joint_id, joint_id-1, local_pos, bone, null_constraint);
            ++joint_id;
        }
        S16 last_joint_id = joint_id - 1;

        F32 reach = solver.computeReach(root_joint_id, last_joint_id).length();
        F32 expected_reach = num_joints * POS_LENGTH + BONE_LENGTH;
        ensure_equals("LLIK::Solver computeReach", reach, expected_reach);

        LLVector3 end_direction = LLVector3::y_axis + 0.1f * LLVector3::z_axis;
        end_direction.normalize();

        // reachable end-effector target
        {
            // Note: FABRIK is fast and accurate for very reachable positions,
            // however it can converge slowly at the reachable edge.
            // Here we test the reachable edge and allow max_error
            // to be 10X ACCEPTABLE_ERROR.
            F32 allowable_error = 10.0f * ACCEPTABLE_ERROR;
            LLIK::Joint::Config config;
            F32 reachable = 0.99f * reach;
            config.setTargetPos(reachable * end_direction);
            LLIK::Solver::joint_config_map_t configs;
            configs.insert({last_joint_id, config});

            F32 max_error = solver.configureAndSolve(configs);
            ensure("LLIK::Solver reachable target sans-constraints should have low error", max_error < allowable_error);
        }

        // unreachable end-effector target
        {
            LLIK::Joint::Config config;
            F32 unreachable = reach + ACCEPTABLE_ERROR;
            config.setTargetPos(unreachable * end_direction);
            LLIK::Solver::joint_config_map_t configs;
            configs.insert({last_joint_id, config});

            F32 max_error = solver.configureAndSolve(configs);
            ensure("LLIK::Solver unreachable target is expected to have high error", max_error > ACCEPTABLE_ERROR);
        }

        // move root so end-effector target is easily reachable again
        {
            LLIK::Joint::Config config;
            F32 unreachable = reach + ACCEPTABLE_ERROR;
            config.setTargetPos(unreachable * end_direction);
            LLIK::Solver::joint_config_map_t configs;
            configs.insert({last_joint_id, config});
            config.setTargetPos(1.0f * LLVector3::y_axis);
            configs.insert({root_joint_id, config});

            //solver.enableDebugIfPossible();
            F32 max_error = solver.configureAndSolve(configs);
            ensure("LLIK::Solver reachable target sans-constraints after moving root", max_error < ACCEPTABLE_ERROR);
        }
	}

    // LLIK::Solver : unconstrained vs constrained
	template<> template<>
	void llik_object::test<8>()
	{
        LLIKConstraintFactory factory;
        S16 joint_id = 0;
        constexpr F32 ACCEPTABLE_ERROR = 3.0e-3f;

        // Consider the following chain of Joints:
        //
        //     .
        //     |
        //    (3)
        //     |
        //     |
        //    (2)   Y
        //     |    |
        //     |    +--X
        //    (1)  /
        //     |  Z
        //    (0)
        //
        // If we set the target for (3) to be a few units out on the +Z axis
        // then a likely unconstrained solution would look like:
        //
        //         _-2.         Y
        //     1.-'    '.       |
        //     |         '.     +--X
        //     0          3    /
        //                |   Z
        //
        // However with suitable constraints we might convince the solution to
        // adopt a more circuitous solution:
        //
        //     1-------2        Y
        //     |       |        |
        //     0       |        +--X
        //             |       /
        //             3--.   Z
        // We will test each case.

        {   // unconstrained
            LLIK::Solver solver;
            solver.setAcceptableError(ACCEPTABLE_ERROR);
            solver.setRootID(joint_id);

            LLIK::Constraint::Info info;
            LLIK::Constraint::ptr_t null_constraint = factory.getConstraint(info);

            // Note: addJoint() API is:
            // Solver::addJoint(joint_id, parent_id, local_pos, bone, constraint)

            // root joint has zero local_pos
            solver.addJoint(joint_id, joint_id-1, 0.0f * LLVector3::y_axis, 1.0f * LLVector3::y_axis, null_constraint);
            ++joint_id;

            solver.addJoint(joint_id, joint_id-1, 1.0f * LLVector3::y_axis, 2.0f * LLVector3::y_axis, null_constraint);
            ++joint_id;
            solver.addJoint(joint_id, joint_id-1, 2.0f * LLVector3::y_axis, 2.0f * LLVector3::y_axis, null_constraint);
            ++joint_id;
            solver.addJoint(joint_id, joint_id-1, 2.0f * LLVector3::y_axis, 1.0f * LLVector3::y_axis, null_constraint);
            ++joint_id;

            S16 last_joint_id = joint_id - 1;

            LLIK::Joint::Config config;
            config.setTargetPos(3.0f * LLVector3::x_axis - 1.0f * LLVector3::y_axis);
            LLIK::Solver::joint_config_map_t configs;
            configs.insert({last_joint_id, config});

            F32 max_error = solver.configureAndSolve(configs);
            ensure("LLIK::Solver reachable target sans-constraints should have low error", max_error < ACCEPTABLE_ERROR);
        }

        {   // constrained
            //     1---2       Y
            //     |   |       |
            //     0   |       +--X
            //        (3)--   /
            //               Z
            LLIK::Solver solver;
            solver.setAcceptableError(ACCEPTABLE_ERROR);
            solver.setRootID(joint_id);

            LLIK::Constraint::Info info;
            F32 del = 0.2f;

            // root joint doesn't move
            LLIK::Constraint::Info info_A;
            LLIK::Constraint::ptr_t null_constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, 0.0f * LLVector3::y_axis, 1.0f * LLVector3::y_axis, null_constraint);
            ++joint_id;

            LLIK::Constraint::Info info_CW;
            info_CW.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info_CW.mVectors.push_back(LLVector3::y_axis); // forward_axis
            info_CW.mVectors.push_back(LLVector3::z_axis); // pivot_axis
            info_CW.mFloats.push_back(-0.5f * F_PI - del); // min_bend
            info_CW.mFloats.push_back(-0.5f * F_PI + del); // max_bend
            LLIK::Constraint::ptr_t right_turn_CW = factory.getConstraint(info_CW);
            solver.addJoint(joint_id, joint_id-1, 1.0f * LLVector3::y_axis, 2.0f * LLVector3::y_axis, right_turn_CW);
            ++joint_id;

            LLIK::Constraint::Info info_C;
            solver.addJoint(joint_id, joint_id-1, 2.0f * LLVector3::y_axis, 2.0f * LLVector3::y_axis, right_turn_CW);
            ++joint_id;

            LLIK::Constraint::Info info_CCW;
            info_CCW.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info_CCW.mVectors.push_back(LLVector3::y_axis); // forward_axis
            info_CCW.mVectors.push_back(LLVector3::z_axis); // pivot_axis
            info_CCW.mFloats.push_back(0.5f * F_PI - del); // min_bend
            info_CCW.mFloats.push_back(0.5f * F_PI + del); // max_bend
            LLIK::Constraint::ptr_t right_turn_CCW = factory.getConstraint(info_CCW);
            solver.addJoint(joint_id, joint_id-1, 2.0f * LLVector3::y_axis, 1.0f * LLVector3::y_axis, right_turn_CCW);
            ++joint_id;

            S16 last_joint_id = joint_id - 1;

            LLIK::Joint::Config config;
            config.setTargetPos(3.0f * LLVector3::x_axis - 1.0f * LLVector3::y_axis);
            LLIK::Solver::joint_config_map_t configs;
            configs.insert({last_joint_id, config});

            //solver.enableDebugIfPossible();
            F32 max_error = solver.configureAndSolve(configs);
            ensure("LLIK::Solver reachable target with constraints should have low error", max_error < ACCEPTABLE_ERROR);
        }
	}

    // IK::Solver multi-chain skeleton
	template<> template<>
	void llik_object::test<9>()
	{
        LLIKConstraintFactory factory;
        LLIK::Constraint::Info info;
        LLIK::Constraint::ptr_t null_constraint = factory.getConstraint(info);

        S16 joint_id = 0;
        LLIK::Solver solver;
        constexpr F32 ACCEPTABLE_ERROR = 1.0e-3f; // one mm
        solver.setAcceptableError(ACCEPTABLE_ERROR);
        solver.setRootID(joint_id);

        // We'll start with a simple skeleton like this: two "arms"
        // joined at a "neck" and a "spine" to the "root",
        // where each joint local_pos/bone is equal length...
        //
        //  <---(9)<--(8)<--(7,4)-->(5)-->(6)--->
        //                    ^
        //                    |
        //                   (3)
        //                    ^
        //                    |
        //                   (2)
        //                    ^
        //                    |
        //                   (1) Y
        //                    ^  |
        //                    |  |
        //                   (0) +----X
        //
        // ... and we'll specify reachable endpoints for (3), (6) and (9)
        // to see if it can reach a solution.

        F32 bone_length = 1.0f;

        // spine
        S32 num_spine_joints = 4;
        for (S32 i = 0; i < num_spine_joints; ++i)
        {
            LLVector3 local_pos = bone_length * LLVector3::y_axis;
            LLVector3 bone = local_pos;
            solver.addJoint(joint_id, joint_id-1, local_pos, bone, null_constraint);
            ++joint_id;
        }

        S16 neck_id = joint_id - 1;
        S16 parent_joint_id = neck_id;

        // right arm
        S32 num_arm_joints = 3;
        {
            // first joint is special
            LLVector3 local_pos = bone_length * LLVector3::y_axis;
            LLVector3 bone = bone_length * LLVector3::x_axis;
            solver.addJoint(joint_id, parent_joint_id, local_pos, bone, null_constraint);
            parent_joint_id = joint_id;
            ++joint_id;
        }
        for (S32 i = 1; i < num_arm_joints; ++i)
        {
            LLVector3 local_pos = bone_length * LLVector3::x_axis;
            LLVector3 bone = local_pos;
            solver.addJoint(joint_id, parent_joint_id, local_pos, bone, null_constraint);
            parent_joint_id = joint_id;
            ++joint_id;
        }
        S16 right_hand_id = joint_id - 1;

        // left-arm
        parent_joint_id = neck_id;
        {
            // first joint is special
            LLVector3 local_pos = bone_length * LLVector3::y_axis;
            LLVector3 bone = - bone_length * LLVector3::x_axis;
            solver.addJoint(joint_id, parent_joint_id, local_pos, bone, null_constraint);
            parent_joint_id = joint_id;
            ++joint_id;
        }
        for (S32 i = 1; i < num_arm_joints; ++i)
        {
            LLVector3 local_pos = - bone_length * LLVector3::x_axis;
            LLVector3 bone = local_pos;
            solver.addJoint(joint_id, parent_joint_id, local_pos, bone, null_constraint);
            parent_joint_id = joint_id;
            ++joint_id;
        }
        S16 left_hand_id = joint_id - 1;

        // These target points represent the "reachable edge":
        // the skeleton can get there but only by extending its arms straight
        // in opposite directions.
        LLVector3 neck_pos = bone_length * LLVector3(2.0f, 2.0f, 0.0f);
        LLVector3 right_hand_pos = bone_length * LLVector3(2.0f, -1.0f, 0.0f);
        LLVector3 left_hand_pos = bone_length * LLVector3(2.0f, 5.0f, 0.0f);

        // build the configs
        LLIK::Joint::Config neck_config, right_hand_config, left_hand_config;
        neck_config.setTargetPos(neck_pos);
        right_hand_config.setTargetPos(right_hand_pos);
        left_hand_config.setTargetPos(left_hand_pos);

        {
            // assemble list of all configs
            LLIK::Solver::joint_config_map_t configs;
            configs.insert({neck_id, neck_config});
            configs.insert({right_hand_id, right_hand_config});
            configs.insert({left_hand_id, left_hand_config});

            // Unfortunately FABRIK can be slow to converge for the reachable edge
            // which is what this scenario presents, so we relax to "allowable_error".
            F32 allowable_error = 0.03f;

            //solver.enableDebugIfPossible();

            F32 max_error = solver.configureAndSolve(configs);
            ensure("LLIK::Solver reachable multi-targets (3) are expected to have low error", max_error < allowable_error);
            F32 error = dist_vec(solver.getJointWorldEndPos(neck_id), neck_pos);
            ensure("LLIK::Solver Neck should reach target", error < ACCEPTABLE_ERROR);
            error = dist_vec(solver.getJointWorldEndPos(right_hand_id), right_hand_pos);
            ensure("LLIK::Solver RightHand should reach target", error < allowable_error);
            error = dist_vec(solver.getJointWorldEndPos(left_hand_id), left_hand_pos);
            ensure("LLIK::Solver LeftHand should reach target", error < allowable_error);
        }
        {
            // assemble list of configs
            // but this time only target the hands but not the neck
            LLIK::Solver::joint_config_map_t configs;
            configs.insert({right_hand_id, right_hand_config});
            configs.insert({left_hand_id, left_hand_config});

            // This time we expect the accuracy to be worse since only two configs
            // are pulling the skeleton into place.
            F32 allowable_error = 0.03f;

            F32 max_error = solver.configureAndSolve(configs);
            ensure("LLIK::Solver reachable multi-targets (2) are expected to have low error", max_error < allowable_error);
            F32 error = dist_vec(solver.getJointWorldEndPos(right_hand_id), right_hand_pos);
            ensure("LLIK::Solver RightHand should reach target", error < allowable_error);
            error = dist_vec(solver.getJointWorldEndPos(left_hand_id), left_hand_pos);
            ensure("LLIK::Solver LeftHand should reach target", error < allowable_error);
        }
    }

#ifdef ENABLE_FAILING_UNIT_TESTS
    // These hard-coded indices and constraints for a simplified two-fingered arm:
    //
    // chest   collar  shoulder   elbow    wrist (5)--(6)--(7)-. index
    //  (0)------(1)------(2)------(3)------(4)---.                         +--Y
    //                                           (8)--(9)--(10)-. ring     /|
    //                                                                    Z X
    constexpr S16 CHEST = 0;
    constexpr S16 COLLAR = 1;
    constexpr S16 SHOULDER = 2;
    constexpr S16 ELBOW = 3;
    constexpr S16 WRIST = 4;
    constexpr S16 INDEX_1 = 5;
    constexpr S16 INDEX_2 = 6;
    constexpr S16 INDEX_3 = 7;
    constexpr S16 RING_1 = 8;
    constexpr S16 RING_2 = 9;
    constexpr S16 RING_3 = 10;

    LLIK::Constraint::Info get_constraint_info(S16 id, const LLVector3& forward_axis)
    {
        LLIK::Constraint::Info info;
        switch (id)
        {
            case CHEST:
            {
                info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mFloats.push_back(0.01f * F_PI); // cone_angle
                info.mFloats.push_back(-0.01f * F_PI); // min_twist
                info.mFloats.push_back(0.01f * F_PI); // max_twist
            }
            break;
            case COLLAR:
            {
                info.mType = LLIK::Constraint::Info::SIMPLE_CONE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mFloats.push_back(0.05f * F_PI); // cone_angle
            }
            break;
            case SHOULDER:
            {
                info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mFloats.push_back(F_PI * 1.0f / 4.0f); // cone_angle
                info.mFloats.push_back(-F_PI * 2.0f / 5.0f); // min_twist
                info.mFloats.push_back(F_PI * 4.0f / 7.0f); // max_twist
            }
            break;
            case ELBOW:
            {
                info.mType = LLIK::Constraint::Info::ELBOW_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mVectors.push_back(LLVector3::z_axis); // pivot_axis
                info.mFloats.push_back(- F_PI * 7.0f / 8.0f); // min_bend
                info.mFloats.push_back(0.0f); // max_bend
                info.mFloats.push_back(-F_PI * 1.0f / 4.0f); // min_twist
                info.mFloats.push_back(F_PI * 1.0f / 4.0f); // max_twist
            }
            break;
            case WRIST:
            {
                info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mFloats.push_back(F_PI * 1.0f / 5.0f); // cone_angle
                info.mFloats.push_back(-0.05f); // min_twist
                info.mFloats.push_back(0.05f); // max_twist
            }
            break;
            // INDEX
            case INDEX_1:
            {
                info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mVectors.push_back(LLVector3::z_axis); // up_axis
                info.mFloats.push_back(-0.05f * F_PI); // min_yaw
                info.mFloats.push_back(0.05f * F_PI); // max_yaw
                info.mFloats.push_back(0.0f); // min_pitch
                info.mFloats.push_back(F_PI * 4.0f / 9.0f); // max_pitch
            }
            break;
            case INDEX_2:
            {
                info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mVectors.push_back(-LLVector3::x_axis); // pivot_axis
                info.mFloats.push_back(0.0f); // min_bend
                info.mFloats.push_back(0.5f * F_PI); // max_bend
            }
            break;
            case INDEX_3:
            {
                info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mVectors.push_back(-LLVector3::x_axis); // pivot_axis
                info.mFloats.push_back(0.0f); // min_bend
                info.mFloats.push_back(0.4f * F_PI); // max_bend
            }
            break;
            // RING
            case RING_1:
            {
                info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mVectors.push_back(LLVector3::z_axis); // up_axis
                info.mFloats.push_back(-0.05f * F_PI); // min_yaw
                info.mFloats.push_back(0.05f * F_PI); // max_yaw
                info.mFloats.push_back(0.0f); // min_pitch
                info.mFloats.push_back(F_PI * 4.0f / 9.0f); // max_pitch
            }
            break;
            case RING_2:
            {
                info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mVectors.push_back(-LLVector3::x_axis); // pivot_axis
                info.mFloats.push_back(0.0f); // min_bend
                info.mFloats.push_back(0.5f * F_PI); // max_bend
            }
            break;
            case RING_3:
            {
                info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
                info.mVectors.push_back(forward_axis); // forward_axis
                info.mVectors.push_back(-LLVector3::x_axis); // pivot_axis
                info.mFloats.push_back(0.0f); // min_bend
                info.mFloats.push_back(0.4f * F_PI); // max_bend
            }
            break;
            default:
            break;
        }; // switch
        return info;
    }

    // DISABLED: we're not trying to handle fingers yet
    // LLIK::Solver : simple hand with two fingers
	template<> template<>
	void llik_object::test<10>()
	{
        // We create a simplified two-finger arm:
        //
        // chest   collar  shoulder   elbow    wrist (5)--(6)--(7)-. index
        //  (0)------(1)------(2)------(3)------(4)---.                         +--Y
        //                                           (8)--(9)--(10)-. ring     /|
        //                                                                    Z X
        //
        // We try to bend the elbow near PI/2 by setting fingertip targets
        // in the positive X-direction:
        //
        // chest   collar  shoulder   elbow
        //  (0)------(1)------(2)------(3)
        //                              |
        //                              |
        //                              |
        //                             (4)
        //           +--Y               |
        //          /|               (8)|(5)
        //         Z X                |   |
        //                           (9) (6)
        //                            |   |
        //                          (10) (7)
        //                            |   |
        //
        std::vector<LLVector3> local_positions;
        std::vector<LLVector3> bones;

        // mChest (the root for this test)
        local_positions.push_back(LLVector3::zero);
        bones.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        // mCollarLeft
        local_positions.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        bones.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        // mShoulderLeft
        local_positions.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        bones.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        // mElbowLeft
        local_positions.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        bones.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        // mWristLeft
        local_positions.push_back(LLVector3(0.0f, 1.0f, 0.0f));
        bones.push_back(LLVector3(0.0f, 0.5f, 0.0f));

        // mHandIndex1Left
        local_positions.push_back(LLVector3(0.2f, 0.5f, 0.0f));
        bones.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        // mHandIndex2Left
        local_positions.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        bones.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        // mHandIndex3Left
        local_positions.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        bones.push_back(LLVector3(0.0f, 0.3f, 0.0f));

        // mHandRing1Left
        local_positions.push_back(LLVector3(-0.2f, 0.5f, 0.0f));
        bones.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        // mHandRing2Left
        local_positions.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        bones.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        // mHandRing3Left
        local_positions.push_back(LLVector3(0.0f, 0.3f, 0.0f));
        bones.push_back(LLVector3(0.0f, 0.3f, 0.0f));

        LLIKConstraintFactory factory;
        LLIK::Constraint::Info info;
        LLIK::Constraint::ptr_t null_constraint = factory.getConstraint(info);

        S16 joint_id = 0;
        LLIK::Solver solver;
        constexpr F32 ACCEPTABLE_ERROR = 1.0e-3f; // one mm
        solver.setAcceptableError(ACCEPTABLE_ERROR);
        solver.setRootID(joint_id);
        for (S32 id = 0; id < (S16)(local_positions.size()); ++id)
        {
            S16 parent_id = id - 1;
            if (id == INDEX_1 || id == RING_1)
            {
                parent_id = WRIST;
            }
            LLIK::Constraint::Info info = get_constraint_info(id, local_positions[id]);
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(id, parent_id, local_positions[id], bones[id], constraint);
        }
        std::vector<S16> fingertip_indices = { INDEX_3, RING_3 };

        // measure the initial offsets to fingers from elbow_tip
        LLVector3 elbow_tip = solver.getJointWorldTipPos(ELBOW);
        std::vector<LLVector3> finger_offsets;
        for (auto id : fingertip_indices)
        {
            finger_offsets.push_back(solver.getJointWorldEndPos(id) - elbow_tip);
        }

        // compute finger_target_positions
        std::vector<LLVector3> finger_target_positions;
        LLQuaternion Q;
        Q.setAngleAxis(-0.5f * F_PI, LLVector3::z_axis);
        for (size_t i = 0; i < finger_offsets.size(); ++i)
        {
            finger_target_positions.push_back(elbow_tip + finger_offsets[i] * Q);
        }

        // build the configs
        LLIK::Solver::joint_config_map_t configs;
        for (size_t i = 0; i < fingertip_indices.size(); ++i)
        {
            LLIK::Joint::Config config;
            config.setTargetPos(finger_target_positions[i]);
            configs.insert({fingertip_indices[i], config});
        }

        // solve
        //solver.enableDebugIfPossible();
        solver.configureAndSolve(configs);

        // check results
        // Note; this test does not quite reach ACCEPTABLE_ERROR after 16 iterations
        // however it gets close
        F32 allowable_error = 0.033f;
        for (size_t i = 0; i < fingertip_indices.size(); ++i)
        {
            F32 error = dist_vec(solver.getJointWorldEndPos(fingertip_indices[i]), finger_target_positions[i]);
            ensure("LLIK::Solver finger should reach target", error < allowable_error);
        }
    }
#endif // ENABLE_FAILING_UNIT_TESTS

    void build_skeleton_arm(LLIKConstraintFactory& factory, LLIK::Solver& solver, bool with_fingers=true)
	{
        // This builds arm+hand+fingers skeleton based on the default SL avatar.

        //
        //
        //            Shoulder        Wrist        (10)--(11)--(12)- Index1,2,3
        //   Collar (3)---(4)---(5)---(6)--        (7)---(8)---(9)-- Middle1,2,3
        //          /         Elbow                (13)--(14)--(15)- Ring1,2,3
        //  Chest (2)                              (16)--(17)--(18)- Pinky1,2,3
        //         |                               (19)--(20)--(21)- Thumb1,2,3
        //  Torso (1)
        //         |
        // Pelvis (0)
        //

        S16 joint_id = 0;
        solver.setRootID(joint_id);

        // Pelvis
        {
            LLVector3 local_position = LLVector3::zero;
            LLVector3 bone(0.0f,0.0f,0.08757f);
            LLIK::Constraint::ptr_t null_constraint = factory.getConstraint(LLIK::Constraint::Info());
            solver.addJoint(joint_id, -1, local_position, bone, null_constraint);
        }
        ++joint_id;

        // Torso
        {
            LLVector3 local_position(0.0f,0.0f,0.08757f);
            LLVector3 bone(-0.014445f,0.0f,0.213712f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.0628319f); // cone_angle
            info.mFloats.push_back(-0.0628319f); // min_twist
            info.mFloats.push_back(0.0628319f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Chest
        {
            LLVector3 local_position(-0.015318f,0.0f,0.213712f);
            LLVector3 bone(-0.01f,0.0f,0.2151);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.0628319f); // cone_angle
            info.mFloats.push_back(-0.0628319f); // min_twist
            info.mFloats.push_back(0.0628319f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Collar
        {
            LLVector3 local_position(-0.021f,0.123583f,0.165f);
            LLVector3 bone(0.0f,0.10349f,0.0f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::SIMPLE_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.15708f); // cone_angle
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Shoulder
        {
            LLVector3 local_position(0.0f,0.10349f,0.0f);
            LLVector3 bone(0.0f,0.260152f,0.0f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            //info.mFloats.push_back(0.785398f); // cone_angle
            info.mFloats.push_back(1.5f); // cone_angle
            info.mFloats.push_back(-1.25664f); // min_twist
            info.mFloats.push_back(1.7952f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Elbow
        {
            LLVector3 local_position(0.0f,0.260152f,0.0f);
            LLVector3 bone(0.0f,0.2009f,0.0f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::ELBOW_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // pivot_axis
            info.mFloats.push_back(-2.74889f); // min_bend
            info.mFloats.push_back(0.0f); // max_bend
            info.mFloats.push_back(-0.785398f); // min_twist
            info.mFloats.push_back(2.35619f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        //S16 ELBOW_INDEX = joint_id;
        ++joint_id;

        // Wrist
        {
            LLVector3 local_position(0.0f,0.2009f,0.0f);
            LLVector3 bone(0.01274f,0.09898f,0.0147f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.628318f); // cone_angle
            info.mFloats.push_back(-0.05f); // min_twist
            info.mFloats.push_back(0.05f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        S16 WRIST_INDEX = joint_id;
        ++joint_id;

        if (!with_fingers)
        {
            return;
        }

        // Middle1
        {
            LLVector3 local_position(0.01274f,0.09898f,0.0147f);
            LLVector3 bone(-0.00098f,0.0392f,-0.00588f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.15708f); // min_yaw
            info.mFloats.push_back(0.15708f); // max_yaw
            info.mFloats.push_back(0.0f); // min_pitch
            info.mFloats.push_back(0.942478f); // max_pitch
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, WRIST_INDEX, local_position, bone, constraint);
        }
        ++joint_id;

        // Middle2
        {
            LLVector3 local_position(-0.00098f,0.0392f,-0.00588f);
            LLVector3 bone(-0.00098,0.04802,-0.00784);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.5708f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Middle3
        {
            LLVector3 local_position(-0.00098f,0.04802f,-0.00784f);
            LLVector3 bone(-0.00196f,0.03234f,-0.00588f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.25664f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        //S16 MIDDLE_END_INDEX = joint_id;
        ++joint_id;

        // Index1
        {
            LLVector3 local_position(0.03724f,0.09506f,0.0147f);
            LLVector3 bone(0.01666f,0.03528f,-0.00588f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.15708f); // min_yaw
            info.mFloats.push_back(0.15708f); // max_yaw
            info.mFloats.push_back(0.0f); // min_pitch
            info.mFloats.push_back(1.39626f); // max_pitch
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, WRIST_INDEX, local_position, bone, constraint);
        }
        ++joint_id;

        // Index2
        {
            LLVector3 local_position(0.01666f,0.03528f,-0.00588f);
            LLVector3 bone(0.01372f,0.03136f,-0.00588f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.5708f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Index3
        {
            LLVector3 local_position(0.01372f,0.03136f,-0.00588f);
            LLVector3 bone(0.01078f,0.0245f,-0.00392f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.25664f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        //S16 INDEX_END_INDEX = joint_id;
        ++joint_id;

        // Ring1
        {
            LLVector3 local_position(-0.0098f,0.09702f,0.00882f);
            LLVector3 bone(-0.01274f,0.03724f,-0.00784f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.15708f); // min_yaw
            info.mFloats.push_back(0.15708f); // max_yaw
            info.mFloats.push_back(0.0f); // min_pitch
            info.mFloats.push_back(1.39626f); // max_pitch
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, WRIST_INDEX, local_position, bone, constraint);
        }
        ++joint_id;

        // Ring2
        {
            LLVector3 local_position(-0.01274f,0.03724f,-0.00784f);
            LLVector3 bone(-0.01274f,0.0392f,-0.00882f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.5708f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Ring3
        {
            LLVector3 local_position(-0.01274f,0.0392f,-0.00882f);
            LLVector3 bone(-0.0098f,0.02744f,-0.00588f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.25664f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        //S16 RING_END_INDEX = joint_id;
        ++joint_id;

        // Pinky1
        {
            LLVector3 local_position(-0.03038f,0.0931f,0.00294f);
            LLVector3 bone(-0.02352f,0.0245f,-0.00588f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.15708f); // min_yaw
            info.mFloats.push_back(0.15708f); // max_yaw
            info.mFloats.push_back(0.0f); // min_pitch
            info.mFloats.push_back(1.39626f); // max_pitch
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, WRIST_INDEX, local_position, bone, constraint);
        }
        ++joint_id;

        // Pinky2
        {
            LLVector3 local_position(-0.02352f,0.0245f,-0.00588f);
            LLVector3 bone(-0.0147f,0.01764f,-0.00392f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.5708f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Pinky3
        {
            LLVector3 local_position(-0.0147f,0.01764f,-0.00392f);
            LLVector3 bone(-0.01274f,0.01568f,-0.00392f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 pivot_axis = LLVector3::z_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.25664f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        //S16 PINKY_END_INDEX = joint_id;
        ++joint_id;

        // Thumb1
        {
            LLVector3 local_position(0.03038f,0.02548f,0.00392f);
            LLVector3 bone(0.02744f,0.03136f,-0.00098f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mVectors.push_back(LLVector3(1.0f, -1.0f, 1.0f)); // up_axis
            info.mFloats.push_back(-0.15708f); // min_yaw
            info.mFloats.push_back(0.15708f); // max_yaw
            info.mFloats.push_back(-0.1f); // min_pitch
            info.mFloats.push_back(0.785398f); // max_pitch
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, WRIST_INDEX, local_position, bone, constraint);
        }
        ++joint_id;

        // Thumb2
        {
            LLVector3 local_position(0.02744f,0.03136f,-0.00098f);
            LLVector3 bone(0.02254f,0.03038f,-0.00098f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 up_axis = LLVector3(1.0f, -1.0f, 1.0f);
            LLVector3 pivot_axis = up_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.942478f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Thumb3
        {
            LLVector3 local_position(0.02254f,0.03038f,-0.00098f);
            LLVector3 bone(0.0147f,0.0245f,0.0f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            LLVector3 forward_axis = local_position;
            LLVector3 up_axis = LLVector3(1.0f, -1.0f, 1.0f);
            LLVector3 pivot_axis = up_axis % forward_axis;
            info.mVectors.push_back(forward_axis);
            info.mVectors.push_back(pivot_axis);
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(1.25664f); // max_bend
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        //S16 THUMB_END_INDEX = joint_id;
    }

    // Troubleshoot failing arm IK
    // LLIK::Solver : reachable elbow position
	template<> template<>
	void llik_object::test<11>()
    {
        LLIKConstraintFactory factory;

        LLIK::Solver solver;
        constexpr F32 ACCEPTABLE_ERROR = 1.0e-3f; // one mm
        solver.setAcceptableError(ACCEPTABLE_ERROR);

        bool with_fingers = false;
        build_skeleton_arm(factory, solver, with_fingers);
        constexpr S16 WRIST_INDEX = 6;
        solver.addWristID(WRIST_INDEX);

        // here are some potentially bad targets for constrained left arm:
        LLIK::Joint::Config config;
        //LLVector3 target_position(-0.139651f,  0.25808f,  0.191372f);
        //LLVector3 target_position(-0.122246f,  0.258118f, 0.179148f);
        //LLVector3 target_position(-0.103941f,  0.25815f,  0.168989f);
        //LLVector3 target_position(-0.0846756f, 0.258176f, 0.160799f);
        //LLVector3 target_position(-0.0643341f, 0.258197f, 0.154587f);
        LLVector3 target_position(-0.0434687f, 0.258212f, 0.150573f);
        //LLVector3 target_position(-0.0222741f, 0.25822f,  0.148794f);

        F32 EFFECTOR_NORMAL = 1.7f;
        target_position *= EFFECTOR_NORMAL;

        config.setTargetPos(target_position);

        // build the configs
        LLIK::Solver::joint_config_map_t configs;
        constexpr S16 ELBOW_INDEX = 5;
        configs.insert({ELBOW_INDEX, config});

        // solve
        //solver.enableDebugIfPossible();
        solver.configureAndSolve(configs);
        F32 error = dist_vec(solver.getJointWorldEndPos(ELBOW_INDEX), target_position);
        ensure("LLIK::Solver elbow should reach target", error < ACCEPTABLE_ERROR);
    }

#ifdef ENABLE_FAILING_UNIT_TESTS
    // LLIK::Solver : wrist position and fingers
    // DISABLED: we're not trying to handle fingers yet
	template<> template<>
	void llik_object::test<12>()
    {
        LLIKConstraintFactory factory;

        LLIK::Solver solver;
        constexpr F32 ACCEPTABLE_ERROR = 1.0e-3f; // one mm
        solver.setAcceptableError(ACCEPTABLE_ERROR);

        build_skeleton_arm(factory, solver);

        //std::vector<S16> target_indices = { ELBOW_INDEX, MIDDLE_END_INDEX, INDEX_END_INDEX, RING_END_INDEX, PINKY_END_INDEX, THUMB_END_INDEX };
        //std::vector<S16> target_indices = { 5, 9, 12, 15, 18, 21 };
        //std::vector<S16> target_indices = { MIDDLE_END_INDEX, INDEX_END_INDEX, RING_END_INDEX, PINKY_END_INDEX, THUMB_END_INDEX };
        std::vector<S16> target_indices = { 9, 12, 15, 18, 21 };
        std::vector<LLVector3> finger_target_positions;

        // Note: these targets are attempts to create reasonably reachable positions
        // that cause the fingers to clench a little bit.  With constrained fingers
        // it is very easy to supply an unreachable configuration and ATM the IK
        // system does not do very well at finding reasonable compromise solutions
        // for such: instead of all fingers getting as close as possible with no
        // broken fingers, typically a finger or two will end up askanse such that
        // it looks painfully broken.
        //finger_target_positions.push_back(LLVector3(0.049f,0.575f,0.483f)); // elbow
        //finger_target_positions.push_back(LLVector3(0.13f,0.545f,0.495f)); // wrist
        finger_target_positions.push_back(LLVector3(0.185f,0.52f,0.385f)); // middle
        finger_target_positions.push_back(LLVector3(0.17f,0.48f,0.41f)); // index
        finger_target_positions.push_back(LLVector3(0.18f,0.56f,0.4f)); // ring
        finger_target_positions.push_back(LLVector3(0.15f,0.62f,0.425f)); // pinky
        finger_target_positions.push_back(LLVector3(0.10f,0.44f,0.42f)); // thumb

        // build the configs
        LLIK::Solver::joint_config_map_t configs;
        for (size_t i = 0; i < target_indices.size(); ++i)
        {
            LLIK::Joint::Config config;
            config.setTargetPos(finger_target_positions[i]);
            configs.insert({target_indices[i], config});
        }

        // solve
        //solver.enableDebugIfPossible();
        solver.configureAndSolve(configs);

        // check results
        // Note; this test does not quite reach ACCEPTABLE_ERROR after 16 iterations
        // however it gets close
        F32 allowable_error = 0.044f;
        for (size_t i = 0; i < target_indices.size(); ++i)
        {
            F32 error = dist_vec(solver.getJointWorldEndPos(target_indices[i]), finger_target_positions[i]);
            ensure("LLIK::Solver avatar joint should reach target", error < allowable_error);
        }
    }
#endif // ENABLE_FAILING_UNIT_TESTS

    void build_skeleton_with_head_and_arm(LLIKConstraintFactory& factory, LLIK::Solver& solver)
	{
        // This builds spine, head, and left arm based on the default SL avatar.
        //
        //   Head (4)       Shoulder   Wrist
        //         | (5)---(6)---(7)---(8)--
        //   Neck (3)/Collar      Elbow
        //         |/
        //  Chest (2)
        //         |
        //  Torso (1)
        //         |
        // Pelvis (0)
        //

        S16 joint_id = 0;
        solver.setRootID(joint_id);

        // Pelvis
        {
            LLVector3 local_position = LLVector3::zero;
            LLVector3 bone(0.0f,0.0f,0.08757f);
            LLIK::Constraint::ptr_t null_constraint = factory.getConstraint(LLIK::Constraint::Info());
            solver.addJoint(joint_id, -1, local_position, bone, null_constraint);
        }
        ++joint_id;

        // Torso
        {
            LLVector3 local_position(0.0f,0.0f,0.08757f);
            LLVector3 bone(-0.014445f,0.0f,0.213712f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.0628319f); // cone_angle
            info.mFloats.push_back(-0.0628319f); // min_twist
            info.mFloats.push_back(0.0628319f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Chest
        {
            LLVector3 local_position(-0.015318f,0.0f,0.213712f);
            LLVector3 bone(-0.01f,0.0f,0.2151);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.0628319f); // cone_angle
            info.mFloats.push_back(-0.0628319f); // min_twist
            info.mFloats.push_back(0.0628319f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        S16 CHEST_ID = joint_id;
        ++joint_id;

        // Neck
        {
            LLVector3 local_position(-0.01f, 0.0, 0.251f);
            LLVector3 bone(0.0f, 0.0f, 0.077f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.0628319f); // cone_angle
            info.mFloats.push_back(-0.0628319f); // min_twist
            info.mFloats.push_back(0.0628319f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Head
        {
            LLVector3 local_position(0.0f, 0.0f, 0.076);
            LLVector3 bone(0.0f, 0.0f,0.079f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.0628319f); // cone_angle
            info.mFloats.push_back(-0.0628319f); // min_twist
            info.mFloats.push_back(0.0628319f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Collar
        {
            LLVector3 local_position(-0.021f,0.123583f,0.165f);
            LLVector3 bone(0.0f,0.10349f,0.0f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::SIMPLE_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.15708f); // cone_angle
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, CHEST_ID, local_position, bone, constraint);
        }
        ++joint_id;

        // Shoulder
        {
            LLVector3 local_position(0.0f,0.10349f,0.0f);
            LLVector3 bone(0.0f,0.260152f,0.0f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            //info.mFloats.push_back(0.785398f); // cone_angle
            info.mFloats.push_back(1.5f); // cone_angle
            info.mFloats.push_back(-0.5f * F_PI); // min_twist
            info.mFloats.push_back(0.5f * F_PI); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        ++joint_id;

        // Elbow
        {
            LLVector3 local_position(0.0f,0.260152f,0.0f);
            LLVector3 bone(0.0f,0.2009f,0.0f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::ELBOW_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // pivot_axis
            info.mFloats.push_back(-2.74889f); // min_bend
            info.mFloats.push_back(0.0f); // max_bend
            info.mFloats.push_back(-0.785398f); // min_twist
            info.mFloats.push_back(2.35619f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        //S16 ELBOW_INDEX = joint_id;
        ++joint_id;

        // Wrist
        {
            LLVector3 local_position(0.0f,0.2009f,0.0f);
            LLVector3 bone(0.01274f,0.09898f,0.0147f);
            LLIK::Constraint::Info info;
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(local_position); // forward_axis
            info.mFloats.push_back(0.628318f); // cone_angle
            info.mFloats.push_back(-0.05f); // min_twist
            info.mFloats.push_back(0.05f); // max_twist
            LLIK::Constraint::ptr_t constraint = factory.getConstraint(info);
            solver.addJoint(joint_id, joint_id-1, local_position, bone, constraint);
        }
        S16 WRIST_ID = joint_id;

        // HACK: whitelist of sub-bases
        std::set<S16> ids;
        ids.insert(CHEST_ID);
        ids.insert(WRIST_ID);
        solver.setSubBaseIds(ids);
    }

    // Troubleshoot failing arm IK
    // LLIK::Solver : reachable wrist position and orientation
	template<> template<>
	void llik_object::test<13>()
    {
        LLIKConstraintFactory factory;

        LLIK::Solver solver;
        constexpr F32 ACCEPTABLE_ERROR = 2.0e-2f;
        solver.setAcceptableError(ACCEPTABLE_ERROR);

        build_skeleton_with_head_and_arm(factory, solver);
        constexpr U16 WRIST_INDEX = 8;
        solver.addWristID(WRIST_INDEX);

        // If you identify a position+orientation where the viewer IK fails
        // update them here, run the test, and animate the results to see
        // where it fails.
        LLVector3 target_position(0.00834371f, 0.49807f, 0.470742f);
        LLQuaternion target_orientation(-0.00999829f, 0.0f, -0.0167486f, 0.99981f);

        LLIK::Joint::Config config;
        config.setTargetPos(target_position);
        config.setTargetRot(target_orientation);

        // build the configs
        LLIK::Solver::joint_config_map_t configs;
        configs.insert({WRIST_INDEX, config});

        // solve
        //solver.enableDebugIfPossible();
        solver.configureAndSolve(configs);

        LLVector3 actual_position = solver.getJointWorldEndPos(WRIST_INDEX);
        F32 position_error = dist_vec(target_position, actual_position);
        LLQuaternion actual_orientation = solver.getJointWorldRot(WRIST_INDEX);
        ensure("LLIK::Solver wrist should reach target position", position_error < ACCEPTABLE_ERROR);
        constexpr F32 MIN_ANGLE_ERROR = 0.005f * F_PI;
        ensure("LLIK::Solver wrist should reach target orientation", LLQuaternion::almost_equal(target_orientation, actual_orientation, MIN_ANGLE_ERROR));
    }
}
