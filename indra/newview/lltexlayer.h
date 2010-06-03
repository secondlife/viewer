/** 
 * @file lltexlayer.h
 * @brief Texture layer classes. Used for avatars.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLTEXLAYER_H
#define LL_LLTEXLAYER_H

#include <deque>
#include "lldynamictexture.h"
#include "llvoavatardefines.h"
#include "lltexlayerparams.h"

class LLVOAvatar;
class LLVOAvatarSelf;
class LLImageTGA;
class LLImageRaw;
class LLXmlTreeNode;
class LLTexLayerSet;
class LLTexLayerSetInfo;
class LLTexLayerInfo;
class LLTexLayerSetBuffer;
class LLWearable;
class LLViewerVisualParam;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerInterface
//
// Interface class to generalize functionality shared by LLTexLayer 
// and LLTexLayerTemplate.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerInterface 
{
public:
	enum ERenderPass
	{
		RP_COLOR,
		RP_BUMP,
		RP_SHINE
	};

	LLTexLayerInterface(LLTexLayerSet* const layer_set);
	LLTexLayerInterface(const LLTexLayerInterface &layer, LLWearable *wearable);
	virtual ~LLTexLayerInterface() {}

	virtual BOOL			render(S32 x, S32 y, S32 width, S32 height) = 0;
	virtual void			deleteCaches() = 0;
	virtual BOOL			blendAlphaTexture(S32 x, S32 y, S32 width, S32 height) = 0;
	virtual BOOL			isInvisibleAlphaMask() const = 0;

	const LLTexLayerInfo* 	getInfo() const 			{ return mInfo; }
	virtual BOOL			setInfo(const LLTexLayerInfo *info, LLWearable* wearable); // sets mInfo, calls initialization functions

	const std::string&		getName() const;
	const LLTexLayerSet* const getTexLayerSet() const 	{ return mTexLayerSet; }
	LLTexLayerSet* const 	getTexLayerSet() 			{ return mTexLayerSet; }

	void					invalidateMorphMasks();
	virtual void			setHasMorph(BOOL newval) 	{ mHasMorph = newval; }
	BOOL					hasMorph() const			{ return mHasMorph; }
	BOOL					isMorphValid() const		{ return mMorphMasksValid; }

	void					requestUpdate();
	virtual void			gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height) = 0;
	BOOL					hasAlphaParams() const 		{ return !mParamAlphaList.empty(); }

	ERenderPass				getRenderPass() const;
	BOOL					isVisibilityMask() const;

protected:
	const std::string&		getGlobalColor() const;
	LLViewerVisualParam*	getVisualParamPtr(S32 index) const;

protected:
	LLTexLayerSet* const	mTexLayerSet;
	const LLTexLayerInfo*	mInfo;
	BOOL					mMorphMasksValid;
	BOOL					mHasMorph;

	// Layers can have either mParamColorList, mGlobalColor, or mFixedColor.  They are looked for in that order.
	param_color_list_t		mParamColorList;
	param_alpha_list_t		mParamAlphaList;
	// 						mGlobalColor name stored in mInfo
	// 						mFixedColor value stored in mInfo
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerTemplate
//
// Only exists for llvoavatarself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerTemplate : public LLTexLayerInterface
{
public:
	LLTexLayerTemplate(LLTexLayerSet* const layer_set);
	LLTexLayerTemplate(const LLTexLayerTemplate &layer);
	/*virtual*/ ~LLTexLayerTemplate();
	/*virtual*/ BOOL		render(S32 x, S32 y, S32 width, S32 height);
	/*virtual*/ BOOL		setInfo(const LLTexLayerInfo *info, LLWearable* wearable); // This sets mInfo and calls initialization functions
	/*virtual*/ BOOL		blendAlphaTexture(S32 x, S32 y, S32 width, S32 height); // Multiplies a single alpha texture against the frame buffer
	/*virtual*/ void		gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	/*virtual*/ void		setHasMorph(BOOL newval);
	/*virtual*/ void		deleteCaches();
	/*virtual*/ BOOL		isInvisibleAlphaMask() const;
protected:
	U32 					updateWearableCache() const;
	LLTexLayer* 			getLayer(U32 i) const;
