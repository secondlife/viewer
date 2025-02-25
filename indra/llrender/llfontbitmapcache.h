/**
 * @file llfontbitmapcache.h
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

#ifndef LL_LLFONTBITMAPCACHE_H
#define LL_LLFONTBITMAPCACHE_H

#include <vector>
#include "lltrace.h"

enum class EFontGlyphType : U32
{
    Grayscale = 0,
    Color,
    Count,
    Unspecified,
};

// Maintain a collection of bitmaps containing rendered glyphs.
// Generalizes the single-bitmap logic from LLFontFreetype and LLFontGL.
class LLFontBitmapCache
{
public:
    LLFontBitmapCache();
    ~LLFontBitmapCache();

    // Need to call this once, before caching any glyphs.
    void init(S32 max_char_width,
              S32 max_char_height);

    void reset();

    bool nextOpenPos(S32 width, S32& posX, S32& posY, EFontGlyphType bitmapType, U32& bitmapNum);

    void destroyGL();

    LLImageRaw* getImageRaw(EFontGlyphType bitmapType, U32 bitmapNum) const;
    LLImageGL* getImageGL(EFontGlyphType bitmapType, U32 bitmapNum) const;

    S32 getMaxCharWidth() const { return mMaxCharWidth; }
    U32 getNumBitmaps(EFontGlyphType bitmapType) const { return (bitmapType < EFontGlyphType::Count) ? static_cast<U32>(mImageRawVec[static_cast<U32>(bitmapType)].size()) : 0U; }
    S32 getBitmapWidth() const { return mBitmapWidth; }
    S32 getBitmapHeight() const { return mBitmapHeight; }
    S32 getCacheGeneration() const { return mGeneration; }

protected:
    static U32 getNumComponents(EFontGlyphType bitmap_type);

private:
    S32 mBitmapWidth = 0;
    S32 mBitmapHeight = 0;
    S32 mCurrentOffsetX[static_cast<U32>(EFontGlyphType::Count)] = { 1 };
    S32 mCurrentOffsetY[static_cast<U32>(EFontGlyphType::Count)] = { 1 };
    S32 mMaxCharWidth = 0;
    S32 mMaxCharHeight = 0;
    S32 mGeneration = 0;
    std::vector<LLPointer<LLImageRaw>> mImageRawVec[static_cast<U32>(EFontGlyphType::Count)];
    std::vector<LLPointer<LLImageGL>> mImageGLVec[static_cast<U32>(EFontGlyphType::Count)];
};

#endif //LL_LLFONTBITMAPCACHE_H
