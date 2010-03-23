/** 
 * @file lltexlayerparams.cpp
 * @brief SERAPH - ADD IN
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

#include "llviewerprecompiledheaders.h"

#include "lltexlayerparams.h"

#include "llagentcamera.h"
#include "llimagetga.h"
#include "lltexlayer.h"
#include "llvoavatarself.h"
#include "llwearable.h"
#include "llui.h"

//-----------------------------------------------------------------------------
// LLTexLayerParam
//-----------------------------------------------------------------------------
LLTexLayerParam::LLTexLayerParam(LLTexLayerInterface *layer) :
	mTexLayer(layer),
	mAvatar(NULL)
{
	if (mTexLayer != NULL)
	{
		mAvatar = mTexLayer->getTexLayerSet()->getAvatar();
	}
	else
	{
		llerrs << "LLTexLayerParam constructor passed with NULL reference for layer!" << llendl;
	}
}

LLTexLayerParam::LLTexLayerParam(LLVOAvatar *avatar) :
	mTexLayer(NULL)
{
	mAvatar = avatar;
}


BOOL LLTexLayerParam::setInfo(LLViewerVisualParamInfo *info, BOOL add_to_avatar  )
{	
	LLViewerVisualParam::setInfo(info);

	if (add_to_avatar)
	{
		mAvatar->addVisualParam( this);
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// LLTexLayerParamAlpha
//-----------------------------------------------------------------------------

// static 
LLTexLayerParamAlpha::param_alpha_ptr_list_t LLTexLayerParamAlpha::sInstances;

// static 
void LLTexLayerParamAlpha::dumpCacheByteCount()
{
	S32 gl_bytes = 0;
	getCacheByteCount( &gl_bytes);
	llinfos << "Processed Alpha Texture Cache GL:" << (gl_bytes/1024) << "KB" << llendl;
}

// static 
void LLTexLayerParamAlpha::getCacheByteCount(S32* gl_bytes)
{
	*gl_bytes = 0;

	for (param_alpha_ptr_list_t::iterator iter = sInstances.begin();
		 iter != sInstances.end(); iter++)
	{
		LLTexLayerParamAlpha* instance = *iter;
		LLViewerTexture* tex = instance->mCachedProcessedTexture;
		if (tex)
		{
			S32 bytes = (S32)tex->getWidth() * tex->getHeight() * tex->getComponents();

			if (tex->hasGLTexture())
			{
				*gl_bytes += bytes;
			}
		}
	}
}

LLTexLayerParamAlpha::LLTexLayerParamAlpha(LLTexLayerInterface* layer) :
	LLTexLayerParam(layer),
	mCachedProcessedTexture(NULL),
	mNeedsCreateTexture(FALSE),
	mStaticImageInvalid(FALSE),
	mAvgDistortionVec(1.f, 1.f, 1.f),
	mCachedEffectiveWeight(0.f)
{
	sInstances.push_front(this);
}

LLTexLayerParamAlpha::LLTexLayerParamAlpha(LLVOAvatar* avatar) :
	LLTexLayerParam(avatar),
	mCachedProcessedTexture(NULL),
	mNeedsCreateTexture(FALSE),
	mStaticImageInvalid(FALSE),
	mAvgDistortionVec(1.f, 1.f, 1.f),
	mCachedEffectiveWeight(0.f)
{
	sInstances.push_front(this);
}


LLTexLayerParamAlpha::~LLTexLayerParamAlpha()
{
	deleteCaches();
	sInstances.remove(this);
}

/*virtual*/ LLViewerVisualParam* LLTexLayerParamAlpha::cloneParam(LLWearable* wearable) const
{
	LLTexLayerParamAlpha *new_param = new LLTexLayerParamAlpha(mTexLayer);
	*new_param = *this;
	return new_param;
}

void LLTexLayerParamAlpha::deleteCaches()
{
	mStaticImageTGA = NULL; // deletes image
	mCachedProcessedTexture = NULL;
	mStaticImageRaw = NULL;
	mNeedsCreateTexture = FALSE;
}

BOOL LLTexLayerParamAlpha::getMultiplyBlend() const
{
	return ((LLTexLayerParamAlphaInfo *)getInfo())->mMultiplyBlend; 	
}

