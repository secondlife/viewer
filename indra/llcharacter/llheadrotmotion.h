/**
 * @file llheadrotmotion.h
 * @brief Implementation of LLHeadRotMotion class.
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

#ifndef LL_LLHEADROTMOTION_H
#define LL_LLHEADROTMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "llframetimer.h"

/// Minimum avatar pixel coverage required to activate head rotation tracking
/// Lower threshold than other motions since head movement is visible at medium distances
#define MIN_REQUIRED_PIXEL_AREA_HEAD_ROT 500.f;
/// Minimum avatar pixel coverage required to activate detailed eye movement  
/// High threshold since eye details only matter in close-up views
#define MIN_REQUIRED_PIXEL_AREA_EYE 25000.f;

/**
 * @brief Procedural head rotation motion for natural avatar head movement.
 * 
 * LLHeadRotMotion provides realistic head tracking and orientation behavior
 * for avatars, making them appear more lifelike by automatically adjusting
 * head position based on various factors like look-at targets, conversation
 * context, and environmental awareness.
 * 
 * This motion system handles:
 * - Look-at target tracking (focusing on other avatars, objects, or UI elements)
 * - Natural head bobbing and subtle movements during idle states
 * - Smooth transitions between different head orientations
 * - Integration with conversation systems for social interactions
 * 
 * The motion affects multiple joints in the spine/neck hierarchy:
 * - Torso: Subtle upper body lean for more natural posture
 * - Neck: Primary rotation point for head movement
 * - Head: Fine-tuned adjustments for precise targeting
 * 
 * Performance optimization: Head rotation is only active when the avatar
 * is visible enough on screen (MIN_REQUIRED_PIXEL_AREA_HEAD_ROT threshold).
 * 
 * @code
 * // Real registration pattern in LLVOAvatar::initInstance() 
 * registerMotion(ANIM_AGENT_HEAD_ROT, LLHeadRotMotion::create);
 * 
 * // Head rotation is managed automatically - no direct API calls needed
 * // The motion reads look-at targets from character animation data
 * // and coordinates with LLHUDEffectLookAt for target positioning
 * @endcode
 */
class LLHeadRotMotion :
    public LLMotion
{
public:
    /**
     * @brief Constructor for head rotation motion controller.
     * 
     * @param id Motion UUID identifier for registration with motion controller
     */
    LLHeadRotMotion(const LLUUID &id);

    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~LLHeadRotMotion();

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * @param id Motion UUID identifier
     * @return New LLHeadRotMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLHeadRotMotion(id); }

public:
    /**
     * @brief Indicates this motion loops continuously.
     * 
     * Head rotation runs continuously to provide ongoing head tracking behavior.
     * 
     * @return Always true for continuous head tracking
     */
    virtual bool getLoop() { return true; }

    /**
     * @brief Gets the total duration of the motion.
     * 
     * Head rotation has no fixed duration as it provides continuous tracking.
     * 
     * @return 0.0 indicating indefinite duration
     */
    virtual F32 getDuration() { return 0.0; }

    /**
     * @brief Gets the ease-in transition duration for smooth activation.
     * 
     * Head movement eases in over 1 second to avoid jarring transitions
     * when the motion becomes active.
     * 
     * @return 1.0 second ease-in duration
     */
    virtual F32 getEaseInDuration() { return 1.f; }

    /**
     * @brief Gets the ease-out transition duration for smooth deactivation.
     * 
     * Head movement eases out over 1 second to return naturally to
     * default position when the motion is deactivated.
     * 
     * @return 1.0 second ease-out duration
     */
    virtual F32 getEaseOutDuration() { return 1.f; }

    /**
     * @brief Gets minimum avatar pixel coverage to activate head tracking.
     * 
     * Head rotation is visible even at smaller avatar sizes, so it has
     * a lower threshold (500px) compared to:
     * - Eye movement: 25,000px (close-up only)
     * - Hand gestures: 10,000px (medium distance)
     * - Body noise: 10,000px (breathing, subtle movements)
     * 
     * @return Minimum pixel area (500 pixels)
     */
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_HEAD_ROT; }

    /**
     * @brief Gets the animation priority for blending with other motions.
     * 
     * Medium priority allows head rotation to blend naturally with most
     * other animations while being overridden by high-priority actions.
     * 
     * @return MEDIUM_PRIORITY for natural blending behavior
     */
    virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }

    /**
     * @brief Gets the blending type for combining with other animations.
     * 
     * @return NORMAL_BLEND for standard animation blending
     */
    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    /**
     * @brief Initializes the head rotation motion for the specified character.
     * 
     * Locates and caches references to the torso, neck, head, pelvis, and root
     * joints required for natural head movement. Creates joint states for
     * animation blending.
     * 
     * @param character Avatar character to initialize head rotation for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    /**
     * @brief Activates the head rotation motion system.
     * 
     * Prepares the motion for active head tracking by initializing
     * starting positions and clearing any previous state.
     * 
     * @return true if activation succeeded, false to deactivate immediately
     */
    virtual bool onActivate();

    /**
     * @brief Updates head rotation to track current look-at targets.
     * 
     * Called every frame while active. Calculates appropriate head orientation
     * based on look-at targets, applies smooth transitions, and updates the
     * torso, neck, and head joint rotations for natural movement.
     * 
     * @param time Current animation time
     * @param joint_mask Bitmask of joints affected by this motion
     * @return true to continue running, false when motion completes
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

    /**
     * @brief Deactivates the head rotation motion system.
     * 
     * Called when motion stops. Allows natural ease-out transition
     * back to default head position.
     */
    virtual void onDeactivate();

