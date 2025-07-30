/**
 * @file lljoint.h
 * @brief Implementation of LLJoint class.
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

#ifndef LL_LLJOINT_H
#define LL_LLJOINT_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include <string>
#include <list>

#include "v3math.h"
#include "v4math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "xform.h"
#include "llmatrix4a.h"

/// Maximum joints that can be influenced by a single mesh attachment
constexpr S32 LL_CHARACTER_MAX_JOINTS_PER_MESH = 15;

/// Total animated joints in the avatar skeleton system
/// Calculated as #bones + #collision_volumes + #attachments + 2, rounded to next multiple of 4
/// Must be divisible by 4 for SIMD optimization
constexpr U32 LL_CHARACTER_MAX_ANIMATED_JOINTS = 216;

/// Maximum joints that can be bound to a single mesh object for rendering
constexpr U32 LL_MAX_JOINTS_PER_MESH_OBJECT = 110;

/// Reserved joint numbers for special motion types to avoid conflicts in updateMotionsByType()
constexpr U32 LL_HAND_JOINT_NUM = (LL_CHARACTER_MAX_ANIMATED_JOINTS-1);
constexpr U32 LL_FACE_JOINT_NUM = (LL_CHARACTER_MAX_ANIMATED_JOINTS-2);

/// Maximum priority level for joint animations (0=lowest, 7=highest)
constexpr S32 LL_CHARACTER_MAX_PRIORITY = 7;

/// Maximum allowed pelvis offset from default position in meters
constexpr F32 LL_MAX_PELVIS_OFFSET = 5.f;

/// Minimum position change threshold to trigger joint updates (0.1mm precision)
constexpr F32 LL_JOINT_TRESHOLD_POS_OFFSET = 0.0001f;

/**
 * @brief Manages position and scale overrides from attached mesh objects.
 * 
 * This class tracks how attached objects (like rigged mesh clothing or attachments)
 * modify joint positions and scales from their default skeleton values. Each
 * override is identified by the mesh object's UUID, allowing multiple attachments
 * to contribute different modifications to the same joint.
 * 
 * The system finds the "active" override by using a deterministic selection
 * method when multiple overrides exist for the same joint.
 */
class LLVector3OverrideMap
{
public:
    LLVector3OverrideMap() {}
    
    /**
     * @brief Finds the currently active override for this joint.
     * 
     * When multiple attachments provide overrides for the same joint, this
     * method determines which one is currently in effect using a consistent
     * selection algorithm.
     * 
     * @param mesh_id [out] UUID of the attachment providing the active override
     * @param pos [out] The override vector value
     * @return true if an active override exists, false if using default values
     */
    bool findActiveOverride(LLUUID& mesh_id, LLVector3& pos) const;
    
    /**
     * @brief Outputs all current overrides to a debug stream.
     * 
     * @param os Output stream to write debug information to
     */
    void showJointVector3Overrides(std::ostringstream& os) const;
    
    /**
     * @brief Gets the number of registered overrides.
     * 
     * @return Count of mesh attachments currently providing overrides
     */
    U32 count() const;
    
    /**
     * @brief Adds or updates an override from a mesh attachment.
     * 
     * @param mesh_id UUID of the attachment providing this override
     * @param pos The position or scale override vector
     */
    void add(const LLUUID& mesh_id, const LLVector3& pos);
    
    /**
     * @brief Removes an override from a specific mesh attachment.
     * 
     * @param mesh_id UUID of the attachment whose override to remove
     * @return true if an override was removed, false if none existed
     */
    bool remove(const LLUUID& mesh_id);
    
    /**
     * @brief Removes all overrides, reverting joint to default values.
     */
    void clear();

    typedef std::map<LLUUID,LLVector3> map_type;
    
    /**
     * @brief Gets direct access to the internal override map.
     * 
     * @return Const reference to the UUID->Vector3 mapping
     */
    const map_type& getMap() const { return m_map; }
    
private:
    map_type m_map;  /// Maps attachment UUIDs to their override vectors
};

inline bool operator==(const LLVector3OverrideMap& a, const LLVector3OverrideMap& b)
{
    return a.getMap() == b.getMap();
}

