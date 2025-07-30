/**
 * @file llmotioncontroller.h
 * @brief Implementation of LLMotionController class.
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

#ifndef LL_LLMOTIONCONTROLLER_H
#define LL_LLMOTIONCONTROLLER_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include <string>
#include <map>
#include <deque>

#include "llmotion.h"
#include "llpose.h"
#include "llframetimer.h"
#include "llstatemachine.h"
#include "llstring.h"

//-----------------------------------------------------------------------------
// Class predeclaration
// This is necessary because llcharacter.h includes this file.
//-----------------------------------------------------------------------------
class LLCharacter;

/**
 * @brief Function pointer type for motion constructor callbacks.
 * 
 * Motion constructors are factory functions that create new instances of
 * specific motion types. Each motion class must provide a static constructor
 * function that matches this signature to be registered with the motion system.
 * 
 * @param id UUID identifying the specific motion instance to create
 * @return Pointer to newly created motion instance, nullptr on failure
 */
typedef LLMotion*(*LLMotionConstructor)(const LLUUID &id);

/**
 * @brief Factory registry for creating motion instances by UUID.
 * 
 * LLMotionRegistry serves as a factory system that maps motion UUIDs to their
 * corresponding constructor functions. This allows the motion controller to
 * dynamically create motion instances without compile-time knowledge of all
 * available motion types.
 * 
 * The registry supports:
 * - Registration of motion constructors by UUID
 * - Dynamic creation of motion instances
 * - Blacklisting of failed motion types to prevent retry loops
 * 
 * This design enables a plugin-like architecture where new motion types can
 * be added to the system simply by registering their constructor functions.
 * 
 * Actual Registration Pattern in LLVOAvatar::initInstance():
 * All motion types are registered once during the first avatar initialization
 * to populate the global registry shared by all characters:
 * 
 * @code
 * // During avatar initialization when first character is created
 * if (LLCharacter::sInstances.size() == 1) {
 *     // Basic locomotion and poses
 *     registerMotion(ANIM_AGENT_WALK,         LLKeyframeWalkMotion::create);
 *     registerMotion(ANIM_AGENT_RUN,          LLKeyframeWalkMotion::create);
 *     registerMotion(ANIM_AGENT_STAND,        LLKeyframeStandMotion::create);
 *     registerMotion(ANIM_AGENT_CROUCH,       LLKeyframeStandMotion::create);
 *     
 *     // Facial expressions (all use same LLEmote class)
 *     registerMotion(ANIM_AGENT_EXPRESS_SAD,  LLEmote::create);
 *     registerMotion(ANIM_AGENT_EXPRESS_SMILE, LLEmote::create);
 *     // ... 12 more expressions
 *     
 *     // Always-active procedural motions  
 *     registerMotion(ANIM_AGENT_BODY_NOISE,   LLBodyNoiseMotion::create);
 *     registerMotion(ANIM_AGENT_BREATHE_ROT,  LLBreatheMotionRot::create);
 *     registerMotion(ANIM_AGENT_HEAD_ROT,     LLHeadRotMotion::create);
 *     registerMotion(ANIM_AGENT_EYE,          LLEyeMotion::create);
 *     registerMotion(ANIM_AGENT_TARGET,       LLTargetingMotion::create);
 *     
 *     // Utility and special cases
 *     registerMotion(ANIM_AGENT_DO_NOT_DISTURB, LLNullMotion::create);
 * }
 * @endcode
 * 
 * Thread safety: This class is not thread-safe and should only be accessed
 * from the main rendering thread where character updates occur.
 */
class LLMotionRegistry
{
public:
    /**
     * @brief Constructs an empty motion registry.
     * 
     * Initializes the registry with no registered motion types. Motion
     * constructors must be explicitly registered before they can be used.
     */
    LLMotionRegistry();

    /**
     * @brief Destroys the registry and cleans up resources.
     * 
     * Does not destroy any motion instances that may have been created
     * through this registry. The motion controller is responsible for
     * managing the lifetime of individual motion instances.
     */
    ~LLMotionRegistry();

