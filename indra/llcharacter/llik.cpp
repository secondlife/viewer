/**
 * @file llik.cpp
 * @brief Implementation of LLIK::Solver class and related helpers.
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <limits>

#include "llik.h"

#include "llmath.h"
#include "v3math.h"
#include "llquaternion.h"


#ifdef DEBUG_LLIK_UNIT_TESTS
    // If there is a error in the IK Solver logic you will probably not be able
    // to find it by code inspection or by looking at the final output of the
    // Solver.  You won't be able to make sense out of debug printouts of
    // positions and orientations of the various joints: there will just be too
    // much information in an obscure format.  The only reliable way to zero in
    // the error is to write a unit-test that demonstrates the error and then
    // render the skeleton state at each step of the Solver's iterations so you
    // can visually spot the exact moment where the data goes wrong.
    //
    // When DEBUG_LLIK_UNIT_TESTS is defined the macros here will print out the
    // the Joint data at each step in a format that can be pasted into a Python
    // file for visualization of the Solver's process.  For more info about how
    // to debug unit tests please read the instructions in llik_test.cpp.

    // HACK: gDebugEnabled is used to propagate DEBUG status from LLIK::Solver
    // to the LLIK::Joint context.  When debugging is enabled an properly
    // configured LLIK::Solver will print out its solution data into a
    // python-esq variable which can be pasted into a python script to plot the
    // results.  See comments in llik unit test code for instructions about how
    // to plot the data.
    bool gDebugEnabled = false;
    bool gConfigLogged = false;
    std::string gPhase = "";
    std::string gContext = "";

    #define DEBUG_SET_PHASE(PHASE) if (gDebugEnabled && gPhase != #PHASE) { gPhase = #PHASE; }
    #define DEBUG_SET_CONTEXT(CONTEXT) if (gDebugEnabled && gContext != #CONTEXT) { \
        gContext = #CONTEXT; \
        std::cout << "    ('context','" << gPhase << ":" << gContext << "')," << std::endl; }
    #define DEBUG_LOG_EVENT if (gDebugEnabled) { std::cout << "    "; dumpState(); std::cout << "," << std::endl; }
    #define DEBUG_LOG_EVENT_DETAIL(DETAIL) if (gDebugEnabled) { \
        std::cout << "    ('context','" << gPhase << ":" << gContext << ":" << #DETAIL << "'),\n"; \
        std::cout << "    "; dumpState(); std::cout << "," << std::endl; }
#else
    // When DEBUG_LLIK_UNIT_TESTS is NOT enabled the debug macros resolve to
    // no-op code, which a smart compiler can optimize out of the final byte code.
    #define NO_OP_MACRO do {} while(0)
    #define DEBUG_SET_PHASE(PHASE) NO_OP_MACRO
    #define DEBUG_SET_CONTEXT(CONTEXT) NO_OP_MACRO
    #define DEBUG_LOG_EVENT NO_OP_MACRO
    #define DEBUG_LOG_EVENT_DETAIL(DETAIL) NO_OP_MACRO
#endif


F32 remove_multiples_of_two_pi(F32 angle)
{
    // utility function for truncating angle to range: [0, F_TWO_PI)
    return angle - F_TWO_PI * (S32)(angle / F_TWO_PI);
}

void compute_angle_limits(F32& min_angle, F32& max_angle)
{
    // utility function for clamping angle limits in range [-PI, PI]
    // Note: arguments are passed by reference and modified as side-effect
    max_angle = remove_multiples_of_two_pi(max_angle);
    if (max_angle > F_PI)
    {
        max_angle -= F_TWO_PI;
    }
    min_angle = remove_multiples_of_two_pi(min_angle);
    if (min_angle > F_PI)
    {
        min_angle -= F_TWO_PI;
    }
    if (min_angle > max_angle)
    {
        F32 temp = min_angle;
        min_angle = max_angle;
        max_angle = temp;
    }
}

F32 compute_clamped_angle(F32 angle, F32 min_angle, F32 max_angle)
{
    // utility function for clamping angle between two limits
    // Consider angle limits: min_angle and max_angle
    // with axis out of the page.  There exists an "invalid bisector"
    // angle which splits the invalid zone between the that which is
    // closest to mMinBend or mMaxBend.
    //
    //                max_angle
    //                  \
    //                   \
    //                    \
    //                    (o)--------> 0
    //                 .-'  \
    //              .-'      \
    //           .-'          \
    // invalid_bisector       min_angle
    //
    F32 invalid_bisector = max_angle + 0.5 * (F_TWO_PI - (max_angle - min_angle));
    if ((angle > max_angle && angle < invalid_bisector)
            || angle < invalid_bisector - F_TWO_PI)
    {
        angle = max_angle;
    }
    else
    {
        angle = min_angle;
    }
    return angle;
}

void LLIK::Joint::Config::setLocalPos(const LLVector3& pos)
{
    mLocalPos = pos;
    mFlags |= FLAG_LOCAL_POS;
}

void LLIK::Joint::Config::setLocalRot(const LLQuaternion& rot)
{
    mLocalRot = rot;
    mLocalRot.normalize();
    mFlags |= FLAG_LOCAL_ROT;
}

void LLIK::Joint::Config::setTargetPos(const LLVector3& pos)
{
    mTargetPos = pos;
    mFlags |= FLAG_TARGET_POS;
}

void LLIK::Joint::Config::setTargetRot(const LLQuaternion& rot)
{
    mTargetRot = rot;
    mTargetRot.normalize();
    mFlags |= FLAG_TARGET_ROT;
}

void LLIK::Joint::Config::updateFrom(const Config& other_config)
{
    if (mFlags == other_config.mFlags)
    {
        // other_config updates everything
        *this = other_config;
    }
    else
    {
        // find and apply all parameters in other_config
        if (other_config.hasLocalPos())
        {
            setLocalPos(other_config.getLocalPos());
        }
        if (other_config.hasLocalRot())
        {
            setLocalRot(other_config.getLocalRot());
        }
        if (other_config.hasTargetPos())
        {
            setTargetPos(other_config.getTargetPos());
        }
        if (other_config.hasTargetRot())
        {
            setTargetRot(other_config.getTargetRot());
        }
        if (other_config.constraintIsDisabled())
        {
            disableConstraint();
        }
    }
}

std::string LLIK::Constraint::Info::getString() const
{
    std::ostringstream s;
    s << mType;
    for (const auto& v : mVectors)
    {
        s << "," << v;
    }
    for (F32 f : mFloats)
    {
        s << "," << f;
    }
    return s.str();
}

bool LLIK::Constraint::enforce(Joint& joint) const
{
    const LLQuaternion& local_rot = joint.getLocalRot();
    LLQuaternion adjusted_local_rot = computeAdjustedLocalRot(local_rot);
    if (! LLQuaternion::almost_equal(adjusted_local_rot, local_rot))
    {
        joint.setLocalRot(adjusted_local_rot);
        return true;
    }
    return false;
}

LLQuaternion LLIK::Constraint::minimizeTwist(const LLQuaternion& joint_local_rot) const
{
    // Default behavior of minimizeTwist() is to compute
    // the shortest rotation that produces the same swing.
    LLVector3 joint_forward = mForward * joint_local_rot;
    LLVector3 swing_axis = mForward % joint_forward;
    constexpr F32 MIN_AXIS_LENGTH = 1.0e-5f;
    LLQuaternion new_local_rot = LLQuaternion::DEFAULT;
    if (swing_axis.length() > MIN_AXIS_LENGTH)
    {
        F32 swing_angle = std::acos(mForward * joint_forward);
        new_local_rot.setAngleAxis(swing_angle, swing_axis);
    }
    return new_local_rot;
}

LLIK::SimpleCone::SimpleCone(const LLVector3& forward, F32 max_angle)
{
    mForward = forward;
    mForward.normalize();
    max_angle = std::abs(max_angle);
    mCosConeAngle = std::cos(max_angle);
    mSinConeAngle = std::sin(max_angle);
    mType = Info::SIMPLE_CONE_CONSTRAINT;
}

LLQuaternion LLIK::SimpleCone::computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const
{
    LLVector3 joint_forward = mForward * joint_local_rot;
    F32 forward_component = joint_forward * mForward;
    if (forward_component < mCosConeAngle)
    {
        // the joint's version of mForward lies outside the cone
        // so we project it onto the surface of the cone...
        //
        // projection               = (forward_part)         + (orthogonal_part)
        LLVector3 perp = joint_forward - forward_component * mForward;
        perp.normalize();
        LLVector3 new_joint_forward = mCosConeAngle * mForward + mSinConeAngle * perp;

        // ... then compute the adjusted rotation
        LLQuaternion adjustment;
        adjustment.shortestArc(joint_forward, new_joint_forward);
        LLQuaternion adjusted_local_rot = joint_local_rot * adjustment;
        adjusted_local_rot.normalize();
        return adjusted_local_rot;
    }
    else
    {
        return joint_local_rot;
    }
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::SimpleCone::dumpConfig() const
{
    F32 angle = std::atan2(mSinConeAngle, mCosConeAngle);
    LL_INFOS("debug") << "{'type':'SimpleCone'"
        << ",'forward':(" << mForward.mV[VX] << "," << mForward.mV[VY] << "," << mForward.mV[VZ] << ")"
        << ",'cone_angle':" << angle << ")}" << LL_ENDL;
}
#endif

LLIK::TwistLimitedCone::TwistLimitedCone(const LLVector3& forward, F32 cone_angle, F32 min_twist, F32 max_twist)
{
    mForward = forward;
    mForward.normalize();
    mCosConeAngle = std::cos(cone_angle);
    mSinConeAngle = std::sin(cone_angle);

    mMinTwist = min_twist;
    mMaxTwist = max_twist;
    compute_angle_limits(mMinTwist, mMaxTwist);
    mType = Info::TWIST_LIMITED_CONE_CONSTRAINT;
}

LLQuaternion LLIK::TwistLimitedCone::computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const
{
    LLVector3 joint_forward = mForward * joint_local_rot;
    LLQuaternion adjusted_local_rot = joint_local_rot;
    F32 forward_component = joint_forward * mForward;
    if (forward_component < mCosConeAngle)
    {
        // the joint's version of mForward lies outside the cone
        // so we project it onto the surface of the cone...
        //
        // projection               = (forward_part)         + (orthogonal_part)
        LLVector3 perp = joint_forward - forward_component * mForward;
        perp.normalize();
        LLVector3 new_joint_forward = mCosConeAngle * mForward + mSinConeAngle * perp;

        // ... then compute the adjusted rotation
        LLQuaternion adjustment;
        adjustment.shortestArc(joint_forward, new_joint_forward);
        adjusted_local_rot = joint_local_rot * adjustment;
    }

    // rotate mForward by adjusted_local_rot
    joint_forward = mForward * adjusted_local_rot;
    forward_component = joint_forward * mForward;

    // compute two axes perpendicular to joint_forward: perp_x and perp_y
    LLVector3 perp_x = mForward % joint_forward;
    F32 perp_length = perp_x.length();
    constexpr F32 MIN_PERP_LENGTH = 1.0e-3f;
    if (perp_length < MIN_PERP_LENGTH)
    {
        perp_x = LLVector3::y_axis % mForward;
        perp_length = perp_x.length();
        if (perp_length < MIN_PERP_LENGTH)
        {
            perp_x = mForward % LLVector3::x_axis;
        }
    }
    perp_x.normalize();
    LLVector3 perp_y = joint_forward % perp_x;

    // the components of joint_perp on each direction allow us to compute twist angle
    LLVector3 joint_perp = perp_x * adjusted_local_rot;
    F32 twist = std::atan2(joint_perp * perp_y, joint_perp * perp_x);

    // clamp twist within bounds
    if (twist > mMaxTwist || twist < mMinTwist)
    {
        twist = compute_clamped_angle(twist, mMinTwist, mMaxTwist);
        joint_perp -= (joint_perp * joint_forward) * joint_forward;
        LLVector3 new_joint_perp = std::cos(twist) * perp_x + std::sin(twist) * perp_y;
        LLQuaternion adjustment;
        adjustment.shortestArc(joint_perp, new_joint_perp);
        adjusted_local_rot = adjusted_local_rot * adjustment;
    }
    adjusted_local_rot.normalize();
    return adjusted_local_rot;
}

LLQuaternion LLIK::TwistLimitedCone::minimizeTwist(const LLQuaternion& joint_local_rot) const
{
    // Compute the swing and combine with default twist
    // which is the midpoint of the twist range.
    LLQuaternion mid_twist;
    mid_twist.setAngleAxis(0.5f * (mMinTwist + mMaxTwist), mForward);

    // joint_local_rot = mid_twist * swing
    LLQuaternion new_local_rot = mid_twist;

    LLVector3 joint_forward = mForward * joint_local_rot;
    LLVector3 swing_axis = mForward % joint_forward;
    constexpr F32 MIN_SWING_AXIS_LENGTH = 1.0e-3f;
    if (swing_axis.length() > MIN_SWING_AXIS_LENGTH)
    {
        LLQuaternion swing;
        F32 swing_angle = std::acos(mForward * joint_forward);
        swing.setAngleAxis(swing_angle, swing_axis);
        new_local_rot = mid_twist * swing;
    }

    return new_local_rot;
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::TwistLimitedCone::dumpConfig() const
{
    LL_INFOS("debug") << "{'type':'TwistLimitedCone'"
        << ",'forward':(" << mForward.mV[VX] << "," << mForward.mV[VY] << "," << mForward.mV[VZ] << ")"
        << ",'cone_angle':" << std::atan2(mSinConeAngle, mCosConeAngle)
        << ",'min_twist':" << mMinTwist
        << ",'max_twist':" << mMaxTwist << "}" << LL_ENDL;
}
#endif

LLIK::ElbowConstraint::ElbowConstraint(const LLVector3& forward_axis, const LLVector3& pivot_axis, F32 min_bend, F32 max_bend, F32 min_twist, F32 max_twist)
{
    mForward = forward_axis;
    mForward.normalize();
    mPivotAxis = mForward % (pivot_axis % mForward);
    mPivotAxis.normalize();
    mLeft = mPivotAxis % mForward;

    mMinBend = min_bend;
    mMaxBend = max_bend;
    compute_angle_limits(mMinBend, mMaxBend);

    mMinTwist = min_twist;
    mMaxTwist = max_twist;
    compute_angle_limits(mMinTwist, mMaxTwist);
    mType = Info::ELBOW_CONSTRAINT;
}

LLQuaternion LLIK::ElbowConstraint::computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const
{
    // rotate mForward into joint-frame
    LLVector3 joint_forward = mForward * joint_local_rot;

    // compute adjustment required to move joint_forward back into hinge plane
    LLVector3 projected_joint_forward = joint_forward - (joint_forward * mPivotAxis) * mPivotAxis;
    LLQuaternion adjustment;
    adjustment.shortestArc(joint_forward, projected_joint_forward);
    LLQuaternion adjusted_local_rot = joint_local_rot * adjustment;

    // measure twist
    LLVector3 twisted_pivot = mPivotAxis * adjusted_local_rot;
    F32 cos_part = twisted_pivot * mPivotAxis;
    F32 sin_part = (mLeft * adjusted_local_rot) * mPivotAxis;
    F32 twist = std::atan2(sin_part, cos_part);

    LLVector3 new_joint_forward = mForward * adjusted_local_rot;
    if (twist < mMinTwist || twist > mMaxTwist)
    {
        // adjust twist
        twist = compute_clamped_angle(twist, mMinTwist, mMaxTwist);
        LLVector3 swung_left_axis = mPivotAxis % new_joint_forward;
        LLVector3 new_twisted_pivot = std::cos(twist) * mPivotAxis - std::sin(twist) * swung_left_axis;
        adjustment.shortestArc(twisted_pivot, new_twisted_pivot);
        adjusted_local_rot = adjusted_local_rot * adjustment;
        new_joint_forward = mForward * adjusted_local_rot;
    }

    // measure bend
    F32 bend = std::atan2(new_joint_forward * mLeft, new_joint_forward * mForward);

    if (bend > mMaxBend || bend < mMinBend)
    {
        // adjust bend
        bend = compute_clamped_angle(bend, mMinBend, mMaxBend);
        new_joint_forward = std::cos(bend) * mForward + std::sin(bend) * mLeft;
        adjustment.shortestArc(joint_forward, new_joint_forward);
        adjusted_local_rot = adjusted_local_rot * adjustment;
    }
    adjusted_local_rot.normalize();
    return adjusted_local_rot;
}

LLQuaternion LLIK::ElbowConstraint::minimizeTwist(const LLQuaternion& joint_local_rot) const
{
    // Assume all swing is really just bend about mPivotAxis and twist is centered
    // in the valid twist range.
    // If bend_angle is outside the limits then we check both +/- bend_angle and pick
    // the one closest to the allowed range.  This comes down to a simple question:
    // which is closer to the midpoint of the bend range?

    LLVector3 joint_forward = mForward * joint_local_rot;
    F32 fdot = joint_forward * mForward;
    LLVector3 perp_part = joint_forward - fdot * mForward;
    F32 bend_angle = std::atan2(perp_part.length(), fdot);

    if (bend_angle < mMinBend  || bend_angle > mMaxBend)
    {
        F32 alt_bend_angle = - bend_angle;
        F32 mid_bend = 0.5f * (mMinBend + mMaxBend);
        if (std::abs(alt_bend_angle - mid_bend) < std::abs(bend_angle - mid_bend))
        {
            bend_angle = alt_bend_angle;
        }
    }
    LLQuaternion bend;
    bend.setAngleAxis(bend_angle, mPivotAxis);

    LLQuaternion mid_twist;
    mid_twist.setAngleAxis(0.5f * (mMinTwist + mMaxTwist), mForward);
    return mid_twist * bend;
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::ElbowConstraint::dumpConfig() const
{
    LL_INFOS("debug") << "{'type':'Elbow'"
        << ",'forward':(" << mForward.mV[VX] << "," << mForward.mV[VY] << "," << mForward.mV[VZ] << ")"
        << ",'pivot':(" << mPivotAxis.mV[VX] << "," << mPivotAxis.mV[VY] << "," << mPivotAxis.mV[VZ] << ")"
        << ",'min_bend':" << mMinBend
        << ",'max_bend':" << mMaxBend
        << ",'min_twist':" << mMinTwist
        << ",'max_twist':" << mMaxTwist << "}" << LL_ENDL;
}
#endif

LLIK::KneeConstraint::KneeConstraint(const LLVector3& forward_axis, const LLVector3& pivot_axis, F32 min_bend, F32 max_bend)
{
    mForward = forward_axis;
    mForward.normalize();
    mPivotAxis = mForward % (pivot_axis % mForward);
    mPivotAxis.normalize();
    mLeft = mPivotAxis % mForward;

    mMinBend = min_bend;
    mMaxBend = max_bend;
    compute_angle_limits(mMinBend, mMaxBend);
    mType = Info::KNEE_CONSTRAINT;
}

LLQuaternion LLIK::KneeConstraint::computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const
{
    // rotate mPivotAxis into joint-frame
    LLVector3 joint_axis = mPivotAxis * joint_local_rot;
    LLQuaternion adjustment;
    adjustment.shortestArc(joint_axis, mPivotAxis);
    LLQuaternion adjusted_local_rot = joint_local_rot * adjustment;

    // rotate mForward into joint-frame
    LLVector3 joint_forward = mForward * adjusted_local_rot;

    LLVector3 new_joint_forward = joint_forward;

    // compute angle between mForward and new_joint_forward
    F32 bend = std::atan2(new_joint_forward * mLeft, new_joint_forward * mForward);
    if (bend > mMaxBend || bend < mMinBend)
    {
        bend = compute_clamped_angle(bend, mMinBend, mMaxBend);
        new_joint_forward = std::cos(bend) * mForward + std::sin(bend) * mLeft;
        adjustment.shortestArc(joint_forward, new_joint_forward);
        adjusted_local_rot = adjusted_local_rot * adjustment;
    }

    adjusted_local_rot.normalize();
    return adjusted_local_rot;
}

LLQuaternion LLIK::KneeConstraint::minimizeTwist(const LLQuaternion& joint_local_rot) const
{
    // Assume all swing is really just bend about mPivotAxis.
    // If bend_angle is outside the limits then we check both +/- bend_angle and pick
    // the one closest to the allowed range.  This comes down to a simple question:
    // which is closer to the midpoint of the bend range?
    LLVector3 joint_forward = mForward * joint_local_rot;
    F32 fdot = joint_forward * mForward;
    LLVector3 perp_part = joint_forward - fdot * mForward;
    F32 bend_angle = std::atan2(perp_part.length(), fdot);
    if (bend_angle < mMinBend  || bend_angle > mMaxBend)
    {
        F32 alt_bend_angle = - bend_angle;
        F32 mid_bend = 0.5f * (mMinBend + mMaxBend);
        if (std::abs(alt_bend_angle - mid_bend) < std::abs(bend_angle - mid_bend))
        {
            bend_angle = alt_bend_angle;
        }
    }
    LLQuaternion bend;
    bend.setAngleAxis(bend_angle, mPivotAxis);
    return bend;
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::KneeConstraint::dumpConfig() const
{
    LL_INFOS("debug") << "{'type':'Knee'"
        << ",'forward':(" << mForward.mV[VX] << "," << mForward.mV[VY] << "," << mForward.mV[VZ] << ")"
        << ",'pivot':(" << mPivotAxis.mV[VX] << "," << mPivotAxis.mV[VY] << "," << mPivotAxis.mV[VZ] << ")"
        << ",'min_bend':" << mMinBend
        << ",'max_bend':" << mMaxBend << "}" << LL_ENDL;
}
#endif

LLIK::AcuteEllipsoidalCone::AcuteEllipsoidalCone(
        const LLVector3& forward_axis,
        const LLVector3& up_axis,
        F32 forward,
        F32 up, F32 left, F32 down, F32 right)
{
    mUp = up_axis;
    mUp.normalize();
    mForward = (mUp % forward_axis) % mUp;
    mForward.normalize();
    mLeft = mUp % mForward; // already normalized

    // Divide everything by 'foward' and take make sure they are positive.
    // This normalizes the forward component (adjacent side) of all the
    // triangles to have length=1.0, which is important for our trigonometry
    // math later.
    //
    // up  left            |
    //  | /                | /
    //  |/                 |/
    //  @------------------+
    //         1.0        /|
    //                     |
    up = std::abs(up / forward);
    left = std::abs(left / forward);
    down = std::abs(down / forward);
    right = std::abs(right / forward);

    // These are the indices of the directions and quadrants.
    // With 'forward' pointing into the page.
    //             up
    //              |
    //          1   |   0
    //              |
    //  left ------(x)------ right
    //              |
    //          2   |   3
    //              |
    //            down
    //
    // When projecting vectors onto the ellipsoidal surface we will
    // always scale the left-axis into the frame in which the ellipsoid
    // is circular. We cache the necessary scale coefficients now:
    //
    mQuadrantScales[0] = up / right;
    mQuadrantScales[1] = up / left;
    mQuadrantScales[2] = down / left;
    mQuadrantScales[3] = down / right;

    // When determining whether a direction is inside or outside the
    // ellipsoid we will need the cosine and cotangent of the cone
    // angles in the scaled frames. We cache them now:
    //     cosine = adjacent / hypotenuse
    //     cotangent = adjacent / opposite
    //
    mQuadrantCosAngles[0] = 1.0f / std::sqrt(up * up + 1.0f);
    mQuadrantCotAngles[0] = 1.0f / up;
    mQuadrantCosAngles[1] = mQuadrantCosAngles[0];
    mQuadrantCotAngles[1] = mQuadrantCotAngles[0];
    mQuadrantCosAngles[2] = 1.0f / std::sqrt(down * down + 1.0f);
    mQuadrantCotAngles[2] = 1.0f / down;
    mQuadrantCosAngles[3] = mQuadrantCosAngles[2];
    mQuadrantCotAngles[3] = mQuadrantCotAngles[2];
    mType = Info::ACUTE_ELLIPSOIDAL_CONE_CONSTRAINT;
}

LLQuaternion LLIK::AcuteEllipsoidalCone::computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const
{
    // rotate mForward into joint-frame
    LLVector3 joint_forward = mForward * joint_local_rot;
    // joint_forward is normalized

    // determine its quadrant
    F32 up_component = joint_forward * mUp;
    F32 left_component = joint_forward * mLeft;
    U32 q = 0;
    if (up_component < 0.0f)
    {
        if (left_component < 0.0f)
        {
            q = 2;
        }
        else
        {
            q = 3;
        }
    }
    else if (left_component < 0.0f)
    {
        q = 1;
    }

    // scale left axis to frame in which ellipse is a circle
    F32 scaled_left_component = left_component * mQuadrantScales[q];

    // reassemble in scaled frame
    F32 forward_component = joint_forward * mForward;
    LLVector3 new_joint_forward = forward_component * mForward
        + up_component * mUp
        + scaled_left_component * mLeft;
    // new_joint_forward is not normalized
    // which means we must adjust its the forward_component when
    // checking for violation in scaled frame
    if (forward_component / new_joint_forward.length() < mQuadrantCosAngles[q])
    {
        // joint violates constraint --> project onto cone
        //
        // violates      projected
        //       +        +
        //        .      /|
        //         .    / |
        //          .  // |
        //           .//  |
        //            @---+----
        //             \
        //              \
        //
        // Orthogonal components remain unchanged but we need to compute
        // a corrected forward_component (adjacent leg of the right triangle)
        // in the scaled frame. We can use the formula:
        //     adjacent = opposite * cos(angle) / sin(angle)
        //     adjacent = opposite * cot(angle)
        //
        F32 orthogonal_component = std::sqrt(scaled_left_component * scaled_left_component + up_component * up_component);
        forward_component = orthogonal_component * mQuadrantCotAngles[q];

        // re-assemble the projected direction in the non-scaled frame:
        new_joint_forward = forward_component * mForward
            + up_component * mUp
            + left_component * mLeft;
        // new_joint_forward is not normalized, but it doesn't matter

        // compute adjusted_local_rot
        LLQuaternion adjustment;
        adjustment.shortestArc(joint_forward, new_joint_forward);
        LLQuaternion adjusted_local_rot = joint_local_rot * adjustment;
        adjusted_local_rot.normalize();
        return adjusted_local_rot;
    }
    else
    {
        return joint_local_rot;
    }
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::AcuteEllipsoidalCone::dumpConfig() const
{
    LL_INFOS("debug") << "{'type':'AcuteEllipsoidalCone'"
        << ",'TODO':'implement this'}" << LL_ENDL;
}
#endif

LLIK::DoubleLimitedHinge::DoubleLimitedHinge(
        const LLVector3& forward_axis,
        const LLVector3& up_axis,
        F32 min_yaw,
        F32 max_yaw,
        F32 min_pitch,
        F32 max_pitch)
{
    mForward = forward_axis;
    mForward.normalize();
    mUp = mForward % (up_axis % mForward);
    mUp.normalize();
    mLeft = mUp % mForward;

    mMinYaw = min_yaw;
    mMaxYaw = max_yaw;
    compute_angle_limits(mMinYaw, mMaxYaw);

    // keep pitch in range [-PI/2, PI/2]
    F32 HALF_PI = 0.5f * F_PI;
    mMinPitch = remove_multiples_of_two_pi(min_pitch);
    if (mMinPitch > HALF_PI)
    {
        mMinPitch = HALF_PI;
    }
    else if (mMinPitch < -HALF_PI)
    {
        mMinPitch = -HALF_PI;
    }
    mMaxPitch = remove_multiples_of_two_pi(max_pitch);
    if (mMaxPitch > HALF_PI)
    {
        mMaxPitch = HALF_PI;
    }
    else if (mMaxPitch < -HALF_PI)
    {
        mMaxPitch = -HALF_PI;
    }
    if (mMinPitch > mMaxPitch)
    {
        F32 temp = mMinPitch;
        mMinPitch = mMaxPitch;
        mMaxPitch = temp;
    }
}

LLQuaternion LLIK::DoubleLimitedHinge::computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const
{
    // twist
    // eliminate twist by adjusting the rotated mLeft axis
    // to remain in horizontal plane
    LLVector3 joint_left = mLeft * joint_local_rot;
    LLQuaternion adjustment;
    adjustment.shortestArc(joint_left, joint_left - (joint_left * mUp) * mUp);
    LLQuaternion adjusted_local_rot = joint_local_rot * adjustment;

    LLVector3 joint_forward = mForward * adjusted_local_rot;

    // yaw
    F32 up_component = joint_forward * mUp;
    LLVector3 horizontal_axis = joint_forward - up_component * mUp;
    F32 yaw = std::atan2(horizontal_axis * mLeft, horizontal_axis * mForward);
    if (yaw > mMaxYaw || yaw < mMinYaw)
    {
        yaw = compute_clamped_angle(yaw, mMinYaw, mMaxYaw);
        horizontal_axis = std::cos(yaw) * mForward + std::sin(yaw) * mLeft;
    }
    else
    {
        horizontal_axis.normalize();
    }

    // pitch
    // Note: the minus-sign in the "opposite" (sin) term here is because
    // our pitch-axis is mLeft and according to the right-hand-rule positive
    // pitch drops the forward axis down.
    F32 horizontal_component = std::sqrt(std::max(1.0f - up_component * up_component, 0.0f));
    F32 pitch = std::atan2(-up_component, horizontal_component);
    if (pitch > mMaxPitch || pitch < mMinPitch)
    {
        pitch = compute_clamped_angle(pitch, mMinPitch, mMaxPitch);
        up_component = -std::sin(pitch);
        horizontal_component = std::sqrt(std::max(1.0f - up_component * up_component, 0.0f));
    }

    LLVector3 new_joint_forward = horizontal_component * horizontal_axis + up_component * mUp;
    new_joint_forward.normalize();
    if (dist_vec(joint_forward, new_joint_forward) > 1.0e-3f)
    {
        // compute adjusted_local_rot
        adjustment.shortestArc(joint_forward, new_joint_forward);
        adjusted_local_rot = adjusted_local_rot * adjustment;
    }
    adjusted_local_rot.normalize();
    return adjusted_local_rot;
}

LLQuaternion LLIK::DoubleLimitedHinge::minimizeTwist(const LLQuaternion& joint_local_rot) const
{
    // eliminate twist by adjusting the rotated mLeft axis
    // to remain in horizontal plane
    LLVector3 joint_left = mLeft * joint_local_rot;
    LLQuaternion adjustment;
    adjustment.shortestArc(joint_left, joint_left - (joint_left * mUp) * mUp);
    LLQuaternion adjusted_local_rot = joint_local_rot * adjustment;
    adjusted_local_rot.normalize();
    return adjusted_local_rot;
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::DoubleLimitedHinge::dumpConfig() const
{
    LL_INFOS("debug") << "{'type':'DoubleLimitedHinge'"
        << ",'forward':(" << mForward.mV[VX] << "," << mForward.mV[VY] << "," << mForward.mV[VZ] << ")"
        << ",'up':(" << mUp.mV[VX] << "," << mUp.mV[VY] << "," << mUp.mV[VZ] << ")"
        << ",'min_yaw':" << mMinYaw
        << ",'max_yaw':" << mMaxYaw
        << ",'min_pitch':" << mMinPitch
        << ",'max_pitch':" << mMaxPitch << "}" << LL_ENDL;
}
#endif

LLIK::Joint::Joint(S16 id, const LLVector3& local_pos, const LLVector3& bone)
    :   mDefaultLocalPos(local_pos), mBone(bone), mID(id)
{
    mLocalPos = mDefaultLocalPos;
    mLocalPosLength = mLocalPos.length();
}

void LLIK::Joint::addChild(const ptr_t& child)
{
    if (child)
    {
        mChildren.push_back(child);
    }
}

void LLIK::Joint::setTargetPos(const LLVector3& pos)
{
    if (hasPosTarget())
    {
        // HACK: cast mConfig to non-const pointer so we can modify it
        Config* config = const_cast<Config*>(mConfig);
        config->setTargetPos(pos);
    }
}

void LLIK::Joint::setParent(const ptr_t& parent)
{
    mParent = parent;
    if (!mParent)
    {
        // The root's local orientation is never updated by the IK algorithm.
        // Whatever orientation it has at the start of IK will be its final,
        // which is why we flag it as "locked".  This also simplifies logic
        // elsewhere: in a few places we assume any non-locked Joint has a parent.
        mConfigFlags = FLAG_LOCAL_ROT;
    }
    reset();
}

void LLIK::Joint::reset()
{
    mLocalPos = mDefaultLocalPos;
    // Note: we don't bother to enforce localRotLocked() here because any call
    // to reset() is expected to be outside the Solver IK iterations.
    mLocalRot = LLQuaternion::DEFAULT;
    if (mParent)
    {
        mPos = mParent->mPos + mLocalPos * mParent->mRot;
        mRot = mParent->mRot;
    }
    else
    {
        mPos = mLocalPos;
        mRot = mLocalRot;
    }
}

void LLIK::Joint::relaxRot(F32 blend_factor)
{
    if (!localRotLocked())
    {
        mLocalRot = lerp(blend_factor, mLocalRot, LLQuaternion::DEFAULT);
    }
    if (mParent)
    {
        // we always recompute world-frame transform because parent may have relaxed
        mRot = mLocalRot * mParent->mRot;
        mRot.normalize();
        mPos = mParent->mPos + mLocalPos * mParent->mRot;
    }
    else
    {
        mRot = mLocalRot;
        mPos = mLocalPos;
    }
}

void LLIK::Joint::resetRecursively()
{
    reset();
    for (auto& child: mChildren)
    {
        child->resetRecursively();
    }
}

void LLIK::Joint::relaxRotationsRecursively(F32 blend_factor)
{
    blend_factor = std::min(std::max(blend_factor, 0.0f), 1.0f);
    relaxRot(blend_factor);
    for (auto& child: mChildren)
    {
        if (child->isActive())
        {
            child->relaxRotationsRecursively(blend_factor);
        }
    }
}

F32 LLIK::Joint::recursiveComputeLongestChainLength(F32 length) const
{
    length += mLocalPosLength;
    F32 longest_length = length;
    if (mChildren.empty())
    {
        longest_length += mBone.length();
    }
    else
    {
        for (const auto& child : mChildren)
        {
            F32 child_length = child->recursiveComputeLongestChainLength(length);
            if (child_length > longest_length)
            {
                longest_length = child_length;
            }
        }
    }
    return longest_length;
}

void LLIK::Joint::reconfigure(const LLVector3& local_pos, const LLVector3& bone)
{
    mDefaultLocalPos = local_pos;
    mLocalPos = local_pos;
    mBone = bone;
    mLocalPosLength = mLocalPos.length();
}

LLVector3 LLIK::Joint::computeEndTargetPos() const
{
    // Note: we expect this Joint has either: a target,
    // or at least one active child
    if (hasPosTarget())
    {
        return mConfig->getTargetPos();
    }
    LLVector3 target_pos = LLVector3::zero;
    S32 num_active_children = 0;
    for (const auto& child : mChildren)
    {
        if (child->isActive())
        {
            target_pos += child->mPos;
            ++num_active_children;
        }
    }
    return (1.0f / F32(num_active_children)) * target_pos;
}

LLVector3 LLIK::Joint::computeWorldTipOffset() const
{
    LLVector3 offset = mPos;
    if (mParent)
    {
        offset -= mParent->mPos + mLocalPos * mParent->mRot;
    }
    return offset;
}

void LLIK::Joint::updateEndInward()
{
    // Note: during FABRIK we DO NOT enforce constraints.
    DEBUG_SET_CONTEXT(inward);
    if (hasRotTarget())
    {
        mRot = mConfig->getTargetRot();
        if (hasPosTarget())
        {
            mPos = mConfig->getTargetPos() - mBone * mRot;
        }
    }
    else
    {
        std::vector<LLVector3> local_targets;
        std::vector<LLVector3> world_targets;
        collectTargetPositions(local_targets, world_targets);
        size_t num_targets = local_targets.size();

        if (num_targets == 1)
        {
            // special handling for the most common num_targets=1 case
            // compute mPos
            LLVector3 bone_dir = world_targets[0] - mPos;
            bone_dir.normalize();
            mPos = world_targets[0] - (local_targets[0].length() * bone_dir);

            // compute new mRot
            LLVector3 old_bone = local_targets[0] * mRot;
            LLQuaternion adjustment;
            adjustment.shortestArc(old_bone, bone_dir);
            mRot = mRot * adjustment;
            mRot.normalize();
        }
        else
        {
            LLVector3 new_pos(0.0f, 0.0f, 0.0f);
            LLQuaternion avg_adjustment(0.0f, 0.0f, 0.0f, 0.0f); // origin in quaternion space
            for (size_t i = 0; i < num_targets; ++i)
            {
                // mPos
                LLVector3 new_bone = world_targets[i] - mPos;
                new_bone.normalize();
                new_bone *= local_targets[i].length();
                new_pos += world_targets[i] - new_bone;

                // mRot
                LLVector3 old_bone = local_targets[i] * mRot;
                LLQuaternion adjustment;
                adjustment.shortestArc(old_bone, new_bone);
                if (adjustment.mQ[VW] < 0.0f)
                {
                    // negate to keep all arithmetic on the same hypersphere
                    avg_adjustment = avg_adjustment - adjustment;
                }
                else
                {
                    avg_adjustment = avg_adjustment + adjustment;
                }
            }
            if (mParent && mParent->isActive())
            {
                // compute mPos
                mPos = new_pos / (F32)(num_targets);
            }

            // compute mRot
            avg_adjustment.normalize();
            mRot = mRot * avg_adjustment;
            mRot.normalize();
        }
    }
    DEBUG_LOG_EVENT_DETAIL(outer_end);
    // Note: mLocalRot will be updated later when we know mParent's location

    // now that we know mRot --> update children mLocalRot
    for (auto& child : mChildren)
    {
        if (child->isActive())
        {
            child->updateLocalRot();
        }
    }
}

void LLIK::Joint::updateEndOutward()
{
    // Note: during FABRIK we DO NOT enforce constraints.
    // mParent is expected to be non-null.
    mPos = mParent->mPos + mLocalPos * mParent->mRot;

    // mRot
    if (localRotLocked())
    {
        mRot = mLocalRot * mParent->mRot;
        DEBUG_LOG_EVENT_DETAIL(lock_local);
        return;
    }

    if (hasRotTarget())
    {
        mRot = mConfig->getTargetRot();
        if (hasPosTarget())
        {
            mPos = mConfig->getTargetPos() - mBone * mRot;
        }
    }
    else
    {
        std::vector<LLVector3> local_targets;
        std::vector<LLVector3> world_targets;
        collectTargetPositions(local_targets, world_targets);
        size_t num_targets = local_targets.size();
        if (num_targets == 1)
        {
            // special handling for the most common num_targets=1 case
            LLVector3 new_bone = world_targets[0] - mPos;
            LLVector3 old_bone = local_targets[0] * mRot;
            LLQuaternion adjustment;
            adjustment.shortestArc(old_bone, new_bone);
            mRot = mRot * adjustment;
        }
        else
        {
            LLQuaternion avg_adjustment(0.0f, 0.0f, 0.0f, 0.0f); // origin in quaternion space
            LLQuaternion adjustment;
            for (size_t i = 0; i < num_targets; ++i)
            {
                LLVector3 new_bone = world_targets[i] - mPos;
                LLVector3 old_bone = local_targets[i] * mRot;
                adjustment.shortestArc(old_bone, new_bone);
                if (adjustment.mQ[VW] < 0.0f)
                {
                    // negate to keep all Quaternion arithmetic on one hypersphere
                    avg_adjustment = avg_adjustment - adjustment;
                }
                else
                {
                    avg_adjustment = avg_adjustment + adjustment;
                }
            }
            avg_adjustment.normalize();
            mRot = mRot * avg_adjustment;
        }
        mRot.normalize();
    }
    DEBUG_LOG_EVENT_DETAIL(outer_end);

    updateLocalRot();
}

// this Joint's child is specified in argument
// in case this Joint has multiple children.
void LLIK::Joint::updateInward(const Joint::ptr_t& child)
{
    // Note: during FABRIK we DO NOT enforce constraints.
    // mParent is expected to be non-null.
    // compute mPos
    LLVector3 old_pos = mPos;
    LLVector3 bone_dir = child->mPos - old_pos;
    bone_dir.normalize();
    mPos = child->mPos - child->mLocalPosLength * bone_dir;
    // compute mRot
    LLVector3 old_bone = child->mLocalPos * mRot;
    LLQuaternion adjustment;
    adjustment.shortestArc(old_bone, bone_dir);
    mRot = mRot * adjustment;
    mRot.normalize();
    DEBUG_LOG_EVENT;
    // compute child->mLocalRot
    child->updateLocalRot();
    // this->mLocalRot will be updated later
}

void LLIK::Joint::updatePosAndRotFromParent()
{
    if (mParent)
    {
        mPos = mParent->mPos + mLocalPos * mParent->mRot;
        mRot = mLocalRot * mParent->mRot;
        mRot.normalize();
        DEBUG_LOG_EVENT;
    }
}

void LLIK::Joint::updateOutward()
{
    // Note: during FABRIK we DO NOT enforce constraints.
    // mParent is expected to be non-null.
    LLVector3 old_end_pos = mPos + mBone * mRot;

    // mPos
    mPos = mParent->mPos + mLocalPos * mParent->mRot;

    // mRot
    LLVector3 new_bone = old_end_pos - mPos;
    LLVector3 old_bone = mBone * mRot;
    LLQuaternion dQ;
    dQ.shortestArc(old_bone, new_bone);
    mRot = mRot * dQ;
    mRot.normalize();
    DEBUG_LOG_EVENT;

    updateLocalRot();
}

void LLIK::Joint::applyLocalRot()
{
    if (mParent)
    {
        if (hasRotTarget())
        {
            // apply backpressure by lerping toward new_rot
            LLQuaternion new_rot = mLocalRot * mParent->mRot;
            constexpr F32 WORLD_ROT_TARGET_BACKPRESSURE_COEF = 0.5f;
            mRot = lerp(WORLD_ROT_TARGET_BACKPRESSURE_COEF, mConfig->getTargetRot(), new_rot);

            // recompute mLocalRot
            LLQuaternion inv_parentRot = mParent->mRot;
            inv_parentRot.conjugate();
            mLocalRot = mRot * inv_parentRot;
            mLocalRot.normalize();
        }
        else
        {
            mRot = mLocalRot * mParent->mRot;
            mRot.normalize();
        }
        DEBUG_LOG_EVENT_DETAIL(enforce);
    }
}

void LLIK::Joint::updateLocalRot()
{
    if (!localRotLocked())
    {
        // mPos and mRot are expected to be correct
        // and mParent is expected to be valid
        LLQuaternion inv_parentRot = mParent->mRot;
        inv_parentRot.conjugate();
        mLocalRot = mRot * inv_parentRot;
        mLocalRot.normalize();
    }
}

LLQuaternion LLIK::Joint::computeParentRot() const
{
    // formula is:
    //     mRot = mLocalRot * mParent->mRot
    // solving for mParent->mRot gives:
    //     mParent->mRot = mLocalRotInv * mRot
    LLQuaternion q = mLocalRot;
    q.conjugate();
    q = q * mRot;
    q.normalize();
    return q;
}

void LLIK::Joint::updateChildLocalRots() const
{
    // now that we know mRot we can update the childrens' mLocalRot
    for (const Joint::ptr_t& child: mChildren)
    {
        if (child->isActive())
        {
            child->updateLocalRot();
        }
    }
}

LLVector3 LLIK::Joint::computePosFromParent() const
{
    return mParent->mPos + mLocalPos * mParent->mRot;
}

void LLIK::Joint::shiftPos(const LLVector3& shift)
{
    mPos += shift;
    DEBUG_LOG_EVENT;
}

void LLIK::Joint::setConfig(const Config& config)
{
    mConfig = &config;
    mConfigFlags = mConfig ? mConfig->getFlags() : 0;
}

void LLIK::Joint::resetFlags()
{
    mConfig = nullptr;
    // root Joint always has FLAG_LOCAL_ROT bit set
    mConfigFlags = mParent ? 0 : FLAG_LOCAL_ROT;
}

void LLIK::Joint::lockLocalRot(const LLQuaternion& local_rot)
{
    mLocalRot = local_rot;
    activate();
    if (!mParent)
    {
        mRot = local_rot;
    }
}

bool LLIK::Joint::enforceConstraint()
{
    if (localRotLocked())
    {
        // A fixed mLocalRot is effectively like a fixed Constraint so we
        // always return 'true' here: the Constraint is in effect and mRot may
        // have been optimistically modified but mLocalRot was not.
        return true;
    }
    if (mConstraint && !(hasDisabledConstraint()))
    {
        return mConstraint->enforce(*this);
    }
    return false;
}

void LLIK::Joint::updateWorldTransformsRecursively()
{
    updatePosAndRotFromParent();
    for (Joint::ptr_t& child: mChildren)
    {
        if (child->isActive())
        {
            child->updateWorldTransformsRecursively();
        }
    }
}

// return valid Joint::ptr_t to child iff only one child is active
// else return null Joint::ptr_t
LLIK::Joint::ptr_t LLIK::Joint::getSingleActiveChild()
{
    Joint::ptr_t active_child;
    for (Joint::ptr_t& child: mChildren)
    {
        if (child->isActive())
        {
            if (active_child)
            {
                // second child --> this Joint is not a "false" sub-base
                active_child.reset();
                break;
            }
            active_child = child;
        }
    }
    return active_child;
}

LLVector3 LLIK::Joint::computeWorldEndPos() const
{
    return mPos + mBone * mRot;
}

void LLIK::Joint::setWorldPos(const LLVector3& pos)
{
    mPos = pos;
    DEBUG_LOG_EVENT;
}

// only call this if you know what you're doing
// this should only be called once before starting IK algorithm iterations
void LLIK::Joint::setLocalPos(const LLVector3& pos)
{
    mLocalPos = pos;
    if (!mParent)
    {
        mPos = mLocalPos;
    }
}

void LLIK::Joint::setWorldRot(const LLQuaternion& rot)
{
    mRot = rot;
    DEBUG_LOG_EVENT;
}

void LLIK::Joint::setLocalRot(const LLQuaternion& new_local_rot)
{
    if (!localRotLocked())
    {
        constexpr F32 BLEND_COEF = 0.25f;
        mLocalRot = lerp(BLEND_COEF, mLocalRot, new_local_rot);
        //mLocalRot = new_local_rot;
    }
}

void LLIK::Joint::adjustWorldRot(const LLQuaternion& adjustment)
{
    mRot = mRot * adjustment;
    DEBUG_LOG_EVENT;
    updateLocalRot();
    if (enforceConstraint())
    {
        applyLocalRot();
    }
}

void LLIK::Joint::collectTargetPositions(std::vector<LLVector3>& local_targets, std::vector<LLVector3>& world_targets) const
{
    // The "target positions" are points in the Joint local-frame which correspond
    // to points in other frames: either child positions or a target end-effector.
    // We need to know these positions in both local- and world-frame.
    //
    // Note: it is expected this Joint has either: a target,
    // or at least one active child
    //
    if (hasPosTarget())
    {
        local_targets.push_back(mBone);
        world_targets.push_back(mConfig->getTargetPos());
    }
    else
    {
        // TODO: local_centroid and its length are invarient for the lifetime
        // of the Chains so we could pre-compute and cache them and simplify
        // the logic which consumes this info.
        for (const auto& child : mChildren)
        {
            if (child->isActive())
            {
                local_targets.push_back(child->mLocalPos);
                world_targets.push_back(child->mPos);
            }
        }
    }
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::Joint::dumpConfig() const
{
    S16 parent_id = mParent ? mParent->mID : -1;
    LL_INFOS("debug") << "{'id':" << mID << ",'parent_id':" << parent_id
        << "'world_pos':(" << mPos.mV[VX] << "," << mPos.mV[VY] << "," << mPos.mV[VZ] << ")"
        << ",'local_pos':(" << mLocalPos.mV[VX] << "," << mLocalPos.mV[VY] << "," << mLocalPos.mV[VZ] << ")"
        << ",'bone':(" << mBone.mV[VX] << "," << mBone.mV[VY] << "," << mBone.mV[VZ] << ")" << LL_ENDL;
    if (mConstraint)
    {
        LL_INFOS("debug") << ",'constraint':" << LL_ENDL;
        mConstraint->dumpConfig();
    }
    LL_INFOS("debug") << "}" << LL_ENDL;
}

void LLIK::Joint::dumpState() const
{
    // Outputs a pytyhon-friendly tuple: (id,(tip),(bone))
    // Use std::cout because we'll be debugging in a stand-alone test app.
    LLVector3 bone = computeWorldEndPos() - mPos;
    std::cout << "(" << mID << ",(" << mPos.mV[VX] << "," << mPos.mV[VY] << "," << mPos.mV[VZ] << "), "
        << "(" << bone.mV[VX] << "," << bone.mV[VY] << "," << bone.mV[VZ] << "))";
}
#endif // DEBUG_LLIK_UNIT_TESTS

void LLIK::Joint::transformTargetsToParentLocal(std::vector<LLVector3>& local_targets) const
{
    if (mParent)
    {
        LLQuaternion world_to_parent = mParent->mRot;
        world_to_parent.conjugate();
        for (auto& target : local_targets)
        {
            LLVector3 world_target = (mPos + target * mRot) - mParent->mPos;
            target = world_target * world_to_parent;
        }
    }
}

bool LLIK::Joint::swingTowardTargets(const std::vector<LLVector3>& local_targets, const std::vector<LLVector3>& world_targets)
{
    if (localRotLocked())
    {
        // nothing to do
        // but we assume targets are not yet reached and return 'true'
        return true;
    }

    constexpr F32 MIN_SWING_ANGLE = 0.001f * F_PI;
    bool something_changed = false;
    if (hasRotTarget())
    {
        mRot = mConfig->getTargetRot();
        something_changed = true;
    }
    else
    {
        size_t num_targets = local_targets.size();
        LLQuaternion adjustment;
        if (num_targets == 1)
        {
            LLVector3 old_bone = local_targets[0] * mRot;
            LLVector3 new_bone = world_targets[0] - mPos;
            adjustment.shortestArc(old_bone, new_bone);
        }
        else
        {
            adjustment.mQ[VW] = 0.0f;
            for (size_t i = 0; i < num_targets; ++i)
            {
                LLVector3 old_bone = local_targets[i] * mRot;
                LLVector3 new_bone = world_targets[i] - mPos;
                LLQuaternion adj;
                adj.shortestArc(old_bone, new_bone);
                if (adj.mQ[VW] < 0.0f)
                {
                    // negate to keep all arithmetic on the same hypersphere
                    adjustment = adjustment - adj;
                }
                else
                {
                    adjustment = adjustment + adj;
                }
            }
            adjustment.normalize();
        }

        if (!LLQuaternion::almost_equal(adjustment, LLQuaternion::DEFAULT, MIN_SWING_ANGLE))
        {
            // lerp the adjustment instead of using the full rotation
            // this allows swing to distribute along the length of the chain
            constexpr F32 SWING_FACTOR = 0.25f;
            adjustment = lerp(SWING_FACTOR, LLQuaternion::DEFAULT, adjustment);

            // compute mRot
            mRot = mRot * adjustment;
            mRot.normalize();
            something_changed = true;
        }
    }
    if (something_changed)
    {
        DEBUG_LOG_EVENT;

        // compute mLocalRot
        // instead of calling updateLocalRot() which has extra checks
        // unnecessary in this context: we do the math explicitly
        LLQuaternion inv_parentRot = mParent->mRot;
        inv_parentRot.conjugate();
        mLocalRot = mRot * inv_parentRot;
        mLocalRot.normalize();

        if (enforceConstraint())
        {
            applyLocalRot();

            // EXPERIMENTAL
            // we hit the constraint during the swing
            // perhaps some twist can get us closer
            //twistTowardTargets(local_targets, world_targets); // DISABLED EXPERIMENT
        }
    }
    return something_changed;
}

// EXPERIMENTAL
void LLIK::Joint::twistTowardTargets(const std::vector<LLVector3>& local_targets, const std::vector<LLVector3>& world_targets)
{
    if (!mConstraint->allowsTwist())
    {
        return;
    }
    // always twist about mConstraint->mForward axis
    LLVector3 axis = mConstraint->getForwardAxis() * mRot;
    LLQuaternion adjustment;
    size_t num_targets = local_targets.size();
    if (num_targets == 1)
    {
        // transform to the world-frame with mPos as origin
        LLVector3 local_target = local_targets[0] * mRot;
        LLVector3 world_target = world_targets[0] - mPos;
        F32 target_length = local_target.length();
        constexpr F32 MIN_TARGET_LENGTH = 1.0e-2f;
        if (target_length < MIN_TARGET_LENGTH)
        {
            // bone is too short
            return;
        }

        // remove components parallel to axis
        local_target -= (local_target * axis) * axis;
        world_target -= (world_target * axis) * axis;

        if (local_target * world_target < 0.0f)
        {
            // this discrepancy is better served with a swing
            return;
        }

        F32 radius = local_target.length();
        constexpr F32 MIN_RADIUS_FRACTION = 1.0e-2f;
        const F32 MIN_RADIUS = MIN_RADIUS_FRACTION * target_length;
        if (radius < MIN_RADIUS || world_target.length() < MIN_RADIUS)
        {
            // twist movement too small to bother
            return;
        }

        // compute the adjustment
        adjustment.shortestArc(local_target, world_target);
    }
    else
    {
        adjustment.mQ[VW] = 0.0f;
        U32 num_adjustments = 0;
        for (size_t i = 0; i < local_targets.size(); ++i)
        {
            LLQuaternion adj;
            // transform to the world-frame with mPos as origin
            LLVector3 local_target = local_targets[i] * mRot;
            LLVector3 world_target = world_targets[i] - mPos;
            F32 target_length = local_target.length();
            constexpr F32 MIN_TARGET_LENGTH = 1.0e-2f;
            if (target_length < MIN_TARGET_LENGTH)
            {
                // bone is too short
                adjustment = adjustment + adj;
                return;
            }

            // remove components parallel to axis
            local_target -= (local_target * axis) * axis;
            world_target -= (world_target * axis) * axis;

            if (local_target * world_target < 0.0f)
            {
                // this discrepancy is better served with a swing
                adjustment = adjustment + adj;
                return;
            }

            F32 radius = local_target.length();
            constexpr F32 MIN_RADIUS_FRACTION = 1.0e-2f;
            const F32 MIN_RADIUS = MIN_RADIUS_FRACTION * target_length;
            if (radius < MIN_RADIUS || world_target.length() < MIN_RADIUS)
            {
                // twist movement will be too small
                adjustment = adjustment + adj;
                return;
            }

            // compute the adjustment
            adj.shortestArc(local_target, world_target);
            adjustment = adjustment + adj;
            ++num_adjustments;
        }
        if (num_adjustments == 0)
        {
            return;
        }
        adjustment.normalize();
    }

    // lerp the adjustment instead of using the full rotation
    // this allows twist to distribute along the length of the chain
    constexpr F32 TWIST_BLEND = 0.4f;
    adjustment = lerp(TWIST_BLEND, LLQuaternion::DEFAULT, adjustment);

    // compute mRot
    mRot = mRot * adjustment;
    mRot.normalize();
    DEBUG_LOG_EVENT;

    // compute mLocalRot
    // instead of calling updateLocalRot() which has extra checks
    // unnecessary in this context: we do the math explicitly
    LLQuaternion inv_parentRot = mParent->mRot;
    inv_parentRot.conjugate();
    mLocalRot = mRot * inv_parentRot;
    mLocalRot.normalize();

    if (enforceConstraint())
    {
        applyLocalRot();
    }
}

void LLIK::Joint::untwist()
{
    if (hasRotTarget())
    {
        mRot = mConfig->getTargetRot();
        updateLocalRot();
        DEBUG_LOG_EVENT;
    }
    else if (!localRotLocked())
    {
        // compute new_local_rot
        LLQuaternion new_local_rot = LLQuaternion::DEFAULT;
        if (mConstraint && !(hasDisabledConstraint()))
        {
            new_local_rot = mConstraint->minimizeTwist(mLocalRot);
        }
        else
        {
            LLVector3 bone = mBone;
            bone.normalize();
            LLVector3 new_bone = bone * mLocalRot;
            LLVector3 swing_axis = bone % new_bone;
            constexpr F32 MIN_SWING_AXIS_LENGTH = 1.0e-3f;
            if (swing_axis.length() > MIN_SWING_AXIS_LENGTH)
            {
                F32 swing_angle = std::acos(new_bone * bone);
                new_local_rot.setAngleAxis(swing_angle, swing_axis);
            }
        }

        // blend toward new_local_rot
        constexpr F32 UNTWIST_BLEND = 0.25f;
        mLocalRot = lerp(UNTWIST_BLEND, mLocalRot, new_local_rot);
        // Note: if UNTWIST_BLEND is increased here the consequence will be
        // more noticeable occasional pops in some joints. It is an interaction
        // with transitions in/out of the
        //     if (swing_axis.length() > MIN_SWING_AXIS_LENGTH)
        // condition above.

        // apply new mLocalRot
        LLQuaternion new_rot = mLocalRot * mParent->mRot;
        if (!mParent->localRotLocked())
        {
            // check to see if new mLocalRot would change world-frame bone
            // (which only happens for some Constraints)
            LLVector3 old_bone = mBone * mRot;
            LLVector3 new_bone = mBone * new_rot;
            constexpr F32 MIN_DELTA_COEF = 0.01f;
            if ((new_bone - old_bone).length() > MIN_DELTA_COEF * mBone.length())
            {
                // The new mLocalRot would change the world-frame bone direction
                // so we counter-rotate mParent to compensate.

                // compute axis of correction
                LLVector3 axis = mParent->mBone * mParent->mRot;
                axis.normalize();

                // project child bones to plane
                old_bone = old_bone - (old_bone * axis) * axis;
                new_bone = new_bone - (new_bone * axis) * axis;

                // compute correction from new_bone back to old_bone
                LLQuaternion twist;
                twist.shortestArc(new_bone, old_bone);

                // compute new parent rot
                DEBUG_SET_CONTEXT(pre_untwist_parent);
                LLQuaternion new_parent_rot = mParent->mRot * twist;
                new_parent_rot.normalize();
                mParent->setWorldRot(new_parent_rot);
                mParent->updateLocalRot();

                // compute new rot
                new_rot = mLocalRot * mParent->mRot;
            }
        }
        mRot = new_rot;
        mRot.normalize();
        DEBUG_LOG_EVENT;
    }
}

//LLIK::Solver class

void LLIK::Solver::setSubBaseIds(const std::set<S16>& ids)
{
    mSubBaseIds = ids;
}

void LLIK::Solver::setSubRootIds(const std::set<S16>& ids)
{
    mSubRootIds = ids;
}

bool LLIK::Solver::isSubBase(S16 joint_id) const
{
    // Sometimes we can't rely on the skeleton topology to determine
    // whether a Joint is a sub-base or not.  So so we offer this workaround:
    // outside logic can supply a whitelist of sub-base ids.
    //
    return mSubBaseIds.find(joint_id) != mSubBaseIds.end();
}

bool LLIK::Solver::isSubRoot(S16 joint_id) const
{
    return !mSubRootIds.empty() && mSubRootIds.find(joint_id) != mSubRootIds.end();
}

LLIK::Solver::Solver() : mRootID(-1)
{
#ifdef DEBUG_LLIK_UNIT_TESTS
    mDebugEnabled = false;
    gDebugEnabled = mDebugEnabled;
#endif
}

void LLIK::Solver::resetSkeleton()
{
    mSkeleton.begin()->second->resetRecursively();
}

// compute the offset from the "tip" of from_id to the "end" of to_id
// or the negative when from_id > to_id
LLVector3 LLIK::Solver::computeReach(S16 to_id, S16 from_id) const
{
    S16 ancestor = from_id;
    S16 descendent = to_id;
    bool swapped = false;
    if (ancestor > descendent)
    {
        ancestor = to_id;
        descendent = from_id;
        swapped = true;
    }
    LLVector3 reach = LLVector3::zero;

    // start at descendent and traverse up the limb
    // until we find the ancestor
    joint_map_t::const_iterator itr = mSkeleton.find(descendent);
    if (itr != mSkeleton.end())
    {
        Joint::ptr_t joint = itr->second;
        LLVector3 chain_reach = joint->getBone();
        while (joint)
        {
            chain_reach += joint->getLocalPos();
            joint = joint->getParent();
            if (joint && joint->getID() == ancestor)
            {
                // success!
                reach = chain_reach;
                break;
            }
        }
    }
    if (swapped)
    {
        reach = - reach;
    }
    return reach;
}

void LLIK::Solver::addJoint(
        S16 joint_id,
        S16 parent_id,
        const LLVector3& local_pos,
        const LLVector3& bone,
        const Constraint::ptr_t& constraint)
{
    // Note: parent Joints must be added BEFORE their children.
    if (joint_id < 0)
    {
        LL_WARNS("Puppet") << "failed to add invalid joint_id=" << joint_id << LL_ENDL;
        return;
    }
    joint_map_t::iterator itr = mSkeleton.find(joint_id);
    if (itr != mSkeleton.end())
    {
        LL_WARNS("Puppet") << "failed to add joint_id=" << joint_id << ": already exists" << LL_ENDL;
        return;
    }

    Joint::ptr_t parent;
    itr = mSkeleton.find(parent_id);
    if (itr == mSkeleton.end())
    {
        if (parent_id >= mRootID)
        {
            LL_WARNS("Puppet") << "failed to add joint_id=" << joint_id << ": could not find parent_id=" << parent_id << LL_ENDL;
            return;
        }
    }
    else
    {
        parent = itr->second;
    }
    Joint::ptr_t joint = std::make_shared<Joint>(joint_id, local_pos, bone);
    joint->setParent(parent);
    if (parent)
    {
        parent->addChild(joint);
    }
    mSkeleton.insert({joint_id, joint});

    joint->setConstraint(constraint);
}

void LLIK::Solver::addWristID(S16 wrist_id)
{
    auto joint_itr = mSkeleton.find(wrist_id);
    if (joint_itr == mSkeleton.end())
    {
        LL_INFOS("LLIK") << "failed to find wrist_id=" << wrist_id << LL_ENDL;
        return;
    }
    mWristJoints.push_back(joint_itr->second);
}

/*
// EXPERIMENTAL: keep this
void LLIK::Solver::adjustTargets(joint_config_map_t& targets)
{
    // When an end-effector has both target_position and target_orientation
    // the IK problem can be reduced by giving the parent a target_position.
    // We scan targets for such conditions and when found: add/update the
    // parent's Target with target_position.
    for (auto& data_pair : targets)
    {
        Joint::Config& target = data_pair.second;
        U8 mask = target.getFlags();
        if (!target.hasWorldPos() || target.hasLocalRot() || !target.hasWorldRot())
        {
            // target does not match our needs
            continue;
        }

        S16 id = data_pair.first;
        auto joint_itr = mSkeleton.find(id);
        if (joint_itr == mSkeleton.end())
        {
            // joint doesn't exist
            continue;
        }

        Joint::ptr_t& joint = joint_itr->second;
        const Joint::ptr_t& parent = joint->getParent();
        if (!parent)
        {
            // no parent
            continue;
        }

        // compute parent's target pos
        // Note: we assume joint->mLocalPos == parent_joint->mBone
        // (e.g. parent's end is same position as joint's tip)
        // which is not true in general, but is true for elbow->wrist.
        LLVector3 parent_target_pos = target.getPos() - joint->getBone() * target.getRot();

        auto parent_target_itr = targets.find(parent->getID());
        if (parent_target_itr != targets.end())
        {
            // parent already has a target --> modify it
            parent_target_itr->second.setPos(parent_target_pos);
        }
        else
        {
            // parent doesn't have a target yet --> give it one
            Joint::Config parent_target;
            parent_target.setPos(parent_target_pos);
            targets.insert({parent->getID(), parent_target});
        }
        // delegate joint's target but set the joint active
        // The joint's world transform will be updated during the IK iterations
        // after all chains have been processed.
        target.delegate();
        joint->activate();
    }
}
*/

