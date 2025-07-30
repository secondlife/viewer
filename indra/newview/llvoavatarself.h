/**
 * @file llvoavatarself.h
 * @brief Declaration of LLVOAvatar class which is a derivation fo
 * LLViewerObject
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

#ifndef LL_LLVOAVATARSELF_H
#define LL_LLVOAVATARSELF_H

#include "llviewertexture.h"
#include "llvoavatar.h"
#include <map>
#include "lleventcoro.h"
#include "llcoros.h"

struct LocalTextureData;
class LLInventoryCallback;


/**
 * @brief Specialized avatar implementation for the viewer's own agent.
 * 
 * LLVOAvatarSelf extends LLVOAvatar with additional functionality specific to
 * the user's own avatar, including:
 * - **Appearance Editing**: Real-time appearance customization and preview
 * - **First-Person Integration**: Camera attachment and view management
 * - **Attachment Management**: Enhanced attachment loading and positioning
 * - **Performance Optimization**: Never uses HIDDEN_UPDATE (always visible to self)
 * - **Texture Baking**: Local texture baking and upload capabilities
 * - **Wearables Management**: Direct integration with inventory and appearance panels
 * 
 * This class represents the single instance of the user's avatar in the world.
 * It has special privileges and behaviors not available to other avatars:
 * - Always considered "self" (isSelf() returns true)
 * - Receives priority updates and full processing
 * - Can edit appearance in real-time
 * - Has access to local texture baking
 * - Integrates with the agent's movement and camera systems
 * 
 * Key differences from LLVOAvatar:
 * - Never uses HIDDEN_UPDATE optimization (always fully processed)
 * - Has additional methods for appearance editing and baking
 * - Manages the connection between avatar appearance and user interface
 * - Handles attachment notifications and updates differently
 * - Provides local texture layer compositing
 * 
 * Performance considerations:
 * - Always receives full updates regardless of camera position
 * - Texture baking operations are more complex due to local processing
 * - Additional overhead for appearance editing and real-time preview
 * - Priority scheduling ensures smooth user experience
 * 
 * Usage: There is exactly one instance of this class per viewer session,
 * created when the agent enters the world and destroyed on logout.
 */