public:
    /// Avatar character that owns the head being animated
    LLCharacter         *mCharacter;

    /// Cached joint references for head rotation chain
    LLJoint             *mTorsoJoint;   /// Upper torso for subtle body lean
    LLJoint             *mHeadJoint;    /// Primary head joint
    LLJoint             *mRootJoint;    /// Avatar root for world positioning
    LLJoint             *mPelvisJoint;  /// Pelvis reference for body coordination

    /// Joint states for animation blending on rotation chain
    LLPointer<LLJointState> mTorsoState;  /// Torso rotation state for subtle lean
    LLPointer<LLJointState> mNeckState;   /// Neck rotation state for primary head turn
    LLPointer<LLJointState> mHeadState;   /// Head rotation state for fine adjustment

    /// Previous frame head rotation for smooth transition calculations
    LLQuaternion        mLastHeadRot;
};

/**
 * @brief Procedural eye movement and blinking animation system.
 * 
 * LLEyeMotion provides realistic eye behavior including:
 * - Eye tracking of look-at targets with natural lag behind head movement
 * - Automatic eye jitter and micro-movements for lifelike appearance
 * - Periodic look-away behavior to avoid "staring" effect
 * - Automatic blinking with realistic timing and duration
 * - Separate left/right eye control for slight asymmetry
 * 
 * Eye movement works in coordination with LLHeadRotMotion but operates
 * independently with its own targeting and timing. Eyes typically follow
 * the same targets as head rotation but with different movement characteristics:
 * - Faster response time than head movement
 * - Smaller range of motion (eyes can't rotate as far as head)
 * - More frequent micro-adjustments and natural jitter
 * 
 * The system supports both primary and alternative eye joint sets for
 * different avatar skeleton configurations (base vs extended rigs).
 * 
 * Performance optimization: Eye movement requires high avatar pixel coverage
 * (MIN_REQUIRED_PIXEL_AREA_EYE) since eye details are only visible in close-up.
 * 
 * @code
 * // Real registration pattern in LLVOAvatar::initInstance()
 * registerMotion(ANIM_AGENT_EYE, LLEyeMotion::create);
 * 
 * // Eye motion coordinates automatically with head rotation
 * // Uses internal timers for jitter, blinking, and look-away behavior
 * // No direct API calls needed - works through animation data system
 * @endcode
 */
