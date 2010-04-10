/** 
 * @file lltexlayer.h
 * @brief A texture layer. Used for avatars.
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
#include "llwearable.h"
#include "llvoavatardefines.h"

class LLVOAvatar;
class LLVOAvatarSelf;
class LLImageTGA;
class LLImageRaw;
class LLXmlTreeNode;
class LLPolyMorphTarget;
class LLTexLayerSet;
class LLTexLayerSetInfo;
class LLTexLayerInfo;
class LLTexLayerSetBuffer;
class LLTexLayerParamColor;
class LLTexLayerParamColorInfo;
class LLTexLayerParamAlpha;
class LLTexLayerParamAlphaInfo;
class LLWearable;
class LLViewerVisualParam;

typedef std::vector<LLTexLayerParamColor *> param_color_list_t;
typedef std::vector<LLTexLayerParamAlpha *> param_alpha_list_t;
typedef std::vector<LLTexLayerParamColorInfo *> param_color_info_list_t;
typedef std::vector<LLTexLayerParamAlphaInfo *> param_alpha_info_list_t;


//-----------------------------------------------------------------------------
// LLTexLayerInterface
// Interface class to generalize functionality shared by LLTexLayer and LLTexLayerTemplate.

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

	const LLTexLayerInfo* 	getInfo() const { return mInfo; }
	virtual BOOL			setInfo(const LLTexLayerInfo *info, LLWearable* wearable ); // This sets mInfo and calls initialization functions
	virtual BOOL			render(S32 x, S32 y, S32 width, S32 height) = 0;
	void					requestUpdate();
	LLTexLayerSet*			const getTexLayerSet() const { return mTexLayerSet; }

	virtual void			deleteCaches() = 0;
	void					invalidateMorphMasks();
	virtual void			setHasMorph(BOOL newval) { mHasMorph = newval; }
	BOOL					hasMorph()				 { return mHasMorph; }
	BOOL					isMorphValid()			 { return mMorphMasksValid; }

	const std::string&		getName() const;
	ERenderPass				getRenderPass() const;
	const std::string&		getGlobalColor() const;

	virtual BOOL			blendAlphaTexture( S32 x, S32 y, S32 width, S32 height) = 0;
	virtual void			gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height) = 0;
	BOOL					hasAlphaParams() const { return !mParamAlphaList.empty(); }
	BOOL					isVisibilityMask() const;
	virtual BOOL			isInvisibleAlphaMask() = 0;

	LLTexLayerSet*			getLayerSet() {return mTexLayerSet;}

	LLViewerVisualParam*	getVisualParamPtr(S32 index);


protected:
	LLTexLayerSet*			const mTexLayerSet;

	// Layers can have either mParamColorList, mGlobalColor, or mFixedColor.  They are looked for in that order.
	param_color_list_t		mParamColorList;
	// 						mGlobalColor name stored in mInfo
	// 						mFixedColor value stored in mInfo
	param_alpha_list_t		mParamAlphaList;

	BOOL					mMorphMasksValid;
	BOOL					mStaticImageInvalid;

	BOOL					mHasMorph;

	const LLTexLayerInfo			*mInfo;

};

//-----------------------------------------------------------------------------
// LLTexLayerTemplate
// Template class 
// Only exists for llvoavatarself

class LLTexLayerTemplate : public LLTexLayerInterface
{
public:
	LLTexLayerTemplate(LLTexLayerSet* const layer_set);
	LLTexLayerTemplate(const LLTexLayerTemplate &layer);
	/*virtual*/ ~LLTexLayerTemplate();

	/*virtual*/ BOOL		render(S32 x, S32 y, S32 width, S32 height);
	/*virtual*/ BOOL		setInfo(const LLTexLayerInfo *info, LLWearable* wearable ); // This sets mInfo and calls initialization functions
	/*virtual*/ BOOL		blendAlphaTexture( S32 x, S32 y, S32 width, S32 height); // Multiplies a single alpha texture against the frame buffer
	/*virtual*/ void		gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	/*virtual*/ void		setHasMorph(BOOL newval);
	/*virtual*/ void		deleteCaches();
	/*virtual*/ BOOL		isInvisibleAlphaMask();

private:
	U32 	updateWearableCache();
	LLTexLayer* getLayer(U32 i);
	typedef std::vector<LLWearable*> wearable_cache_t;
	wearable_cache_t mWearableCache;

};

//-----------------------------------------------------------------------------
// LLTexLayer
// A single texture layer
// Only exists for llvoavatarself

