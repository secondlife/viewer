/**
 * @file llvisualparam.h
 * @brief Avatar visual parameter system for appearance customization.
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

#ifndef LL_LLVisualParam_H
#define LL_LLVisualParam_H

#include "v3math.h"
#include "llstring.h"
#include "llxmltree.h"
#include <boost/function.hpp>

class LLPolyMesh;
class LLXmlTreeNode;

/**
 * @brief Gender specification for visual parameters with sex-specific effects.
 * 
 * Visual parameters can be applied selectively based on avatar gender,
 * allowing different appearance effects for male and female avatars.
 * Values are designed as bit flags to enable efficient gender checking.
 * 
 * Real-world Usage:
 * Gender checking is performed during parameter application:
 * @code
 * // Only apply parameter if it matches avatar's gender
 * F32 effective_weight = (param->getSex() & mSex) ? 
 *                        param->getWeight() : param->getDefaultWeight();
 * @endcode
 */
enum ESex
{
    SEX_FEMALE =    0x01,  ///< Parameter applies to female avatars only
    SEX_MALE =      0x02,  ///< Parameter applies to male avatars only
    SEX_BOTH =      0x03   ///< Parameter applies to both genders (bitwise OR of above)
};

/**
 * @brief Categories for visual parameters controlling their behavior and network transmission.
 * 
 * Visual parameters are grouped by how they can be modified and whether they need
 * to be synchronized across the network. This affects UI presentation and network
 * efficiency.
 * 
 * Parameter groups control:
 * - Whether users can adjust them via sliders (tweakable)
 * - Whether they animate smoothly over time (animatable)
 * - Whether changes are sent to other viewers (transmit)
 * 
 * Network optimization is critical since visual parameter updates can be frequent
 * and numerous (avatars may have hundreds of parameters).
 */
enum EVisualParamGroup
{
    /// User-adjustable parameters that are transmitted to other viewers
    VISUAL_PARAM_GROUP_TWEAKABLE,
    /// Parameters that change smoothly over time (e.g., animations, morph targets)
    VISUAL_PARAM_GROUP_ANIMATABLE,
    /// User-adjustable parameters kept local (not sent to other viewers)
    VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT,
    /// Legacy parameters that were once tweakable but now deprecated
    VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE,
    /// Total number of parameter groups (must be last)
    NUM_VISUAL_PARAM_GROUPS
};

/**
 * @brief Specifies where a visual parameter's data is stored and managed.
 * 
 * Visual parameters can be stored in different locations depending on their
 * purpose and scope. This affects how they're saved, loaded, and synchronized.
 * 
 * Location tracking enables proper parameter management across different systems:
 * - Avatar parameters for body shape and appearance
 * - Wearable parameters for clothing and attachments
 * - Self vs. other avatar parameter handling
 */
enum EParamLocation
{
    LOC_UNKNOWN,    ///< Parameter location not specified or determined
    LOC_AV_SELF,    ///< Parameter belongs to the user's own avatar
    LOC_AV_OTHER,   ///< Parameter belongs to another user's avatar
    LOC_WEARABLE    ///< Parameter belongs to a wearable item (clothing, etc.)
};

/**
 * @brief Converts parameter location enum to human-readable string.
 * 
 * Utility function for debugging and logging that returns descriptive names
 * for parameter location values.
 * 
 * @param loc Parameter location enum value
 * @return String representation of the location
 */
const std::string param_location_name(const EParamLocation& loc);

/// Maximum number of visual parameters that can be transmitted over the network
const S32 MAX_TRANSMITTED_VISUAL_PARAMS = 255;

/**
 * @brief Shared configuration data for visual parameters.
 * 
 * LLVisualParamInfo contains the static configuration information shared by
 * all instances of a particular visual parameter type. This includes metadata
 * like names, weight ranges, and behavioral settings that don't change per
 * avatar instance.
 * 
 * Design Pattern:
 * Visual parameters use a two-part design:
 * - LLVisualParamInfo: Shared static configuration (this class)
 * - LLVisualParam: Per-avatar instance data (current weight, animation state)
 * 
 * This separation reduces memory usage when many avatars use the same parameter
 * types, as the static configuration is shared rather than duplicated.
 * 
 * Real-world Usage:
 * Parameter info is typically loaded from XML configuration files that define
 * the avatar's visual parameter schema:
 * @code
 * // XML parsing creates shared info objects
 * LLVisualParamInfo* info = new LLPolyMorphTargetInfo();
 * if (info->parseXml(xml_node)) {
 *     // Create parameter instances that reference this shared info
 *     LLVisualParam* param = new LLPolyMorphTarget();
 *     param->setInfo(info);
 * }
 * @endcode
 */