class LLEyeMotion :
    public LLMotion
{
public:
    /**
     * @brief Constructor for eye movement motion controller.
     * 
     * @param id Motion UUID identifier for registration with motion controller
     */
    LLEyeMotion(const LLUUID &id);

    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~LLEyeMotion();

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * @param id Motion UUID identifier
     * @return New LLEyeMotion instance
     */
    static LLMotion *create( const LLUUID &id) { return new LLEyeMotion(id); }

public:
    /**
     * @brief Indicates this motion loops continuously.
     * 
     * Eye movement and blinking run continuously to provide ongoing
     * lifelike eye behavior.
     * 
     * @return Always true for continuous eye animation
     */
    virtual bool getLoop() { return true; }

    /**
     * @brief Gets the total duration of the motion.
     * 
     * Eye motion has no fixed duration as it provides continuous animation.
     * 
     * @return 0.0 indicating indefinite duration
     */
    virtual F32 getDuration() { return 0.0; }

    /**
     * @brief Gets the ease-in transition duration for smooth activation.
     * 
     * Eye movement eases in over 0.5 seconds to avoid sudden eye snapping
     * when the motion becomes active.
     * 
     * @return 0.5 second ease-in duration
     */
    virtual F32 getEaseInDuration() { return 0.5f; }

    /**
     * @brief Gets the ease-out transition duration for smooth deactivation.
     * 
     * Eye movement eases out over 0.5 seconds to return naturally to
     * default position when deactivated.
     * 
     * @return 0.5 second ease-out duration
     */
    virtual F32 getEaseOutDuration() { return 0.5f; }

    /**
     * @brief Gets minimum avatar pixel coverage to activate eye movement.
     * 
     * Eye details are only visible in close-up views, requiring the highest
     * pixel coverage threshold (25,000px) of all avatar motions. For comparison:
     * - Head rotation: 500px (visible at medium distance)
     * - Hand gestures: 10,000px (visible at closer range)
     * - Walk adjust: 20px (always active)
     * 
     * @return Minimum pixel area (25,000 pixels) for eye detail visibility
     */
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EYE; }

    /**
     * @brief Gets the animation priority for blending with other motions.
     * 
     * Medium priority allows eye movement to blend with other facial
     * animations while being overridden by high-priority actions.
     * 
     * @return MEDIUM_PRIORITY for natural blending behavior
     */
    virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }

    /**
     * @brief Gets the blending type for combining with other animations.
     * 
     * @return NORMAL_BLEND for standard animation blending
     */
    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    /**
     * @brief Initializes the eye motion for the specified character.
     * 
     * Locates and caches references to head and eye joints, creates joint
     * states for both primary and alternative eye configurations, and
     * initializes timing systems for jitter and blinking.
     * 
     * @param character Avatar character to initialize eye motion for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    /**
     * @brief Activates the eye motion system.
     * 
     * Prepares for active eye tracking by resetting timers and
     * initializing starting eye positions.
     * 
     * @return true if activation succeeded, false to deactivate immediately
     */
    virtual bool onActivate();

    /**
     * @brief Adjusts eye target position and applies to left/right eye states.
     * 
     * This helper function takes a target position and calculates appropriate
     * rotations for both left and right eyes, handling the geometric constraints
     * of eye movement and applying slight asymmetry for natural appearance.
     * 
     * @param targetPos Target position in world coordinates to look at
     * @param left_eye_state Joint state for left eye rotation
     * @param right_eye_state Joint state for right eye rotation
     */
    void adjustEyeTarget(LLVector3* targetPos, LLJointState& left_eye_state, LLJointState& right_eye_state);

    /**
     * @brief Updates eye movement, jitter, and blinking behavior.
     * 
     * Called every frame while active. Handles:
     * - Eye tracking of look-at targets
     * - Natural eye jitter and micro-movements
     * - Periodic look-away behavior
     * - Automatic blinking with realistic timing
     * 
     * @param time Current animation time
     * @param joint_mask Bitmask of joints affected by this motion
     * @return true to continue running, false when motion completes
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

    /**
     * @brief Deactivates the eye motion system.
     * 
     * Called when motion stops. Allows natural ease-out transition
     * back to default eye position and stops blinking behavior.
     */
    virtual void onDeactivate();

public:
    /// Avatar character that owns the eyes being animated
    LLCharacter         *mCharacter;

    /// Cached head joint reference for eye movement calculations
    LLJoint             *mHeadJoint;
    
    /// Joint states for primary eye configuration
    LLPointer<LLJointState> mLeftEyeState;   /// Left eye rotation state
    LLPointer<LLJointState> mRightEyeState;  /// Right eye rotation state
    
    /// Joint states for alternative eye configuration (extended avatar rigs)
    LLPointer<LLJointState> mAltLeftEyeState;   /// Alternative left eye state
    LLPointer<LLJointState> mAltRightEyeState;  /// Alternative right eye state

    /// Timing system for natural eye jitter and micro-movements
    LLFrameTimer        mEyeJitterTimer;    /// Timer for jitter intervals
    F32                 mEyeJitterTime;     /// Duration of current jitter cycle
    F32                 mEyeJitterYaw;      /// Current horizontal jitter offset
    F32                 mEyeJitterPitch;    /// Current vertical jitter offset
    
    /// Timing system for periodic look-away behavior to avoid staring
    F32                 mEyeLookAwayTime;   /// Duration of current look-away cycle
    F32                 mEyeLookAwayYaw;    /// Horizontal look-away offset
    F32                 mEyeLookAwayPitch;  /// Vertical look-away offset

    /// Automatic blinking system for realistic eye behavior
    LLFrameTimer        mEyeBlinkTimer;     /// Timer for blink intervals
    F32                 mEyeBlinkTime;      /// Duration of current blink cycle
    bool                mEyesClosed;        /// Current eyelid state (open/closed)
};

#endif // LL_LLHEADROTMOTION_H

