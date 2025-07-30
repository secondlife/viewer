/**
 * @file llavatarappearance.h
 * @brief Declaration of LLAvatarAppearance class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_AVATAR_APPEARANCE_H
#define LL_AVATAR_APPEARANCE_H

#include "llcharacter.h"
#include "llavatarappearancedefines.h"
#include "llavatarjointmesh.h"
#include "lldriverparam.h"
#include "lltexlayer.h"
#include "llviewervisualparam.h"
#include "llxmltree.h"

class LLTexLayerSet;
class LLTexGlobalColor;
class LLTexGlobalColorInfo;
class LLWearableData;
class LLAvatarBoneInfo;
class LLAvatarSkeletonInfo;

/**
 * @brief Core avatar appearance and rendering system.
 * 
 * LLAvatarAppearance extends LLCharacter to provide the fundamental avatar
 * appearance, rendering, and customization functionality used throughout
 * Second Life. This class serves as the foundation for both user avatars
 * and animated objects that need human-like appearance.
 * 
 * Key responsibilities:
 * - **Skeleton Management**: Loads and manages the avatar's joint hierarchy from XML
 * - **Mesh Rendering**: Handles polygon meshes for body parts and attachments
 * - **Visual Parameters**: Implements the morph system for body shape customization
 * - **Texture Baking**: Manages the complex system of layered textures and clothing
 * - **Wearables Integration**: Provides the foundation for clothing and attachment systems
 * - **Physics Integration**: Handles collision volumes and physics constraints
 * 
 * This is an abstract base class in the avatar inheritance hierarchy:
 * LLCharacter -> **LLAvatarAppearance** -> LLVOAvatar -> LLVOAvatarSelf
 * 
 * LLVOAvatar provides the concrete implementation with viewer-specific rendering,
 * networking, and world interaction. LLVOAvatarSelf extends LLVOAvatar for the
 * user's own avatar with special behaviors (appearance editing, first-person view, etc.).
 * 
 * See LLVOAvatar for the primary concrete implementation and LLVOAvatarSelf for
 * the specialized self-avatar implementation.
 * 
 * Architecture:
 * - Inherits motion and animation capabilities from LLCharacter
 * - Uses XML-driven configuration for skeleton, meshes, and visual parameters
 * - Employs a complex texture layering system for customizable appearance
 * - Integrates with the wearables system for clothing and accessories
 * 
 * Performance considerations:
 * - Avatar construction is deferred and checked via mIsBuilt flag in all major operations
 * - Texture baking (invalidateComposite) is extremely expensive and triggers full rebaking
 * - Many methods early-exit with !mIsBuilt checks to avoid processing incomplete avatars
 * - The appearance system is heavily profiled under LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR
 * - Level-of-detail optimizations use HIDDEN_UPDATE for distant/invisible avatars
 * 
 * Critical usage pattern: Most avatar operations check isBuilt() first, as attempting
 * to use an incompletely constructed avatar can cause crashes or incorrect behavior.
 */
class LLAvatarAppearance : public LLCharacter
{
    LOG_CLASS(LLAvatarAppearance);

protected:
    struct LLAvatarXmlInfo;

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/
private:
    /**
     * @brief Default constructor is hidden to enforce proper initialization.
     * 
     * LLAvatarAppearance requires a wearable data object for proper operation,
     * so the default constructor is private to prevent invalid instantiation.
     */
    LLAvatarAppearance() {}

public:
    /**
     * @brief Constructs an avatar appearance with required wearable data.
     * 
     * Initializes the avatar appearance system with the provided wearable data
     * container. The wearable data manages clothing, attachments, and other
     * customizable appearance elements.
     * 
     * @param wearable_data Pointer to wearable data container (not owned by this class)
     */
    LLAvatarAppearance(LLWearableData* wearable_data);
    
    /**
     * @brief Destroys the avatar appearance and cleans up resources.
     * 
     * Performs cleanup of meshes, textures, visual parameters, and other
     * appearance-related resources. Subclasses should call the parent
     * destructor to ensure complete cleanup.
     */
    virtual ~LLAvatarAppearance();