void LLTexLayerParamAlpha::setWeight(F32 weight, BOOL upload_bake)
{
	if (mIsAnimating || mTexLayer == NULL)
	{
		return;
	}
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	F32 new_weight = llclamp(weight, min_weight, max_weight);
	U8 cur_u8 = F32_to_U8(mCurWeight, min_weight, max_weight);
	U8 new_u8 = F32_to_U8(new_weight, min_weight, max_weight);
	if (cur_u8 != new_u8)
	{
		mCurWeight = new_weight;

		if ((mAvatar->getSex() & getSex()) && (mAvatar->isSelf() && !mIsDummy)) // only trigger a baked texture update if we're changing a wearable's visual param.
		{
			if (gAgentCamera.cameraCustomizeAvatar())
			{
				upload_bake = FALSE;
			}
			mAvatar->invalidateComposite(mTexLayer->getTexLayerSet(), upload_bake);
			mTexLayer->invalidateMorphMasks();
		}
	}
}

void LLTexLayerParamAlpha::setAnimationTarget(F32 target_value, BOOL upload_bake)
{ 
	// do not animate dummy parameters
	if (mIsDummy)
	{
		setWeight(target_value, upload_bake);
		return;
	}

	mTargetWeight = target_value; 
	setWeight(target_value, upload_bake); 
	mIsAnimating = TRUE;
	if (mNext)
	{
		mNext->setAnimationTarget(target_value, upload_bake);
	}
}

void LLTexLayerParamAlpha::animate(F32 delta, BOOL upload_bake)
{
	if (mNext)
	{
		mNext->animate(delta, upload_bake);
	}
}

BOOL LLTexLayerParamAlpha::getSkip() const
{
	if (!mTexLayer)
	{
		return TRUE;
	}

	const LLVOAvatar *avatar = mTexLayer->getTexLayerSet()->getAvatar();

	if (((LLTexLayerParamAlphaInfo *)getInfo())->mSkipIfZeroWeight)
	{
		F32 effective_weight = (avatar->getSex() & getSex()) ? mCurWeight : getDefaultWeight();
		if (is_approx_zero(effective_weight)) 
		{
			return TRUE;
		}
	}

	EWearableType type = (EWearableType)getWearableType();
	if ((type != WT_INVALID) && !avatar->isWearingWearableType(type))
	{
		return TRUE;
	}

	return FALSE;
}


BOOL LLTexLayerParamAlpha::render(S32 x, S32 y, S32 width, S32 height)
{
	BOOL success = TRUE;

	if (!mTexLayer)
	{
		return success;
	}

	F32 effective_weight = (mTexLayer->getTexLayerSet()->getAvatar()->getSex() & getSex()) ? mCurWeight : getDefaultWeight();
	BOOL weight_changed = effective_weight != mCachedEffectiveWeight;
	if (getSkip())
	{
		return success;
	}

	LLTexLayerParamAlphaInfo *info = (LLTexLayerParamAlphaInfo *)getInfo();
	gGL.flush();
	if (info->mMultiplyBlend)
	{
		gGL.blendFunc(LLRender::BF_DEST_ALPHA, LLRender::BF_ZERO); // Multiplication: approximates a min() function
	}
	else
	{
		gGL.setSceneBlendType(LLRender::BT_ADD);  // Addition: approximates a max() function
	}

	if (!info->mStaticImageFileName.empty() && !mStaticImageInvalid)
	{
		if (mStaticImageTGA.isNull())
		{
			// Don't load the image file until we actually need it the first time.  Like now.
			mStaticImageTGA = LLTexLayerStaticImageList::getInstance()->getImageTGA(info->mStaticImageFileName);  
			// We now have something in one of our caches
			LLTexLayerSet::sHasCaches |= mStaticImageTGA.notNull() ? TRUE : FALSE;

			if (mStaticImageTGA.isNull())
			{
				llwarns << "Unable to load static file: " << info->mStaticImageFileName << llendl;
				mStaticImageInvalid = TRUE; // don't try again.
				return FALSE;
			}
		}

		const S32 image_tga_width = mStaticImageTGA->getWidth();
		const S32 image_tga_height = mStaticImageTGA->getHeight(); 
		if (!mCachedProcessedTexture ||
			(mCachedProcessedTexture->getWidth() != image_tga_width) ||
			(mCachedProcessedTexture->getHeight() != image_tga_height) ||
			(weight_changed))
		{
//			llinfos << "Building Cached Alpha: " << mName << ": (" << mStaticImageRaw->getWidth() << ", " << mStaticImageRaw->getHeight() << ") " << effective_weight << llendl;
			mCachedEffectiveWeight = effective_weight;

			if (!mCachedProcessedTexture)
			{
				mCachedProcessedTexture = LLViewerTextureManager::getLocalTexture(image_tga_width, image_tga_height, 1, FALSE);

				// We now have something in one of our caches
				LLTexLayerSet::sHasCaches |= mCachedProcessedTexture ? TRUE : FALSE;

				mCachedProcessedTexture->setExplicitFormat(GL_ALPHA8, GL_ALPHA);
			}

			// Applies domain and effective weight to data as it is decoded. Also resizes the raw image if needed.
			mStaticImageRaw = NULL;
			mStaticImageRaw = new LLImageRaw;
			mStaticImageTGA->decodeAndProcess(mStaticImageRaw, info->mDomain, effective_weight);
			mNeedsCreateTexture = TRUE;			
		}

		if (mCachedProcessedTexture)
		{
			{
				// Create the GL texture, and then hang onto it for future use.
				if (mNeedsCreateTexture)
				{
					mCachedProcessedTexture->createGLTexture(0, mStaticImageRaw);
					mNeedsCreateTexture = FALSE;
					gGL.getTexUnit(0)->bind(mCachedProcessedTexture);
					mCachedProcessedTexture->setAddressMode(LLTexUnit::TAM_CLAMP);
				}

				LLGLSNoAlphaTest gls_no_alpha_test;
				gGL.getTexUnit(0)->bind(mCachedProcessedTexture);
				gl_rect_2d_simple_tex(width, height);
				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				stop_glerror();
			}
		}

		// Don't keep the cache for other people's avatars
		// (It's not really a "cache" in that case, but the logic is the same)
		if (!mAvatar->isSelf())
		{
			mCachedProcessedTexture = NULL;
		}
	}
	else
	{
		LLGLDisable no_alpha(GL_ALPHA_TEST);
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f(0.f, 0.f, 0.f, effective_weight);
		gl_rect_2d_simple(width, height);
	}

	return success;
}

