/**
 * @file llmotion.h
 * @brief Implementation of LLMotion class.
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

#ifndef LL_LLMOTION_H
#define LL_LLMOTION_H

#include <string>

#include "llerror.h"
#include "llpose.h"
#include "lluuid.h"

class LLCharacter;

/**
 * @brief Abstract base class for all character animations in Second Life.
 * 
 * LLMotion defines the interface that all character animations must implement,
 * providing a standardized framework for motion creation, playback, and blending.
 * It serves as the foundation for the entire animation system in Second Life.
 * 
 * Key Responsibilities:
 * - Define motion properties (duration, priority, blend type, pixel area requirements)
 * - Implement animation lifecycle callbacks (initialize, activate, update, deactivate)
 * - Manage joint state and pose data for skeletal animation
 * - Support motion blending through normal and additive modes
 * - Handle ease-in/ease-out transitions for smooth animation blending
 * 
 * Motion Lifecycle with Error Handling:
 * @code
 * // 1. Motion creation (via LLMotionController::createMotion)
 * LLMotion* motion = motionRegistry.createMotion(id);
 * if (!motion) return nullptr; // Failed or blacklisted
 * 
 * // 2. Initialization with robust error handling
 * LLMotionInitStatus status = motion->onInitialize(character);
 * switch (status) {
 *     case STATUS_FAILURE:
 *         LL_INFOS() << "Motion " << id << " init failed." << LL_ENDL;
 *         sRegistry.markBad(id);  // Blacklist to prevent future attempts
 *         delete motion;
 *         return nullptr;
 *     case STATUS_HOLD:
 *         mLoadingMotions.insert(motion);  // Retry next frame
 *         break;
 *     case STATUS_SUCCESS:
 *         mLoadedMotions.insert(motion);   // Ready for activation
 *         break;
 * }
 * 
 * // 3. Activation with failure recovery
 * if (!motion->onActivate()) {
 *     // Motion failed to activate, immediately deactivate
 *     motion->onDeactivate();
 *     return false;
 * }
 * 
 * // 4. Frame updates with null pointer safety
 * if (motion && motion->getPose()) {
 *     bool continue_motion = motion->onUpdate(activeTime, joint_mask);
 *     if (!continue_motion) {
 *         // Motion signals completion, begin deactivation
 *         motion->onDeactivate();
 *     }
 * }
 * @endcode
 * 
 * Blending Modes:
 * - NORMAL_BLEND: Replaces existing joint rotations (typical for most animations)
 * - ADDITIVE_BLEND: Adds to existing rotations (used for targeting, breathing, etc.)
 * 
 * Performance Optimization - Pixel Area Requirements:
 * Motions use pixel area thresholds to automatically disable on distant avatars:
 * - LLKeyframeMotion: 40.f (most animations, very low threshold)
 * - LLTargetingMotion: 1000.f (aiming, medium threshold) 
 * - LLHandMotion: 10000.f (hand gestures, high threshold)
 * - LLEyeMotion: 25000.f (eye details, highest threshold - close-up only)
 * - LLTestMotion/LLNullMotion: 0.f (always active regardless of distance)
 * 
 * Additional Optimizations:
 * - canDeprecate() determines crossfade support (LLHandMotion returns false)
 * - Joint signature arrays track which joints need updates per motion
 * - Shared data caching in LLKeyframeDataCache prevents duplicate asset loading
 * 
 * Thread Safety: This class is not thread-safe and must only be used from
 * the main rendering thread where character updates occur.
 * 
 * Real-world Subclass Implementations:
 * The system includes numerous concrete motion types, each serving specific purposes:
 * 
 * Asset-Based Motions (require server data):
 * - LLKeyframeMotion: Complex BVH animation files with keyframe interpolation,
 *   joint constraints, and caching system. Supports position/rotation/scale curves
 *   with multiple interpolation types (IT_STEP, IT_LINEAR, IT_SPLINE). Includes
 *   sophisticated constraint system for IK and ground contact. (MIN_PIXEL_AREA: 40.f)
 * - LLKeyframeWalkMotion: Enhanced walking with terrain adaptation 
 * - LLKeyframeStandMotion: Standing poses with subtle variation
 * - LLKeyframeFallMotion: Falling and recovery animations
 * 
 * Procedural Motions (computed in real-time):
 * - LLTargetingMotion: Weapon/tool aiming (MIN_PIXEL_AREA: 1000.f, ADDITIVE_BLEND)
 * - LLHandMotion: Hand poses and gestures (MIN_PIXEL_AREA: 10000.f, MEDIUM_PRIORITY)
 * - LLHeadRotMotion: Head tracking and attention (MIN_PIXEL_AREA: 500.f, 1s ease-in/out)
 * - LLEyeMotion: Eye movement, blinking, jitter (MIN_PIXEL_AREA: 25000.f)
 * - LLBodyNoiseMotion: Subtle idle movement for realism
 * - LLBreatheMotionRot: Breathing chest movement
 * - LLEditingMotion: Special pose during object editing
 * - LLPhysicsMotionController: Breast/clothing physics simulation
 * 
 * Utility Motions:
 * - LLEmote: Facial expressions (registered for all ANIM_AGENT_EXPRESS_* UUIDs)
 * - LLNullMotion: Placeholder/fallback motion (1-second loops, HIGH_PRIORITY)
 * - LLTestMotion: Debug motion with lifecycle logging (HIGH_PRIORITY, no pixel requirements)
 * 
 * Registration occurs once during LLVOAvatar::initInstance() when the first avatar
 * is created (LLCharacter::sInstances.size() == 1), registering ~40 motion types
 * with the global LLMotionRegistry.
 */