class LLVisualParamInfo
{
    friend class LLVisualParam;
public:
    /**
     * @brief Constructs visual parameter info with default values.
     * 
     * Initializes the info object with safe defaults. Configuration must be
     * loaded via parseXml() or set manually before use.
     */
    LLVisualParamInfo();
    
    /**
     * @brief Virtual destructor for safe inheritance.
     * 
     * Enables proper cleanup when deleting info objects through base pointers.
     */
    virtual ~LLVisualParamInfo() {};

    /**
     * @brief Parses visual parameter configuration from XML.
     * 
     * Loads parameter metadata from XML configuration files, including names,
     * weight ranges, gender restrictions, and UI grouping information.
     * 
     * @param node XML node containing parameter configuration
     * @return true if parsing succeeded, false on error
     */
    virtual bool parseXml(LLXmlTreeNode *node);

    /**
     * @brief Gets the unique identifier for this parameter type.
     * 
     * Returns the numeric ID that uniquely identifies this parameter type
     * across the system. Used for parameter lookup and network transmission.
     * 
     * @return Unique parameter ID
     */
    S32 getID() const { return mID; }

    /**
     * @brief Outputs parameter info to a stream for debugging.
     * 
     * Writes human-readable parameter information to the specified output
     * stream for debugging and diagnostic purposes.
     * 
     * @param out Output stream to write to
     */
    virtual void toStream(std::ostream &out);

protected:
    /// Unique identifier for this parameter type
    S32                 mID;

    /// Internal name used for code references and debugging
    std::string         mName;
    /// User-friendly name displayed in UI elements
    std::string         mDisplayName;
    /// Label shown for minimum slider position
    std::string         mMinName;
    /// Label shown for maximum slider position
    std::string         mMaxName;
    /// Parameter group controlling UI organization and behavior
    EVisualParamGroup   mGroup;
    /// Minimum weight value this parameter can have
    F32                 mMinWeight;
    /// Maximum weight value this parameter can have
    F32                 mMaxWeight;
    /// Default weight value when parameter is reset
    F32                 mDefaultWeight;
    /// Gender(s) this parameter applies to (bit flags)
    ESex                mSex;
};

/**
 * @brief Abstract base class for parametric modifications of avatar appearance.
 * 
 * LLVisualParam represents a single adjustable aspect of avatar appearance,
 * such as body shape, facial features, or clothing characteristics. Each parameter
 * has a weight value that controls the intensity of its effect on the avatar.
 * 
 * Key Concepts:
 * - **Weight**: Numeric value (typically 0.0-1.0) controlling parameter intensity
 * - **Animation**: Smooth interpolation between weight values over time
 * - **Chaining**: Parameters can be linked to drive related parameters
 * - **Gender**: Parameters can be restricted to specific avatar genders
 * 
 * Architecture:
 * Visual parameters use a shared info pattern where LLVisualParamInfo contains
 * static configuration data, while LLVisualParam instances hold per-avatar state.
 * This reduces memory usage when many avatars share the same parameter types.
 * 
 * Real-world Usage:
 * Parameters are extensively used throughout the avatar system:
 * @code
 * // Typical parameter weight setting from wearables
 * param->setWeight(desired_value);
 * mSavedVisualParamMap[id] = param->getWeight();
 * 
 * // Gender-aware parameter application
 * F32 effective_weight = (param->getSex() & avatar_sex) ? 
 *                        param->getWeight() : param->getDefaultWeight();
 * if (effective_weight != param->getLastWeight()) {
 *     param->apply(avatar_sex);
 *     param->setLastWeight(effective_weight);
 * }
 * 
 * // Parameter animation for smooth transitions
 * param->setAnimationTarget(target_weight);
 * param->animate(time_delta);
 * @endcode
 * 
 * Performance Considerations:
 * - 16-byte alignment for SIMD optimization in math operations
 * - Last weight caching to avoid redundant apply() calls
 * - Gender filtering to skip inapplicable parameters
 * - Animation system to smooth visual transitions
 * 
 * Thread Safety: Not thread-safe, must be used from main rendering thread.
 */
LL_ALIGN_PREFIX(16)
class LLVisualParam
{
public:
    /// Function type for mapping parameter IDs to parameter instances
    typedef boost::function<LLVisualParam*(S32)> visual_param_mapper;

    /**
     * @brief Constructs a visual parameter with default values.
     * 
     * Creates an uninitialized parameter. Configuration must be set via
     * setInfo() before the parameter can be used.
     */
    LLVisualParam();
    
    /**
     * @brief Virtual destructor for safe inheritance.
     * 
     * Ensures proper cleanup of derived parameter types when deleted
     * through base class pointers.
     */
    virtual ~LLVisualParam();

    /**
     * @brief Gets the shared configuration info for this parameter.
     * 
     * Returns the LLVisualParamInfo object containing static configuration
     * data like names, ranges, and behavioral settings.
     * 
     * Note: Not virtual because derived classes use specific info types.
     * 
     * @return Pointer to parameter configuration info
     */
    LLVisualParamInfo*      getInfo() const { return mInfo; }
    
