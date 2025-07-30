/**
 * @file llpose.h
 * @brief Implementation of LLPose class.
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

#ifndef LL_LLPOSE_H
#define LL_LLPOSE_H


#include "lljointstate.h"
#include "lljoint.h"
#include "llpointer.h"

#include <map>
#include <string>


/**
 * @brief Container for joint transformations that make up a character pose.
 * 
 * LLPose represents a complete character pose as a collection of joint states,
 * each containing position, rotation, and scale data for specific joints.
 * Poses are the fundamental building blocks of the animation system, combining
 * to create complex character movements through blending.
 * 
 * The pose system enables:
 * - Hierarchical joint transformation storage
 * - Weight-based pose blending for smooth transitions
 * - Efficient joint lookup by name or joint pointer
 * - Iteration over all joint states in the pose
 * 
 * Real-world Usage:
 * Poses are created by motions during their update cycles and processed by
 * the pose blender to produce final skeleton configurations:
 * 
 * @code
 * // Typical usage in motion update - motion creates/updates its pose
 * LLPose* pose = motion->getPose();
 * pose->setWeight(motion_weight);  // Set overall pose influence
 * 
 * // Joint state iteration pattern used in pose blending
 * for (LLJointState* jsp = pose->getFirstJointState(); 
 *      jsp; 
 *      jsp = pose->getNextJointState()) {
 *     // Process each joint transformation
 *     joint_blender->addJointState(jsp, priority, additive);
 * }
 * @endcode
 * 
 * Memory Management:
 * LLPose owns the joint states it contains through LLPointer references.
 * Joint states are automatically cleaned up when removed from the pose
 * or when the pose is destroyed.
 * 
 * Thread Safety: Not thread-safe, must be used from animation update thread.
 */
class LLPose
{
    friend class LLPoseBlender;
protected:
    /// Map type for storing joint states by joint name
    typedef std::map<std::string, LLPointer<LLJointState> > joint_map;
    /// Iterator type for joint state map
    typedef joint_map::iterator joint_map_iterator;
    /// Value type for joint state map entries
    typedef joint_map::value_type joint_map_value_type;

    /// Map storing all joint states in this pose, indexed by joint name
    joint_map                   mJointMap;
    /// Overall weight/influence of this pose in blending calculations
    F32                         mWeight;
    /// Iterator used for getFirstJointState/getNextJointState traversal
    joint_map_iterator          mListIter;
public:
    /**
     * @brief Begins iteration over joint states in this pose.
     * 
     * Starts sequential traversal of all joint states. Use with getNextJointState()
     * to iterate through all joints. This pattern is commonly used in pose
     * blending operations.
     * 
     * @return Pointer to first joint state, nullptr if pose is empty
     */
    LLJointState* getFirstJointState();
    
    /**
     * @brief Continues iteration over joint states in this pose.
     * 
     * Advances to the next joint state in the iteration sequence started by
     * getFirstJointState(). Returns nullptr when all joints have been visited.
     * 
     * @return Pointer to next joint state, nullptr when iteration complete
     */
    LLJointState* getNextJointState();
    
    /**
     * @brief Finds a joint state by joint pointer.
     * 
     * Searches for a joint state that affects the specified joint. More efficient
     * than name-based lookup when joint pointer is available.
     * 
     * @param joint Pointer to the joint to find state for
     * @return Pointer to joint state, nullptr if not found
     */
    LLJointState* findJointState(LLJoint *joint);
    
    /**
     * @brief Finds a joint state by joint name.
     * 
     * Searches for a joint state affecting the named joint. Uses string
     * comparison so joint pointer lookup is preferred when available.
     * 
     * @param name Name of the joint to find state for
     * @return Pointer to joint state, nullptr if not found
     */
    LLJointState* findJointState(const std::string &name);
public:
    /**
     * @brief Constructs an empty pose with zero weight.
     * 
     * Creates a new pose with no joint states and zero influence weight.
     * Joint states must be added separately before the pose is useful.
     */
    LLPose() : mWeight(0.f) {}
    
