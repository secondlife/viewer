/**
 * @file llcharacter.h
 * @brief Implementation of LLCharacter class.
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

#ifndef LL_LLCHARACTER_H
#define LL_LLCHARACTER_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include <string>

#include "lljoint.h"
#include "llmotioncontroller.h"
#include "llvisualparam.h"
#include "llstringtable.h"
#include "llpointer.h"
#include "llrefcount.h"

class LLPolyMesh;

/**
 * @brief Reference-counted handle for animation pause requests.
 * 
 * This class provides a thread-safe mechanism for managing animation pause
 * requests. When all references to a pause request handle are released,
 * the animations automatically resume. This allows multiple systems to
 * independently request animation pausing without interfering with each other.
 * 
 * The handle uses reference counting to track when pause requests are no
 * longer needed, enabling automatic cleanup and animation resumption.
 */
class LLPauseRequestHandle : public LLThreadSafeRefCount
{
public:
    LLPauseRequestHandle() {};
};

typedef LLPointer<LLPauseRequestHandle> LLAnimPauseRequest;

/**
 * @brief Base class for all animated characters in Second Life.
 * 
 * LLCharacter serves as the foundation for avatars, NPCs, and other animated
 * entities in the virtual world. It provides a unified interface for:
 * - Skeletal animation and motion control
 * - Visual parameter management (shape, appearance customization)
 * - Coordinate transformations between local and global space
 * - Ground collision detection and physics integration
 * 
 * This is an abstract base class that defines the essential interface all
 * character implementations must provide. Concrete subclasses like LLVOAvatar
 * implement the pure virtual methods to provide specific functionality for
 * different character types.
 * 
 * The class manages two key systems:
 * 1. Motion Controller - Handles skeletal animations, blending, and playback
 * 2. Visual Parameters - Controls morphable appearance attributes (body shape, facial features)
 * 
 * Performance note: Character updates are on the critical rendering path and are
 * extensively profiled (LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR). The motion system
 * uses three update modes: NORMAL_UPDATE for visible characters, HIDDEN_UPDATE for
 * performance optimization of distant/occluded avatars, and FORCE_UPDATE for
 * animation previews requiring maximum fidelity.
 * 
 * Real-world usage: The inheritance chain is:
 * - LLCharacter (abstract base) -> LLAvatarAppearance (abstract appearance system)
 * - -> LLVOAvatar (concrete viewer implementation) -> LLVOAvatarSelf (user's avatar)
 * 
 * Avatar updates are driven by LLVOAvatar::updateCharacter() which calls updateMotions().
 * See LLVOAvatar and LLVOAvatarSelf for concrete implementations of this interface.
 */
class LLCharacter
{
public:
    /**
     * @brief Constructs a new character instance.
     * 
     * Initializes the motion controller, visual parameter system, and sets
     * default values for appearance properties. Creates a pause request handle
     * for animation control.
     */
    LLCharacter();

    /**
     * @brief Destroys the character and cleans up resources.
     * 
     * Releases all visual parameters and ensures proper cleanup of the
     * motion controller. Subclasses should call parent destructor to
     * ensure complete resource deallocation.
     */
    virtual ~LLCharacter();

    //-------------------------------------------------------------------------
    // LLCharacter Interface
    // These functions must be implemented by subclasses.
    //-------------------------------------------------------------------------

    /**
     * @brief Gets the file path prefix for loading animation data.
     * 
     * Returns the directory prefix used to locate animation files in the
     * viewer's data directory structure. Different character types may use
     * different animation sets (e.g., "avatar_" vs "npc_").
     * 
     * @return Null-terminated string containing the animation file prefix
     */
    virtual const char *getAnimationPrefix() = 0;

    /**
     * @brief Gets the root joint of the character's skeleton.
     * 
     * Returns the top-level joint that serves as the parent for the entire
     * skeletal hierarchy. All other joints in the character are descendants
     * of this root joint. Used for skeleton traversal and coordinate transforms.
     * 
     * @return Pointer to the root joint, or nullptr if skeleton not initialized
     */
    virtual LLJoint *getRootJoint() = 0;