class LLMotion
{
    friend class LLMotionController;

public:
    /**
     * @brief Blending modes for motion composition.
     * 
     * Determines how this motion's joint transformations combine with
     * existing pose data from other active motions.
     */
    enum LLMotionBlendType
    {
        /**
         * @brief Normal blending mode - replaces existing joint rotations.
         * 
         * Used by most character animations like walking, dancing, sitting.
         * Higher priority motions override lower priority ones for the same joints.
         */
        NORMAL_BLEND,
        
        /**
         * @brief Additive blending mode - adds to existing joint rotations.
         * 
         * Used for secondary motions like targeting, head following, breathing
         * that should layer on top of primary locomotion without replacing it.
         */
        ADDITIVE_BLEND
    };

    /**
     * @brief Status codes returned by motion initialization.
     * 
     * Indicates the result of the onInitialize() callback and determines
     * whether the motion can proceed to the loaded state.
     */
    enum LLMotionInitStatus
    {
        /**
         * @brief Initialization failed - motion cannot be used.
         * 
         * Returned when motion data is corrupted, incompatible, or missing.
         * The motion will be marked as bad and future creation attempts
         * for this UUID will be blocked.
         */
        STATUS_FAILURE,
        
        /**
         * @brief Initialization succeeded - motion is ready for activation.
         * 
         * The motion has completed all necessary setup and can be activated
         * immediately when requested. This is the normal success path.
         */
        STATUS_SUCCESS,
        
        /**
         * @brief Initialization on hold - try again later.
         * 
         * Returned when the motion is waiting for additional data or resources
         * that are not yet available. The motion controller will retry
         * initialization on subsequent update cycles.
         */
        STATUS_HOLD
    };

    /**
     * @brief Constructs a new motion with the specified UUID.
     * 
     * Initializes the motion with default settings and associates it with
     * the given identifier. The motion starts in an uninitialized state
     * and must complete initialization before it can be activated.
     * 
     * @param id UUID that uniquely identifies this motion instance
     */
    LLMotion(const LLUUID &id);

    /**
     * @brief Destroys the motion and cleans up resources.
     * 
     * Ensures proper cleanup of joint states and any motion-specific data.
     * The motion should already be deactivated before destruction.
     */
    virtual ~LLMotion();

public:
    //-------------------------------------------------------------------------
    // Interface functions to support MotionController and MotionRegistry
    //-------------------------------------------------------------------------

    /**
     * @brief Gets the user-readable name of this motion instance.
     * 
     * The name is typically set by the motion controller for debugging
     * and diagnostic purposes. It may differ from the motion's UUID.
     * 
     * @return Human-readable name string
     */
    const std::string &getName() const { return mName; }

    /**
     * @brief Sets the user-readable name of this motion instance.
     * 
     * Used by the motion controller to assign descriptive names for
     * debugging and logging purposes.
     * 
     * @param name Human-readable name to assign to this motion
     */
    void setName(const std::string &name) { mName = name; }

    /**
     * @brief Gets the UUID that uniquely identifies this motion.
     * 
     * @return Motion's unique identifier
     */
    const LLUUID& getID() const { return mID; }