    /**
     * @brief Registers a motion constructor function for a specific UUID.
     * 
     * Associates a motion UUID with a constructor function that can create
     * instances of that motion type. If a motion with this UUID is already
     * registered, the old constructor is replaced with the new one.
     * 
     * The constructor function must create a new motion instance and return
     * a valid pointer, or return nullptr if creation fails.
     * 
     * @param id UUID that uniquely identifies this motion type
     * @param create Function pointer to motion constructor
     * @return true if registration succeeded, false if parameters invalid
     */
    bool registerMotion( const LLUUID& id, LLMotionConstructor create);

    /**
     * @brief Creates a new instance of a registered motion type.
     * 
     * Looks up the constructor function for the specified UUID and calls it
     * to create a new motion instance. If the motion type is not registered
     * or has been marked as bad, returns nullptr.
     * 
     * The caller takes ownership of the returned motion instance and is
     * responsible for its destruction (typically handled by the motion controller).
     * 
     * @param id UUID of the motion type to create
     * @return Pointer to new motion instance, nullptr if creation failed
     * @warning The returned pointer must be managed carefully to avoid memory leaks
     */
    LLMotion *createMotion( const LLUUID &id );

    /**
     * @brief Marks a motion type as permanently failed.
     * 
     * Blacklists a motion UUID to prevent future creation attempts. This is
     * typically called when a motion's initialization fails, indicating that
     * the motion data is corrupted or incompatible. Subsequent calls to
     * createMotion() for this UUID will return nullptr immediately.
     * 
     * This prevents infinite retry loops when loading broken animation data
     * and improves performance by avoiding repeated failure attempts.
     * 
     * @param id UUID of the motion type to blacklist
     */
    void markBad( const LLUUID& id );


protected:
    /**
     * @brief Map type for storing motion constructor associations.
     * 
     * Associates motion UUIDs with their corresponding constructor functions.
     * Uses std::map for O(log n) lookup performance during motion creation.
     */
    typedef std::map<LLUUID, LLMotionConstructor> motion_map_t;
    
    /**
     * @brief Registry table mapping motion UUIDs to constructor functions.
     * 
     * Contains all registered motion types that can be dynamically created.
     * Entries are added via registerMotion() and used by createMotion().
     */
    motion_map_t mMotionTable;
};

/**
 * @brief Central animation coordinator that manages all motion playback for a character.
 * 
 * LLMotionController serves as the heart of the Second Life animation system,
 * orchestrating the lifecycle and blending of all character motions. It manages:
 * - Motion creation through the factory registry system
 * - Animation lifecycle (loading -> loaded -> active -> deprecated)
 * - Motion blending and priority resolution
 * - Performance optimization through minimal update modes
 * - Joint signature management for efficient updates
 * 
 * The controller maintains several motion collections representing different states:
 * - mAllMotions: Complete registry of all motion instances (persistent)
 * - mLoadingMotions: Motions currently fetching asset data from servers
 * - mLoadedMotions: Motions ready for activation (initialized)
 * - mActiveMotions: Currently playing motions (affecting skeleton)
 * - mDeprecatedMotions: Motions being phased out during crossfading
 * 
 * Motion Lifecycle in Practice:
 * @code
 * // 1. Motion creation and loading
 * LLMotion* motion = controller.createMotion(ANIM_AGENT_WALK);
 * // -> Added to mAllMotions and mLoadingMotions
 * 
 * // 2. Asset loading completes
 * updateLoadingMotions();
 * // -> Motion moves from mLoadingMotions to mLoadedMotions
 * 
 * // 3. Motion activation
 * controller.startMotion(ANIM_AGENT_WALK, 0.0f);
 * // -> Motion moves to mActiveMotions, begins affecting skeleton
 * 
 * // 4. Motion termination
 * controller.stopMotionLocally(ANIM_AGENT_WALK, false);
 * // -> Motion may move to mDeprecatedMotions for blend-out
 * @endcode
 * 
 * Performance Optimization:
 * The controller supports multiple update modes for performance scaling:
 * - updateMotions(false): Full processing for visible avatars
 * - updateMotionsMinimal(): Reduced processing for hidden/distant avatars
 * 
 * Joint Signature System:
 * Tracks which joints are modified by which motions at what priority levels,
 * enabling efficient selective updates and avoiding unnecessary calculations.
 * 
 * Real-world Usage Patterns:
 * Each LLCharacter instance contains exactly one motion controller, initialized
 * via mMotionController.setCharacter(this) in the character constructor. The
 * controller is driven by LLCharacter::updateMotions() called from LLVOAvatar::updateCharacter().
 * 
 * Critical Performance Optimizations:
 * - Maximum 32 loaded motion instances (MAX_MOTION_INSTANCES) with aggressive purging
 * - Three-tier update system: NORMAL_UPDATE/HIDDEN_UPDATE/None based on visibility
 * - Joint signature optimization to avoid redundant calculations
 * - Self-avatar special handling (mIsSelf = true) for enhanced responsiveness
 * 
 * Motion Instance Lifecycle Management:
 * The controller implements aggressive cleanup with purgeExcessMotions() called
 * every frame to prevent memory bloat. When exceeding 32 loaded motions, deprecated
 * motions are purged first, followed by inactive motions. Systems log warnings
 * when motion counts exceed 64 instances.
 * 
 * Real Update Flow in LLMotionController::updateMotions():
 * @code
 * updateMotions(force_update) {
 *     purgeExcessMotions();              // Prevent memory bloat
 *     updateLoadingMotions();            // Move loading -> loaded
 *     resetJointSignatures();            // Clear joint usage tracking
 *     if (paused && !force_update) {
 *         updateIdleActiveMotions();     // Minimal updates when paused
 *     } else {
 *         updateAdditiveMotions();       // Process ADDITIVE_BLEND motions
 *         resetJointSignatures();
 *         updateRegularMotions();        // Process NORMAL_BLEND motions
 *         mPoseBlender.blendAndApply();  // Apply final pose
 *     }
 * }
 * @endcode
 * 
 * Thread safety: This class is not thread-safe and must only be used from
 * the main rendering thread where character updates occur.
 */
