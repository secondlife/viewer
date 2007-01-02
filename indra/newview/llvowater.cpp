/** 
 * @file llvowater.cpp
 * @brief LLVOWater class implementation
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvowater.h"

#include "imageids.h"
#include "llviewercontrol.h"

#include "llagent.h"
#include "lldrawable.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llsky.h"
#include "llsurface.h"
#include "llvosky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "pipeline.h"

const BOOL gUseRoam = FALSE;


///////////////////////////////////

#include "randgauss.h"

template<class T> inline T LERP(T a, T b, F32 factor)
{
	return a + (b - a) * factor;
}

const U32 N_RES_HALF	= (N_RES >> 1);

const U32 WIDTH			= (N_RES * WAVE_STEP); //128.f //64		// width of wave tile, in meters
const F32 WAVE_STEP_INV	= (1. / WAVE_STEP);

const F32 g				= 9.81f;          // gravitational constant (m/s^2)

///////////////////////////////////

LLWaterSurface::LLWaterSurface() :
	mInitialized(FALSE),
	mWind(9, 0, 0),
	mA(0.2f),
	mVisc(0.001f),
	mShininess(8.0f)
{}


LLWaterGrid *LLVOWater::sGrid = 0;


LLVOWater::LLVOWater(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLViewerObject(id, LL_VO_WATER, regionp)
{
	// Terrain must draw during selection passes so it can block objects behind it.
	mbCanSelect = FALSE;
	setScale(LLVector3(256.f, 256.f, 0.f)); // Hack for setting scale for bounding boxes/visibility.

	mUseTexture = TRUE;
}


void LLVOWater::markDead()
{
	LLViewerObject::markDead();
}


BOOL LLVOWater::isActive() const
{
	return FALSE;
}


void LLVOWater::setPixelAreaAndAngle(LLAgent &agent)
{
	mAppAngle = 50;
	mPixelArea = 500*500;
}


// virtual
void LLVOWater::updateTextures(LLAgent &agent)
{
}

// virtual
void LLVOWater::updateDrawable(BOOL force_damped)
{
	// Force an immediate rebuild on any update
	if (mDrawable.notNull())
	{
		gPipeline.updateMoveNormalAsync(mDrawable);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	clearChanged(SHIFTED);
}

// Never gets called
BOOL LLVOWater::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
 	if (mDead || !(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_WATER)))
	{
		return TRUE;
	}
	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}
	return TRUE;
}

LLDrawable *LLVOWater::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_WATER);

	LLDrawPoolWater *pool = (LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER);

	if (mUseTexture)
	{
		mDrawable->setNumFaces(1, pool, mRegionp->getLand().getWaterTexture());
	}
	else
	{
		mDrawable->setNumFaces(1, pool, gWorldp->getDefaultWaterTexture());
	}

	return mDrawable;
}

BOOL LLVOWater::updateGeometry(LLDrawable *drawable)
{
	return updateGeometryFlat(drawable);
}


BOOL LLVOWater::updateGeometryFlat(LLDrawable *drawable)
{
	LLFace *face;

	if (drawable->getNumFaces() < 1)
	{
		drawable->addFace(gPipeline.getPool(LLDrawPool::POOL_WATER), NULL);
	}
	face = drawable->getFace(0);

	LLVector2 uvs[4];
	LLVector3 vtx[4];

	LLStrider<LLVector3> verticesp, normalsp;
	LLStrider<LLVector2> texCoordsp;
	U32 *indicesp;
	S32 index_offset;

	S32 size = 16;

	S32 num_quads = size*size;

	face->setPrimType(LLTriangles);
	face->setSize(4*num_quads, 6*num_quads);
	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return TRUE;
	}
	
	LLVector3 position_agent;
	position_agent = getPositionAgent();
	face->mCenterAgent = position_agent;

	S32 x, y;
	F32 step_x = getScale().mV[0] / size;
	F32 step_y = getScale().mV[1] / size;

	const LLVector3 up(0.f, step_y * 0.5f, 0.f);
	const LLVector3 right(step_x * 0.5f, 0.f, 0.f);
	const LLVector3 normal(0.f, 0.f, 1.f);

	F32 size_inv = 1.f / size;

	for (y = 0; y < size; y++)
	{
		for (x = 0; x < size; x++)
		{
			S32 toffset = index_offset + 4*(y*size + x);
			position_agent = getPositionAgent() - getScale() * 0.5f;
			position_agent.mV[VX] += (x + 0.5f) * step_x;
			position_agent.mV[VY] += (y + 0.5f) * step_y;

			vtx[0] = position_agent - right + up;
			vtx[1] = position_agent - right - up;
			vtx[2] = position_agent + right + up;
			vtx[3] = position_agent + right - up;

			*(verticesp++)  = vtx[0];
			*(verticesp++)  = vtx[1];
			*(verticesp++)  = vtx[2];
			*(verticesp++)  = vtx[3];

			uvs[0].setVec(x*size_inv, (y+1)*size_inv);
			uvs[1].setVec(x*size_inv, y*size_inv);
			uvs[2].setVec((x+1)*size_inv, (y+1)*size_inv);
			uvs[3].setVec((x+1)*size_inv, y*size_inv);

			*(texCoordsp) = uvs[0];
			texCoordsp++;
			*(texCoordsp) = uvs[1];
			texCoordsp++;
			*(texCoordsp) = uvs[2];
			texCoordsp++;
			*(texCoordsp) = uvs[3];
			texCoordsp++;

			*(normalsp++)   = normal;
			*(normalsp++)   = normal;
			*(normalsp++)   = normal;
			*(normalsp++)   = normal;

			*indicesp++ = toffset + 0;
			*indicesp++ = toffset + 1;
			*indicesp++ = toffset + 2;

			*indicesp++ = toffset + 1;
			*indicesp++ = toffset + 3;
			*indicesp++ = toffset + 2;
		}
	}


	mDrawable->movePartition();
	LLPipeline::sCompiles++;
	return TRUE;
}


BOOL LLVOWater::updateGeometryHeightFieldRoam(LLDrawable *drawable)
{
	LLVector3 position_agent = getPositionAgent();
	const LLVector3 region_size = getScale();
	const LLVector3 region_origin = position_agent - region_size * 0.5f;

	S32 patch_origx = llround(region_origin.mV[VX] - sGrid->mRegionOrigin.mV[VX]) / sGrid->mRegionWidth;
	S32 patch_origy = llround(region_origin.mV[VY] - sGrid->mRegionOrigin.mV[VY]) / sGrid->mRegionWidth;
	S32 patch_dimx = llround(region_size.mV[VX]) / sGrid->mRegionWidth;
	S32 patch_dimy = llround(region_size.mV[VY]) / sGrid->mRegionWidth;

	static S32 res = (S32)sGrid->mPatchRes;
	if (patch_origx < 0)
	{
		patch_dimx -= - patch_origx;
		if (patch_dimx < 1)
		{
			return TRUE;
		}
		patch_origx = 0;
	}
	if (patch_origy < 0)
	{
		patch_dimy -= - patch_origy;
		if (patch_dimy < 1)
		{
			return TRUE;
		}
		patch_origy = 0;
	}
	if (patch_origx >= res)
	{
		return TRUE;
	}
	if (patch_origy >= res)
	{
		return TRUE;
	}

	patch_dimx = llmin<U32>(patch_dimx, res - patch_origx);
	patch_dimy = llmin<U32>(patch_dimy, res - patch_origy);

	U32 num_of_tris = 0;
	S32 px, py;
	for (py = patch_origy; py < patch_origy + patch_dimy; py++)
	{
		for (px = patch_origx; px < patch_origx + patch_dimx; px++)
		{
			const U32 ind = py * sGrid->mPatchRes + px;
			if (sGrid->mPatches[ind].visible() && sGrid->mTab[px][py] == 0)
			{
				num_of_tris += sGrid->mPatches[ind].numTris();
				sGrid->mTab[px][py] = this;
			}
		}
	}

	if (num_of_tris == 0)
	{
		return TRUE;
	}

	if (drawable->getNumFaces() < 1)
	{
		drawable->addFace((LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER),
								gWorldp->getDefaultWaterTexture());
	}

	LLFace *face;

	face = drawable->getFace(0);
	face->mCenterAgent = position_agent;

	LLStrider<LLVector3> verticesp, normalsp;
	LLStrider<LLVector2> texCoordsp;
	U32 *indicesp;
	S32 index_offset;

	const F32 water_height = getRegion()->getWaterHeight();

	
	face->setPrimType(LLTriangles);
	face->setSize(3 * num_of_tris, 3 * num_of_tris);
	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return TRUE;
	}

	U32 num_of_vtx = 0;

	for (py = patch_origy; py < patch_origy + patch_dimy; py++)
	{
		for (px = patch_origx; px < patch_origx + patch_dimx; px++)
		{
			for (U8 h = 0; h < 2; h++)
			{
				const U32 ind = py * sGrid->mPatchRes + px;
				if (!sGrid->mPatches[ind].visible() || sGrid->mTab[px][py] != this)
					continue;
				LLWaterTri* half = (LLWaterTri*) sGrid->mPatches[ind].half(h);
				for (const LLWaterTri*  tri = (LLWaterTri*) half->getFirstLeaf();
										tri != NULL;
										tri = (LLWaterTri*) tri->getNextLeaf())
				{
					/////// check for coordinates
					*(verticesp++)  = sGrid->vtx(tri->Lvtx(), water_height);
					*(verticesp++)  = sGrid->vtx(tri->Rvtx(), water_height);
					*(verticesp++)  = sGrid->vtx(tri->Tvtx(), water_height);
					
					*(normalsp++)   = sGrid->norm(tri->Lvtx());
					*(normalsp++)   = sGrid->norm(tri->Rvtx());
					*(normalsp++)   = sGrid->norm(tri->Tvtx());
					
					*(indicesp++) = index_offset + num_of_vtx + 0;
					*(indicesp++) = index_offset + num_of_vtx + 1;
					*(indicesp++) = index_offset + num_of_vtx + 2;
					num_of_vtx += 3;
				}
			}
		}
	}


	LLPipeline::sCompiles++;
	return TRUE;
}



BOOL LLVOWater::updateGeometryHeightFieldSimple(LLDrawable *drawable)
{
	LLVector3 position_agent = getPositionAgent();
	const LLVector3 region_size = getScale();
	const LLVector3 region_origin = position_agent - region_size * 0.5f;

	S32 patch_origx = llround(region_origin.mV[VX] - sGrid->mRegionOrigin.mV[VX]) / sGrid->mRegionWidth;
	S32 patch_origy = llround(region_origin.mV[VY] - sGrid->mRegionOrigin.mV[VY]) / sGrid->mRegionWidth;
	S32 patch_dimx = llround(region_size.mV[VX]) / sGrid->mRegionWidth;
	S32 patch_dimy = llround(region_size.mV[VY]) / sGrid->mRegionWidth;

	static S32 res = sGrid->mPatchRes;
	if (patch_origx < 0)
	{
		patch_dimx -= - patch_origx;
		if (patch_dimx < 1)
		{
			return TRUE;
		}
		patch_origx = 0;
	}
	if (patch_origy < 0)
	{
		patch_dimy -= - patch_origy;
		if (patch_dimy < 1)
		{
			return TRUE;
		}
		patch_origy = 0;
	}
	if (patch_origx >= res)
	{
		return TRUE;
	}
	if (patch_origy >= res)
	{
		return TRUE;
	}

	patch_dimx = llmin<U32>(patch_dimx, res - patch_origx);
	patch_dimy = llmin<U32>(patch_dimy, res - patch_origy);


	U32 num_of_regions = 0;
	S32 px, py;

	for (py = patch_origy; py < patch_origy + patch_dimy; py++)
	{
		for (px = patch_origx; px < patch_origx + patch_dimx; px++)
		{
		//	if (sGrid->mTab[px][py] != 0)
		//		bool stop = true;
			if (sGrid->mPatches[py * sGrid->mPatchRes + px].visible() && sGrid->mTab[px][py] == 0)
			{
				num_of_regions++;
				sGrid->mTab[px][py] = this;
			}
		}
	}

	if (num_of_regions == 0)
	{
		return TRUE;
	}

	if (drawable->getNumFaces() < 1)
	{
		drawable->addFace((LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER),
								gWorldp->getDefaultWaterTexture());
	}

	LLFace *face;

	face = drawable->getFace(0);
	face->mCenterAgent = position_agent;

	LLStrider<LLVector3> verticesp, normalsp;
	LLStrider<LLVector2> texCoordsp;
	U32 *indicesp;
	S32 index_offset;

	const F32 water_height = getRegion()->getWaterHeight();

	const U32 steps_in_region = sGrid->mStepsInRegion / sGrid->mResDecrease;
	const U32 num_quads = steps_in_region * steps_in_region * num_of_regions;

	face->setPrimType(LLTriangles);
	face->setSize(4*num_quads, 6*num_quads);
	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return TRUE;
	}

	U32 num_of_vtx = 0;

	for (py = patch_origy; py < patch_origy + patch_dimy; py++)
	{
		for (px = patch_origx; px < patch_origx + patch_dimx; px++)
		{
			if (!sGrid->mPatches[py * sGrid->mPatchRes + px].visible() || sGrid->mTab[px][py] != this)
			{
				continue;
			}

			U32 orig_indx = px * sGrid->mStepsInRegion;
			U32 orig_indy = py * sGrid->mStepsInRegion;

			for (U32 qy = 0; qy < steps_in_region; qy++)
			{
				for (U32 qx = 0; qx < steps_in_region; qx++)
				{
					const S32 x0 = orig_indx + qx * sGrid->mResDecrease;
					const S32 y0 = orig_indy + qy * sGrid->mResDecrease;
					const S32 x1 = x0 + sGrid->mResDecrease;
					const S32 y1 = y0 + sGrid->mResDecrease;
					
					sGrid->setVertex(x0, y1, water_height, *(verticesp));
					verticesp++;
					sGrid->setVertex(x0, y0, water_height, *(verticesp));
					verticesp++;
					sGrid->setVertex(x1, y1, water_height, *(verticesp));
					verticesp++;
					sGrid->setVertex(x1, y0, water_height, *(verticesp));
					verticesp++;
					/*
					*(verticesp++)  = sGrid->vtx(x0, y1, water_height);
					*(verticesp++)  = sGrid->vtx(x0, y0, water_height);
					*(verticesp++)  = sGrid->vtx(x1, y1, water_height);
					*(verticesp++)  = sGrid->vtx(x1, y0, water_height);
					*/
					*(normalsp++)   = sGrid->norm(x0, y1);
					*(normalsp++)   = sGrid->norm(x0, y0);
					*(normalsp++)   = sGrid->norm(x1, y1);
					*(normalsp++)   = sGrid->norm(x1, y0);
					
					const S32 curr_index_offset = index_offset + num_of_vtx;

					*indicesp++ = curr_index_offset  + 0;
					*indicesp++ = curr_index_offset  + 1;
					*indicesp++ = curr_index_offset  + 2;
					
					*indicesp++ = curr_index_offset  + 1;
					*indicesp++ = curr_index_offset  + 3;
					*indicesp++ = curr_index_offset  + 2;
					num_of_vtx += 4;
				}
			}
		}
	}


	LLPipeline::sCompiles++;
	return TRUE;
}