    /**
     * @brief Destroys the pose and releases all joint state references.
     * 
     * Automatically cleans up all contained joint states through smart pointers.
     * The pose should not be in active use when destroyed.
     */
    ~LLPose();
    
    /**
     * @brief Adds a joint state to this pose.
     * 
     * Incorporates a joint transformation into the pose. If a joint state for
     * the same joint already exists, it may be replaced depending on implementation.
     * The pose takes ownership of the joint state reference.
     * 
     * @param jointState Smart pointer to joint state to add
     * @return true if joint state was added successfully
     */
    bool addJointState(const LLPointer<LLJointState>& jointState);
    
    /**
     * @brief Removes a joint state from this pose.
     * 
     * Removes the specified joint state from the pose and releases the reference.
     * The joint will no longer be affected by this pose after removal.
     * 
     * @param jointState Smart pointer to joint state to remove
     * @return true if joint state was found and removed
     */
    bool removeJointState(const LLPointer<LLJointState>& jointState);
    
    /**
     * @brief Removes all joint states from this pose.
     * 
     * Clears the pose completely, removing all joint transformations and
     * releasing all joint state references. Results in an empty pose.
     * 
     * @return true if any joint states were removed
     */
    bool removeAllJointStates();
    
    /**
     * @brief Sets the overall weight for this pose and all its joint states.
     * 
     * Updates both the pose weight and the weight of all contained joint states.
     * This is the primary mechanism for controlling pose influence during blending.
     * Weight typically ranges from 0.0 (no influence) to 1.0 (full influence).
     * 
     * Real usage shows weights are calculated from motion fade states:
     * @code
     * // Common weight calculation patterns from motion controller
     * pose->setWeight(motion->getFadeWeight());
     * pose->setWeight(motion->getFadeWeight() * fade_factor);
     * pose->setWeight(0.f);  // Disable pose influence
     * @endcode
     * 
     * @param weight New weight value for pose and all joint states
     */
    void setWeight(F32 weight);
    
    /**
     * @brief Gets the current overall weight of this pose.
     * 
     * Returns the weight value that controls this pose's influence in blending
     * operations. Used by pose blenders to determine mixing ratios.
     * 
     * @return Current pose weight (typically 0.0 to 1.0)
     */
    F32 getWeight() const;
    
    /**
     * @brief Gets the number of joint states in this pose.
     * 
     * Returns the count of joints that have transformation data in this pose.
     * Used for validation and debugging of pose completeness.
     * 
     * @return Number of joint states currently stored
     */
    S32 getNumJointStates() const;
};

/// Maximum number of joint states that can be blended per joint simultaneously
const S32 JSB_NUM_JOINT_STATES = 6;

/**
 * @brief High-performance blender for multiple joint states affecting a single joint.
 * 
 * LLJointStateBlender combines up to JSB_NUM_JOINT_STATES (6) different joint states
 * that affect the same joint, handling priority-based blending and additive/normal
 * blend modes. This enables complex layering of animations on individual joints.
 * 
 * The blender uses 16-byte alignment for optimal SIMD performance during the
 * intensive mathematical operations required for real-time pose blending.
 * 
 * Blending Modes:
 * - Normal blending: Higher priority states replace lower priority ones
 * - Additive blending: States are mathematically added together
 * - Priority resolution: Same-priority states are averaged
 * 
 * Performance Optimization:
 * - Fixed-size arrays for predictable memory layout
 * - Cached joint for reduced memory allocations
 * - SIMD-aligned operations for vector math
 * 
 * Real-world Usage Pattern:
 * Joint state blenders are created on-demand by LLPoseBlender and reused
 * across frames to minimize allocation overhead:
 * 
 * @code
 * // Typical usage in pose blending
 * LLJointStateBlender* blender = getBlenderForJoint(joint);
 * blender->addJointState(joint_state, priority, is_additive);
 * blender->blendJointStates(true);  // Apply immediately to joint
 * @endcode
 */