    /**
     * @brief Finds a joint by name in the character's skeleton.
     * 
     * Searches the skeletal hierarchy for a joint with the specified name.
     * The default implementation performs a recursive search starting from
     * the root joint. Subclasses may override this to provide optimized
     * lookups using caching or direct indexing.
     * 
     * @param name Name of the joint to find (case-sensitive)
     * @return Pointer to the joint if found, nullptr otherwise
     */
    virtual LLJoint* getJoint(std::string_view name);

    /**
     * @brief Gets the character's current world position.
     * 
     * @return 3D position vector in world coordinates
     */
    virtual LLVector3 getCharacterPosition() = 0;

    /**
     * @brief Gets the character's current world rotation.
     * 
     * @return Quaternion representing the character's orientation in world space
     */
    virtual LLQuaternion getCharacterRotation() = 0;

    /**
     * @brief Gets the character's current velocity.
     * 
     * @return 3D velocity vector in units per second
     */
    virtual LLVector3 getCharacterVelocity() = 0;

    /**
     * @brief Gets the character's current angular velocity.
     * 
     * @return 3D angular velocity vector in radians per second
     */
    virtual LLVector3 getCharacterAngularVelocity() = 0;

    /**
     * @brief Performs ground collision detection at a specified position.
     * 
     * Traces downward from the input position to find the ground surface,
     * returning both the ground contact point and surface normal. This is
     * essential for proper character placement and physics simulation.
     * 
     * @param inPos Input position to trace from (world coordinates)
     * @param outPos Output ground contact position
     * @param outNorm Output ground surface normal vector
     */
    virtual void getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm) = 0;

    /**
     * @brief Gets a joint by index from the character's skeleton.
     * 
     * Provides indexed access to joints in the character's skeletal hierarchy.
     * This method supports joint subclasses that may need direct index-based
     * access rather than name-based lookups.
     * 
     * @param i Zero-based index of the joint to retrieve
     * @return Pointer to the joint at the specified index, nullptr if invalid
     */
    virtual LLJoint *getCharacterJoint( U32 i ) = 0;

    /**
     * @brief Gets the current physics time dilation factor.
     * 
     * Returns the time scaling factor applied by the simulator for physics
     * calculations. Values less than 1.0 indicate slowed simulation (lag),
     * while 1.0 represents normal time flow.
     * 
     * @return Time dilation factor (0.0 to 1.0, where 1.0 is normal time)
     */
    virtual F32 getTimeDilation() = 0;

    /**
     * @brief Gets the character's current screen pixel area.
     * 
     * Returns the approximate number of pixels the character occupies on
     * screen, used for level-of-detail calculations and rendering optimization.
     * Larger values indicate the character is closer or more prominent.
     * 
     * @return Pixel area in screen space, 0.0 if not visible
     */
    virtual F32 getPixelArea() const = 0;

    /**
     * @brief Gets the character's head mesh for rendering and collision.
     * 
     * @return Pointer to the head polygon mesh, nullptr if not available
     */
    virtual LLPolyMesh* getHeadMesh() = 0;

    /**
     * @brief Gets the character's upper body mesh for rendering and collision.
     * 
     * @return Pointer to the upper body polygon mesh, nullptr if not available
     */
    virtual LLPolyMesh* getUpperBodyMesh() = 0;

    /**
     * @brief Converts agent-local coordinates to global world coordinates.
     * 
     * Transforms a position from the character's local coordinate system
     * to the global world coordinate system used by the simulator.
     * 
     * @param position Position in agent-local coordinates
     * @return Position in global world coordinates (double precision)
     */
    virtual LLVector3d  getPosGlobalFromAgent(const LLVector3 &position) = 0;

    /**
     * @brief Converts global world coordinates to agent-local coordinates.
     * 
     * Transforms a position from the global world coordinate system
     * to the character's local coordinate system.
     * 
     * @param position Position in global world coordinates (double precision)
     * @return Position in agent-local coordinates
     */
    virtual LLVector3   getPosAgentFromGlobal(const LLVector3d &position) = 0;

    /**
     * @brief Updates all visual parameters that have changed.
     * 
     * Applies visual parameter changes to the character's appearance by
     * calling apply() on parameters whose effective weight has changed
     * since the last update. This is the primary method for synchronizing
     * appearance changes with the visual representation.
     * 
     * Only processes non-animating parameters to avoid interference with
     * ongoing parameter animations.
     */
    virtual void updateVisualParams();

    /**
     * @brief Adds debug text to be displayed above the character.
     * 
     * @param text Debug text string to display
     */
    virtual void addDebugText( const std::string& text ) = 0;

    /**
     * @brief Gets the unique identifier for this character.
     * 
     * @return UUID that uniquely identifies this character instance
     */
    virtual const LLUUID&   getID() const = 0;
    //-------------------------------------------------------------------------
    // End Interface
    //-------------------------------------------------------------------------
    /**
     * @brief Registers a motion constructor for this character.
     * 
     * Associates a motion UUID with a constructor function that can create
     * instances of that motion type. This is typically called during character
     * initialization to set up the available animation repertoire.
     * 
     * @param id UUID identifying the motion type
     * @param create Constructor function that creates motion instances
     * @return true if registration succeeded, false if it failed
     */
    bool registerMotion( const LLUUID& id, LLMotionConstructor create );

    /**
     * @brief Removes a motion registration from this character.
     * 
     * Unregisters the motion constructor for the specified UUID and cleans up
     * any existing motion instances. The motion will no longer be available
     * for playback after removal.
     * 
     * @param id UUID of the motion to remove
     */
    void removeMotion( const LLUUID& id );

    /**
     * @brief Creates or retrieves a motion instance for the specified UUID.
     * 
     * Returns an existing motion instance if one exists, otherwise creates
     * a new instance using the registered constructor. The motion must be
     * registered before calling this method.
     * 
     * @param id UUID of the motion to create or retrieve
     * @return Pointer to the motion instance, nullptr if creation failed
     * @warning Always assign the result to an LLPointer for proper reference counting
     */
    LLMotion* createMotion( const LLUUID &id );

    /**
     * @brief Finds an existing motion instance by UUID.
     * 
     * Searches for a motion instance that has already been created for the
     * specified UUID. Unlike createMotion(), this will not create a new
     * instance if one doesn't exist.
     * 
     * @param id UUID of the motion to find
     * @return Pointer to existing motion instance, nullptr if not found
     */
    LLMotion* findMotion( const LLUUID &id );

    /**
     * @brief Starts playing a motion animation.
     * 
     * Begins playback of the specified motion, creating the motion instance
     * if necessary. The motion will be blended in according to its blend
     * parameters and priority settings.
     * 
     * @param id UUID of the motion to start
     * @param start_offset Time offset in seconds to begin playback from
     * @return true if motion started successfully, false if error occurred
     */
    virtual bool startMotion( const LLUUID& id, F32 start_offset = 0.f);

    /**
     * @brief Stops a currently playing motion animation.
     * 
     * Initiates the stop sequence for the specified motion. Depending on the
     * stop_immediate flag, the motion will either blend out gracefully or
     * stop immediately.
     * 
     * @param id UUID of the motion to stop
     * @param stop_immediate If true, stops immediately; if false, blends out
     * @return true if stop was initiated successfully, false otherwise
     */
    virtual bool stopMotion( const LLUUID& id, bool stop_immediate = false );

    /**
     * @brief Checks if a motion is currently active (playing).
     * 
     * @param id UUID of the motion to check
     * @return true if the motion is currently playing, false otherwise
     */
    bool isMotionActive( const LLUUID& id );

    /**
     * @brief Event handler called when a motion has completely stopped.
     * 
     * This callback is invoked when a motion has finished its stop sequence
     * and has been fully deactivated. Subclasses can override this to perform
     * cleanup or trigger follow-up actions when specific motions end.
     * 
     * The default implementation does nothing.
     * 
     * @param motion Pointer to the motion that has been deactivated
     */
    virtual void requestStopMotion( LLMotion* motion );

    /**
     * @brief Update type flags for motion controller updates.
     * 
     * NORMAL_UPDATE: Standard frame update with full motion processing
     * HIDDEN_UPDATE: Minimal update for non-visible characters (performance optimization)
     * FORCE_UPDATE: Forces full update regardless of other conditions
     */
    enum e_update_t { NORMAL_UPDATE, HIDDEN_UPDATE, FORCE_UPDATE };
    
    /**
     * @brief Updates the motion controller and all active motions.
     * 
     * This is the primary update method that advances all motion playback,
     * handles blending, and applies skeletal transformations. Called from
     * LLVOAvatar::updateCharacter() based on avatar visibility and state.
     * 
     * Update modes in practice:
     * - NORMAL_UPDATE: Standard visible avatar processing
     * - HIDDEN_UPDATE: Minimal processing for invisible/distant avatars (major optimization)
     * - FORCE_UPDATE: Maximum fidelity for animation previews
     * 
     * Performance note: This method is heavily profiled and optimized as it's
     * called for every avatar every frame. HIDDEN_UPDATE can provide significant
     * performance gains by skipping expensive calculations for non-visible avatars.
     * 
     * @param update_type Type of update to perform (affects processing level)
     *
     * @code
     * // Real usage pattern from LLVOAvatar::updateCharacter()
     * if (!visible && !isSelf()) {
     *     updateMotions(LLCharacter::HIDDEN_UPDATE);
     * } else if (mSpecialRenderMode == 1) {
     *     updateMotions(LLCharacter::FORCE_UPDATE);
     * } else {
     *     updateMotions(LLCharacter::NORMAL_UPDATE);
     * }
     * @endcode
     */
    void updateMotions(e_update_t update_type);

    /**
     * @brief Requests that all animations be paused.
     * 
     * Returns a reference-counted pause request handle. Animations remain
     * paused as long as any pause request handles exist. When all handles
     * are released, animations automatically resume.
     * 
     * This design allows multiple systems to independently pause animations
     * without interfering with each other.
     * 
     * @return Pause request handle that maintains the pause state
     */
    LLAnimPauseRequest requestPause();
    
    /**
     * @brief Checks if animations are currently paused.
     * 
     * @return true if animations are paused, false if running normally
     */
    bool areAnimationsPaused() const { return mMotionController.isPaused(); }
    
    /**
     * @brief Sets the time scaling factor for all animations.
     * 
     * Values less than 1.0 slow down animations, greater than 1.0 speed them up.
     * Useful for slow-motion effects or animation debugging.
     * 
     * @param factor Time scaling multiplier (1.0 = normal speed)
     */
    void setAnimTimeFactor(F32 factor) { mMotionController.setTimeFactor(factor); }
    
    /**
     * @brief Sets the fixed time step for animation updates.
     * 
     * Forces animations to advance by a fixed time increment rather than
     * using variable frame times. Useful for deterministic animation playback.
     * 
     * @param time_step Fixed time step in seconds per update
     */
    void setTimeStep(F32 time_step) { mMotionController.setTimeStep(time_step); }

    /**
     * @brief Gets direct access to the motion controller.
     * 
     * Provides low-level access to the motion controller for advanced
     * operations not exposed through the character interface. Use with
     * caution as direct manipulation can interfere with character state.
     * 
     * @return Reference to the underlying motion controller
     */
    LLMotionController& getMotionController() { return mMotionController; }

    /**
     * @brief Releases all motion instances and clears motion cache.
     * 
     * Destroys all motion instances, which removes any cached references
     * to character joint data. This is essential when rebuilding a character's
     * skeleton, as it ensures no stale joint references remain.
     * 
     * This is more thorough than deactivateAllMotions() - it actually
     * destroys the motion objects rather than just stopping them.
     */
    virtual void flushAllMotions();

    /**
     * @brief Deactivates all currently active motions.
     * 
     * Stops all playing animations but preserves the motion instances in
     * memory for potential reuse. This is less destructive than flushAllMotions()
     * and is suitable for temporarily stopping all animation playback.
     */
    virtual void deactivateAllMotions();

    /**
     * @brief Dumps character and skeleton information for debugging.
     * 
     * Recursively prints the character's joint hierarchy and related debug
     * information to the log. Useful for diagnosing skeleton structure issues
     * and understanding character state.
     * 
     * @param joint Starting joint for dump (nullptr to dump entire skeleton)
     */
    virtual void dumpCharacter( LLJoint *joint = NULL );

    /**
     * @brief Gets the preferred pelvis height for this character.
     * 
     * Returns the target height for the character's pelvis joint, used for
     * proper character positioning and ground alignment calculations.
     * 
     * @return Preferred pelvis height in world units
     */
    virtual F32 getPreferredPelvisHeight() { return mPreferredPelvisHeight; }

    /**
     * @brief Gets the position of a collision volume relative to a joint.
     * 
     * Calculates the world position of a collision volume attached to the
     * specified joint. The default implementation returns zero vector.
     * Subclasses should override to provide actual collision volume positioning.
     * 
     * @param joint_index Index of the joint the volume is attached to
     * @param volume_offset Offset of the volume from the joint
     * @return World position of the collision volume
     */
    virtual LLVector3 getVolumePos(S32 joint_index, LLVector3& volume_offset) { return LLVector3::zero; }

    /**
     * @brief Finds the joint associated with a collision volume.
     * 
     * Looks up which joint a collision volume is attached to based on the
     * volume's ID. The default implementation returns nullptr. Subclasses
     * should override to provide actual collision volume lookup.
     * 
     * @param volume_id Identifier of the collision volume
     * @return Joint the volume is attached to, nullptr if not found
     */
    virtual LLJoint* findCollisionVolume(S32 volume_id) { return NULL; }

    /**
     * @brief Gets the ID of a collision volume by name.
     * 
     * Looks up the numeric ID for a named collision volume. The default
     * implementation returns -1 (not found). Subclasses should override
     * to provide actual volume name resolution.
     * 
     * @param name Name of the collision volume to find
     * @return Volume ID, -1 if not found
     */
    virtual S32 getCollisionVolumeID(std::string &name) { return -1; }

    /**
     * @brief Associates arbitrary data with an animation name.
     * 
     * Stores custom data that can be retrieved later using the same name.
     * This provides a way to attach application-specific information to
     * animations for custom processing or state tracking.
     * 
     * @param name Key string to associate with the data
     * @param data Pointer to data to store (not managed by character)
     */
    void setAnimationData(std::string name, void *data);

    /**
     * @brief Retrieves data previously associated with an animation name.
     * 
     * @param name Key string to look up
     * @return Pointer to associated data, nullptr if not found
     */
    void *getAnimationData(std::string name);

    /**
     * @brief Removes data association for an animation name.
     * 
     * Clears the data entry but does not delete the data itself.
     * The caller is responsible for any cleanup of the data pointer.
     * 
     * @param name Key string to remove
     */
    void removeAnimationData(std::string name);

    /**
     * @brief Adds a visual parameter to this character.
     * 
     * Registers a visual parameter with both the index map (for ID-based lookup)
     * and name map (for string-based lookup). The character takes ownership
     * of the parameter and will delete it during destruction.
     * 
     * @param param Visual parameter to add (character takes ownership)
     */
    void addVisualParam(LLVisualParam *param);
    
    /**
     * @brief Adds a shared visual parameter that links to an existing parameter.
     * 
     * Creates a parameter chain where multiple parameters can share the same
     * underlying data. The shared parameter is linked to an existing parameter
     * with the same ID. This is used for parameters that affect multiple
     * aspects of appearance simultaneously.
     * 
     * @param param Shared parameter to add (must have same ID as existing param)
     */
    void addSharedVisualParam(LLVisualParam *param);

    /**
     * @brief Sets the weight of a visual parameter by parameter object.
     * 
     * @param which_param Pointer to the parameter to modify
     * @param weight New weight value to set
     * @return true if parameter was found and weight set, false otherwise
     */
    virtual bool setVisualParamWeight(const LLVisualParam *which_param, F32 weight);
    
    /**
     * @brief Sets the weight of a visual parameter by name.
     * 
     * Parameter names are case-insensitive and stored in a string table
     * for efficient lookup.
     * 
     * @param param_name Name of the parameter to modify
     * @param weight New weight value to set
     * @return true if parameter was found and weight set, false otherwise
     */
    virtual bool setVisualParamWeight(const char* param_name, F32 weight);
    
    /**
     * @brief Sets the weight of a visual parameter by ID.
     * 
     * @param index Numeric ID of the parameter to modify
     * @param weight New weight value to set
     * @return true if parameter was found and weight set, false otherwise
     */
    virtual bool setVisualParamWeight(S32 index, F32 weight);

    /**
     * @brief Gets the current weight of a visual parameter by parameter object.
     * 
     * @param distortion Pointer to the parameter to query
     * @return Current weight value, 0.0 if parameter not found
     */
    F32 getVisualParamWeight(LLVisualParam *distortion);
    
    /**
     * @brief Gets the current weight of a visual parameter by name.
     * 
     * @param param_name Name of the parameter to query (case-insensitive)
     * @return Current weight value, 0.0 if parameter not found
     */
    F32 getVisualParamWeight(const char* param_name);
    
    /**
     * @brief Gets the current weight of a visual parameter by ID.
     * 
     * @param index Numeric ID of the parameter to query
     * @return Current weight value, 0.0 if parameter not found
     */
    F32 getVisualParamWeight(S32 index);

    /**
     * @brief Resets all tweakable visual parameters to their default weights.
     * 
     * Only affects parameters marked as tweakable (typically user-adjustable
     * appearance sliders). Non-tweakable parameters retain their current values.
     * This is commonly used to reset avatar appearance to default settings.
     */
    void clearVisualParamWeights();

    /**
     * @brief Begins iteration over all visual parameters.
     * 
     * Resets the internal iterator to the first parameter and returns it.
     * Use with getNextVisualParam() to iterate through all parameters.
     * 
     * @return First visual parameter, nullptr if no parameters exist
     * 
     * @code
     * for (LLVisualParam* param = character.getFirstVisualParam();
     *      param != nullptr;
     *      param = character.getNextVisualParam())
     * {
     *     // Process each parameter
     * }
     * @endcode
     */
    LLVisualParam*  getFirstVisualParam()
    {
        mCurIterator = mVisualParamIndexMap.begin();
        return getNextVisualParam();
    }
    
    /**
     * @brief Gets the next visual parameter in the iteration sequence.
     * 
     * Returns the next parameter after the current iterator position and
     * advances the iterator. Must be called after getFirstVisualParam().
     * 
     * @return Next visual parameter, nullptr when iteration is complete
     */
    LLVisualParam*  getNextVisualParam()
    {
        if (mCurIterator == mVisualParamIndexMap.end())
            return 0;
        return (mCurIterator++)->second;
    }

    /**
     * @brief Counts visual parameters in a specific group.
     * 
     * Visual parameters are organized into groups (e.g., VISUAL_PARAM_GROUP_BODY,
     * VISUAL_PARAM_GROUP_HEAD) for organizational purposes. This method counts
     * how many parameters belong to a particular group.
     * 
     * @param group Visual parameter group to count
     * @return Number of parameters in the specified group
     */
    S32 getVisualParamCountInGroup(const EVisualParamGroup group) const
    {
        S32 rtn = 0;
        for (const visual_param_index_map_t::value_type& index_pair : mVisualParamIndexMap)
        {
            if (index_pair.second->getGroup() == group)
            {
                ++rtn;
            }
        }
        return rtn;
    }

    /**
     * @brief Gets a visual parameter by numeric ID.
     * 
     * Provides efficient O(log n) lookup of parameters by their unique
     * numeric identifier.
     * 
     * @param id Numeric ID of the parameter
     * @return Pointer to the parameter, nullptr if not found
     */
    LLVisualParam*  getVisualParam(S32 id) const
    {
        visual_param_index_map_t::const_iterator iter = mVisualParamIndexMap.find(id);
        return (iter == mVisualParamIndexMap.end()) ? 0 : iter->second;
    }
    /**
     * @brief Gets the numeric ID of a visual parameter.
     * 
     * Performs a reverse lookup to find the ID associated with a parameter
     * object. This is an O(n) operation as it searches through all parameters.
     * 
     * @param id Pointer to the visual parameter
     * @return Numeric ID of the parameter, 0 if not found
     */
    S32 getVisualParamID(LLVisualParam *id)
    {
        for (visual_param_index_map_t::value_type& index_pair : mVisualParamIndexMap)
        {
            if (index_pair.second == id)
                return index_pair.first;
        }
        return 0;
    }
    /**
     * @brief Gets the total number of visual parameters.
     * 
     * @return Count of all registered visual parameters
     */
    S32             getVisualParamCount() const { return (S32)mVisualParamIndexMap.size(); }
    /**
     * @brief Gets a visual parameter by name.
     * 
     * @param name Name of the parameter (case-insensitive)
     * @return Pointer to the parameter, nullptr if not found
     */
    LLVisualParam*  getVisualParam(const char *name);

    /**
     * @brief Advances animation for all tweakable visual parameters.
     * 
     * Updates the animation state of parameters that are currently animating
     * (transitioning between values). Only affects tweakable parameters,
     * which are typically user-adjustable appearance controls.
     * 
     * @param delta Time elapsed since last update in seconds
     */
    void animateTweakableVisualParams(F32 delta)
    {
        for (auto& it : mVisualParamIndexMap)
        {
            if (it.second->isTweakable())
            {
                it.second->animate(delta);
            }
        }
    }

    /**
     * @brief Applies all visual parameters for a specific avatar gender.
     * 
     * Forces all visual parameters to apply their current values to the
     * character's appearance. Gender-specific parameters will only take
     * effect if they match the specified sex.
     * 
     * @param avatar_sex Gender to apply parameters for (SEX_MALE or SEX_FEMALE)
     */
    void applyAllVisualParams(ESex avatar_sex)
    {
        for (auto& it : mVisualParamIndexMap)
        {
            it.second->apply(avatar_sex);
        }
    }

    /**
     * @brief Gets the character's current gender setting.
     * 
     * @return Current gender (SEX_MALE or SEX_FEMALE)
     */
    ESex getSex() const         { return mSex; }
    
    /**
     * @brief Sets the character's gender.
     * 
     * Changes the character's gender, which affects which visual parameters
     * are active and how they're applied to the character's appearance.
     * 
     * @param sex New gender to set (SEX_MALE or SEX_FEMALE)
     */
    void setSex( ESex sex )     { mSex = sex; }

    /**
     * @brief Gets the appearance change serial number.
     * 
     * The serial number increments whenever the character's appearance changes,
     * allowing other systems to detect when appearance updates are needed.
     * 
     * @return Current appearance serial number
     */
    U32             getAppearanceSerialNum() const      { return mAppearanceSerialNum; }
    
    /**
     * @brief Sets the appearance change serial number.
     * 
     * @param num New serial number to set
     */
    void            setAppearanceSerialNum( U32 num )   { mAppearanceSerialNum = num; }

    /**
     * @brief Gets the skeleton change serial number.
     * 
     * The serial number increments whenever the character's skeleton structure
     * changes, allowing systems to detect when skeleton-dependent data needs
     * to be rebuilt.
     * 
     * @return Current skeleton serial number
     */
    U32             getSkeletonSerialNum() const        { return mSkeletonSerialNum; }
    
    /**
     * @brief Sets the skeleton change serial number.
     * 
     * @param num New serial number to set
     */
    void            setSkeletonSerialNum( U32 num ) { mSkeletonSerialNum = num; }

    static std::list< LLCharacter* > sInstances;        /// Global list of all character instances
    static bool sAllowInstancesChange ;                 /// Debug flag to control instance creation/destruction

    /**
     * @brief Sets the character's hover offset from the ground.
     * 
     * The hover offset adjusts how far above the ground surface the character
     * appears to float. This is commonly used for flight or hovering effects.
     * 
     * @param hover_offset 3D offset vector from ground contact point
     * @param send_update Whether to broadcast the change to other clients
     */
    virtual void    setHoverOffset(const LLVector3& hover_offset, bool send_update=true) { mHoverOffset = hover_offset; }
    
    /**
     * @brief Gets the character's current hover offset.
     * 
     * @return 3D offset vector from ground contact point
     */
    const LLVector3& getHoverOffset() const { return mHoverOffset; }

