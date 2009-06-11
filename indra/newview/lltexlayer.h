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
#include "llassetstorage.h"
#include "lldynamictexture.h"
#include "llrect.h"
#include "llstring.h"
#include "lluuid.h"
#include "llviewerimage.h"
#include "llviewervisualparam.h"
#include "llwearable.h"
#include "v4color.h"
#include "llfloater.h"

class LLTexLayerSetInfo;
class LLTexLayerSet;
class LLTexLayerInfo;
class LLTexLayer;
class LLImageGL;
class LLImageTGA;
class LLTexGlobalColorInfo;
class LLTexLayerParamAlphaInfo;
class LLTexLayerParamAlpha;
class LLTexParamColorInfo;
class LLTexParamColor;
class LLPolyMesh;
class LLXmlTreeNode;
class LLImageRaw;
class LLPolyMorphTarget;

class LLTextureCtrl;
class LLVOAvatar;


enum EColorOperation
{
	OP_ADD = 0,
	OP_MULTIPLY = 1,
	OP_BLEND = 2,
	OP_COUNT = 3 // Number of operations
};


//-----------------------------------------------------------------------------
// LLTexLayerParamAlphaInfo
//-----------------------------------------------------------------------------
class LLTexLayerParamAlphaInfo : public LLViewerVisualParamInfo
{
	friend class LLTexLayerParamAlpha;
public:
	LLTexLayerParamAlphaInfo();
	/*virtual*/ ~LLTexLayerParamAlphaInfo() {};

	/*virtual*/ BOOL parseXml(LLXmlTreeNode* node);

protected:
	std::string				mStaticImageFileName;
	BOOL					mMultiplyBlend;
	BOOL					mSkipIfZeroWeight;
	F32						mDomain;
};

//-----------------------------------------------------------------------------
// LLTexParamColorInfo
//-----------------------------------------------------------------------------
class LLTexParamColorInfo : public LLViewerVisualParamInfo
{
	friend class LLTexParamColor;

public:
	LLTexParamColorInfo();
	virtual ~LLTexParamColorInfo() {};
	BOOL parseXml( LLXmlTreeNode* node );
		
protected:
	enum { MAX_COLOR_VALUES = 20 };
	EColorOperation		mOperation;
	LLColor4			mColors[MAX_COLOR_VALUES];
	S32					mNumColors;
};

//-----------------------------------------------------------------------------
// LLTexGlobalColorInfo
//-----------------------------------------------------------------------------
class LLTexGlobalColorInfo
{
	friend class LLTexGlobalColor;
public:
	LLTexGlobalColorInfo();
	~LLTexGlobalColorInfo();

	BOOL parseXml(LLXmlTreeNode* node);
	
protected:
	typedef std::vector<LLTexParamColorInfo *> color_info_list_t;
	color_info_list_t		mColorInfoList;
	std::string				mName;
};

//-----------------------------------------------------------------------------
// LLTexLayerSetInfo
// Containes shared layer set data
//-----------------------------------------------------------------------------
class LLTexLayerSetInfo
{
	friend class LLTexLayerSet;
public:
	LLTexLayerSetInfo();
	~LLTexLayerSetInfo();
	
	BOOL parseXml(LLXmlTreeNode* node);

protected:
	std::string				mBodyRegion;
	S32						mWidth;
	S32						mHeight;
	std::string				mStaticAlphaFileName;
	BOOL					mClearAlpha;		// Set alpha to 1 for this layerset (if there is no mStaticAlphaFileName)
	
	typedef std::vector<LLTexLayerInfo*> layer_info_list_t;
	layer_info_list_t		mLayerInfoList;
};

//-----------------------------------------------------------------------------
// LLTexLayerInfo
//-----------------------------------------------------------------------------
enum ERenderPass 
{
	RP_COLOR,
	RP_BUMP,
	RP_SHINE
};

class LLTexLayerInfo
{
	friend class LLTexLayer;
public:
	LLTexLayerInfo();
	~LLTexLayerInfo();

	BOOL parseXml(LLXmlTreeNode* node);

protected:
	std::string				mName;
	
	BOOL					mWriteAllChannels;  // Don't use masking.  Just write RGBA into buffer,
	ERenderPass				mRenderPass;

	std::string				mGlobalColor;
	LLColor4				mFixedColor;

	S32						mLocalTexture;
	std::string				mStaticImageFileName;
	BOOL					mStaticImageIsMask;
	BOOL					mUseLocalTextureAlphaOnly;	// Ignore RGB channels from the input texture.  Use alpha as a mask

	typedef std::vector<std::pair<std::string,BOOL> > morph_name_list_t;
	morph_name_list_t		mMorphNameList;

	typedef std::vector<LLTexParamColorInfo*> color_info_list_t;
	color_info_list_t		mColorInfoList;