class LLTexLayer : public LLTexLayerInterface
{
public:
	LLTexLayer(LLTexLayerSet* const layer_set);
	LLTexLayer(const LLTexLayer &layer, LLWearable *wearable);
	LLTexLayer(const LLTexLayerTemplate &layer_template, LLLocalTextureObject *lto, LLWearable *wearable);
	/*virtual*/ ~LLTexLayer();

	/*virtual*/ BOOL		setInfo(const LLTexLayerInfo *info, LLWearable* wearable ); // This sets mInfo and calls initialization functions
	/*virtual*/ BOOL		render(S32 x, S32 y, S32 width, S32 height);

	/*virtual*/ void		deleteCaches();
	U8*						getAlphaData();

	BOOL					findNetColor(LLColor4* color) const;
	/*virtual*/ BOOL		blendAlphaTexture( S32 x, S32 y, S32 width, S32 height); // Multiplies a single alpha texture against the frame buffer
	/*virtual*/ void		gatherAlphaMasks(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	BOOL					renderMorphMasks(S32 x, S32 y, S32 width, S32 height, const LLColor4 &layer_color);
	void					addAlphaMask(U8 *data, S32 originX, S32 originY, S32 width, S32 height);
	/*virtual*/ BOOL		isInvisibleAlphaMask();

	void					setLTO(LLLocalTextureObject *lto) { mLocalTextureObject = lto; }
	LLLocalTextureObject* 	getLTO() { return mLocalTextureObject; }

	static void calculateTexLayerColor(const param_color_list_t &param_list, LLColor4 &net_color);

private:
	LLUUID					getUUID();

	typedef std::map<U32, U8*> alpha_cache_t;
	alpha_cache_t			mAlphaCache;
	LLLocalTextureObject 	*mLocalTextureObject;
};

// Make private
class LLTexLayerInfo
{
	friend class LLTexLayer;
	friend class LLTexLayerTemplate;
	friend class LLTexLayerInterface;
public:
	LLTexLayerInfo();
	~LLTexLayerInfo();

	BOOL parseXml(LLXmlTreeNode* node);
	BOOL createVisualParams(LLVOAvatar *avatar);
	BOOL isUserSettable() { return mLocalTexture != -1;	}
	S32  getLocalTexture() const { return mLocalTexture; }
	BOOL getOnlyAlpha() const { return mUseLocalTextureAlphaOnly; }
	std::string getName() const { return mName;	}

private:
	std::string				mName;
	
	BOOL					mWriteAllChannels; // Don't use masking.  Just write RGBA into buffer,
	LLTexLayer::ERenderPass				mRenderPass;

	std::string				mGlobalColor;
	LLColor4				mFixedColor;

	S32						mLocalTexture;
	std::string				mStaticImageFileName;
	BOOL					mStaticImageIsMask;
	BOOL					mUseLocalTextureAlphaOnly; // Ignore RGB channels from the input texture.  Use alpha as a mask
	BOOL					mIsVisibilityMask;

	typedef std::vector< std::pair< std::string,BOOL > > morph_name_list_t;
	morph_name_list_t		    mMorphNameList;
	param_color_info_list_t		mParamColorInfoList;
	param_alpha_info_list_t		mParamAlphaInfoList;
};

//
// LLTexLayer
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
// Only exists for llvoavatarself

class LLTexLayerSet
{
	friend class LLTexLayerSetBuffer;
public:
	LLTexLayerSet(LLVOAvatarSelf* const avatar);
	~LLTexLayerSet();

	const LLTexLayerSetInfo* 		getInfo() const { return mInfo; }
	BOOL					setInfo(const LLTexLayerSetInfo *info); // This sets mInfo and calls initialization functions

	BOOL					render(S32 x, S32 y, S32 width, S32 height);
	void					renderAlphaMaskTextures(S32 x, S32 y, S32 width, S32 height, bool forceClear = false);

	BOOL					isBodyRegion(const std::string& region) const;
	LLTexLayerSetBuffer*	getComposite();
	void					requestUpdate();
	void					requestUpload();
	void					cancelUpload();
	void					updateComposite();
	BOOL					isLocalTextureDataAvailable() const;
	BOOL					isLocalTextureDataFinal() const;
	void					createComposite();
	void					destroyComposite();
	void					setUpdatesEnabled(BOOL b);
	BOOL					getUpdatesEnabled()	const { return mUpdatesEnabled; }
	void					deleteCaches();
	void					gatherMorphMaskAlpha(U8 *data, S32 width, S32 height);
	void					applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components);
	BOOL					isMorphValid();
	void					invalidateMorphMasks();
	LLTexLayerInterface*	findLayerByName(const std::string& name);
	void					cloneTemplates(LLLocalTextureObject *lto, LLVOAvatarDefines::ETextureIndex tex_index, LLWearable* wearable);
	
