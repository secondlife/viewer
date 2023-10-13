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


LLVLComposition::LLVLComposition(LLSurface *surfacep, const U32 width, const F32 scale) :
	LLViewerLayer(width, scale)
{
	mSurfacep = surfacep;

	// Load Terrain Textures - Original ones
	setDetailAssetID(0, TERRAIN_DIRT_DETAIL);
	setDetailAssetID(1, TERRAIN_GRASS_DETAIL);
	setDetailAssetID(2, TERRAIN_MOUNTAIN_DETAIL);
	setDetailAssetID(3, TERRAIN_ROCK_DETAIL);

	// Initialize the texture matrix to defaults.
	for (S32 i = 0; i < CORNER_COUNT; ++i)
	{
		mStartHeight[i] = gSavedSettings.getF32("TerrainColorStartHeight");
		mHeightRange[i] = gSavedSettings.getF32("TerrainColorHeightRange");
	}

    for (S32 i = 0; i < ASSET_COUNT; ++i)
    {
        mMaterialTexturesSet[i] = false;
    }
}


LLVLComposition::~LLVLComposition()
{
}


void LLVLComposition::setSurface(LLSurface *surfacep)
{
	mSurfacep = surfacep;
}

LLPointer<LLViewerFetchedTexture> fetch_terrain_texture(const LLUUID& id)
{
    if (id.isNull())
    {
        return nullptr;
    }

    LLPointer<LLViewerFetchedTexture> tex = LLViewerTextureManager::getFetchedTexture(id);
    tex->setNoDelete();
    return tex;
}

void LLVLComposition::setDetailAssetID(S32 asset, const LLUUID& id)
{
	if(id.isNull())
	{
		return;
	}
	// This is terrain texture, but we are not setting it as BOOST_TERRAIN
	// since we will be manipulating it later as needed.
	mDetailTextures[asset] = fetch_terrain_texture(id);
	mRawImages[asset] = NULL;
    LLPointer<LLFetchedGLTFMaterial>& mat = mDetailMaterials[asset];
    mat = gGLTFMaterialList.getMaterial(id);
    mMaterialTexturesSet[asset] = false;
}

BOOL LLVLComposition::generateHeights(const F32 x, const F32 y,
									  const F32 width, const F32 height)
{
	if (!mParamsReady)
	{
		// All the parameters haven't been set yet (we haven't gotten the message from the sim)
		return FALSE;
	}

	llassert(mSurfacep);

	if (!mSurfacep || !mSurfacep->getRegion()) 
	{
		// We don't always have the region yet here....
		return FALSE;
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
	const F32 noise_magnitude = 2.f;		//  Degree to which noise modulates composition layer (versus
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
			vec[0] = (F32)(origin_global.mdV[VX]+location.mV[VX])*xyScaleInv;	//  Adjust to non-integer lattice
			vec[1] = (F32)(origin_global.mdV[VY]+location.mV[VY])*xyScaleInv;
			vec[2] = height*zScaleInv;
			//
			//  Choose material value by adding to the exact height a random value 
			//
			vec1[0] = vec[0]*(0.2222222222f);
			vec1[1] = vec[1]*(0.2222222222f);
			vec1[2] = vec[2]*(0.2222222222f);
			twiddle = noise2(vec1)*6.5f;					//  Low freq component for large divisions

			twiddle += turbulence2(vec, 2)*slope_squared;	//  High frequency component
			twiddle *= noise_magnitude;

			F32 scaled_noisy_height = (height + twiddle - start_height) * F32(ASSET_COUNT) / height_range;

			scaled_noisy_height = llmax(0.f, scaled_noisy_height);
			scaled_noisy_height = llmin(3.f, scaled_noisy_height);
			*(mDatap + i + j*mWidth) = scaled_noisy_height;
		}
	}
	return TRUE;
}

static const U32 BASE_SIZE = 128;

// Boost the texture loading priority
// Return true when ready to use (i.e. texture is sufficiently loaded)
// static
BOOL LLVLComposition::textureReady(LLPointer<LLViewerFetchedTexture>& tex, BOOL boost)
{
    llassert(tex.notNull());

    if (tex->getDiscardLevel() < 0)
    {
        if (boost)
        {
            tex->setBoostLevel(LLGLTexture::BOOST_TERRAIN); // in case we are at low detail
            tex->addTextureStats(BASE_SIZE*BASE_SIZE);
        }
        return FALSE;
    }
    if ((tex->getDiscardLevel() != 0 &&
         (tex->getWidth() < BASE_SIZE ||
          tex->getHeight() < BASE_SIZE)))
    {
        if (boost)
        {
            S32 width = tex->getFullWidth();
            S32 height = tex->getFullHeight();
            S32 min_dim = llmin(width, height);
            S32 ddiscard = 0;
            while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
            {
                ddiscard++;
                min_dim /= 2;
            }
            tex->setBoostLevel(LLGLTexture::BOOST_TERRAIN); // in case we are at low detail
            tex->setMinDiscardLevel(ddiscard);
            tex->addTextureStats(BASE_SIZE*BASE_SIZE); // priority
        }
        return FALSE;
    }
    return TRUE;
}

