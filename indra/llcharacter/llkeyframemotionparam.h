/**
 * @file llkeyframemotionparam.h
 * @brief Parametric motion blending system for smooth animation transitions.
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

#ifndef LL_LLKEYFRAMEMOTIONPARAM_H
#define LL_LLKEYFRAMEMOTIONPARAM_H

#include <string>

#include "llmotion.h"
#include "lljointstate.h"
#include "v3math.h"
#include "llquaternion.h"
#include "llkeyframemotion.h"

/**
 * @brief Multi-motion blending system that interpolates between keyframe animations based on parameters.
 * 
 * LLKeyframeMotionParam enables smooth transitions between multiple related animations
 * by blending them based on continuous parameter values. This creates fluid motion
 * variations without requiring separate animations for every possible state.
 * 
 * Core functionality:
 * - Parameterized motion blending (e.g., walking speed variations)
 * - Smooth interpolation between discrete animation keyframes
 * - Multi-dimensional parameter space support
 * - Pose blending using LLPoseBlender for natural transitions
 * - Dynamic loading and caching of component motions
 * 
 * Common use cases:
 * - Speed-based walking variations (slow walk to fast run)
 * - Directional movement blending (forward/backward/strafe combinations)
 * - Mood-based gesture variations (happy/sad/angry expressions)
 * - Size-based animation scaling (different avatar proportions)
 * 
 * The system organizes motions by parameter name and value, using a sorted
 * data structure (motion_map_t) for efficient parameter-based lookup and
 * interpolation between adjacent parameter values.
 * 
 * Performance characteristics:
 * - Activates at MIN_REQUIRED_PIXEL_AREA_KEYFRAME (40px) like base keyframe motions
 * - Uses pose blending to minimize per-joint computation overhead
 * - Caches loaded motions to avoid redundant asset fetching
 * - Supports continuous looping with seamless parameter transitions
 * 
 * @code
 * // Real usage: Parameter-driven animation system
 * // Example: Walking speed parameter from 0.0 (slow) to 1.0 (fast)
 * LLKeyframeMotionParam* walkMotion = new LLKeyframeMotionParam(walkMotionUUID);
 * 
 * // Add component motions at different parameter values
 * walkMotion->addKeyframeMotion("walk_slow", slowWalkUUID, "speed", 0.0f);
 * walkMotion->addKeyframeMotion("walk_normal", normalWalkUUID, "speed", 0.5f);
 * walkMotion->addKeyframeMotion("walk_fast", fastWalkUUID, "speed", 1.0f);
 * 
 * // Set default motion for LOD fallback
 * walkMotion->setDefaultKeyframeMotion("walk_normal");
 * 
 * // Runtime parameter control (handled by motion controller)
 * character->setAnimationData("Walk Speed Parameter", &currentSpeed);
 * @endcode
 */
class LLKeyframeMotionParam :
    public LLMotion
{
public:
    /**
     * @brief Constructor for parametric motion blending system.
     * 
     * Initializes motion parameter maps, pose blending system, and
     * timing variables for smooth multi-motion interpolation.
     * 
     * @param id Motion UUID identifier for registration with motion controller
     */
    LLKeyframeMotionParam(const LLUUID &id);

    /**
     * @brief Virtual destructor ensures proper cleanup of motion maps and pose blender.
     */
    virtual ~LLKeyframeMotionParam();

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * Creates new parametric motion instances for the global motion registry.
     * Used for complex animations requiring smooth parameter-based blending.
     * 
     * @param id Motion UUID identifier
     * @return New LLKeyframeMotionParam instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLKeyframeMotionParam(id); }

public:

    /**
     * @brief Indicates this motion loops continuously.
     * 
     * Parametric motions typically loop to provide continuous parameter-based
     * animation blending, allowing smooth transitions as parameter values change.
     * 
     * @return Always true for continuous parametric blending
     */
    virtual bool getLoop() {
        return true;
    }

    /**
     * @brief Gets the motion duration from the longest component animation.
     * 
     * Duration is determined by the longest keyframe motion in the parameter set,
     * ensuring all component animations can complete their cycles.
     * 
     * @return Duration in seconds of the longest component motion
     */
    virtual F32 getDuration() {
        return mDuration;
    }

    /**
     * @brief Gets the ease-in duration for smooth motion activation.
     * 
     * @return Ease-in time in seconds for gradual motion activation
     */
    virtual F32 getEaseInDuration() {
        return mEaseInDuration;
    }

    /**
     * @brief Gets the ease-out duration for smooth motion deactivation.
     * 
     * @return Ease-out time in seconds for gradual motion deactivation
     */
    virtual F32 getEaseOutDuration() {
        return mEaseOutDuration;
    }

    /**
     * @brief Gets the animation priority for blending with other motions.
     * 
     * Priority is typically inherited from component motions or set based
     * on the parametric motion's intended use case.
     * 
     * @return Priority level for animation blending resolution
     */
    virtual LLJoint::JointPriority getPriority() {
        return mPriority;
    }

    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    // called to determine when a motion should be activated/deactivated based on avatar pixel coverage
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_KEYFRAME; }

    // run-time (post constructor) initialization,
    // called after parameters have been set
    // must return true to indicate success and be available for activation
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    // called when a motion is activated
    // must return true to indicate success, or else
    // it will be deactivated
    virtual bool onActivate();

    // called per time step
    // must return true while it is active, and
    // must return false when the motion is completed.
    virtual bool onUpdate(F32 time, U8* joint_mask);

    // called when a motion is deactivated
    virtual void onDeactivate();

    virtual LLPose* getPose() { return mPoseBlender.getBlendedPose();}

