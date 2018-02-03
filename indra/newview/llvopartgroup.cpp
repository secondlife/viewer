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

extern U64MicrosecondsImplicit gFrameTime;

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

	//TODO: optimize out binormal mask here.  Specular and normal coords as well.
	sVB = new LLVertexBuffer(VERTEX_DATA_MASK | LLVertexBuffer::MAP_TANGENT | LLVertexBuffer::MAP_TEXCOORD1 | LLVertexBuffer::MAP_TEXCOORD2, GL_STREAM_DRAW_ARB);
	U32 count = LL_MAX_PARTICLE_COUNT;
	if (!sVB->allocateBuffer(count*4, count*6, true))
	{
		LL_WARNS() << "Failed to allocate Vertex Buffer to "
			<< count*4 << " vertices and "
			<< count * 6 << " indices" << LL_ENDL;
		// we are likelly to crash at following getTexCoord0Strider(), so unref and return
		sVB = NULL;
		return;
	}

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
	if (sVBSlotCursor >= sVBSlotFree + LL_MAX_PARTICLE_COUNT)
	{ //no more available slots
		return -1;
	}

	S32 ret = *sVBSlotCursor;
	sVBSlotCursor++;
	return ret;
}

bool ll_is_part_idx_allocated(S32 idx, S32* start, S32* end)
{
	/*while (start < end)
	{
		if (*start == idx)
		{ //not allocated (in free list)
			return false;
		}
		++start;
	}*/

	//allocated (not in free list)
	return false;
}

//static
void LLVOPartGroup::freeVBSlot(S32 idx)
{
	llassert(idx < LL_MAX_PARTICLE_COUNT && idx >= 0);
	//llassert(sVBSlotCursor > sVBSlotFree);
	//llassert(ll_is_part_idx_allocated(idx, sVBSlotCursor, sVBSlotFree+LL_MAX_PARTICLE_COUNT));

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
	return mViewerPartGroupp->getBoxSide();
}

void LLVOPartGroup::updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{		
	const LLVector3& pos_agent = getPositionAgent();

	LLVector4a scale;
	LLVector4a p;

	p.load3(pos_agent.mV);

	scale.splat(mScale.mV[0]+mViewerPartGroupp->getBoxSide()*0.5f);

	newMin.setSub(p, scale);
	newMax.setAdd(p,scale);

	llassert(newMin.isFinite3());
	llassert(newMax.isFinite3());

	llassert(p.isFinite3());
	mDrawable->setPositionGroup(p);
}