void LLVOWater::initClass()
{
	sGrid = new LLWaterGrid;
}

void LLVOWater::cleanupClass()
{
	if (sGrid)
	{
		sGrid->cleanup();
		delete sGrid;
		sGrid = 0;
	}
}


LLWaterGrid::LLWaterGrid() : mResIncrease(1)//0.5)
{
	init();
}


void LLWaterGrid::init()
{
	//mRegionOrigin = LLVector3(-2 * mRegionWidth, -2 * mRegionWidth, 0);
	mRegionWidth = 256;
	mPatchRes = 5;
	mMaxGridSize = mPatchRes * mRegionWidth;
	mMinStep = (U32)(WAVE_STEP * mResIncrease);

	LLWaterTri::sMinStep = mMinStep;
	LLWaterTri::sQueues = &mRoam;

	setGridDim(mMaxGridSize / mMinStep);

	mVtx = new LLVector3[mGridDim1 * mGridDim1];
	mNorms = new LLVector3[mGridDim1 * mGridDim1];

	mPatches = new LLWaterPatch[mPatchRes * mPatchRes];

	mStepsInRegion = mRegionWidth / mMinStep;
	const U32 max_div_level = 2 * (U32)(log((F32)mStepsInRegion) / log(2.0f));

	for (U32 y = 0; y < mPatchRes; y++)
	{
		for (U32 x = 0; x < mPatchRes; x++)
		{
			LLVector3 patch_center(mRegionWidth * (x + 0.5f), mRegionWidth * (y + 0.5f), 0);

			mPatches[y * mPatchRes + x].set(x * mStepsInRegion, y * mStepsInRegion,
											mStepsInRegion, mRegionWidth, patch_center, max_div_level);
			if (x > 0)
			{
				mPatches[y * mPatchRes + x].left()->setRight(mPatches[y * mPatchRes + x - 1].right());
				mPatches[y * mPatchRes + x - 1].right()->setRight(mPatches[y * mPatchRes + x].left());
			}
			if (y > 0)
			{
				mPatches[y * mPatchRes + x].left()->setLeft(mPatches[(y - 1) * mPatchRes + x].right());
				mPatches[(y - 1) * mPatchRes + x].right()->setLeft(mPatches[y * mPatchRes + x].left());
			}
		}
	}
}

