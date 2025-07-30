/**
 * @file llkeyframestandmotion.h
 * @brief Procedural standing motion with inverse kinematics for realistic foot placement.
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

#ifndef LL_LLKEYFRAMESTANDMOTION_H
#define LL_LLKEYFRAMESTANDMOTION_H

#include "llkeyframemotion.h"
#include "lljointsolverrp3.h"


/**
 * @brief Procedural standing animation with adaptive foot placement.
 * 
 * LLKeyframeStandMotion creates realistic standing poses by using inverse
 * kinematics to place feet properly on uneven terrain. Unlike pure keyframe
 * animations, this motion dynamically adjusts leg positions based on ground
 * height and surface normals, ensuring avatars appear naturally grounded.
 * 
 * Key features:
 * - Inverse kinematics solver (LLJointSolverRP3) for each leg
 * - Ground collision detection for realistic foot placement
 * - Ankle tracking to prevent foot sliding and penetration
 * - Pelvis height adjustment to maintain natural leg extension
 * - Support for both left and right foot target positioning
 * - Integration with terrain mesh for accurate surface normals
 * 
 * The motion maintains joint states for the complete leg chain:
 * - Pelvis: Central balance point and height adjustment
 * - Hip joints: Primary leg attachment and rotation
 * - Knee joints: Leg bending for natural stance
 * - Ankle joints: Final foot orientation and ground contact
 * 
 * Performance considerations:
 * - Uses SIMD-aligned (16-byte) data structures for optimal math operations
 * - Tracks ankle positions to minimize expensive IK calculations
 * - Only recalculates IK when foot targets move significantly
 * - Caches last good pelvis rotation to prevent instability
 * 
 * @code
 * // Real usage: Registration during avatar initialization
 * // Standing motion is registered globally for all avatars
 * if (LLCharacter::sInstances.size() == 1) {
 *     registerMotion(ANIM_AGENT_STAND, LLKeyframeStandMotion::create);
 *     registerMotion(ANIM_AGENT_STAND_1, LLKeyframeStandMotion::create);
 *     registerMotion(ANIM_AGENT_STAND_2, LLKeyframeStandMotion::create);
 * }
 * 
 * // IK solver setup during motion initialization
 * mIKLeft.setJointChain(mHipLeftJoint, mKneeLeftJoint, mAnkleLeftJoint);
 * mIKLeft.setPoleVector(LLVector3(0.f, 0.f, 1.f));  // Knee direction
 * @endcode
 */
LL_ALIGN_PREFIX(16)
class LLKeyframeStandMotion :
    public LLKeyframeMotion
{
    LL_ALIGN_NEW
public:
    /**
     * @brief Constructor for procedural standing motion.
     * 
     * Creates a standing motion instance with IK solvers and joint tracking
     * initialized to safe defaults. The motion must still be initialized
     * with a character to set up joint references.
     * 
     * @param id Motion UUID identifier for registration with motion controller
     */
    LLKeyframeStandMotion(const LLUUID &id);

    /**
     * @brief Virtual destructor ensures proper cleanup of IK solvers.
     */
    virtual ~LLKeyframeStandMotion();

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * Creates new standing motion instances for the global motion registry.
     * All motion subclasses must provide this factory function.
     * 
     * @param id Motion UUID identifier
     * @return New LLKeyframeStandMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeStandMotion(id); }