    /**
     * @brief Initializes the avatar appearance system with configuration files.
     * 
     * Loads and parses the avatar configuration XML files that define the
     * skeleton structure, mesh definitions, visual parameters, and texture
     * layers. This must be called once before creating any avatar instances.
     * 
     * @param avatar_file_name Path to the main avatar configuration XML file
     * @param skeleton_file_name Path to the skeleton definition XML file
     */
    static void         initClass(const std::string& avatar_file_name, const std::string& skeleton_file_name);
    
    /**
     * @brief Initializes the avatar appearance system with default files.
     * 
     * Convenience method that calls initClass with default avatar and skeleton
     * file names. Used when standard configuration files are sufficient.
     */
    static void         initClass();
    
    /**
     * @brief Cleans up static avatar appearance data.
     * 
     * Releases shared resources that are initialized once per class, such as
     * skeleton definitions, mesh templates, and visual parameter configurations.
     * Should be called during application shutdown.
     */
    static void         cleanupClass();
    
    /**
     * @brief Initializes this avatar instance after construction.
     * 
     * Performs instance-specific initialization that couldn't be done in the
     * constructor. This includes building the skeleton, loading meshes, and
     * setting up the texture layering system. Must be called after construction
     * and before using the avatar.
     */
    virtual void        initInstance();
    
    S32                 mInitFlags{ 0 };                    /// Flags tracking initialization progress
    
    /**
     * @brief Loads and builds the avatar's skeletal structure.
     * 
     * Creates the joint hierarchy from the parsed skeleton XML data,
     * establishing the bone structure that animations will manipulate.
     * This is a critical step in avatar initialization.
     * 
     * @return true if skeleton loaded successfully, false on error
     */
    virtual bool        loadSkeletonNode();
    
    /**
     * @brief Loads polygon mesh data for avatar body parts.
     * 
     * Creates the geometric meshes that define the avatar's visual appearance,
     * including body parts, clothing attachment points, and collision volumes.
     * 
     * @return true if meshes loaded successfully, false on error
     */
    bool                loadMeshNodes();
    
    /**
     * @brief Loads texture layer definitions for appearance customization.
     * 
     * Sets up the complex texture layering system that enables clothing,
     * skin textures, tattoos, and other appearance modifications to be
     * composited together into final baked textures.
     * 
     * @return true if layer sets loaded successfully, false on error
     */
    bool                loadLayersets();


/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    INHERITED
 **/

    /**
     * @brief Gets a joint by numeric index from the avatar skeleton.
     * 
     * Provides indexed access to joints in the avatar's skeletal hierarchy.
     * This implementation accesses the joint list directly by index for
     * efficient lookup during animation processing.
     * 
     * @param num Zero-based index of the joint to retrieve
     * @return Pointer to the joint at the specified index, nullptr if invalid
     */
    /*virtual*/ LLJoint*        getCharacterJoint(U32 num);

    /**
     * @brief Gets the animation file prefix for avatar motions.
     * 
     * Returns "avatar" as the prefix used when loading animation files
     * from the viewer data directory. This distinguishes avatar animations
     * from other character types.
     * 
     * @return "avatar" string constant
     */
    /*virtual*/ const char*     getAnimationPrefix() { return "avatar"; }
    
    /**
     * @brief Gets the world position of a collision volume attached to a joint.
     * 
     * Calculates the world space position of a collision volume by combining
     * the joint's world transform with the volume's local offset.
     * 
     * @param joint_index Index of the joint the volume is attached to
     * @param volume_offset Local offset of the volume from the joint
     * @return World position of the collision volume
     */
    /*virtual*/ LLVector3       getVolumePos(S32 joint_index, LLVector3& volume_offset);
    
    /**
     * @brief Finds the joint associated with a collision volume ID.
     * 
     * Looks up which joint a collision volume is attached to based on the
     * volume's numeric identifier. Used for physics and collision detection.
     * 
     * @param volume_id Numeric ID of the collision volume
     * @return Pointer to the joint the volume is attached to, nullptr if not found
     */
    /*virtual*/ LLJoint*        findCollisionVolume(S32 volume_id);
    
    /**
     * @brief Gets the numeric ID of a named collision volume.
     * 
     * Looks up the numeric identifier for a collision volume by name.
     * This allows code to reference volumes by meaningful names rather
     * than hardcoded numbers.
     * 
     * @param name Name of the collision volume to find
     * @return Numeric ID of the volume, -1 if not found
     */
    /*virtual*/ S32             getCollisionVolumeID(std::string &name);
    