inline bool operator!=(const LLVector3OverrideMap& a, const LLVector3OverrideMap& b)
{
    return !(a == b);
}

/**
 * @brief Core building block of the avatar skeleton system.
 * 
 * LLJoint represents a single bone or connection point in the avatar's hierarchical
 * skeleton structure. Each joint maintains its own transformation (position, rotation, scale)
 * relative to its parent, and can have multiple children forming a tree structure.
 * 
 * This class serves as the foundation for:
 * - Avatar skeleton definition and bone hierarchy
 * - Animation target points for motion controllers
 * - Attachment points for rigged mesh objects
 * - Inverse kinematics and procedural animation
 * 
 * Key features:
 * - Hierarchical parent/child relationships
 * - Local and world-space transformations
 * - Attachment-based position/scale overrides for rigged mesh
 * - Efficient matrix caching with dirty flag system
 * - Animation priority handling
 * 
 * Performance note: This class is heavily optimized as joints are updated every
 * frame during animation. Matrix calculations use SIMD when possible and employ
 * dirty flagging to minimize unnecessary recalculations.
 * 
 * @code
 * // Real usage: Motion registration during avatar initialization
 * // All motions are registered once when first avatar is created
 * if (LLCharacter::sInstances.size() == 1) {
 *     registerMotion(ANIM_AGENT_HAND_MOTION, LLHandMotion::create);
 *     registerMotion(ANIM_AGENT_HEAD_ROT, LLHeadRotMotion::create);
 *     registerMotion(ANIM_AGENT_EYE, LLEyeMotion::create);
 *     // ~40 more motion types registered globally
 * }
 * 
 * // Joint access patterns in motion initialization
 * mShoulderState->setJoint(character->getJoint("mShoulderLeft"));
 * mElbowState->setJoint(character->getJoint("mElbowLeft"));
 * @endcode
 */
LL_ALIGN_PREFIX(16)
class LLJoint
{
    LL_ALIGN_NEW
public:
    /**
     * @brief Animation priority levels for motion blending.
     * 
     * When multiple animations affect the same joint, priority determines
     * which animation takes precedence. Higher priority animations override
     * or blend with lower priority ones.
     */
    enum JointPriority
    {
        USE_MOTION_PRIORITY = -1,   /// Use the motion's own priority setting
        LOW_PRIORITY = 0,           /// Background/idle animations
        MEDIUM_PRIORITY,            /// Standard locomotion and gestures
        HIGH_PRIORITY,              /// Important character actions
        HIGHER_PRIORITY,            /// Critical animations that rarely blend
        HIGHEST_PRIORITY,           /// Override animations, death poses
        ADDITIVE_PRIORITY = LL_CHARACTER_MAX_PRIORITY  /// Additive effects layered on top
    };

    /**
     * @brief Flags indicating which transformation components need recalculation.
     * 
     * This optimization system avoids expensive matrix operations by tracking
     * which parts of the joint's transformation have changed since the last
     * world matrix update.
     */
    enum DirtyFlags
    {
        MATRIX_DIRTY = 0x1 << 0,    /// World matrix needs recalculation
        ROTATION_DIRTY = 0x1 << 1,  /// Rotation has changed
        POSITION_DIRTY = 0x1 << 2,  /// Position has changed
        ALL_DIRTY = 0x7             /// All components need updating
    };
public:
    /**
     * @brief Support level classification for animation compatibility.
     * 
     * Determines which animation sets and features this joint supports,
     * used for backwards compatibility with different avatar rigs.
     */
    enum SupportCategory
    {
        SUPPORT_BASE,       /// Basic avatar rig joints (original SL skeleton)
        SUPPORT_EXTENDED    /// Extended rig joints (bento hands, face, etc.)
    };
protected:
    /// 16-byte aligned world transformation matrix for SIMD operations
    LL_ALIGN_16(LLMatrix4a          mWorldMatrix);
    /// Local transformation matrix (position, rotation, scale relative to parent)
    LLXformMatrix       mXform;

    /// Human-readable joint name for debugging and animation targeting
    std::string mName;