protected:
    
    /**
     * @brief Container for motion-parameter pairs in the blending system.
     * 
     * Each ParameterizedMotion represents one component animation at a specific
     * parameter value. The motion controller interpolates between these to create
     * smooth parameter-driven animation blending.
     */
    struct ParameterizedMotion
    {
        /**
         * @brief Constructor for motion-parameter pair.
         * 
         * @param motion Keyframe motion for this parameter value
         * @param param Parameter value this motion represents
         */
        ParameterizedMotion(LLMotion* motion, F32 param) : mMotion(motion), mParam(param) {}
        
        /// Keyframe motion instance for this parameter value
        LLMotion* mMotion;
        /// Parameter value this motion represents (e.g., speed = 0.5)
        F32 mParam;
    };

    /**
     * @brief Adds a component motion at a specific parameter value.
     * 
     * Registers a keyframe motion to be used at the specified parameter value.
     * Multiple motions can be added for different parameter values, and the
     * system will interpolate between them during playback.
     * 
     * @param name Human-readable name for this motion component
     * @param id UUID of the keyframe motion asset to load
     * @param param Parameter name this motion affects (e.g., "speed")
     * @param value Parameter value this motion represents
     * @return true if motion was added successfully
     */
    bool addKeyframeMotion(char *name, const LLUUID &id, char *param, F32 value);

    /**
     * @brief Sets the default motion for fallback and LOD purposes.
     * 
     * The default motion is used when parameter values are out of range
     * or when the system needs to fall back to a single representative motion.
     * 
     * @param name Name of the motion to use as default
     */
    void setDefaultKeyframeMotion(char *);

    /**
     * @brief Loads all registered component motions from asset system.
     * 
     * Triggers loading of all keyframe motion assets registered via
     * addKeyframeMotion(). Called during motion initialization.
     * 
     * @return true if all motions loaded successfully
     */
    bool loadMotions();

protected:

    /**
     * @brief Comparison functor for sorting parameterized motions.
     * 
     * Motions are sorted first by parameter value, then by motion pointer
     * for consistent ordering in the motion_list_t set.
     */
    struct compare_motions
    {
        /**
         * @brief Compares two parameterized motions for sorting.
         * 
         * @param a First motion to compare
         * @param b Second motion to compare
         * @return true if a should come before b in sorted order
         */
        bool operator() (const ParameterizedMotion& a, const ParameterizedMotion& b) const
        {
            if (a.mParam != b.mParam)
                return (a.mParam < b.mParam);
            else
                return a.mMotion < b.mMotion;
        }
    };

    /// Sorted set of motions for a single parameter
    typedef std::set < ParameterizedMotion, compare_motions > motion_list_t;
    /// Map from parameter names to their motion sets
    typedef std::map <std::string, motion_list_t > motion_map_t;
    
    /// All registered parameterized motions organized by parameter name
    motion_map_t        mParameterizedMotions;
    
    /// Fallback motion when parameters are out of range or for LOD
    LLMotion*           mDefaultKeyframeMotion;
    
    /// Avatar character this parametric motion is applied to
    LLCharacter*        mCharacter;
    
    /// Pose blending system for smooth interpolation between component motions
    LLPoseBlender       mPoseBlender;

    /// Transition timing from component motions or configuration
    F32                 mEaseInDuration;     /// Time to blend in when motion starts
    F32                 mEaseOutDuration;    /// Time to blend out when motion stops
    F32                 mDuration;           /// Total duration from longest component motion
    
    /// Animation priority for blending with other motions
    LLJoint::JointPriority  mPriority;

    /// Asset loading transaction ID for motion loading coordination
    LLUUID              mTransactionID;
};

#endif // LL_LLKEYFRAMEMOTIONPARAM_H
