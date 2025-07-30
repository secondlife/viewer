/**
 * @file lljointsolverrp3.h
 * @brief Implementation of LLJointSolverRP3 class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLJOINTSOLVERRP3_H
#define LL_LLJOINTSOLVERRP3_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "lljoint.h"

/**
 * @brief Inverse kinematics solver for 3-joint kinematic chains.
 * 
 * LLJointSolverRP3 is a "poor man's" IK system for simple 3-joint chains,
 * modeled after Maya's ikRPSolver. It's commonly used for limbs like arms
 * and legs where you have shoulder-elbow-wrist or hip-knee-ankle chains.
 * 
 * The solver takes 4 joints:
 * - jointA: Root of the chain (shoulder, hip)
 * - jointB: Middle joint (elbow, knee) 
 * - jointC: End effector (wrist, ankle)
 * - jointGoal: Target position for jointC to reach
 * 
 * When solve() is called, the rotations of jointA and jointB are modified
 * so that jointC attempts to reach the position of jointGoal while preserving
 * the bone lengths between joints.
 * 
 * Visual representation:
 * ```
 *  A          A
 *  |          |
 *  |          |
 *  B          B---CG     A---B---C...G
 *   \                    (goal position)
 *    \
 *     CG
 * ```
 * 
 * Key features:
 * - Preserves bone lengths established during initialization
 * - Uses pole vector to constrain solution plane and avoid flipping
 * - Supports twist parameter for rotating the solution plane
 * - Optional bend axis specification for smarter non-coplanar limb handling
 * 
 * The pole vector serves two purposes:
 * 1. Defines the plane where the solution occurs (reduces infinite solutions to 2)
 * 2. Disambiguates between the two possible solutions
 * 
 * Real usage in Second Life viewer:
 * - LLEditingMotion: Left arm IK for object manipulation (shoulder-elbow-wrist)
 * - LLKeyframeStandMotion: Foot placement IK for ground contact (hip-knee-ankle)
 * - Any procedural animation requiring 3-joint chain positioning
 * 
 * @code
 * // Real usage from LLEditingMotion for left arm IK
 * LLJointSolverRP3 mIKSolver;
 * mIKSolver.setPoleVector(LLVector3(-1.0f, 1.0f, 0.0f));  // Point elbow up and back
 * mIKSolver.setBAxis(LLVector3(-0.682683f, 0.0f, -0.730714f));  // Elbow bend axis
 * mIKSolver.setupJoints(&mShoulderJoint, &mElbowJoint, &mWristJoint, &mTarget);
 * mIKSolver.solve();  // Called each frame during editing
 * @endcode
 */
class LLJointSolverRP3
{
protected:
    /// Root joint of the kinematic chain (e.g., shoulder, hip)
    LLJoint     *mJointA;
    /// Middle joint of the chain (e.g., elbow, knee)
    LLJoint     *mJointB;
    /// End effector joint (e.g., wrist, ankle)
    LLJoint     *mJointC;
    /// Target goal position that jointC should reach
    LLJoint     *mJointGoal;

    /// Cached distance between jointA and jointB (preserved during solving)
    F32         mLengthAB;
    /// Cached distance between jointB and jointC (preserved during solving)
    F32         mLengthBC;

    /// Pole vector defining solution plane and preferred bend direction
    LLVector3   mPoleVector;
    /// Bend axis in jointB's local frame for smarter non-coplanar solving
    LLVector3   mBAxis;
    /// Whether to use the custom bend axis for solving
    bool        mbUseBAxis;

    /// Twist angle in radians for rotating the solution plane
    F32         mTwist;

    /// Flag indicating if this is the first solve() call for initialization
    bool        mFirstTime;
    /// Saved transformation matrix from jointA for reference
    LLMatrix4   mSavedJointAMat;
    /// Saved inverse plane matrix for coordinate transformations
    LLMatrix4   mSavedInvPlaneMat;

    /// Base rotation of jointA before IK solving
    LLQuaternion    mJointABaseRotation;
    /// Base rotation of jointB before IK solving
    LLQuaternion    mJointBBaseRotation;

public:
    /**
     * @brief Default constructor initializes solver to safe defaults.
     * 
     * Creates an uninitialized solver. You must call setupJoints() before
     * using solve().
     */
    LLJointSolverRP3();
    
    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~LLJointSolverRP3();