// The Skeleton relaxes toward the T-pose and the IK solution will tend to put
// the elbows higher than normal for a humanoid character.  The dropElbow()
// method tries to orient the elbows lower to achieve a more natural pose.
//
void LLIK::Solver::dropElbow(const Joint::ptr_t& wrist_joint)
{
    const Joint::ptr_t& elbow_joint = wrist_joint->getParent();
    const Joint::ptr_t& shoulder_joint = elbow_joint->getParent();
    if (shoulder_joint->hasPosTarget())
    {
        // remember: end-of-shoulder is tip-of-elbow
        // Assume whoever is setting the shoulder's target position
        // knows what they are doing.
        return;
    }

    DEBUG_SET_PHASE(drop_elbow);
    // compute some geometry
    LLVector3 shoulder_tip = shoulder_joint->getWorldTipPos();
    LLVector3 elbow_tip = elbow_joint->getWorldTipPos();
    LLVector3 elbow_end = elbow_joint->computeWorldEndPos();
    LLVector3 axis = elbow_end - shoulder_tip;
    axis.normalize();

    // compute rotation of shoulder to bring upper-arm down
    LLVector3 down = (LLVector3::z_axis % axis) % axis;
    LLVector3 shoulder_bone = elbow_tip - shoulder_tip;
    LLVector3 projection = shoulder_bone - (shoulder_bone * axis) * axis;
    LLQuaternion adjustment;
    adjustment.shortestArc(projection, down);

    // adjust shoulder to bring upper-arm down
    DEBUG_SET_CONTEXT(shoulder);
    shoulder_joint->adjustWorldRot(adjustment);

    // elbow_joint's mLocalRot remains unchanged,
    // but we need to update its world-frame transforms
    DEBUG_SET_CONTEXT(elbow);
    elbow_joint->updatePosAndRotFromParent();

    if (wrist_joint->isActive())
    {
        // in theory: only wrist_joint's mLocalRot has changed,
        // not its world-frame transform
        wrist_joint->updateLocalRot();

        // TODO?: enforce twist of wrist's Constraint
        // and back-rotate the elbow-drop to compensate
    }
}

