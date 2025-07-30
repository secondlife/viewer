/**
 * @file llhandmotion.h
 * @brief Implementation of LLHandMotion class.
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

#ifndef LL_LLHANDMOTION_H
#define LL_LLHANDMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "lltimer.h"

/// Minimum avatar pixel coverage required to activate detailed hand animations
/// Mid-range threshold - higher than head rotation (500px) but lower than eyes (25,000px)
#define MIN_REQUIRED_PIXEL_AREA_HAND 10000.f;

/**
 * @brief Visual parameter morphing system for avatar hand poses.
 * 
 * LLHandMotion controls avatar hand appearance through visual parameter morphs
 * rather than joint rotations. Each hand pose corresponds to a named visual
 * parameter (like "Hands_Relaxed", "Hands_Fist") that morphs the hand mesh
 * geometry to achieve the desired finger positions.
 * 
 * The system works by:
 * - Setting visual parameter weights from 0.0 to 1.0 for different poses
 * - Blending between poses over HAND_MORPH_BLEND_TIME (0.2 seconds)
 * - Using the character's "Hand Pose" animation data to determine active pose
 * - Registering with joint signature at LL_HAND_JOINT_NUM for priority control
 * 
 * Supported hand poses (with their morph names):
 * - HAND_POSE_RELAXED → "Hands_Relaxed"
 * - HAND_POSE_FIST → "Hands_Fist" 
 * - HAND_POSE_POINT → "Hands_Point"
 * - HAND_POSE_TYPING → "Hands_Typing"
 * - HAND_POSE_PEACE_R → "Hands_Peace_R"
 * - HAND_POSE_SPREAD is special: not a morph, represents default hand shape
 * 
 * Performance: Only active when avatar pixel coverage exceeds 10,000 pixels,
 * since hand details are only visible at close range.
 * 
 * @code
 * // Actual usage in avatar system - registered during LLVOAvatar::initInstance()
 * registerMotion(ANIM_AGENT_HAND_MOTION, LLHandMotion::create);
 * 
 * // Hand poses are controlled via animation data, not direct API calls
 * character->setAnimationData("Hand Pose", &desired_pose);
 * @endcode
 */