    /// Animation compatibility support level
    SupportCategory mSupport;

    /// Parent joint in the skeleton hierarchy (NULL for root joints)
    LLJoint *mParent;

    /// Default position defined by the base skeleton before any overrides
    LLVector3       mDefaultPosition;
    /// Default scale defined by the base skeleton before any overrides
    LLVector3       mDefaultScale;

public:
    /// Bitmask of transformation components needing recalculation
    U32             mDirtyFlags;
    /// Whether local transform matrix needs updating
    bool            mUpdateXform;

    /// Offset from joint center to skin binding point for mesh deformation
    LLVector3       mSkinOffset;

    /// Bone endpoint for visualization and external tool compatibility
    /// Only used for diagnostic display and export to modeling programs
    LLVector3       mEnd;

    /// Unique numeric identifier used by animation system for fast joint lookup
    S32             mJointNum;

    /// Child joints in the skeleton hierarchy
    typedef std::vector<LLJoint*> joints_t;
    joints_t mChildren;

    /// Global debug counters for performance monitoring
    static S32      sNumTouches;    /// Count of joints marked dirty
    static S32      sNumUpdates;    /// Count of matrix recalculations
    
    /// Joint names to include in debug output (empty = log all joints)
    typedef std::set<std::string> debug_joint_name_t;
    static debug_joint_name_t s_debugJointNames;
    
    /**
     * @brief Sets which joints to include in debug logging.
     * 
     * @param names Set of joint names to monitor, empty set logs all joints
     */
    static void setDebugJointNames(const debug_joint_name_t& names);
    
    /**
     * @brief Sets debug joint names from a comma-separated string.
     * 
     * @param names_string Comma-separated list of joint names to monitor
     */
    static void setDebugJointNames(const std::string& names_string);

    /// Position modifications from rigged mesh attachments
    LLVector3OverrideMap m_attachmentPosOverrides;
    /// Original position before any attachment overrides were applied
    LLVector3 m_posBeforeOverrides;

    /// Scale modifications from rigged mesh attachments
    LLVector3OverrideMap m_attachmentScaleOverrides;
    /// Original scale before any attachment overrides were applied
    LLVector3 m_scaleBeforeOverrides;

    /**
     * @brief Recalculates joint position considering all active overrides.
     * 
     * This is called internally when attachment overrides change to update
     * the joint's effective position.
     * 
     * @param av_info Avatar debugging identifier for logging purposes
     */
    void updatePos(const std::string& av_info);
    
    /**
     * @brief Recalculates joint scale considering all active overrides.
     * 
     * This is called internally when attachment overrides change to update
     * the joint's effective scale.
     * 
     * @param av_info Avatar debugging identifier for logging purposes
     */
    void updateScale(const std::string& av_info);

public:
    /**
     * @brief Default constructor for an uninitialized joint.
     * 
     * Creates a joint with identity transform and no parent. You must call
     * setup() or manually configure the joint before use.
     */
    LLJoint();

    /**
     * @brief Constructor with joint number for animation system compatibility.
     * 
     * @deprecated This constructor exists only for legacy appearance utility
     * compatibility and should not be used in new code. Joint numbers are
     * typically assigned after construction during skeleton initialization.
     * 
     * @param joint_num Numeric identifier for animation system lookups
     */
    LLJoint(S32 joint_num);

    /**
     * @brief Constructor with name and optional parent joint.
     * 
     * Note: This constructor is primarily used for LLVOAvatarSelf::mScreenp
     * and may not initialize all members completely.
     * 
     * @param name Human-readable name for this joint
     * @param parent Parent joint in skeleton hierarchy, NULL for root joints
     */
    LLJoint( const std::string &name, LLJoint *parent=NULL );
    
    /**
     * @brief Virtual destructor ensures proper cleanup in inheritance hierarchy.
     */
    virtual ~LLJoint();

private:
    /**
     * @brief Initializes joint to default state.
     * 
     * This private method is called by all constructors to set up
     * default transformation values, clear dirty flags, and initialize
     * member variables to safe defaults.
     */
    void init();

public:
    /**
     * @brief Configures joint with name and parent relationship.
     * 
     * This is the preferred way to initialize a joint after construction,
     * establishing its place in the skeleton hierarchy.
     * 
     * @param name Human-readable identifier for debugging and animation targeting
     * @param parent Parent joint in hierarchy, NULL for root joints
     */
    void setup( const std::string &name, LLJoint *parent=NULL );

