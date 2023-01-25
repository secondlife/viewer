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


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLVOAvatarSelf
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLVOAvatarSelf :
	public LLVOAvatar
{
	LOG_CLASS(LLVOAvatarSelf);

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/

public:
	LLVOAvatarSelf(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual 				~LLVOAvatarSelf();
	void			        markDead() override;
	void 			        initInstance() override; // Called after construction to initialize the class.
	void					cleanup();
protected:
	BOOL		            loadAvatar() override;
	BOOL					loadAvatarSelf();
	BOOL					buildSkeletonSelf(const LLAvatarSkeletonInfo *info);
	BOOL					buildMenus();

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

	void					onSimulatorFeaturesReceived(const LLUUID& region_id);
	void 		            updateRegion(LLViewerRegion *regionp) override;
	void   	 	            idleUpdate(LLAgent &agent, const F64 &time) override;

	//--------------------------------------------------------------------
	// LLCharacter interface and related
	//--------------------------------------------------------------------
public:
	bool 		            hasMotionFromSource(const LLUUID& source_id) override;
	void 		            stopMotionFromSource(const LLUUID& source_id) override;
	void 		            requestStopMotion(const LLMotion::ptr_t & motion) override;
	LLJoint*	            getJoint(const std::string &name) override;
	
	BOOL                    setVisualParamWeight(const LLVisualParam *which_param, F32 weight) override;
	BOOL                    setVisualParamWeight(const char* param_name, F32 weight) override;
	BOOL                    setVisualParamWeight(S32 index, F32 weight) override;
	void                    updateVisualParams() override;
	void                    writeWearablesToAvatar();
	void                    idleUpdateAppearanceAnimation() override;

private:
	// helper function. Passed in param is assumed to be in avatar's parameter list.
	BOOL                    setParamWeight(const LLViewerVisualParam *param, F32 weight);

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/

public:
    bool 	                isSelf() const override { return true; }
    BOOL	                isValid() const override;

	//--------------------------------------------------------------------
	// Updates
	//--------------------------------------------------------------------
public:
	bool 	            updateCharacter(LLAgent &agent) override;
	void 	            idleUpdateTractorBeam();
	bool				checkStuckAppearance();

	//--------------------------------------------------------------------
	// Loading state
	//--------------------------------------------------------------------
public:
	bool                getIsCloud() const override;

	//--------------------------------------------------------------------
	// Region state
	//--------------------------------------------------------------------
	void			    resetRegionCrossingTimer()	{ mRegionCrossingTimer.reset();	}

private:
	U64				mLastRegionHandle;
	LLFrameTimer	mRegionCrossingTimer;
	S32				mRegionCrossingCount;
	
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
	BOOL 		        needsRenderBeam();
private:
	LLPointer<LLHUDEffectSpiral> mBeam;
	LLFrameTimer mBeamTimer;

	//--------------------------------------------------------------------
	// LLVOAvatar Constants
	//--------------------------------------------------------------------
public:
	LLViewerTexture::EBoostLevel 	getAvatarBoostLevel() const override { return LLGLTexture::BOOST_AVATAR_SELF; }
	LLViewerTexture::EBoostLevel 	getAvatarBakedBoostLevel() const override { return LLGLTexture::BOOST_AVATAR_BAKED_SELF; }
	S32 						    getTexImageSize() const override { return LLVOAvatar::getTexImageSize()*4; }

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
	S32					getLocalDiscardLevel(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
	bool				areTexturesCurrent() const;
	BOOL				isLocalTextureDataAvailable(const LLViewerTexLayerSet* layerset) const;
	BOOL				isLocalTextureDataFinal(const LLViewerTexLayerSet* layerset) const;
	// If you want to check all textures of a given type, pass gAgentWearables.getWearableCount() for index
	BOOL                isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const override;
	BOOL	            isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, U32 index = 0) const override;
	BOOL	            isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerWearable *wearable) const override;


	//--------------------------------------------------------------------
	// Local Textures
	//--------------------------------------------------------------------
public:
	BOOL				getLocalTextureGL(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerTexture** image_gl_pp, U32 index) const;
	LLViewerFetchedTexture*	getLocalTextureGL(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
	const LLUUID&		getLocalTextureID(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const;
	void				setLocalTextureTE(U8 te, LLViewerTexture* image, U32 index);
	void	            setLocalTexture(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerTexture* tex, BOOL baked_version_exits, U32 index) override;
protected:
	void	            setBakedReady(LLAvatarAppearanceDefines::ETextureIndex type, BOOL baked_version_exists, U32 index) override;
	void				localTextureLoaded(BOOL succcess, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	void				getLocalTextureByteCount(S32* gl_byte_count) const;
	void	            addLocalTextureStats(LLAvatarAppearanceDefines::ETextureIndex i, LLViewerFetchedTexture* imagep, F32 texel_area_ratio, BOOL rendered, BOOL covered_by_baked) override;
	LLLocalTextureObject* getLocalTextureObject(LLAvatarAppearanceDefines::ETextureIndex i, U32 index) const;

private:
	static void			onLocalTextureLoaded(BOOL succcess, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);

	void	            setImage(const U8 te, LLViewerTexture *imagep, const U32 index) override; 
	LLViewerTexture*    getImage(const U8 te, const U32 index) const override;


	//--------------------------------------------------------------------
	// Baked textures
	//--------------------------------------------------------------------
public:
	LLAvatarAppearanceDefines::ETextureIndex getBakedTE(const LLViewerTexLayerSet* layerset ) const;
	// SUNSHINE CLEANUP - dead? or update to just call request appearance update?
	void				forceBakeAllTextures(bool slam_for_debug = false);
protected:
	void	            removeMissingBakedTextures() override;

	//--------------------------------------------------------------------
	// Layers
	//--------------------------------------------------------------------
public:
	void				requestLayerSetUpdate(LLAvatarAppearanceDefines::ETextureIndex i);
	LLViewerTexLayerSet* getLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;
	LLViewerTexLayerSet* getLayerSet(LLAvatarAppearanceDefines::ETextureIndex index) const;

	
	//--------------------------------------------------------------------
	// Composites
	//--------------------------------------------------------------------
public:
	void	            invalidateComposite(LLTexLayerSet* layerset) override;
	void	            invalidateAll() override;
	void	            setCompositeUpdatesEnabled(bool b) override; // only works for self
	void                setCompositeUpdatesEnabled(U32 index, bool b) override;
	bool 	            isCompositeUpdateEnabled(U32 index) override;
	void				setupComposites();
	void				updateComposites();

	const LLUUID&		grabBakedTexture(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;
	BOOL				canGrabBakedTexture(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;


	//--------------------------------------------------------------------
	// Scratch textures (used for compositing)
	//--------------------------------------------------------------------
public:
	static void		deleteScratchTextures();
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
	void                restoreMeshData() override;

/**                    Meshes
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    WEARABLES
 **/

public:
	void				wearableUpdated(LLWearableType::EType type);
protected:
	U32 getNumWearables(LLAvatarAppearanceDefines::ETextureIndex i) const;

	//--------------------------------------------------------------------
	// Attachments
	//--------------------------------------------------------------------
public:
	void 				updateAttachmentVisibility(U32 camera_mode);
	BOOL 				isWearingAttachment(const LLUUID& inv_item_id) const;
	LLViewerObject* 	getWornAttachment(const LLUUID& inv_item_id);
	bool				getAttachedPointName(const LLUUID& inv_item_id, std::string& name) const;
	const LLViewerJointAttachment *attachObject(LLViewerObject *viewer_object) override;
	BOOL 	            detachObject(LLViewerObject *viewer_object) override;
	static BOOL			detachAttachmentIntoInventory(const LLUUID& item_id);

	//--------------------------------------------------------------------
	// HUDs
	//--------------------------------------------------------------------
private:
	LLViewerJoint* 		mScreenp; // special purpose joint for HUD attachments
	
/**                    Attachments
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    ANIMATIONS
 **/
public:
    // Note: DEFAULT_ATTACHMENT_UPDATE_PERIOD (seconds) is F32
    // to agree with what the server API supplies
    // however we employ an U64 (usec) expiry pattern for mAttachmentUpdateExpiry
    // under the hood (see below)
    static constexpr F32    DEFAULT_ATTACHMENT_UPDATE_PERIOD = 0.1f;

    void                updateMotions(LLCharacter::e_update_t update_type) override;  // from llCharacter
    void                setAttachmentUpdatePeriod(F32 period_sec);
    void                setAttachmentUpdateEnabled(bool enable) { mAttachmentUpdateEnabled = enable; }
    bool                getAttachmentUpdateEnabled() const { return mAttachmentUpdateEnabled; }

private:
    bool                mAttachmentUpdateEnabled;
    U64                 mAttachmentUpdatePeriod; // usec
    U64                 mAttachmentUpdateExpiry; // usec

 /**                   ANIMATIONS
  **                                                                            **
  *******************************************************************************/


/********************************************************************************
 **                                                                            **
 **                    APPEARANCE
 **/

public:
	static void		onCustomizeStart(bool disable_camera_switch = false);
	static void		onCustomizeEnd(bool disable_camera_switch = false);
	LLPointer<LLInventoryCallback> mEndCustomizeCallback;

	//--------------------------------------------------------------------
	// Visibility
	//--------------------------------------------------------------------

    bool            shouldRenderRigged() const override;
    
public:
	bool			sendAppearanceMessage(LLMessageSystem *mesgsys) const;

	// -- care and feeding of hover height.
	void 			setHoverIfRegionEnabled();
	void			sendHoverHeight() const;
	void            setHoverOffset(const LLVector3& hover_offset, bool send_update=true) override;

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
	static void		dumpTotalLocalTextureByteCount();
	void			dumpLocalTextures() const;
	static void		dumpScratchTextureByteCount();
	void			dumpWearableInfo(LLAPRFile& outfile);

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
		LLUUID			mAvatarID;
		LLAvatarAppearanceDefines::ETextureIndex	mIndex;
	};

	LLTimer					mTimeSinceLastRezMessage;
	bool					updateAvatarRezMetrics(bool force_send);

	std::vector<LLSD>		mPendingTimerRecords;
	void 					addMetricsTimerRecord(const LLSD& record);
	
	void 					debugWearablesLoaded() { mDebugTimeWearablesLoaded = mDebugSelfLoadTimer.getElapsedTimeF32(); }
	void 					debugAvatarVisible() { mDebugTimeAvatarVisible = mDebugSelfLoadTimer.getElapsedTimeF32(); }
	void 					outputRezDiagnostics() const;
	void					outputRezTiming(const std::string& msg) const;
	void					reportAvatarRezTime() const;
	void 					debugBakedTextureUpload(LLAvatarAppearanceDefines::EBakedTextureIndex index, BOOL finished);
	static void				debugOnTimingLocalTexLoaded(BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);

	BOOL					isAllLocalTextureDataFinal() const;

	const LLViewerTexLayerSet*	debugGetLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex index) const { return (LLViewerTexLayerSet*)(mBakedTextureDatas[index].mTexLayerSet); }
	const std::string		verboseDebugDumpLocalTextureDataInfo(const LLViewerTexLayerSet* layerset) const; // Lists out state of this particular baked texture layer
	void					dumpAllTextures() const;
	const std::string		debugDumpLocalTextureDataInfo(const LLViewerTexLayerSet* layerset) const; // Lists out state of this particular baked texture layer
	const std::string		debugDumpAllLocalTextureDataInfo() const; // Lists out which baked textures are at highest LOD
	void					sendViewerAppearanceChangeMetrics(); // send data associated with completing a change.
private:
	LLFrameTimer    		mDebugSelfLoadTimer;
	F32						mDebugTimeWearablesLoaded;
	F32 					mDebugTimeAvatarVisible;
	F32 					mDebugTextureLoadTimes[LLAvatarAppearanceDefines::TEX_NUM_INDICES][MAX_DISCARD_LEVEL+1]; // load time for each texture at each discard level
	F32 					mDebugBakedTextureTimes[LLAvatarAppearanceDefines::BAKED_NUM_INDICES][2]; // time to start upload and finish upload of each baked texture
	void					debugTimingLocalTexLoaded(BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);

    void                    appearanceChangeMetricsCoro(std::string url);
    bool                    mInitialMetric;
    S32                     mMetricSequence;
/**                    Diagnostics
 **                                                                            **
 *******************************************************************************/

};

extern LLPointer<LLVOAvatarSelf> gAgentAvatarp;

BOOL isAgentAvatarValid();

void selfStartPhase(const std::string& phase_name);
void selfStopPhase(const std::string& phase_name, bool err_check = true);
void selfClearPhases();

#endif // LL_VO_AVATARSELF_H
