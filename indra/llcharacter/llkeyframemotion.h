/**
 * @file llkeyframemotion.h
 * @brief Core keyframe animation system for avatar motion playback.
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

#ifndef LL_LLKEYFRAMEMOTION_H
#define LL_LLKEYFRAMEMOTION_H

#include <string>

#include "llassetstorage.h"
#include "llbboxlocal.h"
#include "llhandmotion.h"
#include "lljointstate.h"
#include "llmotion.h"
#include "llquaternion.h"
#include "v3dmath.h"
#include "v3math.h"
#include "llbvhconsts.h"

/// Forward declaration of keyframe animation data cache
class LLKeyframeDataCache;
/// Forward declaration of binary data serialization utility
class LLDataPacker;

/// Minimum avatar pixel coverage required for keyframe motion activation (LOD threshold)
#define MIN_REQUIRED_PIXEL_AREA_KEYFRAME (40.f)
/// Maximum number of joints allowed in an IK constraint chain
#define MAX_CHAIN_LENGTH (4)

/// Binary format version number for .anim file compatibility
const S32 KEYFRAME_MOTION_VERSION = 1;
/// Binary format subversion for minor format changes within same version
const S32 KEYFRAME_MOTION_SUBVERSION = 0;

/**
 * @brief Foundation class for keyframe-based animation in the avatar system.
 * 
 * LLKeyframeMotion is the workhorse of avatar animation, handling playback of
 * keyframe animation data loaded from .anim files. This class manages the complex
 * process of interpolating between keyframes, applying joint transformations,
 * and maintaining animation constraints like inverse kinematics.
 * 
 * Key responsibilities:
 * - Loading and deserializing binary animation files (.anim format)
 * - Interpolating position, rotation, and scale curves between keyframes
 * - Managing animation loops, ease-in/out timing, and priority blending
 * - Handling inverse kinematics constraints for realistic foot placement
 * - Coordinating with hand motion and facial expression systems
 * - Optimizing performance through joint masking and LOD systems
 * 
 * The system supports three interpolation types:
 * - IT_STEP: No interpolation, snaps to keyframe values
 * - IT_LINEAR: Linear interpolation between keyframes
 * - IT_SPLINE: Smooth spline interpolation for natural motion
 * 
 * Performance characteristics:
 * - Activates only when avatar pixel coverage exceeds MIN_REQUIRED_PIXEL_AREA_KEYFRAME (40px)
 * - Uses joint masking to skip updates for non-visible joints
 * - Employs keyframe data caching to avoid redundant file loading
 * - Supports up to MAX_CHAIN_LENGTH (4) joints in IK constraint chains
 * 
 * @code
 * // Real usage: Motion registration during avatar initialization
 * // All motions are registered once when first avatar is created
 * if (LLCharacter::sInstances.size() == 1) {
 *     registerMotion(ANIM_AGENT_WALK, LLKeyframeWalkMotion::create);
 *     registerMotion(ANIM_AGENT_RUN, LLKeyframeMotion::create);
 *     registerMotion(ANIM_AGENT_TURNLEFT, LLKeyframeMotion::create);
 *     // ~40 more motion types registered globally
 * }
 * 
 * // Animation loading and playback
 * character->startMotion(ANIM_AGENT_DANCE1);  // Triggers asset fetch if needed
 * // Asset system calls onLoadComplete() when .anim file arrives
 * // Motion becomes active and updates joints every frame
 * @endcode
 */