//-----------------------------------------------------------------------------
// LLTexLayerParamAlphaInfo
//-----------------------------------------------------------------------------
LLTexLayerParamAlphaInfo::LLTexLayerParamAlphaInfo() :
	mMultiplyBlend(FALSE),
	mSkipIfZeroWeight(FALSE),
	mDomain(0.f)
{
}

BOOL LLTexLayerParamAlphaInfo::parseXml(LLXmlTreeNode* node)
{
	llassert(node->hasName("param") && node->getChildByName("param_alpha"));

	if (!LLViewerVisualParamInfo::parseXml(node))
		return FALSE;

	LLXmlTreeNode* param_alpha_node = node->getChildByName("param_alpha");
	if (!param_alpha_node)
	{
		return FALSE;
	}

	static LLStdStringHandle tga_file_string = LLXmlTree::addAttributeString("tga_file");
	if (param_alpha_node->getFastAttributeString(tga_file_string, mStaticImageFileName))
	{
		// Don't load the image file until it's actually needed.
	}
//	else
//	{
//		llwarns << "<param_alpha> element is missing tga_file attribute." << llendl;
//	}
	
	static LLStdStringHandle multiply_blend_string = LLXmlTree::addAttributeString("multiply_blend");
	param_alpha_node->getFastAttributeBOOL(multiply_blend_string, mMultiplyBlend);

	static LLStdStringHandle skip_if_zero_string = LLXmlTree::addAttributeString("skip_if_zero");
	param_alpha_node->getFastAttributeBOOL(skip_if_zero_string, mSkipIfZeroWeight);

	static LLStdStringHandle domain_string = LLXmlTree::addAttributeString("domain");
	param_alpha_node->getFastAttributeF32(domain_string, mDomain);

	return TRUE;
}




LLTexLayerParamColor::LLTexLayerParamColor(LLTexLayerInterface* layer) :
	LLTexLayerParam(layer),
	mAvgDistortionVec(1.f, 1.f, 1.f)
{
}

LLTexLayerParamColor::LLTexLayerParamColor(LLVOAvatar *avatar) :
	LLTexLayerParam(avatar),
	mAvgDistortionVec(1.f, 1.f, 1.f)
{
}

LLTexLayerParamColor::~LLTexLayerParamColor()
{
}

/*virtual*/ LLViewerVisualParam* LLTexLayerParamColor::cloneParam(LLWearable* wearable) const
{
	LLTexLayerParamColor *new_param = new LLTexLayerParamColor(mTexLayer);
	*new_param = *this;
	return new_param;
}

