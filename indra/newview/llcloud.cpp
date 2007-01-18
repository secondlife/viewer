/** 
 * @file llcloud.cpp
 * @brief Implementation of viewer LLCloudLayer class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llmath.h"
//#include "vmath.h"
#include "v3math.h"
#include "v4math.h"
#include "llquaternion.h"
#include "v4color.h"

#include "llwind.h"
#include "llcloud.h"
#include "llgl.h"
#include "llviewerobjectlist.h"
#include "llvoclouds.h"
#include "llvosky.h"
#include "llsky.h"
#include "llviewerregion.h"
#include "patch_dct.h"
#include "patch_code.h"
#include "llglheaders.h"
#include "pipeline.h"
#include "lldrawpool.h"
#include "llworld.h"

extern LLPipeline gPipeline;

const F32 CLOUD_UPDATE_RATE = 1.0f;  // Global time dilation for clouds
const F32 CLOUD_GROW_RATE = 0.05f;
const F32 CLOUD_DECAY_RATE = -0.05f;
const F32 CLOUD_VELOCITY_SCALE = 0.01f;
const F32 CLOUD_DENSITY = 25.f;
const S32 CLOUD_COUNT_MAX = 20;
const F32 CLOUD_HEIGHT_RANGE = 48.f;
const F32 CLOUD_HEIGHT_MEAN = 192.f;

enum
{
	LL_PUFF_GROWING = 0,
	LL_PUFF_DYING = 1
};

// Used for patch decoder
S32 gBuffer[16*16];


//static
S32 LLCloudPuff::sPuffCount = 0;

LLCloudPuff::LLCloudPuff() :
	mAlpha(0.01f),
	mRate(CLOUD_GROW_RATE*CLOUD_UPDATE_RATE),
	mLifeState(LL_PUFF_GROWING)
{
}

LLCloudGroup::LLCloudGroup() :
	mCloudLayerp(NULL),
	mDensity(0.f),
	mTargetPuffCount(0),
	mVOCloudsp(NULL)
{
}

void LLCloudGroup::cleanup()
{
	if (mVOCloudsp)
	{
		if (!mVOCloudsp->isDead())
		{
			gObjectList.killObject(mVOCloudsp);
		}
		mVOCloudsp = NULL;
	}
}

void LLCloudGroup::setCenterRegion(const LLVector3 &center)
{
	mCenterRegion = center;
}

void LLCloudGroup::updatePuffs(const F32 dt)
{
	mDensity = mCloudLayerp->getDensityRegion(mCenterRegion);

	if (!mVOCloudsp)
	{
		mVOCloudsp = (LLVOClouds *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_CLOUDS, mCloudLayerp->getRegion());
		mVOCloudsp->setCloudGroup(this);
		mVOCloudsp->setPositionRegion(mCenterRegion);
		mVOCloudsp->setScale(LLVector3(256.f/CLOUD_GROUPS_PER_EDGE + CLOUD_PUFF_WIDTH,
										 256.f/CLOUD_GROUPS_PER_EDGE + CLOUD_PUFF_WIDTH,
										 CLOUD_HEIGHT_RANGE + CLOUD_PUFF_HEIGHT));
		gPipeline.addObject(mVOCloudsp);
	}

	S32 i;

	LLVector3 velocity;
	LLVector3d vel_d;
	// Update the positions of all of the clouds
	for (i = 0; i < mCloudPuffs.count(); i++)
	{
		LLCloudPuff &puff = mCloudPuffs[i];
		velocity = mCloudLayerp->getRegion()->mWind.getCloudVelocity(mCloudLayerp->getRegion()->getPosRegionFromGlobal(puff.mPositionGlobal));
		velocity *= CLOUD_VELOCITY_SCALE*CLOUD_UPDATE_RATE;
		vel_d.setVec(velocity);
		mCloudPuffs[i].mPositionGlobal += vel_d;
		mCloudPuffs[i].mAlpha += mCloudPuffs[i].mRate * dt;
		mCloudPuffs[i].mAlpha = llmin(1.f, mCloudPuffs[i].mAlpha);
		mCloudPuffs[i].mAlpha = llmax(0.f, mCloudPuffs[i].mAlpha);
	}
}

void LLCloudGroup::updatePuffOwnership()
{
	S32 i = 0;
	while (i < mCloudPuffs.count())
	{
		if (mCloudPuffs[i].getLifeState() == LL_PUFF_DYING)
		{
			i++;
			continue;
		}
		if (inGroup(mCloudPuffs[i]))
		{
			i++;
			continue;
		}

		//llinfos << "Cloud moving to new group" << llendl;
		LLCloudGroup *new_cgp = gWorldPointer->findCloudGroup(mCloudPuffs[i]);
		if (!new_cgp)
		{
			//llinfos << "Killing puff not in group" << llendl;
			mCloudPuffs[i].setLifeState(LL_PUFF_DYING);
			mCloudPuffs[i].mRate = CLOUD_DECAY_RATE*CLOUD_UPDATE_RATE;
			i++;
			continue;
		}
		//llinfos << "Puff handed off!" << llendl;
		LLCloudPuff *puffp = new_cgp->mCloudPuffs.reserve_block(1);
		puffp->mPositionGlobal = mCloudPuffs[i].mPositionGlobal;
		puffp->mAlpha = mCloudPuffs[i].mAlpha;
		mCloudPuffs.remove(i);
	}

	//llinfos << "Puff count: " << LLCloudPuff::sPuffCount << llendl;
}

void LLCloudGroup::updatePuffCount()
{
	if (!mVOCloudsp)
	{
		return;
	}
	S32 i;
	S32 target_puff_count = llround(CLOUD_DENSITY * mDensity);
	target_puff_count = llmax(0, target_puff_count);
	target_puff_count = llmin(CLOUD_COUNT_MAX, target_puff_count);
	S32 current_puff_count = mCloudPuffs.count();
	// Create a new cloud if we need one
	if (current_puff_count < target_puff_count)
	{
		LLVector3d puff_pos_global;
		mCloudPuffs.resize(target_puff_count);
		for (i = current_puff_count; i < target_puff_count; i++)
		{
			puff_pos_global = mVOCloudsp->getPositionGlobal();
			F32 x = ll_frand(256.f/CLOUD_GROUPS_PER_EDGE) - 128.f/CLOUD_GROUPS_PER_EDGE;
			F32 y = ll_frand(256.f/CLOUD_GROUPS_PER_EDGE) - 128.f/CLOUD_GROUPS_PER_EDGE;
			F32 z = ll_frand(CLOUD_HEIGHT_RANGE) - 0.5f*CLOUD_HEIGHT_RANGE;
			puff_pos_global += LLVector3d(x, y, z);
			mCloudPuffs[i].mPositionGlobal = puff_pos_global;
			mCloudPuffs[i].mAlpha = 0.01f;
			LLCloudPuff::sPuffCount++;
		}
	}

	// Count the number of live puffs
	S32 live_puff_count = 0;
	for (i = 0; i < mCloudPuffs.count(); i++)
	{
		if (mCloudPuffs[i].getLifeState() != LL_PUFF_DYING)
		{
			live_puff_count++;
		}
	}


	// Start killing enough puffs so the live puff count == target puff count
	S32 new_dying_count = llmax(0, live_puff_count - target_puff_count);
	i = 0;
	while (new_dying_count > 0)
	{
		if (mCloudPuffs[i].getLifeState() != LL_PUFF_DYING)
		{
			//llinfos << "Killing extra live cloud" << llendl;
			mCloudPuffs[i].setLifeState(LL_PUFF_DYING);
			mCloudPuffs[i].mRate = CLOUD_DECAY_RATE*CLOUD_UPDATE_RATE;
			new_dying_count--;
		}
		i++;
	}

	// Remove fully dead puffs
	i = 0;
	while (i < mCloudPuffs.count())
	{
		if (mCloudPuffs[i].isDead())
		{
			//llinfos << "Removing dead puff!" << llendl;
			mCloudPuffs.remove(i);
			LLCloudPuff::sPuffCount--;
		}
		else
		{
			i++;
		}
	}
}

BOOL LLCloudGroup::inGroup(const LLCloudPuff &puff) const
{
	// Do min/max check on center of the cloud puff
	F32 min_x, min_y, max_x, max_y;
	F32 delta = 128.f/CLOUD_GROUPS_PER_EDGE;
	min_x = mCenterRegion.mV[VX] - delta;
	min_y = mCenterRegion.mV[VY] - delta;
	max_x = mCenterRegion.mV[VX] + delta;
	max_y = mCenterRegion.mV[VY] + delta;

	LLVector3 pos_region = mCloudLayerp->getRegion()->getPosRegionFromGlobal(puff.getPositionGlobal());

	if ((pos_region.mV[VX] < min_x)
		|| (pos_region.mV[VY] < min_y)
		|| (pos_region.mV[VX] > max_x)
		|| (pos_region.mV[VY] > max_y))
	{
		return FALSE;
	}
	return TRUE;
}

LLCloudLayer::LLCloudLayer()
: 	mOriginGlobal(0.0f, 0.0f, 0.0f),
	mMetersPerEdge(1.0f),
	mMetersPerGrid(1.0f),
	mWindp(NULL),
	mDensityp(NULL)
{
	S32 i, j;
	for (i = 0; i < 4; i++)
	{
		mNeighbors[i] = NULL;
	}

	F32 x, y;
	for (i = 0; i < CLOUD_GROUPS_PER_EDGE; i++)
	{
		y = (0.5f + i)*(256.f/CLOUD_GROUPS_PER_EDGE);
		for (j = 0; j < CLOUD_GROUPS_PER_EDGE; j++)
		{
			x = (0.5f + j)*(256.f/CLOUD_GROUPS_PER_EDGE);

			mCloudGroups[i][j].setCloudLayerp(this);
			mCloudGroups[i][j].setCenterRegion(LLVector3(x, y, CLOUD_HEIGHT_MEAN));
		}
	}
}



LLCloudLayer::~LLCloudLayer()
{
	destroy();
}


void LLCloudLayer::create(LLViewerRegion *regionp)
{
	llassert(regionp);

	mRegionp = regionp;
	mDensityp = new F32 [CLOUD_GRIDS_PER_EDGE * CLOUD_GRIDS_PER_EDGE];

	U32 i;
	for (i = 0; i < CLOUD_GRIDS_PER_EDGE*CLOUD_GRIDS_PER_EDGE; i++)
	{
		mDensityp[i] = 0.f;
	}
}

void LLCloudLayer::setRegion(LLViewerRegion *regionp)
{
	mRegionp = regionp;
}

void LLCloudLayer::destroy()
{
	// Kill all of the existing puffs
	S32 i, j;
	
	for (i = 0; i < CLOUD_GROUPS_PER_EDGE; i++)
	{
		for (j = 0; j < CLOUD_GROUPS_PER_EDGE; j++)
		{
			mCloudGroups[i][j].cleanup();
		}
	}

	delete [] mDensityp;
	mDensityp = NULL;
	mWindp = NULL;
}


void LLCloudLayer::reset()
{
}


void LLCloudLayer::setWindPointer(LLWind *windp)
{
	if (mWindp)
	{
		mWindp->setCloudDensityPointer(NULL);
	}
	mWindp = windp;
	if (mWindp)
	{
		mWindp->setCloudDensityPointer(mDensityp);
	}
}


void LLCloudLayer::setWidth(F32 width)
{
	mMetersPerEdge = width;
	mMetersPerGrid = width / CLOUD_GRIDS_PER_EDGE;
}


F32 LLCloudLayer::getDensityRegion(const LLVector3 &pos_region)
{	
	// "position" is region-local
	S32 i, j, ii, jj;

	i = lltrunc(pos_region.mV[VX] / mMetersPerGrid);
	j = lltrunc(pos_region.mV[VY] / mMetersPerGrid);
	ii = i + 1;
	jj = j + 1;


	// clamp 
	if (i >= (S32)CLOUD_GRIDS_PER_EDGE)
	{
		i = CLOUD_GRIDS_PER_EDGE - 1;
		ii = i;
	}
	else if (i < 0)
	{
		i = 0;
		ii = i;
	}
	else if (ii >= (S32)CLOUD_GRIDS_PER_EDGE || ii < 0)
	{
		ii = i;
	}

	if (j >= (S32)CLOUD_GRIDS_PER_EDGE)
	{
		j = CLOUD_GRIDS_PER_EDGE - 1;
		jj = j;
	}
	else if (j < 0)
	{
		j = 0;
		jj = j;
	}
	else if (jj >= (S32)CLOUD_GRIDS_PER_EDGE || jj < 0)
	{
		jj = j;
	}

	F32 dx = (pos_region.mV[VX] - (F32) i * mMetersPerGrid) / mMetersPerGrid;
	F32 dy = (pos_region.mV[VY] - (F32) j * mMetersPerGrid) / mMetersPerGrid;
	F32 omdx = 1.0f - dx;
	F32 omdy = 1.0f - dy;

	F32 density = dx * dy * *(mDensityp + ii + jj * CLOUD_GRIDS_PER_EDGE) + 
	   			  dx * omdy * *(mDensityp + i + jj * CLOUD_GRIDS_PER_EDGE) +
				  omdx * dy * *(mDensityp + ii + j * CLOUD_GRIDS_PER_EDGE) +
				  omdx * omdy * *(mDensityp + i + j * CLOUD_GRIDS_PER_EDGE);	  

	return density;
}

// a debug method that may yet be useful
void LLCloudLayer::renderDensityField()
{
// 	F32 x, y, z;
// 	U32 i, j, k;
// 	LLGLSNoTexture gls_ui_no_texture;
// 	// Render a bunch of triangles to represent the cloud density field
// 	glBegin(GL_TRIANGLES);
// 	for(j=0; j<CLOUD_GRIDS_PER_EDGE-1; j++)
// 	{
// 		y = j * mMetersPerGrid;

// 		for(i=0; i<CLOUD_GRIDS_PER_EDGE; i++)
// 		{
// 			k = i + j*CLOUD_GRIDS_PER_EDGE;
// 			x = i * mMetersPerGrid;

// 			z = 0.5f * CLOUD_MAX_HEIGHT + 40.0f * *(mDensityp + k + CLOUD_GRIDS_PER_EDGE);
// 			glColor3f(1.0f - *(mDensityp + k + CLOUD_GRIDS_PER_EDGE), *(mDensityp + k + CLOUD_GRIDS_PER_EDGE), 0.0f);
// 			glVertex3f(x, y+mMetersPerGrid, z);

// 			z = 0.5f * CLOUD_MAX_HEIGHT + 40.0f * *(mDensityp + k);
// 			glColor3f(1.0f - *(mDensityp + k), *(mDensityp + k), 0.0f);
// 			glVertex3f(x, y, z);

// 			z = 0.5f * CLOUD_MAX_HEIGHT + 40.0f * *(mDensityp + k + 1);
// 			glColor3f(1.0f - *(mDensityp + k + 1), *(mDensityp + k + 1), 0.0f);
// 			glVertex3f(x+mMetersPerGrid, y, z);

// 		}
// 	}
// 	glEnd();
}


void LLCloudLayer::decompress(LLBitPack &bitpack, LLGroupHeader *group_headerp)
{
	LLPatchHeader  patch_header;

	init_patch_decompressor(group_headerp->patch_size);

	// Don't use the packed group_header stride because the strides used on
	// simulator and viewer are not equal.
	group_headerp->stride = group_headerp->patch_size; 		// offset required to step up one row
	set_group_of_patch_header(group_headerp);

	decode_patch_header(bitpack, &patch_header);
	decode_patch(bitpack, gBuffer);
	decompress_patch(mDensityp, gBuffer, &patch_header); 
}

void LLCloudLayer::updatePuffs(const F32 dt)
{
	// We want to iterate through all of the cloud groups
	// and update their density targets

	S32 i, j;
	
	for (i = 0; i < CLOUD_GROUPS_PER_EDGE; i++)
	{
		for (j = 0; j < CLOUD_GROUPS_PER_EDGE; j++)
		{
			mCloudGroups[i][j].updatePuffs(dt);
		}
	}
}

void LLCloudLayer::updatePuffOwnership()
{
	S32 i, j;
	
	for (i = 0; i < CLOUD_GROUPS_PER_EDGE; i++)
	{
		for (j = 0; j < CLOUD_GROUPS_PER_EDGE; j++)
		{
			mCloudGroups[i][j].updatePuffOwnership();
		}
	}
}

void LLCloudLayer::updatePuffCount()
{
	S32 i, j;
	
	for (i = 0; i < CLOUD_GROUPS_PER_EDGE; i++)
	{
		for (j = 0; j < CLOUD_GROUPS_PER_EDGE; j++)
		{
			mCloudGroups[i][j].updatePuffCount();
		}
	}
}

LLCloudGroup *LLCloudLayer::findCloudGroup(const LLCloudPuff &puff)
{
	S32 i, j;
	
	for (i = 0; i < CLOUD_GROUPS_PER_EDGE; i++)
	{
		for (j = 0; j < CLOUD_GROUPS_PER_EDGE; j++)
		{
			if (mCloudGroups[i][j].inGroup(puff))
			{
				return &(mCloudGroups[i][j]);
			}
		}
	}
	return NULL;
}



void LLCloudLayer::connectNeighbor(LLCloudLayer *cloudp, U32 direction)
{
	if (direction >= 4)
	{
		// Only care about cardinal 4 directions.
		return;
	}

	mNeighbors[direction] = cloudp;
	if (cloudp)
		mNeighbors[direction]->mNeighbors[gDirOpposite[direction]] = this;
}


void LLCloudLayer::disconnectNeighbor(U32 direction)
{
	if (direction >= 4)
	{
		// Only care about cardinal 4 directions.
		return;
	}

	if (mNeighbors[direction])
	{
		mNeighbors[direction]->mNeighbors[gDirOpposite[direction]] = NULL;
		mNeighbors[direction] = NULL;
	}
}


void LLCloudLayer::disconnectAllNeighbors()
{
	S32 i;
	for (i = 0; i < 4; i++)
	{
		disconnectNeighbor(i);
	}
}