LL_ALIGN_PREFIX(16)
class LLJointStateBlender
{
    LL_ALIGN_NEW
protected:
    /// Array of joint states to blend (up to JSB_NUM_JOINT_STATES)
    LLPointer<LLJointState> mJointStates[JSB_NUM_JOINT_STATES];
    /// Priority values for each joint state (higher = more important)
    S32             mPriorities[JSB_NUM_JOINT_STATES];
    /// Flags indicating if each joint state uses additive blending
    bool            mAdditiveBlends[JSB_NUM_JOINT_STATES];
public:
    /**
     * @brief Constructs a new joint state blender with empty state arrays.
     * 
     * Initializes all joint state slots to nullptr and prepares the blender
     * for receiving joint states. The cached joint is also initialized.
     */
    LLJointStateBlender();
    
    /**
     * @brief Destroys the blender and releases joint state references.
     * 
     * Cleans up all joint state references and cached joint data.
     * The blender should not be in active use when destroyed.
     */
    ~LLJointStateBlender();
    
    /**
     * @brief Performs the actual blending calculation and optionally applies result.
     * 
     * Combines all added joint states according to their priorities and blend modes,
     * producing a final transformation. If apply_now is true, immediately applies
     * the result to the target joint.
     * 
     * Blending algorithm:
     * 1. Sort states by priority (highest first)
     * 2. Apply normal blend states by priority (replacement)
     * 3. Add all additive blend states mathematically
     * 4. Normalize and apply final transformation
     * 
     * @param apply_now If true, immediately apply result to joint
     */
    void blendJointStates(bool apply_now = true);
    
    /**
     * @brief Adds a joint state to the blending operation.
     * 
     * Incorporates a joint state into the blend with specified priority and mode.
     * If all JSB_NUM_JOINT_STATES slots are full, lower priority states may be
     * displaced by higher priority ones.
     * 
     * @param joint_state Joint state to add to blend
     * @param priority Priority level (higher values take precedence)
     * @param additive_blend If true, use additive blending; if false, normal blending
     * @return true if joint state was added successfully
     */
    bool addJointState(const LLPointer<LLJointState>& joint_state, S32 priority, bool additive_blend);
    
    /**
     * @brief Interpolates between cached joint and current blend result.
     * 
     * Performs smooth interpolation between the previously cached joint state
     * and the current blending result. Used for temporal smoothing of rapid
     * animation changes.
     * 
     * @param u Interpolation factor (0.0 = cached state, 1.0 = current blend)
     */
    void interpolate(F32 u);
    
    /**
     * @brief Clears all joint states from the blender.
     * 
     * Removes all joint state references and resets the blender to empty state.
     * Prepares the blender for reuse in the next blending cycle.
     */
    void clear();
    
    /**
     * @brief Resets the cached joint to default state.
     * 
     * Clears cached joint transformation data, forcing recalculation on next
     * blend operation. Used when joint hierarchy changes or cache invalidation
     * is needed.
     */
    void resetCachedJoint();

public:
    /// Cached joint instance for reduced allocation overhead during blending
    LL_ALIGN_16(LLJoint mJointCache);
} LL_ALIGN_POSTFIX(16);

class LLMotion;

/**
 * @brief Master pose blending system that combines multiple motion poses into final skeleton state.
 * 
 * LLPoseBlender is the central component responsible for taking all active motion poses
 * and combining them into a final character pose. It manages per-joint blending operations,
 * handles priority resolution, and applies the final result to the character skeleton.
 * 
 * Architecture:
 * - Maintains a pool of LLJointStateBlender objects for reuse efficiency
 * - Processes motions sequentially, accumulating joint states per joint
 * - Handles both normal and additive blend modes
 * - Caches results for temporal smoothing
 * 
 * The blender is used by LLMotionController as the final stage of animation processing:
 * 
 * Real-world Integration:
 * @code
 * // Typical usage in motion controller update cycle
 * void LLMotionController::updateMotions() {
 *     // 1. Add all active motions to blender
 *     for (LLMotion* motion : mActiveMotions) {
 *         mPoseBlender.addMotion(motion);
 *     }
 *     
 *     // 2. Blend all poses and apply to skeleton
 *     mPoseBlender.blendAndApply();
 *     
 *     // 3. Clear for next frame
 *     mPoseBlender.clearBlenders();
 * }
 * @endcode
 * 
 * Performance Characteristics:
 * - O(n*m) complexity where n=motions, m=avg joints per motion
 * - Blender reuse reduces allocation overhead
 * - SIMD-optimized joint blending operations
 * - Selective updates based on joint signatures
 * 
 * The blender coordinates with joint signatures to skip unnecessary calculations
 * for joints that haven't changed, providing significant performance benefits
 * in complex animation scenarios.
 */