protected:
    LLMotionController  mMotionController;              /// Handles all motion playback and blending

    typedef std::map<std::string, void *> animation_data_map_t;
    animation_data_map_t mAnimationData;                /// Custom data associated with animation names

    F32                 mPreferredPelvisHeight;         /// Target height for pelvis positioning
    ESex                mSex;                           /// Character gender (affects visual parameters)
    U32                 mAppearanceSerialNum;           /// Increments when appearance changes
    U32                 mSkeletonSerialNum;             /// Increments when skeleton structure changes
    LLAnimPauseRequest  mPauseRequest;                  /// Reference-counted pause request handle

private:
    typedef std::map<S32, LLVisualParam *>      visual_param_index_map_t;
    typedef std::map<char *, LLVisualParam *>   visual_param_name_map_t;

    visual_param_index_map_t::iterator          mCurIterator;           /// Iterator for getFirstVisualParam/getNextVisualParam
    visual_param_index_map_t                    mVisualParamIndexMap;   /// Visual parameters indexed by numeric ID
    visual_param_name_map_t                     mVisualParamNameMap;    /// Visual parameters indexed by name string

    static LLStringTable sVisualParamNames;                             /// Shared string table for parameter names

    LLVector3 mHoverOffset;                                             /// Character's offset from ground surface
};

#endif // LL_LLCHARACTER_H

