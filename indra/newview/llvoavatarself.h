/**
 * @file llvoavatarself.h
 * @brief Declaration of LLVOAvatar class which is a derivation fo
 * LLViewerObject
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLVOAVATARSELF_H
#define LL_LLVOAVATARSELF_H

#include "llvoavatar.h"

struct LocalTextureData;

//------------------------------------------------------------------------
// LLVOAvatarSelf
//------------------------------------------------------------------------
class LLVOAvatarSelf :
	public LLVOAvatar
{

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/

public:
	LLVOAvatarSelf(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual 				~LLVOAvatarSelf();
	virtual void			markDead();
protected:
	BOOL					loadAvatarSelf();
	BOOL					buildSkeletonSelf(const LLVOAvatarSkeletonInfo *info);
	BOOL					buildMenus();
	/*virtual*/ BOOL		loadLayersets();

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
	/*virtual*/ void 		updateRegion(LLViewerRegion *regionp);

	//--------------------------------------------------------------------
	// LLCharacter interface and related
	//--------------------------------------------------------------------
public:
	/*virtual*/ void 		stopMotionFromSource(const LLUUID& source_id);
	/*virtual*/ void 		requestStopMotion(LLMotion* motion);
	/*virtual*/ LLJoint*	getJoint(const std::string &name);

/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/

public:
	/*virtual*/ bool 	isSelf() const { return true; }

	//--------------------------------------------------------------------
	// Updates
	//--------------------------------------------------------------------
public:
	/*virtual*/ BOOL 	updateCharacter(LLAgent &agent);
	/*virtual*/ void 	idleUpdateTractorBeam();

	//--------------------------------------------------------------------
	// Loading state
	//--------------------------------------------------------------------
public:
	/*virtual*/ BOOL    updateIsFullyLoaded();
private:
	BOOL                mIsBaked; // are the stored baked textures up to date?

	//--------------------------------------------------------------------
	// Region state
	//--------------------------------------------------------------------
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
	BOOL 		needsRenderBeam();
private:
	LLPointer<LLHUDEffectSpiral> mBeam;
	LLFrameTimer mBeamTimer;

	//--------------------------------------------------------------------
	// LLVOAvatar Constants
	//--------------------------------------------------------------------
public:
	/*virtual*/ LLViewerImage::EBoostLevel 	getAvatarBoostLevel() const { return LLViewerImage::BOOST_AVATAR_SELF; }
	/*virtual*/ LLViewerImage::EBoostLevel 	getAvatarBakedBoostLevel() const { return LLViewerImage::BOOST_AVATAR_BAKED_SELF; }
	/*virtual*/ S32 						getTexImageSize() const { return LLVOAvatar::getTexImageSize()*4; }

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
	/*virtual*/ bool	hasPendingBakedUploads() const;
	S32					getLocalDiscardLevel(LLVOAvatarDefines::ETextureIndex type, U32 index = 0) const;
	bool				areTexturesCurrent() const;
	BOOL				isLocalTextureDataAvailable(const LLTexLayerSet* layerset) const;
	BOOL				isLocalTextureDataFinal(const LLTexLayerSet* layerset) const;
	/*virtual*/ BOOL    isTextureDefined(LLVOAvatarDefines::ETextureIndex type, U32 index = 0) const;

	//--------------------------------------------------------------------
	// Local Textures
	//--------------------------------------------------------------------
public:
	BOOL				getLocalTextureGL(LLVOAvatarDefines::ETextureIndex type, LLImageGL** image_gl_pp, U32 index = 0) const;
	const LLUUID&		getLocalTextureID(LLVOAvatarDefines::ETextureIndex type, U32 index = 0) const;
	void				setLocalTextureTE(U8 te, LLViewerImage* image, BOOL set_by_user, U32 index = 0);
	const LLUUID&		grabLocalTexture(LLVOAvatarDefines::ETextureIndex type, U32 index = 0) const;
	BOOL				canGrabLocalTexture(LLVOAvatarDefines::ETextureIndex type, U32 index = 0) const;