    /**
     * @brief Sets the configuration info and initializes the parameter.
     * 
     * Associates this parameter instance with shared configuration data
     * and calls initialization functions. Must be called before use.
     * 
     * @param info Pointer to parameter configuration info
     * @return true if initialization succeeded
     */
    bool                    setInfo(LLVisualParamInfo *info);

    /**
     * @brief Applies this parameter's effect to the avatar.
     * 
     * Pure virtual method that must be implemented by concrete parameter types
     * to define how the parameter affects avatar appearance. Called when the
     * parameter's effective weight changes.
     * 
     * @param avatar_sex Gender of avatar for gender-specific effects
     */
    virtual void            apply( ESex avatar_sex ) = 0;
    
    /**
     * @brief Sets the parameter weight value.
     * 
     * Updates the current weight, handling animation state and linked parameters.
     * Weight changes trigger recalculation of avatar appearance.
     * 
     * Real usage pattern from wearable system:
     * @code
     * param->setWeight(value);
     * saved_params[id] = param->getWeight();  // Cache for persistence
     * @endcode
     * 
     * @param weight New weight value (typically 0.0 to 1.0)
     */
    virtual void            setWeight(F32 weight);
    
    /**
     * @brief Sets target weight for smooth animation.
     * 
     * Initiates animation toward the target weight value. The parameter will
     * smoothly interpolate from current weight to target over time.
     * 
     * @param target_value Weight value to animate toward
     */
    virtual void            setAnimationTarget( F32 target_value);
    
    /**
     * @brief Updates animation progress by time delta.
     * 
     * Advances the parameter's animation state by the specified time amount.
     * Should be called each frame during active animation.
     * 
     * @param delta Time elapsed since last animation update
     */
    virtual void            animate(F32 delta);
    
    /**
     * @brief Stops any active animation and settles at current weight.
     */
    virtual void            stopAnimating();

    /**
     * @brief Links this parameter to drive other dependent parameters.
     * 
     * Establishes relationships where changes to this parameter automatically
     * affect related parameters. Used for complex appearance effects.
     * 
     * @param mapper Function to look up parameters by ID
     * @param only_cross_params If true, only link cross-wearable parameters
     * @return true if linking succeeded
     */
    virtual bool            linkDrivenParams(visual_param_mapper mapper, bool only_cross_params);
    
    /**
     * @brief Resets all parameters driven by this parameter.
     */
    virtual void            resetDrivenParams();

    /**
     * @brief Gets the unique identifier for this parameter.
     * 
     * @return Parameter ID used for lookup and network transmission
     */
    S32                     getID() const       { return mID; }
    
    /**
     * @brief Sets the parameter ID (only valid before info is set).
     * 
     * @param id New parameter ID
     */
    void                    setID(S32 id)       { llassert(!mInfo); mID = id; }

    /**
     * @brief Gets the internal name of this parameter.
     * 
     * @return Internal parameter name for code references
     */ 
    const std::string&      getName() const             { return mInfo->mName; }
    
    /**
     * @brief Gets the user-friendly display name.
     * 
     * @return Display name shown in UI elements
     */
    const std::string&      getDisplayName() const      { return mInfo->mDisplayName; }
    
    /**
     * @brief Gets the label for maximum slider position.
     * 
     * @return Label displayed at slider maximum
     */
    const std::string&      getMaxDisplayName() const   { return mInfo->mMaxName; }
    
    /**
     * @brief Gets the label for minimum slider position.
     * 
     * @return Label displayed at slider minimum
     */
    const std::string&      getMinDisplayName() const   { return mInfo->mMinName; }

    /**
     * @brief Sets the user-friendly display name.
     * 
     * @param s New display name
     */
    void                    setDisplayName(const std::string& s)     { mInfo->mDisplayName = s; }
    
    /**
     * @brief Sets the maximum slider label.
     * 
     * @param s New maximum label
     */
    void                    setMaxDisplayName(const std::string& s) { mInfo->mMaxName = s; }
    
    /**
     * @brief Sets the minimum slider label.
     * 
     * @param s New minimum label
     */
    void                    setMinDisplayName(const std::string& s) { mInfo->mMinName = s; }

    /**
     * @brief Gets the parameter group for UI organization.
     * 
     * @return Parameter group enum value
     */
    EVisualParamGroup       getGroup() const            { return mInfo->mGroup; }
    
    /**
     * @brief Gets the minimum allowed weight value.
     * 
     * @return Minimum weight (typically 0.0)
     */
    F32                     getMinWeight() const        { return mInfo->mMinWeight; }
    
    /**
     * @brief Gets the maximum allowed weight value.
     * 
     * @return Maximum weight (typically 1.0)
     */
    F32                     getMaxWeight() const        { return mInfo->mMaxWeight; }
    