    /**
     * @brief Gets the polygon mesh for the avatar's head.
     * 
     * Returns the head mesh used for rendering and collision detection.
     * The head mesh includes facial features and is separate from hair
     * and other attachments.
     * 
     * @return Pointer to the head polygon mesh, nullptr if not available
     */
    /*virtual*/ LLPolyMesh*     getHeadMesh();
    
    /**
     * @brief Gets the polygon mesh for the avatar's upper body.
     * 
     * Returns the upper body mesh that includes the torso, arms, and neck.
     * This mesh is used for rendering and as a base for clothing layers.
     * 
     * @return Pointer to the upper body polygon mesh, nullptr if not available
     */
    /*virtual*/ LLPolyMesh*     getUpperBodyMesh();

/**                    Inherited
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/
public:
    /**
     * @brief Checks if this avatar represents the viewer's own agent.
     * 
     * The base implementation always returns false. LLVOAvatarSelf overrides this
     * to return true for the user's own avatar, enabling special behaviors like
     * appearance editing, first-person view, and different update priorities.
     * This distinction is critical for performance - self avatars never use HIDDEN_UPDATE.
     * 
     * @return false in base class, true in self avatar implementations
     */
    virtual bool    isSelf() const { return false; }
    
    /**
     * @brief Checks if the avatar is in a valid, usable state.
     * 
     * Verifies that the avatar has been properly initialized and all critical
     * components (skeleton, meshes, textures) are loaded and functional.
     * 
     * @return true if avatar is valid and ready for use, false otherwise
     */
    virtual bool    isValid() const;
    
    /**
     * @brief Checks if the avatar is using local appearance data.
     * 
     * Determines whether the avatar's appearance is being driven by local
     * customization data (such as during appearance editing) rather than
     * server-provided appearance information.
     * 
     * @return true if using local appearance data, false if using server data
     */
    virtual bool    isUsingLocalAppearance() const = 0;
    
    /**
     * @brief Checks if the avatar is currently in appearance editing mode.
     * 
     * Indicates whether the avatar appearance is actively being edited,
     * which may affect rendering, updates, and user interaction modes.
     * 
     * @return true if appearance is being edited, false otherwise
     */
    virtual bool    isEditingAppearance() const = 0;

    /**
     * @brief Checks if the avatar has been fully constructed and initialized.
     * 
     * The avatar building process is deferred and complex, involving skeleton
     * construction, mesh loading, and texture setup. This method indicates
     * whether that process has completed successfully.
     * 
     * Critical usage: Nearly all avatar operations in LLVOAvatar check this flag
     * first and early-exit if false to prevent operating on incomplete avatars.
     * 
     * @return true if avatar construction is complete, false if still building
     * 
     * @code
     * // Typical usage pattern throughout codebase:
     * if (!mIsBuilt) {
     *     return false; // or appropriate default value
     * }
     * // ... proceed with avatar operations
     * @endcode
     */
    bool isBuilt() const { return mIsBuilt; }


/**                    State
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    SKELETON
 **/

protected:
    /**
     * @brief Creates a new avatar joint instance.
     * 
     * Factory method that subclasses must implement to create appropriate
     * joint objects for their specific avatar type. This allows different
     * avatar implementations to use specialized joint classes.
     * 
     * @return Pointer to newly created avatar joint
     */
    virtual LLAvatarJoint*  createAvatarJoint() = 0;
    
    /**
     * @brief Creates a new avatar joint with a specific joint number.
     * 
     * Factory method for creating joints with a predefined numeric identifier.
     * The joint number is used for efficient lookup and animation targeting.
     * 
     * @param joint_num Numeric identifier for the joint
     * @return Pointer to newly created avatar joint with specified number
     */
    virtual LLAvatarJoint*  createAvatarJoint(S32 joint_num) = 0;
    
    /**
     * @brief Creates a new avatar joint mesh for rendering.
     * 
     * Factory method for creating joint mesh objects that handle the
     * geometric representation and rendering of avatar body parts.
     * 
     * @return Pointer to newly created avatar joint mesh
     */
    virtual LLAvatarJointMesh*  createAvatarJointMesh() = 0;
    
