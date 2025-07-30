/**
 * @file llkeyframewalkmotion.h
 * @brief Implementation of LLKeframeWalkMotion class.
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

#ifndef LL_LLKEYFRAMEWALKMOTION_H
#define LL_LLKEYFRAMEWALKMOTION_H

#include "llkeyframemotion.h"
#include "llcharacter.h"
#include "v3dmath.h"

#define MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST (20.f)
#define MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST (20.f)

/**
 * @brief Speed-adaptive walking animation that adjusts playback rate to match avatar velocity.
 * 
 * LLKeyframeWalkMotion extends the basic keyframe system to create realistic walking
 * animations that respond to avatar movement speed. Unlike static animations, this
 * motion dynamically adjusts its playback rate and provides foot-down timing data
 * to other systems for realistic ground interaction.
 * 
 * Core functionality:
 * - Speed-based animation playback (0.1 to 8.0 m/s range with MAX_WALK_PLAYBACK_SPEED)
 * - Down foot detection for footstep sounds and particle effects
 * - Real-time cycle phase tracking for animation synchronization
 * - Integration with walk adjustment motion for fine-tuned movement
 * 
 * This motion is used for all locomotion animations including:
 * - ANIM_AGENT_WALK, ANIM_AGENT_WALK_NEW (basic walking)
 * - ANIM_AGENT_RUN, ANIM_AGENT_RUN_NEW (running animations)
 * - ANIM_AGENT_FEMALE_WALK, ANIM_AGENT_FEMALE_WALK_NEW (gender-specific variants)
 * - ANIM_AGENT_CROUCHWALK (crouched movement)
 * - ANIM_AGENT_TURNLEFT, ANIM_AGENT_TURNRIGHT (turning while moving)
 * 
 * Performance characteristics:
 * - Activates at MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST (20px) - always visible
 * - Uses "Walk Speed" animation data for velocity-based speed adjustment
 * - Tracks "Down Foot" state for integration with ground effects
 * - Maintains separate real-time and adjusted-time tracking
 * 
 * @code
 * // Real usage: Registered for all walking/running animations
 * registerMotion(ANIM_AGENT_WALK, LLKeyframeWalkMotion::create);
 * registerMotion(ANIM_AGENT_RUN, LLKeyframeWalkMotion::create);
 * registerMotion(ANIM_AGENT_FEMALE_WALK, LLKeyframeWalkMotion::create);
 * registerMotion(ANIM_AGENT_CROUCHWALK, LLKeyframeWalkMotion::create);
 * 
 * // Speed adjustment through animation data
 * F32 walkSpeed = 2.5f;  // meters per second
 * character->setAnimationData("Walk Speed", &walkSpeed);
 * 
 * // Down foot detection for effects
 * void* down_foot_ptr = character->getAnimationData("Down Foot");
 * S32 down_foot = down_foot_ptr ? *((S32*)down_foot_ptr) : 0;
 * @endcode
 */
class LLKeyframeWalkMotion :
    public LLKeyframeMotion
{
    friend class LLWalkAdjustMotion;
public:
    /**
     * @brief Constructor for speed-adaptive walk motion.
     * 
     * Initializes timing variables and cycle tracking for velocity-based
     * animation speed adjustment.
     * 
     * @param id Motion UUID identifier for registration with motion controller
     */
    LLKeyframeWalkMotion(const LLUUID &id);

    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~LLKeyframeWalkMotion();

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * Creates new walk motion instances for the global motion registry.
     * Used for all locomotion animations including walk, run, and turn motions.
     * 
     * @param id Motion UUID identifier
     * @return New LLKeyframeWalkMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeWalkMotion(id); }

