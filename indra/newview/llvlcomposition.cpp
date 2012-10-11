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

#include "imageids.h"
#include "llerror.h"
#include "v3math.h"
#include "llsurface.h"
#include "lltextureview.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
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
	LLViewerLayer(width, scale),
	mParamsReady(FALSE)
{
	mSurfacep = surfacep;

	// Load Terrain Textures - Original ones
	setDetailTextureID(0, TERRAIN_DIRT_DETAIL);
	setDetailTextureID(1, TERRAIN_GRASS_DETAIL);
	setDetailTextureID(2, TERRAIN_MOUNTAIN_DETAIL);
	setDetailTextureID(3, TERRAIN_ROCK_DETAIL);

	// Initialize the texture matrix to defaults.
	for (S32 i = 0; i < CORNER_COUNT; ++i)
	{
		mStartHeight[i] = gSavedSettings.getF32("TerrainColorStartHeight");
		mHeightRange[i] = gSavedSettings.getF32("TerrainColorHeightRange");
	}
	mTexScaleX = 16.f;
	mTexScaleY = 16.f;
	mTexturesLoaded = FALSE;
}


LLVLComposition::~LLVLComposition()
{
}


void LLVLComposition::setSurface(LLSurface *surfacep)
{
	mSurfacep = surfacep;
}


void LLVLComposition::setDetailTextureID(S32 corner, const LLUUID& id)
{
	if(id.isNull())
	{
		return;
	}
	mDetailTextures[corner] = LLViewerTextureManager::getFetchedTexture(id);
	mDetailTextures[corner]->setNoDelete() ;
	mRawImages[corner] = NULL;
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

	x_begin = llround( x * mScaleInv );
	y_begin = llround( y * mScaleInv );
	x_end = llround( (x + width) * mScaleInv );
	y_end = llround( (y + width) * mScaleInv );

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

	// Heights map into textures as 0-1 = first, 1-2 = second, etc.
	// So we need to compress heights into this range.
	const S32 NUM_TEXTURES = 4;

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

			F32 scaled_noisy_height = (height + twiddle - start_height) * F32(NUM_TEXTURES) / height_range;

			scaled_noisy_height = llmax(0.f, scaled_noisy_height);
			scaled_noisy_height = llmin(3.f, scaled_noisy_height);
			*(mDatap + i + j*mWidth) = scaled_noisy_height;
		}
	}
	return TRUE;
}

static const U32 BASE_SIZE = 128;

BOOL LLVLComposition::generateComposition()
{

	if (!mParamsReady)
	{
		// All the parameters haven't been set yet (we haven't gotten the message from the sim)
		return FALSE;
	}

	for (S32 i = 0; i < 4; i++)
	{
		if (mDetailTextures[i]->getDiscardLevel() < 0)
		{
			mDetailTextures[i]->setBoostLevel(LLGLTexture::BOOST_TERRAIN); // in case we are at low detail
			mDetailTextures[i]->addTextureStats(BASE_SIZE*BASE_SIZE);
			return FALSE;
		}
		if ((mDetailTextures[i]->getDiscardLevel() != 0 &&
			 (mDetailTextures[i]->getWidth() < BASE_SIZE ||
			  mDetailTextures[i]->getHeight() < BASE_SIZE)))
		{
			S32 width = mDetailTextures[i]->getFullWidth();
			S32 height = mDetailTextures[i]->getFullHeight();
			S32 min_dim = llmin(width, height);
			S32 ddiscard = 0;
			while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
			{
				ddiscard++;
				min_dim /= 2;
			}
			mDetailTextures[i]->setBoostLevel(LLGLTexture::BOOST_TERRAIN); // in case we are at low detail
			mDetailTextures[i]->setMinDiscardLevel(ddiscard);
			return FALSE;
		}
	}
	
	return TRUE;
}

BOOL LLVLComposition::generateTexture(const F32 x, const F32 y,
									  const F32 width, const F32 height)
{
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
	U8* st_data[4];
	S32 st_data_size[4]; // for debugging

	for (S32 i = 0; i < 4; i++)
	{
		if (mRawImages[i].isNull())
		{
			// Read back a raw image for this discard level, if it exists
			S32 min_dim = llmin(mDetailTextures[i]->getFullWidth(), mDetailTextures[i]->getFullHeight());
			S32 ddiscard = 0;
			while (min_dim > BASE_SIZE && ddiscard < MAX_DISCARD_LEVEL)
			{
				ddiscard++;
				min_dim /= 2;
			}

			BOOL delete_raw = (mDetailTextures[i]->reloadRawImage(ddiscard) != NULL) ;
			if(mDetailTextures[i]->getRawImageLevel() != ddiscard)//raw iamge is not ready, will enter here again later.
			{
				if(delete_raw)
				{
					mDetailTextures[i]->destroyRawImage() ;
				}
				lldebugs << "cached raw data for terrain detail texture is not ready yet: " << mDetailTextures[i]->getID() << llendl;
				return FALSE;
			}

			mRawImages[i] = mDetailTextures[i]->getRawImage() ;
			if(delete_raw)
			{
				mDetailTextures[i]->destroyRawImage() ;
			}
			if (mDetailTextures[i]->getWidth(ddiscard) != BASE_SIZE ||
				mDetailTextures[i]->getHeight(ddiscard) != BASE_SIZE ||
				mDetailTextures[i]->getComponents() != 3)
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
	x_end = llround( (x + width) * mScaleInv );
	y_end = llround( (y + width) * mScaleInv );

	if (x_end > mWidth)
	{
		llwarns << "x end > width" << llendl;
		x_end = mWidth;
	}
	if (y_end > mWidth)
	{
		llwarns << "y end > width" << llendl;
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
		llwarns << "Base texture comps != input texture comps" << llendl;
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
					//llwarns << "offset 0 [" << tex0 << "] =" << st_offset << " >= size=" << st_data_size[tex0] << llendl;
					//llwarns << "offset 1 [" << tex1 << "] =" << st_offset << " >= size=" << st_data_size[tex1] << llendl;
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
	LLSurface::sTextureUpdateTime += gen_timer.getElapsedTimeF32();
	LLSurface::sTexelsUpdated += (tex_x_end - tex_x_begin) * (tex_y_end - tex_y_begin);

	for (S32 i = 0; i < 4; i++)
	{
		// Un-boost detatil textures (will get re-boosted if rendering in high detail)
		mDetailTextures[i]->setBoostLevel(LLGLTexture::BOOST_NONE);
		mDetailTextures[i]->setMinDiscardLevel(MAX_DISCARD_LEVEL + 1);
	}
	
	return TRUE;
}

LLUUID LLVLComposition::getDetailTextureID(S32 corner)
{
	return mDetailTextures[corner]->getID();
}

LLViewerFetchedTexture* LLVLComposition::getDetailTexture(S32 corner)
{
	return mDetailTextures[corner];
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