    /**
     * @brief Creates joint name aliases from bone information.
     * 
     * Processes bone information to establish alternative names for joints,
     * allowing animations and other systems to reference joints by multiple
     * names for compatibility and convenience.
     * 
     * @param bone_info Bone information containing alias definitions
     */
    void makeJointAliases(LLAvatarBoneInfo *bone_info);


public:
    /**
     * @brief Gets the distance from pelvis to foot for this avatar.
     * 
     * Returns the vertical distance measurement used for avatar scaling
     * and ground alignment calculations. This value is computed during
     * avatar construction based on the skeleton configuration.
     * 
     * @return Distance from pelvis to foot in world units
     */
    F32                 getPelvisToFoot() const { return mPelvisToFoot; }
    
    /**
     * @brief Gets the root joint of the avatar skeleton.
     * 
     * Returns the top-level joint that serves as the parent for the entire
     * avatar skeletal hierarchy. All avatar joints are descendants of this root.
     * 
     * @return Pointer to the root avatar joint
     */
    /*virtual*/ LLJoint*    getRootJoint() { return mRoot; }

    LLVector3           mHeadOffset{};                      /// Current head position offset
    LLAvatarJoint*      mRoot{ nullptr };                  /// Root joint of the skeleton hierarchy

    typedef std::map<std::string, LLJoint*, std::less<>> joint_map_t;
    joint_map_t         mJointMap;                          /// Fast lookup of joints by name

    typedef std::map<std::string, LLVector3> joint_state_map_t;
    joint_state_map_t mLastBodySizeState;                   /// Previous frame's body size joint states
    joint_state_map_t mCurrBodySizeState;                   /// Current frame's body size joint states
    
    /**
     * @brief Compares joint state maps to detect body size changes.
     * 
     * Analyzes differences between previous and current joint states to
     * determine if the avatar's body proportions have changed, triggering
     * necessary updates to dependent systems.
     * 
     * @param last_state Previous joint state map
     * @param curr_state Current joint state map
     */
    void compareJointStateMaps(joint_state_map_t& last_state,
                               joint_state_map_t& curr_state);
    
    /**
     * @brief Computes the avatar's overall body size measurements.
     * 
     * Calculates body dimensions based on current joint positions and
     * visual parameter weights. This affects avatar bounding boxes,
     * collision detection, and appearance scaling.
     */
    void        computeBodySize();

public:
    typedef std::vector<LLAvatarJoint*> avatar_joint_list_t;
    
    /**
     * @brief Gets the complete list of avatar joints.
     * 
     * Returns a vector containing all joints in the avatar skeleton,
     * providing access to the complete joint hierarchy for iteration
     * and processing operations.
     * 
     * @return Reference to the skeleton joint list
     */
    const avatar_joint_list_t& getSkeleton() { return mSkeleton; }
    
    typedef std::map<std::string, std::string, std::less<>> joint_alias_map_t;
    
    /**
     * @brief Gets the map of joint name aliases.
     * 
     * Returns the mapping of alternative joint names to canonical names,
     * allowing joints to be referenced by multiple names for compatibility
     * with different animation systems and legacy content.
     * 
     * @return Reference to the joint alias mapping
     */
    const joint_alias_map_t& getJointAliases();


protected:
    static bool         parseSkeletonFile(const std::string& filename, LLXmlTree& skeleton_xml_tree);
    virtual void        buildCharacter();
    virtual bool        loadAvatar();

    bool                setupBone(const LLAvatarBoneInfo* info, LLJoint* parent, S32 &current_volume_num, S32 &current_joint_num);
    bool                allocateCharacterJoints(U32 num);
    bool                buildSkeleton(const LLAvatarSkeletonInfo *info);

    void                clearSkeleton();
    bool                mIsBuilt{ false }; // state of deferred character building
    avatar_joint_list_t mSkeleton;
    LLVector3OverrideMap    mPelvisFixups;
    joint_alias_map_t   mJointAliasMap;

    /**
     * @brief Adds a pelvis height adjustment for a specific mesh.
     * 
     * Registers a height offset that should be applied to the avatar's
     * pelvis position when the specified mesh is worn. This compensates
     * for clothing and attachments that change the apparent avatar height.
     * 
     * @param fixup Height adjustment value in world units
     * @param mesh_id UUID of the mesh requiring the adjustment
     */
    void                addPelvisFixup( F32 fixup, const LLUUID& mesh_id );
    