class LLMotionController
{
public:
    /**
     * @brief Type alias for lists of motion pointers.
     * 
     * Used for ordered collections where motion sequence matters,
     * such as the active motion list for blending calculations.
     */
    typedef std::list<LLMotion*> motion_list_t;
    
    /**
     * @brief Type alias for sets of motion pointers.
     * 
     * Used for unordered collections where uniqueness is important,
     * such as loading motions and deprecated motions.
     */
    typedef std::set<LLMotion*> motion_set_t;
    
    /**
     * @brief Flag indicating if this controller belongs to the user's own avatar.
     * 
     * Self avatars receive special treatment in the animation system:
     * - Never use HIDDEN_UPDATE mode (always maintain full fidelity)
     * - May have different priority rules for certain motions
     * - Receive more frequent updates for responsive control
     * 
     * This optimization ensures the user's avatar always appears responsive
     * while allowing distant avatars to use reduced processing modes.
     */
    bool mIsSelf;

public:
    /**
     * @brief Constructs a new motion controller.
     * 
     * Initializes the controller with default settings and empty motion
     * collections. The character association must be set separately via
     * setCharacter() before the controller can be used.
     */
    LLMotionController();

    /**
     * @brief Destroys the motion controller and cleans up all resources.
     * 
     * Releases all motion instances and cleans up internal data structures.
     * The destructor ensures no motion instances remain allocated when the
     * controller is destroyed.
     */
    virtual ~LLMotionController();

    /**
     * @brief Associates this controller with a character instance.
     * 
     * Establishes the bidirectional relationship between the motion controller
     * and its containing character. This must be called exactly once during
     * character initialization before any motions can be processed.
     * 
     * The character pointer is used to query character state (position, joints,
     * pixel area) during motion updates and blending calculations.
     * 
     * @param character Pointer to the character that owns this controller
     * @warning Must be called exactly once, typically in character constructor
     */
    void setCharacter( LLCharacter *character );

    /**
     * @brief Registers a motion constructor with the global motion registry.
     * 
     * Forwards the registration call to the static motion registry shared by
     * all motion controllers. This allows motion types to be registered once
     * and used by any character that needs them.
     * 
     * @param id UUID that uniquely identifies the motion type
     * @param create Constructor function that creates motion instances
     * @return true if registration succeeded, false if parameters invalid
     */
    bool registerMotion( const LLUUID& id, LLMotionConstructor create );

