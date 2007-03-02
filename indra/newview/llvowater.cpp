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

LLVOWater::LLVOWater(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLStaticViewerObject(id, LL_VO_WATER, regionp)
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
	LLFastTimer ftm(LLFastTimer::FTM_UPDATE_WATER);
	LLFace *face;

	if (drawable->getNumFaces() < 1)
	{
		LLDrawPoolWater *poolp = (LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER);
		drawable->addFace(poolp, NULL);
	}
	face = drawable->getFace(0);

	LLVector2 uvs[4];
	LLVector3 vtx[4];

	LLStrider<LLVector3> verticesp, normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U32> indicesp;
	S32 index_offset;

	S32 size = 16;

	if (face->mVertexBuffer.isNull())
	{
		S32 num_quads = size*size;	
		face->setSize(4*num_quads, 6*num_quads);
		
		face->mVertexBuffer = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK, GL_DYNAMIC_DRAW_ARB);
		face->mVertexBuffer->allocateBuffer(4*num_quads, 6*num_quads, TRUE);
		face->setIndicesIndex(0);
		face->setGeomIndex(0);
	}
	else
	{
		face->mVertexBuffer->resizeBuffer(face->getGeomCount(), face->getIndicesCount());
	}
		
	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return TRUE;
	}
	
	LLVector3 position_agent;
	position_agent = getPositionAgent();
	face->mCenterAgent = position_agent;
	face->mCenterLocal = position_agent;

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

void LLVOWater::initClass()
{
}

void LLVOWater::cleanupClass()
{
}

void setVecZ(LLVector3& v)
{
	v.mV[VX] = 0;
	v.mV[VY] = 0;
	v.mV[VZ] = 1;
}

void LLVOWater::setUseTexture(const BOOL use_texture)
{
	mUseTexture = use_texture;
}

void LLVOWater::updateSpatialExtents(LLVector3 &newMin, LLVector3& newMax)
{
	LLVector3 pos = getPositionAgent();
	LLVector3 scale = getScale();

	newMin = pos - scale * 0.5f;
	newMax = pos + scale * 0.5f;

	mDrawable->setPositionGroup((newMin + newMax) * 0.5f);
}

U32 LLVOWater::getPartitionType() const
{ 
	return LLPipeline::PARTITION_WATER; 
}

LLWaterPartition::LLWaterPartition()
: LLSpatialPartition(0)
{
	mRenderByGroup = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_WATER;
	mPartitionType = LLPipeline::PARTITION_WATER;
}
