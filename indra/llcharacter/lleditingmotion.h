/**
 * @file lleditingmotion.h
 * @brief Avatar animation for object editing and manipulation interactions.
 *
 * This motion provides realistic avatar arm positioning when users are editing
 * or manipulating objects in the 3D world. The system uses inverse kinematics
 * to point the avatar's left arm toward the object being edited, creating a
 * natural interaction pose.
 *
 * Key features:
 * - Inverse kinematics solver for shoulder/elbow/wrist chain
 * - Real-time targeting based on object selection and cursor position  
 * - Smooth interpolation to prevent jerky arm movements
 * - Automatic hand pose coordination for natural grip positioning
 * - Performance optimization with pixel area thresholds
 *
 * The motion is activated by the motion controller when editing begins and 
 * continuously updates the arm position to track the target from the "PointAtPoint"
 * animation data set by LLHUDEffectPointAt. It deactivates when editing ends, 
 * smoothly returning the arm to default positioning.
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

#ifndef LL_LLEDITINGMOTION_H
#define LL_LLEDITINGMOTION_H

#include "llmotion.h"
#include "lljointsolverrp3.h"
#include "v3dmath.h"

/// No ease-in time - editing motion starts immediately when activated
#define EDITING_EASEIN_DURATION 0.0f

/// Half-second ease-out for smooth transition when editing ends
#define EDITING_EASEOUT_DURATION 0.5f

/// High priority ensures editing motion overrides most other arm animations
#define EDITING_PRIORITY LLJoint::HIGH_PRIORITY

/// Minimum avatar screen size (in pixels) for editing motion to activate
#define MIN_REQUIRED_PIXEL_AREA_EDITING 500.f

/**
 * @brief Avatar motion that positions the left arm for object editing interactions.
 * 
 * This motion creates realistic avatar poses during object editing by using inverse
 * kinematics to point the left arm toward the editing target. The system provides
 * smooth, natural-looking arm movement that follows the user's cursor position
 * and selected objects.
 * 
 * Implementation details:
 * - Uses a 3-joint IK chain (shoulder -> elbow -> wrist) for the left arm
 * - Maintains local kinematic joint hierarchy for IK calculations
 * - Applies smooth interpolation to prevent jerky movements during rapid cursor motion
 * - Coordinates with hand pose system for natural grip appearances
 * - Only activates for avatars above minimum pixel size to optimize performance
 * 
 * The motion is activated automatically by the motion controller and deactivated
 * when exiting edit mode. Target position comes from the "PointAtPoint" animation data
 * which is set by LLHUDEffectPointAt based on cursor position and object selection.
 */
LL_ALIGN_PREFIX(16)
class LLEditingMotion :
    public LLMotion
{
    LL_ALIGN_NEW
public:
    /**
     * @brief Constructs the editing motion with kinematic chain setup.
     * @param id Motion UUID identifier
     */
    LLEditingMotion(const LLUUID &id);

    /**
     * @brief Destroys the editing motion and cleans up joint states.
     */
    virtual ~LLEditingMotion();

public:
    /**
     * @brief Factory method for motion registration system.
     * @param id Motion UUID identifier
     * @return New LLEditingMotion instance
     * 
     * Required by the motion controller to create editing motion instances.
     * Called during avatar initialization to register this motion type.
     */
    static LLMotion *create(const LLUUID &id) { return new LLEditingMotion(id); }

public:
    /// @brief Editing motion loops continuously while active
    virtual bool getLoop() { return true; }

    /// @brief Editing motion has no fixed duration (runs until deactivated)
    virtual F32 getDuration() { return 0.0; }

    /// @brief No ease-in time for immediate response to editing actions
    virtual F32 getEaseInDuration() { return EDITING_EASEIN_DURATION; }

    /// @brief Half-second ease-out for smooth transition when editing ends
    virtual F32 getEaseOutDuration() { return EDITING_EASEOUT_DURATION; }

    /// @brief High priority to override other arm animations during editing
    virtual LLJoint::JointPriority getPriority() { return EDITING_PRIORITY; }

    /// @brief Uses normal blending with other animations
    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    /// @brief Only activates for avatars above minimum screen size for performance
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EDITING; }

    /**
     * @brief Initializes the editing motion with character joint references.
     * @param character The avatar character to operate on
     * @return STATUS_SUCCESS if initialization succeeded, STATUS_FAILURE otherwise
     * 
     * Sets up joint state pointers for the left arm (shoulder, elbow, wrist) and
     * initializes the inverse kinematics solver with appropriate constraints.
     * Validates that all required joints exist in the character skeleton.
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    /**
     * @brief Called when editing motion becomes active.
     * @return True if activation succeeded
     * 
     * Synchronizes the kinematic chain with current avatar joint positions
     * and prepares the IK solver for target tracking.
     */
    virtual bool onActivate();

    /**
     * @brief Updates arm position to track editing target each frame.
     * @param time Current animation time (not used by this motion)
     * @param joint_mask Bitmask of joints affected by this motion
     * @return True to continue running, false to stop the motion
     * 
     * Retrieves the current editing target from animation data, applies the
     * IK solver to position the arm, and smoothly interpolates joint rotations
     * to prevent jerky movement during rapid cursor motion.
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

    /**
     * @brief Called when editing motion becomes inactive.
     * 
     * Performs any cleanup needed when editing ends. The motion system
     * automatically handles returning joints to their default positions.
     */
    virtual void onDeactivate();

public:
    // Kinematic chain for inverse kinematics calculations (local copies)
    LL_ALIGN_16(LLJoint             mParentJoint);      /// Local parent joint for IK chain root
    LL_ALIGN_16(LLJoint             mShoulderJoint);    /// Local shoulder joint for IK calculations  
    LL_ALIGN_16(LLJoint             mElbowJoint);       /// Local elbow joint for IK calculations
    LL_ALIGN_16(LLJoint             mWristJoint);       /// Local wrist joint for IK calculations
    LL_ALIGN_16(LLJoint             mTarget);           /// IK target position in world space
    
    LLJointSolverRP3    mIKSolver;                      /// Inverse kinematics solver for 3-joint chain

    LLCharacter         *mCharacter;                    /// Avatar character this motion operates on
    LLVector3           mWristOffset;                   /// Offset from wrist joint to apparent hand position

    // Joint state pointers for applying computed rotations to avatar
    LLPointer<LLJointState> mParentState;               /// Parent joint state (shoulder's parent)
    LLPointer<LLJointState> mShoulderState;             /// Left shoulder joint state
    LLPointer<LLJointState> mElbowState;                /// Left elbow joint state  
    LLPointer<LLJointState> mWristState;                /// Left wrist joint state
    LLPointer<LLJointState> mTorsoState;                /// Torso joint state for coordination

    // Hand pose coordination (shared across all editing motions)
    static S32          sHandPose;                      /// Current hand pose ID for all editing avatars
    static S32          sHandPosePriority;              /// Priority for hand pose animations
    
    LLVector3           mLastSelectPt;                  /// Last known editing target position for fallback
} LL_ALIGN_POSTFIX(16);

#endif // LL_LLKEYFRAMEMOTION_H