void LLVOPartGroup::idleUpdate(LLAgent &agent, const F64 &time)
{
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

 LLUUID LLVOPartGroup::getPartOwner(S32 idx)
 {
	 LLUUID ret = LLUUID::null;

	 if (idx < (S32) mViewerPartGroupp->mParticles.size())
	 {
		 ret = mViewerPartGroupp->mParticles[idx]->mPartSourcep->getOwnerUUID();
	 }

	 return ret;
 }

 LLUUID LLVOPartGroup::getPartSource(S32 idx)
 {
	 LLUUID ret = LLUUID::null;

	 if (idx < (S32) mViewerPartGroupp->mParticles.size())
	 {
		 LLViewerPart* part = mViewerPartGroupp->mParticles[idx];
		 if (part && part->mPartSourcep.notNull() &&
			 part->mPartSourcep->mSourceObjectp.notNull())
		 {
			 LLViewerObject* source = part->mPartSourcep->mSourceObjectp;
			 ret = source->getID();
		 }
	 }

	 return ret;
 }


F32 LLVOPartGroup::getPartSize(S32 idx)
{
	if (idx < (S32) mViewerPartGroupp->mParticles.size())
	{
		return mViewerPartGroupp->mParticles[idx]->mScale.mV[0];
	}

	return 0.f;
}

void LLVOPartGroup::getBlendFunc(S32 idx, U32& src, U32& dst)
{
	if (idx < (S32) mViewerPartGroupp->mParticles.size())
	{
		LLViewerPart* part = mViewerPartGroupp->mParticles[idx];
		src = part->mBlendFuncSource;
		dst = part->mBlendFuncDest;
	}
}

LLVector3 LLVOPartGroup::getCameraPosition() const
{
	return gAgentCamera.getCameraPositionAgent();
}

static LLTrace::BlockTimerStatHandle FTM_UPDATE_PARTICLES("Update Particles");
BOOL LLVOPartGroup::updateGeometry(LLDrawable *drawable)
{
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_PARTICLES);

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
	
	F32 max_scale = 0.f;


	for (i = 0 ; i < (S32)mViewerPartGroupp->mParticles.size(); i++)
	{
		const LLViewerPart *part = mViewerPartGroupp->mParticles[i];


		//remember the largest particle
		max_scale = llmax(max_scale, part->mScale.mV[0], part->mScale.mV[1]);

		if (part->mFlags & LLPartData::LL_PART_RIBBON_MASK)
		{ //include ribbon segment length in scale
			const LLVector3* pos_agent = NULL;
			if (part->mParent)
			{
				pos_agent = &(part->mParent->mPosAgent);
			}
			else if (part->mPartSourcep.notNull())
			{
				pos_agent = &(part->mPartSourcep->mPosAgent);
			}

			if (pos_agent)
			{
				F32 dist = (*pos_agent-part->mPosAgent).length();

				max_scale = llmax(max_scale, dist);
			}
		}

		LLVector3 part_pos_agent(part->mPosAgent);
		LLVector3 at(part_pos_agent - camera_agent);

		
		F32 camera_dist_squared = at.lengthSquared();
		F32 inv_camera_dist_squared;
		if (camera_dist_squared > 1.f)
			inv_camera_dist_squared = 1.f / camera_dist_squared;
		else
			inv_camera_dist_squared = 1.f;

		llassert(llfinite(inv_camera_dist_squared));
		llassert(!llisnan(inv_camera_dist_squared));

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
			LL_WARNS() << "No face found for index " << i << "!" << LL_ENDL;
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
			LL_WARNS() << "No face found for index " << i << "!" << LL_ENDL;
			continue;
		}
		facep->setTEOffset(i);
		facep->setSize(0, 0);
	}

	//record max scale (used to stretch bounding box for visibility culling)
	
	mScale.set(max_scale, max_scale, max_scale);

	mDrawable->movePartition();
	LLPipeline::sCompiles++;
	return TRUE;
}


BOOL LLVOPartGroup::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
										  S32 face,
										  BOOL pick_transparent,
										  BOOL pick_rigged,
										  S32* face_hit,
										  LLVector4a* intersection,
										  LLVector2* tex_coord,
										  LLVector4a* normal,
										  LLVector4a* bi_normal)
{
	LLVector4a dir;
	dir.setSub(end, start);

	F32 closest_t = 2.f;
	BOOL ret = FALSE;
	
	for (U32 idx = 0; idx < mViewerPartGroupp->mParticles.size(); ++idx)
	{
		const LLViewerPart &part = *((LLViewerPart*) (mViewerPartGroupp->mParticles[idx]));

		LLVector4a v[4];
		LLStrider<LLVector4a> verticesp;
		verticesp = v;
		
		getGeometry(part, verticesp);

		F32 a,b,t;
		if (LLTriangleRayIntersect(v[0], v[1], v[2], start, dir, a,b,t) ||
			LLTriangleRayIntersect(v[1], v[3], v[2], start, dir, a,b,t))
		{
			if (t >= 0.f &&
				t <= 1.f &&
				t < closest_t)
			{
				ret = TRUE;
				closest_t = t;
				if (face_hit)
				{
					*face_hit = idx;
				}

				if (intersection)
				{
					LLVector4a intersect = dir;
					intersect.mul(closest_t);
					intersection->setAdd(intersect, start);
				}
			}
		}
	}

	return ret;
}