    /**
     * @brief Creates or retrieves a motion instance for the specified UUID.
     * 
     * Checks if a motion instance already exists in mAllMotions for this UUID.
     * If not found, creates a new instance using the registered constructor
     * and adds it to the appropriate motion collections based on its state.
     * 
     * New motions typically start in the loading state if they require asset
     * data, or move directly to loaded if they're procedural animations.
     * 
     * @param id UUID of the motion to create or retrieve
     * @return Pointer to the motion instance, nullptr if creation failed
     * @warning Caller must not delete the returned pointer - managed by controller
     */
    LLMotion *createMotion( const LLUUID &id );

    /**
     * @brief Removes a motion type from the global motion registry.
     * 
     * Forwards the removal call to the static motion registry, preventing
     * future creation of this motion type. Does not affect existing motion
     * instances that have already been created.
     * 
     * @param id UUID of the motion type to remove from registry
     */
    void removeMotion( const LLUUID& id );

    /**
     * @brief Starts playing a motion animation.
     * 
     * Creates the motion instance if necessary, initializes it, and adds it
     * to the active motion list for blending. The motion will begin its
     * ease-in phase and gradually blend into the character's pose.
     * 
     * If the motion is already playing, this may restart it from the specified
     * offset or begin a crossfade transition depending on the motion's settings.
     * 
     * @param id UUID of the motion to start
     * @param start_offset Time offset in seconds to begin playback from
     * @return true if motion started successfully, false if error occurred
     */
    bool startMotion( const LLUUID &id, F32 start_offset );

    /**
     * @brief Initiates the stop sequence for a playing motion.
     * 
     * Begins the motion's termination process. Unless stop_immediate is true,
     * the motion enters its ease-out phase and gradually blends out of the
     * character's pose before being fully deactivated.
     * 
     * The "Locally" suffix indicates this only affects the local character
     * and does not send network messages to other clients.
     * 
     * @param id UUID of the motion to stop
     * @param stop_immediate If true, stops immediately; if false, blends out
     * @return true if stop was initiated successfully, false otherwise
     */
    bool stopMotionLocally( const LLUUID &id, bool stop_immediate );

    /**
     * @brief Processes motions that are currently loading asset data.
     * 
     * Checks all motions in the loading state to see if their required asset
     * data has finished downloading. Motions that have completed loading are
     * moved to the loaded state and become available for activation.
     * 
     * This method is called regularly during the update cycle to ensure
     * responsive activation of motions once their data becomes available.
     */
    void updateLoadingMotions();

    /**
     * @brief Primary update method that advances all motion processing.
     * 
     * This is the core animation processing method that:
     * - Updates all active motions (calls LLMotion::onUpdate())
     * - Processes motion state transitions (loading -> loaded -> active)
     * - Applies joint transformations through the pose blender
     * - Handles motion crossfading and blending
     * - Updates joint signatures for selective processing
     * 
     * The force_update parameter bypasses certain optimizations and ensures
     * full processing even for hidden or distant characters. This is used
     * for animation previews where maximum fidelity is required.
     * 
     * Performance note: This is called every frame for every visible character
     * and is extensively optimized and profiled.
     * 
     * @param force_update If true, forces full update regardless of optimizations
     */
    void updateMotions(bool force_update = false);

    /**
     * @brief Performs minimal motion updates for hidden or distant characters.
     * 
     * Provides a lightweight update path that maintains motion state without
     * performing expensive calculations. Used when characters are too far away
     * or occluded to justify full animation processing.
     * 
     * This optimization can significantly improve performance in crowded areas
     * by reducing the computational cost of non-visible avatars.
     */
    void updateMotionsMinimal();

    /**
     * @brief Clears all pose blenders and resets blending state.
     * 
     * Removes all accumulated joint transformations from the pose blending
     * system. This effectively resets the character to its base pose without
     * any motion influences.
     */
    void clearBlenders() { mPoseBlender.clearBlenders(); }