protected:
	/*virtual*/ void	setLocalTexture(LLVOAvatarDefines::ETextureIndex type, LLViewerImage* tex, BOOL baked_version_exits, U32 index = 0);
	void				localTextureLoaded(BOOL succcess, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	void				getLocalTextureByteCount(S32* gl_byte_count) const;
	/*virtual*/ void	addLocalTextureStats(LLVOAvatarDefines::ETextureIndex i, LLViewerImage* imagep, F32 texel_area_ratio, BOOL rendered, BOOL covered_by_baked, U32 index = 0);
private:
	static void			onLocalTextureLoaded(BOOL succcess, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);

	//--------------------------------------------------------------------
	// Baked textures
	//--------------------------------------------------------------------
public:
	LLVOAvatarDefines::ETextureIndex getBakedTE(const LLTexLayerSet* layerset ) const;
	void				setNewBakedTexture(LLVOAvatarDefines::ETextureIndex i, const LLUUID& uuid);
	void				setCachedBakedTexture(LLVOAvatarDefines::ETextureIndex i, const LLUUID& uuid);
	void				forceBakeAllTextures(bool slam_for_debug = false);
	static void			processRebakeAvatarTextures(LLMessageSystem* msg, void**);
protected:
	/*virtual*/ void	removeMissingBakedTextures();

	//--------------------------------------------------------------------
	// Layers
	//--------------------------------------------------------------------
public:
	void 				requestLayerSetUploads();
	void				requestLayerSetUpdate(LLVOAvatarDefines::ETextureIndex i);
protected:
	LLTexLayerSet*		getLayerSet(LLVOAvatarDefines::ETextureIndex index) const;
	
	//--------------------------------------------------------------------
	// Composites
	//--------------------------------------------------------------------
public:
	/* virtual */ void	invalidateComposite(LLTexLayerSet* layerset, BOOL set_by_user);
	/* virtual */ void	invalidateAll();
	/* virtual */ void	setCompositeUpdatesEnabled(BOOL b); // only works for self
	void				setupComposites();
	void				updateComposites();

	//--------------------------------------------------------------------
	// Scratch textures (used for compositing)
	//--------------------------------------------------------------------
public:
	BOOL			bindScratchTexture(LLGLenum format);
	static void		deleteScratchTextures();
protected:
	LLGLuint		getScratchTexName(LLGLenum format, U32* texture_bytes);
private:
	static S32 		sScratchTexBytes;
	static LLMap< LLGLenum, LLGLuint*> sScratchTexNames;
	static LLMap< LLGLenum, F32*> sScratchTexLastBindTime;

	//--------------------------------------------------------------------
	// Texture Data
	//--------------------------------------------------------------------
private:
	typedef std::vector<LocalTextureData*> 	localtexture_vec_t; // all textures of a certain TE
	typedef std::map<LLVOAvatarDefines::ETextureIndex, localtexture_vec_t> localtexture_map_t; // texture TE vectors arranged by texture type
	localtexture_map_t 						mLocalTextureDatas;

	const localtexture_vec_t&				getLocalTextureData(LLVOAvatarDefines::ETextureIndex index) const; // const accessor into mLocalTextureDatas
	const LocalTextureData*					getLocalTextureData(LLVOAvatarDefines::ETextureIndex type, U32 index) const; // const accessor to individual LocalTextureData

	localtexture_vec_t&				getLocalTextureData(LLVOAvatarDefines::ETextureIndex index); // accessor into mLocalTextureDatas
	LocalTextureData*					getLocalTextureData(LLVOAvatarDefines::ETextureIndex type, U32 index); // accessor to individual LocalTextureData

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
	void			wearableUpdated(EWearableType type);

	//--------------------------------------------------------------------
	// Attachments
	//--------------------------------------------------------------------
public:
	void 				updateAttachmentVisibility(U32 camera_mode);
	BOOL 				isWearingAttachment(const LLUUID& inv_item_id);
	LLViewerObject* 	getWornAttachment(const LLUUID& inv_item_id ) const;
	const std::string   getAttachedPointName(const LLUUID& inv_item_id) const;
	/*virtual*/ LLViewerJointAttachment *attachObject(LLViewerObject *viewer_object);

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
 **                    APPEARANCE
 **/

public:
	static void		onCustomizeStart();
	static void		onCustomizeEnd();

	//--------------------------------------------------------------------
	// Visibility
	//--------------------------------------------------------------------
public:
	static void		onChangeSelfInvisible(BOOL newvalue);
	void			setInvisible(BOOL newvalue);

/**                    Appearance
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    DIAGNOSTICS
 **/

public:	
	static void		dumpTotalLocalTextureByteCount();
	void			dumpLocalTextures() const;
	static void		dumpScratchTextureByteCount();

/**                    Diagnostics
 **                                                                            **
 *******************************************************************************/

};

#endif // LL_VO_AVATARSELF_H
