/** 
 * @file llvoclouds.cpp
 * @brief Implementation of LLVOClouds class which is a derivation fo LLViewerObject
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvoclouds.h"

#include "llviewercontrol.h"

#include "llagent.h"		// to get camera position
#include "lldrawable.h"
#include "llface.h"
#include "llprimitive.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvosky.h"
#include "llworld.h"
#include "pipeline.h"
#include "viewer.h"

LLVOClouds::LLVOClouds(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLViewerObject(id, LL_VO_CLOUDS, regionp)
{
	mCloudGroupp = NULL;
	mbCanSelect = FALSE;
	setNumTEs(1);
	LLViewerImage* image = gImageList.getImage(gCloudTextureID);
	image->setBoostLevel(LLViewerImage::BOOST_CLOUDS);
	setTEImage(0, image);
}


LLVOClouds::~LLVOClouds()
{
}


BOOL LLVOClouds::isActive() const
{
	return TRUE;
}


BOOL LLVOClouds::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS)))
		return TRUE;
	
	// Set dirty flag (so renderer will rebuild primitive)
	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}
	return TRUE;
}


void LLVOClouds::setPixelAreaAndAngle(LLAgent &agent)
{
	mAppAngle = 50;
	mPixelArea = 1500*100;
}

void LLVOClouds::updateTextures(LLAgent &agent)
{
	getTEImage(0)->addTextureStats(mPixelArea);
}

LLDrawable* LLVOClouds::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_CLOUDS);

	LLDrawPool *pool = gPipeline.getPool(LLDrawPool::POOL_CLOUDS);

	mDrawable->setNumFaces(1, pool, getTEImage(0));

	return mDrawable;
}

BOOL LLVOClouds::updateGeometry(LLDrawable *drawable)
{
 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS)))
		return TRUE;
	
	LLVector3 at;
	LLVector3 up;
	LLVector3 right;
	LLVector3 normal(0.f,0.f,-1.f);
	LLVector3 position_agent;
	//LLVector3 v[4];
	LLFace *facep;
	const LLVector3 region_pos_agent = mRegionp->getOriginAgent();
	const LLVector3 camera_agent = gAgent.getCameraPositionAgent();
	LLVector3 center_offset = getPositionRegion();
	LLVector2 uvs[4];

	uvs[0].setVec(0.f, 1.f);
	uvs[1].setVec(0.f, 0.f);
	uvs[2].setVec(1.f, 1.f);
	uvs[3].setVec(1.f, 0.f);

	LLVector3 vtx[4];

	S32 num_faces = mCloudGroupp->getNumPuffs();

	drawable->setNumFacesFast(num_faces, gPipeline.getPool(LLDrawPool::POOL_CLOUDS), getTEImage(0));

	S32 face_indx = 0;
	for ( ;	face_indx < num_faces; face_indx++)
	{
		facep = drawable->getFace(face_indx);

		LLStrider<LLVector3> verticesp, normalsp;
		LLStrider<LLVector2> texCoordsp;
		U32 *indicesp;
		S32 index_offset;

		facep->setPrimType(LLTriangles);
		facep->setSize(4, 6);
		index_offset = facep->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
		if (-1 == index_offset)
		{
			return TRUE;
		}

		const LLCloudPuff &puff = mCloudGroupp->getPuff(face_indx);
		const LLVector3 puff_pos_agent = gAgent.getPosAgentFromGlobal(puff.getPositionGlobal());
		facep->mCenterAgent = puff_pos_agent;

		LLVector3 from_camera_vec = gCamera->getAtAxis();//puff_pos_agent - camera_agent;
		at = from_camera_vec;
		right = at % LLVector3(0.f, 0.f, 1.f);
		right.normVec();
		up = right % at;
		up.normVec();
		right *= 0.5f*CLOUD_PUFF_WIDTH;
		up *= 0.5f*CLOUD_PUFF_HEIGHT;;

		facep->mCenterAgent = puff_pos_agent;

		LLColor4 color(1.f, 1.f, 1.f, puff.getAlpha());
		facep->setFaceColor(color);
		
		vtx[0] = puff_pos_agent - right + up;
		vtx[1] = puff_pos_agent - right - up;
		vtx[2] = puff_pos_agent + right + up;
		vtx[3] = puff_pos_agent + right - up;

		*verticesp++  = vtx[0];
		*verticesp++  = vtx[1];
		*verticesp++  = vtx[2];
		*verticesp++  = vtx[3];

		*texCoordsp++ = uvs[0];
		*texCoordsp++ = uvs[1];
		*texCoordsp++ = uvs[2];
		*texCoordsp++ = uvs[3];

		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;

		*indicesp++ = index_offset + 0;
		*indicesp++ = index_offset + 1;
		*indicesp++ = index_offset + 2;

		*indicesp++ = index_offset + 1;
		*indicesp++ = index_offset + 3;
		*indicesp++ = index_offset + 2;
	}
	for ( ; face_indx < drawable->getNumFaces(); face_indx++) 
	{
		drawable->getFace(face_indx)->setSize(0,0);
	}

	return TRUE;
}
