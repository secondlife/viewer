/** 
 * @file llsurface.cpp
 * @brief Implementation of LLSurface class
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llsurface.h"

#include "llrender.h"

#include "llviewertexturelist.h"
#include "llpatchvertexarray.h"
#include "patch_dct.h"
#include "patch_code.h"
#include "bitpack.h"
#include "llviewerobjectlist.h"
#include "llregionhandle.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llworld.h"
#include "llviewercontrol.h"
#include "llviewertexture.h"
#include "llsurfacepatch.h"
#include "llvosurfacepatch.h"
#include "llvowater.h"
#include "pipeline.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"
#include "noise.h"
#include "llviewercamera.h"
#include "llglheaders.h"
#include "lldrawpoolterrain.h"
#include "lldrawable.h"

extern LLPipeline gPipeline;
extern bool gShiftFrame;

LLColor4U MAX_WATER_COLOR(0, 48, 96, 240);


S32 LLSurface::sTextureSize = 256;
S32 LLSurface::sTexelsUpdated = 0;
F32 LLSurface::sTextureUpdateTime = 0.f;

// ---------------- LLSurface:: Public Members ---------------

LLSurface::LLSurface(U32 type, LLViewerRegion *regionp) :
	mGridsPerEdge(0),
	mOOGridsPerEdge(0.f),
	mPatchesPerEdge(0),
	mNumberOfPatches(0),
	mType(type),
	mDetailTextureScale(0.f),
	mOriginGlobal(0.0, 0.0, 0.0),
	mSTexturep(NULL),
	mWaterTexturep(NULL),
	mGridsPerPatchEdge(0),
	mMetersPerGrid(1.0f),
	mMetersPerEdge(1.0f),
	mRegionp(regionp)
{
	// Surface data
	mSurfaceZ = NULL;
	mNorm = NULL;

	// Patch data
	mPatchList = NULL;

	// One of each for each camera
	mVisiblePatchCount = 0;

	mHasZData = FALSE;
	// "uninitialized" min/max z
	mMinZ = 10000.f;
	mMaxZ = -10000.f;

	mWaterObjp = NULL;

	// In here temporarily.
	mSurfacePatchUpdateCount = 0;

	for (S32 i = 0; i < 8; i++)
	{
		mNeighbors[i] = NULL;
	}
}


LLSurface::~LLSurface()
{
	delete [] mSurfaceZ;
	mSurfaceZ = NULL;

	delete [] mNorm;

	mGridsPerEdge = 0;
	mGridsPerPatchEdge = 0;
	mPatchesPerEdge = 0;
	mNumberOfPatches = 0;
	destroyPatchData();

	LLDrawPoolTerrain *poolp = (LLDrawPoolTerrain*) gPipeline.findPool(LLDrawPool::POOL_TERRAIN, mSTexturep);
	if (!poolp)
	{
		llwarns << "No pool for terrain on destruction!" << llendl;
	}
	else if (poolp->mReferences.empty())
	{
		gPipeline.removePool(poolp);
		// Don't enable this until we blitz the draw pool for it as well.  -- djs
		if (mSTexturep)
		{
			mSTexturep = NULL;
		}
		if (mWaterTexturep)
		{
			mWaterTexturep = NULL;
		}
	}
	else
	{
		llerrs << "Terrain pool not empty!" << llendl;
	}
}

void LLSurface::initClasses()
{
}

void LLSurface::setRegion(LLViewerRegion *regionp)
{
	mRegionp = regionp;
	mWaterObjp = NULL; // depends on regionp, needs recreating
}

// Assumes that arguments are powers of 2, and that
// grids_per_edge / grids_per_patch_edge = power of 2 
void LLSurface::create(const S32 grids_per_edge,
					   const S32 grids_per_patch_edge,
					   const LLVector3d &origin_global,
					   const F32 width) 
{
	// Initialize various constants for the surface
	mGridsPerEdge = grids_per_edge + 1;  // Add 1 for the east and north buffer
	mOOGridsPerEdge = 1.f / mGridsPerEdge;
	mGridsPerPatchEdge = grids_per_patch_edge;
	mPatchesPerEdge = (mGridsPerEdge - 1) / mGridsPerPatchEdge;
	mNumberOfPatches = mPatchesPerEdge * mPatchesPerEdge;
	mMetersPerGrid = width / ((F32)(mGridsPerEdge - 1));
	mMetersPerEdge = mMetersPerGrid * (mGridsPerEdge - 1);

	mOriginGlobal.setVec(origin_global);

	mPVArray.create(mGridsPerEdge, mGridsPerPatchEdge, LLWorld::getInstance()->getRegionScale());

	S32 number_of_grids = mGridsPerEdge * mGridsPerEdge;

	/////////////////////////////////////
	//
	// Initialize data arrays for surface
	///
	mSurfaceZ = new F32[number_of_grids];
	mNorm = new LLVector3[number_of_grids];

	// Reset the surface to be a flat square grid
	for(S32 i=0; i < number_of_grids; i++) 
	{
		// Surface is flat and zero
		// Normals all point up
		mSurfaceZ[i] = 0.0f;
		mNorm[i].setVec(0.f, 0.f, 1.f);
	}


	mVisiblePatchCount = 0;


	///////////////////////
	//
	// Initialize textures
	//

	initTextures();

	// Has to be done after texture initialization
	createPatchData();
}

LLViewerTexture* LLSurface::getSTexture()
{
	if (mSTexturep.notNull() && !mSTexturep->hasGLTexture())
	{
		createSTexture();
	}
	return mSTexturep;
}

LLViewerTexture* LLSurface::getWaterTexture()
{
	if (mWaterTexturep.notNull() && !mWaterTexturep->hasGLTexture())
	{
		createWaterTexture();
	}
	return mWaterTexturep;
}

void LLSurface::createSTexture()
{
	if (!mSTexturep)
	{
		// Fill with dummy gray data.	
		// GL NOT ACTIVE HERE
		LLPointer<LLImageRaw> raw = new LLImageRaw(sTextureSize, sTextureSize, 3);
		U8 *default_texture = raw->getData();
		for (S32 i = 0; i < sTextureSize; i++)
		{
			for (S32 j = 0; j < sTextureSize; j++)
			{
				*(default_texture + (i*sTextureSize + j)*3) = 128;
				*(default_texture + (i*sTextureSize + j)*3 + 1) = 128;
				*(default_texture + (i*sTextureSize + j)*3 + 2) = 128;
			}
		}

		mSTexturep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
		mSTexturep->dontDiscard();
		gGL.getTexUnit(0)->bind(mSTexturep);
		mSTexturep->setAddressMode(LLTexUnit::TAM_CLAMP);		
	}
}

void LLSurface::createWaterTexture()
{
	if (!mWaterTexturep)
	{
		// Create the water texture
		LLPointer<LLImageRaw> raw = new LLImageRaw(sTextureSize/2, sTextureSize/2, 4);
		U8 *default_texture = raw->getData();
		for (S32 i = 0; i < sTextureSize/2; i++)
		{
			for (S32 j = 0; j < sTextureSize/2; j++)
			{
				*(default_texture + (i*sTextureSize/2 + j)*4) = MAX_WATER_COLOR.mV[0];
				*(default_texture + (i*sTextureSize/2 + j)*4 + 1) = MAX_WATER_COLOR.mV[1];
				*(default_texture + (i*sTextureSize/2 + j)*4 + 2) = MAX_WATER_COLOR.mV[2];
				*(default_texture + (i*sTextureSize/2 + j)*4 + 3) = MAX_WATER_COLOR.mV[3];
			}
		}		
		
		mWaterTexturep = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE);
		mWaterTexturep->dontDiscard();
		gGL.getTexUnit(0)->bind(mWaterTexturep);
		mWaterTexturep->setAddressMode(LLTexUnit::TAM_CLAMP);
	}
}

void LLSurface::initTextures()
{
	///////////////////////
	//
	// Main surface texture
	//
	createSTexture();

	///////////////////////
	//
	// Water texture
	//
	if (gSavedSettings.getBOOL("RenderWater") )
	{
		createWaterTexture();
		mWaterObjp = (LLVOWater *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_WATER, mRegionp);
		gPipeline.createObject(mWaterObjp);
		LLVector3d water_pos_global = from_region_handle(mRegionp->getHandle());
		water_pos_global += LLVector3d(128.0, 128.0, DEFAULT_WATER_HEIGHT);		// region doesn't have a valid water height yet
		mWaterObjp->setPositionGlobal(water_pos_global);
	}
}


void LLSurface::setOriginGlobal(const LLVector3d &origin_global) 
{
	LLVector3d new_origin_global;
	mOriginGlobal = origin_global;
	LLSurfacePatch *patchp;
	S32 i, j;
	// Need to update the southwest corners of the patches
	for (j=0; j<mPatchesPerEdge; j++) 
	{
		for (i=0; i<mPatchesPerEdge; i++) 
		{
			patchp = getPatch(i, j);

			new_origin_global = patchp->getOriginGlobal();
			
			new_origin_global.mdV[0] = mOriginGlobal.mdV[0] + i * mMetersPerGrid * mGridsPerPatchEdge;
			new_origin_global.mdV[1] = mOriginGlobal.mdV[1] + j * mMetersPerGrid * mGridsPerPatchEdge;
			patchp->setOriginGlobal(new_origin_global);
		}
	}

	// Hack!
	if (mWaterObjp.notNull() && mWaterObjp->mDrawable.notNull())
	{
		const F64 x = origin_global.mdV[VX] + 128.0;
		const F64 y = origin_global.mdV[VY] + 128.0;
		const F64 z = mWaterObjp->getPositionGlobal().mdV[VZ];

		LLVector3d water_origin_global(x, y, z);

		mWaterObjp->setPositionGlobal(water_origin_global);
	}
}

void LLSurface::getNeighboringRegions( std::vector<LLViewerRegion*>& uniqueRegions )
{
	S32 i;
	for (i = 0; i < 8; i++)
	{
		if ( mNeighbors[i] != NULL )
		{
			uniqueRegions.push_back( mNeighbors[i]->getRegion() );
		}
	}	
}


void LLSurface::getNeighboringRegionsStatus( std::vector<S32>& regions )
{
	S32 i;
	for (i = 0; i < 8; i++)
	{
		if ( mNeighbors[i] != NULL )
		{
			regions.push_back( i );
		}
	}	
}

void LLSurface::connectNeighbor(LLSurface *neighborp, U32 direction)
{
	S32 i;
	LLSurfacePatch *patchp, *neighbor_patchp;

	mNeighbors[direction] = neighborp;
	neighborp->mNeighbors[gDirOpposite[direction]] = this;

	// Connect patches
	if (NORTHEAST == direction)
	{
		patchp = getPatch(mPatchesPerEdge - 1, mPatchesPerEdge - 1);
		neighbor_patchp = neighborp->getPatch(0, 0);

		patchp->connectNeighbor(neighbor_patchp, direction);
		neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);

		patchp->updateNorthEdge(); // Only update one of north or east.
		patchp->dirtyZ();
	}
	else if (NORTHWEST == direction)
	{
		patchp = getPatch(0, mPatchesPerEdge - 1);
		neighbor_patchp = neighborp->getPatch(mPatchesPerEdge - 1, 0);

		patchp->connectNeighbor(neighbor_patchp, direction);
		neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);
	}
	else if (SOUTHWEST == direction)
	{
		patchp = getPatch(0, 0);
		neighbor_patchp = neighborp->getPatch(mPatchesPerEdge - 1, mPatchesPerEdge - 1);

		patchp->connectNeighbor(neighbor_patchp, direction);
		neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);

		neighbor_patchp->updateNorthEdge(); // Only update one of north or east.
		neighbor_patchp->dirtyZ();
	}
	else if (SOUTHEAST == direction)
	{
		patchp = getPatch(mPatchesPerEdge - 1, 0);
		neighbor_patchp = neighborp->getPatch(0, mPatchesPerEdge - 1);

		patchp->connectNeighbor(neighbor_patchp, direction);
		neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);
	}
	else if (EAST == direction)
	{
		// Do east/west connections, first
		for (i = 0; i < (S32)mPatchesPerEdge; i++)
		{
			patchp = getPatch(mPatchesPerEdge - 1, i);
			neighbor_patchp = neighborp->getPatch(0, i);

			patchp->connectNeighbor(neighbor_patchp, direction);
			neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);

			patchp->updateEastEdge();
			patchp->dirtyZ();
		}

		// Now do northeast/southwest connections
		for (i = 0; i < (S32)mPatchesPerEdge - 1; i++)
		{
			patchp = getPatch(mPatchesPerEdge - 1, i);
			neighbor_patchp = neighborp->getPatch(0, i+1);

			patchp->connectNeighbor(neighbor_patchp, NORTHEAST);
			neighbor_patchp->connectNeighbor(patchp, SOUTHWEST);
		}
		// Now do southeast/northwest connections
		for (i = 1; i < (S32)mPatchesPerEdge; i++)
		{
			patchp = getPatch(mPatchesPerEdge - 1, i);
			neighbor_patchp = neighborp->getPatch(0, i-1);

			patchp->connectNeighbor(neighbor_patchp, SOUTHEAST);
			neighbor_patchp->connectNeighbor(patchp, NORTHWEST);
		}
	}
	else if (NORTH == direction)
	{
		// Do north/south connections, first
		for (i = 0; i < (S32)mPatchesPerEdge; i++)
		{
			patchp = getPatch(i, mPatchesPerEdge - 1);
			neighbor_patchp = neighborp->getPatch(i, 0);

			patchp->connectNeighbor(neighbor_patchp, direction);
			neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);

			patchp->updateNorthEdge();
			patchp->dirtyZ();
		}

		// Do northeast/southwest connections
		for (i = 0; i < (S32)mPatchesPerEdge - 1; i++)
		{
			patchp = getPatch(i, mPatchesPerEdge - 1);
			neighbor_patchp = neighborp->getPatch(i+1, 0);

			patchp->connectNeighbor(neighbor_patchp, NORTHEAST);
			neighbor_patchp->connectNeighbor(patchp, SOUTHWEST);
		}
		// Do southeast/northwest connections
		for (i = 1; i < (S32)mPatchesPerEdge; i++)
		{
			patchp = getPatch(i, mPatchesPerEdge - 1);
			neighbor_patchp = neighborp->getPatch(i-1, 0);

			patchp->connectNeighbor(neighbor_patchp, NORTHWEST);
			neighbor_patchp->connectNeighbor(patchp, SOUTHEAST);
		}
	}
	else if (WEST == direction)
	{
		// Do east/west connections, first
		for (i = 0; i < mPatchesPerEdge; i++)
		{
			patchp = getPatch(0, i);
			neighbor_patchp = neighborp->getPatch(mPatchesPerEdge - 1, i);

			patchp->connectNeighbor(neighbor_patchp, direction);
			neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);

			neighbor_patchp->updateEastEdge();
			neighbor_patchp->dirtyZ();
		}

		// Now do northeast/southwest connections
		for (i = 1; i < mPatchesPerEdge; i++)
		{
			patchp = getPatch(0, i);
			neighbor_patchp = neighborp->getPatch(mPatchesPerEdge - 1, i - 1);

			patchp->connectNeighbor(neighbor_patchp, SOUTHWEST);
			neighbor_patchp->connectNeighbor(patchp, NORTHEAST);
		}

		// Now do northwest/southeast connections
		for (i = 0; i < mPatchesPerEdge - 1; i++)
		{
			patchp = getPatch(0, i);
			neighbor_patchp = neighborp->getPatch(mPatchesPerEdge - 1, i + 1);

			patchp->connectNeighbor(neighbor_patchp, NORTHWEST);
			neighbor_patchp->connectNeighbor(patchp, SOUTHEAST);
		}
	}
	else if (SOUTH == direction)
	{
		// Do north/south connections, first
		for (i = 0; i < mPatchesPerEdge; i++)
		{
			patchp = getPatch(i, 0);
			neighbor_patchp = neighborp->getPatch(i, mPatchesPerEdge - 1);

			patchp->connectNeighbor(neighbor_patchp, direction);
			neighbor_patchp->connectNeighbor(patchp, gDirOpposite[direction]);

			neighbor_patchp->updateNorthEdge();
			neighbor_patchp->dirtyZ();
		}

		// Now do northeast/southwest connections
		for (i = 1; i < mPatchesPerEdge; i++)
		{
			patchp = getPatch(i, 0);
			neighbor_patchp = neighborp->getPatch(i - 1, mPatchesPerEdge - 1);

			patchp->connectNeighbor(neighbor_patchp, SOUTHWEST);
			neighbor_patchp->connectNeighbor(patchp, NORTHEAST);
		}
		// Now do northeast/southwest connections
		for (i = 0; i < mPatchesPerEdge - 1; i++)
		{
			patchp = getPatch(i, 0);
			neighbor_patchp = neighborp->getPatch(i + 1, mPatchesPerEdge - 1);

			patchp->connectNeighbor(neighbor_patchp, SOUTHEAST);
			neighbor_patchp->connectNeighbor(patchp, NORTHWEST);
		}
	}		
}

void LLSurface::disconnectNeighbor(LLSurface *surfacep)
{
	S32 i;
	for (i = 0; i < 8; i++)
	{
		if (surfacep == mNeighbors[i])
		{
			mNeighbors[i] = NULL;
		}
	}

	// Iterate through surface patches, removing any connectivity to removed surface.
	for (i = 0; i < mNumberOfPatches; i++)
	{
		(mPatchList + i)->disconnectNeighbor(surfacep);
	}
}


void LLSurface::disconnectAllNeighbors()
{
	S32 i;
	for (i = 0; i < 8; i++)
	{
		if (mNeighbors[i])
		{
			mNeighbors[i]->disconnectNeighbor(this);
			mNeighbors[i] = NULL;
		}
	}
}



const LLVector3d &LLSurface::getOriginGlobal() const
{
	return mOriginGlobal;
}

LLVector3 LLSurface::getOriginAgent() const
{
	return gAgent.getPosAgentFromGlobal(mOriginGlobal);
}

F32 LLSurface::getMetersPerGrid() const
{
	return mMetersPerGrid;
}

S32 LLSurface::getGridsPerEdge() const
{
	return mGridsPerEdge;
}

S32 LLSurface::getPatchesPerEdge() const
{
	return mPatchesPerEdge;
}

S32 LLSurface::getGridsPerPatchEdge() const
{
	return mGridsPerPatchEdge;
}

void LLSurface::moveZ(const S32 x, const S32 y, const F32 delta)
{
	llassert(x >= 0);
	llassert(y >= 0);
	llassert(x < mGridsPerEdge);
	llassert(y < mGridsPerEdge);
	mSurfaceZ[x + y*mGridsPerEdge] += delta;
}


void LLSurface::updatePatchVisibilities(LLAgent &agent) 
{
	if (gShiftFrame)
	{
		return;
	}

	LLVector3 pos_region = mRegionp->getPosRegionFromGlobal(gAgentCamera.getCameraPositionGlobal());

	LLSurfacePatch *patchp;
	
	mVisiblePatchCount = 0;
	for (S32 i=0; i<mNumberOfPatches; i++) 
	{
		patchp = mPatchList + i;

		patchp->updateVisibility();
		if (patchp->getVisible())
		{
			mVisiblePatchCount++;
			patchp->updateCameraDistanceRegion(pos_region);
		}
	}
}

BOOL LLSurface::idleUpdate(F32 max_update_time)
{
	LLMemType mt_ius(LLMemType::MTYPE_IDLE_UPDATE_SURFACE);
	if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_TERRAIN))
	{
		return FALSE;
	}
	
	// Perform idle time update of non-critical stuff.
	// In this case, texture and normal updates.
	LLTimer update_timer;
	BOOL did_update = FALSE;

	// If the Z height data has changed, we need to rebuild our
	// property line vertex arrays.
	if (mDirtyPatchList.size() > 0)
	{
		getRegion()->dirtyHeights();
	}

	// Always call updateNormals() / updateVerticalStats()
	//  every frame to avoid artifacts
	for(std::set<LLSurfacePatch *>::iterator iter = mDirtyPatchList.begin();
		iter != mDirtyPatchList.end(); )
	{
		std::set<LLSurfacePatch *>::iterator curiter = iter++;
		LLSurfacePatch *patchp = *curiter;
		patchp->updateNormals();
		patchp->updateVerticalStats();
		if (max_update_time == 0.f || update_timer.getElapsedTimeF32() < max_update_time)
		{
			if (patchp->updateTexture())
			{
				did_update = TRUE;
				patchp->clearDirty();
				mDirtyPatchList.erase(curiter);
			}
		}
	}
	return did_update;
}

void LLSurface::decompressDCTPatch(LLBitPack &bitpack, LLGroupHeader *gopp, BOOL b_large_patch) 
{

	LLPatchHeader  ph;
	S32 j, i;
	S32 patch[LARGE_PATCH_SIZE*LARGE_PATCH_SIZE];
	LLSurfacePatch *patchp;

	init_patch_decompressor(gopp->patch_size);
	gopp->stride = mGridsPerEdge;
	set_group_of_patch_header(gopp);

	while (1)
	{
		decode_patch_header(bitpack, &ph);
		if (ph.quant_wbits == END_OF_PATCHES)
		{
			break;
		}

		i = ph.patchids >> 5;
		j = ph.patchids & 0x1F;

		if ((i >= mPatchesPerEdge) || (j >= mPatchesPerEdge))
		{
			llwarns << "Received invalid terrain packet - patch header patch ID incorrect!" 
				<< " patches per edge " << mPatchesPerEdge
				<< " i " << i
				<< " j " << j
				<< " dc_offset " << ph.dc_offset
				<< " range " << (S32)ph.range
				<< " quant_wbits " << (S32)ph.quant_wbits
				<< " patchids " << (S32)ph.patchids
				<< llendl;
            LLAppViewer::instance()->badNetworkHandler();
			return;
		}

		patchp = &mPatchList[j*mPatchesPerEdge + i];


		decode_patch(bitpack, patch);
		decompress_patch(patchp->getDataZ(), patch, &ph);

		// Update edges for neighbors.  Need to guarantee that this gets done before we generate vertical stats.
		patchp->updateNorthEdge();
		patchp->updateEastEdge();
		if (patchp->getNeighborPatch(WEST))
		{
			patchp->getNeighborPatch(WEST)->updateEastEdge();
		}
		if (patchp->getNeighborPatch(SOUTHWEST))
		{
			patchp->getNeighborPatch(SOUTHWEST)->updateEastEdge();
			patchp->getNeighborPatch(SOUTHWEST)->updateNorthEdge();
		}
		if (patchp->getNeighborPatch(SOUTH))
		{
			patchp->getNeighborPatch(SOUTH)->updateNorthEdge();
		}

		// Dirty patch statistics, and flag that the patch has data.
		patchp->dirtyZ();
		patchp->setHasReceivedData();
	}
}


// Retrurns TRUE if "position" is within the bounds of surface.
// "position" is region-local
BOOL LLSurface::containsPosition(const LLVector3 &position)
{
	if (position.mV[VX] < 0.0f  ||  position.mV[VX] > mMetersPerEdge ||
		position.mV[VY] < 0.0f  ||  position.mV[VY] > mMetersPerEdge)
	{
		return FALSE;
	}
	return TRUE;
}


F32 LLSurface::resolveHeightRegion(const F32 x, const F32 y) const
{
	F32 height = 0.0f;
	F32 oometerspergrid = 1.f/mMetersPerGrid;

	// Check to see if v is actually above surface 
	// We use (mGridsPerEdge-1) below rather than (mGridsPerEdge) 
	// becuase of the east and north buffers 

	if (x >= 0.f  &&  
		x <= mMetersPerEdge  &&
		y >= 0.f  &&  
		y <= mMetersPerEdge)
	{
		const S32 left   = llfloor(x * oometerspergrid);
		const S32 bottom = llfloor(y * oometerspergrid);

		// Don't walk off the edge of the array!
		const S32 right  = ( left+1   < (S32)mGridsPerEdge-1 ? left+1   : left );
		const S32 top    = ( bottom+1 < (S32)mGridsPerEdge-1 ? bottom+1 : bottom );

		// Figure out if v is in first or second triangle of the square
		// and calculate the slopes accordingly
		//    |       |
		// -(i,j+1)---(i+1,j+1)--   
		//    |  1   /  |          ^
		//    |    /  2 |          |
		//    |  /      |          j
		// --(i,j)----(i+1,j)--
		//    |       |
		// 
		//      i ->
		// where N = mGridsPerEdge

		const F32 left_bottom  = getZ( left,  bottom );
		const F32 right_bottom = getZ( right, bottom );
		const F32 left_top     = getZ( left,  top );
		const F32 right_top    = getZ( right, top );

		// dx and dy are incremental steps from (mSurface + k)
		F32 dx = x - left   * mMetersPerGrid;
		F32 dy = y - bottom * mMetersPerGrid;

		if (dy > dx) 
		{
			// triangle 1
			dy *= left_top  - left_bottom;
			dx *= right_top - left_top;
		}
		else 
		{
			// triangle 2
			dx *= right_bottom - left_bottom;
			dy *= right_top    - right_bottom;
		}
		height = left_bottom + (dx + dy) * oometerspergrid;
	}
	return height;
}


F32 LLSurface::resolveHeightGlobal(const LLVector3d& v) const
{
	if (!mRegionp)
	{
		return 0.f;
	}
	
	LLVector3 pos_region = mRegionp->getPosRegionFromGlobal(v);

	return resolveHeightRegion(pos_region);
}


LLVector3 LLSurface::resolveNormalGlobal(const LLVector3d& pos_global) const
{
	if (!mSurfaceZ)
	{
		// Hmm.  Uninitialized surface!
		return LLVector3::z_axis;
	}
	//
	// Returns the vector normal to a surface at location specified by vector v
	//
	F32 oometerspergrid = 1.f/mMetersPerGrid;
	LLVector3 normal;
	F32 dzx, dzy;

	if (pos_global.mdV[VX] >= mOriginGlobal.mdV[VX]  &&  
		pos_global.mdV[VX] < mOriginGlobal.mdV[VX] + mMetersPerEdge  &&
		pos_global.mdV[VY] >= mOriginGlobal.mdV[VY]  &&  
		pos_global.mdV[VY] < mOriginGlobal.mdV[VY] + mMetersPerEdge)
	{
		U32 i, j, k;
		F32 dx, dy;
		i = (U32) ((pos_global.mdV[VX] - mOriginGlobal.mdV[VX]) * oometerspergrid);
		j = (U32) ((pos_global.mdV[VY] - mOriginGlobal.mdV[VY]) * oometerspergrid );
		k = i + j*mGridsPerEdge;

		// Figure out if v is in first or second triangle of the square
		// and calculate the slopes accordingly
		//    |       |
		// -(k+N)---(k+1+N)--   
		//    |  1 /  |          ^
		//    |   / 2 |          |
		//    |  /    |          j
		// --(k)----(k+1)--
		//    |       |
		// 
		//      i ->
		// where N = mGridsPerEdge

		// dx and dy are incremental steps from (mSurface + k)
		dx = (F32)(pos_global.mdV[VX] - i*mMetersPerGrid - mOriginGlobal.mdV[VX]);
		dy = (F32)(pos_global.mdV[VY] - j*mMetersPerGrid - mOriginGlobal.mdV[VY]);
		if (dy > dx) 
		{  // triangle 1
			dzx = *(mSurfaceZ + k + 1 + mGridsPerEdge) - *(mSurfaceZ + k + mGridsPerEdge);
			dzy = *(mSurfaceZ + k) - *(mSurfaceZ + k + mGridsPerEdge);
			normal.setVec(-dzx,dzy,1);
		}
		else 
		{	// triangle 2
			dzx = *(mSurfaceZ + k) - *(mSurfaceZ + k + 1);
			dzy = *(mSurfaceZ + k + 1 + mGridsPerEdge) - *(mSurfaceZ + k + 1);
			normal.setVec(dzx,-dzy,1);
		}
	}
	normal.normVec();
	return normal;


}

LLSurfacePatch *LLSurface::resolvePatchRegion(const F32 x, const F32 y) const
{
// x and y should be region-local coordinates. 
// If x and y are outside of the surface, then the returned
// index will be for the nearest boundary patch.
//
// 12      | 13| 14|       15
//         |   |   |    
//     +---+---+---+---+
//     | 12| 13| 14| 15|
// ----+---+---+---+---+-----
// 8   | 8 | 9 | 10| 11|   11
// ----+---+---+---+---+-----
// 4   | 4 | 5 | 6 | 7 |    7
// ----+---+---+---+---+-----
//     | 0 | 1 | 2 | 3 |
//     +---+---+---+---+
//         |   |   |    
// 0       | 1 | 2 |        3
//

// When x and y are not region-local do the following first

	S32 i, j;
	if (x < 0.0f)
	{
		i = 0;
	}
	else if (x >= mMetersPerEdge)
	{
		i = mPatchesPerEdge - 1;
	}
	else
	{
		i = (U32) (x / (mMetersPerGrid * mGridsPerPatchEdge));
	}

	if (y < 0.0f)
	{
		j = 0;
	}
	else if (y >= mMetersPerEdge)
	{
		j = mPatchesPerEdge - 1;
	}
	else
	{
		j = (U32) (y / (mMetersPerGrid * mGridsPerPatchEdge));
	}

	// *NOTE: Super paranoia code follows.
	S32 index = i + j * mPatchesPerEdge;
	if((index < 0) || (index >= mNumberOfPatches))
	{
		if(0 == mNumberOfPatches)
		{
			llwarns << "No patches for current region!" << llendl;
			return NULL;
		}
		S32 old_index = index;
		index = llclamp(old_index, 0, (mNumberOfPatches - 1));
		llwarns << "Clamping out of range patch index " << old_index
				<< " to " << index << llendl;
	}
	return &(mPatchList[index]);
}


LLSurfacePatch *LLSurface::resolvePatchRegion(const LLVector3 &pos_region) const
{
	return resolvePatchRegion(pos_region.mV[VX], pos_region.mV[VY]);
}


LLSurfacePatch *LLSurface::resolvePatchGlobal(const LLVector3d &pos_global) const
{
	llassert(mRegionp);
	LLVector3 pos_region = mRegionp->getPosRegionFromGlobal(pos_global);
	return resolvePatchRegion(pos_region);
}


std::ostream& operator<<(std::ostream &s, const LLSurface &S) 
{
	s << "{ \n";
	s << "  mGridsPerEdge = " << S.mGridsPerEdge - 1 << " + 1\n";
	s << "  mGridsPerPatchEdge = " << S.mGridsPerPatchEdge << "\n";
	s << "  mPatchesPerEdge = " << S.mPatchesPerEdge << "\n";
	s << "  mOriginGlobal = " << S.mOriginGlobal << "\n";
	s << "  mMetersPerGrid = " << S.mMetersPerGrid << "\n";
	s << "  mVisiblePatchCount = " << S.mVisiblePatchCount << "\n";
	s << "}";
	return s;
}


// ---------------- LLSurface:: Protected ----------------

void LLSurface::createPatchData() 
{
	// Assumes mGridsPerEdge, mGridsPerPatchEdge, and mPatchesPerEdge have been properly set
	// TODO -- check for create() called when surface is not empty
	S32 i, j;
	LLSurfacePatch *patchp;

	// Allocate memory
	mPatchList = new LLSurfacePatch[mNumberOfPatches];

	// One of each for each camera
	mVisiblePatchCount = mNumberOfPatches;

	for (j=0; j<mPatchesPerEdge; j++) 
	{
		for (i=0; i<mPatchesPerEdge; i++) 
		{
			patchp = getPatch(i, j);
			patchp->setSurface(this);
		}
	}

	for (j=0; j<mPatchesPerEdge; j++) 
	{
		for (i=0; i<mPatchesPerEdge; i++) 
		{
			patchp = getPatch(i, j);
			patchp->mHasReceivedData = FALSE;
			patchp->mSTexUpdate = TRUE;

			S32 data_offset = i * mGridsPerPatchEdge + j * mGridsPerPatchEdge * mGridsPerEdge;

			patchp->setDataZ(mSurfaceZ + data_offset);
			patchp->setDataNorm(mNorm + data_offset);


			// We make each patch point to its neighbors so we can do resolution checking 
			// when butting up different resolutions.  Patches that don't have neighbors
			// somewhere will point to NULL on that side.
			if (i < mPatchesPerEdge-1)  
			{
				patchp->setNeighborPatch(EAST,getPatch(i+1, j));
			}
			else 
			{
				patchp->setNeighborPatch(EAST, NULL);
			}

			if (j < mPatchesPerEdge-1)  
			{
				patchp->setNeighborPatch(NORTH, getPatch(i, j+1));
			}
			else 
			{
				patchp->setNeighborPatch(NORTH, NULL);
			}

			if (i > 0) 
			{
				patchp->setNeighborPatch(WEST, getPatch(i - 1, j));
			}
			else 
			{
				patchp->setNeighborPatch(WEST, NULL);
			}

			if (j > 0)  
			{
				patchp->setNeighborPatch(SOUTH, getPatch(i, j-1));
			}
			else 
			{
				patchp->setNeighborPatch(SOUTH, NULL);
			}

			if (i < (mPatchesPerEdge-1)  &&  j < (mPatchesPerEdge-1)) 
			{
				patchp->setNeighborPatch(NORTHEAST, getPatch(i + 1, j + 1));
			}
			else 
			{
				patchp->setNeighborPatch(NORTHEAST, NULL);		
			}

			if (i > 0  &&  j < (mPatchesPerEdge-1)) 
			{
				patchp->setNeighborPatch(NORTHWEST, getPatch(i - 1, j + 1));
			}
			else 
			{
				patchp->setNeighborPatch(NORTHWEST, NULL);
			}

			if (i > 0  &&  j > 0) 
			{
				patchp->setNeighborPatch(SOUTHWEST, getPatch(i - 1, j - 1));
			}
			else 
			{
				patchp->setNeighborPatch(SOUTHWEST, NULL);
			}

			if (i < (mPatchesPerEdge-1)  &&  j > 0) 
			{
				patchp->setNeighborPatch(SOUTHEAST, getPatch(i + 1, j - 1));
			}
			else 
			{
				patchp->setNeighborPatch(SOUTHEAST, NULL);
			}

			LLVector3d origin_global;
			origin_global.mdV[0] = mOriginGlobal.mdV[0] + i * mMetersPerGrid * mGridsPerPatchEdge;
			origin_global.mdV[1] = mOriginGlobal.mdV[0] + j * mMetersPerGrid * mGridsPerPatchEdge;
			origin_global.mdV[2] = 0.f;
			patchp->setOriginGlobal(origin_global);
		}
	}
}


void LLSurface::destroyPatchData()
{
	// Delete all of the cached patch data for these patches.

	delete [] mPatchList;
	mPatchList = NULL;
	mVisiblePatchCount = 0;
}


void LLSurface::setTextureSize(const S32 texture_size)
{
	sTextureSize = texture_size;
}


U32 LLSurface::getRenderLevel(const U32 render_stride) const
{
	return mPVArray.mRenderLevelp[render_stride];
}


U32 LLSurface::getRenderStride(const U32 render_level) const
{
	return mPVArray.mRenderStridep[render_level];
}


LLSurfacePatch *LLSurface::getPatch(const S32 x, const S32 y) const
{
	if ((x < 0) || (x >= mPatchesPerEdge))
	{
		llerrs << "Asking for patch out of bounds" << llendl;
		return NULL;
	}
	if ((y < 0) || (y >= mPatchesPerEdge))
	{
		llerrs << "Asking for patch out of bounds" << llendl;
		return NULL;
	}

	return mPatchList + x + y*mPatchesPerEdge;
}


void LLSurface::dirtyAllPatches()
{
	S32 i;
	for (i = 0; i < mNumberOfPatches; i++)
	{
		mPatchList[i].dirtyZ();
	}
}

void LLSurface::dirtySurfacePatch(LLSurfacePatch *patchp)
{
	// Put surface patch on dirty surface patch list
	mDirtyPatchList.insert(patchp);
}


void LLSurface::setWaterHeight(F32 height)
{
	if (!mWaterObjp.isNull())
	{
		LLVector3 water_pos_region = mWaterObjp->getPositionRegion();
		bool changed = water_pos_region.mV[VZ] != height;
		water_pos_region.mV[VZ] = height;
		mWaterObjp->setPositionRegion(water_pos_region);
		if (changed)
		{
			LLWorld::getInstance()->updateWaterObjects();
		}
	}
	else
	{
		llwarns << "LLSurface::setWaterHeight with no water object!" << llendl;
	}
}

F32 LLSurface::getWaterHeight() const
{
	if (!mWaterObjp.isNull())
	{
		// we have a water object, the usual case
		return mWaterObjp->getPositionRegion().mV[VZ];
	}
	else
	{
		return DEFAULT_WATER_HEIGHT;
	}
}


BOOL LLSurface::generateWaterTexture(const F32 x, const F32 y,
									 const F32 width, const F32 height)
{
	if (!getWaterTexture())
	{
		return FALSE;
	}

	S32 tex_width = mWaterTexturep->getWidth();
	S32 tex_height = mWaterTexturep->getHeight();
	S32 tex_comps = mWaterTexturep->getComponents();
	S32 tex_stride = tex_width * tex_comps;
	LLPointer<LLImageRaw> raw = new LLImageRaw(tex_width, tex_height, tex_comps);
	U8 *rawp = raw->getData();

	F32 scale = 256.f * getMetersPerGrid() / (F32)tex_width;
	F32 scale_inv = 1.f / scale;

	S32 x_begin, y_begin, x_end, y_end;

	x_begin = llround(x * scale_inv);
	y_begin = llround(y * scale_inv);
	x_end = llround((x + width) * scale_inv);
	y_end = llround((y + width) * scale_inv);

	if (x_end > tex_width)
	{
		x_end = tex_width;
	}
	if (y_end > tex_width)
	{
		y_end = tex_width;
	}

	// OK, for now, just have the composition value equal the height at the point.
	LLVector3 location;
	LLColor4U coloru;

	const F32 WATER_HEIGHT = getWaterHeight();

	S32 i, j, offset;
	for (j = y_begin; j < y_end; j++)
	{
		for (i = x_begin; i < x_end; i++)
		{
			//F32 nv[2];
			//nv[0] = i/256.f;
			//nv[1] = j/256.f;
			// const S32 modulation = noise2(nv)*40;
			offset = j*tex_stride + i*tex_comps;
			location.mV[VX] = i*scale;
			location.mV[VY] = j*scale;

			// Sample multiple points
			const F32 height = resolveHeightRegion(location);

			if (height > WATER_HEIGHT)
			{
				// Above water...
				coloru = MAX_WATER_COLOR;
				coloru.mV[3] = ABOVE_WATERLINE_ALPHA;
				*(rawp + offset++) = coloru.mV[0];
				*(rawp + offset++) = coloru.mV[1];
				*(rawp + offset++) = coloru.mV[2];
				*(rawp + offset++) = coloru.mV[3];
			}
			else
			{
				// Want non-linear curve for transparency gradient
				coloru = MAX_WATER_COLOR;
				const F32 frac = 1.f - 2.f/(2.f - (height - WATER_HEIGHT));
				S32 alpha = 64 + llround((255-64)*frac);

				alpha = llmin(llround((F32)MAX_WATER_COLOR.mV[3]), alpha);
				alpha = llmax(64, alpha);

				coloru.mV[3] = alpha;
				*(rawp + offset++) = coloru.mV[0];
				*(rawp + offset++) = coloru.mV[1];
				*(rawp + offset++) = coloru.mV[2];
				*(rawp + offset++) = coloru.mV[3];
			}
		}
	}

	if (!mWaterTexturep->hasGLTexture())
	{
		mWaterTexturep->createGLTexture(0, raw);
	}

	mWaterTexturep->setSubImage(raw, x_begin, y_begin, x_end - x_begin, y_end - y_begin);
	return TRUE;
}