bool LLIK::Solver::updateJointConfigs(const joint_config_map_t& configs)
{
    bool something_changed = false;
    //Check to see if configs changed since last iteration.
    if (configs.size() == mJointConfigs.size())
    {
        for (const auto& data_pair : mJointConfigs)
        {
            joint_config_map_t::const_iterator itr = configs.find(data_pair.first);
            if (itr != configs.end())
            { //Found old target in current configs.
                const Joint::Config& old_target = data_pair.second;
                const Joint::Config& new_target = itr->second;
                U8 mask = old_target.getFlags();
                if (mask != new_target.getFlags())
                {
                    something_changed = true;
                    break;
                }
                if ((mask & FLAG_TARGET_POS) > 0
                        && std::abs(dist_vec(old_target.getTargetPos(), new_target.getTargetPos())) > mAcceptableError)
                {
                    something_changed = true;
                    break;
                }
                if ((mask & FLAG_TARGET_ROT) > 0
                            && !LLQuaternion::almost_equal(old_target.getTargetRot(), new_target.getTargetRot()))
                {
                    something_changed = true;
                    break;
                }
                if ((mask & FLAG_LOCAL_POS) > 0
                        && std::abs(dist_vec(old_target.getLocalPos(), new_target.getLocalPos())) > mAcceptableError)
                {
                    something_changed = true;
                    break;
                }
                if ((mask & FLAG_LOCAL_ROT) > 0
                            && !LLQuaternion::almost_equal(old_target.getLocalRot(), new_target.getLocalRot()))
                {
                    something_changed = true;
                    break;
                }
            }
            else
            {
                something_changed = true;
                break;
            }
        }
    }
    else
    {
        something_changed = true;
    }
    if (something_changed)
    {
        mJointConfigs = configs;
        rebuildAllChains();
    }
    return something_changed;
}