class LLKeyframeMotion :
    public LLMotion
{
    /// Grant LLKeyframeDataCache access to private members for data caching operations
    friend class LLKeyframeDataCache;
public:
    /**
     * @brief Constructor for keyframe motion.
     * 
     * Initializes motion with default state and prepares for asset loading.
     * Motion remains uninitialized until onInitialize() is called successfully.
     * 
     * @param id UUID identifier for this motion instance
     */
    LLKeyframeMotion(const LLUUID &id);

    /**
     * @brief Virtual destructor ensures proper cleanup of animation data.
     */
    virtual ~LLKeyframeMotion();

private:
    /**
     * @brief Gets joint state by index with bounds checking.
     * 
     * Private helper that wraps array access with assertions for debugging.
     * Used internally to safely access joint states during animation updates.
     * 
     * @param index Index into joint state array
     * @return Reference to joint state at specified index
     */
    LLPointer<LLJointState>& getJointState(U32 index);
    
    /**
     * @brief Gets joint by index with bounds checking.
     * 
     * Private helper that wraps array access with assertions for debugging.
     * Used internally to safely access joints during animation setup.
     * 
     * @param index Index into joint array
     * @return Pointer to joint at specified index
     */
    LLJoint* getJoint(U32 index );

public:
    /**
     * @brief Static factory function for motion registry.
     * 
     * All motion classes must provide a static create function that the motion
     * registry uses to instantiate new motion objects. This factory pattern
     * allows the animation system to create motions by UUID without knowing
     * specific class types.
     * 
     * @param id UUID identifier for this motion instance
     * @return New LLKeyframeMotion instance ready for initialization
     */
    static LLMotion *create(const LLUUID& id);

public:
    /**
     * @brief Indicates whether this animation loops continuously.
     * 
     * Loop behavior is defined in the .anim file data and cached in the
     * JointMotionList. Most avatar animations loop (walk, run, fly) while
     * gestures and emotes typically play once.
     * 
     * @return true if animation should repeat, false for one-shot playback
     */
    virtual bool getLoop() {
        if (mJointMotionList) return mJointMotionList->mLoop;
        else return false;
    }

    /**
     * @brief Gets the total duration of the animation in seconds.
     * 
     * Duration is defined by the last keyframe timestamp in the .anim file.
     * For looping animations, this represents one complete cycle. For
     * non-looping animations, this is the total playback time.
     * 
     * @return Animation duration in seconds, 0.0 if not loaded
     */
    virtual F32 getDuration() {
        if (mJointMotionList) return mJointMotionList->mDuration;
        else return 0.f;
    }

    /**
     * @brief Gets the ease-in transition duration in seconds.
     * 
     * Ease-in time allows smooth blending when this motion starts,
     * gradually increasing its influence from 0 to full weight.
     * Specified in the .anim file metadata.
     * 
     * @return Ease-in duration in seconds, 0.0 for immediate activation
     */
    virtual F32 getEaseInDuration() {
        if (mJointMotionList) return mJointMotionList->mEaseInDuration;
        else return 0.f;
    }

    /**
     * @brief Gets the ease-out transition duration in seconds.
     * 
     * Ease-out time allows smooth blending when this motion stops,
     * gradually decreasing its influence from full weight to 0.
     * Specified in the .anim file metadata.
     * 
     * @return Ease-out duration in seconds, 0.0 for immediate deactivation
     */
    virtual F32 getEaseOutDuration() {
        if (mJointMotionList) return mJointMotionList->mEaseOutDuration;
        else return 0.f;
    }

    /**
     * @brief Gets the animation priority for blending resolution.
     * 
     * Priority determines which animations take precedence when multiple
     * motions affect the same joints. Higher priority animations override
     * or blend with lower priority ones. Specified in .anim file metadata.
     * 
     * @return Priority level from LLJoint::JointPriority enum
     */
    virtual LLJoint::JointPriority getPriority() {
        if (mJointMotionList) return mJointMotionList->mBasePriority;
        else return LLJoint::LOW_PRIORITY;
    }

    /**
     * @brief Gets the number of joints animated by this motion.
     * 
     * Returns the count of joints that have keyframe data in this animation.
     * Used for memory allocation and performance optimization decisions.
     * 
     * @return Number of animated joints, 0 if motion not loaded
     */
    virtual S32 getNumJointMotions()
    {
        if (mJointMotionList)
        {
            return mJointMotionList->getNumJointMotions();
        }
        return 0;
    }

    /**
     * @brief Gets the blending type for combining with other animations.
     * 
     * @return NORMAL_BLEND for standard animation blending
     */
    virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    /**
     * @brief Gets minimum avatar pixel coverage required for motion activation.
     * 
     * Motion LOD system uses pixel area to determine which animations to run
     * based on avatar visibility and distance from camera.
     * 
     * @return MIN_REQUIRED_PIXEL_AREA_KEYFRAME (40px) for keyframe motion activation
     */
    virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_KEYFRAME; }

    /**
     * @brief Initializes keyframe motion for the specified character.
     * 
     * Loads animation data from asset system, sets up joint states, and
     * prepares motion for activation. Must complete successfully before
     * motion can be activated.
     * 
     * @param character Avatar character to initialize motion for
     * @return STATUS_SUCCESS if ready for activation, STATUS_HOLD if still loading
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character);

    /**
     * @brief Activates keyframe motion for playback.
     * 
     * Sets up initial pose state and prepares for frame-by-frame updates.
     * Called when motion transitions from inactive to active state.
     * 
     * @return true if activation succeeded, false to deactivate immediately
     */
    virtual bool onActivate();

    /**
     * @brief Updates keyframe animation for current frame.
     * 
     * Core animation update that applies keyframe data to joints,
     * handles constraints, and manages animation timing and blending.
     * Called every frame while motion is active.
     * 
     * @param time Current animation time in seconds
     * @param joint_mask Bitmask of joints this motion can modify
     * @return true to continue animation, false when complete
     */
    virtual bool onUpdate(F32 time, U8* joint_mask);

    /**
     * @brief Deactivates keyframe motion and cleans up state.
     * 
     * Called when motion transitions from active to inactive state.
     * Releases joint control back to animation system.
     */
    virtual void onDeactivate();

    /**
     * @brief Sets the time at which motion should stop playing.
     * 
     * Used for controlled motion termination at specific points
     * in the animation timeline.
     * 
     * @param time Time in seconds when motion should stop
     */
    virtual void setStopTime(F32 time);

    /**
     * @brief Static callback for animation asset loading completion.
     * 
     * Called by the asset system when .anim file loading completes.
     * Handles both successful loads and error conditions.
     * 
     * @param asset_uuid UUID of the animation asset that finished loading
     * @param type Asset type (should be AT_ANIMATION)
     * @param user_data Motion instance that requested the load
     * @param status Load result status code
     * @param ext_status Extended status information
     */
    static void onLoadComplete(const LLUUID& asset_uuid,
                               LLAssetType::EType type,
                               void* user_data, S32 status, LLExtStat ext_status);

