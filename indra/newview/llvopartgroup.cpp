/** 
 * @file llvopartgroup.cpp
 * @brief Group of particle systems
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llvopartgroup.h"

#include "lldrawpoolalpha.h"

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
	:	LLAlphaObject(id, pcode, regionp),
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

F32 LLVOPartGroup::getBinRadius()
{ 
	return mScale.mV[0]*2.f;
}

void LLVOPartGroup::updateSpatialExtents(LLVector3& newMin, LLVector3& newMax)
{		
	LLVector3 pos_agent = getPositionAgent();
	mExtents[0] = pos_agent - mScale;
	mExtents[1] = pos_agent + mScale;
	newMin = mExtents[0];
	newMax = mExtents[1];
	mDrawable->setPositionGroup(pos_agent);
}

BOOL LLVOPartGroup::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	return TRUE;
}

void LLVOPartGroup::setPixelAreaAndAngle(LLAgent &agent)
{
	// mPixelArea is calculated during render
	F32 mid_scale = getMidScale();
	F32 range = (getRenderPosition()-gCamera->getOrigin()).magVec();

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
	// Texture stats for particles need to be updated in a different way...
}


LLDrawable* LLVOPartGroup::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_PARTICLES);
	return mDrawable;
}

 const F32 MAX_PARTICLE_AREA_SCALE = 0.02f; // some tuned constant, limits on how much particle area to draw

F32 LLVOPartGroup::getPartSize(S32 idx)
{
	if (idx < (S32) mViewerPartGroupp->mParticles.size())
	{
		return mViewerPartGroupp->mParticles[idx]->mScale.mV[0];
	}

	return 0.f;
}

BOOL LLVOPartGroup::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(LLFastTimer::FTM_UPDATE_PARTICLES);

 	LLVector3 at;
	LLVector3 position_agent;
	LLVector3 camera_agent = gCamera->getOrigin();
	
	S32 num_parts = mViewerPartGroupp->getCount();
	LLFace *facep;
	LLSpatialGroup* group = drawable->getSpatialGroup();
	if (!group && num_parts)
	{
		drawable->movePartition();
		group = drawable->getSpatialGroup();
	}

	if (!num_parts)
	{
		if (group && drawable->getNumFaces())
		{
			group->dirtyGeom();
		}
		drawable->setNumFaces(0, NULL, getTEImage(0));
		LLPipeline::sCompiles++;
		return TRUE;
	}

 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES)))
	{
		return TRUE;
	}

	if (num_parts > drawable->getNumFaces())
	{
		drawable->setNumFacesFast(num_parts+num_parts/4, NULL, getTEImage(0));
	}

	F32 tot_area = 0;
	BOOL is_particle = isParticle();

	F32 max_area = LLViewerPartSim::getMaxPartCount() * MAX_PARTICLE_AREA_SCALE; 
	F32 pixel_meter_ratio = gCamera->getPixelMeterRatio();
	pixel_meter_ratio *= pixel_meter_ratio;

	S32 count=0;
	S32 i;
	F32 max_width = 0.f;
	mDepth = 0.f;

	for (i = 0; i < num_parts; i++)
	{
		const LLViewerPart &part = *((LLViewerPart*) mViewerPartGroupp->mParticles[i]);

		LLVector3 part_pos_agent(part.mPosAgent);
		at = part_pos_agent - camera_agent;

		F32 camera_dist_squared = at.magVecSquared();
		F32 inv_camera_dist_squared;
		if (camera_dist_squared > 1.f)
			inv_camera_dist_squared = 1.f / camera_dist_squared;
		else
			inv_camera_dist_squared = 1.f;
		F32 area = part.mScale.mV[0] * part.mScale.mV[1] * inv_camera_dist_squared;
		tot_area += area;
 		
		if (!is_particle && tot_area > max_area)
		{
			break;
		}
	
		count++;

		facep = drawable->getFace(i);
		if (!facep)
		{
			llwarns << "No face found for index " << i << "!" << llendl;
			continue;
		}

		facep->setTEOffset(i);
		const F32 NEAR_PART_DIST_SQ = 5.f*5.f;  // Only discard particles > 5 m from the camera
		const F32 MIN_PART_AREA = .005f*.005f;  // only less than 5 mm x 5 mm at 1 m from camera
		
		if (!is_particle)
		{
			if (camera_dist_squared > NEAR_PART_DIST_SQ && area < MIN_PART_AREA)
			{
				facep->setSize(0, 0);
				continue;
			}

			facep->setSize(4, 6);
		}
		else
		{		
			facep->setSize(1,1);
		}

		facep->setViewerObject(this);

		if (part.mFlags & LLPartData::LL_PART_EMISSIVE_MASK)
		{
			facep->setState(LLFace::FULLBRIGHT);
		}
		else
		{
			facep->clearState(LLFace::FULLBRIGHT);
		}

		facep->mCenterLocal = part.mPosAgent;
		facep->setFaceColor(part.mColor);
		facep->setTexture(part.mImagep);
		
		if (i == 0)
		{
			mExtents[0] = mExtents[1] = part.mPosAgent;
		}
		else
		{
			update_min_max(mExtents[0], mExtents[1], part.mPosAgent);
		}

		max_width = llmax(max_width, part.mScale.mV[0]);
		max_width = llmax(max_width, part.mScale.mV[1]);

		mPixelArea = tot_area * pixel_meter_ratio;
		const F32 area_scale = 10.f; // scale area to increase priority a bit
		facep->setVirtualSize(mPixelArea*area_scale);
	}
	for (i = count; i < drawable->getNumFaces(); i++)
	{
		LLFace* facep = drawable->getFace(i);
		if (!facep)
		{
			llwarns << "No face found for index " << i << "!" << llendl;
			continue;
		}
		facep->setTEOffset(i);
		facep->setSize(0,0);
	}
	
	LLVector3 y = gCamera->mYAxis;
	LLVector3 z = gCamera->mZAxis;

	LLVector3 pad;
	for (i = 0; i < 3; i++)
	{
		pad.mV[i] = llmax(max_width, max_width * (fabsf(y.mV[i]) + fabsf(z.mV[i])));
	}
	
	mExtents[0] -= pad;
	mExtents[1] += pad;
	
	mDrawable->movePartition();
	LLPipeline::sCompiles++;
	return TRUE;
}

void LLVOPartGroup::getGeometry(S32 idx,
								LLStrider<LLVector3>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U32>& indicesp)
{
	if (idx >= (S32) mViewerPartGroupp->mParticles.size())
	{
		return;
	}

	const LLViewerPart &part = *((LLViewerPart*) (mViewerPartGroupp->mParticles[idx]));

	U32 vert_offset = mDrawable->getFace(idx)->getGeomIndex();

	if (isParticle())
	{
		LLVector3 part_pos_agent(part.mPosAgent);

		const LLVector3& normal = -gCamera->getXAxis();

		*verticesp++ = part_pos_agent;
		*normalsp++ = normal;
		*colorsp++ = part.mColor;
		*texcoordsp++ = LLVector2(0.5f, 0.5f);
		*indicesp++ = vert_offset;
	}
	else
	{
		LLVector3 part_pos_agent(part.mPosAgent);
		LLVector3 camera_agent = gAgent.getCameraPositionAgent();
		LLVector3 at = part_pos_agent - camera_agent;
		LLVector3 up, right;

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

		const LLVector3& normal = -gCamera->getXAxis();
		
		*verticesp++ = part_pos_agent + up - right;
		*verticesp++ = part_pos_agent - up - right;
		*verticesp++ = part_pos_agent + up + right;
		*verticesp++ = part_pos_agent - up + right;

		*colorsp++ = part.mColor;
		*colorsp++ = part.mColor;
		*colorsp++ = part.mColor;
		*colorsp++ = part.mColor;

		*texcoordsp++ = LLVector2(0.f, 1.f);
		*texcoordsp++ = LLVector2(0.f, 0.f);
		*texcoordsp++ = LLVector2(1.f, 1.f);
		*texcoordsp++ = LLVector2(1.f, 0.f);

		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;

		*indicesp++ = vert_offset + 0;
		*indicesp++ = vert_offset + 1;
		*indicesp++ = vert_offset + 2;

		*indicesp++ = vert_offset + 1;
		*indicesp++ = vert_offset + 3;
		*indicesp++ = vert_offset + 2;
	}
}

BOOL LLVOPartGroup::isParticle()
{
	return FALSE; //gGLManager.mHasPointParameters && mViewerPartGroupp->mUniformParticles;
}

U32 LLVOPartGroup::getPartitionType() const
{ 
	return LLPipeline::PARTITION_PARTICLE; 
}

LLParticlePartition::LLParticlePartition()
: LLSpatialPartition(LLDrawPoolAlpha::VERTEX_DATA_MASK)
{
	mRenderPass = LLRenderPass::PASS_ALPHA;
	mDrawableType = LLPipeline::RENDER_TYPE_PARTICLES;
	mPartitionType = LLPipeline::PARTITION_PARTICLE;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}

void LLParticlePartition::addGeometryCount(LLSpatialGroup* group, U32& vertex_count, U32& index_count)
{
	group->mBufferUsage = mBufferUsage;

	mFaceList.clear();

	for (LLSpatialGroup::element_iter i = group->getData().begin(); i != group->getData().end(); ++i)
	{
		LLDrawable* drawablep = *i;
		
		if (drawablep->isDead())
		{
			continue;
		}

		LLAlphaObject* obj = (LLAlphaObject*) drawablep->getVObj().get();
		obj->mDepth = 0.f;
		
		if (drawablep->isAnimating())
		{
			group->mBufferUsage = GL_STREAM_DRAW_ARB;
		}

		U32 count = 0;
		for (S32 j = 0; j < drawablep->getNumFaces(); ++j)
		{
			drawablep->updateFaceSize(j);

			LLFace* facep = drawablep->getFace(j);
			if ( !facep || !facep->hasGeometry())
			{
				continue;
			}
			
			count++;
			facep->mDistance = (facep->mCenterLocal - gCamera->getOrigin()) * gCamera->getAtAxis();
			obj->mDepth += facep->mDistance;
			
			mFaceList.push_back(facep);
			vertex_count += facep->getGeomCount();
			index_count += facep->getIndicesCount();
		}
		
		obj->mDepth /= count;
	}
}

void LLParticlePartition::getGeometry(LLSpatialGroup* group)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);

	std::sort(mFaceList.begin(), mFaceList.end(), LLFace::CompareDistanceGreater());

	U32 index_count = 0;
	U32 vertex_count = 0;

	group->clearDrawMap();

	LLVertexBuffer* buffer = group->mVertexBuffer;

	LLStrider<U32> indicesp;
	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texcoordsp;
	LLStrider<LLColor4U> colorsp;

	buffer->getVertexStrider(verticesp);
	buffer->getNormalStrider(normalsp);
	buffer->getColorStrider(colorsp);
	buffer->getTexCoordStrider(texcoordsp);
	buffer->getIndexStrider(indicesp);

	LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[mRenderPass];	

	for (std::vector<LLFace*>::iterator i = mFaceList.begin(); i != mFaceList.end(); ++i)
	{
		LLFace* facep = *i;
		LLAlphaObject* object = (LLAlphaObject*) facep->getViewerObject();
		facep->setGeomIndex(vertex_count);
		facep->setIndicesIndex(index_count);
		facep->mVertexBuffer = buffer;
		facep->setPoolType(LLDrawPool::POOL_ALPHA);
		object->getGeometry(facep->getTEOffset(), verticesp, normalsp, texcoordsp, colorsp, indicesp);
		
		vertex_count += facep->getGeomCount();
		index_count += facep->getIndicesCount();

		S32 idx = draw_vec.size()-1;

		BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
		F32 vsize = facep->getVirtualSize();

		if (idx >= 0 && draw_vec[idx]->mEnd == facep->getGeomIndex()-1 &&
			draw_vec[idx]->mTexture == facep->getTexture() &&
			draw_vec[idx]->mEnd - draw_vec[idx]->mStart + facep->getGeomCount() <= (U32) gGLManager.mGLMaxVertexRange &&
			draw_vec[idx]->mCount + facep->getIndicesCount() <= (U32) gGLManager.mGLMaxIndexRange &&
			draw_vec[idx]->mEnd - draw_vec[idx]->mStart + facep->getGeomCount() < 4096 &&
			draw_vec[idx]->mFullbright == fullbright)
		{
			draw_vec[idx]->mCount += facep->getIndicesCount();
			draw_vec[idx]->mEnd += facep->getGeomCount();
			draw_vec[idx]->mVSize = llmax(draw_vec[idx]->mVSize, vsize);
		}
		else
		{
			U32 start = facep->getGeomIndex();
			U32 end = start + facep->getGeomCount()-1;
			U32 offset = facep->getIndicesStart();
			U32 count = facep->getIndicesCount();
			LLDrawInfo* info = new LLDrawInfo(start,end,count,offset,facep->getTexture(), buffer, fullbright); 
			info->mVSize = vsize;
			draw_vec.push_back(info);
		}
	}

	mFaceList.clear();
}

F32 LLParticlePartition::calcPixelArea(LLSpatialGroup* group, LLCamera& camera)
{
	return 1024.f;
}