void LLIK::Solver::rebuildAllChains()
{
    // before recompute chains: clear active status on old chains
    for (const auto& data_pair : mChainMap)
    {
        const joint_list_t& chain = data_pair.second;
        for (const Joint::ptr_t& joint : chain)
        {
            joint->resetFlags();
        }
    }
    mChainMap.clear();
    mActiveRoots.clear();

    // makeChains
    //
    // Consider the following hypothetical skeleton, where each Joint tip
    // has a numerical ID and each end-effector tip is denoted with
    // bracketed [ID]:
    //                     8             [11]
    //                    /              /
    //                   7---14--[15]   10
    //                  /              /
    //                 6---12---13    9
    //                /              /
    //      0----1---2----3----4---[5]--16---17--[18]
    //                \
    //                 19
    //                  \
    //                  [20]
    //
    // The target ID list is: [5,11,15,18,20].
    // IK would need to solve all joints except for [8,12,13].
    // In other words: all Joints are "active" except [8,12,13].
    //
    // We divide the Skeleton into "chain segments" that start at a targeted
    // Joint and continue up until: root (0), end-effector ([ID]), or
    // sub-base (Joint with multiple children).
    //
    // Inward passes operate on the Chains in order such that when it is time
    // to update a sub-base all of its active children will have already been
    // updated: it will be able to compute the centroid of its mWorldEndPos.
    //
    // Outward passes also only operate on the Chains.  This simplifies
    // the logic because there will be no need to check for target or sub-base
    // until the end of a Chain is reached.  Any Joint not on a Chain (e.g.
    // non-active) will keep its parent-relative rotation.
    //
    // The initial chain list would be:
    //     {  5:[5,4,3,2]
    //       11:[11,10,9,5]
    //       15:[15,14,7]
    //       18:[18,17,16,5]
    //       20:[20,19,2] }
    // Where all chains include their end_point and also sub-base.
    // The remaining active non-targeted sub_base_map would be:
    //     { 2:[2,1,0]
    //       7:[7,6]
    //       6:[6,2] }
    // In this scenario Joints (6) and (7) are "false" sub-bases: they
    // don't have targets and have multiple children but only one of them is "active".
    // We can condense the chains to be:
    //     {  5:[5,4,3,2]
    //       11:[11,10,9,5]
    //       15:[15,14,7,6,2]
    //       18:[18,17,16,5]
    //       20:[20,19,2] }
    // and:
    //     { 2:[2,1,0] }
    //

    std::set<S16> sub_bases;
    // mJointConfigs is sorted by joint_id low-to-high
    // and we rely on this in buildChain().
    for (auto& data_pair : mJointConfigs)
    {
        // make sure joint_id is valid
        S16 joint_id = data_pair.first;
        joint_map_t::iterator itr = mSkeleton.find(joint_id);
        if (itr == mSkeleton.end())
        {
            continue;
        }
        Joint::ptr_t joint = itr->second;

        // Joint caches a pointer to the Target.
        // This is OK because both Joint and Target are managed by this Solver
        // and the Joint::Config will remain valid for the duration of the IK iterations.
        Joint::Config& config = data_pair.second;
        joint->setConfig(config);

        if (joint->getID() == mRootID)
        {
            // for root: world-frame == local-frame
            U8 flags = joint->getConfigFlags();
            if (flags & MASK_ROT)
            {
                LLQuaternion q;
                if (flags & FLAG_LOCAL_ROT)
                {
                    q = config.getLocalRot();
                }
                else
                {
                    q = config.getTargetRot();
                }
                joint->lockLocalRot(q);
                joint->activate();
                mActiveRoots.insert(joint);
            }
            if (flags & MASK_POS)
            {
                LLVector3 p;
                if (flags & FLAG_LOCAL_POS)
                {
                    p = config.getLocalPos();
                }
                else
                {
                    p = config.getTargetPos();
                }
                joint->setLocalPos(p);
                joint->activate();
            }
            continue;
        }

        if (config.hasLocalRot())
        {
            joint->lockLocalRot(config.getLocalRot());
        }

        // EXPERIMENTAL: keep this
        if (config.hasDelegated())
        {
            // don't build chain for delegated Target
            continue;
        }

        if (config.hasTargetPos())
        {
            // add and build chain
            mChainMap[joint_id] = joint_list_t();
            buildChain(joint, mChainMap[joint_id], sub_bases);

            //HACK or FIX?  If we have sequential end effectors, we are not guaranteed the expression
            //module has sent us positions that can be solved.  We will instead assume that the child's
            //position is higher prioriy than the parent, get direction from child to parent and move the
            //parent's target to the exact bone length.
            //TODO:  Will not work correctly for a parent with multiple direct children with effector targets.
            //Because we create the targets form low to high we will know if the parent is an end-effector.
            Joint::ptr_t parent = joint->getParent();
            if (parent->hasPosTarget())
            { //Sequential targets detected
                LLVector3 child_target_pos = config.getTargetPos();
                LLVector3 parent_target_pos = parent->getTargetPos();
                LLVector3 direction = parent_target_pos - child_target_pos;
                direction.normalize();
                direction *= joint->getLocalPosLength();
                parent_target_pos = child_target_pos + direction;
                parent->setTargetPos(parent_target_pos);
            }
        }
        else if (config.hasLocalPos())
        {
            joint->setLocalPos(config.getLocalPos());
            joint->activate();
        }
    }

    // each sub_base gets its own Chain
    while (sub_bases.size() > 0)
    {
        std::set<S16> new_sub_bases;
        for (S16 joint_id : sub_bases)
        {
            // add and build chain
            Joint::ptr_t joint = mSkeleton[joint_id];
            mChainMap[joint_id] = joint_list_t();
            buildChain(joint, mChainMap[joint_id], new_sub_bases);
        }
        sub_bases = std::move(new_sub_bases);
    }

    // eliminate "false" sub-bases and condense the Chains
    // search for Chain-joins
    std::vector<U16> joins;
    for (const auto& data_pair : mChainMap)
    {
        const Joint::ptr_t& outer_end = data_pair.second[0];
        if (!outer_end->hasPosTarget()
                && !isSubBase(outer_end->getID()))
        {
            Joint::ptr_t active_child = outer_end->getSingleActiveChild();
            if (active_child)
            {
                // outer_end doesn't have a target, isn't flagged as subbase,
                // and has only one active_child
                // --> it is a "false" sub-base and we will try to "join"
                // this Chain to another
                joins.push_back(outer_end->getID());
            }
        }
    }
    // make the joins
    for (U16 id : joins)
    {
        // hunt for recipient chain
        for (auto& data_pair : mChainMap)
        {
            auto& recipient = data_pair.second;
            const Joint::ptr_t& inner_end = recipient[recipient.size() - 1];
            if (inner_end->getID() == id)
            {
                // copy donor to recipient
                const auto& donor = mChainMap[id];
                recipient.insert(recipient.end(), ++(donor.begin()), donor.end());
                // erase donor
                mChainMap.erase(id);
                break;
            }
        }
    }

    // cache the set of active branch roots
    for (auto& data_pair : mChainMap)
    {
        auto& chain = data_pair.second;
        size_t last_index = chain.size() - 1;
        Joint::ptr_t chain_base = chain[last_index];
        Joint::ptr_t parent = chain_base->getParent();
        if (!parent || !parent->isActive())
        {
            mActiveRoots.insert(chain_base);
        }
    }

#ifdef DEBUG_LLIK_UNIT_TESTS
    if (mDebugEnabled)
    {
        std::cout << "joint_configs=[" << std::endl;
        for (const auto& data_pair : mJointConfigs)
        {
            const Joint::Config& config = data_pair.second;
            std::cout << "    {";
            std::cout << "'id':" << data_pair.first;
            if (config.hasTargetPos())
            {
                const LLVector3& p = config.getTargetPos();
                std::cout << ",'p':(" << p.mV[VX] << "," << p.mV[VY] << "," << p.mV[VZ] << ")";
            }
            else if (config.hasLocalPos())
            {
                const LLVector3& p = config.getLocalPos();
                std::cout << ",'P':(" << p.mV[VX] << "," << p.mV[VY] << "," << p.mV[VZ] << ")";
            }
            if (config.hasTargetRot())
            {
                const LLQuaternion& q = config.getTargetRot();
                std::cout << ",'q':(" << q.mQ[VX] << "," << q.mQ[VY] << "," << q.mQ[VZ] << "," << q.mQ[VW] << ")";
            }
            else if (config.hasLocalRot())
            {
                const LLQuaternion& q = config.getLocalRot();
                std::cout << ",'Q':(" << q.mQ[VX] << "," << q.mQ[VY] << "," << q.mQ[VZ] << "," << q.mQ[VW] << ")";
            }
            std::cout << "},\n";
        }
        std::cout << "]" << std::endl;
    }
#endif

    // cache the list of all active joints
    mActiveJoints.clear();
    for (auto& data_pair : mSkeleton)
    {
        if (data_pair.second->isActive())
        {
            mActiveJoints.push_back(data_pair.second);
        }
    }
}

