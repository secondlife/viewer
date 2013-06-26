/** 
 * @file llvoground.cpp
 * @brief LLVOGround class implementation
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

#include "llvoground.h"
#include "lldrawpoolground.h"

#include "llviewercontrol.h"

#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "pipeline.h"

LLVOGround::LLVOGround(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLStaticViewerObject(id, pcode, regionp, TRUE)
{
	mbCanSelect = FALSE;
}


LLVOGround::~LLVOGround()
{
}

void LLVOGround::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
}


void LLVOGround::updateTextures()
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

// TO DO - this always returns TRUE, 
BOOL LLVOGround::updateGeometry(LLDrawable *drawable)
{
	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	LLStrider<U16> indicesp;
	S32 index_offset;
	LLFace *face;	

	LLDrawPoolGround *poolp = (LLDrawPoolGround*) gPipeline.getPool(LLDrawPool::POOL_GROUND);

	if (drawable->getNumFaces() < 1)
		drawable->addFace(poolp, NULL);
	face = drawable->getFace(0); 
	if (!face)
		return TRUE;
		
	if (!face->getVertexBuffer())
	{
		face->setSize(5, 12);
		LLVertexBuffer* buff = new LLVertexBuffer(LLDrawPoolGround::VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
		buff->allocateBuffer(face->getGeomCount(), face->getIndicesCount(), TRUE);
		face->setGeomIndex(0);
		face->setIndicesIndex(0);
		face->setVertexBuffer(buff);
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
	LLVector3 at_dir = LLViewerCamera::getInstance()->getAtAxis();
	at_dir.mV[VZ] = 0.f;
	if (at_dir.normVec() < 0.01)
	{
		// We really don't care, as we're not looking anywhere near the horizon.
	}
	LLVector3 left_dir = LLViewerCamera::getInstance()->getLeftAxis();
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
	
	face->getVertexBuffer()->flush();
	LLPipeline::sCompiles++;
	return TRUE;
}