class LLVOAvatarSelf :
    public LLVOAvatar
{
    LOG_CLASS(LLVOAvatarSelf);

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/

public:
    /**
     * @brief Constructs the user's own avatar.
     * 
     * Creates the special self avatar instance with enhanced capabilities
     * for appearance editing, texture baking, and first-person integration.
     * 
     * @param id UUID of the user's agent
     * @param pcode Should be LL_PCODE_LEGACY_AVATAR
     * @param regionp Region where the avatar is being created
     */
    LLVOAvatarSelf(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
    
    /**
     * @brief Destroys the self avatar and cleans up special resources.
     * 
     * Performs cleanup specific to the self avatar, including appearance
     * editing state, texture layers, and UI connections.
     */
    virtual                 ~LLVOAvatarSelf();
    
    /**
     * @brief Marks the self avatar as dead (usually during logout).
     * 
     * Initiates shutdown of self-specific systems including appearance
     * editing, texture baking, and attachment management.
     */
    virtual void            markDead();
    
    /**
     * @brief Initializes the self avatar with enhanced capabilities.
     * 
     * Extends the base avatar initialization with self-specific features
     * including appearance editing setup, local texture baking, and
     * attachment management systems.
     */
    virtual void            initInstance();
    
    /**
     * @brief Performs cleanup of self-avatar specific resources.
     * 
     * Cleans up appearance editing state, texture layers, and other
     * resources specific to the self avatar before destruction.
     */
    void                    cleanup();
    
protected:
    /**
     * @brief Loads avatar data with self-specific enhancements.
     * 
     * Extends base avatar loading with additional functionality for
     * appearance editing and local texture baking capabilities.
     * 
     * @return true if loading successful, false on error
     */
    /*virtual*/ bool        loadAvatar();
    
    /**
     * @brief Loads self-specific avatar components.
     * 
     * Handles loading of appearance editing interfaces, texture baking
     * systems, and other components unique to the self avatar.
     * 
     * @return true if self-loading successful, false on error
     */
    bool                    loadAvatarSelf();
    
    /**
     * @brief Builds skeleton with self-specific enhancements.
     * 
     * Creates the avatar skeleton with additional features needed for
     * appearance editing and attachment management.
     * 
     * @param info Skeleton configuration information
     * @return true if skeleton built successfully, false on error
     */
    bool                    buildSkeletonSelf(const LLAvatarSkeletonInfo *info);
    
    /**
     * @brief Builds avatar-specific context menus and UI elements.
     * 
     * Sets up the context menus and interface elements specific to
     * the self avatar, including appearance editing options.
     * 
     * @return true if menus built successfully, false on error
     */
    bool                    buildMenus();

/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    INHERITED
 **/

    //--------------------------------------------------------------------
    // LLViewerObject interface and related
    //--------------------------------------------------------------------
public:
    boost::signals2::connection                   mRegionChangedSlot;

    void                    onSimulatorFeaturesReceived(const LLUUID& region_id);
    /*virtual*/ void        updateRegion(LLViewerRegion *regionp);
    /*virtual*/ void        idleUpdate(LLAgent &agent, const F64 &time);

    //--------------------------------------------------------------------
    // LLCharacter interface and related
    //--------------------------------------------------------------------
public:
    /*virtual*/ bool        hasMotionFromSource(const LLUUID& source_id);
    /*virtual*/ void        stopMotionFromSource(const LLUUID& source_id);
    /*virtual*/ void        requestStopMotion(LLMotion* motion);
    /*virtual*/ LLJoint*    getJoint(std::string_view name);

    /*virtual*/ void renderJoints();

    /*virtual*/ bool setVisualParamWeight(const LLVisualParam *which_param, F32 weight);
    /*virtual*/ bool setVisualParamWeight(const char* param_name, F32 weight);
    /*virtual*/ bool setVisualParamWeight(S32 index, F32 weight);
    /*virtual*/ void updateVisualParams();
    void writeWearablesToAvatar();
    /*virtual*/ void idleUpdateAppearanceAnimation();

private:
    // helper function. Passed in param is assumed to be in avatar's parameter list.
    bool setParamWeight(const LLViewerVisualParam *param, F32 weight);

    std::mutex          mJointMapMutex; // getJoint gets used from mesh thread

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/

public:
    /**
     * @brief Always returns true - this is the user's own avatar.
     * 
     * Overrides the base implementation to identify this as the self avatar.
     * This enables special behaviors throughout the codebase:
     * - Never uses HIDDEN_UPDATE (always gets full processing)
     * - Has access to appearance editing features
     * - Receives priority in update scheduling
     * - Can perform local texture baking
     * - Integrates with first-person camera and movement
     * 
     * @return true always (this is the self avatar)
     */
    /*virtual*/ bool    isSelf() const { return true; }
    
    /**
     * @brief Returns false - self avatar is not considered a "buddy".
     * 
     * @return false always
     */
    virtual bool    isBuddy() const { return false; }
    
    /**
     * @brief Enhanced validity check for the self avatar.
     * 
     * Performs more comprehensive validation than the base class,
     * checking self-specific systems and states. Consider using
     * isAgentAvatarValid() for the most complete validation.
     * 
     * @return true if self avatar is in a valid, usable state
     */
    /*virtual*/ bool    isValid() const;

    //--------------------------------------------------------------------
    // Updates
    //--------------------------------------------------------------------
public:
    /*virtual*/ bool    updateCharacter(LLAgent &agent);
    /*virtual*/ void    idleUpdateTractorBeam();
    bool                checkStuckAppearance();

    //--------------------------------------------------------------------
    // Loading state
    //--------------------------------------------------------------------
public:
    /*virtual*/ bool    getHasMissingParts() const;

    //--------------------------------------------------------------------
    // Region state
    //--------------------------------------------------------------------
    void            resetRegionCrossingTimer()  { mRegionCrossingTimer.reset(); }

private:
    U64             mLastRegionHandle;
    LLFrameTimer    mRegionCrossingTimer;
    S32             mRegionCrossingCount;

/**                    State
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    RENDERING
 **/

    //--------------------------------------------------------------------
    // Render beam
    //--------------------------------------------------------------------
protected:
    bool        needsRenderBeam();
private:
    LLPointer<LLHUDEffectSpiral> mBeam;
    LLFrameTimer mBeamTimer;

    //--------------------------------------------------------------------
    // LLVOAvatar Constants
    //--------------------------------------------------------------------
public:
    /*virtual*/ LLViewerTexture::EBoostLevel    getAvatarBoostLevel() const { return LLGLTexture::BOOST_AVATAR_SELF; }
    /*virtual*/ LLViewerTexture::EBoostLevel    getAvatarBakedBoostLevel() const { return LLGLTexture::BOOST_AVATAR_BAKED_SELF; }
    /*virtual*/ S32                         getTexImageSize() const { return LLVOAvatar::getTexImageSize()*4; }

/**                    Rendering
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    TEXTURES
 **/

    //--------------------------------------------------------------------
    // Loading status
    //--------------------------------------------------------------------
public:
    S32                 getLocalDiscardLevel(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
    bool                areTexturesCurrent() const;
    bool                isLocalTextureDataAvailable(const LLViewerTexLayerSet* layerset) const;
    bool                isLocalTextureDataFinal(const LLViewerTexLayerSet* layerset) const;
    // If you want to check all textures of a given type, pass gAgentWearables.getWearableCount() for index
    /*virtual*/ bool    isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
    /*virtual*/ bool    isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, U32 index = 0) const;
    /*virtual*/ bool    isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerWearable *wearable) const;


    //--------------------------------------------------------------------
    // Local Textures
    //--------------------------------------------------------------------
public:
    bool                getLocalTextureGL(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerTexture** image_gl_pp, U32 index) const;
    LLViewerFetchedTexture* getLocalTextureGL(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
    const LLUUID&       getLocalTextureID(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
    void                setLocalTextureTE(U8 te, LLViewerTexture* image, U32 index);
    /*virtual*/ void    setLocalTexture(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerTexture* tex, bool baked_version_exits, U32 index);
protected:
    /*virtual*/ void    setBakedReady(LLAvatarAppearanceDefines::ETextureIndex type, bool baked_version_exists, U32 index);
    void                localTextureLoaded(bool succcess, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata);
    void                getLocalTextureByteCount(S32* gl_byte_count) const;
    /*virtual*/ void    addLocalTextureStats(LLAvatarAppearanceDefines::ETextureIndex i, LLViewerFetchedTexture* imagep, F32 texel_area_ratio, bool rendered, bool covered_by_baked);
    LLLocalTextureObject* getLocalTextureObject(LLAvatarAppearanceDefines::ETextureIndex i, U32 index) const;

private:
    static void         onLocalTextureLoaded(bool succcess, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata);

    /*virtual*/ void    setImage(const U8 te, LLViewerTexture *imagep, const U32 index);
    /*virtual*/ LLViewerTexture* getImage(const U8 te, const U32 index) const;


    //--------------------------------------------------------------------
    // Baked textures
    //--------------------------------------------------------------------
public:
    LLAvatarAppearanceDefines::ETextureIndex getBakedTE(const LLViewerTexLayerSet* layerset ) const;
    // SUNSHINE CLEANUP - dead? or update to just call request appearance update?
    void                forceBakeAllTextures(bool slam_for_debug = false);
protected:
    /*virtual*/ void    removeMissingBakedTextures();

    //--------------------------------------------------------------------
    // Layers
    //--------------------------------------------------------------------
public:
    void                requestLayerSetUpdate(LLAvatarAppearanceDefines::ETextureIndex i);
    LLViewerTexLayerSet* getLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;
    LLViewerTexLayerSet* getLayerSet(LLAvatarAppearanceDefines::ETextureIndex index) const;


    //--------------------------------------------------------------------
    // Composites
    //--------------------------------------------------------------------
public:
    /* virtual */ void  invalidateComposite(LLTexLayerSet* layerset);
    /* virtual */ void  invalidateAll();
    /* virtual */ void  setCompositeUpdatesEnabled(bool b); // only works for self
    /* virtual */ void  setCompositeUpdatesEnabled(U32 index, bool b);
    /* virtual */ bool  isCompositeUpdateEnabled(U32 index);
    void                setupComposites();
    void                updateComposites();

    const LLUUID&       grabBakedTexture(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;
    bool                canGrabBakedTexture(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;


    //--------------------------------------------------------------------
    // Scratch textures (used for compositing)
    //--------------------------------------------------------------------
public:
    static void     deleteScratchTextures();
private:
    static S32Bytes sScratchTexBytes;
    static std::map< LLGLenum, LLGLuint*> sScratchTexNames;

/**                    Textures
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    MESHES
 **/
protected:
    /*virtual*/ void   restoreMeshData();

/**                    Meshes
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    WEARABLES
 **/

public:
    void                wearableUpdated(LLWearableType::EType type);
protected:
    U32 getNumWearables(LLAvatarAppearanceDefines::ETextureIndex i) const;

    //--------------------------------------------------------------------
    // Attachments
    //--------------------------------------------------------------------
public:
    void                updateAttachmentVisibility(U32 camera_mode);
    bool                isWearingAttachment(const LLUUID& inv_item_id) const;
    LLViewerObject*     getWornAttachment(const LLUUID& inv_item_id);
    bool                getAttachedPointName(const LLUUID& inv_item_id, std::string& name) const;
    /*virtual*/ const LLViewerJointAttachment *attachObject(LLViewerObject *viewer_object);
    /*virtual*/ bool    detachObject(LLViewerObject *viewer_object);
    static bool         detachAttachmentIntoInventory(const LLUUID& item_id);

    bool hasAttachmentsInTrash();

    //--------------------------------------------------------------------
    // HUDs
    //--------------------------------------------------------------------
private:
    LLViewerJoint*      mScreenp; // special purpose joint for HUD attachments

/**                    Attachments
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    APPEARANCE
 **/

public:
    static void     onCustomizeStart(bool disable_camera_switch = false);
    static void     onCustomizeEnd(bool disable_camera_switch = false);
    LLPointer<LLInventoryCallback> mEndCustomizeCallback;

    //--------------------------------------------------------------------
    // Visibility
    //--------------------------------------------------------------------

    /* virtual */ bool shouldRenderRigged() const;

public:
    bool            sendAppearanceMessage(LLMessageSystem *mesgsys) const;

    // -- care and feeding of hover height.
    void            setHoverIfRegionEnabled();
    void            sendHoverHeight() const;
    /*virtual*/ void setHoverOffset(const LLVector3& hover_offset, bool send_update=true);

private:
    mutable LLVector3 mLastHoverOffsetSent;

/**                    Appearance
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    DIAGNOSTICS
 **/

    //--------------------------------------------------------------------
    // General
    //--------------------------------------------------------------------
public:
    static void     dumpTotalLocalTextureByteCount();
    void            dumpLocalTextures() const;
    static void     dumpScratchTextureByteCount();
    void            dumpWearableInfo(LLAPRFile& outfile);

    //--------------------------------------------------------------------
    // Avatar Rez Metrics
    //--------------------------------------------------------------------
public:
    struct LLAvatarTexData
    {
        LLAvatarTexData(const LLUUID& id, LLAvatarAppearanceDefines::ETextureIndex index) :
            mAvatarID(id),
            mIndex(index)
        {}
        LLUUID          mAvatarID;
        LLAvatarAppearanceDefines::ETextureIndex    mIndex;
    };

    LLTimer                 mTimeSinceLastRezMessage;
    bool                    updateAvatarRezMetrics(bool force_send);

    std::vector<LLSD>       mPendingTimerRecords;
    void                    addMetricsTimerRecord(const LLSD& record);

    void                    debugWearablesLoaded() { mDebugTimeWearablesLoaded = mDebugSelfLoadTimer.getElapsedTimeF32(); }
    void                    debugAvatarVisible() { mDebugTimeAvatarVisible = mDebugSelfLoadTimer.getElapsedTimeF32(); }
    void                    outputRezDiagnostics() const;
    void                    outputRezTiming(const std::string& msg) const;
    void                    reportAvatarRezTime() const;
    void                    debugBakedTextureUpload(LLAvatarAppearanceDefines::EBakedTextureIndex index, bool finished);
    static void             debugOnTimingLocalTexLoaded(bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata);

    bool                    isAllLocalTextureDataFinal() const;

    const LLViewerTexLayerSet*  debugGetLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex index) const { return (LLViewerTexLayerSet*)(mBakedTextureDatas[index].mTexLayerSet); }
    const std::string       verboseDebugDumpLocalTextureDataInfo(const LLViewerTexLayerSet* layerset) const; // Lists out state of this particular baked texture layer
    void                    dumpAllTextures() const;
    const std::string       debugDumpLocalTextureDataInfo(const LLViewerTexLayerSet* layerset) const; // Lists out state of this particular baked texture layer
    const std::string       debugDumpAllLocalTextureDataInfo() const; // Lists out which baked textures are at highest LOD
    void                    sendViewerAppearanceChangeMetrics(); // send data associated with completing a change.
private:
    LLFrameTimer            mDebugSelfLoadTimer;
    F32                     mDebugTimeWearablesLoaded;
    F32                     mDebugTimeAvatarVisible;
    F32                     mDebugTextureLoadTimes[LLAvatarAppearanceDefines::TEX_NUM_INDICES][MAX_DISCARD_LEVEL+1]; // load time for each texture at each discard level
    F32                     mDebugBakedTextureTimes[LLAvatarAppearanceDefines::BAKED_NUM_INDICES][2]; // time to start upload and finish upload of each baked texture
    void                    debugTimingLocalTexLoaded(bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata);

    void                    appearanceChangeMetricsCoro(std::string url);
    bool                    mInitialMetric;
    S32                     mMetricSequence;
/**                    Diagnostics
 **                                                                            **
 *******************************************************************************/

};

extern LLPointer<LLVOAvatarSelf> gAgentAvatarp;

bool isAgentAvatarValid();

void selfStartPhase(const std::string& phase_name);
void selfStopPhase(const std::string& phase_name, bool err_check = true);
void selfClearPhases();

#endif // LL_VO_AVATARSELF_H