public:
    /**
     * @brief Initializes IK solvers and joint references for the character.
     * 
     * Sets up the inverse kinematics chain for both legs, establishing
     * joint references from hip to ankle. Creates joint states for tracking
     * animated positions and configures IK solver parameters.
     * 
     * @param character Avatar character to initialize standing motion for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);
    
    /**
     * @brief Activates the standing motion and prepares IK calculations.
     * 
     * Sets up initial foot target positions and enables ankle tracking.
     * Establishes baseline pelvis rotation and position for stability.
     * 
     * @return true if activation succeeded, false to deactivate immediately
     */
    virtual bool onActivate();
    
    /**
     * @brief Deactivates standing motion and releases joint control.
     * 
     * Cleans up IK solver state and releases joint states back to
     * the animation system for other motions to control.
     */
    void onDeactivate();
    
    /**
     * @brief Updates foot placement using ground collision and IK solving.
     * 
     * Core update loop that:
     * 1. Detects ground height and surface normals under each foot
     * 2. Sets IK target positions based on terrain data
     * 3. Solves inverse kinematics for natural leg positioning
     * 4. Adjusts pelvis height to maintain comfortable leg extension
     * 5. Updates joint rotations to achieve computed poses
     * 
     * The system tracks ankle positions to minimize expensive IK recalculations,
     * only updating when foot targets move beyond threshold distances.
     * 
     * @param time Current animation time in seconds
     * @param joint_mask Bitmask of joints that can be modified by this motion
     * @return true to continue running, false when motion should stop
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

public:
    /// IK kinematic chain representing the skeleton hierarchy for solving
    LLJoint             mPelvisJoint;     /// Root joint for both leg chains

    /// Left leg joint chain for inverse kinematics
    LLJoint             mHipLeftJoint;    /// Left hip attachment point
    LLJoint             mKneeLeftJoint;   /// Left knee bend joint
    LLJoint             mAnkleLeftJoint;  /// Left ankle for ground contact
    LLJoint             mTargetLeft;      /// Left foot IK target position

    /// Right leg joint chain for inverse kinematics
    LLJoint             mHipRightJoint;   /// Right hip attachment point
    LLJoint             mKneeRightJoint;  /// Right knee bend joint
    LLJoint             mAnkleRightJoint; /// Right ankle for ground contact
    LLJoint             mTargetRight;     /// Right foot IK target position

    /// Avatar character this motion is applied to
    LLCharacter *mCharacter;

    /// Whether to swap left/right foot assignments for varied standing poses
    bool                mFlipFeet;

    /// Joint states for animation blending and motion control
    LLPointer<LLJointState> mPelvisState;    /// Pelvis position and rotation state

    /// Left leg joint states for keyframe animation integration
    LLPointer<LLJointState> mHipLeftState;   /// Left hip rotation state
    LLPointer<LLJointState> mKneeLeftState;  /// Left knee bending state
    LLPointer<LLJointState> mAnkleLeftState; /// Left ankle orientation state

    /// Right leg joint states for keyframe animation integration
    LLPointer<LLJointState> mHipRightState;  /// Right hip rotation state
    LLPointer<LLJointState> mKneeRightState; /// Right knee bending state
    LLPointer<LLJointState> mAnkleRightState;/// Right ankle orientation state

    /// Inverse kinematics solvers for realistic leg positioning
    LLJointSolverRP3    mIKLeft;          /// Left leg IK solver (hip->knee->ankle)
    LLJointSolverRP3    mIKRight;         /// Right leg IK solver (hip->knee->ankle)

    /// Ground contact positions for foot placement
    LLVector3           mPositionLeft;    /// Left foot ground contact point
    LLVector3           mPositionRight;   /// Right foot ground contact point
    
    /// Ground surface normals for natural foot orientation
    LLVector3           mNormalLeft;      /// Left foot ground surface normal
    LLVector3           mNormalRight;     /// Right foot ground surface normal
    
    /// Computed foot rotations aligned to ground surfaces
    LLQuaternion        mRotationLeft;   /// Left foot orientation for terrain
    LLQuaternion        mRotationRight;  /// Right foot orientation for terrain

    /// Stability tracking to prevent unstable IK solutions
    LLQuaternion        mLastGoodPelvisRotation; /// Previous stable pelvis rotation
    LLVector3           mLastGoodPosition;       /// Previous stable pelvis position
    
    /// Whether to use ankle position tracking for optimization
    bool                mTrackAnkles;

    /// Frame counter for animation timing and debugging
    S32                 mFrameNum;
} LL_ALIGN_POSTFIX(16);

/// SIMD alignment ensures optimal performance for matrix calculations in IK solving

#endif // LL_LLKEYFRAMESTANDMOTION_H