    /**
     * @brief Gets the pose data representing the motion's current joint state.
     * 
     * The pose contains joint transformations that are applied to the character's
     * skeleton. This is the primary output of the motion system that affects
     * character appearance.
     * 
     * @return Pointer to the motion's current pose data
     */
    virtual LLPose* getPose() { return &mPose;}

    /**
     * @brief Initiates fade-out transition for this motion.
     * 
     * Begins reducing the motion's influence on the character pose, typically
     * used when the motion is being stopped or replaced by another motion.
     */
    void fadeOut();

    /**
     * @brief Initiates fade-in transition for this motion.
     * 
     * Begins increasing the motion's influence on the character pose, typically
     * used when the motion is being activated and needs to blend in smoothly.
     */
    void fadeIn();

    /**
     * @brief Gets the current fade weight for LOD-based blending.
     * 
     * The fade weight determines how much influence this motion has on the
     * final character pose. Used for level-of-detail optimizations where
     * distant characters may have reduced motion fidelity.
     * 
     * @return Current fade weight (0.0 = no influence, 1.0 = full influence)
     */
    F32 getFadeWeight() const { return mFadeWeight; }

    /**
     * @brief Gets the time when this motion was requested to stop.
     * 
     * @return Timestamp when stop was initiated (seconds since motion start)
     */
    F32 getStopTime() const { return mStopTimestamp; }

    /**
     * @brief Sets the time when this motion should stop.
     * 
     * Used by the motion controller to schedule motion termination.
     * The motion may continue playing during its ease-out phase after this time.
     * 
     * @param time Timestamp when motion should begin stopping
     */
    virtual void setStopTime(F32 time);

    /**
     * @brief Checks if this motion has been flagged as stopped.
     * 
     * @return true if motion is in stopped state, false if still playing
     */
    bool isStopped() const { return mStopped; }

    /**
     * @brief Sets the stopped flag for this motion.
     * 
     * Used internally by the motion controller to track motion state.
     * 
     * @param stopped true to mark as stopped, false to mark as playing
     */
    void setStopped(bool stopped) { mStopped = stopped; }

    /**
     * @brief Checks if this motion is currently in a blending transition.
     * 
     * Determines if the motion is fading in, fading out, or transitioning
     * between states. Used for optimization and state management.
     * 
     * @return true if motion is blending, false if at full influence
     */
    bool isBlending();

    /**
     * @brief Motion activation control documentation.
     * 
     * Activation rules:
     * - Any code can call activate() to request motion activation
     * - Only LLMotionController can call deactivate() to ensure proper cleanup
     * - mActive == true means the motion *may* be on the controller's active list
     * - mActive == false guarantees the motion is NOT on the active list
     * 
     * This design prevents external code from improperly deactivating motions
     * and ensures the motion controller maintains consistent state.
     */

protected:
    /**
     * @brief Deactivates this motion (LLMotionController only).
     * 
     * Marks the motion as inactive and removes it from active processing.
     * This method is private to ensure only the motion controller can
     * deactivate motions and maintain proper state consistency.
     */
    void deactivate();
    
    /**
     * @brief Checks if this motion is currently active.
     * 
     * @return true if motion is marked as active, false otherwise
     */
    bool isActive() { return mActive; }

public:
    /**
     * @brief Activates this motion for playback.
     * 
     * Marks the motion as active and sets the activation timestamp.
     * The motion controller will subsequently call onActivate() to
     * allow the motion to perform its activation logic.
     * 
     * @param time Current animation time when activation occurs
     */
    void activate(F32 time);

public:
    //-------------------------------------------------------------------------
    // Pure virtual interface - animation callbacks to be implemented by subclasses
    //-------------------------------------------------------------------------

    /**
     * @brief Determines if this motion loops continuously.
     * 
     * Looping motions automatically restart when they reach the end of their
     * duration. Non-looping motions play once and then stop. This affects
     * how the motion controller handles motion timing and termination.
     * 
     * @return true if motion loops, false if it plays once and stops
     */
    virtual bool getLoop() = 0;

    /**
     * @brief Gets the total duration of this motion in seconds.
     * 
     * For looping motions, this represents one complete cycle. For non-looping
     * motions, this is the total playback time. A duration of 0.0 indicates
     * an infinite or procedural motion that doesn't have a fixed endpoint.
     * 
     * @return Motion duration in seconds, 0.0 for infinite motions
     */
    virtual F32 getDuration() = 0;