public:
    /**
     * @brief Initializes walk motion for the specified character.
     * 
     * Sets up character reference and delegates to parent keyframe motion
     * initialization for loading animation data.
     * 
     * @param character Avatar character to initialize walk motion for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);
    
    /**
     * @brief Activates walk motion and resets timing tracking.
     * 
     * Initializes real-time and adjusted-time tracking variables to
     * ensure smooth animation startup.
     * 
     * @return true if activation succeeded, false to deactivate immediately
     */
    virtual bool onActivate();
    
    /**
     * @brief Deactivates walk motion and cleans up animation data.
     * 
     * Removes "Down Foot" animation data to prevent stale foot-down
     * state from affecting other motions or effects systems.
     */
    virtual void onDeactivate();
    
    /**
     * @brief Updates animation with speed-adjusted timing.
     * 
     * Core update loop that:
     * 1. Calculates delta time since last update
     * 2. Reads "Walk Speed" from character animation data
     * 3. Adjusts animation timing based on movement speed
     * 4. Handles time wrap-around for looping animations
     * 5. Delegates to parent keyframe motion with adjusted time
     * 
     * Speed adjustment allows walking animations to match avatar velocity:
     * - Speed 1.0 = normal animation playback rate
     * - Speed 2.0 = double-speed playback for running
     * - Speed 0.5 = half-speed for slow walking
     * 
     * @param time Current animation time in seconds
     * @param joint_mask Bitmask of joints that can be modified by this motion
     * @return true to continue running, false when motion should stop
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

public:
    /// Avatar character this walk motion is applied to
    LLCharacter *mCharacter;
    
    /// Current position in the walk cycle (0.0 to 1.0)
    F32         mCyclePhase;
    
    /// Real-world time of last update for delta time calculation
    F32         mRealTimeLast;
    
    /// Speed-adjusted animation time for synchronized playback
    F32         mAdjTimeLast;
    
    /// Which foot is currently down (0=left, 1=right) for footstep effects
    S32         mDownFoot;
};

/**
 * @brief Real-time gait adjustment motion for smooth avatar movement.
 * 
 * LLWalkAdjustMotion provides fine-tuned corrections to walking animations
 * to ensure avatars appear to move naturally across terrain. This motion
 * runs continuously alongside walk motions to adjust pelvis position and
 * ankle placement based on actual avatar velocity and ground contact.
 * 
 * Key features:
 * - High priority (HIGH_PRIORITY) additive blending over base walk motions
 * - Minimal pixel area requirement (20px) - always active when walking
 * - Real-time speed adjustment based on actual avatar velocity vs animation speed
 * - Foot position tracking to prevent sliding and ground penetration
 * - Pelvis offset adjustment for natural stride length adaptation
 * 
 * This motion is registered as ANIM_AGENT_WALK_ADJUST and works in conjunction
 * with LLKeyframeWalkMotion to create realistic ground-based locomotion.
 * 
 * @code
 * // Real usage: Registered alongside other continuous motions
 * registerMotion(ANIM_AGENT_WALK_ADJUST, LLWalkAdjustMotion::create);
 * 
 * // Runs automatically when avatar is moving
 * // Adjusts pelvis and ankles based on actual vs animated movement
 * F32 speedDiff = actualSpeed - animatedSpeed;
 * mPelvisOffset.mV[VZ] += speedDiff * PELVIS_ADJUST_FACTOR;
 * @endcode
 */

class LLWalkAdjustMotion : public LLMotion
{
public:
    /**
     * @brief Constructor for walk adjustment motion.
     * 
     * Initializes pelvis state and timing variables for real-time
     * gait adjustment during avatar movement.
     * 
     * @param id Motion UUID identifier for registration
     */
    LLWalkAdjustMotion(const LLUUID &id);

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * @param id Motion UUID identifier
     * @return New LLWalkAdjustMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLWalkAdjustMotion(id); }

public:
    /**
     * @brief Initializes walk adjustment for the specified character.
     * 
     * Sets up ankle joint references and pelvis state for real-time
     * movement adjustment calculations.
     * 
     * @param character Avatar to initialize walk adjustment for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);
    
    /**
     * @brief Activates walk adjustment motion.
     * 
     * Resets timing and position tracking for smooth adjustment startup.
     * 
     * @return true if activation succeeded
     */
    virtual bool onActivate();
    
    /**
     * @brief Deactivates walk adjustment motion.
     */
    virtual void onDeactivate();
    
    /**
     * @brief Updates gait adjustment based on avatar movement.
     * 
     * Compares actual avatar velocity with animation speed to compute
     * pelvis and ankle adjustments for natural-looking movement.
     * 
     * @param time Current animation time
     * @param joint_mask Bitmask of joints this motion can affect
     * @return true to continue running
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);
    
    /**
     * @brief Gets high priority for additive blending over base motions.
     * 
     * @return HIGH_PRIORITY to ensure adjustments apply over base animations
     */
    virtual LLJoint::JointPriority getPriority(){return LLJoint::HIGH_PRIORITY;}
    
    /**
     * @brief Indicates this motion loops continuously.
     * 
     * @return Always true for continuous gait adjustment
     */
    virtual bool getLoop() { return true; }
    
    /**
     * @brief Motion has no fixed duration.
     * 
     * @return 0.0 for continuous operation
     */
    virtual F32 getDuration() { return 0.f; }
    
    /**
     * @brief No ease-in period for immediate adjustment response.
     * 
     * @return 0.0 for immediate activation
     */
    virtual F32 getEaseInDuration() { return 0.f; }
    
    /**
     * @brief No ease-out period for immediate adjustment cessation.
     * 
     * @return 0.0 for immediate deactivation
     */
    virtual F32 getEaseOutDuration() { return 0.f; }
    
    /**
     * @brief Very low pixel area requirement for universal activation.
     * 
     * Walk adjustment should be active whenever the avatar is visible,
     * regardless of distance, to maintain movement quality.
     * 
     * @return MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST (20px)
     */
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST; }
    
    /**
     * @brief Uses additive blending to layer over base walk motions.
     * 
     * @return ADDITIVE_BLEND for non-destructive adjustment
     */
    virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