    /**
     * @brief Marks joint transformation components as needing recalculation.
     * 
     * This is called whenever joint properties change to ensure world matrices
     * are updated on the next frame. Increments the global sNumTouches counter.
     * 
     * @param flags Bitmask of DirtyFlags indicating which components changed
     */
    void touch(U32 flags = ALL_DIRTY);

    /**
     * @brief Gets the joint's name.
     * 
     * @return Human-readable joint identifier
     */
    const std::string& getName() const { return mName; }
    
    /**
     * @brief Sets the joint's name for debugging and identification.
     * 
     * @param name New human-readable identifier
     */
    void setName( const std::string &name ) { mName = name; }

    /**
     * @brief Gets the joint's numeric identifier used by animation system.
     * 
     * @return Numeric ID for fast animation system lookups, -1 if unassigned
     */
    S32 getJointNum() const { return mJointNum; }
    
    /**
     * @brief Sets the joint's numeric identifier for animation system.
     * 
     * Joint numbers are used by the animation system for fast lookups when
     * applying motion data. Typically assigned during skeleton initialization.
     * 
     * @param joint_num Unique numeric identifier
     */
    void setJointNum(S32 joint_num);

    /**
     * @brief Gets the joint's animation compatibility support level.
     * 
     * @return Support category (base or extended rig)
     */
    SupportCategory getSupport() const { return mSupport; }
    
    /**
     * @brief Sets the joint's animation compatibility support level.
     * 
     * @param support Support category for animation system compatibility
     */
    void setSupport( const SupportCategory& support) { mSupport = support; }
    
    /**
     * @brief Sets support level from string representation.
     * 
     * @param support_string "base" or "extended" support level
     */
    void setSupport( const std::string& support_string);

    /**
     * @brief Sets the bone endpoint for visualization and export.
     * 
     * This is primarily used for diagnostic display and compatibility
     * with external modeling tools like Blender.
     * 
     * @param end Endpoint position in joint's local coordinate space
     */
    void setEnd( const LLVector3& end) { mEnd = end; }
    
    /**
     * @brief Gets the bone endpoint position.
     * 
     * @return Endpoint in joint's local coordinate space
     */
    const LLVector3& getEnd() const { return mEnd; }

    /**
     * @brief Gets the parent joint in the skeleton hierarchy.
     * 
     * @return Parent joint pointer, NULL for root joints
     */
    LLJoint *getParent() { return mParent; }

    /**
     * @brief Finds the root joint by traversing up the parent chain.
     * 
     * @return Root joint of this skeleton hierarchy
     */
    LLJoint *getRoot();

    /**
     * @brief Recursively searches for a child joint by name.
     * 
     * Performs depth-first search through the joint's descendants
     * to find a joint with the specified name.
     * 
     * @param name Name of the joint to find
     * @return Pointer to found joint, NULL if not found
     */
    LLJoint* findJoint(std::string_view name);

    /**
     * @brief Adds a child joint to this joint's hierarchy.
     * 
     * The child joint's parent pointer is automatically updated to point
     * to this joint, establishing the parent-child relationship.
     * 
     * @param joint Child joint to add to the hierarchy
     */
    void addChild( LLJoint *joint );
    
    /**
     * @brief Removes a specific child joint from this joint's hierarchy.
     * 
     * The child joint's parent pointer is set to NULL, but the child
     * joint itself is not deleted.
     * 
     * @param joint Child joint to remove from hierarchy
     */
    void removeChild( LLJoint *joint );
    
    /**
     * @brief Removes all child joints from this joint's hierarchy.
     * 
     * All child joints have their parent pointers set to NULL, but
     * the child joints themselves are not deleted.
     */
    void removeAllChildren();