////////////////////////////////////LLIK::Solver/////////////////////////////
//////////////////////////////////// Solvers /////////////////////////////

F32 LLIK::Solver::configureAndSolve(const joint_config_map_t& configs)
{
    if (!updateJointConfigs(configs))
    {
        // nothing changed --> nothing to do
        return mLastError;
    }

    // Before each solve: we relax a fraction toward the reset pose.
    // This provides return pressure that removes floating-point drift that would
    // otherwise wander around within the valid zones of the constraints.
    constexpr F32 INITIAL_RELAXATION_FACTOR = 0.25f;
    for (auto& root : mActiveRoots)
    {
        root->relaxRotationsRecursively(INITIAL_RELAXATION_FACTOR);
    }

#ifdef DEBUG_LLIK_UNIT_TESTS
    if (mDebugEnabled)
    {
        if (!gConfigLogged)
        {
            dumpConfig();
            gConfigLogged = true;
        }
        std::cout << "initial_data = [" << std::endl;
        dumpActiveState();
        std::cout << "]" << std::endl;
        std::cout << "solution_data = [" << std::endl;

        // when plotting the results it helps to know the bounds of the data
        F32 A = std::numeric_limits<F32>::min();
        F32 Z = std::numeric_limits<F32>::max();
        mMinPos= LLVector3(Z, Z, Z);
        mMaxPos = LLVector3(A, A, A);
        for (const auto& data_pair : mJointConfigs)
        {
            const Joint::Config& target = data_pair.second;
            if (target.hasTargetPos())
            {
                updateBounds(target.getTargetPos());
            }
        }
        for (const auto& data_pair : mSkeleton)
        {
            const Joint::ptr_t& joint = data_pair.second;
            updateBounds(joint->getWorldTipPos());
            updateBounds(joint->computeWorldEndPos());
        }
    }
#endif

    // start with one FABRIK pass (does not enforce constraints)
    executeFabrikPass();

    constexpr U32 MAX_FABRIK_ITERATIONS = 16;
    constexpr U32 MIN_FABRIK_ITERATIONS = 4;
    F32 max_error = std::numeric_limits<F32>::max();
    for (U32 loop = 0; loop < MIN_FABRIK_ITERATIONS || (loop < MAX_FABRIK_ITERATIONS && max_error > mAcceptableError); ++loop)
    {
#ifdef DEBUG_LLIK_UNIT_TESTS
        if (mDebugEnabled) { std::cout << "    ('loop'," << loop << ")," << std::endl; }
#endif

        // pull elbows downward toward a more natual pose
        for (const auto& wrist_joint : mWristJoints)
        {
            dropElbow(wrist_joint);
        }

        // since our FABRIK implementation doesn't enforce constraints
        // during the forward/backward passes we do it here
        enforceConstraintsOutward();

        // It is often possible to remove excess twist between the Joints
        // without swinging their bones in the world-frame.  We try this now
        // to help reduce the "spin drift" that can occur where Joint
        // orientations pick up systematic and floating-point errors and
        // drift within the twist-limits of their constraints.
        for (const auto& data_pair : mChainMap)
        {
            const joint_list_t& chain = data_pair.second;
            untwistChain(chain);
        }

        executeFabrikPass();

        // Note: we don't bother enforcing constraints after this FABRIK pass.
        // The algorithm is expected to converge so final constraint violations
        // should be small.
        max_error = measureMaxError();
    }
    mLastError = max_error;

#ifdef DEBUG_LLIK_UNIT_TESTS
    if (mDebugEnabled)
    {
        std::cout << "]\n";
        // we're using Python's matplotlib for visualizing the data
        // and it helps to supply the min/max limits for automatic boxing
        std::cout << "xlim = [" << mMinPos.mV[VX] << "," << mMaxPos.mV[VX] << "]\n";
        std::cout << "ylim = [" << mMinPos.mV[VY] << "," << mMaxPos.mV[VY] << "]\n";
        std::cout << "zlim = [" << mMinPos.mV[VZ] << "," << mMaxPos.mV[VZ] << "]" << std::endl;
    }
#endif
    return mLastError;
}

