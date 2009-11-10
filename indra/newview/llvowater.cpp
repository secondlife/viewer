/** 
 * @file llvowater.cpp
 * @brief LLVOWater class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llvowater.h"

#include "imageids.h"
#include "llviewercontrol.h"

#include "lldrawable.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llsky.h"
#include "llsurface.h"
#include "llvosky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"

const BOOL gUseRoam = FALSE;


///////////////////////////////////

template<class T> inline T LERP(T a, T b, F32 factor)
{
	return a + (b - a) * factor;
}

const U32 N_RES_HALF	= (N_RES >> 1);

const U32 WIDTH			= (N_RES * WAVE_STEP); //128.f //64		// width of wave tile, in meters
const F32 WAVE_STEP_INV	= (1. / WAVE_STEP);


LLVOWater::LLVOWater(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLStaticViewerObject(id, LL_VO_WATER, regionp)
{
	// Terrain must draw during selection passes so it can block objects behind it.
	mbCanSelect = FALSE;
	setScale(LLVector3(256.f, 256.f, 0.f)); // Hack for setting scale for bounding boxes/visibility.

	mUseTexture = TRUE;
	mIsEdgePatch = FALSE;
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
void LLVOWater::updateTextures()
{
}

// Never gets called
BOOL LLVOWater::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
 	/*if (mDead || !(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_WATER)))
	{
		return TRUE;
	}
	if (mDrawable) 
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}*/
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
		mDrawable->setNumFaces(1, pool, LLWorld::getInstance()->getDefaultWaterTexture());
	}

	return mDrawable;
}

static LLFastTimer::DeclareTimer FTM_UPDATE_WATER("Update Water");

BOOL LLVOWater::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(FTM_UPDATE_WATER);
	LLFace *face;

	if (drawable->getNumFaces() < 1)
	{
		LLDrawPoolWater *poolp = (LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER);
		drawable->addFace(poolp, NULL);
	}
	face = drawable->getFace(0);

//	LLVector2 uvs[4];
//	LLVector3 vtx[4];

	LLStrider<LLVector3> verticesp, normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U16> indicesp;
	U16 index_offset;

	S32 size = 16;

	S32 num_quads = size*size;	
	face->setSize(4*num_quads, 6*num_quads);

	if (face->mVertexBuffer.isNull())
	{
		face->mVertexBuffer = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK, GL_DYNAMIC_DRAW_ARB);
		face->mVertexBuffer->allocateBuffer(face->getGeomCount(), face->getIndicesCount(), TRUE);
		face->setIndicesIndex(0);
		face->setGeomIndex(0);
	}
	else
	{
		face->mVertexBuffer->resizeBuffer(face->getGeomCount(), face->getIndicesCount());
	}
		
	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
		
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

			*verticesp++  = position_agent - right + up;
			*verticesp++  = position_agent - right - up;
			*verticesp++  = position_agent + right + up;
			*verticesp++  = position_agent + right - up;

			*texCoordsp++ = LLVector2(x*size_inv, (y+1)*size_inv);
			*texCoordsp++ = LLVector2(x*size_inv, y*size_inv);
			*texCoordsp++ = LLVector2((x+1)*size_inv, (y+1)*size_inv);
			*texCoordsp++ = LLVector2((x+1)*size_inv, y*size_inv);
			
			*normalsp++   = normal;
			*normalsp++   = normal;
			*normalsp++   = normal;
			*normalsp++   = normal;

			*indicesp++ = toffset + 0;
			*indicesp++ = toffset + 1;
			*indicesp++ = toffset + 2;

			*indicesp++ = toffset + 1;
			*indicesp++ = toffset + 3;
			*indicesp++ = toffset + 2;
		}
	}
	
	face->mVertexBuffer->setBuffer(0);

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

void LLVOWater::setIsEdgePatch(const BOOL edge_patch)
{
	mIsEdgePatch = edge_patch;
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
	return LLViewerRegion::PARTITION_WATER; 
}

LLWaterPartition::LLWaterPartition()
: LLSpatialPartition(0, FALSE, 0)
{
	mInfiniteFarClip = TRUE;
	mDrawableType = LLPipeline::RENDER_TYPE_WATER;
	mPartitionType = LLViewerRegion::PARTITION_WATER;
}