    /**
     * @brief Destroys all motion instances and clears all motion collections.
     * 
     * Completely removes all motions from the controller, including those in
     * loading, loaded, active, and deprecated states. This is more thorough
     * than deactivateAllMotions() as it actually destroys the motion objects.
     * 
     * Typically used when rebuilding a character's skeleton, as it ensures
     * no stale joint references remain in motion instances.
     */
    void flushAllMotions();

    /**
     * @brief Deactivates all currently active motions without destroying them.
     * 
     * Stops all playing animations but preserves the motion instances in
     * memory for potential reuse. This is less destructive than flushAllMotions()
     * and is suitable for temporarily stopping all animation playback.
     * 
     * Comment note: "Flush is a liar" suggests this method doesn't actually
     * flush (destroy) motions despite its name - it only deactivates them.
     */
    void deactivateAllMotions();

    /**
     * @brief Pauses all motion processing.
     * 
     * Stops advancing motion time and blocks all motion updates. Motions
     * remain in their current state and will resume from the same point
     * when unpaused. Useful for debugging or pause menu functionality.
     */
    void pauseAllMotions();
    
    /**
     * @brief Resumes motion processing after a pause.
     * 
     * Restores normal motion time advancement and update processing.
     * Motions continue from their state at the time of pausing.
     */
    void unpauseAllMotions();
    
    /**
     * @brief Checks if motion processing is currently paused.
     * 
     * @return true if motions are paused, false if running normally
     */
    bool isPaused() const { return mPaused; }
    
    /**
     * @brief Gets the frame number when motions were paused.
     * 
     * @return Frame count at time of pause, useful for debugging
     */
    S32 getPausedFrame() const { return mPausedFrame; }

    /**
     * @brief Sets the fixed time step for motion updates.
     * 
     * Forces motions to advance by a fixed time increment rather than
     * using variable frame times. Useful for deterministic animation
     * playback or slow-motion effects.
     * 
     * @param step Fixed time step in seconds per update
     */
    void setTimeStep(F32 step);
    
    /**
     * @brief Gets the current fixed time step setting.
     * 
     * @return Fixed time step in seconds, 0.0 if using variable timing
     */
    F32 getTimeStep() const { return mTimeStep; }

    /**
     * @brief Sets the time scaling factor for all motions.
     * 
     * Multiplies the time advancement rate for all animations. Values less
     * than 1.0 create slow-motion effects, while values greater than 1.0
     * speed up animations. Useful for debugging or special effects.
     * 
     * @param time_factor Time scaling multiplier (1.0 = normal speed)
     */
    void setTimeFactor(F32 time_factor);
    
    /**
     * @brief Gets the current time scaling factor.
     * 
     * @return Current time factor (1.0 = normal speed)
     */
    F32 getTimeFactor() const { return mTimeFactor; }

    /**
     * @brief Gets the current animation time.
     * 
     * Returns the accumulated animation time used for motion calculations.
     * This time is affected by time factors and pause states.
     * 
     * @return Current animation time in seconds
     */
    F32 getAnimTime() const { return mAnimTime; }

    /**
     * @brief Gets direct access to the active motion list.
     * 
     * Provides access to the list of currently playing motions for external
     * processing or debugging. Use with caution as modifying this list
     * directly can disrupt motion controller state.
     * 
     * @return Reference to the active motion list
     */
    motion_list_t& getActiveMotions() { return mActiveMotions; }

    /**
     * @brief Counts motions in each state category for statistics.
     * 
     * Tallies the number of motions in each lifecycle state and updates
     * the provided counter variables. Used for performance monitoring
     * and debugging to understand motion system load.
     * 
     * @param num_motions Total number of motion instances
     * @param num_loading_motions Number of motions loading asset data
     * @param num_loaded_motions Number of motions ready for activation
     * @param num_active_motions Number of currently playing motions
     * @param num_deprecated_motions Number of motions being phased out
     */
    void incMotionCounts(S32& num_motions, S32& num_loading_motions, S32& num_loaded_motions, S32& num_active_motions, S32& num_deprecated_motions);

//protected:
    bool isMotionActive( LLMotion *motion );
    bool isMotionLoading( LLMotion *motion );
    LLMotion *findMotion( const LLUUID& id ) const;