void LLWaterGrid::cleanup()
{
	delete[] mVtx;
	mVtx = NULL;

	delete[] mNorms;
	mNorms = NULL;

	delete[] mPatches;
	mPatches = NULL;
}


void setVecZ(LLVector3& v)
{
	v.mV[VX] = 0;
	v.mV[VY] = 0;
	v.mV[VZ] = 1;
}

void LLWaterGrid::update()
{
	static LLViewerRegion* prev_region = gAgent.getRegion();
	LLViewerRegion* region = gAgent.getRegion();

	mRegionOrigin = region->getOriginAgent();
	mRegionOrigin.mV[VX] -= 2 * mRegionWidth;
	mRegionOrigin.mV[VY] -= 2 * mRegionWidth;
	mRegionOrigin.mV[VZ] = 0;

	const F32 clip_far = gCamera->getFar() - 31;
	const F32 clip_far2 = clip_far * clip_far;

	const LLVector3 camera_pos = gAgent.getCameraPositionAgent();
	const LLVector3 look_at = gCamera->getAtAxis();


	if (camera_pos.mV[VZ] > 200)
	{
		mResDecrease = 4;
	}
	else if (camera_pos.mV[VZ] > 100)
	{
		mResDecrease = 2;
	}
	else
	{
		mResDecrease = 1;
	}


	//U32 mResDecrease = res_decrease;
	U32 res_decrease = 1;

	const F32 res_change = mResIncrease;// * res_decrease ;

	F32 height;

	// Set the grid

	//U32 fractions = 1;
	U32 fractions_res = res_decrease;
	if (res_change < 1)
	{
		//fractions = llround(1. / res_change);
		fractions_res = llround(1.f / mResIncrease);
	}


	//const U32 fractions_res = fractions * res_decrease;

	LLVector3 cur_pos;
	U32 x, y;
	U32 ind = 0;
	for (y = 0; y < mGridDim1; y += fractions_res)
	{
		const F32 dispy = (F32)(y * mMinStep);//step;
		for (x = 0; x < mGridDim1; x += fractions_res)
		{
			const F32 dispx = (F32)(x * mMinStep);//step;
			cur_pos = mRegionOrigin;
			cur_pos.mV[VX] += dispx;
			cur_pos.mV[VY] += dispy;

			const F32 x_dist = cur_pos.mV[VX] - camera_pos.mV[VX];
			const F32 y_dist = cur_pos.mV[VY] - camera_pos.mV[VY];

			if (x_dist * look_at.mV[VX] + y_dist * look_at.mV[VY] < 0)
			{
				mVtx[ind] = cur_pos;
				setVecZ(mNorms[ind]);
				ind++;
				continue;
			}
				
			const F32 dist_to_vtx2 = x_dist * x_dist + y_dist * y_dist;
			if (dist_to_vtx2 > .81 * clip_far2)
			{
				mVtx[ind] = cur_pos;
				setVecZ(mNorms[ind]);
				ind++;
				continue;
			}

			mWater.getIntegerHeightAndNormal(llround(WAVE_STEP_INV * dispx),
					llround(WAVE_STEP_INV * dispy),	height, mNorms[ind]);

			cur_pos.mV[VZ] += height;
			mVtx[ind] = cur_pos;
			ind++;
		}
	}

	if (res_change < 1)
	{
		U32 fractions = llround(1.f / mResIncrease);
		for (y = 0; y < mGridDim1; y += fractions_res)
		{
			for (x = 0; x < mGridDim; x += fractions_res)
			{
				const U32 ind00 = index(x, y);
				const U32 ind01 = ind00 + fractions_res;
				for (U32 frx = 1; frx < fractions; frx += res_decrease)
				{
					const U32 ind = ind00 + frx;
					mNorms[ind] = LERP(mNorms[ind00], mNorms[ind01], frx * res_change);
					mVtx[ind]   = LERP(  mVtx[ind00],   mVtx[ind01], frx * res_change);
				}
			}
		}
		for (x = 0; x < mGridDim1; x += res_decrease)
		{
			for (y = 0; y < mGridDim; y += fractions_res)
			{
				const U32 ind00 = index(x, y);
				const U32 ind10 = ind00 + fractions_res * mGridDim1;//(y + fractions) * quad_resx1 + x;
				for (U32 fry = 1; fry < fractions; fry += res_decrease)
				{
					const U32 ind = ind00 + fry * mGridDim1;//(y + fry) * quad_resx1 + x;
					mNorms[ind] = LERP(mNorms[ind00], mNorms[ind10], fry * res_change);
					mVtx[ind]   = LERP(  mVtx[ind00],   mVtx[ind10], fry * res_change);
				}
			}
		}
	}

	if (gUseRoam)
	{
		updateTree(camera_pos, look_at, clip_far, prev_region != region);
	}
	else
	{
		updateVisibility(camera_pos, look_at, clip_far);
	}

	prev_region = region;


	//mTab[0][0] = 0;
	for (y = 0; y < mPatchRes; y++)
	{
		for (x = 0; x < mPatchRes; x++)
			mTab[x][y] = 0;
	}

}

