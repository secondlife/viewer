/**
 * @file llvlcomposition.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llvlcomposition.h"

#include <functional>

#include "llerror.h"
#include "v3math.h"
#include "llsurface.h"
#include "lltextureview.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llfetchedgltfmaterial.h"
#include "llgltfmateriallist.h"
#include "llviewerregion.h"
#include "noise.h"
#include "llregionhandle.h" // for from_region_handle
#include "llviewercontrol.h"


extern LLColor4U MAX_WATER_COLOR;

static const U32 BASE_SIZE = 128;
static const F32 TERRAIN_DECODE_PRIORITY = 2048.f * 2048.f;

namespace
{
    F32 bilinear(const F32 v00, const F32 v01, const F32 v10, const F32 v11, const F32 x_frac, const F32 y_frac)
    {
        // Not sure if this is the right math...
        // Take weighted average of all four points (bilinear interpolation)
        F32 result;

        const F32 inv_x_frac = 1.f - x_frac;
        const F32 inv_y_frac = 1.f - y_frac;
        result = inv_x_frac*inv_y_frac*v00
                + x_frac*inv_y_frac*v10
                + inv_x_frac*y_frac*v01
                + x_frac*y_frac*v11;

        return result;
    }

    void boost_minimap_texture(LLViewerFetchedTexture* tex, F32 virtual_size)
    {
        llassert(tex);
        if (!tex) { return; }

        tex->setBoostLevel(LLGLTexture::BOOST_TERRAIN); // in case the raw image is at low detail
        tex->addTextureStats(virtual_size); // priority
    }

    void boost_minimap_material(LLFetchedGLTFMaterial* mat, F32 virtual_size)
    {
        if (!mat) { return; }
        if (mat->mBaseColorTexture) { boost_minimap_texture(mat->mBaseColorTexture, virtual_size); }
        if (mat->mNormalTexture) { boost_minimap_texture(mat->mNormalTexture, virtual_size); }
        if (mat->mMetallicRoughnessTexture) { boost_minimap_texture(mat->mMetallicRoughnessTexture, virtual_size); }
        if (mat->mEmissiveTexture) { boost_minimap_texture(mat->mEmissiveTexture, virtual_size); }
    }

    void unboost_minimap_texture(LLViewerFetchedTexture* tex)
    {
        if (!tex) { return; }
        tex->setBoostLevel(LLGLTexture::BOOST_NONE);
        tex->setMinDiscardLevel(MAX_DISCARD_LEVEL + 1);
    }

    void unboost_minimap_material(LLFetchedGLTFMaterial* mat)
    {
        if (!mat) { return; }
        if (mat->mBaseColorTexture) { unboost_minimap_texture(mat->mBaseColorTexture); }
        if (mat->mNormalTexture) { unboost_minimap_texture(mat->mNormalTexture); }
        if (mat->mMetallicRoughnessTexture) { unboost_minimap_texture(mat->mMetallicRoughnessTexture); }
        if (mat->mEmissiveTexture) { unboost_minimap_texture(mat->mEmissiveTexture); }
    }
};

LLTerrainMaterials::~LLTerrainMaterials()
{
    unboost();
}

void LLTerrainMaterials::apply(const LLModifyRegion& other)
{
    for (S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        const LLGLTFMaterial* other_override = other.getMaterialOverride(i);
        LLGLTFMaterial* material_override = other_override ? new LLGLTFMaterial(*other_override) : nullptr;
        setMaterialOverride(i, material_override);
    }
}

bool LLTerrainMaterials::generateMaterials()
{
    if (makeTexturesReady(true, true))
    {
        return true;
    }

    if (makeMaterialsReady(true, true))
    {
        return true;
    }

    return false;
}

void LLTerrainMaterials::boost()
{
    for (S32 i = 0; i < ASSET_COUNT; ++i)
    {
        LLPointer<LLViewerFetchedTexture>& tex = mDetailTextures[i];
        llassert(tex.notNull());
        boost_minimap_texture(tex, TERRAIN_DECODE_PRIORITY);

        LLPointer<LLFetchedGLTFMaterial>& mat = mDetailMaterials[i];
        boost_minimap_material(mat, TERRAIN_DECODE_PRIORITY);
    }
}

void LLTerrainMaterials::unboost()
{
    for (S32 i = 0; i < ASSET_COUNT; ++i)
    {
        LLPointer<LLViewerFetchedTexture>& tex = mDetailTextures[i];
        unboost_minimap_texture(tex);

        LLPointer<LLFetchedGLTFMaterial>& mat = mDetailMaterials[i];
        unboost_minimap_material(mat);
    }
}

LLUUID LLTerrainMaterials::getDetailAssetID(S32 asset)
{
    llassert(mDetailTextures[asset] && mDetailMaterials[asset]);
    // Assume both the the material and texture were fetched in the same way
    // using the same UUID. However, we may not know at this point which one
    // will load.
    return mDetailTextures[asset] ? mDetailTextures[asset]->getID() : LLUUID::null;
}

LLPointer<LLViewerFetchedTexture> fetch_terrain_texture(const LLUUID& id)
{
    if (id.isNull())
    {
        return nullptr;
    }

    LLPointer<LLViewerFetchedTexture> tex = LLViewerTextureManager::getFetchedTexture(id);
    return tex;
}

void LLTerrainMaterials::setDetailAssetID(S32 asset, const LLUUID& id)
{
    // *NOTE: If there were multiple terrain swatches using the same asset
    // ID, the asset still in use will be temporarily unboosted.
    // It will be boosted again during terrain rendering.
    unboost_minimap_texture(mDetailTextures[asset]);
    unboost_minimap_material(mDetailMaterials[asset]);

    // This is terrain texture, but we are not setting it as BOOST_TERRAIN
    // since we will be manipulating it later as needed.
    mDetailTextures[asset] = fetch_terrain_texture(id);
    LLPointer<LLFetchedGLTFMaterial>& mat = mDetailMaterials[asset];
    mat = id.isNull() ? nullptr : gGLTFMaterialList.getMaterial(id);
    mDetailRenderMaterials[asset] = nullptr;
}

const LLGLTFMaterial* LLTerrainMaterials::getMaterialOverride(S32 asset) const
{
    return mDetailMaterialOverrides[asset];
}

void LLTerrainMaterials::setMaterialOverride(S32 asset, LLGLTFMaterial* mat_override)
{
    // Non-null overrides must be nontrivial. Otherwise, please set the override to null instead.
    llassert(!mat_override || *mat_override != LLGLTFMaterial::sDefault);

    mDetailMaterialOverrides[asset] = mat_override;
    mDetailRenderMaterials[asset] = nullptr;
}

LLTerrainMaterials::Type LLTerrainMaterials::getMaterialType()
{
    LL_PROFILE_ZONE_SCOPED;

    const bool use_textures = makeTexturesReady(false, false) || !makeMaterialsReady(false, false);
    return use_textures ? Type::TEXTURE : Type::PBR;
}

bool LLTerrainMaterials::makeTexturesReady(bool boost, bool strict)
{
    bool ready[ASSET_COUNT];
    // *NOTE: Calls to makeTextureReady may boost textures. Do not early-return.
    for (S32 i = 0; i < ASSET_COUNT; i++)
    {
        ready[i] = mDetailTextures[i].notNull() && makeTextureReady(mDetailTextures[i], boost);
    }

    bool one_ready = false;
    for (S32 i = 0; i < ASSET_COUNT; i++)
    {
        const bool current_ready = ready[i];
        one_ready = one_ready || current_ready;
        if (!current_ready && strict)
        {
            return false;
        }
    }
    return one_ready;
}

namespace
{
    bool material_asset_ready(LLFetchedGLTFMaterial* mat) { return mat && mat->isLoaded(); }
};

bool LLTerrainMaterials::makeMaterialsReady(bool boost, bool strict)
{
    bool ready[ASSET_COUNT];
    // *NOTE: This section may boost materials/textures. Do not early-return if ready[i] is false.
    for (S32 i = 0; i < ASSET_COUNT; i++)
    {
        ready[i] = false;
        LLPointer<LLFetchedGLTFMaterial>& mat = mDetailMaterials[i];
        if (!material_asset_ready(mat)) { continue; }

        LLPointer<LLFetchedGLTFMaterial>& render_mat = mDetailRenderMaterials[i];
        // This will be mutated by materialTexturesReady, due to the way that
        // function is implemented.
        bool render_material_textures_set = bool(render_mat);
        if (!render_mat)
        {
            render_mat = new LLFetchedGLTFMaterial();
            *render_mat = *mat;
            // This render_mat is effectively already loaded, because it gets its data from mat.
            // However, its textures may not be loaded yet.
            render_mat->materialBegin();
            render_mat->materialComplete(true);

            LLPointer<LLGLTFMaterial>& override_mat = mDetailMaterialOverrides[i];
            if (override_mat)
            {
                render_mat->applyOverride(*override_mat);
            }
        }

        ready[i] = materialTexturesReady(render_mat, render_material_textures_set, boost, strict);
        llassert(render_material_textures_set);
    }

#if 1
    static LLCachedControl<bool> sRenderTerrainPBREnabled(gSavedSettings, "RenderTerrainPBREnabled", false);
    static LLCachedControl<bool> sRenderTerrainPBRForce(gSavedSettings, "RenderTerrainPBRForce", false);
    if (sRenderTerrainPBREnabled && sRenderTerrainPBRForce)
    {
        bool defined = true;
        for (S32 i = 0; i < ASSET_COUNT; i++)
        {
            if (!mDetailMaterials[i])
            {
                defined = false;
                break;
            }
        }
        if (defined)
        {
            return true;
        }
    }
#endif

    bool one_ready = false;
    for (S32 i = 0; i < ASSET_COUNT; i++)
    {
        const bool current_ready = ready[i];
        one_ready = one_ready || current_ready;
        if (!current_ready && strict)
        {
            return false;
        }
    }
    return one_ready;
}

LLViewerTexture* LLTerrainMaterials::getPaintMap()
{
    return mPaintMap.get();
}

void LLTerrainMaterials::setPaintMap(LLViewerTexture* paint_map)
{
    llassert(!paint_map || mPaintType == TERRAIN_PAINT_TYPE_PBR_PAINTMAP);
    const bool changed = paint_map != mPaintMap;
    mPaintMap = paint_map;
    // The paint map has changed, so edits are no longer valid
    mPaintRequestQueue.clear();
    mPaintMapQueue.clear();
}

// Boost the texture loading priority
// Return true when ready to use (i.e. texture is sufficiently loaded)
// static
bool LLTerrainMaterials::makeTextureReady(LLPointer<LLViewerFetchedTexture>& tex, bool boost)
{
    //llassert(tex); ??? maybe ok ???
    if (!tex) { return false; }

    if (tex->getDiscardLevel() < 0)
    {
        if (boost)
        {
            boost_minimap_texture(tex, BASE_SIZE*BASE_SIZE);
        }
        return false;
    }
    if ((tex->getDiscardLevel() != 0 &&
         (tex->getWidth() < BASE_SIZE ||
          tex->getHeight() < BASE_SIZE)))
    {
        if (boost)
        {
            boost_minimap_texture(tex, BASE_SIZE*BASE_SIZE);

            S32 width = tex->getFullWidth();
            S32 height = tex->getFullHeight();
            S32 min_dim = llmin(width, height);
            S32 ddiscard = 0;
            while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
            {
                ddiscard++;
                min_dim /= 2;
            }
            tex->setMinDiscardLevel(ddiscard);
        }
        return false;
    }
    if (tex->getComponents() == 0)
    {
        return false;
    }
    return true;
}

// Make sure to call material_asset_ready first
// strict = true -> all materials must be sufficiently loaded
// strict = false -> at least one material must be loaded
// static
bool LLTerrainMaterials::materialTexturesReady(LLPointer<LLFetchedGLTFMaterial>& mat, bool& textures_set, bool boost, bool strict)
{
    llassert(mat);

    // Material is loaded, but textures may not be
    if (!textures_set)
    {
        textures_set = true;
        // *NOTE: These can sometimes be set to to nullptr due to
        // updateTEMaterialTextures. For the sake of robustness, we emulate
        // that fetching behavior by setting textures of null IDs to nullptr.
        mat->mBaseColorTexture         = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR]);
        mat->mNormalTexture            = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL]);
        mat->mMetallicRoughnessTexture = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS]);
        mat->mEmissiveTexture          = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE]);
    }

    // *NOTE: Calls to makeTextureReady may boost textures. Do not early-return.
    bool ready[LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT];
    ready[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] =
        mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR].isNull() || makeTextureReady(mat->mBaseColorTexture, boost);
    ready[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] =
        mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL].isNull() || makeTextureReady(mat->mNormalTexture, boost);
    ready[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] =
        mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS].isNull() ||
        makeTextureReady(mat->mMetallicRoughnessTexture, boost);
    ready[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] =
        mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE].isNull() || makeTextureReady(mat->mEmissiveTexture, boost);

    if (strict)
    {
        for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
        {
            if (!ready[i])
            {
                return false;
            }
        }
    }

    return true;
}

// static
const LLUUID (&LLVLComposition::getDefaultTextures())[ASSET_COUNT]
{
    const static LLUUID default_textures[LLVLComposition::ASSET_COUNT] =
    {
        TERRAIN_DIRT_DETAIL,
        TERRAIN_GRASS_DETAIL,
        TERRAIN_MOUNTAIN_DETAIL,
        TERRAIN_ROCK_DETAIL
    };
    return default_textures;
}

LLVLComposition::LLVLComposition(LLSurface *surfacep, const U32 width, const F32 scale) :
    LLTerrainMaterials(),
    LLViewerLayer(width, scale)
{
    // Load Terrain Textures - Original ones
    const LLUUID (&default_textures)[LLVLComposition::ASSET_COUNT] = LLVLComposition::getDefaultTextures();
    for (S32 i = 0; i < ASSET_COUNT; ++i)
    {
        setDetailAssetID(i, default_textures[i]);
    }

    mSurfacep = surfacep;

    // Initialize the texture matrix to defaults.
    for (S32 i = 0; i < CORNER_COUNT; ++i)
    {
        mStartHeight[i] = gSavedSettings.getF32("TerrainColorStartHeight");
        mHeightRange[i] = gSavedSettings.getF32("TerrainColorHeightRange");
    }
}


LLVLComposition::~LLVLComposition()
{
    LLTerrainMaterials::~LLTerrainMaterials();
}


void LLVLComposition::setSurface(LLSurface *surfacep)
{
    mSurfacep = surfacep;
}

bool LLVLComposition::generateHeights(const F32 x, const F32 y,
                                      const F32 width, const F32 height)
{
    if (!mParamsReady)
    {
        // All the parameters haven't been set yet (we haven't gotten the message from the sim)
        return false;
    }

    llassert(mSurfacep);

    if (!mSurfacep || !mSurfacep->getRegion())
    {
        // We don't always have the region yet here....
        return false;
    }

    S32 x_begin, y_begin, x_end, y_end;

    x_begin = ll_round( x * mScaleInv );
    y_begin = ll_round( y * mScaleInv );
    x_end = ll_round( (x + width) * mScaleInv );
    y_end = ll_round( (y + width) * mScaleInv );

    if (x_end > mWidth)
    {
        x_end = mWidth;
    }
    if (y_end > mWidth)
    {
        y_end = mWidth;
    }

    LLVector3d origin_global = from_region_handle(mSurfacep->getRegion()->getHandle());

    // For perlin noise generation...
    const F32 slope_squared = 1.5f*1.5f;
    const F32 xyScale = 4.9215f; //0.93284f;
    const F32 zScale = 4; //0.92165f;
    const F32 z_offset = 0.f;
    const F32 noise_magnitude = 2.f;        //  Degree to which noise modulates composition layer (versus
                                            //  simple height)

    const F32 xyScaleInv = (1.f / xyScale);
    const F32 zScaleInv = (1.f / zScale);

    const F32 inv_width = 1.f/mWidth;

    // OK, for now, just have the composition value equal the height at the point.
    for (S32 j = y_begin; j < y_end; j++)
    {
        for (S32 i = x_begin; i < x_end; i++)
        {

            F32 vec[3];
            F32 vec1[3];
            F32 twiddle;

            // Bilinearly interpolate the start height and height range of the textures
            F32 start_height = bilinear(mStartHeight[SOUTHWEST],
                                        mStartHeight[SOUTHEAST],
                                        mStartHeight[NORTHWEST],
                                        mStartHeight[NORTHEAST],
                                        i*inv_width, j*inv_width); // These will be bilinearly interpolated
            F32 height_range = bilinear(mHeightRange[SOUTHWEST],
                                        mHeightRange[SOUTHEAST],
                                        mHeightRange[NORTHWEST],
                                        mHeightRange[NORTHEAST],
                                        i*inv_width, j*inv_width); // These will be bilinearly interpolated

            LLVector3 location(i*mScale, j*mScale, 0.f);

            F32 height = mSurfacep->resolveHeightRegion(location) + z_offset;

            // Step 0: Measure the exact height at this texel
            vec[0] = (F32)(origin_global.mdV[VX]+location.mV[VX])*xyScaleInv;   //  Adjust to non-integer lattice
            vec[1] = (F32)(origin_global.mdV[VY]+location.mV[VY])*xyScaleInv;
            vec[2] = height*zScaleInv;
            //
            //  Choose material value by adding to the exact height a random value
            //
            vec1[0] = vec[0]*(0.2222222222f);
            vec1[1] = vec[1]*(0.2222222222f);
            vec1[2] = vec[2]*(0.2222222222f);
            twiddle = noise2(vec1)*6.5f;                    //  Low freq component for large divisions

            twiddle += turbulence2(vec, 2)*slope_squared;   //  High frequency component
            twiddle *= noise_magnitude;

            F32 scaled_noisy_height = (height + twiddle - start_height) * F32(ASSET_COUNT) / height_range;

            scaled_noisy_height = llmax(0.f, scaled_noisy_height);
            scaled_noisy_height = llmin(3.f, scaled_noisy_height);
            *(mDatap + i + j*mWidth) = scaled_noisy_height;
        }
    }
    return true;
}

LLTerrainMaterials gLocalTerrainMaterials;

bool LLVLComposition::generateComposition()
{
    if (!mParamsReady)
    {
        // All the parameters haven't been set yet (we haven't gotten the message from the sim)
        return false;
    }

    return LLTerrainMaterials::generateMaterials();
}

F32 LLVLComposition::getStartHeight(S32 corner)
{
    return mStartHeight[corner];
}

void LLVLComposition::setDetailAssetID(S32 asset, const LLUUID& id)
{
    if (id.isNull())
    {
        return;
    }
    LLTerrainMaterials::setDetailAssetID(asset, id);
    mRawImages[asset] = NULL;
    mRawImagesBaseColor[asset] = NULL;
    mRawImagesEmissive[asset] = NULL;
}

void LLVLComposition::setStartHeight(S32 corner, const F32 start_height)
{
    mStartHeight[corner] = start_height;
}

F32 LLVLComposition::getHeightRange(S32 corner)
{
    return mHeightRange[corner];
}

void LLVLComposition::setHeightRange(S32 corner, const F32 range)
{
    mHeightRange[corner] = range;
}