	typedef std::vector<LLTexLayerParamAlphaInfo*> alpha_info_list_t;
	alpha_info_list_t		mAlphaInfoList;
	
};

//-----------------------------------------------------------------------------
// LLTexLayerSetBuffer
// The composite image that a LLTexLayerSet writes to.  Each LLTexLayerSet has one.
//-----------------------------------------------------------------------------
class LLTexLayerSetBuffer : public LLDynamicTexture
{
public:
	LLTexLayerSetBuffer( LLTexLayerSet*	owner, S32 width, S32 height, BOOL has_bump );
	virtual ~LLTexLayerSetBuffer();

	virtual void			preRender(BOOL clear_depth);
	virtual void			postRender(BOOL success);
	virtual BOOL			render();
	BOOL					updateImmediate();
	void					bindBumpTexture( U32 stage );
	bool					isInitialized(void) const;
	BOOL					needsRender();
	void					requestUpdate();
	void					requestUpload();
	void					cancelUpload();
	BOOL					uploadPending() { return mUploadPending; }
	BOOL					render( S32 x, S32 y, S32 width, S32 height );
	void					readBackAndUpload(U8* baked_bump_data);
	void                    createBumpTexture() ;

	static void				onTextureUploadComplete( const LLUUID& uuid,
													 void* userdata,
													 S32 result, LLExtStat ext_status);
	static void				dumpTotalByteCount();

	virtual void restoreGLTexture() ;
	virtual void destroyGLTexture() ;

private:
	void					pushProjection();
	void					popProjection();

private:
	BOOL                    mHasBump ;
	BOOL					mNeedsUpdate;
	BOOL					mNeedsUpload;
	BOOL					mUploadPending;
	LLUUID					mUploadID;		// Identifys the current upload process (null if none).  Used to avoid overlaps (eg, when the user rapidly makes two changes outside of Face Edit)
	LLTexLayerSet*			mTexLayerSet;
	LLPointer<LLImageGL>	mBumpTex;	// zero if none

	static S32				sGLByteCount;
	static S32				sGLBumpByteCount;
};

//-----------------------------------------------------------------------------
// LLTexLayerSet
// An ordered set of texture layers that get composited into a single texture.
//-----------------------------------------------------------------------------
class LLTexLayerSet
{
public:
	LLTexLayerSet( LLVOAvatar* avatar );
	~LLTexLayerSet();

	//BOOL					parseData(LLXmlTreeNode* node);
	LLTexLayerSetInfo* 		getInfo() const { return mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLTexLayerSetInfo *info);
	
	BOOL					render( S32 x, S32 y, S32 width, S32 height );
	BOOL					renderBump( S32 x, S32 y, S32 width,S32 height );
	BOOL					isBodyRegion( const std::string& region ) { return mInfo->mBodyRegion == region; }
	LLTexLayerSetBuffer*	getComposite();
	void					requestUpdate();
	void					requestUpload();
	void					cancelUpload();
	LLVOAvatar*				getAvatar()								{ return mAvatar; }
	void					updateComposite();
	BOOL					isLocalTextureDataAvailable();
	BOOL					isLocalTextureDataFinal();
	void					createComposite();
	void					destroyComposite();
	void					setUpdatesEnabled( BOOL b );
	BOOL					getUpdatesEnabled()						{ return mUpdatesEnabled; }
	void					deleteCaches();
	void					gatherAlphaMasks(U8 *data, S32 width, S32 height);
	void					applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components);
	const std::string		getBodyRegion() 				{ return mInfo->mBodyRegion; }
	BOOL					hasComposite()					{ return (mComposite != NULL); }
	void					setBump( BOOL b )				{ mHasBump = b; }
	BOOL					hasBump()						{ return mHasBump; }

public:
	static BOOL		sHasCaches;

protected:
	typedef std::vector<LLTexLayer *> layer_list_t;
	layer_list_t			mLayerList;
	LLTexLayerSetBuffer*	mComposite;
	// Backlink only; don't make this an LLPointer.
	LLVOAvatar*				mAvatar;
	BOOL					mUpdatesEnabled;
	BOOL					mHasBump;

	LLTexLayerSetInfo 		*mInfo;
};

//-----------------------------------------------------------------------------
// LLMaskedMorph
//-----------------------------------------------------------------------------

class LLMaskedMorph
{
public:
	LLMaskedMorph( LLPolyMorphTarget *morph_target, BOOL invert );
	
public:
	LLPolyMorphTarget	*mMorphTarget;
	BOOL				mInvert;
};

//-----------------------------------------------------------------------------
// LLTexLayer
// A single texture layer
//-----------------------------------------------------------------------------
class LLTexLayer
{
public:
	LLTexLayer( LLTexLayerSet* layer_set );
	~LLTexLayer();

	//BOOL					parseData(LLXmlTreeNode* node);
	LLTexLayerInfo* 		getInfo() const { return mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLTexLayerInfo *info);
	
