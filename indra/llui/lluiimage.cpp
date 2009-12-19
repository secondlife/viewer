/** 
 * @file lluiimage.cpp
 * @brief UI implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

// Utilities functions the user interface needs

//#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

// Project includes
#include "lluiimage.h"
#include "llui.h"

LLUIImage::LLUIImage(const std::string& name, LLPointer<LLTexture> image)
:	mName(name),
	mImage(image),
	mScaleRegion(0.f, 1.f, 1.f, 0.f),
	mClipRegion(0.f, 1.f, 1.f, 0.f),
	mUniformScaling(TRUE),
	mNoClip(TRUE),
	mImageLoaded(NULL)
{
}

LLUIImage::~LLUIImage()
{
	delete mImageLoaded;
}

void LLUIImage::setClipRegion(const LLRectf& region) 
{ 
	mClipRegion = region; 
	mNoClip = mClipRegion.mLeft == 0.f
				&& mClipRegion.mRight == 1.f
				&& mClipRegion.mBottom == 0.f
				&& mClipRegion.mTop == 1.f;
}

void LLUIImage::setScaleRegion(const LLRectf& region) 
{ 
	mScaleRegion = region; 
	mUniformScaling = mScaleRegion.mLeft == 0.f
					&& mScaleRegion.mRight == 1.f
					&& mScaleRegion.mBottom == 0.f
					&& mScaleRegion.mTop == 1.f;
}

//TODO: move drawing implementation inside class
void LLUIImage::draw(S32 x, S32 y, const LLColor4& color) const
{
	gl_draw_scaled_image(x, y, getWidth(), getHeight(), mImage, color, mClipRegion);
}

void LLUIImage::draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
	if (mUniformScaling)
	{
		gl_draw_scaled_image(x, y, width, height, mImage, color, mClipRegion);
	}
	else
	{
		gl_draw_scaled_image_with_border(
			x, y, 
			width, height, 
			mImage, 
			color,
			FALSE,
			mClipRegion,
			mScaleRegion);
	}
}

void LLUIImage::drawSolid(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const
{
	gl_draw_scaled_image_with_border(
		x, y, 
		width, height, 
		mImage, 
		color, 
		TRUE,
		mClipRegion,
		mScaleRegion);
}

void LLUIImage::drawBorder(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, S32 border_width) const
{
	LLRect border_rect;
	border_rect.setOriginAndSize(x, y, width, height);
	border_rect.stretch(border_width, border_width);
	drawSolid(border_rect, color);
}

S32 LLUIImage::getWidth() const
{ 
	// return clipped dimensions of actual image area
	return llround((F32)mImage->getWidth(0) * mClipRegion.getWidth()); 
}

S32 LLUIImage::getHeight() const
{ 
	// return clipped dimensions of actual image area
	return llround((F32)mImage->getHeight(0) * mClipRegion.getHeight()); 
}

S32 LLUIImage::getTextureWidth() const
{
	return mImage->getWidth(0);
}

S32 LLUIImage::getTextureHeight() const
{
	return mImage->getHeight(0);
}

boost::signals2::connection LLUIImage::addLoadedCallback( const image_loaded_signal_t::slot_type& cb ) 
{
	if (!mImageLoaded) 
	{
		mImageLoaded = new image_loaded_signal_t();
	}
	return mImageLoaded->connect(cb);
}


void LLUIImage::onImageLoaded()
{
	if (mImageLoaded)
	{
		(*mImageLoaded)();
	}
}


namespace LLInitParam
{
	void TypedParam<LLUIImage*>::setValueFromBlock() const
	{
		// The keyword "none" is specifically requesting a null image
		// do not default to current value. Used to overwrite template images. 
		if (name() == "none")
		{
			mData.mValue = NULL;
		}

		LLUIImage* imagep =  LLUI::getUIImage(name());
		if (imagep)
		{
			mData.mValue = imagep;
		}
	}
	
	void TypedParam<LLUIImage*>::setBlockFromValue()
	{
		if (mData.mValue == NULL)
		{
			name = "none";
		}
		else
		{
			name = mData.mValue->getName();
		}
	}

	
	bool ParamCompare<LLUIImage*, false>::equals(
		LLUIImage* const &a,
		LLUIImage* const &b)
	{
		// force all LLUIImages for XML UI export to be "non-default"
		if (!a && !b)
			return false;
		else
			return (a == b);
	}
}