void LLWaterGrid::updateTree(const LLVector3 &camera_pos, const LLVector3 &look_at, F32 clip_far,
							 BOOL restart = FALSE)
{
	static S8 recalculate_frame = 0;

	if (restart)
	{
		recalculate_frame = 0;
	}

	if (recalculate_frame == 0)
	{
		LLWaterTri::nextRound();
		setCamPosition(LLWaterTri::sCam, camera_pos);
		LLWaterTri::sClipFar = clip_far;


		const U32 step = (U32)(WAVE_STEP * mResIncrease * mResDecrease);
		const U32 steps_in_region = mRegionWidth / step;
		LLWaterTri::sMaxDivLevel = 2 * llround(log((F32)steps_in_region) / log(2.0f));

		for (U32 y = 0; y < mPatchRes; y++)
		{
			for (U32 x = 0; x < mPatchRes; x++)
			{
				U32 patch_ind = y * mPatchRes + x;				
				mPatches[patch_ind].updateTree(camera_pos, look_at, mRegionOrigin);
			}
		}

		mRoam.process();

		// debug
		/*
		for (y = 0; y < mPatchRes; y++)
		{
			for (U32 x = 0; x < mPatchRes; x++)
			{
				//mPatches[y * mPatchRes + x].checkUpToDate();
				//mPatches[y * mPatchRes + x].checkConsistensy();
				mPatches[y * mPatchRes + x].checkCount();
			}
		}
		*/
	}
	++recalculate_frame;
	recalculate_frame = recalculate_frame % 2;
}