LLVector3 LLIK::Solver::getJointLocalPos(S16 joint_id) const
{
    LLVector3 pos;
    auto itr = mSkeleton.find(joint_id);
    if (itr != mSkeleton.end())
    {
        pos = itr->second->getLocalPos();
    }
    return pos;
}

bool LLIK::Solver::getJointLocalTransform(S16 joint_id, LLVector3& pos, LLQuaternion& rot) const
{
    auto itr = mSkeleton.find(joint_id);
    if (itr != mSkeleton.end())
    {
        pos = itr->second->getLocalPos();
        rot = itr->second->getLocalRot();
        return true;
    }
    return false;
}

LLVector3 LLIK::Solver::getJointWorldEndPos(S16 joint_id) const
{
    LLVector3 pos;
    joint_map_t::const_iterator itr = mSkeleton.find(joint_id);
    if (itr != mSkeleton.end())
    {
        pos = itr->second->computeWorldEndPos();
    }
    return pos;
}

LLQuaternion LLIK::Solver::getJointWorldRot(S16 joint_id) const
{
    LLQuaternion rot;
    joint_map_t::const_iterator itr = mSkeleton.find(joint_id);
    if (itr != mSkeleton.end())
    {
        rot = itr->second->getWorldRot();
    }
    return rot;
}