LLColor4 LLTexLayerParamColor::getNetColor() const
{
	const LLTexLayerParamColorInfo *info = (LLTexLayerParamColorInfo *)getInfo();
	
	llassert(info->mNumColors >= 1);

	F32 effective_weight = (mAvatar && (mAvatar->getSex() & getSex())) ? mCurWeight : getDefaultWeight();

	S32 index_last = info->mNumColors - 1;
	F32 scaled_weight = effective_weight * index_last;
	S32 index_start = (S32) scaled_weight;
	S32 index_end = index_start + 1;
	if (index_start == index_last)
	{
		return info->mColors[index_last];
	}
	else
	{
		F32 weight = scaled_weight - index_start;
		const LLColor4 *start = &info->mColors[ index_start ];
		const LLColor4 *end   = &info->mColors[ index_end ];
		return LLColor4((1.f - weight) * start->mV[VX] + weight * end->mV[VX],
						(1.f - weight) * start->mV[VY] + weight * end->mV[VY],
						(1.f - weight) * start->mV[VZ] + weight * end->mV[VZ],
						(1.f - weight) * start->mV[VW] + weight * end->mV[VW]);
	}
}

void LLTexLayerParamColor::setWeight(F32 weight, BOOL upload_bake)
{
	if (mIsAnimating)
	{
		return;
	}

	const LLTexLayerParamColorInfo *info = (LLTexLayerParamColorInfo *)getInfo();
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	F32 new_weight = llclamp(weight, min_weight, max_weight);
	U8 cur_u8 = F32_to_U8(mCurWeight, min_weight, max_weight);
	U8 new_u8 = F32_to_U8(new_weight, min_weight, max_weight);
	if (cur_u8 != new_u8)
	{
		mCurWeight = new_weight;

		if (info->mNumColors <= 0)
		{
			// This will happen when we set the default weight the first time.
			return;
		}

		if ((mAvatar->getSex() & getSex()) && (mAvatar->isSelf() && !mIsDummy)) // only trigger a baked texture update if we're changing a wearable's visual param.
		{
			onGlobalColorChanged(upload_bake);
			if (mTexLayer)
			{
				mAvatar->invalidateComposite(mTexLayer->getTexLayerSet(), upload_bake);
			}
		}

//		llinfos << "param " << mName << " = " << new_weight << llendl;
	}
}

void LLTexLayerParamColor::setAnimationTarget(F32 target_value, BOOL upload_bake)
{ 
	// set value first then set interpolating flag to ignore further updates
	mTargetWeight = target_value; 
	setWeight(target_value, upload_bake);
	mIsAnimating = TRUE;
	if (mNext)
	{
		mNext->setAnimationTarget(target_value, upload_bake);
	}
}

void LLTexLayerParamColor::animate(F32 delta, BOOL upload_bake)
{
	if (mNext)
	{
		mNext->animate(delta, upload_bake);
	}
}

//-----------------------------------------------------------------------------
// LLTexLayerParamColorInfo
//-----------------------------------------------------------------------------
LLTexLayerParamColorInfo::LLTexLayerParamColorInfo() :
	mOperation(LLTexLayerParamColor::OP_ADD),
	mNumColors(0)
{
}

BOOL LLTexLayerParamColorInfo::parseXml(LLXmlTreeNode *node)
{
	llassert(node->hasName("param") && node->getChildByName("param_color"));

	if (!LLViewerVisualParamInfo::parseXml(node))
		return FALSE;

	LLXmlTreeNode* param_color_node = node->getChildByName("param_color");
	if (!param_color_node)
	{
		return FALSE;
	}

	std::string op_string;
	static LLStdStringHandle operation_string = LLXmlTree::addAttributeString("operation");
	if (param_color_node->getFastAttributeString(operation_string, op_string))
	{
		LLStringUtil::toLower(op_string);
		if		(op_string == "add") 		mOperation = LLTexLayerParamColor::OP_ADD;
		else if	(op_string == "multiply")	mOperation = LLTexLayerParamColor::OP_MULTIPLY;
		else if	(op_string == "blend")	    mOperation = LLTexLayerParamColor::OP_BLEND;
	}

	mNumColors = 0;

	LLColor4U color4u;
	for (LLXmlTreeNode* child = param_color_node->getChildByName("value");
		 child;
		 child = param_color_node->getNextNamedChild())
	{
		if ((mNumColors < MAX_COLOR_VALUES))
		{
			static LLStdStringHandle color_string = LLXmlTree::addAttributeString("color");
			if (child->getFastAttributeColor4U(color_string, color4u))
			{
				mColors[ mNumColors ].setVec(color4u);
				mNumColors++;
			}
		}
	}
	if (!mNumColors)
	{
		llwarns << "<param_color> is missing <value> sub-elements" << llendl;
		return FALSE;
	}

	if ((mOperation == LLTexLayerParamColor::OP_BLEND) && (mNumColors != 1))
	{
		llwarns << "<param_color> with operation\"blend\" must have exactly one <value>" << llendl;
		return FALSE;
	}
	
	return TRUE;
}