    /**
     * @brief Gets the default weight for reset operations.
     * 
     * @return Default weight value
     */
    F32                     getDefaultWeight() const    { return mInfo->mDefaultWeight; }
    
    /**
     * @brief Gets the gender applicability of this parameter.
     * 
     * @return Gender flags indicating which sexes this parameter affects
     */
    ESex                    getSex() const          { return mInfo->mSex; }

    /**
     * @brief Gets the effective weight value.
     * 
     * Returns target weight if animating, otherwise current weight.
     * This is the weight value that should be used for UI display.
     * 
     * @return Effective parameter weight
     */
    F32                     getWeight() const       { return mIsAnimating ? mTargetWeight : mCurWeight; }
    
    /**
     * @brief Gets the current actual weight value.
     * 
     * Returns the current weight regardless of animation state.
     * Used internally for animation calculations.
     * 
     * @return Current weight value
     */
    F32                     getCurrentWeight() const    { return mCurWeight; }
    
    /**
     * @brief Gets the last applied weight value.
     * 
     * Used to detect weight changes and avoid redundant apply() calls.
     * Critical performance optimization for avatar rendering.
     * 
     * @return Last weight that was applied
     */
    F32                     getLastWeight() const   { return mLastWeight; }
    
    /**
     * @brief Sets the last applied weight value.
     * 
     * Updates the cached last weight to match current state.
     * Called after successful apply() operations.
     * 
     * @param val New last weight value
     */
    void                    setLastWeight(F32 val) { mLastWeight = val; }
    
    /**
     * @brief Checks if parameter is currently animating.
     * 
     * @return true if parameter is smoothly transitioning between weights
     */
    bool                    isAnimating() const { return mIsAnimating; }
    
    /**
     * @brief Checks if parameter can be adjusted by users.
     * 
     * Returns true for parameters that should appear in UI sliders.
     * Based on parameter group classification.
     * 
     * @return true if parameter is user-tweakable
     */
    bool                    isTweakable() const { return (getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)  || (getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT); }

    /**
     * @brief Gets the next parameter in a linked chain.
     * 
     * @return Pointer to next chained parameter, nullptr if none
     */
    LLVisualParam*          getNextParam()      { return mNext; }
    
    /**
     * @brief Sets the next parameter in a linked chain.
     * 
     * @param next Pointer to parameter to chain after this one
     */
    void                    setNextParam( LLVisualParam *next );
    
    /**
     * @brief Removes this parameter from any linked chain.
     */
    void                    clearNextParam();

    /**
     * @brief Enables or disables animation for this parameter.
     * 
     * @param is_animating If true, enables animation; if false, disables
     */
    virtual void            setAnimating(bool is_animating) { mIsAnimating = is_animating && !mIsDummy; }
    
    /**
     * @brief Checks if parameter animation is enabled.
     * 
     * @return true if animation is enabled
     */
    bool                    getAnimating() const { return mIsAnimating; }

    /**
     * @brief Marks parameter as dummy (non-functional).
     * 
     * Dummy parameters don't animate or affect appearance. Used for
     * placeholder parameters in certain configurations.
     * 
     * @param is_dummy If true, marks as dummy parameter
     */
    void                    setIsDummy(bool is_dummy) { mIsDummy = is_dummy; }

    /**
     * @brief Sets where this parameter's data is stored.
     * 
     * @param loc Parameter location (avatar, wearable, etc.)
     */
    void                    setParamLocation(EParamLocation loc);
    
    /**
     * @brief Gets where this parameter's data is stored.
     * 
     * @return Parameter location enum value
     */
    EParamLocation          getParamLocation() const { return mParamLocation; }

protected:
    /**
     * @brief Copy constructor marked protected to control copying.
     * 
     * Visual parameters have complex state and relationships that make
     * copying semantically unclear. Use explicit methods when needed.
     */
    LLVisualParam(const LLVisualParam& pOther);

    /// Current weight value affecting avatar appearance
    F32                 mCurWeight;
    /// Last weight value that was applied (for change detection)
    F32                 mLastWeight;
    /// Next parameter in linked chain (for related parameters)
    LLVisualParam*      mNext;
    /// Target weight for animation interpolation
    F32                 mTargetWeight;
    /// Flag indicating if parameter is currently animating
    bool                mIsAnimating;
    /// Flag indicating if this is a dummy (non-functional) parameter
    bool                mIsDummy;

    /// Unique identifier for this parameter type
    S32                 mID;
    /// Shared configuration info for this parameter type
    LLVisualParamInfo   *mInfo;
    /// Where this parameter's data is stored and managed
    EParamLocation      mParamLocation;
} LL_ALIGN_POSTFIX(16);

#endif // LL_LLVISUALPARAM_H
