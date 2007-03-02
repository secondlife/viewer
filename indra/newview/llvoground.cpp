/** 
 * @file llvoground.cpp
 * @brief LLVOGround class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvoground.h"
#include "lldrawpoolground.h"

#include "llviewercontrol.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "pipeline.h"

LLVOGround::LLVOGround(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLStaticViewerObject(id, pcode, regionp)
{
	mbCanSelect = FALSE;
}


LLVOGround::~LLVOGround()
{
}

BOOL LLVOGround::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
 	if (mDead || !(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_GROUND)))
	{
		return TRUE;
	}
	
	/*if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}*/
	return TRUE;
}


void LLVOGround::updateTextures(LLAgent &agent)
{
}


LLDrawable *LLVOGround::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);

	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_GROUND);
	LLDrawPoolGround *poolp = (LLDrawPoolGround*) gPipeline.getPool(LLDrawPool::POOL_GROUND);

	mDrawable->addFace(poolp, NULL);

	return mDrawable;
}

BOOL LLVOGround::updateGeometry(LLDrawable *drawable)
{
	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U32> indicesp;
	S32 index_offset;
	LLFace *face;	

	LLDrawPoolGround *poolp = (LLDrawPoolGround*) gPipeline.getPool(LLDrawPool::POOL_GROUND);

	if (drawable->getNumFaces() < 1)
		drawable->addFace(poolp, NULL);
	face = drawable->getFace(0); 
		
	if (face->mVertexBuffer.isNull())
	{
		face->setSize(5, 12);
		face->mVertexBuffer = new LLVertexBuffer(LLDrawPoolGround::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
		face->mVertexBuffer->allocateBuffer(face->getGeomCount(), face->getIndicesCount(), TRUE);
		face->setGeomIndex(0);
		face->setIndicesIndex(0);
	}
	
	index_offset = face->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
	if (-1 == index_offset)
	{
		return TRUE;
	}

	///////////////////////////////////////
	//
	//
	//
	LLVector3 at_dir = gCamera->getAtAxis();
	at_dir.mV[VZ] = 0.f;
	if (at_dir.normVec() < 0.01)
	{
		// We really don't care, as we're not looking anywhere near the horizon.
	}
	LLVector3 left_dir = gCamera->getLeftAxis();
	left_dir.mV[VZ] = 0.f;
	left_dir.normVec();

	// Our center top point
	LLColor4 ground_color = gSky.getFogColor();
	ground_color.mV[3] = 1.f;
	face->setFaceColor(ground_color);
	
	*(verticesp++)  = LLVector3(64, 64, 0);
	*(verticesp++)  = LLVector3(-64, 64, 0);
	*(verticesp++)  = LLVector3(-64, -64, 0);
	*(verticesp++)  = LLVector3(64, -64, 0);
	*(verticesp++)  = LLVector3(0, 0, -1024);
	
	
	// Triangles for each side
	*indicesp++ = index_offset + 0;
	*indicesp++ = index_offset + 1;
	*indicesp++ = index_offset + 4;

	*indicesp++ = index_offset + 1;
	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 4;

	*indicesp++ = index_offset + 2;
	*indicesp++ = index_offset + 3;
	*indicesp++ = index_offset + 4;

	*indicesp++ = index_offset + 3;
	*indicesp++ = index_offset + 0;
	*indicesp++ = index_offset + 4;

	*(texCoordsp++) = LLVector2(0.f, 0.f);
	*(texCoordsp++) = LLVector2(1.f, 0.f);
	*(texCoordsp++) = LLVector2(1.f, 1.f);
	*(texCoordsp++) = LLVector2(0.f, 1.f);
	*(texCoordsp++) = LLVector2(0.5f, 0.5f);
	
	LLPipeline::sCompiles++;
	return TRUE;
}
