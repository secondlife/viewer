/** 
 * @file llvopartgroup.cpp
 * @brief Group of particle systems
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

#include "llvopartgroup.h"

#include "lldrawpoolalpha.h"

#include "llfasttimer.h"
#include "message.h"
#include "v2math.h"

#include "llagentcamera.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerpartsim.h"
#include "llviewerregion.h"
#include "pipeline.h"
#include "llspatialpartition.h"

const F32 MAX_PART_LIFETIME = 120.f;

extern U64 gFrameTime;

LLPointer<LLVertexBuffer> LLVOPartGroup::sVB = NULL;
S32 LLVOPartGroup::sVBSlotFree[];
S32* LLVOPartGroup::sVBSlotCursor = NULL;

void LLVOPartGroup::initClass()
{
	for (S32 i = 0; i < LL_MAX_PARTICLE_COUNT; ++i)
	{
		sVBSlotFree[i] = i;
	}

	sVBSlotCursor = sVBSlotFree;
}

//static
void LLVOPartGroup::restoreGL()
{
	sVB = new LLVertexBuffer(VERTEX_DATA_MASK, GL_STREAM_DRAW_ARB);
	U32 count = LL_MAX_PARTICLE_COUNT;
	sVB->allocateBuffer(count*4, count*6, true);

	//indices and texcoords are always the same, set once
	LLStrider<U16> indicesp;

	LLStrider<LLVector4a> verticesp;

	sVB->getIndexStrider(indicesp);
	sVB->getVertexStrider(verticesp);

	LLVector4a v;
	v.set(0,0,0,0);

	
	U16 vert_offset = 0;

	for (U32 i = 0; i < LL_MAX_PARTICLE_COUNT; i++)
	{
		*indicesp++ = vert_offset + 0;
		*indicesp++ = vert_offset + 1;
		*indicesp++ = vert_offset + 2;

		*indicesp++ = vert_offset + 1;
		*indicesp++ = vert_offset + 3;
		*indicesp++ = vert_offset + 2;

		*verticesp++ = v;

		vert_offset += 4;
	}

	LLStrider<LLVector2> texcoordsp;
	sVB->getTexCoord0Strider(texcoordsp);

	for (U32 i = 0; i < LL_MAX_PARTICLE_COUNT; i++)
	{
		*texcoordsp++ = LLVector2(0.f, 1.f);
		*texcoordsp++ = LLVector2(0.f, 0.f);
		*texcoordsp++ = LLVector2(1.f, 1.f);
		*texcoordsp++ = LLVector2(1.f, 0.f);
	}

	sVB->flush();

}

//static
void LLVOPartGroup::destroyGL()
{
	sVB = NULL;
}

//static
S32 LLVOPartGroup::findAvailableVBSlot()
{
	if (sVBSlotCursor >= sVBSlotFree+LL_MAX_PARTICLE_COUNT)
	{ //no more available slots
		return -1;
	}

	S32 ret = *sVBSlotCursor;
	sVBSlotCursor++;

	return ret;
}

bool ll_is_part_idx_allocated(S32 idx, S32* start, S32* end)
{
	while (start < end)
	{
		if (*start == idx)
		{ //not allocated (in free list)
			return false;
		}
		++start;
	}

	//allocated (not in free list)
	return true;
}

//static
void LLVOPartGroup::freeVBSlot(S32 idx)
{
	llassert(idx < LL_MAX_PARTICLE_COUNT && idx >= 0);
	llassert(sVBSlotCursor > sVBSlotFree);
	llassert(ll_is_part_idx_allocated(idx, sVBSlotCursor, sVBSlotFree+LL_MAX_PARTICLE_COUNT));

	if (sVBSlotCursor > sVBSlotFree)
	{
		sVBSlotCursor--;
		*sVBSlotCursor = idx;
	}
}

LLVOPartGroup::LLVOPartGroup(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	:	LLAlphaObject(id, pcode, regionp),
		mViewerPartGroupp(NULL)
{
	setNumTEs(1);
	setTETexture(0, LLUUID::null);
	mbCanSelect = FALSE;			// users can't select particle systems
}


LLVOPartGroup::~LLVOPartGroup()
{
}

BOOL LLVOPartGroup::isActive() const
{
	return FALSE;
}

F32 LLVOPartGroup::getBinRadius()
{ 
	return mScale.mV[0]*2.f;
}

void LLVOPartGroup::updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{		
	const LLVector3& pos_agent = getPositionAgent();
	newMin.load3( (pos_agent - mScale).mV);
	newMax.load3( (pos_agent + mScale).mV);
	LLVector4a pos;
	pos.load3(pos_agent.mV);
	mDrawable->setPositionGroup(pos);
}

BOOL LLVOPartGroup::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	return TRUE;
}

void LLVOPartGroup::setPixelAreaAndAngle(LLAgent &agent)
{
	// mPixelArea is calculated during render
	F32 mid_scale = getMidScale();
	F32 range = (getRenderPosition()-LLViewerCamera::getInstance()->getOrigin()).length();

	if (range < 0.001f || isHUDAttachment())		// range == zero
	{
		mAppAngle = 180.f;
	}
	else
	{
		mAppAngle = (F32) atan2( mid_scale, range) * RAD_TO_DEG;
	}
}

void LLVOPartGroup::updateTextures()
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

LLVector3 LLVOPartGroup::getCameraPosition() const
{
	return gAgentCamera.getCameraPositionAgent();
}

static LLFastTimer::DeclareTimer FTM_UPDATE_PARTICLES("Update Particles");
BOOL LLVOPartGroup::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(FTM_UPDATE_PARTICLES);

	dirtySpatialGroup();
	
	S32 num_parts = mViewerPartGroupp->getCount();
	LLFace *facep;
	LLSpatialGroup* group = drawable->getSpatialGroup();
	if (!group && num_parts)
	{
		drawable->movePartition();
		group = drawable->getSpatialGroup();
	}

	if (group && group->isVisible())
	{
		dirtySpatialGroup(TRUE);
	}

	if (!num_parts)
	{
		if (group && drawable->getNumFaces())
		{
			group->setState(LLSpatialGroup::GEOM_DIRTY);
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

	F32 max_area = LLViewerPartSim::getMaxPartCount() * MAX_PARTICLE_AREA_SCALE; 
	F32 pixel_meter_ratio = LLViewerCamera::getInstance()->getPixelMeterRatio();
	pixel_meter_ratio *= pixel_meter_ratio;

	LLViewerPartSim::checkParticleCount(mViewerPartGroupp->mParticles.size()) ;

	S32 count=0;
	mDepth = 0.f;
	S32 i = 0 ;
	LLVector3 camera_agent = getCameraPosition();
	for (i = 0 ; i < (S32)mViewerPartGroupp->mParticles.size(); i++)
	{
		const LLViewerPart *part = mViewerPartGroupp->mParticles[i];

		LLVector3 part_pos_agent(part->mPosAgent);
		LLVector3 at(part_pos_agent - camera_agent);

		F32 camera_dist_squared = at.lengthSquared();
		F32 inv_camera_dist_squared;
		if (camera_dist_squared > 1.f)
			inv_camera_dist_squared = 1.f / camera_dist_squared;
		else
			inv_camera_dist_squared = 1.f;
		F32 area = part->mScale.mV[0] * part->mScale.mV[1] * inv_camera_dist_squared;
		tot_area = llmax(tot_area, area);
 		
		if (tot_area > max_area)
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
		
		if (camera_dist_squared > NEAR_PART_DIST_SQ && area < MIN_PART_AREA)
		{
			facep->setSize(0, 0);
			continue;
		}

		facep->setSize(4, 6);
		
		facep->setViewerObject(this);

		if (part->mFlags & LLPartData::LL_PART_EMISSIVE_MASK)
		{
			facep->setState(LLFace::FULLBRIGHT);
		}
		else
		{
			facep->clearState(LLFace::FULLBRIGHT);
		}

		facep->mCenterLocal = part->mPosAgent;
		facep->setFaceColor(part->mColor);
		facep->setTexture(part->mImagep);
			
		//check if this particle texture is replaced by a parcel media texture.
		if(part->mImagep.notNull() && part->mImagep->hasParcelMedia()) 
		{
			part->mImagep->getParcelMedia()->addMediaToFace(facep) ;
		}

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
		facep->setSize(0, 0);
	}

	mDrawable->movePartition();
	LLPipeline::sCompiles++;
	return TRUE;
}

void LLVOPartGroup::getGeometry(S32 idx,
								LLStrider<LLVector4a>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U16>& indicesp)
{
	if (idx >= (S32) mViewerPartGroupp->mParticles.size())
	{
		return;
	}

	const LLViewerPart &part = *((LLViewerPart*) (mViewerPartGroupp->mParticles[idx]));

	LLVector4a part_pos_agent;
	part_pos_agent.load3(part.mPosAgent.mV);
	LLVector4a camera_agent;
	camera_agent.load3(getCameraPosition().mV); 
	LLVector4a at;
	at.setSub(part_pos_agent, camera_agent);
	LLVector4a up(0, 0, 1);
	LLVector4a right;

	right.setCross3(at, up);
	right.normalize3fast();
	up.setCross3(right, at);
	up.normalize3fast();

	if (part.mFlags & LLPartData::LL_PART_FOLLOW_VELOCITY_MASK)
	{
		LLVector4a normvel;
		normvel.load3(part.mVelocity.mV);
		normvel.normalize3fast();
		LLVector2 up_fracs;
		up_fracs.mV[0] = normvel.dot3(right).getF32();
		up_fracs.mV[1] = normvel.dot3(up).getF32();
		up_fracs.normalize();
		LLVector4a new_up;
		LLVector4a new_right;

		//new_up = up_fracs.mV[0] * right + up_fracs.mV[1]*up;
		LLVector4a t = right;
		t.mul(up_fracs.mV[0]);
		new_up = up;
		new_up.mul(up_fracs.mV[1]);
		new_up.add(t);

		//new_right = up_fracs.mV[1] * right - up_fracs.mV[0]*up;
		t = right;
		t.mul(up_fracs.mV[1]);
		new_right = up;
		new_right.mul(up_fracs.mV[0]);
		t.sub(new_right);

		up = new_up;
		right = t;
		up.normalize3fast();
		right.normalize3fast();
	}

	right.mul(0.5f*part.mScale.mV[0]);
	up.mul(0.5f*part.mScale.mV[1]);


	LLVector3 normal = -LLViewerCamera::getInstance()->getXAxis();

	//HACK -- the verticesp->mV[3] = 0.f here are to set the texture index to 0 (particles don't use texture batching, maybe they should)
	// this works because there is actually a 4th float stored after the vertex position which is used as a texture index
	// also, somebody please VECTORIZE THIS

	LLVector4a ppapu;
	LLVector4a ppamu;

	ppapu.setAdd(part_pos_agent, up);
	ppamu.setSub(part_pos_agent, up);

	verticesp->setSub(ppapu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;
	verticesp->setSub(ppamu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;
	verticesp->setAdd(ppapu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;
	verticesp->setAdd(ppamu, right);
	(*verticesp++).getF32ptr()[3] = 0.f;

	*colorsp++ = part.mColor;
	*colorsp++ = part.mColor;
	*colorsp++ = part.mColor;
	*colorsp++ = part.mColor;

	if (!(part.mFlags & LLPartData::LL_PART_EMISSIVE_MASK))
	{ //not fullbright, needs normal
		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;
		*normalsp++   = normal;
	}
}

U32 LLVOPartGroup::getPartitionType() const
{ 
	return LLViewerRegion::PARTITION_PARTICLE; 
}

LLParticlePartition::LLParticlePartition()
: LLSpatialPartition(LLDrawPoolAlpha::VERTEX_DATA_MASK | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, GL_STREAM_DRAW_ARB)
{
	mRenderPass = LLRenderPass::PASS_ALPHA;
	mDrawableType = LLPipeline::RENDER_TYPE_PARTICLES;
	mPartitionType = LLViewerRegion::PARTITION_PARTICLE;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}

LLHUDParticlePartition::LLHUDParticlePartition() :
	LLParticlePartition()
{
	mDrawableType = LLPipeline::RENDER_TYPE_HUD_PARTICLES;
	mPartitionType = LLViewerRegion::PARTITION_HUD_PARTICLE;
}

static LLFastTimer::DeclareTimer FTM_REBUILD_PARTICLE_VBO("Particle VBO");

void LLParticlePartition::rebuildGeom(LLSpatialGroup* group)
{
	if (group->isDead() || !group->isState(LLSpatialGroup::GEOM_DIRTY))
	{
		return;
	}

	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
		group->mLastUpdateViewAngle = group->mViewAngle;
	}
	
	LLFastTimer ftm(FTM_REBUILD_PARTICLE_VBO);	

	group->clearDrawMap();
	
	//get geometry count
	U32 index_count = 0;
	U32 vertex_count = 0;

	addGeometryCount(group, vertex_count, index_count);
	

	if (vertex_count > 0 && index_count > 0)
	{ 
		group->mBuilt = 1.f;
		//use one vertex buffer for all groups
		group->mVertexBuffer = LLVOPartGroup::sVB;
		getGeometry(group);
	}
	else
	{
		group->mVertexBuffer = NULL;
		group->mBufferMap.clear();
	}

	group->mLastUpdateTime = gFrameTimeSeconds;
	group->clearState(LLSpatialGroup::GEOM_DIRTY);
}

void LLParticlePartition::addGeometryCount(LLSpatialGroup* group, U32& vertex_count, U32& index_count)
{
	group->mBufferUsage = mBufferUsage;

	mFaceList.clear();

	LLViewerCamera* camera = LLViewerCamera::getInstance();
	for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
	{
		LLDrawable* drawablep = *i;
		
		if (drawablep->isDead())
		{
			continue;
		}

		LLAlphaObject* obj = (LLAlphaObject*) drawablep->getVObj().get();
		obj->mDepth = 0.f;
		
		U32 count = 0;
		for (S32 j = 0; j < drawablep->getNumFaces(); ++j)
		{
			drawablep->updateFaceSize(j);

			LLFace* facep = drawablep->getFace(j);
			if ( !facep || !facep->hasGeometry())
			{
				continue;
			}
			
			vertex_count += facep->getGeomCount();
			index_count += facep->getIndicesCount();

			count++;
			facep->mDistance = (facep->mCenterLocal - camera->getOrigin()) * camera->getAtAxis();
			obj->mDepth += facep->mDistance;
			
			mFaceList.push_back(facep);
			llassert(facep->getIndicesCount() < 65536);
		}
		
		obj->mDepth /= count;
	}
}


static LLFastTimer::DeclareTimer FTM_REBUILD_PARTICLE_GEOM("Particle Geom");

void LLParticlePartition::getGeometry(LLSpatialGroup* group)
{
	LLMemType mt(LLMemType::MTYPE_SPACE_PARTITION);
	LLFastTimer ftm(FTM_REBUILD_PARTICLE_GEOM);

	std::sort(mFaceList.begin(), mFaceList.end(), LLFace::CompareDistanceGreater());

	U32 index_count = 0;
	U32 vertex_count = 0;

	group->clearDrawMap();

	LLVertexBuffer* buffer = group->mVertexBuffer;

	LLStrider<U16> indicesp;
	LLStrider<LLVector4a> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> texcoordsp;
	LLStrider<LLColor4U> colorsp;

	buffer->getVertexStrider(verticesp);
	buffer->getNormalStrider(normalsp);
	buffer->getColorStrider(colorsp);
	
	LLSpatialGroup::drawmap_elem_t& draw_vec = group->mDrawMap[mRenderPass];	

	for (std::vector<LLFace*>::iterator i = mFaceList.begin(); i != mFaceList.end(); ++i)
	{
		LLFace* facep = *i;
		LLAlphaObject* object = (LLAlphaObject*) facep->getViewerObject();

		if (!facep->isState(LLFace::PARTICLE))
		{ //set the indices of this face
			S32 idx = LLVOPartGroup::findAvailableVBSlot();
			if (idx >= 0)
			{
				facep->setGeomIndex(idx*4);
				facep->setIndicesIndex(idx*6);
				facep->setVertexBuffer(LLVOPartGroup::sVB);
				facep->setPoolType(LLDrawPool::POOL_ALPHA);
				facep->setState(LLFace::PARTICLE);
			}
			else
			{
				continue; //out of space in particle buffer
			}		
		}

		S32 geom_idx = (S32) facep->getGeomIndex();

		LLStrider<U16> cur_idx = indicesp + facep->getIndicesStart();
		LLStrider<LLVector4a> cur_vert = verticesp + geom_idx;
		LLStrider<LLVector3> cur_norm = normalsp + geom_idx;
		LLStrider<LLVector2> cur_tc = texcoordsp + geom_idx;
		LLStrider<LLColor4U> cur_col = colorsp + geom_idx;

		object->getGeometry(facep->getTEOffset(), cur_vert, cur_norm, cur_tc, cur_col, cur_idx);
		
		llassert(facep->getGeomCount() == 4);
		llassert(facep->getIndicesCount() == 6);


		vertex_count += facep->getGeomCount();
		index_count += facep->getIndicesCount();

		S32 idx = draw_vec.size()-1;

		BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
		F32 vsize = facep->getVirtualSize();

		bool batched = false;
	
		if (idx >= 0 &&
			draw_vec[idx]->mTexture == facep->getTexture() &&
			draw_vec[idx]->mFullbright == fullbright)
		{
			if (draw_vec[idx]->mEnd == facep->getGeomIndex()-1)
			{
				batched = true;
				draw_vec[idx]->mCount += facep->getIndicesCount();
				draw_vec[idx]->mEnd += facep->getGeomCount();
				draw_vec[idx]->mVSize = llmax(draw_vec[idx]->mVSize, vsize);
			}
			else if (draw_vec[idx]->mStart == facep->getGeomIndex()+facep->getGeomCount()+1)
			{
				batched = true;
				draw_vec[idx]->mCount += facep->getIndicesCount();
				draw_vec[idx]->mStart -= facep->getGeomCount();
				draw_vec[idx]->mOffset = facep->getIndicesStart();
				draw_vec[idx]->mVSize = llmax(draw_vec[idx]->mVSize, vsize);
			}
		}


		if (!batched)
		{
			U32 start = facep->getGeomIndex();
			U32 end = start + facep->getGeomCount()-1;
			U32 offset = facep->getIndicesStart();
			U32 count = facep->getIndicesCount();
			LLDrawInfo* info = new LLDrawInfo(start,end,count,offset,facep->getTexture(), 
				//facep->getTexture(),
				buffer, fullbright); 
			info->mExtents[0] = group->mObjectExtents[0];
			info->mExtents[1] = group->mObjectExtents[1];
			info->mVSize = vsize;
			draw_vec.push_back(info);
			//for alpha sorting
			facep->setDrawInfo(info);
		}
	}

	mFaceList.clear();
}

F32 LLParticlePartition::calcPixelArea(LLSpatialGroup* group, LLCamera& camera)
{
	return 1024.f;
}

U32 LLVOHUDPartGroup::getPartitionType() const
{
	return LLViewerRegion::PARTITION_HUD_PARTICLE; 
}

LLDrawable* LLVOHUDPartGroup::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_HUD_PARTICLES);
	return mDrawable;
}

LLVector3 LLVOHUDPartGroup::getCameraPosition() const
{
	return LLVector3(-1,0,0);
}

