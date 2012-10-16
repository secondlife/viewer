/** 
 * @file llvowater.cpp
 * @brief LLVOWater class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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


LLVOWater::LLVOWater(const LLUUID &id, 
					 const LLPCode pcode, 
					 LLViewerRegion *regionp) :
	LLStaticViewerObject(id, pcode, regionp),
	mRenderType(LLPipeline::RENDER_TYPE_WATER)
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
void  LLVOWater::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
}

LLDrawable *LLVOWater::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	mDrawable->setRenderType(mRenderType);

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
	if (!face)
	{
		return TRUE;
	}

//	LLVector2 uvs[4];
//	LLVector3 vtx[4];

	LLStrider<LLVector3> verticesp, normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U16> indicesp;
	U16 index_offset;


	// A quad is 4 vertices and 6 indices (making 2 triangles)
	static const unsigned int vertices_per_quad = 4;
	static const unsigned int indices_per_quad = 6;

	const S32 size = gSavedSettings.getBOOL("RenderTransparentWater") && LLGLSLShader::sNoFixedFunction ? 16 : 1;

	const S32 num_quads = size * size;
	face->setSize(vertices_per_quad * num_quads,
				  indices_per_quad * num_quads);
	
	LLVertexBuffer* buff = face->getVertexBuffer();
	if (!buff || !buff->isWriteable())
	{
		buff = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK, GL_DYNAMIC_DRAW_ARB);
		buff->allocateBuffer(face->getGeomCount(), face->getIndicesCount(), TRUE);
		face->setIndicesIndex(0);
		face->setGeomIndex(0);
		face->setVertexBuffer(buff);
	}
	else
	{
		buff->resizeBuffer(face->getGeomCount(), face->getIndicesCount());
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

	F32 z_fudge = 0.f;

	if (getIsEdgePatch())
	{ //bump edge patches down 10 cm to prevent aliasing along edges
		z_fudge = -0.1f;
	}

	for (y = 0; y < size; y++)
	{
		for (x = 0; x < size; x++)
		{
			S32 toffset = index_offset + 4*(y*size + x);
			position_agent = getPositionAgent() - getScale() * 0.5f;
			position_agent.mV[VX] += (x + 0.5f) * step_x;
			position_agent.mV[VY] += (y + 0.5f) * step_y;
			position_agent.mV[VZ] += z_fudge;

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
	
	buff->flush();

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

void LLVOWater::updateSpatialExtents(LLVector4a &newMin, LLVector4a& newMax)
{
	LLVector4a pos;
	pos.load3(getPositionAgent().mV);
	LLVector4a scale;
	scale.load3(getScale().mV);
	scale.mul(0.5f);

	newMin.setSub(pos, scale);
	newMax.setAdd(pos, scale);
	
	pos.setAdd(newMin,newMax);
	pos.mul(0.5f);

	mDrawable->setPositionGroup(pos);
}

U32 LLVOWater::getPartitionType() const
{ 
	if (mIsEdgePatch)
	{
		return LLViewerRegion::PARTITION_VOIDWATER;
	}

	return LLViewerRegion::PARTITION_WATER; 
}

U32 LLVOVoidWater::getPartitionType() const
{
	return LLViewerRegion::PARTITION_VOIDWATER;
}

LLWaterPartition::LLWaterPartition()
: LLSpatialPartition(0, FALSE, GL_DYNAMIC_DRAW_ARB)
{
	mInfiniteFarClip = TRUE;
	mDrawableType = LLPipeline::RENDER_TYPE_WATER;
	mPartitionType = LLViewerRegion::PARTITION_WATER;
}

LLVoidWaterPartition::LLVoidWaterPartition()
{
	mOcclusionEnabled = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_VOIDWATER;
	mPartitionType = LLViewerRegion::PARTITION_VOIDWATER;
}