public:
    /**
     * @brief Gets the size in bytes of the loaded animation data.
     * 
     * @return Size of animation file data in bytes, 0 if not loaded
     */
    U32     getFileSize();
    
    /**
     * @brief Serializes animation data to a data packer.
     * 
     * Used for caching and network transmission of animation data.
     * 
     * @param dp Data packer to write animation data to
     * @return true if serialization succeeded
     */
    bool    serialize(LLDataPacker& dp) const;
    
    /**
     * @brief Deserializes animation data from a data packer.
     * 
     * Loads animation data from .anim file format, creating joint motion
     * curves and constraint data.
     * 
     * @param dp Data packer containing animation file data
     * @param asset_id UUID of the animation asset for identification
     * @param allow_invalid_joints Whether to skip unknown joint names
     * @return true if deserialization succeeded
     */
    bool    deserialize(LLDataPacker& dp, const LLUUID& asset_id, bool allow_invalid_joints = true);
    
    /**
     * @brief Checks if animation data has been loaded from asset system.
     * 
     * @return true if motion data is loaded and ready for playback
     */
    bool    isLoaded() { return mJointMotionList != NULL; }
    
    /**
     * @brief Exports animation data to a file for debugging.
     * 
     * Writes human-readable animation data including keyframes,
     * constraints, and timing information.
     * 
     * @param name Filename to write debug data to
     * @return true if file write succeeded
     */
    bool    dumpToFile(const std::string& name);


    /**
     * @brief Sets whether this animation should loop continuously.
     * 
     * @param loop true for looping playback, false for one-shot
     */
    void setLoop(bool loop);

    /**
     * @brief Gets the loop-in point for looping animations.
     * 
     * @return Time in seconds where loop cycles begin, 0.0 if not loaded
     */
    F32 getLoopIn() {
        return (mJointMotionList) ? mJointMotionList->mLoopInPoint : 0.f;
    }

    /**
     * @brief Gets the loop-out point for looping animations.
     * 
     * @return Time in seconds where loop cycles end, 0.0 if not loaded
     */
    F32 getLoopOut() {
        return (mJointMotionList) ? mJointMotionList->mLoopOutPoint : 0.f;
    }

    /**
     * @brief Sets the loop-in point for looping animations.
     * 
     * @param in_point Time in seconds where loops should begin
     */
    void setLoopIn(F32 in_point);

    /**
     * @brief Sets the loop-out point for looping animations.
     * 
     * @param out_point Time in seconds where loops should end
     */
    void setLoopOut(F32 out_point);

    /**
     * @brief Sets the hand pose for this animation.
     * 
     * Hand poses work alongside keyframe data to control hand appearance
     * through visual parameter morphs.
     * 
     * @param pose Hand pose type from LLHandMotion::eHandPose
     */
    void setHandPose(LLHandMotion::eHandPose pose) {
        if (mJointMotionList) mJointMotionList->mHandPose = pose;
    }

    /**
     * @brief Gets the hand pose associated with this animation.
     * 
     * @return Hand pose type, HAND_POSE_RELAXED if not loaded
     */
    LLHandMotion::eHandPose getHandPose() {
        return (mJointMotionList) ? mJointMotionList->mHandPose : LLHandMotion::HAND_POSE_RELAXED;
    }

    /**
     * @brief Sets the animation priority for blending resolution.
     * 
     * @param priority Priority level for blend conflict resolution
     */
    void setPriority(S32 priority);

    /**
     * @brief Associates a facial expression with this animation.
     * 
     * @param emote_id UUID of facial expression to play alongside animation
     */
    void setEmote(const LLUUID& emote_id);

    /**
     * @brief Sets the ease-in duration for smooth motion activation.
     * 
     * @param ease_in Duration in seconds for gradual blend-in
     */
    void setEaseIn(F32 ease_in);

    /**
     * @brief Sets the ease-out duration for smooth motion deactivation.
     * 
     * @param ease_out Duration in seconds for gradual blend-out
     */
    void setEaseOut(F32 ease_out);

    /**
     * @brief Gets the time of the last animation loop completion.
     * 
     * Used for timing coordination and loop cycle tracking.
     * 
     * @return Time in seconds of last loop completion
     */
    F32 getLastUpdateTime() { return mLastLoopedTime; }

    /**
     * @brief Gets the pelvis bounding box for this animation.
     * 
     * Used for animation range estimation and LOD calculations.
     * 
     * @return Local bounding box of pelvis movement during animation
     */
    const LLBBoxLocal& getPelvisBBox();

    /**
     * @brief Clears all cached keyframe data to free memory.
     * 
     * Called during memory pressure or when animation system is reset.
     * Forces reload of all animation data on next use.
     */
    static void flushKeyframeCache();