    /**
     * @brief Gets the ease-in duration for smooth motion blending.
     * 
     * The ease-in period is the time at the beginning of motion playback
     * during which the motion gradually increases its influence on the
     * character pose. This prevents jarring transitions when motions start.
     * 
     * @return Ease-in duration in seconds
     */
    virtual F32 getEaseInDuration() = 0;

    /**
     * @brief Gets the ease-out duration for smooth motion blending.
     * 
     * The ease-out period is the time at the end of motion playback during
     * which the motion gradually decreases its influence on the character
     * pose. This ensures smooth transitions when motions stop or are replaced.
     * 
     * @return Ease-out duration in seconds
     */
    virtual F32 getEaseOutDuration() = 0;

    /**
     * @brief Gets the priority level for motion blending conflicts.
     * 
     * When multiple motions affect the same joints, higher priority motions
     * override lower priority ones. This enables a hierarchical blending
     * system where important motions (like falling) can override less
     * critical ones (like idle animations).
     * 
     * @return Joint priority level for blending resolution
     */
    virtual LLJoint::JointPriority getPriority() = 0;

    /**
     * @brief Gets the number of joints this motion affects.
     * 
     * Used for performance optimization and motion analysis. Motions that
     * affect fewer joints may be processed more efficiently. The default
     * implementation returns 0, indicating the motion doesn't report this metric.
     * 
     * @return Number of joints modified by this motion, 0 if not tracked
     */
    virtual S32 getNumJointMotions() { return 0; };

    /**
     * @brief Gets the blending mode for this motion.
     * 
     * Determines how this motion's joint transformations combine with
     * existing pose data. Most locomotion uses NORMAL_BLEND, while
     * secondary effects like targeting use ADDITIVE_BLEND.
     * 
     * @return Blending mode (NORMAL_BLEND or ADDITIVE_BLEND)
     */
    virtual LLMotionBlendType getBlendType() = 0;

    /**
     * @brief Gets the minimum pixel area required for this motion to activate.
     * 
     * Used for level-of-detail optimization. Characters that occupy fewer
     * pixels on screen (distant or small avatars) may skip motions with
     * high pixel area requirements to improve performance. Essential motions
     * should return a low value to ensure they always play.
     * 
     * @return Minimum pixel area needed for activation, 0.0 for always active
     */
    virtual F32 getMinPixelArea() = 0;

    /**
     * @brief Initializes the motion after creation and parameter setup.
     * 
     * Called by the motion controller after the motion is created but before
     * it can be activated. This is where motions should load required data,
     * validate parameters, and prepare for playback. The character pointer
     * provides access to the skeleton and other character properties.
     * 
     * @param character Pointer to the character this motion will animate
     * @return STATUS_SUCCESS if ready, STATUS_FAILURE if unusable, STATUS_HOLD to retry later
     */
    virtual LLMotionInitStatus onInitialize(LLCharacter *character) = 0;

    /**
     * @brief Updates the motion state and joint transformations each frame.
     * 
     * Called every frame while the motion is active. This is where the motion
     * calculates its current pose based on the elapsed active time and applies
     * transformations to the character's joints. The joint mask indicates
     * which joints are modified.
     * 
     * @param activeTime Time in seconds since motion activation
     * @param joint_mask Array indicating which joints this motion affects
     * @return true to continue playing, false to signal motion completion
     */
    virtual bool onUpdate(F32 activeTime, U8* joint_mask) = 0;

    /**
     * @brief Performs cleanup when the motion is deactivated.
     * 
     * Called when the motion stops playing, either due to natural completion
     * or external termination. Motions should clean up any temporary state
     * and ensure they leave the character in a consistent condition.
     */
    virtual void onDeactivate() = 0;

    /**
     * @brief Determines if this motion supports crossfading with new instances.
     * 
     * When a motion is restarted while already playing, crossfading allows
     * smooth transition from the old instance to the new one. Some motion
     * types may not support this due to implementation constraints.
     * 
     * The default implementation allows deprecation. Motion types that cannot
     * handle crossfading should override this to return false.
     * 
     * @return true if motion supports crossfade transitions, false otherwise
     */
    virtual bool canDeprecate();