void LLWaterGrid::updateVisibility(const LLVector3 &camera_pos, const LLVector3 &look_at, F32 clip_far)
{
	for (U32 y = 0; y < mPatchRes; y++)
	{
		for (U32 x = 0; x < mPatchRes; x++)
		{
			mPatches[y * mPatchRes + x].updateVisibility(camera_pos, look_at, mRegionOrigin);
		}
	}
}


void LLVOWater::setUseTexture(const BOOL use_texture)
{
	mUseTexture = use_texture;
}

F32 LLWaterSurface::agentDepth() const
{
	const LLViewerRegion* region = gAgent.getRegion();
	LLVector3 position_agent = region->getOriginAgent();// getPositionAgent();
	const LLVector3 region_origin = position_agent;
	const LLVector3 camera_pos = gAgent.getCameraPositionAgent();

	F32 height;
	LLVector3 normal;

	getHeightAndNormal(WAVE_STEP_INV * camera_pos.mV[VX], 
						WAVE_STEP_INV * camera_pos.mV[VY], height, normal);
	F32 agent_water_height = gAgent.getRegion()->getWaterHeight();
	return camera_pos.mV[VZ] - (agent_water_height + height);
}

////////////////////////////////////////////////


void LLWaterSurface::getHeightAndNormal(F32 i, F32 j, F32& wave_height, LLVector3& normal) const
{
	S32 i_ind = llfloor(i);
	S32 j_ind = llfloor(j);
	F32 i_fr = i - i_ind;
	F32 j_fr = j - j_ind;

	i_ind = i_ind % N_RES;
	j_ind = j_ind % N_RES;

	S32 i_ind_next = i_ind + 1;
	S32 j_ind_next = j_ind + 1;
	if (i_ind_next == (S32)N_RES)	i_ind_next = 0;
	if (j_ind_next == (S32)N_RES)	j_ind_next = 0;

	const F32 i_fr1 = 1 - i_fr;
	const F32 j_fr1 = 1 - j_fr;

	const F32 hi0	= i_fr1 * height(i_ind, j_ind)		+	i_fr * height(i_ind_next, j_ind);
	const F32 hi1	= i_fr1 * height(i_ind, j_ind_next)	+	i_fr * height(i_ind_next, j_ind_next);
	wave_height		= j_fr1 * hi0 + j_fr * hi1;

	normal			= i_fr1 * mNorms[i_ind][j_ind];
	normal			+=				i_fr * mNorms[i_ind_next][j_ind];
	LLVector3 vi1	= i_fr1 * mNorms[i_ind][j_ind_next];
	vi1				+=				i_fr * mNorms[i_ind_next][j_ind_next];
	normal			*= j_fr1;
	normal			+=				j_fr * vi1;

	//normal.normVec();
}

