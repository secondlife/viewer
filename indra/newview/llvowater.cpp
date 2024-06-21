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

#include "llviewercontrol.h"

#include "lldrawable.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llsky.h"
#include "llsurface.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "pipeline.h"
#include "llspatialpartition.h"

///////////////////////////////////

template<class T> inline T LERP(T a, T b, F32 factor)
{
    return a + (b - a) * factor;
}

LLVOWater::LLVOWater(const LLUUID &id,
                     const LLPCode pcode,
                     LLViewerRegion *regionp) :
    LLStaticViewerObject(id, pcode, regionp),
    mRenderType(LLPipeline::RENDER_TYPE_WATER)
{
    // Terrain must draw during selection passes so it can block objects behind it.
    mbCanSelect = false;
    setScale(LLVector3(256.f, 256.f, 0.f)); // Hack for setting scale for bounding boxes/visibility.

    mUseTexture = true;
    mIsEdgePatch = false;
}


void LLVOWater::markDead()
{
    LLViewerObject::markDead();
}


bool LLVOWater::isActive() const
{
    return false;
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
void  LLVOWater::idleUpdate(LLAgent &agent, const F64 &time)
{
}

LLDrawable *LLVOWater::createDrawable(LLPipeline *pipeline)
{
    pipeline->allocDrawable(this);
    mDrawable->setLit(false);
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

bool LLVOWater::updateGeometry(LLDrawable *drawable)
{
    LL_PROFILE_ZONE_SCOPED;
    LLFace *face;

    if (drawable->getNumFaces() < 1)
    {
        LLDrawPoolWater *poolp = (LLDrawPoolWater*) gPipeline.getPool(LLDrawPool::POOL_WATER);
        drawable->addFace(poolp, NULL);
    }
    face = drawable->getFace(0);
    if (!face)
    {
        return true;
    }

//  LLVector2 uvs[4];
//  LLVector3 vtx[4];

    LLStrider<LLVector3> verticesp, normalsp;
    LLStrider<LLVector2> texCoordsp;
    LLStrider<U16> indicesp;
    U16 index_offset;


    // A quad is 4 vertices and 6 indices (making 2 triangles)
    static const unsigned int vertices_per_quad = 4;
    static const unsigned int indices_per_quad = 6;

    S32 size_x = LLPipeline::sRenderTransparentWater ? 8 : 1;
    S32 size_y = LLPipeline::sRenderTransparentWater ? 8 : 1;

    const LLVector3& scale = getScale();
    size_x *= llmin(llround(scale.mV[0] / 256.f), 8);
    size_y *= llmin(llround(scale.mV[1] / 256.f), 8);

    const S32 num_quads = size_x * size_y;
    face->setSize(vertices_per_quad * num_quads,
                  indices_per_quad * num_quads);

    LLVertexBuffer* buff = face->getVertexBuffer();
    if (!buff ||
        buff->getNumIndices() != face->getIndicesCount() ||
        buff->getNumVerts() != face->getGeomCount() ||
        face->getIndicesStart() != 0 ||
        face->getGeomIndex() != 0)
    {
        buff = new LLVertexBuffer(LLDrawPoolWater::VERTEX_DATA_MASK);
        if (!buff->allocateBuffer(face->getGeomCount(), face->getIndicesCount()))
        {
            LL_WARNS() << "Failed to allocate Vertex Buffer on water update to "
                << face->getGeomCount() << " vertices and "
                << face->getIndicesCount() << " indices" << LL_ENDL;
        }
        face->setIndicesIndex(0);
        face->setGeomIndex(0);
        face->setVertexBuffer(buff);
    }

    index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);

    LLVector3 position_agent;
    position_agent = getPositionAgent();
    face->mCenterAgent = position_agent;
    face->mCenterLocal = position_agent;

    S32 x, y;
    F32 step_x = getScale().mV[0] / size_x;
    F32 step_y = getScale().mV[1] / size_y;

    const LLVector3 up(0.f, step_y * 0.5f, 0.f);
    const LLVector3 right(step_x * 0.5f, 0.f, 0.f);
    const LLVector3 normal(0.f, 0.f, 1.f);

    F32 size_inv_x = 1.f / size_x;
    F32 size_inv_y = 1.f / size_y;

    for (y = 0; y < size_y; y++)
    {
        for (x = 0; x < size_x; x++)
        {
            S32 toffset = index_offset + 4*(y*size_x + x);
            position_agent = getPositionAgent() - getScale() * 0.5f;
            position_agent.mV[VX] += (x + 0.5f) * step_x;
            position_agent.mV[VY] += (y + 0.5f) * step_y;

            position_agent.mV[VX] = llround(position_agent.mV[VX]);
            position_agent.mV[VY] = llround(position_agent.mV[VY]);

            *verticesp++  = position_agent - right + up;
            *verticesp++  = position_agent - right - up;
            *verticesp++  = position_agent + right + up;
            *verticesp++  = position_agent + right - up;

            *texCoordsp++ = LLVector2(x*size_inv_x, (y+1)*size_inv_y);
            *texCoordsp++ = LLVector2(x*size_inv_x, y*size_inv_y);
            *texCoordsp++ = LLVector2((x+1)*size_inv_x, (y+1)*size_inv_y);
            *texCoordsp++ = LLVector2((x+1)*size_inv_x, y*size_inv_y);

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

    buff->unmapBuffer();

    mDrawable->movePartition();
    LLPipeline::sCompiles++;
    return true;
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

void LLVOWater::setUseTexture(const bool use_texture)
{
    mUseTexture = use_texture;
}

void LLVOWater::setIsEdgePatch(const bool edge_patch)
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

LLWaterPartition::LLWaterPartition(LLViewerRegion* regionp)
: LLSpatialPartition(0, false, regionp)
{
    mInfiniteFarClip = true;
    mDrawableType = LLPipeline::RENDER_TYPE_WATER;
    mPartitionType = LLViewerRegion::PARTITION_WATER;
}

LLVoidWaterPartition::LLVoidWaterPartition(LLViewerRegion* regionp) : LLWaterPartition(regionp)
{
    mOcclusionEnabled = false;
    mDrawableType = LLPipeline::RENDER_TYPE_VOIDWATER;
    mPartitionType = LLViewerRegion::PARTITION_VOIDWATER;
}