	BOOL					render( S32 x, S32 y, S32 width, S32 height );
	void					requestUpdate();
	LLTexLayerSet*			getTexLayerSet()						{ return mTexLayerSet; }

	const std::string&		getName()								{ return mInfo->mName; }

	void					addMaskedMorph(LLPolyMorphTarget* morph_target, BOOL invert);
	void					deleteCaches();
	U8*						getAlphaData();
	void					applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components);

	void					invalidateMorphMasks();
	ERenderPass				getRenderPass() 						{ return mInfo->mRenderPass; }
	const std::string&			getGlobalColor() 						{ return mInfo->mGlobalColor; }
	BOOL					findNetColor( LLColor4* color );
	BOOL					renderImageRaw( U8* in_data, S32 in_width, S32 in_height, S32 in_components, S32 width, S32 height, BOOL is_mask );
	BOOL					renderAlphaMasks(  S32 x, S32 y, S32 width, S32 height, LLColor4* colorp );
	BOOL					hasAlphaParams() { return (!mParamAlphaList.empty());}

protected:
	LLTexLayerSet*			mTexLayerSet;
	LLPointer<LLImageRaw>	mStaticImageRaw;

	// Layers can have either mParamColorList, mGlobalColor, or mFixedColor.  They are looked for in that order.
	typedef std::vector<LLTexParamColor *> color_list_t;
	color_list_t			mParamColorList;
	// 						mGlobalColor name stored in mInfo
	// 						mFixedColor value stored in mInfo

	typedef std::vector<LLTexLayerParamAlpha *> alpha_list_t;
	alpha_list_t			mParamAlphaList;
	
	
	typedef std::deque<LLMaskedMorph> morph_list_t;
	morph_list_t			mMaskedMorphs;
	typedef std::map<U32, U8*> alpha_cache_t;
	alpha_cache_t			mAlphaCache;
	BOOL					mMorphMasksValid;
	BOOL					mStaticImageInvalid;

	LLTexLayerInfo			*mInfo;
};

//-----------------------------------------------------------------------------
// LLTexLayerParamAlpha
//-----------------------------------------------------------------------------
class LLTexLayerParamAlpha : public LLViewerVisualParam
{
public:
	LLTexLayerParamAlpha( LLTexLayer* layer );
	/*virtual*/ ~LLTexLayerParamAlpha();

	// Special: These functions are overridden by child classes
	LLTexLayerParamAlphaInfo* 	getInfo() const { return (LLTexLayerParamAlphaInfo*)mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL						setInfo(LLTexLayerParamAlphaInfo *info);
	
	// LLVisualParam Virtual functions
	///*virtual*/ BOOL		parseData(LLXmlTreeNode* node);
	/*virtual*/ void		apply( ESex avatar_sex ) {}
	/*virtual*/ void		setWeight(F32 weight, BOOL set_by_user);
	/*virtual*/ void		setAnimationTarget(F32 target_value, BOOL set_by_user); 
	/*virtual*/ void		animate(F32 delta, BOOL set_by_user);

	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion()									{ return 1.f; }
	/*virtual*/ const LLVector3&	getAvgDistortion()										{ return mAvgDistortionVec; }
	/*virtual*/ F32					getMaxDistortion()										{ return 3.f; }
	/*virtual*/ LLVector3			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)	{ return LLVector3(1.f, 1.f, 1.f);}
	/*virtual*/ const LLVector3*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return &mAvgDistortionVec;};
	/*virtual*/ const LLVector3*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return NULL;};

	// New functions
	BOOL					render( S32 x, S32 y, S32 width, S32 height );
	BOOL					getSkip();
	void					deleteCaches();
	LLTexLayer*				getTexLayer()		{ return mTexLayer; }
	BOOL					getMultiplyBlend()	{ return getInfo()->mMultiplyBlend; }

protected:
	LLPointer<LLImageGL>	mCachedProcessedImageGL;
	LLTexLayer*				mTexLayer;
	LLPointer<LLImageTGA>	mStaticImageTGA;
	LLPointer<LLImageRaw>	mStaticImageRaw;
	BOOL					mNeedsCreateTexture;
	BOOL					mStaticImageInvalid;
	LLVector3				mAvgDistortionVec;
	F32						mCachedEffectiveWeight;

public:
	// Global list of instances for gathering statistics
	static void				dumpCacheByteCount();
	static void				getCacheByteCount( S32* gl_bytes );

	typedef std::list< LLTexLayerParamAlpha* > param_alpha_ptr_list_t;
	static param_alpha_ptr_list_t sInstances;
};


//-----------------------------------------------------------------------------
// LLTexGlobalColor
//-----------------------------------------------------------------------------
class LLTexGlobalColor
{
public:
	LLTexGlobalColor( LLVOAvatar* avatar );
	~LLTexGlobalColor();