void LLWaterSurface::getIntegerHeightAndNormal(S32 i, S32 j, F32& wave_height, LLVector3& normal) const
{
	S32 i_ind = i % N_RES;
	S32 j_ind = j % N_RES;

	wave_height		= height(i_ind, j_ind);
	normal			= mNorms[i_ind][j_ind];
}

F32 LLWaterSurface::phillips(const LLVector2& k, const LLVector2& wind_n, F32 L, F32 L_small)
{
	F32 k2 = k * k;
	F32 k_dot_wind = k * wind_n;
	F32 spectrum = mA * (F32) exp(-1 / (L * L * k2)) / (k2 * k2) * (k_dot_wind * k_dot_wind / k2);

	if (k_dot_wind < 0) spectrum *= .25f;  // scale down waves that move opposite to the wind

	F32 damp = (F32) exp(- k2 * L_small * L_small);

	return (spectrum * damp);
}



void LLWaterSurface::initAmplitudes()
{
	U16 i, j;
	LLVector2 k;
	F32 sqrtPhillips;

	const LLVector2 wind(mWind.mV);

	LLVector2 wind_n = wind;
	const F32 wind_vel = wind_n.normVec();

	const F32 L = wind_vel * wind_vel / g;	// largest wave arising from constant wind of speed wind_vel

	const F32 L_small = L / 70;		// eliminate waves with very small length (L_small << L)


	for (i = 0; i <= N_RES; i++)
	{
		k.mV[VX] = (- (S32)N_RES_HALF + i) * (F_TWO_PI / WIDTH);
		for (j = 0; j <= N_RES; j++)
		{
			k.mV[VY] = (- (S32)N_RES_HALF + j) * (F_TWO_PI / WIDTH);

			const F32 k_mag = k.magVec();
			mOmega[i][j] = (F32) sqrt(g * k_mag);

			if (k_mag < F_APPROXIMATELY_ZERO)
				sqrtPhillips = 0;
			else
				sqrtPhillips = (F32) sqrt(phillips(k, wind_n, L, L_small));

			//const F32 r1 = rand() / (F32) RAND_MAX;
			//const F32 r2 = rand() / (F32) RAND_MAX;
			const F32 r1 = randGauss(0, 1);
			const F32 r2 = randGauss(0, 1);

			mHtilda0[i][j].re = sqrtPhillips * r1 * OO_SQRT2;
			mHtilda0[i][j].im = sqrtPhillips * r2 * OO_SQRT2;

		}
	}

	mPlan.init(N_RES, N_RES);
	mInitialized = 1;  // initialization complete
}