    /**
     * @brief Gets the joint's current local position including any overrides.
     * 
     * Returns the effective position after applying attachment overrides
     * from rigged mesh objects.
     * 
     * @return Position relative to parent joint
     */
    const LLVector3& getPosition();
    
    /**
     * @brief Sets the joint's local position relative to its parent.
     * 
     * @param pos New position relative to parent joint
     * @param apply_attachment_overrides Whether to recalculate attachment overrides
     */
    void setPosition( const LLVector3& pos, bool apply_attachment_overrides = false );

    /**
     * @brief Sets the joint's default position as defined by the base skeleton.
     * 
     * This is the position before any attachment overrides or animations
     * are applied, used as the baseline for calculations.
     * 
     * @param pos Default position from skeleton definition
     */
    void setDefaultPosition( const LLVector3& pos );
    
    /**
     * @brief Gets the joint's default position from the base skeleton.
     * 
     * @return Position before any overrides or animations
     */
    const LLVector3& getDefaultPosition() const;

    /**
     * @brief Sets the joint's default scale as defined by the base skeleton.
     * 
     * This is the scale before any attachment overrides or animations
     * are applied, used as the baseline for calculations.
     * 
     * @param scale Default scale from skeleton definition
     */
    void setDefaultScale( const LLVector3& scale );
    
    /**
     * @brief Gets the joint's default scale from the base skeleton.
     * 
     * @return Scale before any overrides or animations
     */
    const LLVector3& getDefaultScale() const;

    /**
     * @brief Gets the joint's position in world coordinates.
     * 
     * Transforms the local position through the entire parent chain
     * to get the absolute world position.
     * 
     * @return Position in world coordinate system
     */
    LLVector3 getWorldPosition();
    
    /**
     * @brief Gets the joint's world position from the previous frame.
     * 
     * Used for motion blur and velocity calculations.
     * 
     * @return Previous frame's world position
     */
    LLVector3 getLastWorldPosition();
    
    /**
     * @brief Sets the joint's world position by computing local transform.
     * 
     * Automatically calculates the required local position to achieve
     * the specified world position given the current parent transforms.
     * 
     * @param pos Desired world position
     */
    void setWorldPosition( const LLVector3& pos );

    /**
     * @brief Gets the joint's local rotation relative to its parent.
     * 
     * @return Rotation as quaternion relative to parent joint
     */
    const LLQuaternion& getRotation();
    
    /**
     * @brief Sets the joint's local rotation relative to its parent.
     * 
     * @param rot New rotation quaternion relative to parent
     */
    void setRotation( const LLQuaternion& rot );

    /**
     * @brief Gets the joint's rotation in world coordinates.
     * 
     * Combines local rotation with all parent rotations to get
     * absolute world orientation.
     * 
     * @return Rotation in world coordinate system
     */
    LLQuaternion getWorldRotation();
    
    /**
     * @brief Gets the joint's world rotation from the previous frame.
     * 
     * Used for motion blur and angular velocity calculations.
     * 
     * @return Previous frame's world rotation
     */
    LLQuaternion getLastWorldRotation();
    
    /**
     * @brief Sets the joint's world rotation by computing local transform.
     * 
     * Automatically calculates the required local rotation to achieve
     * the specified world rotation given the current parent transforms.
     * 
     * @param rot Desired world rotation
     */
    void setWorldRotation( const LLQuaternion& rot );

    /**
     * @brief Gets the joint's current local scale including any overrides.
     * 
     * Returns the effective scale after applying attachment overrides
     * from rigged mesh objects.
     * 
     * @return Scale relative to parent joint
     */
    const LLVector3& getScale();
    
    /**
     * @brief Sets the joint's local scale relative to its parent.
     * 
     * @param scale New scale relative to parent joint
     * @param apply_attachment_overrides Whether to recalculate attachment overrides
     */
    void setScale( const LLVector3& scale, bool apply_attachment_overrides = false );

    /**
     * @brief Gets the joint's world transformation matrix.
     * 
     * Returns the 4x4 matrix that transforms points from this joint's
     * local space to world space, combining all parent transformations.
     * 
     * @return World transformation matrix (standard precision)
     */
    const LLMatrix4 &getWorldMatrix();
    