    /**
     * @brief Removes a pelvis height adjustment for a specific mesh.
     * 
     * Unregisters the height offset for the specified mesh, typically
     * called when the mesh is no longer being worn.
     * 
     * @param mesh_id UUID of the mesh to remove adjustment for
     */
    void                removePelvisFixup( const LLUUID& mesh_id );
    
    /**
     * @brief Checks for pelvis height adjustments and retrieves the values.
     * 
     * Determines if any pelvis height adjustments are active and returns
     * both the adjustment value and the mesh ID responsible for it.
     * 
     * @param fixup Output parameter for the adjustment value
     * @param mesh_id Output parameter for the mesh UUID
     * @return true if adjustments exist, false otherwise
     */
    bool                hasPelvisFixup( F32& fixup, LLUUID& mesh_id ) const;
    
    /**
     * @brief Checks for pelvis height adjustments and retrieves the total value.
     * 
     * Determines if any pelvis height adjustments are active and returns
     * the combined adjustment value.
     * 
     * @param fixup Output parameter for the total adjustment value
     * @return true if adjustments exist, false otherwise
     */
    bool                hasPelvisFixup( F32& fixup ) const;

    LLVector3           mBodySize;                          /// Current computed body dimensions
    LLVector3           mAvatarOffset;                      /// Avatar positioning offset
protected:
    F32                 mPelvisToFoot{ 0.f };               /// Distance from pelvis to foot

    /// @name Cached Joint Pointers
    /// Fast access pointers to commonly used joints, cached during skeleton construction
    /// @{
public:
    LLJoint*        mPelvisp{nullptr};                      /// Pelvis joint (avatar center)
    LLJoint*        mTorsop{ nullptr };                     /// Torso joint (spine base)
    LLJoint*        mChestp{ nullptr };                     /// Chest joint (upper torso)
    LLJoint*        mNeckp{ nullptr };                      /// Neck joint (head connection)
    LLJoint*        mHeadp{ nullptr };                      /// Head joint (skull base)
    LLJoint*        mSkullp{ nullptr };                     /// Skull joint (head top)
    LLJoint*        mEyeLeftp{ nullptr };                   /// Left eye joint
    LLJoint*        mEyeRightp{ nullptr };                  /// Right eye joint
    LLJoint*        mHipLeftp{ nullptr };                   /// Left hip joint
    LLJoint*        mHipRightp{ nullptr };                  /// Right hip joint
    LLJoint*        mKneeLeftp{ nullptr };                  /// Left knee joint
    LLJoint*        mKneeRightp{ nullptr };                 /// Right knee joint
    LLJoint*        mAnkleLeftp{ nullptr };                 /// Left ankle joint
    LLJoint*        mAnkleRightp{ nullptr };                /// Right ankle joint
    LLJoint*        mFootLeftp{ nullptr };                  /// Left foot joint
    LLJoint*        mFootRightp{ nullptr };                 /// Right foot joint
    LLJoint*        mWristLeftp{ nullptr };                 /// Left wrist joint
    LLJoint*        mWristRightp{ nullptr };                /// Right wrist joint
    /// @}

    /// @name XML Configuration Data
    /// Shared parsing results from avatar configuration files
    /// @{
protected:
    static LLAvatarSkeletonInfo*                    sAvatarSkeletonInfo;    /// Parsed skeleton structure data
    static LLAvatarXmlInfo*                         sAvatarXmlInfo;         /// Parsed avatar configuration data
    /// @}


/**                    Skeleton
 **                                                                            **
 *******************************************************************************/


/********************************************************************************
 **                                                                            **
 **                    RENDERING
 **/
public:
    bool        mIsDummy{ false };                          /// True for special views and animated object controllers

    /**
     * @brief Adds a morph target that's masked by a texture layer.
     * 
     * Registers a visual parameter that should be selectively applied based
     * on alpha mask data from a texture layer. This enables complex appearance
     * effects like makeup that only affects certain areas of the avatar.
     * 
     * @param index Baked texture index containing the mask data
     * @param morph_target Visual parameter to be masked
     * @param invert Whether to invert the mask (apply where texture is transparent)
     * @param layer Name of the texture layer providing the mask
     */
    void    addMaskedMorph(LLAvatarAppearanceDefines::EBakedTextureIndex index, LLVisualParam* morph_target, bool invert, std::string layer);
    
