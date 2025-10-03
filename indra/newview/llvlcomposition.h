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

#include "llfetchedgltfmaterial.h"
#include "llimage.h"
#include "llpointer.h"
#include "llterrainpaintmap.h"
#include "llviewerlayer.h"
#include "llviewershadermgr.h"
#include "llviewertexture.h"

class LLSurface;

class LLViewerFetchedTexture;

class LLModifyRegion
{
public:
    virtual const LLGLTFMaterial* getMaterialOverride(S32 asset) const = 0;
};

// The subset of the composition used by local terrain debug materials (gLocalTerrainMaterials)
class LLTerrainMaterials : public LLModifyRegion
{
public:
    friend class LLDrawPoolTerrain;

    LLTerrainMaterials() {}
    virtual ~LLTerrainMaterials();

    void apply(const LLModifyRegion& other);

    // Heights map into textures (or materials) as 0-1 = first, 1-2 = second, etc.
    // So we need to compress heights into this range.
    static const S32 ASSET_COUNT = 4;

    enum class Type
    {
        TEXTURE,
        PBR,
        COUNT
    };

    bool generateMaterials();

    void boost();

    virtual LLUUID getDetailAssetID(S32 asset);
    virtual void setDetailAssetID(S32 asset, const LLUUID& id);
    const LLGLTFMaterial* getMaterialOverride(S32 asset) const override;
    virtual void setMaterialOverride(S32 asset, LLGLTFMaterial* mat_override);
    Type getMaterialType();
    bool makeTexturesReady(bool boost, bool strict);
    // strict = true -> all materials must be sufficiently loaded
    // strict = false -> at least one material must be loaded
    bool makeMaterialsReady(bool boost, bool strict);

    // See TerrainPaintType
    U32 getPaintType() const { return mPaintType; }
    void setPaintType(U32 paint_type) { mPaintType = paint_type; }
    LLViewerTexture* getPaintMap();
    void setPaintMap(LLViewerTexture* paint_map);

protected:
    void unboost();
    static bool makeTextureReady(LLPointer<LLViewerFetchedTexture>& tex, bool boost);
    // strict = true -> all materials must be sufficiently loaded
    // strict = false -> at least one material must be loaded
    static bool materialTexturesReady(LLPointer<LLFetchedGLTFMaterial>& mat, bool& textures_set, bool boost, bool strict);

    LLPointer<LLViewerFetchedTexture> mDetailTextures[ASSET_COUNT];
    // *NOTE: Unlike mDetailRenderMaterials, the textures in this are not
    // guaranteed to be set or loaded after a true return from
    // makeMaterialsReady.
    LLPointer<LLFetchedGLTFMaterial> mDetailMaterials[ASSET_COUNT];
    LLPointer<LLGLTFMaterial> mDetailMaterialOverrides[ASSET_COUNT];
    LLPointer<LLFetchedGLTFMaterial> mDetailRenderMaterials[ASSET_COUNT];

    U32 mPaintType = TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE;
    LLPointer<LLViewerTexture> mPaintMap;
};

// Local materials to override all regions
extern LLTerrainMaterials gLocalTerrainMaterials;

class LLVLComposition : public LLTerrainMaterials, public LLViewerLayer
{
public:
    // Heights map into textures (or materials) as 0-1 = first, 1-2 = second, etc.
    // So we need to compress heights into this range.
    static const S32 ASSET_COUNT = 4;
    static const LLUUID (&getDefaultTextures())[ASSET_COUNT];

    LLVLComposition(LLSurface *surfacep, const U32 width, const F32 scale);
    /*virtual*/ ~LLVLComposition();

    void setSurface(LLSurface *surfacep);

    // Viewer side hack to generate composition values
    bool generateHeights(const F32 x, const F32 y, const F32 width, const F32 height);
    bool generateComposition();

    // Use these as indeces ito the get/setters below that use 'corner'
    enum ECorner
    {
        SOUTHWEST = 0,
        SOUTHEAST = 1,
        NORTHWEST = 2,
        NORTHEAST = 3,
        CORNER_COUNT = 4
    };

    void setDetailAssetID(S32 asset, const LLUUID& id) override;
    F32 getStartHeight(S32 corner);
    F32 getHeightRange(S32 corner);

    void setStartHeight(S32 corner, F32 start_height);
    void setHeightRange(S32 corner, F32 range);

    friend class LLVOSurfacePatch;
    friend class LLDrawPoolTerrain;
    void setParamsReady()       { mParamsReady = true; }
    bool getParamsReady() const { return mParamsReady; }

protected:
    bool mParamsReady = false;
    LLSurface *mSurfacep;

    // Final minimap raw images
    LLPointer<LLImageRaw> mRawImages[LLTerrainMaterials::ASSET_COUNT];

    // Only non-null during minimap tile generation
    LLPointer<LLImageRaw> mRawImagesBaseColor[LLTerrainMaterials::ASSET_COUNT];
    LLPointer<LLImageRaw> mRawImagesEmissive[LLTerrainMaterials::ASSET_COUNT];

    F32 mStartHeight[CORNER_COUNT];
    F32 mHeightRange[CORNER_COUNT];

    F32 mTexScaleX = 16.f;
    F32 mTexScaleY = 16.f;
};

#endif //LL_LLVLCOMPOSITION_H