void LLIK::Solver::reconfigureJoint(S16 joint_id, const LLVector3& local_pos, const LLVector3& bone, const Constraint::ptr_t& constraint)
{
    joint_map_t::iterator itr = mSkeleton.find(joint_id);
    if (itr == mSkeleton.end())
    {
        LL_WARNS("Puppet") << "failed reconfigure unknown joint_id=" << joint_id << LL_ENDL;
        return;
    }
    const Joint::ptr_t& joint = itr->second;
    joint->reconfigure(local_pos, bone);
    joint->setConstraint(constraint);
    // Note: will need to call computeEffectorNormal() after all Joints are reconfigured.
}

void LLIK::Solver::buildChain(Joint::ptr_t joint, joint_list_t& chain, std::set<S16>& sub_bases)
{
    // Builds a Chain in descending order (inward) from end-effector or
    // sub-base.  Stops at next end-effector (has target), sub-base (more than
    // one active child), or root.
    // Side effect: sets each Joint on chain "active".
    chain.push_back(joint);
    joint->activate();
    // Walk up the chain of ancestors and add to chain but stop at: end-effector,
    // sub-base, or root.  When a sub-base is encountered push its id
    // onto sub_bases.
    joint = joint->getParent();
    while(joint)
    {
        chain.push_back(joint);
        joint->activate();
        S16 joint_id = joint->getID();
        // Yes, add the joint to the chain before the break checks below
        // because we want to include the final joint (e.g. root, sub-base,
        // or previously targeted joint) at the end of the chain.
        if (isSubRoot(joint_id))
        {
            //AURA hack to deal with lack of constraints in spine
            break;
        }
        if (joint_id == mRootID)
        {
            break;
        }
        if (joint->hasPosTarget())
        {
            // truncate this chain at targeted ancestor joint
            break;
        }
        if ((mSubBaseIds.empty() && joint->getNumChildren() > 1)
            || isSubBase(joint_id))
        {
            sub_bases.insert(joint_id);
            break;
        }
        joint = joint->getParent();
    }
}

void LLIK::Solver::executeFabrikInward(const joint_list_t& chain)
{
    DEBUG_SET_CONTEXT(inward);
    // chain starts at end-effector or sub-base.
    // Don't forget: chain is organized in descending order:
    // for inward pass we traverse the chain forward.

    // outer end of chain is special: it either has a target
    // or is a sub-base with active children
    chain[0]->updateEndInward();

    // traverse Chain forward
    // Skip first Joint in chain (the "outer end"): we just handled it.
    // Also skip last Joint in chain (the "inner end"): it is either
    // the outer end of another chain (and will be updated then) or
    // it is one of the "active roots" and will be handled after all chains.
    S32 last_index = (S32)(chain.size()) - 1;
    for (S32 i = 1; i < last_index; ++i)
    {
        chain[i]->updateInward(chain[i-1]);
    }
}

void LLIK::Solver::executeFabrikOutward(const joint_list_t& chain)
{
    DEBUG_SET_CONTEXT(outward);
    // chain starts at a end-effector or sub-base.
    // Don't forget: chain is organized in descending order:
    // for outward pass we traverse the chain in reverse.
    S32 last_index = (S32)(chain.size()) - 1;

    // skip the Joint at last_index:
    // chain's inner-end doesn't move at this stage.
    //
    // traverse the middle of chain in reverse
    for (S32 i = last_index - 1; i > 0; --i)
    {
        chain[i]->updateOutward();
    }

    // outer end of chain is special: it either has a target
    // or is a sub-base with active children
    chain[0]->updateEndOutward();
}