    void dumpMotions();

    const LLFrameTimer& getFrameTimer() { return mTimer; }

    static F32  getCurrentTimeFactor()              { return sCurrentTimeFactor;    };
    static void setCurrentTimeFactor(F32 factor)    { sCurrentTimeFactor = factor;  };

protected:
    // internal operations act on motion instances directly
    // as there can be duplicate motions per id during blending overlap
    void deleteAllMotions();
    bool activateMotionInstance(LLMotion *motion, F32 time);
    bool deactivateMotionInstance(LLMotion *motion);
    void deprecateMotionInstance(LLMotion* motion);
    bool stopMotionInstance(LLMotion *motion, bool stop_imemdiate);
    void removeMotionInstance(LLMotion* motion);
    void updateRegularMotions();
    void updateAdditiveMotions();
    void resetJointSignatures();
    void updateMotionsByType(LLMotion::LLMotionBlendType motion_type);
    void updateIdleMotion(LLMotion* motionp);
    void updateIdleActiveMotions();
    void purgeExcessMotions();
    void deactivateStoppedMotions();

protected:
    /**
     * @brief Time scaling factor for this controller's animations.
     * 
     * Multiplies the animation advancement rate. 1.0 represents normal speed,
     * values less than 1.0 slow down animations, greater than 1.0 speed them up.
     */
    F32                 mTimeFactor;
    
    /**
     * @brief Global default time factor for new controllers.
     * 
     * Static value used to initialize new motion controllers. Allows global
     * time scaling to be applied to all characters simultaneously.
     */
    static F32          sCurrentTimeFactor;
    
    /**
     * @brief Global motion registry shared by all controllers.
     * 
     * Static registry that maps motion UUIDs to constructor functions.
     * Shared across all motion controllers to avoid duplicating motion
     * type registrations for each character.
     */
    static LLMotionRegistry sRegistry;
    
    /**
     * @brief Pose blending system that combines motion transformations.
     * 
     * Accumulates joint transformations from all active motions and applies
     * priority-based blending to produce the final skeleton pose. Handles
     * both normal and additive motion blending modes.
     */
    LLPoseBlender       mPoseBlender;

    /**
     * @brief Pointer to the character that owns this motion controller.
     * 
     * Used to query character state during motion updates, including position,
     * joint hierarchy, pixel area for LOD calculations, and ground collision.
     * Set once during initialization via setCharacter().
     */
    LLCharacter         *mCharacter;

    /**
     * @brief Motion lifecycle documentation.
     * 
     * Animation lifecycle states:
     * 1. Creation: Motions are instantiated and added to mAllMotions for their entire lifetime
     * 2. Loading: If motions require asset data, they're added to mLoadingMotions while fetching
     * 3. Loaded: Once initialization completes, motions move to mLoadedMotions (ready for activation)
     * 4. Active: Playing motions are added to mActiveMotions and affect the skeleton
     * 5. Deprecated: Motions being phased out during crossfading remain in mDeprecatedMotions
     * 
     * A motion can exist in multiple collections simultaneously (e.g., mAllMotions + mActiveMotions).
     */

    /**
     * @brief Map type for storing motion instances by UUID.
     * 
     * Associates motion UUIDs with their corresponding motion instances.
     * Uses std::map for O(log n) lookup performance during motion operations.
     */
    typedef std::map<LLUUID, LLMotion*> motion_map_t;
    
    /**
     * @brief Complete registry of all motion instances for this character.
     * 
     * Contains every motion instance that has been created for this character,
     * regardless of current state. Motions remain in this map for their entire
     * lifetime to enable efficient lookup and prevent duplicate creation.
     */
    motion_map_t    mAllMotions;

    /**
     * @brief Set of motions currently loading asset data from servers.
     * 
     * Contains motions that are waiting for animation asset data to complete
     * downloading. These motions cannot be activated until loading finishes
     * and they move to the loaded state.
     */
    motion_set_t        mLoadingMotions;
    
    /**
     * @brief Set of motions that are initialized and ready for activation.
     * 
     * Contains motions that have completed loading and initialization but
     * are not currently playing. These motions can be activated immediately
     * when requested.
     */
    motion_set_t        mLoadedMotions;
    
