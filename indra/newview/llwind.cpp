/** 
 * @file llwind.cpp
 * @brief LLWind class implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

// Wind is a lattice.  It is computed on the simulator, and transmitted to the viewer.
// It drives special effects like smoke blowing, trees bending, and grass wiggling.
//
// Currently wind lattice does not interpolate correctly to neighbors.  This will need 
// work.

#include "llviewerprecompiledheaders.h"
#include "indra_constants.h"

#include "llwind.h"

// linden libraries
#include "llgl.h"
#include "patch_dct.h"
#include "patch_code.h"

// viewer
#include "noise.h"
#include "v4color.h"
#include "llworld.h"


const F32 CLOUD_DIVERGENCE_COEF = 0.5f; 


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LLWind::LLWind()
:	mSize(16),
	mCloudDensityp(NULL)
{
	init();
}


LLWind::~LLWind()
{
	delete [] mVelX;
	delete [] mVelY;
	delete [] mCloudVelX;
	delete [] mCloudVelY;
}


//////////////////////////////////////////////////////////////////////
// Public Methods
//////////////////////////////////////////////////////////////////////


void LLWind::init()
{
	// Initialize vector data
	mVelX = new F32[mSize*mSize];
	mVelY = new F32[mSize*mSize];

	mCloudVelX = new F32[mSize*mSize];
	mCloudVelY = new F32[mSize*mSize];

	S32 i;
	for (i = 0; i < mSize*mSize; i++)
	{
		mVelX[i] = 0.5f;
		mVelY[i] = 0.5f;
		mCloudVelX[i] = 0.0f;
		mCloudVelY[i] = 0.0f;
	}
}


void LLWind::decompress(LLBitPack &bitpack, LLGroupHeader *group_headerp)
{
	if (!mCloudDensityp)
	{
		return;
	}

	LLPatchHeader  patch_header;
	S32 buffer[16*16];

	init_patch_decompressor(group_headerp->patch_size);

	// Don't use the packed group_header stride because the strides used on
	// simulator and viewer are not equal.
	group_headerp->stride = group_headerp->patch_size;	
	set_group_of_patch_header(group_headerp);

	// X component
	decode_patch_header(bitpack, &patch_header);
	decode_patch(bitpack, buffer);
	decompress_patch(mVelX, buffer, &patch_header);

	// Y component
	decode_patch_header(bitpack, &patch_header);
	decode_patch(bitpack, buffer);
	decompress_patch(mVelY, buffer, &patch_header);



	S32 i, j, k;
	// HACK -- mCloudVelXY is the same as mVelXY, except we add a divergence
	// that is proportional to the gradient of the cloud density 
	// ==> this helps to clump clouds together
	// NOTE ASSUMPTION: cloud density has the same dimensions as the wind field
	// This needs to be fixed... causes discrepency at region boundaries

	for (j=1; j<mSize-1; j++)
	{
		for (i=1; i<mSize-1; i++)
		{
			k = i + j * mSize;
			*(mCloudVelX + k) = *(mVelX + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 1) - *(mCloudDensityp + k - 1));
			*(mCloudVelY + k) = *(mVelY + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + mSize) - *(mCloudDensityp + k - mSize));
		}
	}

	i = mSize - 1;
	for (j=1; j<mSize-1; j++)
	{
		k = i + j * mSize;
		*(mCloudVelX + k) = *(mVelX + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k) - *(mCloudDensityp + k - 2));
		*(mCloudVelY + k) = *(mVelY + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + mSize) - *(mCloudDensityp + k - mSize));
	}
	i = 0;
	for (j=1; j<mSize-1; j++)
	{
		k = i + j * mSize;
		*(mCloudVelX + k) = *(mVelX + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 2) - *(mCloudDensityp + k));
		*(mCloudVelY + k) = *(mVelY + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + mSize) - *(mCloudDensityp + k + mSize));
	}
	j = mSize - 1;
	for (i=1; i<mSize-1; i++)
	{
		k = i + j * mSize;
		*(mCloudVelX + k) = *(mVelX + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 1) - *(mCloudDensityp + k - 1));
		*(mCloudVelY + k) = *(mVelY + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k) - *(mCloudDensityp + k - 2*mSize));
	}
	j = 0;
	for (i=1; i<mSize-1; i++)
	{
		k = i + j * mSize;
		*(mCloudVelX + k) = *(mVelX + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 1) - *(mCloudDensityp + k -1));
		*(mCloudVelY + k) = *(mVelY + k) + CLOUD_DIVERGENCE_COEF * (*(mCloudDensityp + k + 2*mSize) - *(mCloudDensityp + k));
	}
}


LLVector3 LLWind::getAverage()
{
	//  Returns in average_wind the average wind velocity 
	LLVector3 average(0.0f, 0.0f, 0.0f);	
	S32 i, grid_count;
	grid_count = mSize * mSize;
	for (i = 0; i < grid_count; i++)
	{
		average.mV[VX] += mVelX[i];
		average.mV[VY] += mVelY[i];
	}

	average *= 1.f/((F32)(grid_count)) * WIND_SCALE_HACK;
	return average;
}


LLVector3 LLWind::getVelocityNoisy(const LLVector3 &pos_region, const F32 dim)
{
	//  Resolve a value, using fractal summing to perturb the returned value 
	LLVector3 r_val(0,0,0);
	F32 norm = 1.0f;
	if (dim == 8)
	{
		norm = 1.875;
	}
	else if (dim == 4)
	{
		norm = 1.75;
	}
	else if (dim == 2)
	{
		norm = 1.5;
	}

	F32 temp_dim = dim;
	while (temp_dim >= 1.0)
	{
		LLVector3 pos_region_scaled(pos_region * temp_dim);
		r_val += getVelocity(pos_region_scaled) * (1.0f/temp_dim);
		temp_dim /= 2.0;
	}
	
	return r_val * (1.0f/norm) * WIND_SCALE_HACK;
}


LLVector3 LLWind::getVelocity(const LLVector3 &pos_region)
{
	llassert(mSize == 16);
	// Resolves value of wind at a location relative to SW corner of region
	//  
	// Returns wind magnitude in X,Y components of vector3
	LLVector3 r_val;
	F32 dx,dy;
	S32 k;

	LLVector3 pos_clamped_region(pos_region);
	
	F32 region_width_meters = LLWorld::getInstance()->getRegionWidthInMeters();

	if (pos_clamped_region.mV[VX] < 0.f)
	{
		pos_clamped_region.mV[VX] = 0.f;
	}
	else if (pos_clamped_region.mV[VX] >= region_width_meters)
	{
		pos_clamped_region.mV[VX] = (F32) fmod(pos_clamped_region.mV[VX], region_width_meters);
	}

	if (pos_clamped_region.mV[VY] < 0.f)
	{
		pos_clamped_region.mV[VY] = 0.f;
	}
	else if (pos_clamped_region.mV[VY] >= region_width_meters)
	{
		pos_clamped_region.mV[VY] = (F32) fmod(pos_clamped_region.mV[VY], region_width_meters);
	}
	
	
	S32 i = llfloor(pos_clamped_region.mV[VX] * mSize / region_width_meters);
	S32 j = llfloor(pos_clamped_region.mV[VY] * mSize / region_width_meters);
	k = i + j*mSize;
	dx = ((pos_clamped_region.mV[VX] * mSize / region_width_meters) - (F32) i);
	dy = ((pos_clamped_region.mV[VY] * mSize / region_width_meters) - (F32) j);

	if ((i < mSize-1) && (j < mSize-1))
	{
		//  Interior points, no edges
		r_val.mV[VX] =  mVelX[k]*(1.0f - dx)*(1.0f - dy) + 
						mVelX[k + 1]*dx*(1.0f - dy) + 
						mVelX[k + mSize]*dy*(1.0f - dx) + 
						mVelX[k + mSize + 1]*dx*dy;
		r_val.mV[VY] =  mVelY[k]*(1.0f - dx)*(1.0f - dy) + 
						mVelY[k + 1]*dx*(1.0f - dy) + 
						mVelY[k + mSize]*dy*(1.0f - dx) + 
						mVelY[k + mSize + 1]*dx*dy;
	}
	else 
	{
		r_val.mV[VX] = mVelX[k];
		r_val.mV[VY] = mVelY[k];
	}

	r_val.mV[VZ] = 0.f;
	return r_val * WIND_SCALE_HACK;
}


LLVector3 LLWind::getCloudVelocity(const LLVector3 &pos_region)
{
	llassert(mSize == 16);
	// Resolves value of wind at a location relative to SW corner of region
	//  
	// Returns wind magnitude in X,Y components of vector3
	LLVector3 r_val;
	F32 dx,dy;
	S32 k;

	LLVector3 pos_clamped_region(pos_region);
	
	F32 region_width_meters = LLWorld::getInstance()->getRegionWidthInMeters();

	if (pos_clamped_region.mV[VX] < 0.f)
	{
		pos_clamped_region.mV[VX] = 0.f;
	}
	else if (pos_clamped_region.mV[VX] >= region_width_meters)
	{
		pos_clamped_region.mV[VX] = (F32) fmod(pos_clamped_region.mV[VX], region_width_meters);
	}

	if (pos_clamped_region.mV[VY] < 0.f)
	{
		pos_clamped_region.mV[VY] = 0.f;
	}
	else if (pos_clamped_region.mV[VY] >= region_width_meters)
	{
		pos_clamped_region.mV[VY] = (F32) fmod(pos_clamped_region.mV[VY], region_width_meters);
	}
	
	
	S32 i = llfloor(pos_clamped_region.mV[VX] * mSize / region_width_meters);
	S32 j = llfloor(pos_clamped_region.mV[VY] * mSize / region_width_meters);
	k = i + j*mSize;
	dx = ((pos_clamped_region.mV[VX] * mSize / region_width_meters) - (F32) i);
	dy = ((pos_clamped_region.mV[VY] * mSize / region_width_meters) - (F32) j);

	if ((i < mSize-1) && (j < mSize-1))
	{
		//  Interior points, no edges
		r_val.mV[VX] =  mCloudVelX[k]*(1.0f - dx)*(1.0f - dy) + 
						mCloudVelX[k + 1]*dx*(1.0f - dy) + 
						mCloudVelX[k + mSize]*dy*(1.0f - dx) + 
						mCloudVelX[k + mSize + 1]*dx*dy;
		r_val.mV[VY] =  mCloudVelY[k]*(1.0f - dx)*(1.0f - dy) + 
						mCloudVelY[k + 1]*dx*(1.0f - dy) + 
						mCloudVelY[k + mSize]*dy*(1.0f - dx) + 
						mCloudVelY[k + mSize + 1]*dx*dy;
	}
	else 
	{
		r_val.mV[VX] = mCloudVelX[k];
		r_val.mV[VY] = mCloudVelY[k];
	}

	r_val.mV[VZ] = 0.f;
	return r_val * WIND_SCALE_HACK;
}


void LLWind::setCloudDensityPointer(F32 *densityp)
{
	mCloudDensityp = densityp;
}

void LLWind::setOriginGlobal(const LLVector3d &origin_global)
{
	mOriginGlobal = origin_global;
}