    /**
     * @brief Sets a callback function to be called when motion deactivates.
     * 
     * Allows external code to be notified when a specific motion instance
     * stops playing. The callback receives the userdata pointer for context.
     * This is useful for cleanup or triggering follow-up actions.
     * 
     * @param cb Function pointer to call on deactivation
     * @param userdata Arbitrary data pointer passed to callback
     */
    void    setDeactivateCallback( void (*cb)(void *), void* userdata );

protected:
    /**
     * @brief Pure virtual activation callback implemented by subclasses.
     * 
     * Called when the motion is being activated by the motion controller.
     * This is where motions should perform activation-specific setup that
     * requires the motion to be in the active state. The motion has already
     * been successfully initialized at this point.
     * 
     * If this method returns false, the motion will be immediately deactivated
     * and removed from the active motion list.
     * 
     * @return true if activation succeeded, false to immediately deactivate
     */
    virtual bool onActivate() = 0;

    /**
     * @brief Adds a joint state to this motion's pose.
     * 
     * Registers a joint state that will be included in the motion's pose
     * and contribute to the final character skeleton. Joint states define
     * the position and rotation for specific joints.
     * 
     * @param jointState Smart pointer to the joint state to add
     */
    void addJointState(const LLPointer<LLJointState>& jointState);

protected:
    /**
     * @brief Current pose data containing joint transformations.
     * 
     * Contains the joint states that define how this motion affects the
     * character's skeleton. This is the primary output that gets blended
     * with other motions to produce the final character pose.
     */
    LLPose      mPose;
    
    /**
     * @brief Flag indicating if the motion has been stopped.
     * 
     * When true, the motion has received a stop request and is either
     * in its ease-out phase or has completed stopping. Used to track
     * motion lifecycle state.
     */
    bool        mStopped;
    
    /**
     * @brief Flag indicating if the motion is on the active motion list.
     * 
     * When true, the motion may be on the controller's active list and
     * receiving update calls. When false, the motion is guaranteed not
     * to be on the active list. This flag is managed by the motion controller.
     */
    bool        mActive;

    //-------------------------------------------------------------------------
    // Motion state data - set implicitly by the motion controller
    // May be referenced (read-only) by subclass callback handlers
    //-------------------------------------------------------------------------
    
    /**
     * @brief Human-readable name assigned by the motion controller.
     * 
     * Used for debugging and diagnostic purposes. The name may be derived
     * from the motion's UUID or set explicitly for easier identification
     * in logs and debug output.
     */
    std::string     mName;
    
    /**
     * @brief UUID that uniquely identifies this motion instance.
     * 
     * Set during construction and used throughout the motion's lifetime
     * for identification and lookup operations.
     */
    LLUUID          mID;

    /**
     * @brief Timestamp when the motion was activated.
     * 
     * Records the animation time when activate() was called. Used to
     * calculate the activeTime parameter passed to onUpdate().
     */
    F32 mActivationTimestamp;
    
    /**
     * @brief Timestamp when the motion was requested to stop.
     * 
     * Set when stopMotion() is called on the motion controller. The motion
     * may continue playing during its ease-out phase after this time.
     */
    F32 mStopTimestamp;
    
    /**
     * @brief Timestamp for network synchronization of motion stopping.
     * 
     * Used to coordinate motion stopping across the network in multiplayer
     * scenarios. Helps ensure consistent animation state between clients.
     */
    F32 mSendStopTimestamp;
    
    /**
     * @brief Blend weight at the start of the stop motion phase.
     * 
     * Captures the motion's influence level when stopping begins, used
     * for smooth blend-out calculations during the ease-out period.
     */
    F32 mResidualWeight;
    
    /**
     * @brief Current fade weight for level-of-detail adjustments.
     * 
     * Dynamically adjusted based on character pixel area and distance.
     * Allows distant characters to have reduced motion fidelity for
     * performance optimization.
     */
    F32 mFadeWeight;
    
    /**
     * @brief Joint signature tracking which joints are affected by priority.
     * 
     * Three-dimensional array tracking which joints this motion modifies
     * at what priority levels. Used by the motion controller to optimize
     * joint updates and resolve blending conflicts efficiently.
     * 
     * Dimensions: [3 signature types][LL_CHARACTER_MAX_ANIMATED_JOINTS]
     * - Multiple signature types support different update modes
     * - Each joint has a priority value indicating motion influence
     */
    U8  mJointSignature[3][LL_CHARACTER_MAX_ANIMATED_JOINTS];
    