    /**
     * @brief Applies morph masks using texture data.
     * 
     * Uses texture alpha channel data to selectively apply visual parameter
     * morphs across the avatar surface. This enables localized appearance
     * effects like tattoos, makeup, and detailed customization.
     * 
     * @param tex_data Raw texture data containing mask information
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @param num_components Number of color components per pixel
     * @param index Baked texture index being processed
     */
    virtual void    applyMorphMask(const U8* tex_data, S32 width, S32 height, S32 num_components, LLAvatarAppearanceDefines::EBakedTextureIndex index = LLAvatarAppearanceDefines::BAKED_NUM_INDICES) = 0;

/**                    Rendering
 **                                                                            **
 *******************************************************************************/

    /**
     * @brief Invalidates a texture layer composite, forcing regeneration.
     * 
     * Marks a texture layer set as needing to be re-composited, typically
     * called when underlying texture layers or visual parameters have changed.
     * This triggers the expensive texture baking process on the next update.
     * 
     * Performance warning: This is one of the most expensive operations in the
     * avatar system. Called from visual parameter changes (e.g., in lltexlayerparams.cpp)
     * and can cause significant frame rate drops during appearance editing.
     * 
     * @param layerset The texture layer set to invalidate
     */
    virtual void    invalidateComposite(LLTexLayerSet* layerset) = 0;

/********************************************************************************
 **                                                                            **
 **                    MESHES
 **/

public:
    /**
     * @brief Updates mesh textures after appearance changes.
     * 
     * Applies current texture data to avatar meshes, typically called after
     * texture baking completes or when switching between different levels of detail.
     */
    virtual void    updateMeshTextures() = 0;
    
    /**
     * @brief Marks the avatar mesh as needing regeneration.
     * 
     * Flags the avatar mesh for rebuilding, typically called when visual
     * parameters or other appearance properties change significantly.
     */
    virtual void    dirtyMesh() = 0;
    
    /**
     * @brief Gets the avatar appearance data dictionary.
     * 
     * Returns the shared dictionary containing definitions for texture indices,
     * baked texture mappings, and other appearance-related constants.
     * 
     * @return Pointer to the appearance dictionary
     */
    static const LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary *getDictionary() { return sAvatarDictionary; }
    
protected:
    /**
     * @brief Marks the avatar mesh as needing regeneration with priority.
     * 
     * Flags the avatar mesh for rebuilding with a specified priority level,
     * allowing urgent updates to be processed before less critical ones.
     * 
     * @param priority Priority level for the mesh update
     */
    virtual void    dirtyMesh(S32 priority) = 0;

protected:
    typedef std::multimap<std::string, LLPolyMesh*> polymesh_map_t;
    polymesh_map_t                                  mPolyMeshes;            /// Polygon meshes indexed by name
    avatar_joint_list_t                             mMeshLOD;               /// Joints with level-of-detail meshes

    static LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary* sAvatarDictionary; /// Shared appearance data dictionary

/**                    Meshes
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    APPEARANCE
 **/

    //--------------------------------------------------------------------
    // Clothing colors (convenience functions to access visual parameters)
    //--------------------------------------------------------------------
public:
    void            setClothesColor(LLAvatarAppearanceDefines::ETextureIndex te, const LLColor4& new_color);
    LLColor4        getClothesColor(LLAvatarAppearanceDefines::ETextureIndex te);
    static bool     teToColorParams(LLAvatarAppearanceDefines::ETextureIndex te, U32 *param_name);

    //--------------------------------------------------------------------
    // Global colors
    //--------------------------------------------------------------------
public:
    LLColor4        getGlobalColor(const std::string& color_name ) const;
    virtual void    onGlobalColorChanged(const LLTexGlobalColor* global_color) = 0;
protected:
    LLTexGlobalColor* mTexSkinColor{ nullptr };             /// Global skin color controller
    LLTexGlobalColor* mTexHairColor{ nullptr };             /// Global hair color controller
    LLTexGlobalColor* mTexEyeColor{ nullptr };              /// Global eye color controller

