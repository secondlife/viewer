/**
 * @file llkeyframefallmotion.h
 * @brief Procedural falling and landing animation with ground collision response.
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

#ifndef LL_LLKeyframeFallMotion_H
#define LL_LLKeyframeFallMotion_H

#include "llkeyframemotion.h"
#include "llcharacter.h"

/**
 * @brief Procedural falling animation that responds to ground collision and terrain normals.
 * 
 * LLKeyframeFallMotion creates realistic falling and landing animations by monitoring
 * avatar velocity and ground contact state. Unlike pure keyframe animations, this motion
 * dynamically adjusts pelvis rotation based on ground surface normals and falling velocity,
 * creating natural-looking landings on uneven terrain.
 * 
 * Key features:
 * - Velocity-based falling detection and animation triggering
 * - Ground collision response with surface normal alignment
 * - Pelvis rotation adjustment for natural landing poses
 * - Extended ease-in duration for smooth transitions from other motions
 * - Integration with terrain mesh for accurate ground interaction
 * 
 * Motion usage:
 * - ANIM_AGENT_STANDUP: Used for standing up transitions and landing recovery
 * - Procedural falling: Triggered automatically when avatar vertical velocity exceeds threshold
 * - Landing animation: Smooth transition from falling to standing on ground contact
 * 
 * The system tracks vertical velocity (mVelocityZ) to determine falling state and
 * uses ground collision data to compute appropriate landing poses. Pelvis rotation
 * is adjusted to align with ground surface normals for realistic terrain interaction.
 * 
 * @code
 * // Real usage: Registered for standup motion
 * registerMotion(ANIM_AGENT_STANDUP, LLKeyframeFallMotion::create);
 * 
 * // Falling state detection
 * if (character->getVelocity().mV[VZ] < -FALL_VELOCITY_THRESHOLD) {
 *     character->startMotion(ANIM_AGENT_STANDUP);
 * }
 * 
 * // Ground normal integration for landing
 * LLVector3 groundNormal = getGroundNormal(character->getPositionGlobal());
 * mRotationToGroundNormal = computeRotationToNormal(groundNormal);
 * @endcode
 */
//-----------------------------------------------------------------------------
// class LLKeyframeFallMotion
//-----------------------------------------------------------------------------
class LLKeyframeFallMotion :
    public LLKeyframeMotion
{
public:
    /**
     * @brief Constructor for procedural fall motion.
     * 
     * Initializes velocity tracking and ground collision state for
     * realistic falling and landing animation.
     * 
     * @param id Motion UUID identifier for registration with motion controller
     */
    LLKeyframeFallMotion(const LLUUID &id);

    /**
     * @brief Virtual destructor ensures proper cleanup.
     */
    virtual ~LLKeyframeFallMotion();

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * Creates new fall motion instances for the global motion registry.
     * Used primarily for standup/landing animation sequences.
     * 
     * @param id Motion UUID identifier
     * @return New LLKeyframeFallMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeFallMotion(id); }

public:
    /**
     * @brief Initializes fall motion for the specified character.
     * 
     * Sets up character reference, pelvis joint state, and velocity tracking
     * for ground collision and falling detection.
     * 
     * @param character Avatar character to initialize fall motion for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);
    
    /**
     * @brief Activates fall motion and prepares velocity tracking.
     * 
     * Initializes ground collision detection and sets up pelvis state
     * for dynamic rotation adjustment during falling and landing.
     * 
     * @return true if activation succeeded, false to deactivate immediately
     */
    virtual bool onActivate();
    
    /**
     * @brief Gets extended ease-in duration for smooth falling transitions.
     * 
     * Fall motion uses a longer ease-in period to create smooth transitions
     * from other motions into falling state, preventing jarring animation changes.
     * 
     * @return Extended ease-in duration in seconds
     */
    virtual F32 getEaseInDuration();
    
    /**
     * @brief Updates falling animation with velocity and ground collision response.
     * 
     * Core update loop that:
     * 1. Monitors avatar vertical velocity for falling state detection
     * 2. Detects ground collision and computes surface normals
     * 3. Adjusts pelvis rotation to align with ground surface
     * 4. Blends between falling and landing poses based on collision state
     * 5. Updates joint transformations for natural falling motion
     * 
     * The system uses mVelocityZ to track falling speed and mRotationToGroundNormal
     * to smoothly rotate the pelvis toward the ground surface normal for realistic
     * landings on slopes and uneven terrain.
     * 
     * @param activeTime Current animation time in seconds
     * @param joint_mask Bitmask of joints that can be modified by this motion
     * @return true to continue running, false when motion should stop
     */
    virtual bool onUpdate(F32 activeTime, U8* joint_mask);

protected:
    
    /// Avatar character this fall motion is applied to
    LLCharacter*    mCharacter;
    
    /// Vertical velocity component for falling state detection (negative = falling)
    F32             mVelocityZ;
    
    /// Joint state for pelvis rotation adjustment during falling and landing
    LLPointer<LLJointState> mPelvisState;
    
    /// Computed rotation to align pelvis with ground surface normal for natural landing
    LLQuaternion    mRotationToGroundNormal;
};

#endif // LL_LLKeyframeFallMotion_H