// Boost the loading priority of every known texture in the material
// Return true when ready to use (i.e. material and all textures within are sufficiently loaded)
// static
BOOL LLVLComposition::materialReady(LLPointer<LLFetchedGLTFMaterial>& mat, bool& textures_set, BOOL boost)
{
    if (!mat->isLoaded())
    {
        return FALSE;
    }

    // Material is loaded, but textures may not be
    if (!textures_set)
    {
        // *NOTE: These can sometimes be set to to nullptr due to
        // updateTEMaterialTextures. For the sake of robustness, we emulate
        // that fetching behavior by setting textures of null IDs to nullptr.
        mat->mBaseColorTexture = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR]);
        mat->mNormalTexture = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL]);
        mat->mMetallicRoughnessTexture = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS]);
        mat->mEmissiveTexture = fetch_terrain_texture(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE]);
        textures_set = true;

        return FALSE;
    }

    if (mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR].notNull() && !textureReady(mat->mBaseColorTexture, boost))
    {
        return FALSE;
    }
    if (mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL].notNull() && !textureReady(mat->mNormalTexture, boost))
    {
        return FALSE;
    }
    if (mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS].notNull() && !textureReady(mat->mMetallicRoughnessTexture, boost))
    {
        return FALSE;
    }
    if (mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE].notNull() && !textureReady(mat->mEmissiveTexture, boost))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL LLVLComposition::useTextures()
{
	LL_PROFILE_ZONE_SCOPED;

    return texturesReady() || !materialsReady();
}

BOOL LLVLComposition::texturesReady(BOOL boost)
{
	for (S32 i = 0; i < ASSET_COUNT; i++)
	{
        if (!textureReady(mDetailTextures[i], boost))
        {
            return FALSE;
        }
	}
    return TRUE;
}

BOOL LLVLComposition::materialsReady(BOOL boost)
{
	for (S32 i = 0; i < ASSET_COUNT; i++)
	{
        if (!materialReady(mDetailMaterials[i], mMaterialTexturesSet[i], boost))
        {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL LLVLComposition::generateComposition()
{
	if (!mParamsReady)
	{
		// All the parameters haven't been set yet (we haven't gotten the message from the sim)
		return FALSE;
	}

    if (texturesReady(TRUE))
    {
        return TRUE;
    }

    if (materialsReady(TRUE))
    {
        return TRUE;
    }

    return FALSE;
}

// TODO: Re-evaluate usefulness of this function in the PBR case. There is currently a hack here to treat the material base color like a legacy terrain texture, but I'm not sure if that's useful.
BOOL LLVLComposition::generateTexture(const F32 x, const F32 y,
									  const F32 width, const F32 height)
{
	LL_PROFILE_ZONE_SCOPED
	llassert(mSurfacep);
	llassert(x >= 0.f);
	llassert(y >= 0.f);

	LLTimer gen_timer;

	///////////////////////////
	//
	// Generate raw data arrays for surface textures
	//
	//

	// These have already been validated by generateComposition.
	U8* st_data[ASSET_COUNT];
	S32 st_data_size[ASSET_COUNT]; // for debugging

    const bool use_textures = useTextures();

	for (S32 i = 0; i < ASSET_COUNT; i++)
	{
		if (mRawImages[i].isNull())
		{
			// Read back a raw image for this discard level, if it exists
            LLViewerFetchedTexture* tex;
            if (use_textures)
            {
                tex = mDetailTextures[i];
            }
            else
            {
                tex = mDetailMaterials[i]->mBaseColorTexture;
                if (!tex) { tex = LLViewerFetchedTexture::sWhiteImagep; }
            }

			S32 min_dim = llmin(tex->getFullWidth(), tex->getFullHeight());
			S32 ddiscard = 0;
			while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
			{
				ddiscard++;
				min_dim /= 2;
			}

			BOOL delete_raw = (tex->reloadRawImage(ddiscard) != NULL) ;
			if(tex->getRawImageLevel() != ddiscard)//raw iamge is not ready, will enter here again later.
			{
                if (tex->getFetchPriority() <= 0.0f && !tex->hasSavedRawImage())
                {
                    tex->setBoostLevel(LLGLTexture::BOOST_MAP);
                    tex->forceToRefetchTexture(ddiscard);
                }

				if(delete_raw)
				{
					tex->destroyRawImage() ;
				}
				LL_DEBUGS("Terrain") << "cached raw data for terrain detail texture is not ready yet: " << tex->getID() << " Discard: " << ddiscard << LL_ENDL;
				return FALSE;
			}

			mRawImages[i] = tex->getRawImage() ;
			if(delete_raw)
			{
				tex->destroyRawImage() ;
			}
			if (tex->getWidth(ddiscard) != BASE_SIZE ||
				tex->getHeight(ddiscard) != BASE_SIZE ||
				tex->getComponents() != 3)
			{
				LLPointer<LLImageRaw> newraw = new LLImageRaw(BASE_SIZE, BASE_SIZE, 3);
				newraw->composite(mRawImages[i]);
				mRawImages[i] = newraw; // deletes old
			}
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
		LL_WARNS("Terrain") << "x end > width" << LL_ENDL;
		x_end = mWidth;
	}
	if (y_end > mWidth)
	{
		LL_WARNS("Terrain") << "y end > width" << LL_ENDL;
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
		LL_WARNS("Terrain") << "Base texture comps != input texture comps" << LL_ENDL;
		return FALSE;
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

	for (S32 i = 0; i < ASSET_COUNT; i++)
	{
		// Un-boost detatil textures (will get re-boosted if rendering in high detail)
		mDetailTextures[i]->setBoostLevel(LLGLTexture::BOOST_NONE);
		mDetailTextures[i]->setMinDiscardLevel(MAX_DISCARD_LEVEL + 1);
	}
	
	return TRUE;
}

LLUUID LLVLComposition::getDetailAssetID(S32 corner)
{
    llassert(mDetailTextures[corner] && mDetailMaterials[corner]);
    // *HACK: Assume both the the material and texture were fetched in the same
    // way using the same UUID. However, we may not know at this point which
    // one will load.
	return mDetailTextures[corner]->getID();
}

F32 LLVLComposition::getStartHeight(S32 corner)
{
	return mStartHeight[corner];
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