class LLPoseBlender
{
protected:
    /// List type for tracking active joint state blenders
    typedef std::list<LLJointStateBlender*> blender_list_t;
    /// Map type for joint-to-blender associations
    typedef std::map<LLJoint*,LLJointStateBlender*> blender_map_t;
    
    /// Pool of joint state blenders indexed by joint pointer for reuse
    blender_map_t mJointStateBlenderPool;
    /// List of blenders that are currently active and need processing
    blender_list_t mActiveBlenders;

    /// Slot counter for pose assignment (used internally)
    S32         mNextPoseSlot;
    /// Final blended pose result containing all joint transformations
    LLPose      mBlendedPose;
public:
    /**
     * @brief Constructs a new pose blender with empty state.
     * 
     * Initializes the blender with no active motions and prepares internal
     * data structures for pose processing.
     */
    LLPoseBlender();
    
    /**
     * @brief Destroys the pose blender and cleans up all blenders.
     * 
     * Releases all joint state blenders and clears the blender pool.
     * The blender should not be in active use when destroyed.
     */
    ~LLPoseBlender();

    /**
     * @brief Adds a motion's pose to the blending operation.
     * 
     * Incorporates all joint states from the motion's pose into the appropriate
     * joint state blenders. Creates new blenders as needed for joints that
     * don't already have blenders assigned.
     * 
     * This is the primary interface for feeding motion data into the pose
     * blending system. Called once per active motion during update cycles.
     * 
     * Processing:
     * 1. Iterate through all joint states in motion's pose
     * 2. Find or create joint state blender for each joint
     * 3. Add joint state to blender with appropriate priority and blend mode
     * 
     * @param motion Motion whose pose should be added to blending
     * @return true if motion was added successfully
     */
    bool addMotion(LLMotion* motion);

    /**
     * @brief Performs final pose blending and applies result to character skeleton.
     * 
     * Executes the complete blending pipeline:
     * 1. Process all active joint state blenders
     * 2. Blend joint states according to priority and mode
     * 3. Apply final transformations directly to character joints
     * 
     * This is the culmination of the animation system - where all motion
     * calculations are resolved into actual character pose changes.
     * 
     * Called once per frame after all motions have been added to the blender.
     * Performance critical: affects every visible character every frame.
     */
    void blendAndApply();

    /**
     * @brief Clears all joint state blenders for the next frame.
     * 
     * Removes all joint states from blenders and resets them to empty state.
     * Blenders remain in the pool for reuse to avoid allocation overhead.
     * 
     * Called after blendAndApply() to prepare for the next update cycle.
     */
    void clearBlenders();

    /**
     * @brief Performs blending and caches results without applying to skeleton.
     * 
     * Similar to blendAndApply() but stores results in cached joint states
     * instead of applying directly. Used for temporal smoothing operations
     * where interpolation between frames is needed.
     * 
     * @param reset_cached_joints If true, clears cached joint data before blending
     */
    void blendAndCache(bool reset_cached_joints);

    /**
     * @brief Interpolates all joints toward their cached values.
     * 
     * Performs smooth interpolation between current joint states and previously
     * cached states. Used for frame-rate independent animation smoothing.
     * 
     * @param u Interpolation factor (0.0 = current state, 1.0 = cached state)
     */
    void interpolate(F32 u);

    /**
     * @brief Gets the final blended pose containing all joint transformations.
     * 
     * Returns the pose that represents the complete blended result of all
     * active motions. Used for debugging and by systems that need to examine
     * the final pose state.
     * 
     * @return Pointer to the blended pose (owned by this blender)
     */
    LLPose* getBlendedPose() { return &mBlendedPose; }
};

#endif // LL_LLPOSE_H