    /**
     * @brief Sets the world matrix by decomposing into local transforms.
     * 
     * Automatically computes the required local transformation to achieve
     * the specified world matrix given current parent transforms.
     * 
     * @param mat Desired world transformation matrix
     */
    void setWorldMatrix( const LLMatrix4& mat );

    /**
     * @brief Gets the joint's world transformation matrix (SIMD optimized).
     * 
     * Returns the 16-byte aligned version for SIMD operations.
     * 
     * @return World transformation matrix (SIMD aligned)
     */
    const LLMatrix4a& getWorldMatrix4a();

    /**
     * @brief Updates world matrices for all child joints recursively.
     * 
     * Propagates transformation changes down the skeleton hierarchy,
     * updating all descendant joints' world matrices.
     */
    void updateWorldMatrixChildren();
    
    /**
     * @brief Updates world matrix by combining with parent transforms.
     * 
     * Recalculates this joint's world matrix based on its local transform
     * and the current parent chain transformations.
     */
    void updateWorldMatrixParent();

    /**
     * @brief Updates world position, rotation, and scale from parent chain.
     * 
     * Alternative to full matrix update when only PRS (Position, Rotation, Scale)
     * values are needed, which can be more efficient than full matrix math.
     */
    void updateWorldPRSParent();

    /**
     * @brief Updates the world transformation matrix if marked dirty.
     * 
     * Checks dirty flags and recalculates world matrix only if needed,
     * incrementing the global sNumUpdates counter when updates occur.
     */
    void updateWorldMatrix();

    /**
     * @brief Gets the offset from joint center to skin binding point.
     * 
     * This offset is used by the mesh deformation system to position
     * vertex weights relative to the joint's transformation.
     * 
     * @return Skin binding offset in joint's local space
     */
    const LLVector3 &getSkinOffset();
    
    /**
     * @brief Sets the offset from joint center to skin binding point.
     * 
     * @param offset Skin binding offset in joint's local coordinate space
     */
    void setSkinOffset( const LLVector3 &offset);

    /**
     * @brief Gets direct access to the local transformation matrix.
     * 
     * Provides access to the underlying LLXformMatrix for advanced
     * manipulation or direct matrix operations.
     * 
     * @return Pointer to local transformation matrix
     */
    LLXformMatrix   *getXform() { return &mXform; }

    /**
     * @brief Clamps rotation between old and new values for smooth transitions.
     * 
     * Used to prevent jarring rotation changes by limiting the angular
     * change between frames, useful for procedural animations.
     * 
     * @param old_rot Previous rotation quaternion
     * @param new_rot Desired new rotation quaternion
     */
    void clampRotation(LLQuaternion old_rot, LLQuaternion new_rot);

    /**
     * @brief Indicates whether this joint can be targeted by animations.
     * 
     * Virtual method allowing derived joint types to specify if they
     * should be included in animation processing.
     * 
     * @return true if joint accepts animation data, false to skip
     */
    virtual bool isAnimatable() const { return true; }

    /**
     * @brief Adds a position override from a rigged mesh attachment.
     * 
     * When rigged mesh clothing or accessories are attached, they can specify
     * different joint positions than the base skeleton. This method registers
     * such an override and recalculates the joint's effective position.
     * 
     * @param pos Override position in joint's local coordinate space
     * @param mesh_id UUID of the attachment providing this override
     * @param av_info Avatar identifier for debugging purposes
     * @param active_override_changed [out] Set to true if the active override changed
     */
    void addAttachmentPosOverride( const LLVector3& pos, const LLUUID& mesh_id, const std::string& av_info, bool& active_override_changed );
    
    /**
     * @brief Removes a position override from a specific attachment.
     * 
     * Called when a rigged mesh attachment is detached or no longer
     * influences this joint's position.
     * 
     * @param mesh_id UUID of the attachment whose override to remove
     * @param av_info Avatar identifier for debugging purposes
     * @param active_override_changed [out] Set to true if the active override changed
     */
    void removeAttachmentPosOverride( const LLUUID& mesh_id, const std::string& av_info, bool& active_override_changed );
    
