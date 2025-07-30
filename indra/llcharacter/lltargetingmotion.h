/**
 * @file lltargetingmotion.h
 * @brief Implementation of LLTargetingMotion class.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLTARGETINGMOTION_H
#define LL_LLTARGETINGMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"

#define TARGETING_EASEIN_DURATION   0.3f
#define TARGETING_EASEOUT_DURATION 0.5f
#define TARGETING_PRIORITY LLJoint::HIGH_PRIORITY
#define MIN_REQUIRED_PIXEL_AREA_TARGETING 1000.f;


/**
 * @brief Procedural targeting motion that orients the character toward a target.
 * 
 * LLTargetingMotion is a concrete implementation of LLMotion that demonstrates
 * how to create procedural animations that respond to real-time conditions.
 * This motion adjusts the character's torso and arm positions to point toward
 * a target location, typically used for aiming weapons or tools.
 * 
 * Key Characteristics:
 * - Uses ADDITIVE_BLEND mode to layer on top of base locomotion
 * - Infinite duration (procedural, not keyframe-based)
 * - Requires minimum pixel area for performance optimization
 * - Modifies torso and right hand joints for targeting behavior
 * 
 * Technical Implementation:
 * - Calculates targeting rotation based on character and target positions
 * - Uses smooth ease-in/ease-out transitions for natural motion
 * - Maintains targeting while other motions continue (walking, etc.)
 * - Automatically deactivates when target is lost or motion is stopped
 * 
 * Performance Considerations:
 * - Only activates for characters with sufficient pixel area (MIN_REQUIRED_PIXEL_AREA_TARGETING)
 * - Uses HIGH_PRIORITY to ensure targeting overrides conflicting motions
 * - Minimal joint modification (torso and hand) for efficient processing
 * 
 * Real-world Usage:
 * This motion activates automatically when avatars enter aiming mode, determined
 * by the presence of gun aim animations. The system uses AGENT_GUN_AIM_ANIMS
 * to detect when the avatar should be targeting.
 * 
 * @code
 * // Actual activation pattern in LLVOAvatar::updateAnimations()
 * if (isAnyAnimationSignaled(AGENT_GUN_AIM_ANIMS, NUM_AGENT_GUN_AIM_ANIMS)) {
 *     if (mEnableDefaultMotions) {
 *         startMotion(ANIM_AGENT_TARGET);     // Start targeting motion
 *     }
 *     stopMotion(ANIM_AGENT_BODY_NOISE);     // Stop idle body movement
 * } else {
 *     stopMotion(ANIM_AGENT_TARGET);          // Stop targeting when not aiming
 *     if (mEnableDefaultMotions) {
 *         startMotion(ANIM_AGENT_BODY_NOISE); // Resume idle movement
 *     }
 * }
 * @endcode
 * 
 * Registration occurs once in LLVOAvatar::initInstance():
 * @code
 * // During first avatar creation
 * registerMotion(ANIM_AGENT_TARGET, LLTargetingMotion::create);
 * @endcode
 * 
 * The motion integrates with the broader animation system by suppressing
 * body noise motions during targeting for steadier aim, demonstrating how
 * motions interact and override each other for realistic behavior.
 */
class LLTargetingMotion :
    public LLMotion
{
public:
    /**
     * @brief Constructs a new targeting motion instance.
     * 
     * Initializes the motion with the specified UUID and sets up default
     * values for targeting parameters. The motion requires initialization
     * before it can be activated.
     * 
     * @param id UUID identifying this motion instance
     */
    LLTargetingMotion(const LLUUID &id);

    /**
     * @brief Destroys the targeting motion and cleans up resources.
     * 
     * Ensures proper cleanup of joint state references and any motion-specific
     * data. The motion should be deactivated before destruction.
     */
    virtual ~LLTargetingMotion();

public:
    //-------------------------------------------------------------------------
    // Factory function for MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    /**
     * @brief Static factory function for creating targeting motion instances.
     * 
     * This function must be registered with the LLMotionRegistry to enable
     * dynamic creation of targeting motions. All motion subclasses must
     * implement a similar static create function matching this signature.
     * 
     * @param id UUID for the new motion instance
     * @return Pointer to newly created LLTargetingMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLTargetingMotion(id); }