	LLVOAvatarSelf*		    getAvatar()	const { return mAvatar; }
	const std::string		getBodyRegion() const;
	BOOL					hasComposite() const { return (mComposite.notNull()); }
	LLVOAvatarDefines::EBakedTextureIndex getBakedTexIndex() { return mBakedTexIndex; }
	void					setBakedTexIndex( LLVOAvatarDefines::EBakedTextureIndex index) { mBakedTexIndex = index; }
	BOOL					isVisible() const { return mIsVisible; }

public:
	static BOOL		sHasCaches;

	typedef std::vector<LLTexLayerInterface *> layer_list_t;

private:
	layer_list_t			mLayerList;
	layer_list_t			mMaskLayerList;
	LLPointer<LLTexLayerSetBuffer>	mComposite;
	LLVOAvatarSelf*	const	mAvatar; // Backlink only; don't make this an LLPointer.
	BOOL					mUpdatesEnabled;
	BOOL					mIsVisible;

	LLVOAvatarDefines::EBakedTextureIndex mBakedTexIndex;

	const LLTexLayerSetInfo 		*mInfo;
};

// Contains shared layer set data
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

// The composite image that a LLTexLayerSet writes to.  Each LLTexLayerSet has one.
class LLTexLayerSetBuffer : public LLViewerDynamicTexture
{
public:
	LLTexLayerSetBuffer(LLTexLayerSet* const owner, S32 width, S32 height);
	virtual ~LLTexLayerSetBuffer();

	/*virtual*/ S8          getType() const ;
	virtual void			preRender(BOOL clear_depth);
	virtual void			postRender(BOOL success);
	virtual BOOL			render();
	BOOL					updateImmediate();
	bool					isInitialized(void) const;
	BOOL					needsRender();
	void					requestUpdate();
	void					requestUpload();
	void					cancelUpload();
	BOOL					uploadPending() { return mUploadPending; }
	BOOL					render( S32 x, S32 y, S32 width, S32 height );
	void					readBackAndUpload();

	static void				onTextureUploadComplete(const LLUUID& uuid,
													void* userdata,
													S32 result, LLExtStat ext_status);
	static void				dumpTotalByteCount();

	virtual void restoreGLTexture();
	virtual void destroyGLTexture();

private:
	void					pushProjection() const;
	void					popProjection() const;

private:
	LLTexLayerSet* const    mTexLayerSet;

	BOOL					mNeedsUpdate;
	BOOL					mNeedsUpload;
	BOOL					mUploadPending;
	LLUUID					mUploadID; // Identifys the current upload process (null if none).  Used to avoid overlaps (eg, when the user rapidly makes two changes outside of Face Edit)

	static S32				sGLByteCount;
};

//
// LLTexLayerSet
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLTexLayerStaticImageList
//

class LLTexLayerStaticImageList : public LLSingleton<LLTexLayerStaticImageList>
{
public:
	LLTexLayerStaticImageList();
	~LLTexLayerStaticImageList();

	LLViewerTexture*	getTexture(const std::string& file_name, BOOL is_mask);
	LLImageTGA*	getImageTGA(const std::string& file_name);

	void		deleteCachedImages();
	void		dumpByteCount();

private:
	BOOL		loadImageRaw(const std::string& file_name, LLImageRaw* image_raw);

private:
	LLStringTable mImageNames;

	typedef std::map< const char*, LLPointer<LLViewerTexture> > texture_map_t;
	texture_map_t mStaticImageList;
	typedef std::map< const char*, LLPointer<LLImageTGA> > image_tga_map_t;
	image_tga_map_t mStaticImageListTGA;

	S32 mGLBytes;
	S32 mTGABytes;
};

// Used by LLTexLayerSetBuffer for a callback.
// Note to anyone merging branches - this supercedes the previous fix
// for DEV-31590 "Heap corruption and crash after outfit changes",
// here and in lltexlayer.cpp. Equally correct and a bit simpler.
class LLBakedUploadData
{
public:
	LLBakedUploadData(const LLVOAvatarSelf* avatar, LLTexLayerSet* layerset, const LLUUID& id);
	~LLBakedUploadData() {}

	const LLUUID				mID;
	const LLVOAvatarSelf*		mAvatar;	 // just backlink, don't LLPointer 
	LLTexLayerSet*				mTexLayerSet;
   	const U64					mStartTime;		// Used to measure time baked texture upload requires
};


#endif  // LL_LLTEXLAYER_H