void LLIK::Solver::shiftChainToBase(const joint_list_t& chain)
{
    S32 last_index = (S32)(chain.size()) - 1;
    const Joint::ptr_t& inner_end_child = chain[last_index - 1];
    LLVector3 offset = inner_end_child->computeWorldTipOffset();
    if (offset.lengthSquared() > mAcceptableError * mAcceptableError)
    {
        for (size_t i = 0; i < last_index; ++i)
        {
            chain[i]->shiftPos(-offset);
        }
    }
}

void LLIK::Solver::executeFabrikPass()
{
    // FABRIK = Forward And Backward Reching Inverse Kinematics
    // http://andreasaristidou.com/FABRIK.html

    DEBUG_SET_PHASE(FABRIK);
    // mChainMap is sorted by outer_end joint_id, low-to-high so for the inward pass
    // we traverse the chains in reverse order.
    for (chain_map_t::const_reverse_iterator itr = mChainMap.rbegin(); itr != mChainMap.rend(); ++itr)
    {
        executeFabrikInward(itr->second);
    }

    // executeFabrikInward() doesn't update child mLocalRots for the inner_end
    // so we must do it manually for each active root
    for (auto& root : mActiveRoots)
    {
        root->updateChildLocalRots();
    }

    // The outward pass must solve the combined set of chains from-low-to-high
    // so we process them in forward order.
    DEBUG_SET_CONTEXT(outward);
    for (const auto& data_pair : mChainMap)
    {
        const joint_list_t& chain = data_pair.second;
        executeFabrikOutward(chain);
    }
}

void LLIK::Solver::enforceConstraintsOutward()
{
    for (const auto& data_pair : mChainMap)
    {
        const joint_list_t& chain = data_pair.second;

        // chain starts at a end-effector or sub-base.
        // Don't forget: chain is organized in descending order:
        // for outward pass we traverse the chain in reverse.
        S32 last_index = (S32)(chain.size()) - 1;

        // skip the Joint at last_index:
        // chain's inner-end doesn't move at this stage.

        // traverse the middle of chain in reverse
        for (S32 i = last_index - 1; i > -1; --i)
        {
            const Joint::ptr_t& joint = chain[i];
            joint->updatePosAndRotFromParent();
            if (joint->enforceConstraint())
            {
                joint->applyLocalRot();
            }
        }
        chain[0]->updateChildLocalRots();
    }
}

// EXPERIMENTAL: keep this even though it is not being used.
//
// Cyclic Coordinate Descend (CCD) is an alternative IK algorithm.
// http://rodolphe-vaillant.fr/entry/114/cyclic-coordonate-descent-inverse-kynematic-ccd-ik
//
// It converges well however is more susceptible than FABRIK to solution
// instability when Constraints are being enforced. We keep it around just in
// case we want to try it, or for when we figure out how to enforce Constraints
// without making CCD unstable.
void LLIK::Solver::executeCcdPass()
{
    DEBUG_SET_PHASE(CCD);
    // mChainMap is sorted by outer_end joint_id, low-to-high
    // and CCD is an inward pass, so we traverse the map in reverse
    for (chain_map_t::const_reverse_iterator itr = mChainMap.rbegin(); itr != mChainMap.rend(); ++itr)
    {
        executeCcdInward(itr->second);
    }

    // executeCcdInward(chain) recomputes world-frame transform of all Joints in chain
    // ... except the child of the chain's inner_end.  Now that all chains are solved
    // we shift each chain to connect with its sub-base
    DEBUG_SET_CONTEXT(shift_to_base);
    for (auto& data_pair: mChainMap)
    {
        shiftChainToBase(data_pair.second);
    }
}

void LLIK::Solver::executeCcdInward(const joint_list_t& chain)
{
    // 'chain' starts at a end-effector or sub-base.
    // Don't forget: 'chain' is organized in descending order:
    // for inward pass we traverse the Chain forward.
    const Joint::ptr_t& outer_end = chain[0];

    // outer_end has one or more targets known in both local- and world- frame.
    // For CCD we'll be swinging each joint of the Chain as we traverse inward
    // in attempts to get the local-frame targets to align with their world-frame counterparts.
    std::vector<LLVector3> local_targets;
    std::vector<LLVector3> world_targets;
    outer_end->collectTargetPositions(local_targets, world_targets);

    DEBUG_SET_CONTEXT(swing);
    if (!outer_end->swingTowardTargets(local_targets, world_targets))
    {
        // targets are close enough
        return;
    }

    // traverse Chain forward and swing each part
    // Skip first Joint in 'chain' (the "outer end"): we just handled it.
    // Also skip last Joint in 'chain' (the "inner end"): it is either
    // the outer end of another Chain (and will be updated as part of a
    // subsequent Chain) or it is one of the "active roots" and is not
    // moved.
    S32 last_index = (S32)(chain.size()) - 1;
    S32 last_swung_index = 0;
    for (S32 i = 1; i < last_index; ++i)
    {
        chain[i-1]->transformTargetsToParentLocal(local_targets);
        if (!chain[i]->swingTowardTargets(local_targets, world_targets))
        {
            break;
        }
        last_swung_index = i;
    }

    // update the world-frame transforms of swung Joints
    DEBUG_SET_CONTEXT(update_world_xforms);
    for (S32 i = last_swung_index - 1; i > -1; --i)
    {
        chain[i]->updatePosAndRotFromParent();
    }

    // finally: make sure to update outer_end's childrens' mLocalRots
    // Note: we don't bother to enforce constraints in this step
    outer_end->updateChildLocalRots();
}

void LLIK::Solver::untwistChain(const joint_list_t& chain)
{
    DEBUG_SET_CONTEXT(untwist);
    S32 last_index = (S32)(chain.size()) - 1;
    // Note: we start at last_index-1 becuase Joint::untwist() will affect its
    // parent's twist and we don't want to mess with the inner_end of the chain
    // since it will be handled later in another chain.
    for (S32 i = last_index - 1; i > -1; --i)
    {
        chain[i]->untwist();
    }
    chain[0]->updateChildLocalRots();
}

F32 LLIK::Solver::measureMaxError()
{
    F32 max_error = 0.0f;
    for (auto& data_pair : mJointConfigs)
    {
        S16 joint_id = data_pair.first;
        if (joint_id == mRootID)
        {
            // skip error measure of root joint: should always be zero
            continue;
        }
        Joint::Config& target = data_pair.second;
        if (target.hasTargetPos() && !target.hasDelegated())
        {
            LLVector3 end_pos = mSkeleton[joint_id]->computeWorldEndPos();
            F32 dist = dist_vec(end_pos, target.getTargetPos());
            if (dist > max_error)
            {
                max_error = dist;
            }
        }
    }
#ifdef DEBUG_LLIK_UNIT_TESTS
    // when plotting the results it helps to know the bounds of the data
    if (mDebugEnabled) {
        std::cout << "    ('max_error'," << max_error << ")," << std::endl;
        for (auto data_pair : mSkeleton)
        {
            const Joint::ptr_t& joint = data_pair.second;
            updateBounds(joint->getWorldTipPos());
            updateBounds(joint->computeWorldEndPos());
        }
    }
#endif
    return max_error;
}

void LLIK::Solver::enableDebugIfPossible()
{
#ifdef DEBUG_LLIK_UNIT_TESTS
    mDebugEnabled = true;
    gDebugEnabled = true;
#endif
}

#ifdef DEBUG_LLIK_UNIT_TESTS
void LLIK::Solver::dumpConfig() const
{
    for (auto joint : mActiveJoints)
    {
        joint->dumpConfig();
    }
}

void LLIK::Solver::dumpActiveState() const
{
    // Use std::cout because we'll debug using stand-alone test app.
    for (const auto& data_pair : mSkeleton)
    {
        const Joint::ptr_t& joint = data_pair.second;
        if (joint->isActive())
        {
            std::cout << "    "; data_pair.second->dumpState(); std::cout << ",\n";
        }
    }
}

void LLIK::Solver::updateBounds(const LLVector3& point)
{
    mMinPos.mV[VX] = std::min(mMinPos.mV[VX], point.mV[VX]);
    mMinPos.mV[VY] = std::min(mMinPos.mV[VY], point.mV[VY]);
    mMinPos.mV[VZ] = std::min(mMinPos.mV[VZ], point.mV[VZ]);
    mMaxPos.mV[VX] = std::max(mMaxPos.mV[VX], point.mV[VX]);
    mMaxPos.mV[VY] = std::max(mMaxPos.mV[VY], point.mV[VY]);
    mMaxPos.mV[VZ] = std::max(mMaxPos.mV[VZ], point.mV[VZ]);
}
#endif // DEBUG_LLIK_UNIT_TESTS

std::shared_ptr<LLIK::Constraint> LLIKConstraintFactory::getConstraint(const LLIK::Constraint::Info& info)
{
    std::string key = info.getString();
    std::map<std::string, std::shared_ptr<LLIK::Constraint> >::const_iterator itr = mConstraints.find(key);
    if (itr != mConstraints.end())
    {
        return itr->second;
    }

    std::shared_ptr<LLIK::Constraint> ptr = create(info);
    if (ptr)
    {
        mConstraints.insert({key, ptr});
    }
    return ptr;
}

// static
std::shared_ptr<LLIK::Constraint> LLIKConstraintFactory::create(const LLIK::Constraint::Info& info)
{
    std::shared_ptr<LLIK::Constraint> ptr;
    switch (info.mType)
    {
        case LLIK::Constraint::Info::SIMPLE_CONE_CONSTRAINT:
            if (info.mVectors.size() > 0 && info.mFloats.size() > 0)
            {
                ptr = std::make_shared<LLIK::SimpleCone>(
                        info.mVectors[0], // forward_axis
                        info.mFloats[0]); // max_angle
            }
        break;
        case LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT:
            if (info.mVectors.size() > 0 && info.mFloats.size() > 2)
            {
                ptr = std::make_shared<LLIK::TwistLimitedCone>(
                        info.mVectors[0], // forward_axis
                        info.mFloats[0], // cone_angle
                        info.mFloats[1], // min_twist
                        info.mFloats[2]); // max_twist
            }
        break;
        case LLIK::Constraint::Info::ELBOW_CONSTRAINT:
            if (info.mVectors.size() > 1 && info.mFloats.size() > 3)
            {
                ptr = std::make_shared<LLIK::ElbowConstraint>(
                        info.mVectors[0], // forward_axis
                        info.mVectors[1], // pivot_axis
                        info.mFloats[0],  // min_bend
                        info.mFloats[1],  // max_bend
                        info.mFloats[2],  // min_twist
                        info.mFloats[3]); // max_twist
            }
        break;
        case LLIK::Constraint::Info::KNEE_CONSTRAINT:
            if (info.mVectors.size() > 1 && info.mFloats.size() > 1)
            {
                ptr = std::make_shared<LLIK::KneeConstraint>(
                        info.mVectors[0], // forward_axis
                        info.mVectors[1], // pivot_axis
                        info.mFloats[0],  // min_bend
                        info.mFloats[1]); // max_bend
            }
        break;
        case LLIK::Constraint::Info::ACUTE_ELLIPSOIDAL_CONE_CONSTRAINT:
            if (info.mVectors.size() > 1 && info.mFloats.size() > 4)
            {
                ptr = std::make_shared<LLIK::AcuteEllipsoidalCone>(
                        info.mVectors[0], // forward_axis
                        info.mVectors[1], // up_axis
                        info.mFloats[0],  // forward
                        info.mFloats[1],  // up
                        info.mFloats[2],  // left
                        info.mFloats[3],  // down
                        info.mFloats[4]   // right
                        );
            }
        break;
        case LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT:
            if (info.mVectors.size() > 1 && info.mFloats.size() > 3)
            {
                ptr = std::make_shared<LLIK::DoubleLimitedHinge>(
                        info.mVectors[0], // forward_axis
                        info.mVectors[1], // up_axis
                        info.mFloats[0],  // min_yaw
                        info.mFloats[1],  // max_yaw
                        info.mFloats[2],  // min_pitch
                        info.mFloats[3]); // max_pitch
            }
        break;
        default:
        break;
    }
    return ptr;
}