protected:
    /**
     * @brief Shared configuration data for inverse kinematics constraints.
     * 
     * Contains immutable constraint parameters that can be shared across
     * multiple constraint instances. This design reduces memory usage when
     * multiple similar constraints are active (e.g., both feet on ground).
     * 
     * IK constraints define target positions or orientations that joint chains
     * should achieve, with the solver computing appropriate joint rotations
     * to satisfy the constraint while maintaining natural poses.
     */
    class JointConstraintSharedData
    {
    public:
        /**
         * @brief Constructor initializes constraint parameters to safe defaults.
         */
        JointConstraintSharedData() :
            mChainLength(0),
            mEaseInStartTime(0.f),
            mEaseInStopTime(0.f),
            mEaseOutStartTime(0.f),
            mEaseOutStopTime(0.f),
            mUseTargetOffset(false),
            mConstraintType(CONSTRAINT_TYPE_POINT),
            mConstraintTargetType(CONSTRAINT_TARGET_TYPE_BODY),
            mSourceConstraintVolume(0),
            mTargetConstraintVolume(0),
            mJointStateIndices(NULL)
        { };
        
        /**
         * @brief Destructor cleans up joint state index array.
         */
        ~JointConstraintSharedData() { delete [] mJointStateIndices; }

        /// Index of collision volume that serves as constraint source
        S32                     mSourceConstraintVolume;
        /// Position offset from source volume origin for constraint anchor
        LLVector3               mSourceConstraintOffset;
        /// Index of collision volume that serves as constraint target
        S32                     mTargetConstraintVolume;
        /// Position offset from target volume origin for constraint goal
        LLVector3               mTargetConstraintOffset;
        /// Directional vector for constraint orientation (for axis constraints)
        LLVector3               mTargetConstraintDir;
        /// Number of joints in the IK chain (limited by MAX_CHAIN_LENGTH)
        S32                     mChainLength;
        /// Array of joint state indices that comprise the IK chain
        S32*                    mJointStateIndices;
        /// Animation time when constraint begins to activate
        F32                     mEaseInStartTime;
        /// Animation time when constraint reaches full strength
        F32                     mEaseInStopTime;
        /// Animation time when constraint begins to deactivate
        F32                     mEaseOutStartTime;
        /// Animation time when constraint is fully deactivated
        F32                     mEaseOutStopTime;
        /// Whether to apply target offset vector to constraint goal
        bool                    mUseTargetOffset;
        /// Type of constraint (point, plane, axis) defining solving method
        EConstraintType         mConstraintType;
        /// Target reference frame (body, ground, object) for constraint goal
        EConstraintTargetType   mConstraintTargetType;
    };

    /**
     * @brief Runtime inverse kinematics constraint instance.
     * 
     * Represents an active IK constraint that applies forces to a chain of
     * joints to achieve target positioning. Each constraint tracks its own
     * state, weight, and computed joint positions for the IK solver.
     */
    class JointConstraint
    {
    public:
        /**
         * @brief Constructor creates constraint instance with shared configuration.
         * 
         * @param shared_data Pointer to shared constraint parameters
         */
        JointConstraint(JointConstraintSharedData* shared_data);
        
        /**
         * @brief Destructor cleans up constraint instance.
         */
        ~JointConstraint();

        /// Shared configuration data for this constraint type
        JointConstraintSharedData*  mSharedData;
        /// Current constraint strength (0.0 = inactive, 1.0 = full strength)
        F32                         mWeight;
        /// Total length of the joint chain from root to end effector
        F32                         mTotalLength;
        /// Computed positions for each joint in the IK chain during solving
        LLVector3                   mPositions[MAX_CHAIN_LENGTH];
        /// Length of each bone segment in the joint chain
        F32                         mJointLengths[MAX_CHAIN_LENGTH];
        /// Normalized length fractions for proportional chain scaling
        F32                         mJointLengthFractions[MAX_CHAIN_LENGTH];
        /// Whether this constraint is currently participating in IK solving
        bool                        mActive;
        /// Ground contact position in global coordinates for ground-based constraints
        LLVector3d                  mGroundPos;
        /// Ground surface normal at contact point for natural foot placement
        LLVector3                   mGroundNorm;
        /// Source collision volume joint for constraint anchor point
        LLJoint*                    mSourceVolume;
        /// Target collision volume joint for constraint goal position
        LLJoint*                    mTargetVolume;
        /// Root mean square distance error for constraint solving convergence
        F32                         mFixupDistanceRMS;
    };

    /**
     * @brief Applies keyframe animation data to joint states for current time.
     * 
     * Core animation update function that interpolates keyframe data and
     * applies the resulting transformations to all animated joints.
     * 
     * @param time Current animation time in seconds
     */
    void applyKeyframes(F32 time);

    /**
     * @brief Applies inverse kinematics constraints to joint chains.
     * 
     * Processes all active IK constraints, solving for joint positions
     * that satisfy constraint requirements while maintaining natural poses.
     * 
     * @param time Current animation time in seconds
     * @param joint_mask Bitmask of joints that can be modified
     */
    void applyConstraints(F32 time, U8* joint_mask);

    /**
     * @brief Activates an IK constraint for solving.
     * 
     * Enables a constraint and initializes its solver state for
     * active participation in constraint resolution.
     * 
     * @param constraintp Constraint to activate
     */
    void activateConstraint(JointConstraint* constraintp);

    /**
     * @brief Initializes constraint parameters and joint chain references.
     * 
     * Sets up constraint data structures and validates joint chain
     * connectivity for proper IK solving.
     * 
     * @param constraint Constraint to initialize
     */
    void initializeConstraint(JointConstraint* constraint);

    /**
     * @brief Deactivates an IK constraint from solving.
     * 
     * Removes a constraint from active solving and cleans up its state.
     * 
     * @param constraintp Constraint to deactivate
     */
    void deactivateConstraint(JointConstraint *constraintp);

    /**
     * @brief Applies a single IK constraint to its joint chain.
     * 
     * Solves for joint positions that satisfy the constraint target
     * while respecting joint limits and natural pose requirements.
     * 
     * @param constraintp Constraint to apply
     * @param time Current animation time in seconds
     * @param joint_mask Bitmask of joints that can be modified
     */
    void applyConstraint(JointConstraint* constraintp, F32 time, U8* joint_mask);

    /**
     * @brief Initializes joint states and pose blending for animation playback.
     * 
     * Sets up the initial pose configuration and prepares joint states
     * for keyframe animation and constraint solving.
     * 
     * @return true if pose setup succeeded, false for initialization failure
     */
    bool    setupPose();

