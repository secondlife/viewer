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
#include "llpointer.h"

#include "llimage.h"

class LLSurface;

class LLViewerFetchedTexture;
class LLFetchedGLTFMaterial;

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

	// Heights map into textures (or materials) as 0-1 = first, 1-2 = second, etc.
	// So we need to compress heights into this range.
    static const S32 ASSET_COUNT = 4;

	// Use these as indeces ito the get/setters below that use 'corner'
	enum ECorner
	{
		SOUTHWEST = 0,
		SOUTHEAST = 1,
		NORTHWEST = 2,
		NORTHEAST = 3,
		CORNER_COUNT = 4
	};

	LLUUID getDetailAssetID(S32 asset);
	F32 getStartHeight(S32 corner);
	F32 getHeightRange(S32 corner);

	void setDetailAssetID(S32 asset, const LLUUID& id);
	void setStartHeight(S32 corner, F32 start_height);
	void setHeightRange(S32 corner, F32 range);

	friend class LLVOSurfacePatch;
	friend class LLDrawPoolTerrain;
	void setParamsReady()		{ mParamsReady = TRUE; }
	BOOL getParamsReady() const	{ return mParamsReady; }
    BOOL texturesReady(BOOL boost = FALSE);
    BOOL materialsReady(BOOL boost = FALSE);

protected:
    static BOOL textureReady(LLPointer<LLViewerFetchedTexture>& tex, BOOL boost = FALSE);
    static BOOL materialReady(LLPointer<LLFetchedGLTFMaterial>& mat, bool& textures_set, BOOL boost = FALSE);

	BOOL mParamsReady = FALSE;
	LLSurface *mSurfacep;

	LLPointer<LLViewerFetchedTexture> mDetailTextures[ASSET_COUNT];
	LLPointer<LLImageRaw> mRawImages[ASSET_COUNT];
	LLPointer<LLFetchedGLTFMaterial> mDetailMaterials[ASSET_COUNT];
    bool mMaterialTexturesSet[ASSET_COUNT];

	F32 mStartHeight[CORNER_COUNT];
	F32 mHeightRange[CORNER_COUNT];

	F32 mTexScaleX = 16.f;
	F32 mTexScaleY = 16.f;
};

#endif //LL_LLVLCOMPOSITION_H