    //--------------------------------------------------------------------
    // Visibility
    //--------------------------------------------------------------------
public:
    static LLColor4 getDummyColor();
/**                    Appearance
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    WEARABLES
 **/

public:
    /**
     * @brief Gets the wearable data container for this avatar.
     * 
     * Returns the wearable data object that manages clothing, accessories,
     * and other customizable appearance items worn by this avatar.
     * 
     * @return Pointer to the wearable data container
     */
    LLWearableData*         getWearableData() { return mWearableData; }
    
    /**
     * @brief Gets the wearable data container (const version).
     * 
     * @return Const pointer to the wearable data container
     */
    const LLWearableData*   getWearableData() const { return mWearableData; }
    
    /**
     * @brief Checks if a texture is defined at a specific index.
     * 
     * Determines whether the avatar has a valid texture assigned for
     * the specified texture slot, used to verify appearance completeness.
     * 
     * @param te Texture index to check
     * @param index Sub-index for layered textures (default 0)
     * @return true if texture is defined, false otherwise
     */
    virtual bool isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex te, U32 index = 0 ) const = 0;
    
    /**
     * @brief Checks if the avatar is wearing a specific type of wearable.
     * 
     * Determines whether the avatar currently has any wearable items of
     * the specified type (shirt, pants, shoes, etc.) equipped.
     * 
     * @param type Type of wearable to check for
     * @return true if wearing that wearable type, false otherwise
     */
    virtual bool            isWearingWearableType(LLWearableType::EType type ) const;

private:
    LLWearableData* mWearableData{ nullptr };               /// Container for wearable items (not owned)

/********************************************************************************
 **                                                                            **
 **                    BAKED TEXTURES
 **/
public:
    /**
     * @brief Gets the texture layer set for a specific baked texture.
     * 
     * Returns the layer set responsible for compositing textures into
     * a specific baked texture channel (head, upper body, lower body, etc.).
     * 
     * @param baked_index Index of the baked texture to get layers for
     * @return Pointer to the texture layer set, nullptr if not found
     */
    LLTexLayerSet*      getAvatarLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;

protected:
    /**
     * @brief Creates a new texture layer set.
     * 
     * Factory method that subclasses must implement to create appropriate
     * texture layer set objects for their specific avatar implementation.
     * 
     * @return Pointer to newly created texture layer set
     */
    virtual LLTexLayerSet*  createTexLayerSet() = 0;

protected:
    class LLMaskedMorph;
    typedef std::deque<LLMaskedMorph *>     morph_list_t;
    
    /**
     * @brief Data structure for managing baked texture information.
     * 
     * Contains all the data needed to track and manage a single baked texture
     * channel, including texture layers, mesh associations, and masked morphs.
     */
    struct BakedTextureData
    {
        LLUUID                              mLastTextureID;         /// UUID of the last baked texture
        LLTexLayerSet*                      mTexLayerSet{ nullptr }; /// Layer set for compositing (self avatar only)
        bool                                mIsLoaded{ false };     /// Whether texture data is loaded
        bool                                mIsUsed{ false };       /// Whether this baked texture is actively used
        LLAvatarAppearanceDefines::ETextureIndex    mTextureIndex{ LLAvatarAppearanceDefines::ETextureIndex::TEX_INVALID }; /// Corresponding texture slot
        U32                                 mMaskTexName{ 0 };      /// OpenGL texture name for mask data
        avatar_joint_mesh_list_t            mJointMeshes;           /// Meshes affected by this baked texture
        morph_list_t                        mMaskedMorphs;          /// Visual parameters masked by this texture
    };
    typedef std::vector<BakedTextureData>   bakedtexturedata_vec_t;
    bakedtexturedata_vec_t                  mBakedTextureDatas;     /// All baked texture data

/********************************************************************************
 **                                                                            **
 **                    PHYSICS
 **/

    /// @name Collision Volumes
    /// Physics collision detection system for avatar interactions
    /// @{
public:
    S32         mNumBones{ 0 };                             /// Total number of bones in skeleton
    S32         mNumCollisionVolumes{ 0 };                  /// Number of collision volumes allocated
    LLAvatarJointCollisionVolume* mCollisionVolumes{ nullptr }; /// Array of collision volumes
    