    /**
     * @brief Function pointer for deactivation callback.
     * 
     * Optional callback function that is invoked when the motion is
     * deactivated. Allows external code to be notified of motion completion.
     */
    void (*mDeactivateCallback)(void* data);
    
    /**
     * @brief User data pointer passed to deactivation callback.
     * 
     * Arbitrary data pointer that is passed to the deactivation callback
     * function, allowing external code to maintain context across the
     * callback invocation.
     */
    void* mDeactivateCallbackUserData;
};


/**
 * @brief Debugging motion that logs all lifecycle events to the console.
 * 
 * LLTestMotion is a minimal motion implementation designed for debugging and
 * testing the motion system. It provides a concrete implementation of all
 * required LLMotion methods with simple logging output, making it useful for:
 * 
 * - Verifying motion lifecycle behavior during system development
 * - Testing motion registration and activation workflows
 * - Debugging motion controller state transitions
 * - Providing a template for new motion implementations
 * 
 * The motion performs no actual animation - it exists purely to exercise
 * the motion system infrastructure and provide diagnostic output.
 * 
 * Characteristics:
 * - Non-looping (plays once and completes immediately)
 * - Zero duration (completes instantly)
 * - No ease-in/ease-out transitions
 * - High priority for testing priority systems
 * - Normal blend mode (standard replacement blending)
 * - No pixel area requirements (always activates)
 * 
 * Usage: Typically registered during debug builds or testing scenarios
 * to verify that the motion system is functioning correctly.
 */
class LLTestMotion : public LLMotion
{
public:
    /**
     * @brief Constructs a test motion with debug logging.
     * 
     * @param id UUID for this test motion instance
     */
    LLTestMotion(const LLUUID &id) : LLMotion(id){}
    
    /**
     * @brief Destroys the test motion.
     */
    ~LLTestMotion() {}
    
    /**
     * @brief Factory function for creating test motion instances.
     * 
     * @param id UUID for the new motion instance
     * @return Pointer to newly created LLTestMotion
     */
    static LLMotion *create(const LLUUID& id) { return new LLTestMotion(id); }
    
    // LLMotion interface implementation with debug characteristics
    bool getLoop() { return false; }                                           ///< Non-looping for quick testing
    F32 getDuration() { return 0.0f; }                                        ///< Zero duration for instant completion
    F32 getEaseInDuration() { return 0.0f; }                                  ///< No ease-in transition
    F32 getEaseOutDuration() { return 0.0f; }                                 ///< No ease-out transition
    LLJoint::JointPriority getPriority() { return LLJoint::HIGH_PRIORITY; }   ///< High priority for testing
    LLMotionBlendType getBlendType() { return NORMAL_BLEND; }                 ///< Standard normal blending
    F32 getMinPixelArea() { return 0.f; }                                     ///< Always activates regardless of size

    /**
     * @brief Logs initialization event and succeeds immediately.
     * 
     * @param character Character being animated (unused for testing)
     * @return STATUS_SUCCESS always succeeds for testing
     */
    LLMotionInitStatus onInitialize(LLCharacter*) { LL_INFOS() << "LLTestMotion::onInitialize()" << LL_ENDL; return STATUS_SUCCESS; }
    
    /**
     * @brief Logs activation event and succeeds immediately.
     * 
     * @return true - always succeeds for testing
     */
    bool onActivate() { LL_INFOS() << "LLTestMotion::onActivate()" << LL_ENDL; return true; }
    
    /**
     * @brief Logs update events with timing information.
     * 
     * @param time Current animation time since activation
     * @param joint_mask Joint modification mask (unused for testing)
     * @return true - continues indefinitely until externally stopped
     */
    bool onUpdate(F32 time, U8* joint_mask) { LL_INFOS() << "LLTestMotion::onUpdate(" << time << ")" << LL_ENDL; return true; }
    
    /**
     * @brief Logs deactivation event for debugging.
     */
    void onDeactivate() { LL_INFOS() << "LLTestMotion::onDeactivate()" << LL_ENDL; }
};


