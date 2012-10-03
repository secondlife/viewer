/** 
 * @file lltexlayer.h
 * @brief Texture layer classes. Used for avatars.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLTEXLAYER_H
#define LL_LLTEXLAYER_H

#include <deque>
#include "llglslshader.h"
#include "llgltexture.h"
#include "llavatarappearancedefines.h"
#include "lltexlayerparams.h"

class LLAvatarAppearance;
class LLImageTGA;
class LLImageRaw;
class LLLocalTextureObject;
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
	LLWearableType::EType	getWearableType() const;
	LLAvatarAppearanceDefines::ETextureIndex	getLocalTextureIndex() const;

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

	virtual void			asLLSD(LLSD& sd) const {}

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
	LLTexLayerTemplate(LLTexLayerSet* const layer_set, LLAvatarAppearance* const appearance);
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
	LLAvatarAppearance*		getAvatarAppearance()	const		{ return mAvatarAppearance; }
private:
	LLAvatarAppearance*	const	mAvatarAppearance; // note: backlink only; don't make this an LLPointer.
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

	/*virtual*/ void		asLLSD(LLSD& sd) const;

	static void 			calculateTexLayerColor(const param_color_list_t &param_list, LLColor4 &net_color);
protected:
	LLUUID					getUUID() const;
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
	LLTexLayerSet(LLAvatarAppearance* const appearance);
	virtual ~LLTexLayerSet();

	LLTexLayerSetBuffer*		getComposite();
	const LLTexLayerSetBuffer* 	getComposite() const; // Do not create one if it doesn't exist.
	virtual void				createComposite() = 0;
	void						destroyComposite();
	void						gatherMorphMaskAlpha(U8 *data, S32 origin_x, S32 origin_y, S32 width, S32 height);

	const LLTexLayerSetInfo* 	getInfo() const 			{ return mInfo; }
	BOOL						setInfo(const LLTexLayerSetInfo *info); // This sets mInfo and calls initialization functions

	BOOL						render(S32 x, S32 y, S32 width, S32 height);
	void						renderAlphaMaskTextures(S32 x, S32 y, S32 width, S32 height, bool forceClear = false);

	BOOL						isBodyRegion(const std::string& region) const;
	void						applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components);
	BOOL						isMorphValid() const;
	virtual void				requestUpdate() = 0;
	void						invalidateMorphMasks();
	void						deleteCaches();
	LLTexLayerInterface*		findLayerByName(const std::string& name);
	void						cloneTemplates(LLLocalTextureObject *lto, LLAvatarAppearanceDefines::ETextureIndex tex_index, LLWearable* wearable);
	
	LLAvatarAppearance*			getAvatarAppearance()	const		{ return mAvatarAppearance; }
	const std::string			getBodyRegionName() const;
	BOOL						hasComposite() const 		{ return (mComposite.notNull()); }
	LLAvatarAppearanceDefines::EBakedTextureIndex getBakedTexIndex() const { return mBakedTexIndex; }
	void						setBakedTexIndex(LLAvatarAppearanceDefines::EBakedTextureIndex index) { mBakedTexIndex = index; }
	BOOL						isVisible() const 			{ return mIsVisible; }

	static BOOL					sHasCaches;

	virtual void				asLLSD(LLSD& sd) const;

protected:
	typedef std::vector<LLTexLayerInterface *> layer_list_t;
	layer_list_t				mLayerList;
	layer_list_t				mMaskLayerList;
	LLPointer<LLTexLayerSetBuffer>	mComposite;
	LLAvatarAppearance*	const	mAvatarAppearance; // note: backlink only; don't make this an LLPointer.
	BOOL						mIsVisible;

	LLAvatarAppearanceDefines::EBakedTextureIndex mBakedTexIndex;
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
	void createVisualParams(LLAvatarAppearance *appearance);
	S32 getWidth() const { return mWidth; }
	S32 getHeight() const { return mHeight; }
protected:
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
class LLTexLayerSetBuffer : public virtual LLRefCount
{
	LOG_CLASS(LLTexLayerSetBuffer);

public:
	LLTexLayerSetBuffer(LLTexLayerSet* const owner);
	virtual ~LLTexLayerSetBuffer();

protected:
	void					pushProjection() const;
	void					popProjection() const;
	virtual void			preRenderTexLayerSet();
	virtual void			midRenderTexLayerSet(BOOL success) {}
	virtual void			postRenderTexLayerSet(BOOL success);
	virtual S32				getCompositeOriginX() const = 0;
	virtual S32				getCompositeOriginY() const = 0;
	virtual S32				getCompositeWidth() const = 0;
	virtual S32				getCompositeHeight() const = 0;
	BOOL					renderTexLayerSet();

	LLTexLayerSet* const	mTexLayerSet;
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
	LLGLTexture*		getTexture(const std::string& file_name, BOOL is_mask);
	LLImageTGA*			getImageTGA(const std::string& file_name);
	void				deleteCachedImages();
	void				dumpByteCount() const;
protected:
	BOOL				loadImageRaw(const std::string& file_name, LLImageRaw* image_raw);
private:
	LLStringTable 		mImageNames;
	typedef std::map<const char*, LLPointer<LLGLTexture> > texture_map_t;
	texture_map_t 		mStaticImageList;
	typedef std::map<const char*, LLPointer<LLImageTGA> > image_tga_map_t;
	image_tga_map_t 	mStaticImageListTGA;
	S32 				mGLBytes;
	S32 				mTGABytes;
};

#endif  // LL_LLTEXLAYER_H