    /**
     * @brief Checks if this joint has any active position overrides.
     * 
     * @param pos [out] The currently active override position, if any
     * @param mesh_id [out] UUID of attachment providing the active override
     * @return true if an active override exists, false if using default position
     */
    bool hasAttachmentPosOverride( LLVector3& pos, LLUUID& mesh_id ) const;
    
    /**
     * @brief Removes all position overrides, reverting to default position.
     */
    void clearAttachmentPosOverrides();
    
    /**
     * @brief Outputs all position overrides to debug log.
     * 
     * @param av_info Avatar identifier for log context
     */
    void showAttachmentPosOverrides(const std::string& av_info) const;

    /**
     * @brief Adds a scale override from a rigged mesh attachment.
     * 
     * When rigged mesh clothing or accessories are attached, they can specify
     * different joint scales than the base skeleton. This method registers
     * such an override and recalculates the joint's effective scale.
     * 
     * @param scale Override scale in joint's local coordinate space
     * @param mesh_id UUID of the attachment providing this override
     * @param av_info Avatar identifier for debugging purposes
     */
    void addAttachmentScaleOverride( const LLVector3& scale, const LLUUID& mesh_id, const std::string& av_info );
    
    /**
     * @brief Removes a scale override from a specific attachment.
     * 
     * Called when a rigged mesh attachment is detached or no longer
     * influences this joint's scale.
     * 
     * @param mesh_id UUID of the attachment whose override to remove
     * @param av_info Avatar identifier for debugging purposes
     */
    void removeAttachmentScaleOverride( const LLUUID& mesh_id, const std::string& av_info );
    
    /**
     * @brief Checks if this joint has any active scale overrides.
     * 
     * @param scale [out] The currently active override scale, if any
     * @param mesh_id [out] UUID of attachment providing the active override
     * @return true if an active override exists, false if using default scale
     */
    bool hasAttachmentScaleOverride( LLVector3& scale, LLUUID& mesh_id ) const;
    
    /**
     * @brief Removes all scale overrides, reverting to default scale.
     */
    void clearAttachmentScaleOverrides();
    
    /**
     * @brief Outputs all scale overrides to debug log.
     * 
     * @param av_info Avatar identifier for log context
     */
    void showAttachmentScaleOverrides(const std::string& av_info) const;

    /**
     * @brief Collects statistics about all position overrides on this joint.
     * 
     * Used for debugging and performance analysis to understand how many
     * attachments are influencing joint positions.
     * 
     * @param num_pos_overrides [out] Total count of position overrides
     * @param distinct_pos_overrides [out] Set of unique override position values
     */
    void getAllAttachmentPosOverrides(S32& num_pos_overrides,
                                      std::set<LLVector3>& distinct_pos_overrides) const;
    
    /**
     * @brief Collects statistics about all scale overrides on this joint.
     * 
     * Used for debugging and performance analysis to understand how many
     * attachments are influencing joint scales.
     * 
     * @param num_scale_overrides [out] Total count of scale overrides
     * @param distinct_scale_overrides [out] Set of unique override scale values
     */
    void getAllAttachmentScaleOverrides(S32& num_scale_overrides,
                                        std::set<LLVector3>& distinct_scale_overrides) const;

    /**
     * @brief Checks if a position override is significant enough to apply.
     * 
     * Position overrides below the threshold (LL_JOINT_TRESHOLD_POS_OFFSET)
     * are considered negligible and may be ignored to avoid unnecessary
     * computations and visual artifacts.
     * 
     * @param pos Position override to evaluate
     * @return true if override exceeds significance threshold
     */
    bool aboveJointPosThreshold(const LLVector3& pos) const;
    
    /**
     * @brief Checks if a scale override is significant enough to apply.
     * 
     * Scale overrides that are very close to 1.0 may be ignored to avoid
     * unnecessary computations and visual artifacts from tiny scaling.
     * 
     * @param scale Scale override to evaluate  
     * @return true if override exceeds significance threshold
     */
    bool aboveJointScaleThreshold(const LLVector3& scale) const;
} LL_ALIGN_POSTFIX(16);
#endif // LL_LLJOINT_H

