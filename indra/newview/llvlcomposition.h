/** 
 * @file llvlcomposition.h
 * @brief Viewer-side representation of a composition layer...
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLVLCOMPOSITION_H
#define LL_LLVLCOMPOSITION_H

#include "llviewerlayer.h"
#include "llviewertexture.h"

class LLSurface;

class LLVLComposition : public LLViewerLayer
{
public:
    LLVLComposition(LLSurface *surfacep, const U32 width, const F32 scale);
    /*virtual*/ ~LLVLComposition();

    void setSurface(LLSurface *surfacep);

    // Viewer side hack to generate composition values
    BOOL generateHeights(const F32 x, const F32 y, const F32 width, const F32 height);
    BOOL generateComposition();
    // Generate texture from composition values.
    BOOL generateTexture(const F32 x, const F32 y, const F32 width, const F32 height);      

    // Use these as indeces ito the get/setters below that use 'corner'
    enum ECorner
    {
        SOUTHWEST = 0,
        SOUTHEAST = 1,
        NORTHWEST = 2,
        NORTHEAST = 3,
        CORNER_COUNT = 4
    };
    LLUUID getDetailTextureID(S32 corner);
    LLViewerFetchedTexture* getDetailTexture(S32 corner);
    F32 getStartHeight(S32 corner);
    F32 getHeightRange(S32 corner);

    void setDetailTextureID(S32 corner, const LLUUID& id);
    void setStartHeight(S32 corner, F32 start_height);
    void setHeightRange(S32 corner, F32 range);

    friend class LLVOSurfacePatch;
    friend class LLDrawPoolTerrain;
    void setParamsReady()       { mParamsReady = TRUE; }
    BOOL getParamsReady() const { return mParamsReady; }
protected:
    BOOL mParamsReady;
    LLSurface *mSurfacep;
    BOOL mTexturesLoaded;

    LLPointer<LLViewerFetchedTexture> mDetailTextures[CORNER_COUNT];
    LLPointer<LLImageRaw> mRawImages[CORNER_COUNT];

    F32 mStartHeight[CORNER_COUNT];
    F32 mHeightRange[CORNER_COUNT];

    F32 mTexScaleX;
    F32 mTexScaleY;
};

#endif //LL_LLVLCOMPOSITION_H
