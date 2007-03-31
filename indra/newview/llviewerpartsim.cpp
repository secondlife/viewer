/** 
 * @file llviewerpartsim.cpp
 * @brief LLViewerPart class implementation
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewerpartsim.h"

#include "llviewercontrol.h"

#include "llagent.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llviewerpartsource.h"
#include "llviewerregion.h"
#include "llvopartgroup.h"
#include "llworld.h"
#include "pipeline.h"

const S32 MAX_PART_COUNT = 4096;

const F32 PART_SIM_BOX_SIDE = 16.f;
const F32 PART_SIM_BOX_OFFSET = 0.5f*PART_SIM_BOX_SIDE;
const F32 PART_SIM_BOX_RAD = 0.5f*F_SQRT3*PART_SIM_BOX_SIDE;

//static
S32 LLViewerPartSim::sMaxParticleCount = 0;
S32 LLViewerPartSim::sParticleCount = 0;


U32 LLViewerPart::sNextPartID = 1;

F32 calc_desired_size(LLVector3 pos, LLVector2 scale)
{
	F32 desired_size = (pos-gCamera->getOrigin()).magVec();
	desired_size /= 4;
	return llclamp(desired_size, scale.magVec()*0.5f, PART_SIM_BOX_SIDE*2);
}

LLViewerPart::LLViewerPart()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	mPartSourcep = NULL;
}

LLViewerPart::~LLViewerPart()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	mPartSourcep = NULL;
}

LLViewerPart &LLViewerPart::operator=(const LLViewerPart &part)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	mPartID = part.mPartID;
	mFlags = part.mFlags;
	mMaxAge = part.mMaxAge;

	mStartColor = part.mStartColor;
	mEndColor = part.mEndColor;
	mStartScale = part.mStartScale;
	mEndScale = part.mEndScale;

	mPosOffset = part.mPosOffset;
	mParameter = part.mParameter;

	mLastUpdateTime = part.mLastUpdateTime;
	mVPCallback = part.mVPCallback;
	mPartSourcep = part.mPartSourcep;
	
	mImagep = part.mImagep;
	mPosAgent = part.mPosAgent;
	mVelocity = part.mVelocity;
	mAccel = part.mAccel;
	mColor = part.mColor;
	mScale = part.mScale;


	return *this;
}

void LLViewerPart::init(LLViewerPartSource *sourcep, LLViewerImage *imagep, LLVPCallback cb)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	mPartID = LLViewerPart::sNextPartID;
	LLViewerPart::sNextPartID++;
	mFlags = 0x00f;
	mLastUpdateTime = 0.f;
	mMaxAge = 10.f;

	mVPCallback = cb;
	mPartSourcep = sourcep;

	mImagep = imagep;
}


/////////////////////////////
//
// LLViewerPartGroup implementation
//
//


LLViewerPartGroup::LLViewerPartGroup(const LLVector3 &center_agent, const F32 box_side)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	mVOPartGroupp = NULL;
	mUniformParticles = TRUE;

	mRegionp = gWorldPointer->getRegionFromPosAgent(center_agent);
	llassert_always(center_agent.isFinite());
	
	if (!mRegionp)
	{
		//llwarns << "No region at position, using agent region!" << llendl;
		mRegionp = gAgent.getRegion();
	}
	mCenterAgent = center_agent;
	mBoxRadius = F_SQRT3*box_side*0.5f;

	mVOPartGroupp = (LLVOPartGroup *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_PART_GROUP, getRegion());
	mVOPartGroupp->setViewerPartGroup(this);
	mVOPartGroupp->setPositionAgent(getCenterAgent());
	F32 scale = box_side * 0.5f;
	mVOPartGroupp->setScale(LLVector3(scale,scale,scale));
	gPipeline.addObject(mVOPartGroupp);

	LLSpatialGroup* group = mVOPartGroupp->mDrawable->getSpatialGroup();

	LLVector3 center(group->mOctreeNode->getCenter());
	LLVector3 size(group->mOctreeNode->getSize());
	size += LLVector3(0.01f, 0.01f, 0.01f);
	mMinObjPos = center - size;
	mMaxObjPos = center + size;

	static U32 id_seed = 0;
	mID = ++id_seed;
}

LLViewerPartGroup::~LLViewerPartGroup()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	cleanup();
	
	S32 count = (S32) mParticles.size();
	mParticles.clear();
	
	LLViewerPartSim::decPartCount(count);
}

void LLViewerPartGroup::cleanup()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	if (mVOPartGroupp)
	{
		if (!mVOPartGroupp->isDead())
		{
			gObjectList.killObject(mVOPartGroupp);
		}
		mVOPartGroupp = NULL;
	}
}

BOOL LLViewerPartGroup::posInGroup(const LLVector3 &pos, const F32 desired_size)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	if ((pos.mV[VX] < mMinObjPos.mV[VX])
		|| (pos.mV[VY] < mMinObjPos.mV[VY])
		|| (pos.mV[VZ] < mMinObjPos.mV[VZ]))
	{
		return FALSE;
	}

	if ((pos.mV[VX] > mMaxObjPos.mV[VX])
		|| (pos.mV[VY] > mMaxObjPos.mV[VY])
		|| (pos.mV[VZ] > mMaxObjPos.mV[VZ]))
	{
		return FALSE;
	}

	if (desired_size > 0 && 
		(desired_size < mBoxRadius*0.5f ||
		desired_size > mBoxRadius*2.f))
	{
		return FALSE;
	}

	return TRUE;
}


BOOL LLViewerPartGroup::addPart(LLViewerPart* part, F32 desired_size)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	BOOL uniform_part = part->mScale.mV[0] == part->mScale.mV[1] && 
					!(part->mFlags & LLPartData::LL_PART_FOLLOW_VELOCITY_MASK);

	if (!posInGroup(part->mPosAgent, desired_size) ||
		(mUniformParticles && !uniform_part) ||
		(!mUniformParticles && uniform_part))
	{
		return FALSE;
	}

	gPipeline.markRebuild(mVOPartGroupp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	
	mParticles.push_back(part);
	LLViewerPartSim::incPartCount(1);
	return TRUE;
}


void LLViewerPartGroup::removePart(const S32 part_num)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	// Remove the entry for the particle we just deleted.
	mParticles.erase(mParticles.begin() + part_num);
	if (mVOPartGroupp.notNull())
	{
		gPipeline.markRebuild(mVOPartGroupp->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	LLViewerPartSim::decPartCount(1);
}

void LLViewerPartGroup::updateParticles(const F32 dt)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	S32 i, count;
	
	LLVector3 gravity(0.f, 0.f, -9.8f);

	LLViewerRegion *regionp = getRegion();
	count = (S32) mParticles.size();
	for (i = 0; i < count; i++)
	{
		LLVector3 a(0.f, 0.f, 0.f);
		LLViewerPart& part = *((LLViewerPart*) mParticles[i]);

		// Update current time
		const F32 cur_time = part.mLastUpdateTime + dt;
		const F32 frac = cur_time/part.mMaxAge;

		// "Drift" the object based on the source object
		if (part.mFlags & LLPartData::LL_PART_FOLLOW_SRC_MASK)
		{
			part.mPosAgent = part.mPartSourcep->mPosAgent;
			part.mPosAgent += part.mPosOffset;
		}

		// Do a custom callback if we have one...
		if (part.mVPCallback)
		{
			(*part.mVPCallback)(part, dt);
		}

		if (part.mFlags & LLPartData::LL_PART_WIND_MASK)
		{
			LLVector3 tempVel(part.mVelocity);
			part.mVelocity *= 1.f - 0.1f*dt;
			part.mVelocity += 0.1f*dt*regionp->mWind.getVelocity(regionp->getPosRegionFromAgent(part.mPosAgent));
		}

		// Now do interpolation towards a target
		if (part.mFlags & LLPartData::LL_PART_TARGET_POS_MASK)
		{
			F32 remaining = part.mMaxAge - part.mLastUpdateTime;
			F32 step = dt / remaining;

			step = llclamp(step, 0.f, 0.1f);
			step *= 5.f;
			// we want a velocity that will result in reaching the target in the 
			// Interpolate towards the target.
			LLVector3 delta_pos = part.mPartSourcep->mTargetPosAgent - part.mPosAgent;

			delta_pos /= remaining;

			part.mVelocity *= (1.f - step);
			part.mVelocity += step*delta_pos;
		}


		if (part.mFlags & LLPartData::LL_PART_TARGET_LINEAR_MASK)
		{
			LLVector3 delta_pos = part.mPartSourcep->mTargetPosAgent - part.mPartSourcep->mPosAgent;			
			part.mPosAgent = part.mPartSourcep->mPosAgent;
			part.mPosAgent += frac*delta_pos;
			part.mVelocity = delta_pos;
		}
		else
		{
			// Do velocity interpolation
			part.mPosAgent += dt*part.mVelocity;
			part.mPosAgent += 0.5f*dt*dt*part.mAccel;
			part.mVelocity += part.mAccel*dt;
		}

		// Do a bounce test
		if (part.mFlags & LLPartData::LL_PART_BOUNCE_MASK)
		{
			// Need to do point vs. plane check...
			// For now, just check relative to object height...
			F32 dz = part.mPosAgent.mV[VZ] - part.mPartSourcep->mPosAgent.mV[VZ];
			if (dz < 0)
			{
				part.mPosAgent.mV[VZ] += -2.f*dz;
				part.mVelocity.mV[VZ] *= -0.75f;
			}
		}


		// Reset the offset from the source position
		if (part.mFlags & LLPartData::LL_PART_FOLLOW_SRC_MASK)
		{
			part.mPosOffset = part.mPosAgent;
			part.mPosOffset -= part.mPartSourcep->mPosAgent;
		}

		// Do color interpolation
		if (part.mFlags & LLPartData::LL_PART_INTERP_COLOR_MASK)
		{
			part.mColor.setVec(part.mStartColor);
			part.mColor *= 1.f - frac;
			part.mColor.mV[3] *= (1.f - frac)*part.mStartColor.mV[3];
			part.mColor += frac*part.mEndColor;
			part.mColor.mV[3] += frac*part.mEndColor.mV[3];
		}

		// Do scale interpolation
		if (part.mFlags & LLPartData::LL_PART_INTERP_SCALE_MASK)
		{
			part.mScale.setVec(part.mStartScale);
			part.mScale *= 1.f - frac;
			part.mScale += frac*part.mEndScale;
		}

		// Set the last update time to now.
		part.mLastUpdateTime = cur_time;


		// Kill dead particles (either flagged dead, or too old)
		if ((part.mLastUpdateTime > part.mMaxAge) || (LLViewerPart::LL_PART_DEAD_MASK == part.mFlags))
		{
			removePart(i);
			i--;
			count--;
		}
		else 
		{
			F32 desired_size = calc_desired_size(part.mPosAgent, part.mScale);
			if (!posInGroup(part.mPosAgent, desired_size))
			{
				// Transfer particles between groups
				gWorldPointer->mPartSim.put(&part);
				removePart(i);
				i--;
				count--;
			}
		}
	}

	// Kill the viewer object if this particle group is empty
	if (mParticles.empty())
	{
		gObjectList.killObject(mVOPartGroupp);
		mVOPartGroupp = NULL;
	}
}


void LLViewerPartGroup::shift(const LLVector3 &offset)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	mCenterAgent += offset;
	mMinObjPos += offset;
	mMaxObjPos += offset;

	S32 count = (S32) mParticles.size();
	S32 i;
	for (i = 0; i < count; i++)
	{
		mParticles[i]->mPosAgent += offset;
	}
}


//////////////////////////////////
//
// LLViewerPartSim implementation
//
//


LLViewerPartSim::LLViewerPartSim()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	sMaxParticleCount = gSavedSettings.getS32("RenderMaxPartCount");
	static U32 id_seed = 0;
	mID = ++id_seed;
}


LLViewerPartSim::~LLViewerPartSim()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	S32 i;
	S32 count;

	// Kill all of the groups (and particles)
	count = (S32) mViewerPartGroups.size();
	for (i = 0; i < count; i++)
	{
		delete mViewerPartGroups[i];
	}
	mViewerPartGroups.clear();

	// Kill all of the sources 
	mViewerPartSources.clear();
}

BOOL LLViewerPartSim::shouldAddPart()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	if (sParticleCount > 0.75f*sMaxParticleCount)
	{

		F32 frac = (F32)sParticleCount/(F32)sMaxParticleCount;
		frac -= 0.75;
		frac *= 3.f;
		if (ll_frand() < frac)
		{
			// Skip...
			return FALSE;
		}
	}
	if (sParticleCount >= MAX_PART_COUNT)
	{
		return FALSE;
	}

	return TRUE;
}

void LLViewerPartSim::addPart(LLViewerPart* part)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	if (sParticleCount < MAX_PART_COUNT)
	{
		put(part);
	}
}


LLViewerPartGroup *LLViewerPartSim::put(LLViewerPart* part)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	const F32 MAX_MAG = 1000000.f*1000000.f; // 1 million
	if (part->mPosAgent.magVecSquared() > MAX_MAG || !part->mPosAgent.isFinite())
	{
#if 0 && !LL_RELEASE_FOR_DOWNLOAD
		llwarns << "LLViewerPartSim::put Part out of range!" << llendl;
		llwarns << part->mPosAgent << llendl;
#endif
		return NULL;
	}
	
	F32 desired_size = calc_desired_size(part->mPosAgent, part->mScale);

	S32 count = (S32) mViewerPartGroups.size();
	for (S32 i = 0; i < count; i++)
	{
		if (mViewerPartGroups[i]->addPart(part, desired_size))
		{
			// We found a spatial group that we fit into, add us and exit
			return mViewerPartGroups[i];
		}
	}

	// Hmm, we didn't fit in any of the existing spatial groups
	// Create a new one...
	llassert_always(part->mPosAgent.isFinite());
	LLViewerPartGroup *groupp = createViewerPartGroup(part->mPosAgent, desired_size);
	groupp->mUniformParticles = (part->mScale.mV[0] == part->mScale.mV[1] && 
							!(part->mFlags & LLPartData::LL_PART_FOLLOW_VELOCITY_MASK));
	if (!groupp->addPart(part))
	{
		llwarns << "LLViewerPartSim::put - Particle didn't go into its box!" << llendl;
		llinfos << groupp->getCenterAgent() << llendl;
		llinfos << part->mPosAgent << llendl;
		return NULL;
	}
	return groupp;
}

LLViewerPartGroup *LLViewerPartSim::createViewerPartGroup(const LLVector3 &pos_agent, const F32 desired_size)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	//find a box that has a center position divisible by PART_SIM_BOX_SIDE that encompasses
	//pos_agent
	LLViewerPartGroup *groupp = new LLViewerPartGroup(pos_agent, desired_size);
	mViewerPartGroups.push_back(groupp);
	return groupp;
}


void LLViewerPartSim::shift(const LLVector3 &offset)
{
	S32 i;
	S32 count;

	count = (S32) mViewerPartSources.size();
	for (i = 0; i < count; i++)
	{
		mViewerPartSources[i]->mPosAgent += offset;
		mViewerPartSources[i]->mTargetPosAgent += offset;
		mViewerPartSources[i]->mLastUpdatePosAgent += offset;
	}

	count = (S32) mViewerPartGroups.size();
	for (i = 0; i < count; i++)
	{
		mViewerPartGroups[i]->shift(offset);
	}
}

S32 dist_rate_func(F32 distance)
{
	//S32 dist = (S32) sqrtf(distance);
	//dist /= 2;
	//return llmax(dist,1);
	return 1;
}

void LLViewerPartSim::updateSimulation()
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	
	static LLFrameTimer update_timer;

	const F32 dt = update_timer.getElapsedTimeAndResetF32();

 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES)))
	{
		return;
	}

	LLFastTimer ftm(LLFastTimer::FTM_SIMULATE_PARTICLES);

	// Start at a random particle system so the same
	// particle system doesn't always get first pick at the
	// particles.  Theoretically we'd want to do this in distance
	// order or something, but sorting particle sources will be a big
	// pain.
	S32 i;
	S32 count = (S32) mViewerPartSources.size();
	S32 start = (S32)ll_frand((F32)count);
	S32 dir = 1;
	if (ll_frand() > 0.5f)
	{
		dir = -1;
	}

	S32 num_updates = 0;
	for (i = start; num_updates < count;)
	{
		if (i >= count)
		{
			i = 0;
		}
		if (i < 0)
		{
			i = count - 1;
		}

		if (!mViewerPartSources[i]->isDead())
		{
			LLViewerObject* source_object = mViewerPartSources[i]->mSourceObjectp;
			if (source_object && source_object->mDrawable.notNull())
			{
                S32 dist = dist_rate_func(source_object->mDrawable->mDistanceWRTCamera);
				if ((LLDrawable::getCurrentFrame()+mViewerPartSources[i]->mID)%dist == 0)
				{
					mViewerPartSources[i]->update(dt*dist);
				}
			}
			else
			{
				mViewerPartSources[i]->update(dt);
			}
		}

		if (mViewerPartSources[i]->isDead())
		{
			mViewerPartSources.erase(mViewerPartSources.begin() + i);
			count--;
		}
		else
        {
			 i += dir;
        }
		num_updates++;
	}


	count = (S32) mViewerPartGroups.size();
	for (i = 0; i < count; i++)
	{
		LLViewerObject* vobj = mViewerPartGroups[i]->mVOPartGroupp;

		S32 dist = vobj && !vobj->mDrawable->isState(LLDrawable::IN_REBUILD_Q1) ? 
				dist_rate_func(vobj->mDrawable->mDistanceWRTCamera) : 1;
		if (vobj)
		{
			LLSpatialGroup* group = vobj->mDrawable->getSpatialGroup();
			if (group && !group->isVisible()) // && !group->isState(LLSpatialGroup::OBJECT_DIRTY))
			{
				dist *= 8;
			}
		}

		if ((LLDrawable::getCurrentFrame()+mViewerPartGroups[i]->mID)%dist == 0)
		{
			if (vobj)
			{
				gPipeline.markRebuild(vobj->mDrawable, LLDrawable::REBUILD_ALL, TRUE);
			}
			mViewerPartGroups[i]->updateParticles(dt*dist);
			if (!mViewerPartGroups[i]->getCount())
			{
				delete mViewerPartGroups[i];
				mViewerPartGroups.erase(mViewerPartGroups.begin() + i);
				i--;
				count--;
			}
		}
	}
	//llinfos << "Particles: " << sParticleCount << llendl;
}


void LLViewerPartSim::addPartSource(LLViewerPartSource *sourcep)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	if (!sourcep)
	{
		llwarns << "Null part source!" << llendl;
		return;
	}
	mViewerPartSources.push_back(sourcep);
}


void LLViewerPartSim::cleanupRegion(LLViewerRegion *regionp)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	for (group_list_t::iterator i = mViewerPartGroups.begin(); i != mViewerPartGroups.end(); )
	{
		group_list_t::iterator iter = i++;

		if ((*iter)->getRegion() == regionp)
		{
			i = mViewerPartGroups.erase(iter);			
		}
	}
}

void LLViewerPartSim::cleanMutedParticles(const LLUUID& task_id)
{
	LLMemType mt(LLMemType::MTYPE_PARTICLES);
	for (source_list_t::iterator i = mViewerPartSources.begin(); i != mViewerPartSources.end(); )
	{
		source_list_t::iterator iter = i++;

		if ((*iter)->getOwnerUUID() == task_id)
		{
			i = mViewerPartSources.erase(iter);			
		}
	}
}