private:
	typedef std::vector<LLWearable*> wearable_cache_t;
	mutable wearable_cache_t mWearableCache; // mutable b/c most get- require updating this cache
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayer
//
// A single texture layer.  Only exists for llvoavatarself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayer : public LLTexLayerInterface
{
public:
	LLTexLayer(LLTexLayerSet* const layer_set);
	LLTexLayer(const LLTexLayer &layer, LLWearable *wearable);
	LLTexLayer(const LLTexLayerTemplate &layer_template, LLLocalTextureObject *lto, LLWearable *wearable);
	/*virtual*/ ~LLTexLayer();

	/*virtual*/ BOOL		setInfo(const LLTexLayerInfo *info, LLWearable* wearable); // This sets mInfo and calls initialization functions
	/*virtual*/ BOOL		render(S32 x, S32 y, S32 width, S32 height);

	/*virtual*/ void		deleteCaches();
	const U8*				getAlphaData() const;

	BOOL					findNetColor(LLColor4* color) const;
	/*virtual*/ BOOL		blendAlphaTexture(S32 x, S32 y, S32 width, S32 height); // Multiplies a single alpha texture against the frame buffer
	/*virtual*/ void		gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	BOOL					renderMorphMasks(S32 x, S32 y, S32 width, S32 height, const LLColor4 &layer_color);
	void					addAlphaMask(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	/*virtual*/ BOOL		isInvisibleAlphaMask() const;

	void					setLTO(LLLocalTextureObject *lto) 	{ mLocalTextureObject = lto; }
	LLLocalTextureObject* 	getLTO() 							{ return mLocalTextureObject; }

	static void 			calculateTexLayerColor(const param_color_list_t &param_list, LLColor4 &net_color);
protected:
	LLUUID					getUUID() const;
private:
	typedef std::map<U32, U8*> alpha_cache_t;
	alpha_cache_t			mAlphaCache;
	LLLocalTextureObject* 	mLocalTextureObject;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerSet
//
// An ordered set of texture layers that gets composited into a single texture.
// Only exists for llvoavatarself.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerSet
{
	friend class LLTexLayerSetBuffer;
public:
	LLTexLayerSet(LLVOAvatarSelf* const avatar);
	~LLTexLayerSet();

	const LLTexLayerSetInfo* 	getInfo() const 			{ return mInfo; }
	BOOL						setInfo(const LLTexLayerSetInfo *info); // This sets mInfo and calls initialization functions

	BOOL						render(S32 x, S32 y, S32 width, S32 height);
	void						renderAlphaMaskTextures(S32 x, S32 y, S32 width, S32 height, bool forceClear = false);

	BOOL						isBodyRegion(const std::string& region) const;
	LLTexLayerSetBuffer*		getComposite();
	const LLTexLayerSetBuffer* 	getComposite() const; // Do not create one if it doesn't exist.
	void						requestUpdate();
	void						requestUpload();
	void						cancelUpload();
	void						updateComposite();
	BOOL						isLocalTextureDataAvailable() const;
	BOOL						isLocalTextureDataFinal() const;
	void						createComposite();
	void						destroyComposite();
	void						setUpdatesEnabled(BOOL b);
	BOOL						getUpdatesEnabled()	const 	{ return mUpdatesEnabled; }
	void						deleteCaches();
	void						gatherMorphMaskAlpha(U8 *data, S32 width, S32 height);
	void						applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components);
	BOOL						isMorphValid() const;
	void						invalidateMorphMasks();
	LLTexLayerInterface*		findLayerByName(const std::string& name);
	void						cloneTemplates(LLLocalTextureObject *lto, LLVOAvatarDefines::ETextureIndex tex_index, LLWearable* wearable);
	
	LLVOAvatarSelf*		    	getAvatar()	const 			{ return mAvatar; }
	const std::string			getBodyRegionName() const;
	BOOL						hasComposite() const 		{ return (mComposite.notNull()); }
	LLVOAvatarDefines::EBakedTextureIndex getBakedTexIndex() { return mBakedTexIndex; }
	void						setBakedTexIndex(LLVOAvatarDefines::EBakedTextureIndex index) { mBakedTexIndex = index; }
	BOOL						isVisible() const 			{ return mIsVisible; }

	static BOOL					sHasCaches;

private:
	typedef std::vector<LLTexLayerInterface *> layer_list_t;
	layer_list_t				mLayerList;
	layer_list_t				mMaskLayerList;
	LLPointer<LLTexLayerSetBuffer>	mComposite;
	LLVOAvatarSelf*	const		mAvatar; // note: backlink only; don't make this an LLPointer.
	BOOL						mUpdatesEnabled;
	BOOL						mIsVisible;

	LLVOAvatarDefines::EBakedTextureIndex mBakedTexIndex;
	const LLTexLayerSetInfo* 	mInfo;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerSetInfo
//
// Contains shared layer set data.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerSetInfo
{
	friend class LLTexLayerSet;
public:
	LLTexLayerSetInfo();
	~LLTexLayerSetInfo();
	BOOL parseXml(LLXmlTreeNode* node);
	void createVisualParams(LLVOAvatar *avatar);
private:
	std::string				mBodyRegion;
	S32						mWidth;
	S32						mHeight;
	std::string				mStaticAlphaFileName;
	BOOL					mClearAlpha; // Set alpha to 1 for this layerset (if there is no mStaticAlphaFileName)
	typedef std::vector<LLTexLayerInfo*> layer_info_list_t;
	layer_info_list_t		mLayerInfoList;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerSetBuffer
//
// The composite image that a LLTexLayerSet writes to.  Each LLTexLayerSet has one.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerSetBuffer : public LLViewerDynamicTexture
{
public:
	LLTexLayerSetBuffer(LLTexLayerSet* const owner, S32 width, S32 height);
	virtual ~LLTexLayerSetBuffer();
	/*virtual*/ S8          getType() const;
	virtual void			preRender(BOOL clear_depth);
	virtual void			postRender(BOOL success);
	virtual BOOL			render();
	BOOL					updateImmediate();
	bool					isInitialized(void) const;
	/*virtual*/ BOOL		needsRender();
	void					requestUpdate();
	void					requestUpload();
	void					cancelUpload();
	BOOL					uploadPending() const { return mUploadPending; }
	BOOL					render(S32 x, S32 y, S32 width, S32 height);
	void					readBackAndUpload();
	static void				onTextureUploadComplete(const LLUUID& uuid,
													void* userdata,
													S32 result, LLExtStat ext_status);
	static void				dumpTotalByteCount();
	const std::string		dumpTextureInfo() const;
	virtual void 			restoreGLTexture();
	virtual void 			destroyGLTexture();
protected:
	void					pushProjection() const;
	void					popProjection() const;
	BOOL					isReadyToUpload() const;
private:
	LLTexLayerSet* const    mTexLayerSet;
	BOOL					mNeedsUpdate; // whether we need to update our baked textures
	BOOL					mNeedsUpload; // whether we need to send our baked textures to the server
	U32						mNumLowresUploads; // mumber of times we've sent a lowres version of our baked textures to the server
	BOOL					mUploadPending; // whether we have received back the new baked textures
	LLUUID					mUploadID; // the current upload process (null if none).  Used to avoid overlaps, e.g. when the user rapidly makes two changes outside of Face Edit.
	static S32				sGLByteCount;
	LLFrameTimer    		mNeedsUploadTimer; // Tracks time since upload was requested
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLTexLayerStaticImageList
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTexLayerStaticImageList : public LLSingleton<LLTexLayerStaticImageList>
{
public:
	LLTexLayerStaticImageList();
	~LLTexLayerStaticImageList();
	LLViewerTexture*	getTexture(const std::string& file_name, BOOL is_mask);
	LLImageTGA*			getImageTGA(const std::string& file_name);
	void				deleteCachedImages();
	void				dumpByteCount() const;
protected:
	BOOL				loadImageRaw(const std::string& file_name, LLImageRaw* image_raw);
private:
	LLStringTable 		mImageNames;
	typedef std::map<const char*, LLPointer<LLViewerTexture>> texture_map_t;
	texture_map_t 		mStaticImageList;
	typedef std::map<const char*, LLPointer<LLImageTGA>> image_tga_map_t;
	image_tga_map_t 	mStaticImageListTGA;
	S32 				mGLBytes;
	S32 				mTGABytes;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLBakedUploadData
//
// Used by LLTexLayerSetBuffer for a callback.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct LLBakedUploadData
{
	LLBakedUploadData(const LLVOAvatarSelf* avatar, 
					  LLTexLayerSet* layerset, 
					  const LLUUID& id);
	~LLBakedUploadData() {}
	const LLUUID				mID;
	const LLVOAvatarSelf*		mAvatar; // note: backlink only; don't LLPointer 
	LLTexLayerSet*				mTexLayerSet;
   	const U64					mStartTime;	// for measuring baked texture upload time
};

#endif  // LL_LLTEXLAYER_H