protected:
    /**
     * @brief Allocates collision volume array for physics detection.
     * 
     * Creates and initializes the collision volume array used for
     * physics interactions, attachment points, and spatial queries.
     * 
     * @param num Number of collision volumes to allocate
     * @return true if allocation succeeded, false on error
     */
    bool        allocateCollisionVolumes(U32 num);
    /// @}

/**                    Physics
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    SUPPORT CLASSES
 **/

    struct LLAvatarXmlInfo
    {
        LLAvatarXmlInfo();
        ~LLAvatarXmlInfo();

        bool    parseXmlSkeletonNode(LLXmlTreeNode* root);
        bool    parseXmlMeshNodes(LLXmlTreeNode* root);
        bool    parseXmlColorNodes(LLXmlTreeNode* root);
        bool    parseXmlLayerNodes(LLXmlTreeNode* root);
        bool    parseXmlDriverNodes(LLXmlTreeNode* root);
        bool    parseXmlMorphNodes(LLXmlTreeNode* root);

        struct LLAvatarMeshInfo
        {
            typedef std::pair<LLViewerVisualParamInfo*,bool> morph_info_pair_t; // LLPolyMorphTargetInfo stored here
            typedef std::vector<morph_info_pair_t> morph_info_list_t;

            LLAvatarMeshInfo() : mLOD(0), mMinPixelArea(.1f) {}
            ~LLAvatarMeshInfo()
            {
                for (morph_info_list_t::value_type& pair : mPolyMorphTargetInfoList)
                {
                    delete pair.first;
                }
                mPolyMorphTargetInfoList.clear();
            }

            std::string mType;
            S32         mLOD;
            std::string mMeshFileName;
            std::string mReferenceMeshName;
            F32         mMinPixelArea;
            morph_info_list_t mPolyMorphTargetInfoList;
        };
        typedef std::vector<LLAvatarMeshInfo*> mesh_info_list_t;
        mesh_info_list_t mMeshInfoList;

        typedef std::vector<LLViewerVisualParamInfo*> skeletal_distortion_info_list_t; // LLPolySkeletalDistortionInfo stored here
        skeletal_distortion_info_list_t mSkeletalDistortionInfoList;

        struct LLAvatarAttachmentInfo
        {
            LLAvatarAttachmentInfo()
                : mGroup(-1), mAttachmentID(-1), mPieMenuSlice(-1), mVisibleFirstPerson(false),
                  mIsHUDAttachment(false), mHasPosition(false), mHasRotation(false) {}
            std::string mName;
            std::string mJointName;
            LLVector3 mPosition;
            LLVector3 mRotationEuler;
            S32 mGroup;
            S32 mAttachmentID;
            S32 mPieMenuSlice;
            bool mVisibleFirstPerson;
            bool mIsHUDAttachment;
            bool mHasPosition;
            bool mHasRotation;
        };
        typedef std::vector<LLAvatarAttachmentInfo*> attachment_info_list_t;
        attachment_info_list_t mAttachmentInfoList;

        LLTexGlobalColorInfo *mTexSkinColorInfo;
        LLTexGlobalColorInfo *mTexHairColorInfo;
        LLTexGlobalColorInfo *mTexEyeColorInfo;

        typedef std::vector<LLTexLayerSetInfo*> layer_info_list_t;
        layer_info_list_t mLayerInfoList;

        typedef std::vector<LLDriverParamInfo*> driver_info_list_t;
        driver_info_list_t mDriverInfoList;

        struct LLAvatarMorphInfo
        {
            LLAvatarMorphInfo()
                : mInvert(false) {}
            std::string mName;
            std::string mRegion;
            std::string mLayer;
            bool mInvert;
        };

        typedef std::vector<LLAvatarMorphInfo*> morph_info_list_t;
        morph_info_list_t   mMorphMaskInfoList;
    };


    class LLMaskedMorph
    {
    public:
        LLMaskedMorph(LLVisualParam *morph_target, bool invert, std::string layer);

        LLVisualParam   *mMorphTarget;
        bool                mInvert;
        std::string         mLayer;
    };
/**                    Support Classes
 **                                                                            **
 *******************************************************************************/
};

#endif // LL_AVATAR_APPEARANCE_H
