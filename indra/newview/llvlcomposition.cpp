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

LLTerrainMaterials::LLTerrainMaterials()
{
    for (S32 i = 0; i < ASSET_COUNT; ++i)
    {
        mMaterialTexturesSet[i] = false;
    }
}

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
    mMaterialTexturesSet[asset] = false;
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
        if (!render_mat)
        {
            render_mat = new LLFetchedGLTFMaterial();
            *render_mat = *mat;
            // This render_mat is effectively already loaded, because it gets its data from mat.

            LLPointer<LLGLTFMaterial>& override_mat = mDetailMaterialOverrides[i];
            if (override_mat)
            {
                render_mat->applyOverride(*override_mat);
            }
        }

        ready[i] = materialTexturesReady(render_mat, mMaterialTexturesSet[i], boost, strict);
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

// Boost the texture loading priority
// Return true when ready to use (i.e. texture is sufficiently loaded)
// static
bool LLTerrainMaterials::makeTextureReady(LLPointer<LLViewerFetchedTexture>& tex, bool boost)
{
    llassert(tex);
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

// Boost the loading priority of every known texture in the material
// Return true when ready to use
// static
bool LLTerrainMaterials::makeMaterialReady(LLPointer<LLFetchedGLTFMaterial> &mat, bool &textures_set, bool boost, bool strict)
{
    if (!material_asset_ready(mat)) { return false; }

    return materialTexturesReady(mat, textures_set, boost, strict);
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

namespace
{
    void prepare_fallback_image(LLImageRaw* raw_image)
    {
        raw_image->resize(BASE_SIZE, BASE_SIZE, 4);
        raw_image->fill(LLColor4U::white);
    }

    // Check if the raw image is loaded for this texture at a discard
    // level the minimap can use, and if not then try to get it loaded.
    bool prepare_raw_image(LLPointer<LLImageRaw>& raw_image, bool emissive, LLViewerFetchedTexture* tex, bool& delete_raw_post)
    {
        if (!tex)
        {
            if (!emissive)
            {
                prepare_fallback_image(raw_image);
            }
            else
            {
                llassert(!raw_image);
                raw_image = nullptr;
            }
            return true;
        }
        if (raw_image)
        {
            // Callback already initiated
            if (raw_image->getDataSize() > 0)
            {
                // Callback finished
                delete_raw_post = true;
                return true;
            }
            else
            {
                return false;
            }
        }

        raw_image = new LLImageRaw();

        S32 ddiscard = 0;
        {
            S32 min_dim = llmin(tex->getFullWidth(), tex->getFullHeight());
            while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
            {
                ddiscard++;
                min_dim /= 2;
            }
        }

        struct PendingImage
        {
            LLImageRaw* mRawImage;
            S32 mDesiredDiscard;
            LLUUID mTextureId;
            PendingImage(LLImageRaw* raw_image, S32 ddiscard, const LLUUID& texture_id)
                : mRawImage(raw_image)
                , mDesiredDiscard(ddiscard)
                , mTextureId(texture_id)
            {
                mRawImage->ref();
            }
            ~PendingImage()
            {
                mRawImage->unref();
            }
        };
        PendingImage* pending_image = new PendingImage(raw_image, ddiscard, tex->getID());

        loaded_callback_func cb = [](bool success, LLViewerFetchedTexture * src_vi, LLImageRaw * src, LLImageRaw * src_aux, S32 discard_level, bool is_final, void* userdata) {
            PendingImage* pending = (PendingImage*)userdata;
            // Owning LLVLComposition still exists

            // Assume mRawImage only used by single LLVLComposition for now
            const bool in_use_by_composition = pending->mRawImage->getNumRefs() > 1;
            llassert(pending->mRawImage->getNumRefs());
            llassert(pending->mRawImage->getNumRefs() <= 2);
            const bool needs_data = !pending->mRawImage->getDataSize();
            if (in_use_by_composition && needs_data)
            {
                if (success && pending->mDesiredDiscard == discard_level)
                {
                    pending->mRawImage->resize(BASE_SIZE, BASE_SIZE, src->getComponents());
                    pending->mRawImage->copyScaled(src);
                }
                else if (is_final)
                {
                    prepare_fallback_image(pending->mRawImage);
                }
            }

            if (is_final) { delete pending; }
        };
        tex->setLoadedCallback(cb, ddiscard, true, false, pending_image, nullptr);
        tex->forceToSaveRawImage(ddiscard);

        return false;
    }
};

bool LLVLComposition::generateMinimapTileLand(const F32 x, const F32 y,
                                      const F32 width, const F32 height)
{
    LL_PROFILE_ZONE_SCOPED
    llassert(mSurfacep);
    llassert(x >= 0.f);
    llassert(y >= 0.f);

    ///////////////////////////
    //
    // Generate raw data arrays for surface textures
    //
    //

    // These have already been validated by generateComposition.
    U8* st_data[ASSET_COUNT];
    S32 st_data_size[ASSET_COUNT]; // for debugging

    const bool use_textures = getMaterialType() != LLTerrainMaterials::Type::PBR;
    if (use_textures)
    {
        if (!makeTexturesReady(true, true)) { return false; }
    }
    else
    {
        if (!makeMaterialsReady(true, true)) { return false; }
    }

    for (S32 i = 0; i < ASSET_COUNT; i++)
    {
        if (mRawImages[i].isNull())
        {
            // Read back a raw image for this discard level, if it exists
            LLViewerFetchedTexture* tex;
            LLViewerFetchedTexture* tex_emissive; // Can be null
            bool has_base_color_factor;
            bool has_emissive_factor;
            bool has_alpha;
            LLColor3 base_color_factor;
            LLColor3 emissive_factor;
            if (use_textures)
            {
                tex = mDetailTextures[i];
                tex_emissive = nullptr;
                has_base_color_factor = false;
                has_emissive_factor = false;
                has_alpha = false;
                llassert(tex);
            }
            else
            {
                LLPointer<LLFetchedGLTFMaterial>& mat = mDetailRenderMaterials[i];
                tex = mat->mBaseColorTexture;
                tex_emissive = mat->mEmissiveTexture;
                base_color_factor = LLColor3(mat->mBaseColor);
                // *HACK: Treat alpha as black
                base_color_factor *= (mat->mBaseColor.mV[VW]);
                emissive_factor = mat->mEmissiveColor;
                has_base_color_factor = (base_color_factor.mV[VX] != 1.f ||
                                         base_color_factor.mV[VY] != 1.f ||
                                         base_color_factor.mV[VZ] != 1.f);
                has_emissive_factor = (emissive_factor.mV[VX] != 1.f ||
                                       emissive_factor.mV[VY] != 1.f ||
                                       emissive_factor.mV[VZ] != 1.f);
                has_alpha = mat->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_OPAQUE;
            }

            if (!tex) { tex = LLViewerFetchedTexture::sWhiteImagep; }

            bool delete_raw_post = false;
            bool delete_raw_post_emissive = false;
            if (!prepare_raw_image(mRawImagesBaseColor[i], false, tex, delete_raw_post)) { return false; }
            if (tex_emissive && !prepare_raw_image(mRawImagesEmissive[i], true, tex_emissive, delete_raw_post_emissive)) { return false; }
            // tex_emissive can be null, and then will be ignored

            // In the simplest case, the minimap image is just the base color.
            // This will be replaced if we need to do any tinting/compositing.
            mRawImages[i] = mRawImagesBaseColor[i];

            // *TODO: This isn't quite right for PBR:
            // 1) It does not convert the color images from SRGB to linear
            // before mixing (which will always require copying the image).
            // 2) It mixes emissive and base color before mixing terrain
            // materials, but it should be the other way around
            // Long-term, we should consider a method that is more
            // maintainable. Shaders, perhaps? Bake shaders to textures?
            LLPointer<LLImageRaw> raw_emissive;
            if (tex_emissive)
            {
                raw_emissive = mRawImagesEmissive[i];
                if (has_emissive_factor ||
                    tex_emissive->getWidth(tex_emissive->getRawImageLevel()) != BASE_SIZE ||
                    tex_emissive->getHeight(tex_emissive->getRawImageLevel()) != BASE_SIZE ||
                    tex_emissive->getComponents() != 4)
                {
                    LLPointer<LLImageRaw> newraw_emissive = new LLImageRaw(BASE_SIZE, BASE_SIZE, 4);
                    // Copy RGB, leave alpha alone (set to opaque by default)
                    newraw_emissive->copy(mRawImagesEmissive[i]);
                    if (has_emissive_factor)
                    {
                        newraw_emissive->tint(emissive_factor);
                    }
                    raw_emissive = newraw_emissive;
                }
            }
            if (has_base_color_factor ||
                raw_emissive ||
                has_alpha ||
                tex->getWidth(tex->getRawImageLevel()) != BASE_SIZE ||
                tex->getHeight(tex->getRawImageLevel()) != BASE_SIZE ||
                tex->getComponents() != 3)
            {
                LLPointer<LLImageRaw> newraw = new LLImageRaw(BASE_SIZE, BASE_SIZE, 3);
                if (has_alpha)
                {
                    // Approximate the water underneath terrain alpha with solid water color
                    newraw->clear(
                        MAX_WATER_COLOR.mV[VX],
                        MAX_WATER_COLOR.mV[VY],
                        MAX_WATER_COLOR.mV[VZ],
                        255);
                }
                newraw->composite(mRawImagesBaseColor[i]);
                if (has_base_color_factor)
                {
                    newraw->tint(base_color_factor);
                }
                // Apply emissive texture
                if (raw_emissive)
                {
                    newraw->addEmissive(raw_emissive);
                }

                mRawImages[i] = newraw; // deletes old
            }

            if (delete_raw_post)
            {
                tex->destroyRawImage();
            }
            if (delete_raw_post_emissive)
            {
                tex_emissive->destroyRawImage();
            }

            // Remove intermediary image references
            mRawImagesBaseColor[i] = nullptr;
            mRawImagesEmissive[i] = nullptr;
        }
        st_data[i] = mRawImages[i]->getData();
        st_data_size[i] = mRawImages[i]->getDataSize();
    }

    ///////////////////////////////////////
    //
    // Generate and clamp x/y bounding box.
    //
    //

    S32 x_begin, y_begin, x_end, y_end;
    x_begin = (S32)(x * mScaleInv);
    y_begin = (S32)(y * mScaleInv);
    x_end = ll_round( (x + width) * mScaleInv );
    y_end = ll_round( (y + width) * mScaleInv );

    if (x_end > mWidth)
    {
        llassert(false);
        x_end = mWidth;
    }
    if (y_end > mWidth)
    {
        llassert(false);
        y_end = mWidth;
    }


    ///////////////////////////////////////////
    //
    // Generate target texture information, stride ratios.
    //
    //

    LLViewerTexture *texturep;
    U32 tex_width, tex_height, tex_comps;
    U32 tex_stride;
    F32 tex_x_scalef, tex_y_scalef;
    S32 tex_x_begin, tex_y_begin, tex_x_end, tex_y_end;
    F32 tex_x_ratiof, tex_y_ratiof;

    texturep = mSurfacep->getSTexture();
    tex_width = texturep->getWidth();
    tex_height = texturep->getHeight();
    tex_comps = texturep->getComponents();
    tex_stride = tex_width * tex_comps;

    U32 st_comps = 3;
    U32 st_width = BASE_SIZE;
    U32 st_height = BASE_SIZE;

    if (tex_comps != st_comps)
    {
        llassert(false);
        return false;
    }

    tex_x_scalef = (F32)tex_width / (F32)mWidth;
    tex_y_scalef = (F32)tex_height / (F32)mWidth;
    tex_x_begin = (S32)((F32)x_begin * tex_x_scalef);
    tex_y_begin = (S32)((F32)y_begin * tex_y_scalef);
    tex_x_end = (S32)((F32)x_end * tex_x_scalef);
    tex_y_end = (S32)((F32)y_end * tex_y_scalef);

    tex_x_ratiof = (F32)mWidth*mScale / (F32)tex_width;
    tex_y_ratiof = (F32)mWidth*mScale / (F32)tex_height;

    LLPointer<LLImageRaw> raw = new LLImageRaw(tex_width, tex_height, tex_comps);
    U8 *rawp = raw->getData();

    F32 st_x_stride, st_y_stride;
    st_x_stride = ((F32)st_width / (F32)mTexScaleX)*((F32)mWidth / (F32)tex_width);
    st_y_stride = ((F32)st_height / (F32)mTexScaleY)*((F32)mWidth / (F32)tex_height);

    llassert(st_x_stride > 0.f);
    llassert(st_y_stride > 0.f);
    ////////////////////////////////
    //
    // Iterate through the target texture, striding through the
    // subtextures and interpolating appropriately.
    //
    //

    F32 sti, stj;
    S32 st_offset;
    sti = (tex_x_begin * st_x_stride) - st_width*(llfloor((tex_x_begin * st_x_stride)/st_width));
    stj = (tex_y_begin * st_y_stride) - st_height*(llfloor((tex_y_begin * st_y_stride)/st_height));

    st_offset = (llfloor(stj * st_width) + llfloor(sti)) * st_comps;
    for (S32 j = tex_y_begin; j < tex_y_end; j++)
    {
        U32 offset = j * tex_stride + tex_x_begin * tex_comps;
        sti = (tex_x_begin * st_x_stride) - st_width*((U32)(tex_x_begin * st_x_stride)/st_width);
        for (S32 i = tex_x_begin; i < tex_x_end; i++)
        {
            S32 tex0, tex1;
            F32 composition = getValueScaled(i*tex_x_ratiof, j*tex_y_ratiof);

            tex0 = llfloor( composition );
            tex0 = llclamp(tex0, 0, 3);
            composition -= tex0;
            tex1 = tex0 + 1;
            tex1 = llclamp(tex1, 0, 3);

            st_offset = (lltrunc(sti) + lltrunc(stj)*st_width) * st_comps;
            for (U32 k = 0; k < tex_comps; k++)
            {
                // Linearly interpolate based on composition.
                if (st_offset >= st_data_size[tex0] || st_offset >= st_data_size[tex1])
                {
                    // SJB: This shouldn't be happening, but does... Rounding error?
                    //LL_WARNS() << "offset 0 [" << tex0 << "] =" << st_offset << " >= size=" << st_data_size[tex0] << LL_ENDL;
                    //LL_WARNS() << "offset 1 [" << tex1 << "] =" << st_offset << " >= size=" << st_data_size[tex1] << LL_ENDL;
                }
                else
                {
                    F32 a = *(st_data[tex0] + st_offset);
                    F32 b = *(st_data[tex1] + st_offset);
                    rawp[ offset ] = (U8)lltrunc( a + composition * (b - a) );
                }
                offset++;
                st_offset++;
            }

            sti += st_x_stride;
            if (sti >= st_width)
            {
                sti -= st_width;
            }
        }

        stj += st_y_stride;
        if (stj >= st_height)
        {
            stj -= st_height;
        }
    }

    if (!texturep->hasGLTexture())
    {
        texturep->createGLTexture(0, raw);
    }
    texturep->setSubImage(raw, tex_x_begin, tex_y_begin, tex_x_end - tex_x_begin, tex_y_end - tex_y_begin);

    // Un-boost detail textures (will get re-boosted if rendering in high detail)
    for (S32 i = 0; i < ASSET_COUNT; i++)
    {
        unboost_minimap_texture(mDetailTextures[i]);
    }

    // Un-boost textures for each detail material (will get re-boosted if rendering in high detail)
    for (S32 i = 0; i < ASSET_COUNT; i++)
    {
        unboost_minimap_material(mDetailMaterials[i]);
    }

    return true;
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