    /**
     * @brief Configures the 3-joint chain for IK solving.
     * 
     * This must be called once to set up the solver before using solve().
     * Must be called AFTER the skeleton is created with all parent/child
     * relationships established and joints positioned correctly, as the
     * distances between joints will be cached and preserved.
     * 
     * The joint chain must be: jointA -> jointB -> jointC (parent-child links)
     * 
     * Real example from LLEditingMotion:
     * - jointA: mShoulderLeft ("mShoulderLeft")
     * - jointB: mElbowLeft ("mElbowLeft") 
     * - jointC: mWristLeft ("mWristLeft")
     * - jointGoal: mTarget (procedural target position)
     * 
     * @param jointA Root joint of the chain (shoulder, hip, etc.)
     * @param jointB Middle joint of the chain (elbow, knee, etc.)
     * @param jointC End effector joint (wrist, ankle, etc.)
     * @param jointGoal Target position joint that jointC should reach
     */
    void setupJoints(   LLJoint* jointA,
                        LLJoint* jointB,
                        LLJoint* jointC,
                        LLJoint* jointGoal );

    /**
     * @brief Gets the current pole vector.
     * 
     * @return Current pole vector in jointA's parent coordinate space
     */
    const LLVector3& getPoleVector();

    /**
     * @brief Sets the pole vector for constraining the IK solution.
     * 
     * The pole vector defines the preferred direction for the middle joint
     * (jointB) to bend toward. This constrains the infinite possible solutions
     * to a single solution and prevents unwanted flipping between poses.
     * 
     * The pole vector is defined relative to jointA's parent coordinate space.
     * Real usage examples:
     * - (-1,1,0): Used in LLEditingMotion for left arm - points elbow up and back
     * - (1,0,0): Default value - bend toward +X direction
     * - (0,1,0): Bend toward +Y direction  
     * - (0,0,1): Bend toward +Z direction
     * 
     * The vector is automatically normalized when set. Default is (1,0,0).
     * 
     * @param poleVector Direction vector in jointA's parent space
     */
    void setPoleVector( const LLVector3& poleVector );

    /**
     * @brief Sets the bend axis in jointB's local frame for smarter IK solving.
     * 
     * For non-coplanar limb configurations (like twisted arms or legs), 
     * specifying the natural bend axis of jointB in its local coordinate
     * frame allows the solver to produce more anatomically correct results.
     * 
     * Real usage: LLEditingMotion uses (-0.682683, 0.0, -0.730714) for
     * the left elbow's natural bend axis to prevent bad IK solutions
     * in singular configurations.
     * 
     * This enables "smarter" solving that respects the joint's natural
     * range of motion and preferred bend direction.
     * 
     * @param bAxis Bend axis vector in jointB's local coordinate frame
     */
    void setBAxis( const LLVector3& bAxis );

    /**
     * @brief Gets the current twist angle in radians.
     * 
     * @return Twist angle in radians around the A-C axis
     */
    F32 getTwist();

    /**
     * @brief Sets the twist angle for rotating the solution plane.
     * 
     * The twist parameter rotates the entire solution plane around the line
     * connecting jointA and jointC. This is useful for animation effects
     * and achieving specific limb orientations.
     * 
     * A twist of 0.0 uses the standard solution plane defined by the pole vector.
     * Positive values rotate the plane clockwise when looking from jointA to jointC.
     * 
     * @param twist Twist angle in radians (default: 0.0)
     */
    void setTwist( F32 twist );

    /**
     * @brief Performs the inverse kinematics calculation.
     * 
     * This is the main "work" function that computes and applies the IK solution.
     * When called, the rotations of jointA and jointB are modified so that
     * jointC attempts to reach the position of jointGoal.
     * 
     * The solver preserves the bone lengths (mLengthAB and mLengthBC) that were
     * cached during setupJoints(). If the goal is unreachable (beyond the total
     * chain length), the chain will extend as far as possible toward the goal.
     * 
     * This method should be called each frame when IK is needed. In practice:
     * - LLEditingMotion calls this during onUpdate() when editing objects
     * - LLKeyframeStandMotion calls this for foot IK during standing animations
     * - Generally called during LLMotion::onUpdate() implementations
     * 
     * Prerequisites:
     * - setupJoints() must have been called successfully
     * - All joints must be valid and properly linked in hierarchy
     * - Joint world matrices should be up-to-date
     */
    void solve();
};

#endif // LL_LLJOINTSOLVERRP3_H