class LLHandMotion :
    public LLMotion
{
public:
    /**
     * @brief Available hand pose types for procedural hand animation.
     * 
     * Each pose defines a specific finger and thumb configuration that
     * gets procedurally applied to the hand joints. Some poses are generic
     * (both hands), while others are hand-specific for asymmetric gestures.
     */
    typedef enum e_hand_pose
    {
        HAND_POSE_SPREAD,      /// Open hand with spread fingers
        HAND_POSE_RELAXED,     /// Natural relaxed hand position
        HAND_POSE_POINT,       /// Index finger pointing, others curled
        HAND_POSE_FIST,        /// Closed fist
        HAND_POSE_RELAXED_L,   /// Left hand relaxed pose
        HAND_POSE_POINT_L,     /// Left hand pointing
        HAND_POSE_FIST_L,      /// Left hand fist
        HAND_POSE_RELAXED_R,   /// Right hand relaxed pose
        HAND_POSE_POINT_R,     /// Right hand pointing
        HAND_POSE_FIST_R,      /// Right hand fist
        HAND_POSE_SALUTE_R,    /// Right hand military salute
        HAND_POSE_TYPING,      /// Typing position for both hands
        HAND_POSE_PEACE_R,     /// Right hand peace sign (V fingers)
        HAND_POSE_PALM_R,      /// Right hand open palm gesture
        NUM_HAND_POSES         /// Total count of available poses
    } eHandPose;

    /**
     * @brief Constructor for hand motion controller.
     * 
     * @param id Motion UUID identifier for registration with motion controller
     */
    LLHandMotion(const LLUUID &id);

    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~LLHandMotion();

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * All motion classes must provide a static create function that
     * the motion registry uses to instantiate new motion objects.
     * 
     * @param id Motion UUID identifier
     * @return New LLHandMotion instance
     */
    static LLMotion *create(const LLUUID &id) { return new LLHandMotion(id); }

public:
    /**
     * @brief Indicates this motion loops continuously.
     * 
     * Hand poses are maintained as long as the motion is active,
     * so this motion loops rather than playing once.
     * 
     * @return Always true for continuous hand pose control
     */
    virtual bool getLoop() { return true; }

    /**
     * @brief Gets the total duration of the motion.
     * 
     * Hand motion has no fixed duration since it's controlled by
     * external gesture systems and remains active indefinitely.
     * 
     * @return 0.0 indicating indefinite duration
     */
    virtual F32 getDuration() { return 0.0; }

    /**
     * @brief Gets the ease-in transition duration.
     * 
     * Hand poses snap to their target immediately without easing.
     * 
     * @return 0.0 for immediate activation
     */
    virtual F32 getEaseInDuration() { return 0.0; }

    /**
     * @brief Gets the ease-out transition duration.
     * 
     * Hand poses release immediately when deactivated.
     * 
     * @return 0.0 for immediate deactivation
     */
    virtual F32 getEaseOutDuration() { return 0.0; }

    /**
     * @brief Gets minimum avatar pixel coverage to activate detailed hand animation.
     * 
     * Hand details are visible at medium-close range. Pixel area thresholds:
     * - Walk adjust: 20px (always active)
     * - Head rotation: 500px (visible at medium distance)  
     * - Hand gestures: 10,000px (this motion - medium-close range)
     * - Eye movement: 25,000px (close-up only)
     * 
     * @return Minimum pixel area (10,000 pixels)
     */
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_HAND; }

    /**
     * @brief Gets the animation priority for blending with other motions.
     * 
     * Medium priority allows hand gestures to blend with most other animations
     * while being overridden by high-priority actions.
     * 
     * @return MEDIUM_PRIORITY for typical gesture blending
     */
    virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }

    /**
     * @brief Gets the blending type for combining with other animations.
     * 
     * @return NORMAL_BLEND for standard animation blending
     */
    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    /**
     * @brief Initializes the hand motion for the specified character.
     * 
     * Called once after construction to set up joint references and
     * prepare the motion for activation. Must locate and cache pointers
     * to all hand and finger joints in the character's skeleton.
     * 
     * @param character Avatar character to initialize hand motion for
     * @return STATUS_SUCCESS if initialization completed successfully
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    /**
     * @brief Activates the hand motion system.
     * 
     * Called when the motion becomes active. Sets up initial state
     * and prepares for pose calculations.
     * 
     * @return true if activation succeeded, false to deactivate immediately
     */
    virtual bool onActivate();

    /**
     * @brief Updates hand pose visual parameter weights.
     * 
     * Called every frame while active. Reads the "Hand Pose" animation data
     * to determine the desired pose, then smoothly blends visual parameter
     * weights to morph the hand mesh geometry. Handles transitions between
     * different poses over HAND_MORPH_BLEND_TIME.
     * 
     * Note: HAND_POSE_SPREAD is not a morph - it represents the default
     * hand shape when all pose morphs are at 0.0 weight.
     * 
     * @param time Current animation time
     * @param joint_mask Bitmask of joints affected by this motion  
     * @return true to continue running, false when motion completes
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

    /**
     * @brief Deactivates the hand motion system.
     * 
     * Called when the motion is stopped. Cleans up any temporary state
     * and releases hand joints back to default poses.
     */
    virtual void onDeactivate();

    /**
     * @brief Indicates whether this motion can be deprecated by newer motions.
     * 
     * Hand motion should not be deprecated as it provides essential
     * gesture functionality that other motions may depend on.
     * 
     * @return false to prevent motion deprecation
     */
    virtual bool canDeprecate() { return false; }

    /**
     * @brief Converts a hand pose enum to its string representation.
     * 
     * Useful for debugging, logging, and user interface display of
     * current hand pose states.
     * 
     * @param pose Hand pose enum value
     * @return Human-readable string name for the pose
     */
    static std::string getHandPoseName(eHandPose pose);
    
    /**
     * @brief Converts a string name to its corresponding hand pose enum.
     * 
     * Used for parsing hand pose names from configuration files,
     * user commands, or animation scripts.
     * 
     * @param posename String name of the hand pose
     * @return Corresponding eHandPose enum value
     */
    static eHandPose getHandPose(std::string posename);

public:
    /// Avatar character that owns the hands being morphed
    LLCharacter         *mCharacter;

    /// Timestamp of the last animation update for blend timing
    F32                 mLastTime;
    /// Currently active hand pose (visual morph being displayed)
    eHandPose           mCurrentPose;
    /// Target hand pose to transition to (used during HAND_MORPH_BLEND_TIME transitions)
    eHandPose           mNewPose;

private:
    /// Static hand pose names mapped to visual parameter morph names
    /// Used by getHandPoseName() and setVisualParamWeight() calls
    static const char* gHandPoseNames[NUM_HAND_POSES];
    
    /// Blend time constant for smooth transitions between hand poses
    static const F32 HAND_MORPH_BLEND_TIME; // 0.2 seconds
};
#endif // LL_LLHANDMOTION_H

