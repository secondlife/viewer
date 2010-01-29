/** 
 * @file llfontbitmapcache.cpp
 * @brief Storage for previously rendered glyphs.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llgl.h"
#include "llfontbitmapcache.h"

LLFontBitmapCache::LLFontBitmapCache():
	mNumComponents(0),
	mBitmapWidth(0),
	mBitmapHeight(0),
	mBitmapNum(-1),
	mMaxCharWidth(0),
	mMaxCharHeight(0),
	mCurrentOffsetX(1),
	mCurrentOffsetY(1)
{
}

LLFontBitmapCache::~LLFontBitmapCache()
{
}

void LLFontBitmapCache::init(S32 num_components,
							 S32 max_char_width,
							 S32 max_char_height)
{
	reset();
	
	mNumComponents = num_components;
	mMaxCharWidth = max_char_width;
	mMaxCharHeight = max_char_height;
}

LLImageRaw *LLFontBitmapCache::getImageRaw(U32 bitmap_num) const
{
	if (bitmap_num >= mImageRawVec.size())
		return NULL;

	return mImageRawVec[bitmap_num];
}

LLImageGL *LLFontBitmapCache::getImageGL(U32 bitmap_num) const
{
	if (bitmap_num >= mImageGLVec.size())
		return NULL;

	return mImageGLVec[bitmap_num];
}


BOOL LLFontBitmapCache::nextOpenPos(S32 width, S32 &pos_x, S32 &pos_y, S32& bitmap_num)
{
	if ((mBitmapNum<0) || (mCurrentOffsetX + width + 1) > mBitmapWidth)
	{
		if ((mBitmapNum<0) || (mCurrentOffsetY + 2*mMaxCharHeight + 2) > mBitmapHeight)
		{
			// We're out of space in the current image, or no image
			// has been allocated yet.  Make a new one.
			mImageRawVec.push_back(new LLImageRaw);
			mBitmapNum = mImageRawVec.size()-1;
			LLImageRaw *image_raw = getImageRaw(mBitmapNum);

			// Make corresponding GL image.
			mImageGLVec.push_back(new LLImageGL(FALSE));
			LLImageGL *image_gl = getImageGL(mBitmapNum);
			
			S32 image_width = mMaxCharWidth * 20;
			S32 pow_iw = 2;
			while (pow_iw < image_width)
			{
				pow_iw *= 2;
			}
			image_width = pow_iw;
			image_width = llmin(512, image_width); // Don't make bigger than 512x512, ever.
			S32 image_height = image_width;

			image_raw->resize(image_width, image_height, mNumComponents);

			mBitmapWidth = image_width;
			mBitmapHeight = image_height;

			switch (mNumComponents)
			{
				case 1:
					image_raw->clear();
				break;
				case 2:
					image_raw->clear(255, 0);
				break;
			}

			// Start at beginning of the new image.
			mCurrentOffsetX = 1;
			mCurrentOffsetY = 1;

			// Attach corresponding GL texture.
			image_gl->createGLTexture(0, image_raw);
			gGL.getTexUnit(0)->bind(image_gl);
			image_gl->setFilteringOption(LLTexUnit::TFO_POINT); // was setMipFilterNearest(TRUE, TRUE);
		}
		else
		{
			// Move to next row in current image.
			mCurrentOffsetX = 1;
			mCurrentOffsetY += mMaxCharHeight + 1;
		}
	}

	pos_x = mCurrentOffsetX;
	pos_y = mCurrentOffsetY;
	bitmap_num = mBitmapNum;

	mCurrentOffsetX += width + 1;

	return TRUE;
}

void LLFontBitmapCache::destroyGL()
{
	for (std::vector<LLPointer<LLImageGL> >::iterator it = mImageGLVec.begin();
		 it != mImageGLVec.end(); ++it)
	{
		(*it)->destroyGLTexture();
	}
}

void LLFontBitmapCache::reset()
{
	mImageRawVec.clear();
	mImageGLVec.clear();
	
	mBitmapWidth = 0;
	mBitmapHeight = 0;
	mBitmapNum = -1;
	mCurrentOffsetX = 1;
	mCurrentOffsetY = 1;
}