	//BOOL					parseData(LLXmlTreeNode* node);
	LLTexGlobalColorInfo*	getInfo() const { return mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLTexGlobalColorInfo *info);
	
	void					requstUpdate();
	LLVOAvatar*				getAvatar()						{ return mAvatar; }
	LLColor4				getColor();
	const std::string&		getName()						{ return mInfo->mName; }

protected:
	typedef std::vector<LLTexParamColor *> param_list_t;
	param_list_t			mParamList;
	LLVOAvatar*				mAvatar;  // just backlink, don't LLPointer 

	LLTexGlobalColorInfo	*mInfo;
};


//-----------------------------------------------------------------------------
// LLTexParamColor
//-----------------------------------------------------------------------------
class LLTexParamColor : public LLViewerVisualParam
{
public:
	LLTexParamColor( LLTexGlobalColor* tex_color );
	LLTexParamColor( LLTexLayer* layer );
	/* virtual */ ~LLTexParamColor();

	// Special: These functions are overridden by child classes
	LLTexParamColorInfo*		getInfo() const { return (LLTexParamColorInfo*)mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL						setInfo(LLTexParamColorInfo *info);
	
	// LLVisualParam Virtual functions
	///*virtual*/ BOOL			parseData(LLXmlTreeNode* node);
	/*virtual*/ void			apply( ESex avatar_sex ) {}
	/*virtual*/ void			setWeight(F32 weight, BOOL set_by_user);
	/*virtual*/ void			setAnimationTarget(F32 target_value, BOOL set_by_user);
	/*virtual*/ void			animate(F32 delta, BOOL set_by_user);


	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion()									{ return 1.f; }
	/*virtual*/ const LLVector3&	getAvgDistortion()										{ return mAvgDistortionVec; }
	/*virtual*/ F32					getMaxDistortion()										{ return 3.f; }
	/*virtual*/ LLVector3			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)	{ return LLVector3(1.f, 1.f, 1.f); }
	/*virtual*/ const LLVector3*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return &mAvgDistortionVec;};
	/*virtual*/ const LLVector3*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return NULL;};

	// New functions
	LLColor4				getNetColor();
	EColorOperation			getOperation() const	{ return getInfo()->mOperation; }

	
protected:
	LLVector3				mAvgDistortionVec;
	LLTexGlobalColor*		mTexGlobalColor;	// either has mTexGlobalColor or mTexLayer as its parent
	LLTexLayer*				mTexLayer;
	LLVOAvatar*				mAvatar;			// redundant, but simplifies the code (don't LLPointer)
};

//-----------------------------------------------------------------------------
// LLTexStaticImageList
//-----------------------------------------------------------------------------

class LLTexStaticImageList
{
public:
	LLTexStaticImageList();
	~LLTexStaticImageList();

	LLImageRaw*	getImageRaw( const std::string& file_name );
	LLImageGL*	getImageGL( const std::string& file_name, BOOL is_mask  );
	LLImageTGA*	getImageTGA( const std::string& file_name );

	void		deleteCachedImages();
	void		dumpByteCount();

private:
	BOOL		loadImageRaw( const std::string& file_name, LLImageRaw* image_raw );

private:
	static LLStringTable sImageNames;

	typedef std::map< const char *, LLPointer<LLImageGL> > image_gl_map_t;
	typedef std::map< const char *, LLPointer<LLImageTGA> > image_tga_map_t;
	image_gl_map_t mStaticImageListGL;
	image_tga_map_t mStaticImageListTGA;

public:
	S32 mGLBytes;
	S32 mTGABytes;
};

// Used by LLTexLayerSetBuffer for a callback.

// For DEV-DEV-31590, "Heap corruption and crash after outfit
// changes", added the mLayerSet member.  The current
// LLTexLayerSetBuffer can be found by querying mLayerSet->mComposite,
// but we still store the original mLayerSetBuffer here so we can
// detect when an upload is out of date.  This prevents a memory
// stomp.  See LLTexLayerSetBuffer::onTextureUploadComplete() for usage.
class LLBakedUploadData
{
public:
	LLBakedUploadData( LLVOAvatar* avatar, LLTexLayerSet* layerset, LLTexLayerSetBuffer* layerset_buffer, const LLUUID & id);
	~LLBakedUploadData() {}

	LLUUID					mID;
	LLVOAvatar*				mAvatar;	 // just backlink, don't LLPointer 
	LLTexLayerSet*			mLayerSet;
	LLTexLayerSetBuffer*	mLayerSetBuffer;
	LLUUID					mWearableAssets[WT_COUNT];
	U64						mStartTime;		// Used to measure time baked texture upload requires
};

extern LLTexStaticImageList gTexStaticImageList;


#endif  // LL_LLTEXLAYER_H