/**
 * @brief Placeholder motion that performs no animation but maintains motion system state.
 * 
 * LLNullMotion is a "do-nothing" motion implementation that serves as a placeholder
 * in the motion system without affecting the character's pose. It's useful for:
 * 
 * - Occupying motion slots when no actual animation is desired
 * - Maintaining motion system state during transitions
 * - Providing a fallback when motion assets fail to load
 * - Testing motion lifecycle without visual changes
 * - Serving as a base template for minimal motion implementations
 * 
 * Unlike LLTestMotion which logs events, LLNullMotion operates silently and
 * consumes minimal resources while maintaining an active motion slot.
 * 
 * Characteristics:
 * - Looping (runs indefinitely until stopped)
 * - Fixed 1-second duration per loop cycle
 * - No ease-in/ease-out transitions (immediate activation/deactivation)
 * - High priority (to override conflicting motions when used as fallback)
 * - Normal blend mode (standard replacement blending)
 * - No pixel area requirements (always activates)
 * - Silent operation (no logging or visual effects)
 * 
 * Real-world Usage:
 * Often used as a fallback motion when animation assets are missing or corrupted,
 * allowing the motion system to continue functioning without errors while
 * providing no visible animation to the character.
 */
class LLNullMotion : public LLMotion
{
public:
    /**
     * @brief Constructs a null motion placeholder.
     * 
     * @param id UUID for this null motion instance
     */
    LLNullMotion(const LLUUID &id) : LLMotion(id) {}
    
    /**
     * @brief Destroys the null motion.
     */
    ~LLNullMotion() {}
    
    /**
     * @brief Factory function for creating null motion instances.
     * 
     * @param id UUID for the new motion instance
     * @return Pointer to newly created LLNullMotion
     */
    static LLMotion *create(const LLUUID &id) { return new LLNullMotion(id); }

    // LLMotion interface implementation for placeholder behavior

    /**
     * @brief Null motion loops continuously.
     * 
     * Looping allows the null motion to occupy its motion slot indefinitely
     * until explicitly replaced or stopped by the motion controller.
     * 
     * @return true - null motion loops until stopped
     */
    /*virtual*/ bool getLoop() { return true; }

    /**
     * @brief Null motion has a fixed 1-second duration per loop.
     * 
     * The 1-second duration provides a regular cycle for the motion system
     * to process, ensuring the null motion doesn't consume excessive CPU
     * while maintaining its active state.
     * 
     * @return 1.0 second duration per loop cycle
     */
    /*virtual*/ F32 getDuration() { return 1.f; }

    /**
     * @brief No ease-in transition for immediate activation.
     * 
     * @return 0.0 - instant activation
     */
    /*virtual*/ F32 getEaseInDuration() { return 0.f; }

    /**
     * @brief No ease-out transition for immediate deactivation.
     * 
     * @return 0.0 - instant deactivation
     */
    /*virtual*/ F32 getEaseOutDuration() { return 0.f; }

    /**
     * @brief High priority to override conflicting motions when used as fallback.
     * 
     * @return HIGH_PRIORITY for fallback motion scenarios
     */
    /*virtual*/ LLJoint::JointPriority getPriority() { return LLJoint::HIGH_PRIORITY; }

    /**
     * @brief Normal blending mode for standard motion replacement.
     * 
     * @return NORMAL_BLEND for typical motion slot occupation
     */
    /*virtual*/ LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

    /**
     * @brief No pixel area requirements - always activates.
     * 
     * @return 0.0 - null motion can activate on any size character
     */
    /*virtual*/ F32 getMinPixelArea() { return 0.f; }

    /**
     * @brief Always succeeds initialization immediately.
     * 
     * @param character Character being animated (unused by null motion)
     * @return STATUS_SUCCESS - null motion never fails initialization
     */
    /*virtual*/ LLMotionInitStatus onInitialize(LLCharacter *character) { return STATUS_SUCCESS; }

    /**
     * @brief Always succeeds activation immediately.
     * 
     * @return true - null motion never fails activation
     */
    /*virtual*/ bool onActivate() { return true; }

    /**
     * @brief Update does nothing but continues running.
     * 
     * The null motion performs no calculations or pose modifications during
     * updates, making it extremely lightweight while maintaining its active state.
     * 
     * @param activeTime Time since activation (unused)
     * @param joint_mask Joint modification mask (unused)
     * @return true - continues running indefinitely
     */
    /*virtual*/ bool onUpdate(F32 activeTime, U8* joint_mask) { return true; }

    /**
     * @brief Silent deactivation with no cleanup needed.
     */
    /*virtual*/ void onDeactivate() {}
};
#endif // LL_LLMOTION_H

