/** 
 * @file llvopartgroup.cpp
 * @brief Group of particle systems
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvopartgroup.h"

#include "llfasttimer.h"
#include "message.h"
#include "v2math.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerpartsim.h"
#include "llviewerregion.h"
#include "pipeline.h"

const F32 MAX_PART_LIFETIME = 120.f;

extern U64 gFrameTime;

LLVOPartGroup::LLVOPartGroup(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	:	LLViewerObject(id, pcode, regionp),
		mViewerPartGroupp(NULL)
{
	setNumTEs(1);
	setTETexture(0, LLUUID::null);
	mbCanSelect = FALSE;			// users can't select particle systems
	mDebugColor = LLColor4(ll_frand(), ll_frand(), ll_frand(), 1.f);
}


LLVOPartGroup::~LLVOPartGroup()
{
}

BOOL LLVOPartGroup::isActive() const
{
	return TRUE;
}

BOOL LLVOPartGroup::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES))
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}

	// Particle groups don't need any of default idleUpdate velocity interpolation stuff.
	//LLViewerObject::idleUpdate(agent, world, time);
	return TRUE;
}


void LLVOPartGroup::setPixelAreaAndAngle(LLAgent &agent)
{
	// mPixelArea is calculated during render

	LLVector3 viewer_pos_agent = agent.getCameraPositionAgent();
	LLVector3 pos_agent = getRenderPosition();

	F32 dx = viewer_pos_agent.mV[VX] - pos_agent.mV[VX];
	F32 dy = viewer_pos_agent.mV[VY] - pos_agent.mV[VY];
	F32 dz = viewer_pos_agent.mV[VZ] - pos_agent.mV[VZ];

	F32 mid_scale = getMidScale();
	F32 range = sqrt(dx*dx + dy*dy + dz*dz);

	if (range < 0.001f || isHUDAttachment())		// range == zero
	{
		mAppAngle = 180.f;
	}
	else
	{
		mAppAngle = (F32) atan2( mid_scale, range) * RAD_TO_DEG;
	}
}

void LLVOPartGroup::updateTextures(LLAgent &agent)
{
	// Texture stats for particles will need to be updated in a different way...
}


LLDrawable* LLVOPartGroup::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_PARTICLES);

	LLDrawPool *pool = gPipeline.getPool(LLDrawPool::POOL_ALPHA);
	mDrawable->setNumFaces(mViewerPartGroupp->getCount(), pool, getTEImage(0));
	return mDrawable;
}

 const F32 MAX_PARTICLE_AREA_SCALE = 0.02f; // some tuned constant, limits on how much particle area to draw

BOOL LLVOPartGroup::updateGeometry(LLDrawable *drawable)
{
 	LLFastTimer t(LLFastTimer::FTM_UPDATE_PARTICLES);
	
	LLVector3 at;
	LLVector3 up;
	LLVector3 right;
	LLVector3 position_agent;
	LLVector3 camera_agent = gAgent.getCameraPositionAgent();
	LLVector2 uvs[4];

	uvs[0].setVec(0.f, 1.f);
	uvs[1].setVec(0.f, 0.f);
	uvs[2].setVec(1.f, 1.f);
	uvs[3].setVec(1.f, 0.f);

	LLDrawPool *drawpool = gPipeline.getPool(LLDrawPool::POOL_ALPHA);
	S32 num_parts = mViewerPartGroupp->getCount();
	LLFace *facep;
	
	if (!num_parts)
	{
		drawable->setNumFaces(0, drawpool, getTEImage(0));
		LLPipeline::sCompiles++;
		return TRUE;
	}

 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES)))
	{
		return TRUE;
	}

// 	drawable->setNumFaces(num_parts, drawpool, getTEImage(0));
	drawable->setNumFacesFast(num_parts, drawpool, getTEImage(0));
	
	LLVector3 normal(-gCamera->getXAxis()); // make point agent face camera
	
	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texCoordsp;
	U32 *indicesp = 0;

	S32 vert_offset;
	F32 tot_area = 0;
	F32 max_area = LLViewerPartSim::getMaxPartCount() * MAX_PARTICLE_AREA_SCALE; 
	
	S32 count=0;
	for (S32 i = 0; i < num_parts; i++)
	{
		const LLViewerPart &part = mViewerPartGroupp->mParticles[i];

		LLVector3 part_pos_agent(part.mPosAgent);
		at = part_pos_agent - camera_agent;

		//F32 invcamdist = 1.0f / at.magVec();
		//area += (part.mScale.mV[0]*invcamdist)*(part.mScale.mV[1]*invcamdist);
		F32 camera_dist_squared = at.magVecSquared();
		F32 inv_camera_dist_squared;
		if (camera_dist_squared > 1.f)
			inv_camera_dist_squared = 1.f / camera_dist_squared;
		else
			inv_camera_dist_squared = 1.f;
		F32 area = part.mScale.mV[0] * part.mScale.mV[1] * inv_camera_dist_squared;
		tot_area += area;
 		if (tot_area > max_area)
		{
			break;
		}
		
		count++;

		facep = drawable->getFace(i);
		const F32 NEAR_PART_DIST_SQ = 5.f*5.f;  // Only discard particles > 5 m from the camera
		const F32 MIN_PART_AREA = .005f*.005f;  // only less than 5 mm x 5 mm at 1 m from camera
		if (camera_dist_squared > NEAR_PART_DIST_SQ && area < MIN_PART_AREA)
		{
			facep->setSize(0, 0);
			continue;
		}
		facep->setSize(4, 6);
		facep->mCenterAgent = part_pos_agent;
		vert_offset = facep->getGeometry(verticesp,normalsp,texCoordsp, indicesp);
		if (-1 == vert_offset)
		{
			return TRUE;
		}
		
		right = at % LLVector3(0.f, 0.f, 1.f);
		right.normVec();
		up = right % at;
		up.normVec();

		if (part.mFlags & LLPartData::LL_PART_FOLLOW_VELOCITY_MASK)
		{
			LLVector3 normvel = part.mVelocity;
			normvel.normVec();
			LLVector2 up_fracs;
			up_fracs.mV[0] = normvel*right;
			up_fracs.mV[1] = normvel*up;
			up_fracs.normVec();

			LLVector3 new_up;
			LLVector3 new_right;
			new_up = up_fracs.mV[0] * right + up_fracs.mV[1]*up;
			new_right = up_fracs.mV[1] * right - up_fracs.mV[0]*up;
			up = new_up;
			right = new_right;
			up.normVec();
			right.normVec();
		}

		right *= 0.5f*part.mScale.mV[0];
		up *= 0.5f*part.mScale.mV[1];

		*verticesp++ = part_pos_agent + up - right;
		*verticesp++ = part_pos_agent - up - right;
		*verticesp++ = part_pos_agent + up + right;
		*verticesp++ = part_pos_agent - up + right;

		*texCoordsp++ = uvs[0];
		*texCoordsp++ = uvs[1];
		*texCoordsp++ = uvs[2];
		*texCoordsp++ = uvs[3];

		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;

		if (part.mFlags & LLPartData::LL_PART_EMISSIVE_MASK)
		{
			facep->setState(LLFace::FULLBRIGHT);
		}
		else
		{
			facep->clearState(LLFace::FULLBRIGHT);
		}
		facep->setFaceColor(part.mColor);
		
		*indicesp++ = vert_offset + 0;
		*indicesp++ = vert_offset + 1;
		*indicesp++ = vert_offset + 2;

		*indicesp++ = vert_offset + 1;
		*indicesp++ = vert_offset + 3;
		*indicesp++ = vert_offset + 2;
	}
	for (S32 j = count; j < drawable->getNumFaces(); j++) 
	{
		drawable->getFace(j)->setSize(0,0);
	}
	
	mPixelArea = tot_area * gCamera->getPixelMeterRatio() * gCamera->getPixelMeterRatio();
	const F32 area_scale = 10.f; // scale area to increase priority a bit
	getTEImage(0)->addTextureStats(mPixelArea * area_scale);

	LLPipeline::sCompiles++;

	return TRUE;
}