void LLVOPartGroup::getGeometry(const LLViewerPart& part,
								LLStrider<LLVector4a>& verticesp)
{
	if (part.mFlags & LLPartData::LL_PART_RIBBON_MASK)
	{
		LLVector4a axis, pos, paxis, ppos;
		F32 scale, pscale;

		pos.load3(part.mPosAgent.mV);
		axis.load3(part.mAxis.mV);
		scale = part.mScale.mV[0];
		
		if (part.mParent)
		{
			ppos.load3(part.mParent->mPosAgent.mV);
			paxis.load3(part.mParent->mAxis.mV);
			pscale = part.mParent->mScale.mV[0];
		}
		else
		{ //use source object as position
			
			if (part.mPartSourcep->mSourceObjectp.notNull())
			{
				LLVector3 v = LLVector3(0,0,1);
				v *= part.mPartSourcep->mSourceObjectp->getRenderRotation();
				paxis.load3(v.mV);
				ppos.load3(part.mPartSourcep->mPosAgent.mV);
				pscale = part.mStartScale.mV[0];
			}
			else
			{ //no source object, no parent, nothing to draw
				ppos = pos;
				pscale = scale;
				paxis = axis;
			}
		}

		LLVector4a p0, p1, p2, p3;

		scale *= 0.5f;
		pscale *= 0.5f;

		axis.mul(scale);
		paxis.mul(pscale);

		p0.setAdd(pos, axis);
		p1.setSub(pos,axis);
		p2.setAdd(ppos, paxis);
		p3.setSub(ppos, paxis);

		(*verticesp++) = p2;
		(*verticesp++) = p3;
		(*verticesp++) = p0;
		(*verticesp++) = p1;
	}
	else
	{
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
	}
}


								
void LLVOPartGroup::getGeometry(S32 idx,
								LLStrider<LLVector4a>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<LLColor4U>& emissivep,
								LLStrider<U16>& indicesp)
{
	if (idx >= (S32) mViewerPartGroupp->mParticles.size())
	{
		return;
	}
	
	const LLViewerPart &part = *((LLViewerPart*) (mViewerPartGroupp->mParticles[idx]));

	getGeometry(part, verticesp);

	LLColor4U pcolor;
	LLColor4U color = part.mColor;

	LLColor4U pglow;

	if (part.mFlags & LLPartData::LL_PART_RIBBON_MASK)
	{ //make sure color blends properly
		if (part.mParent)
		{
			pglow = part.mParent->mGlow;
			pcolor = part.mParent->mColor;
		}
		else 
		{
			pglow = LLColor4U(0, 0, 0, (U8) ll_round(255.f*part.mStartGlow));
			pcolor = part.mStartColor;
		}
	}
	else
	{
		pglow = part.mGlow;
		pcolor = color;
	}

	*colorsp++ = pcolor;
	*colorsp++ = pcolor;
	*colorsp++ = color;
	*colorsp++ = color;

	//if (pglow.mV[3] || part.mGlow.mV[3])
	{ //only write glow if it is not zero
		*emissivep++ = pglow;
		*emissivep++ = pglow;
		*emissivep++ = part.mGlow;
		*emissivep++ = part.mGlow;
	}


	if (!(part.mFlags & LLPartData::LL_PART_EMISSIVE_MASK))
	{ //not fullbright, needs normal
		LLVector3 normal = -LLViewerCamera::getInstance()->getXAxis();
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

LLParticlePartition::LLParticlePartition(LLViewerRegion* regionp)
: LLSpatialPartition(LLDrawPoolAlpha::VERTEX_DATA_MASK | LLVertexBuffer::MAP_TEXTURE_INDEX, TRUE, GL_STREAM_DRAW_ARB, regionp)
{
	mRenderPass = LLRenderPass::PASS_ALPHA;
	mDrawableType = LLPipeline::RENDER_TYPE_PARTICLES;
	mPartitionType = LLViewerRegion::PARTITION_PARTICLE;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}

LLHUDParticlePartition::LLHUDParticlePartition(LLViewerRegion* regionp) :
	LLParticlePartition(regionp)
{
	mDrawableType = LLPipeline::RENDER_TYPE_HUD_PARTICLES;
	mPartitionType = LLViewerRegion::PARTITION_HUD_PARTICLE;
}

static LLTrace::BlockTimerStatHandle FTM_REBUILD_PARTICLE_VBO("Particle VBO");

void LLParticlePartition::rebuildGeom(LLSpatialGroup* group)
{
	if (group->isDead() || !group->hasState(LLSpatialGroup::GEOM_DIRTY))
	{
		return;
	}

	if (group->changeLOD())
	{
		group->mLastUpdateDistance = group->mDistance;
		group->mLastUpdateViewAngle = group->mViewAngle;
	}
	
	LL_RECORD_BLOCK_TIME(FTM_REBUILD_PARTICLE_VBO);	

	group->clearDrawMap();
	
	//get geometry count
	U32 index_count = 0;
	U32 vertex_count = 0;

	addGeometryCount(group, vertex_count, index_count);
	

	if (vertex_count > 0 && index_count > 0 && LLVOPartGroup::sVB)
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
		LLDrawable* drawablep = (LLDrawable*)(*i)->getDrawable();
		
		if (!drawablep || drawablep->isDead())
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


static LLTrace::BlockTimerStatHandle FTM_REBUILD_PARTICLE_GEOM("Particle Geom");

void LLParticlePartition::getGeometry(LLSpatialGroup* group)
{
	LL_RECORD_BLOCK_TIME(FTM_REBUILD_PARTICLE_GEOM);

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
	LLStrider<LLColor4U> emissivep;

	buffer->getVertexStrider(verticesp);
	buffer->getNormalStrider(normalsp);
	buffer->getColorStrider(colorsp);
	buffer->getEmissiveStrider(emissivep);

	
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
		LLStrider<LLColor4U> cur_glow = emissivep + geom_idx;

		LLColor4U* start_glow = cur_glow.get();

		object->getGeometry(facep->getTEOffset(), cur_vert, cur_norm, cur_tc, cur_col, cur_glow, cur_idx);
		
		BOOL has_glow = FALSE;

		if (cur_glow.get() != start_glow)
		{
			has_glow = TRUE;
		}

		llassert(facep->getGeomCount() == 4);
		llassert(facep->getIndicesCount() == 6);


		vertex_count += facep->getGeomCount();
		index_count += facep->getIndicesCount();

		S32 idx = draw_vec.size()-1;

		BOOL fullbright = facep->isState(LLFace::FULLBRIGHT);
		F32 vsize = facep->getVirtualSize();

		bool batched = false;
	
		U32 bf_src = LLRender::BF_SOURCE_ALPHA;
		U32 bf_dst = LLRender::BF_ONE_MINUS_SOURCE_ALPHA;

		object->getBlendFunc(facep->getTEOffset(), bf_src, bf_dst);

		
		if (idx >= 0)
		{
			LLDrawInfo* info = draw_vec[idx];

			if (info->mTexture == facep->getTexture() &&
				info->mHasGlow == has_glow &&
				info->mFullbright == fullbright &&
				info->mBlendFuncDst == bf_dst &&
				info->mBlendFuncSrc == bf_src)
			{
				if (draw_vec[idx]->mEnd == facep->getGeomIndex()-1)
				{
					batched = true;
					info->mCount += facep->getIndicesCount();
					info->mEnd += facep->getGeomCount();
					info->mVSize = llmax(draw_vec[idx]->mVSize, vsize);
				}
				else if (draw_vec[idx]->mStart == facep->getGeomIndex()+facep->getGeomCount()+1)
				{
					batched = true;
					info->mCount += facep->getIndicesCount();
					info->mStart -= facep->getGeomCount();
					info->mOffset = facep->getIndicesStart();
					info->mVSize = llmax(draw_vec[idx]->mVSize, vsize);
				}
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
				buffer, object->isSelected(), fullbright);

			const LLVector4a* exts = group->getObjectExtents();
			info->mExtents[0] = exts[0];
			info->mExtents[1] = exts[1];
			info->mVSize = vsize;
			info->mBlendFuncDst = bf_dst;
			info->mBlendFuncSrc = bf_src;
			info->mHasGlow = has_glow;
			info->mParticle = TRUE;
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