public:
    /**
     * @brief Asset loading status for animation files.
     * 
     * Tracks the current state of animation asset loading from the asset system.
     * Used to coordinate between animation requests and file availability.
     */
    enum AssetStatus { 
        ASSET_LOADED,       ///< Animation data loaded and ready for use
        ASSET_FETCHED,      ///< Asset downloaded but not yet processed
        ASSET_NEEDS_FETCH,  ///< Asset needs to be requested from asset system
        ASSET_FETCH_FAILED, ///< Asset loading failed permanently
        ASSET_UNDEFINED     ///< Asset status not yet determined
    };

    /**
     * @brief Keyframe interpolation methods for smooth animation.
     * 
     * Defines how values are calculated between discrete keyframes
     * to create smooth motion trajectories.
     */
    enum InterpolationType { 
        IT_STEP,    ///< No interpolation, snap to keyframe values
        IT_LINEAR,  ///< Linear interpolation between keyframes
        IT_SPLINE   ///< Smooth spline interpolation for natural motion
    };

    /**
     * @brief Single keyframe data point for joint scale animation.
     * 
     * Represents one keyframe containing a timestamp and scale vector.
     * Used in ScaleCurve to define scale animation over time.
     */
    class ScaleKey
    {
    public:
        /**
         * @brief Default constructor creates keyframe at time 0.
         */
        ScaleKey() { mTime = 0.0f; }
        
        /**
         * @brief Constructor creates keyframe with specified time and scale.
         * 
         * @param time Timestamp in seconds for this keyframe
         * @param scale Scale vector for joint at this time
         */
        ScaleKey(F32 time, const LLVector3 &scale) { mTime = time; mScale = scale; }

        /// Timestamp in animation timeline for this keyframe
        F32         mTime;
        /// Scale vector (x, y, z scaling factors) for this keyframe
        LLVector3   mScale;
    };

    /**
     * @brief Single keyframe data point for joint rotation animation.
     * 
     * Represents one keyframe containing a timestamp and rotation quaternion.
     * Used in RotationCurve to define rotational animation over time.
     */
    class RotationKey
    {
    public:
        /**
         * @brief Default constructor creates keyframe at time 0.
         */
        RotationKey() { mTime = 0.0f; }
        
        /**
         * @brief Constructor creates keyframe with specified time and rotation.
         * 
         * @param time Timestamp in seconds for this keyframe
         * @param rotation Rotation quaternion for joint at this time
         */
        RotationKey(F32 time, const LLQuaternion &rotation) { mTime = time; mRotation = rotation; }

        /// Timestamp in animation timeline for this keyframe
        F32             mTime;
        /// Rotation quaternion for this keyframe
        LLQuaternion    mRotation;
    };

    /**
     * @brief Single keyframe data point for joint position animation.
     * 
     * Represents one keyframe containing a timestamp and position vector.
     * Used in PositionCurve to define positional animation over time.
     */
    class PositionKey
    {
    public:
        /**
         * @brief Default constructor creates keyframe at time 0.
         */
        PositionKey() { mTime = 0.0f; }
        
        /**
         * @brief Constructor creates keyframe with specified time and position.
         * 
         * @param time Timestamp in seconds for this keyframe
         * @param position Position vector for joint at this time
         */
        PositionKey(F32 time, const LLVector3 &position) { mTime = time; mPosition = position; }

        /// Timestamp in animation timeline for this keyframe
        F32         mTime;
        /// Position vector relative to parent joint for this keyframe
        LLVector3   mPosition;
    };

    /**
     * @brief Animation curve for joint scale over time.
     * 
     * Manages a timeline of scale keyframes and provides interpolated
     * scale values for any point in the animation. Supports multiple
     * interpolation methods and looping behavior.
     */
    class ScaleCurve
    {
    public:
        /**
         * @brief Constructor initializes empty scale curve.
         */
        ScaleCurve();
        
        /**
         * @brief Destructor cleans up curve data.
         */
        ~ScaleCurve();
        
        /**
         * @brief Gets interpolated scale value at specified time.
         * 
         * @param time Current animation time in seconds
         * @param duration Total animation duration for loop calculations
         * @return Interpolated scale vector at the specified time
         */
        LLVector3 getValue(F32 time, F32 duration);
        
        /**
         * @brief Interpolates between two scale keyframes.
         * 
         * @param u Interpolation factor (0.0 to 1.0)
         * @param before Earlier keyframe in timeline
         * @param after Later keyframe in timeline
         * @return Interpolated scale value between the keyframes
         */
        LLVector3 interp(F32 u, ScaleKey& before, ScaleKey& after);

        /// Method used for interpolating between keyframes
        InterpolationType   mInterpolationType;
        /// Number of keyframes in this curve
        S32                 mNumKeys;
        /// Map from time to keyframe data for efficient lookup
        typedef std::map<F32, ScaleKey> key_map_t;
        key_map_t           mKeys;
        /// Cached keyframe for loop-in point
        ScaleKey            mLoopInKey;
        /// Cached keyframe for loop-out point
        ScaleKey            mLoopOutKey;
    };

    /**
     * @brief Animation curve for joint rotation over time.
     * 
     * Manages a timeline of rotation keyframes and provides interpolated
     * rotation values for any point in the animation. Uses quaternion
     * interpolation for smooth rotational transitions.
     */
    class RotationCurve
    {
    public:
        /**
         * @brief Constructor initializes empty rotation curve.
         */
        RotationCurve();
        
        /**
         * @brief Destructor cleans up curve data.
         */
        ~RotationCurve();
        
        /**
         * @brief Gets interpolated rotation value at specified time.
         * 
         * @param time Current animation time in seconds
         * @param duration Total animation duration for loop calculations
         * @return Interpolated quaternion rotation at the specified time
         */
        LLQuaternion getValue(F32 time, F32 duration);
        
        /**
         * @brief Interpolates between two rotation keyframes using quaternions.
         * 
         * @param u Interpolation factor (0.0 to 1.0)
         * @param before Earlier keyframe in timeline
         * @param after Later keyframe in timeline
         * @return Interpolated rotation value between the keyframes
         */
        LLQuaternion interp(F32 u, RotationKey& before, RotationKey& after);

        /// Method used for interpolating between keyframes
        InterpolationType   mInterpolationType;
        /// Number of keyframes in this curve
        S32                 mNumKeys;
        /// Map from time to keyframe data for efficient lookup
        typedef std::map<F32, RotationKey> key_map_t;
        key_map_t       mKeys;
        /// Cached keyframe for loop-in point
        RotationKey     mLoopInKey;
        /// Cached keyframe for loop-out point
        RotationKey     mLoopOutKey;
    };

    /**
     * @brief Animation curve for joint position over time.
     * 
     * Manages a timeline of position keyframes and provides interpolated
     * position values for any point in the animation. Handles relative
     * positioning within parent joint coordinate space.
     */
    class PositionCurve
    {
    public:
        /**
         * @brief Constructor initializes empty position curve.
         */
        PositionCurve();
        
        /**
         * @brief Destructor cleans up curve data.
         */
        ~PositionCurve();
        
        /**
         * @brief Gets interpolated position value at specified time.
         * 
         * @param time Current animation time in seconds
         * @param duration Total animation duration for loop calculations
         * @return Interpolated position vector at the specified time
         */
        LLVector3 getValue(F32 time, F32 duration);
        
        /**
         * @brief Interpolates between two position keyframes.
         * 
         * @param u Interpolation factor (0.0 to 1.0)
         * @param before Earlier keyframe in timeline
         * @param after Later keyframe in timeline
         * @return Interpolated position value between the keyframes
         */
        LLVector3 interp(F32 u, PositionKey& before, PositionKey& after);

        /// Method used for interpolating between keyframes
        InterpolationType   mInterpolationType;
        /// Number of keyframes in this curve
        S32                 mNumKeys;
        /// Map from time to keyframe data for efficient lookup
        typedef std::map<F32, PositionKey> key_map_t;
        key_map_t       mKeys;
        /// Cached keyframe for loop-in point
        PositionKey     mLoopInKey;
        /// Cached keyframe for loop-out point
        PositionKey     mLoopOutKey;
    };

    /**
     * @brief Complete animation data for a single joint.
     * 
     * Contains all animation curves (position, rotation, scale) for one joint
     * along with metadata about how the joint should be animated. This is the
     * core data structure loaded from .anim files.
     */
    class JointMotion
    {
    public:
        /// Position animation curve for this joint
        PositionCurve   mPositionCurve;
        /// Rotation animation curve for this joint
        RotationCurve   mRotationCurve;
        /// Scale animation curve for this joint
        ScaleCurve      mScaleCurve;
        /// Name of the joint this motion applies to
        std::string     mJointName;
        /// Bitmask indicating which transformation components are animated
        U32             mUsage;
        /// Priority level for this joint's animation
        LLJoint::JointPriority  mPriority;

        /**
         * @brief Updates joint state with interpolated values for current time.
         * 
         * Applies position, rotation, and scale values from curves to the
         * joint state based on current animation time.
         * 
         * @param joint_state Target joint state to update
         * @param time Current animation time in seconds
         * @param duration Total animation duration for loop calculations
         */
        void update(LLJointState* joint_state, F32 time, F32 duration);
    };

    /**
     * @brief Complete animation data loaded from a .anim file.
     * 
     * Contains all joint motion data, timing information, constraints, and
     * metadata for a complete animation. This is the root data structure
     * created when deserializing animation files and cached for performance.
     * 
     * Key components:
     * - Array of JointMotion objects, one per animated joint
     * - Animation timing (duration, loop points, ease in/out)
     * - Priority and hand pose information
     * - IK constraints for realistic movement
     * - Pelvis bounding box for animation range estimation
     * - Associated facial expression data
     */
    class JointMotionList
    {
    public:
        /// Array of joint animation data, one per animated joint in the skeleton
        std::vector<JointMotion*> mJointMotionArray;
        /// Total animation duration in seconds from first to last keyframe
        F32                     mDuration;
        /// Whether this animation should loop continuously (true) or play once (false)
        bool                    mLoop;
        /// Time point in seconds where looping animation cycles begin
        F32                     mLoopInPoint;
        /// Time point in seconds where looping animation cycles end
        F32                     mLoopOutPoint;
        /// Duration in seconds for gradual blend-in when animation starts
        F32                     mEaseInDuration;
        /// Duration in seconds for gradual blend-out when animation stops
        F32                     mEaseOutDuration;
        /// Base priority level for animation blending conflict resolution
        LLJoint::JointPriority  mBasePriority;
        /// Hand pose type for coordinated hand animation (fist, relaxed, etc.)
        LLHandMotion::eHandPose mHandPose;
        /// Highest priority level among all joints in this animation
        LLJoint::JointPriority  mMaxPriority;
        /// Type definition for constraint list container
        typedef std::list<JointConstraintSharedData*> constraint_list_t;
        /// List of IK constraints that apply during this animation
        constraint_list_t       mConstraints;
        /// Pelvis movement bounding box for animation range estimation and LOD
        LLBBoxLocal             mPelvisBBox;
        /// Name of associated facial expression animation (cached for performance)
        /// @note mEmoteName is cached here to avoid separate lookups during animation
        /// @todo Consider refactoring to return combined data structure from cache
        std::string             mEmoteName;
        /// UUID of associated facial expression animation
        LLUUID                  mEmoteID;

    public:
        /**
         * @brief Constructor initializes empty joint motion list.
         */
        JointMotionList();
        
        /**
         * @brief Destructor cleans up all joint motion data.
         */
        ~JointMotionList();
        
        /**
         * @brief Prints diagnostic information about animation data.
         * 
         * Outputs details about joint motions, keyframe counts, constraints,
         * and memory usage for debugging and performance analysis.
         * 
         * @return Total memory usage in bytes
         */
        U32 dumpDiagInfo();
        
        /**
         * @brief Gets joint motion data by array index.
         * 
         * @param index Array index of the joint motion to retrieve
         * @return Pointer to joint motion data at specified index
         */
        JointMotion* getJointMotion(U32 index) const { llassert(index < mJointMotionArray.size()); return mJointMotionArray[index]; }
        
        /**
         * @brief Gets the total number of animated joints.
         * 
         * @return Count of joints that have motion data in this animation
         */
        U32 getNumJointMotions() const { return static_cast<U32>(mJointMotionArray.size()); }
    };