public:
    //-------------------------------------------------------------------------
    // LLMotion interface implementation
    //-------------------------------------------------------------------------

    /**
     * @brief Targeting motions loop continuously while active.
     * 
     * Procedural motions like targeting typically loop indefinitely,
     * continuously updating their calculations based on current conditions.
     * 
     * @return true - targeting motion loops until explicitly stopped
     */
    virtual bool getLoop() { return true; }

    /**
     * @brief Targeting motion has infinite duration.
     * 
     * Since this is a procedural motion that responds to real-time conditions
     * rather than playing back a fixed animation, it has no predetermined
     * endpoint. The motion continues until explicitly stopped.
     * 
     * @return 0.0 indicating infinite/procedural duration
     */
    virtual F32 getDuration() { return 0.0; }

    /**
     * @brief Gets the ease-in duration for smooth targeting activation.
     * 
     * @return TARGETING_EASEIN_DURATION (0.3 seconds) for smooth transition
     */
    virtual F32 getEaseInDuration() { return TARGETING_EASEIN_DURATION; }

    /**
     * @brief Gets the ease-out duration for smooth targeting deactivation.
     * 
     * @return TARGETING_EASEOUT_DURATION (0.5 seconds) for smooth transition
     */
    virtual F32 getEaseOutDuration() { return TARGETING_EASEOUT_DURATION; }

    /**
     * @brief Targeting motion uses high priority to override conflicting motions.
     * 
     * High priority ensures targeting behavior takes precedence over lower
     * priority motions that might affect the same joints (torso, arms).
     * 
     * @return TARGETING_PRIORITY (HIGH_PRIORITY) for motion blending
     */
    virtual LLJoint::JointPriority getPriority() { return TARGETING_PRIORITY; }

    /**
     * @brief Targeting motion uses additive blending mode.
     * 
     * ADDITIVE_BLEND allows targeting rotations to be added on top of
     * existing locomotion and idle motions without replacing them entirely.
     * This enables targeting while walking, standing, or performing other actions.
     * 
     * @return ADDITIVE_BLEND for layered motion composition
     */
    virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

    /**
     * @brief Gets minimum pixel area required for targeting motion activation.
     * 
     * Targeting motion requires sufficient character visibility to justify
     * the computational cost. Small or distant characters skip this motion
     * for performance optimization.
     * 
     * @return MIN_REQUIRED_PIXEL_AREA_TARGETING (1000.0) pixels for activation
     */
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_TARGETING; }

    /**
     * @brief Initializes targeting motion with character joint references.
     * 
     * Sets up joint state references for the pelvis, torso, and right hand
     * joints that will be modified during targeting calculations. Validates
     * that required joints exist in the character's skeleton.
     * 
     * @param character Pointer to the character this motion will animate
     * @return STATUS_SUCCESS if initialization completed, STATUS_FAILURE if joints missing
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    /**
     * @brief Activates targeting motion and prepares for target tracking.
     * 
     * Performs activation-specific setup including joint state configuration
     * and initial targeting calculations. The motion begins tracking the
     * current target immediately upon activation.
     * 
     * @return true if activation succeeded, false to immediately deactivate
     */
    virtual bool onActivate();

    /**
     * @brief Updates targeting motion calculations each frame.
     * 
     * Calculates the required joint rotations to orient the character toward
     * the current target. Updates torso and hand positions based on targeting
     * geometry and applies smooth interpolation for natural movement.
     * 
     * @param time Time in seconds since motion activation
     * @param joint_mask Array indicating which joints this motion affects
     * @return true to continue targeting, false to stop motion
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

    /**
     * @brief Performs cleanup when targeting motion is deactivated.
     * 
     * Resets joint states and clears any targeting-specific data to ensure
     * the character returns to a neutral pose when targeting ends.
     */
    virtual void onDeactivate();

public:
    /**
     * @brief Pointer to the character being animated by this motion.
     * 
     * Set during initialization and used throughout the motion's lifetime
     * to access character properties like position and joint hierarchy.
     */
    LLCharacter         *mCharacter;
    
    /**
     * @brief Joint state for the torso joint used in targeting calculations.
     * 
     * The torso is the primary joint modified by targeting motion, providing
     * the main rotation to orient the character toward the target.
     */
    LLPointer<LLJointState> mTorsoState;
    
    /**
     * @brief Reference to the pelvis joint for base positioning.
     * 
     * Used as a reference point for calculating targeting rotations and
     * ensuring proper coordinate system alignment.
     */
    LLJoint*            mPelvisJoint;
    
    /**
     * @brief Reference to the torso joint for primary targeting rotation.
     * 
     * The main joint modified by targeting motion to orient the character's
     * upper body toward the target location.
     */
    LLJoint*            mTorsoJoint;
    
    /**
     * @brief Reference to the right hand joint for weapon/tool aiming.
     * 
     * Secondary joint used for fine-tuning targeting behavior, particularly
     * important for weapon aiming and tool usage scenarios.
     */
    LLJoint*            mRightHandJoint;
};

#endif // LL_LLTARGETINGMOTION_H

