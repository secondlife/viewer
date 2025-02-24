/**
 * @file llfontbitmapcache.cpp
 * @brief Storage for previously rendered glyphs.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "linden_common.h"

#include "llgl.h"
#include "llfontbitmapcache.h"

LLFontBitmapCache::LLFontBitmapCache()

{
}

LLFontBitmapCache::~LLFontBitmapCache()
{
}

void LLFontBitmapCache::init(S32 max_char_width,
                             S32 max_char_height)
{
    reset();

    mMaxCharWidth = max_char_width;
    mMaxCharHeight = max_char_height;

    S32 image_width = mMaxCharWidth * 20;
    S32 pow_iw = 2;
    while (pow_iw < image_width)
    {
        pow_iw <<= 1;
    }
    image_width = pow_iw;
    image_width = llmin(512, image_width); // Don't make bigger than 512x512, ever.

    mBitmapWidth = image_width;
    mBitmapHeight = image_width;
}

LLImageRaw *LLFontBitmapCache::getImageRaw(EFontGlyphType bitmap_type, U32 bitmap_num) const
{
    const U32 bitmap_idx = static_cast<U32>(bitmap_type);
    if (bitmap_type >= EFontGlyphType::Count || bitmap_num >= mImageRawVec[bitmap_idx].size())
        return nullptr;

    return mImageRawVec[bitmap_idx][bitmap_num];
}

LLImageGL *LLFontBitmapCache::getImageGL(EFontGlyphType bitmap_type, U32 bitmap_num) const
{
    const U32 bitmap_idx = static_cast<U32>(bitmap_type);
    if (bitmap_type >= EFontGlyphType::Count || bitmap_num >= mImageGLVec[bitmap_idx].size())
        return nullptr;

    return mImageGLVec[bitmap_idx][bitmap_num];
}


bool LLFontBitmapCache::nextOpenPos(S32 width, S32& pos_x, S32& pos_y, EFontGlyphType bitmap_type, U32& bitmap_num)
{
    if (bitmap_type >= EFontGlyphType::Count)
    {
        return false;
    }

    const U32 bitmap_idx = static_cast<U32>(bitmap_type);
    if (mImageRawVec[bitmap_idx].empty() || (mCurrentOffsetX[bitmap_idx] + width + 1) > mBitmapWidth)
    {
        if ((mImageRawVec[bitmap_idx].empty()) || (mCurrentOffsetY[bitmap_idx] + 2*mMaxCharHeight + 2) > mBitmapHeight)
        {
            // We're out of space in the current image, or no image
            // has been allocated yet.  Make a new one.

            S32 image_width = mMaxCharWidth * 20;
            S32 pow_iw = 2;
            while (pow_iw < image_width)
            {
                pow_iw *= 2;
            }
            image_width = pow_iw;
            image_width = llmin(512, image_width); // Don't make bigger than 512x512, ever.
            S32 image_height = image_width;

            mBitmapWidth = image_width;
            mBitmapHeight = image_height;

            S32 num_components = getNumComponents(bitmap_type);
            mImageRawVec[bitmap_idx].emplace_back(new LLImageRaw(mBitmapWidth, mBitmapHeight, num_components));
            bitmap_num = static_cast<U32>(mImageRawVec[bitmap_idx].size()) - 1;

            LLImageRaw* image_raw = getImageRaw(bitmap_type, bitmap_num);
            if (EFontGlyphType::Grayscale == bitmap_type)
            {
                image_raw->clear(255, 0);
            }

            // Make corresponding GL image.
            mImageGLVec[bitmap_idx].emplace_back(new LLImageGL(image_raw, false, false));
            LLImageGL* image_gl = getImageGL(bitmap_type, bitmap_num);

            // Start at beginning of the new image.
            mCurrentOffsetX[bitmap_idx] = 1;
            mCurrentOffsetY[bitmap_idx] = 1;

            // Attach corresponding GL texture. (*TODO: is this needed?)
            gGL.getTexUnit(0)->bind(image_gl);
            image_gl->setFilteringOption(LLTexUnit::TFO_POINT); // was setMipFilterNearest(true, true);
        }
        else
        {
            // Move to next row in current image.
            mCurrentOffsetX[bitmap_idx] = 1;
            mCurrentOffsetY[bitmap_idx] += mMaxCharHeight + 1;
        }
    }

    pos_x = mCurrentOffsetX[bitmap_idx];
    pos_y = mCurrentOffsetY[bitmap_idx];
    bitmap_num = getNumBitmaps(bitmap_type) - 1;

    mCurrentOffsetX[bitmap_idx] += width + 1;
    mGeneration++;

    return true;
}

void LLFontBitmapCache::destroyGL()
{
    for (U32 idx = 0, cnt = static_cast<U32>(EFontGlyphType::Count); idx < cnt; idx++)
    {
        for (LLImageGL* image_gl : mImageGLVec[idx])
        {
            image_gl->destroyGLTexture();
        }
    }
}

void LLFontBitmapCache::reset()
{
    for (U32 idx = 0, cnt = static_cast<U32>(EFontGlyphType::Count); idx < cnt; idx++)
    {
        mImageRawVec[idx].clear();
        mImageGLVec[idx].clear();
        mCurrentOffsetX[idx] = 1;
        mCurrentOffsetY[idx] = 1;
    }

    mBitmapWidth = 0;
    mBitmapHeight = 0;
    mGeneration++;
}

//static
U32 LLFontBitmapCache::getNumComponents(EFontGlyphType bitmap_type)
{
    switch (bitmap_type)
    {
        case EFontGlyphType::Grayscale:
            return 2;
        case EFontGlyphType::Color:
            return 4;
        default:
            llassert(false);
            return 2;
    }
}