void LLWaterSurface::generateWaterHeightField(F64 t)
{
	S32 i, j;
	S32 mi, mj;                // -K indices
	COMPLEX plus, minus;
	
	if (!mInitialized) initAmplitudes();
	
	for (i = 0; i < (S32)N_RES_HALF; i++)
	{
		mi = N_RES - i;
		for (j = 0; j < (S32)N_RES ; j++)
		{
			mj = N_RES - j;

			const F32 cos_wt =  cosf(mOmega[i][j] * t); // = cos(-mOmega[i][j] * t)
			const F32 sin_wt =  sinf(mOmega[i][j] * t); // = -sin(-mOmega[i][j] * t)
			plus.re  =  mHtilda0[i][j].re   * cos_wt - mHtilda0[i][j].im   * sin_wt;
			plus.im  =  mHtilda0[i][j].re   * sin_wt + mHtilda0[i][j].im   * cos_wt;
			minus.re =  mHtilda0[mi][mj].re * cos_wt - mHtilda0[mi][mj].im * sin_wt;
			minus.im = -mHtilda0[mi][mj].re * sin_wt - mHtilda0[mi][mj].im * cos_wt;

			// now sum the plus and minus waves to get the total wave amplitude h
			mHtilda[i * N_RES + j].re = plus.re + minus.re;
			mHtilda[i * N_RES + j].im = plus.im + minus.im;
			if (mi < (S32)N_RES && mj < (S32)N_RES)
			{
				mHtilda[mi * N_RES + mj].re = plus.re + minus.re;
				mHtilda[mi * N_RES + mj].im = -plus.im - minus.im;
			}
		}
	}

	inverse_fft(mPlan, mHtilda, N_RES, N_RES);

	calcNormals();
}


/*
 * Computer normals by taking finite differences.  
 */

void LLWaterSurface::calcNormals()
{
	LLVector3 n;
	
	for (U32 i = 0; i < N_RES; i++)
	{
		for (U32 j = 0; j < N_RES; j++)
		{
			F32 px = heightWrapped(i + 1, j);
			F32 mx = heightWrapped(i - 1, j);
			F32 py = heightWrapped(i, j + 1);
			F32 my = heightWrapped(i, j - 1);
			F32 pxpy = heightWrapped(i + 1, j + 1);
			F32 pxmy = heightWrapped(i + 1, j - 1);
			F32 mxpy = heightWrapped(i - 1, j + 1); 
			F32 mxmy = heightWrapped(i - 1, j - 1);

			n.mV[VX] = -((2 * px + pxpy + pxmy) - (2 * mx + mxpy + mxmy));
			n.mV[VY] = -((2 * py + pxpy + mxpy) - (2 * my + pxmy + mxmy));
			n.mV[VZ] = 8 * WIDTH / (F32) N_RES;
			n.normVec();
			
			mNorms[i][j] = n;
		}
	}
}

void LLVOWater::generateNewWaves(F64 time)
{
	getWaterSurface()->generateWaterHeightField(time);
	sGrid->update();
}