    /**
     * @brief Ordered list of currently playing motions.
     * 
     * Contains motions that are actively affecting the character's skeleton.
     * Order matters for blending calculations - motions are processed in
     * the sequence they appear in this list.
     */
    motion_list_t       mActiveMotions;
    
    /**
     * @brief Set of motions being phased out during crossfade transitions.
     * 
     * Contains motions that are in their ease-out phase when being replaced
     * by new motions. These motions still contribute to the final pose but
     * with decreasing influence until they fully fade out.
     */
    motion_set_t        mDeprecatedMotions;

    /**
     * @brief High-resolution timer for animation time calculations.
     * 
     * Provides precise timing for motion updates and frame-rate independent
     * animation playback. Used to calculate elapsed time between updates.
     */
    LLFrameTimer        mTimer;
    
    /**
     * @brief Previous frame's elapsed timer value.
     * 
     * Cached value from the last update cycle, used to calculate frame delta
     * time for smooth animation advancement regardless of frame rate variations.
     */
    F32                 mPrevTimerElapsed;
    
    /**
     * @brief Current accumulated animation time.
     * 
     * Master animation clock that advances based on elapsed real time,
     * modified by time factors and pause states. Used as the time base
     * for all motion calculations.
     */
    F32                 mAnimTime;
    
    /**
     * @brief Previous update's animation time value.
     * 
     * Cached time from the last update, used to detect time changes and
     * calculate animation deltas for motion processing.
     */
    F32                 mLastTime;
    
    /**
     * @brief Flag indicating if the controller has completed at least one update.
     * 
     * Prevents certain initialization operations from running on every frame.
     * Set to true after the first successful update cycle completes.
     */
    bool                mHasRunOnce;
    
    /**
     * @brief Flag indicating if motion processing is currently paused.
     * 
     * When true, motion time advancement stops and update processing is
     * blocked. Motions remain in their current state until unpaused.
     */
    bool                mPaused;
    
    /**
     * @brief Frame number when motions were paused.
     * 
     * Records the frame count at the time of pausing for debugging and
     * diagnostic purposes. Helps track how long motions have been paused.
     */
    S32                 mPausedFrame;
    
    /**
     * @brief Fixed time step for deterministic animation playback.
     * 
     * When non-zero, forces animation time to advance by this fixed amount
     * each update instead of using variable frame times. Useful for
     * debugging and ensuring reproducible animation behavior.
     */
    F32                 mTimeStep;
    
    /**
     * @brief Counter tracking number of fixed time step updates.
     * 
     * Increments each time the fixed time step is applied, used for
     * statistical tracking and debugging time step behavior.
     */
    S32                 mTimeStepCount;
    
    /**
     * @brief Last interpolation value used in motion blending.
     * 
     * Cached interpolation factor from the previous update, used for
     * smooth motion transitions and debugging blending calculations.
     */
    F32                 mLastInterp;

    /**
     * @brief Joint signature tracking system for selective updates.
     * 
     * Two-dimensional array tracking which joints are affected by which motions
     * at what priority levels. The first dimension represents different signature
     * sets (current vs. previous), while the second dimension covers all possible
     * animated joints in the character skeleton.
     * 
     * This system enables efficient selective updates by identifying which joints
     * actually need processing, avoiding expensive calculations for unmodified joints.
     * 
     * Array dimensions: [2][LL_CHARACTER_MAX_ANIMATED_JOINTS]
     */
    U8                  mJointSignature[2][LL_CHARACTER_MAX_ANIMATED_JOINTS];
    
private:
    /**
     * @brief Motion count after last purge operation.
     * 
     * Tracks the number of motions remaining after the last purge cycle.
     * Used for logging and debugging to monitor motion memory usage and
     * cleanup effectiveness. Helps detect motion leaks or excessive accumulation.
     */
    U32                 mLastCountAfterPurge;
};

//-----------------------------------------------------------------------------
// Class declaractions
//-----------------------------------------------------------------------------
#include "llcharacter.h"

#endif // LL_LLMOTIONCONTROLLER_H