protected:
    /// Complete animation data loaded from .anim file (position, rotation, scale curves)
    JointMotionList*                mJointMotionList;
    /// Array of joint states for applying animation transformations
    std::vector<LLPointer<LLJointState> > mJointStates;
    /// Pointer to pelvis joint for root motion and constraint calculations
    LLJoint*                        mPelvisp;
    /// Avatar character this motion is applied to
    LLCharacter*                    mCharacter;
    /// Type definition for constraint list container
    typedef std::list<JointConstraint*> constraint_list_t;
    /// Active IK constraints for this motion
    constraint_list_t               mConstraints;
    /// Last known skeleton serial number for change detection
    U32                             mLastSkeletonSerialNum;
    /// Time of the last animation update for delta calculations
    F32                             mLastUpdateTime;
    /// Time when animation last completed a loop cycle
    F32                             mLastLoopedTime;
    /// Current status of animation asset loading
    AssetStatus                     mAssetStatus;

public:
    /**
     * @brief Sets the character this motion applies to.
     * 
     * @param character Avatar character to animate with this motion
     */
    void setCharacter(LLCharacter* character) { mCharacter = character; }
};

/**
 * @brief Global cache for loaded keyframe animation data.
 * 
 * LLKeyframeDataCache provides centralized storage and management of animation
 * data loaded from .anim files. Multiple avatar instances can share the same
 * animation data without redundant loading, significantly reducing memory usage
 * and asset system load.
 * 
 * Key features:
 * - Shared animation data across all avatars using the same motions
 * - Automatic reference counting for memory management
 * - Global static storage for efficient access patterns
 * - Diagnostic tools for memory usage analysis
 * 
 * The cache maps animation UUIDs to JointMotionList objects containing
 * the complete keyframe, constraint, and timing data for each animation.
 * This allows the system to load popular animations (walk, run, stand) once
 * and reuse them across hundreds of avatars in the scene.
 * 
 * @code
 * // Real usage: Animation system checks cache before asset loading
 * LLKeyframeMotion::JointMotionList* data = 
 *     LLKeyframeDataCache::getKeyframeData(motionID);
 * if (!data) {
 *     // Motion not cached, request from asset system
 *     gAssetStorage->getInvItemAsset(motionID, LLAssetType::AT_ANIMATION, 
 *                                    LLKeyframeMotion::onLoadComplete, this);
 * }
 * @endcode
 */
