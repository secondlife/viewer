/** 
 * @file lltexlayerparams.h
 * @brief Texture layer parameters, used by lltexlayer.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLTEXLAYERPARAMS_H
#define LL_LLTEXLAYERPARAMS_H

#include "llviewervisualparam.h"

class LLTexLayer;
class LLVOAvatar;
class LLWearable;

class LLTexLayerParam : public LLViewerVisualParam
{
public: 
	LLTexLayerParam(LLTexLayerInterface *layer);
	LLTexLayerParam(LLVOAvatar *avatar);
	/*virtual*/ BOOL setInfo(LLViewerVisualParamInfo *info, BOOL add_to_avatar  );
	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable) const = 0;

protected:
	LLTexLayerInterface*	mTexLayer;
	LLVOAvatar*             mAvatar;
};

//-----------------------------------------------------------------------------
// LLTexLayerParamAlpha
// 
class LLTexLayerParamAlpha : public LLTexLayerParam
{
public:
	LLTexLayerParamAlpha( LLTexLayerInterface* layer );
	LLTexLayerParamAlpha( LLVOAvatar* avatar );
	/*virtual*/ ~LLTexLayerParamAlpha();

	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable = NULL) const;

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL		parseData(LLXmlTreeNode* node);
	/*virtual*/ void		apply( ESex avatar_sex ) {}
	/*virtual*/ void		setWeight(F32 weight, BOOL upload_bake);
	/*virtual*/ void		setAnimationTarget(F32 target_value, BOOL upload_bake); 
	/*virtual*/ void		animate(F32 delta, BOOL upload_bake);

	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion()									{ return 1.f; }
	/*virtual*/ const LLVector3&	getAvgDistortion()										{ return mAvgDistortionVec; }
	/*virtual*/ F32					getMaxDistortion()										{ return 3.f; }
	/*virtual*/ LLVector3			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)	{ return LLVector3(1.f, 1.f, 1.f);}
	/*virtual*/ const LLVector3*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return &mAvgDistortionVec;};
	/*virtual*/ const LLVector3*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return NULL;};

	// New functions
	BOOL					render( S32 x, S32 y, S32 width, S32 height );
	BOOL					getSkip() const;
	void					deleteCaches();
	BOOL					getMultiplyBlend() const;

private:
	LLPointer<LLViewerTexture>	mCachedProcessedTexture;
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
class LLTexLayerParamAlphaInfo : public LLViewerVisualParamInfo
{
	friend class LLTexLayerParamAlpha;
public:
	LLTexLayerParamAlphaInfo();
	/*virtual*/ ~LLTexLayerParamAlphaInfo() {};

	/*virtual*/ BOOL parseXml(LLXmlTreeNode* node);

private:
	std::string				mStaticImageFileName;
	BOOL					mMultiplyBlend;
	BOOL					mSkipIfZeroWeight;
	F32						mDomain;
};
//
// LLTexLayerParamAlpha
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLTexLayerParamColor
//
class LLTexLayerParamColor : public LLTexLayerParam
{
public:
	enum EColorOperation
	{
		OP_ADD = 0,
		OP_MULTIPLY = 1,
		OP_BLEND = 2,
		OP_COUNT = 3 // Number of operations
	};

	LLTexLayerParamColor( LLTexLayerInterface* layer );
	LLTexLayerParamColor( LLVOAvatar* avatar );
	/* virtual */ ~LLTexLayerParamColor();

	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable = NULL) const;

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL			parseData(LLXmlTreeNode* node);
	/*virtual*/ void			apply( ESex avatar_sex ) {}
	/*virtual*/ void			setWeight(F32 weight, BOOL upload_bake);
	/*virtual*/ void			setAnimationTarget(F32 target_value, BOOL upload_bake);
	/*virtual*/ void			animate(F32 delta, BOOL upload_bake);


	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion()									{ return 1.f; }
	/*virtual*/ const LLVector3&	getAvgDistortion()										{ return mAvgDistortionVec; }
	/*virtual*/ F32					getMaxDistortion()										{ return 3.f; }
	/*virtual*/ LLVector3			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)	{ return LLVector3(1.f, 1.f, 1.f); }
	/*virtual*/ const LLVector3*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return &mAvgDistortionVec;};
	/*virtual*/ const LLVector3*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)	{ index = 0; poly_mesh = NULL; return NULL;};

	// New functions
	LLColor4				getNetColor() const;
protected:
	virtual void onGlobalColorChanged(bool upload_bake) {}
private:
	LLVector3				mAvgDistortionVec;
};

class LLTexLayerParamColorInfo : public LLViewerVisualParamInfo
{
	friend class LLTexLayerParamColor;

public:
	LLTexLayerParamColorInfo();
	virtual ~LLTexLayerParamColorInfo() {};
	BOOL parseXml( LLXmlTreeNode* node );
	LLTexLayerParamColor::EColorOperation getOperation() const { return mOperation; }
private:
	enum { MAX_COLOR_VALUES = 20 };
	LLTexLayerParamColor::EColorOperation		mOperation;
	LLColor4			mColors[MAX_COLOR_VALUES];
	S32					mNumColors;
};

//
// LLTexLayerParamColor
//-----------------------------------------------------------------------------

#endif