public:
    /// Avatar character this adjustment motion is applied to
    LLCharacter     *mCharacter;
    
    /// Joint references for foot position tracking
    LLJoint*        mLeftAnkleJoint;     /// Left ankle for ground contact adjustment
    LLJoint*        mRightAnkleJoint;    /// Right ankle for ground contact adjustment
    
    /// Joint state for pelvis position adjustment
    LLPointer<LLJointState> mPelvisState;
    LLJoint*        mPelvisJoint;        /// Pelvis joint for height/tilt adjustment
    
    /// Foot position tracking for slide prevention
    LLVector3d      mLastLeftFootGlobalPos;  /// Previous left foot world position
    LLVector3d      mLastRightFootGlobalPos; /// Previous right foot world position
    
    /// Timing and speed tracking for adjustment calculations
    F32             mLastTime;           /// Time of previous update
    F32             mAdjustedSpeed;      /// Computed speed adjustment factor
    F32             mAnimSpeed;          /// Current animation playback speed
    F32             mRelativeDir;        /// Movement direction relative to avatar facing
    
    /// Computed adjustment offsets
    LLVector3       mPelvisOffset;       /// Pelvis position adjustment vector
    F32             mAnkleOffset;        /// Ankle height adjustment for ground contact
};

/**
 * @brief Flight adjustment motion for smooth aerial movement.
 * 
 * LLFlyAdjustMotion provides real-time corrections to flying animations
 * to ensure avatars appear to move naturally through 3D space. This motion
 * adjusts pelvis rotation and position based on flight velocity and direction.
 * 
 * Key features:
 * - Higher priority (HIGHER_PRIORITY) than walk adjustments for flight precedence
 * - Minimal pixel area requirement (20px) for universal flight adjustment
 * - Roll adjustment based on turning velocity for banking turns
 * - Continuous operation with additive blending over base flight animations
 * 
 * @code
 * // Real usage: Registered for flight adjustment
 * registerMotion(ANIM_AGENT_FLY_ADJUST, LLFlyAdjustMotion::create);
 * 
 * // Automatic banking during flight turns
 * F32 turnRate = getAngularVelocity().mV[VZ];
 * mRoll = llclamp(turnRate * ROLL_FACTOR, -MAX_ROLL, MAX_ROLL);
 * @endcode
 */

class LLFlyAdjustMotion : public LLMotion
{
public:
    /**
     * @brief Constructor for flight adjustment motion.
     * 
     * Initializes pelvis state and roll tracking for real-time
     * flight adjustment during aerial movement.
     * 
     * @param id Motion UUID identifier for registration
     */
    LLFlyAdjustMotion(const LLUUID &id);

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * @param id Motion UUID identifier
     * @return New LLFlyAdjustMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLFlyAdjustMotion(id); }

public:
    /**
     * @brief Initializes flight adjustment for the specified character.
     * 
     * Sets up pelvis state for roll adjustment during flight maneuvers.
     * 
     * @param character Avatar to initialize flight adjustment for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);
    
    /**
     * @brief Activates flight adjustment motion.
     * 
     * Resets roll state for smooth flight adjustment startup.
     * 
     * @return true if activation succeeded
     */
    virtual bool onActivate();
    
    /**
     * @brief Deactivates flight adjustment motion with no cleanup needed.
     */
    virtual void onDeactivate() {};
    
    /**
     * @brief Updates flight adjustment based on avatar aerial movement.
     * 
     * Computes roll adjustment based on turning velocity and applies
     * pelvis rotation for natural banking during flight turns.
     * 
     * @param time Current animation time
     * @param joint_mask Bitmask of joints this motion can affect
     * @return true to continue running
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);
    
    /**
     * @brief Gets higher priority for flight over walk adjustments.
     * 
     * @return HIGHER_PRIORITY to override ground-based adjustments during flight
     */
    virtual LLJoint::JointPriority getPriority(){return LLJoint::HIGHER_PRIORITY;}
    
    /**
     * @brief Indicates this motion loops continuously.
     * 
     * @return Always true for continuous flight adjustment
     */
    virtual bool getLoop() { return true; }
    
    /**
     * @brief Motion has no fixed duration.
     * 
     * @return 0.0 for continuous operation
     */
    virtual F32 getDuration() { return 0.f; }
    
    /**
     * @brief No ease-in period for immediate adjustment response.
     * 
     * @return 0.0 for immediate activation
     */
    virtual F32 getEaseInDuration() { return 0.f; }
    
    /**
     * @brief No ease-out period for immediate adjustment cessation.
     * 
     * @return 0.0 for immediate deactivation
     */
    virtual F32 getEaseOutDuration() { return 0.f; }
    
    /**
     * @brief Very low pixel area requirement for universal activation.
     * 
     * Flight adjustment should be active whenever the avatar is visible
     * during flight, regardless of distance.
     * 
     * @return MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST (20px)
     */
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST; }
    
    /**
     * @brief Uses additive blending to layer over base flight motions.
     * 
     * @return ADDITIVE_BLEND for non-destructive adjustment
     */
    virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

protected:
    /// Avatar character this flight adjustment is applied to
    LLCharacter     *mCharacter;
    
    /// Joint state for pelvis rotation adjustment during flight
    LLPointer<LLJointState> mPelvisState;
    
    /// Current roll angle for banking during flight turns (in radians)
    F32             mRoll;
};

#endif // LL_LLKeyframeWalkMotion_H

