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

// Maintain a collection of bitmaps containing rendered glyphs.
// Generalizes the single-bitmap logic from LLFontFreetype and LLFontGL.
class LLFontBitmapCache
{
public:
    LLFontBitmapCache();
    ~LLFontBitmapCache();

    // Need to call this once, before caching any glyphs.
    void init(S32 num_components,
              S32 max_char_width,
              S32 max_char_height);

    void reset();

    BOOL nextOpenPos(S32 width, S32 &posX, S32 &posY, S32 &bitmapNum);
    
    void destroyGL();
    
    LLImageRaw *getImageRaw(U32 bitmapNum = 0) const;
    LLImageGL *getImageGL(U32 bitmapNum = 0) const;
    
    S32 getMaxCharWidth() const { return mMaxCharWidth; }
    S32 getNumComponents() const { return mNumComponents; }
    S32 getBitmapWidth() const { return mBitmapWidth; }
    S32 getBitmapHeight() const { return mBitmapHeight; }

private:
    S32 mNumComponents;
    S32 mBitmapWidth;
    S32 mBitmapHeight;
    S32 mBitmapNum;
    S32 mMaxCharWidth;
    S32 mMaxCharHeight;
    S32 mCurrentOffsetX;
    S32 mCurrentOffsetY;
    std::vector<LLPointer<LLImageRaw> > mImageRawVec;
    std::vector<LLPointer<LLImageGL> > mImageGLVec;
};

#endif //LL_LLFONTBITMAPCACHE_H