class LLKeyframeDataCache
{
public:
    /**
     * @brief Default constructor for cache instance.
     * 
     * Note: Should be implemented as singleton member of LLKeyframeMotion
     * for better encapsulation and lifecycle management.
     */
    LLKeyframeDataCache(){};
    
    /**
     * @brief Destructor cleans up cached animation data.
     */
    ~LLKeyframeDataCache();

    /// Map type for UUID to animation data associations
    typedef std::map<LLUUID, class LLKeyframeMotion::JointMotionList*> keyframe_data_map_t;
    /// Global static cache of all loaded animation data
    static keyframe_data_map_t sKeyframeDataMap;

    /**
     * @brief Adds animation data to the global cache.
     * 
     * Stores loaded animation data in the cache for sharing across
     * multiple motion instances and avatar characters.
     * 
     * @param id UUID identifier of the animation
     * @param data Pointer to loaded JointMotionList data
     */
    static void addKeyframeData(const LLUUID& id, LLKeyframeMotion::JointMotionList* data);
    
    /**
     * @brief Retrieves animation data from the cache.
     * 
     * Looks up previously loaded animation data by UUID. Returns NULL
     * if the animation has not been cached yet.
     * 
     * @param id UUID identifier of the animation to retrieve
     * @return Pointer to cached animation data, or NULL if not found
     */
    static LLKeyframeMotion::JointMotionList* getKeyframeData(const LLUUID& id);

    /**
     * @brief Removes animation data from the cache.
     * 
     * Removes cached animation data and frees associated memory.
     * Used during cache cleanup or when animations are no longer needed.
     * 
     * @param id UUID identifier of the animation to remove
     */
    static void removeKeyframeData(const LLUUID& id);

    /**
     * @brief Prints diagnostic information about cached animations.
     * 
     * Outputs details about cache contents, memory usage, and performance
     * statistics for debugging and optimization analysis.
     */
    static void dumpDiagInfo();
    
    /**
     * @brief Clears all cached animation data.
     * 
     * Removes all cached animations and frees associated memory.
     * Used during viewer shutdown or cache reset operations.
     */
    static void clear();
};

#endif // LL_LLKEYFRAMEMOTION_H


