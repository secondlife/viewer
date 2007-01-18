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
#include "llviewerobjectlist.h"
#include "llviewerpartsource.h"
#include "llviewerregion.h"
#include "llvopartgroup.h"
#include "llworld.h"
#include "pipeline.h"

const S32 MAX_PART_COUNT = 4096;

const F32 PART_SIM_BOX_SIDE = 32.f;
const F32 PART_SIM_BOX_OFFSET = 0.5f*PART_SIM_BOX_SIDE;
const F32 PART_SIM_BOX_RAD = 0.5f*F_SQRT3*PART_SIM_BOX_SIDE;

//static
S32 LLViewerPartSim::sMaxParticleCount = 0;
S32 LLViewerPartSim::sParticleCount = 0;


U32 LLViewerPart::sNextPartID = 1;

LLViewerPart::LLViewerPart()
{
	mPartSourcep = NULL;
}

LLViewerPart::~LLViewerPart()
{
	mPartSourcep = NULL;
}

LLViewerPart &LLViewerPart::operator=(const LLViewerPart &part)
{
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
	mVOPartGroupp = NULL;
	mRegionp = gWorldPointer->getRegionFromPosAgent(center_agent);
	if (!mRegionp)
	{
		//llwarns << "No region at position, using agent region!" << llendl;
		mRegionp = gAgent.getRegion();
	}
	mCenterAgent = center_agent;
	mBoxRadius = F_SQRT3*box_side*0.5f;

	LLVector3 rad_vec(box_side*0.5f, box_side*0.5f, box_side*0.5f);
	rad_vec += LLVector3(0.001f, 0.001f, 0.001f);
	mMinObjPos = mCenterAgent - rad_vec;
	mMaxObjPos = mCenterAgent + rad_vec;
}

LLViewerPartGroup::~LLViewerPartGroup()
{
	cleanup();
	S32 count = mParticles.count();
	S32 i;

	for (i = 0; i < count; i++)
	{
		mParticles[i].mPartSourcep = NULL;
	}
	mParticles.reset();
	LLViewerPartSim::decPartCount(count);
}

void LLViewerPartGroup::cleanup()
{
	if (mVOPartGroupp)
	{
		if (!mVOPartGroupp->isDead())
		{
			gObjectList.killObject(mVOPartGroupp);
		}
		mVOPartGroupp = NULL;
	}
}

BOOL LLViewerPartGroup::posInGroup(const LLVector3 &pos)
{
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

	return TRUE;
}


BOOL LLViewerPartGroup::addPart(LLViewerPart &part)
{
	if (!posInGroup(part.mPosAgent) || 
		(mVOPartGroupp.notNull() && (part.mImagep != mVOPartGroupp->getTEImage(0))))
	{
		return FALSE;
	}

	if (!mVOPartGroupp)
	{
		mVOPartGroupp = (LLVOPartGroup *)gObjectList.createObjectViewer(LLViewerObject::LL_VO_PART_GROUP, getRegion());
		mVOPartGroupp->setViewerPartGroup(this);
		mVOPartGroupp->setPositionAgent(getCenterAgent());
		mVOPartGroupp->setScale(LLVector3(PART_SIM_BOX_SIDE, PART_SIM_BOX_SIDE, PART_SIM_BOX_SIDE));
		mVOPartGroupp->setTEImage(0, part.mImagep);
		gPipeline.addObject(mVOPartGroupp);
	}
	
	mParticles.put(part);
	LLViewerPartSim::incPartCount(1);
	return TRUE;
}


void LLViewerPartGroup::removePart(const S32 part_num)
{
	// Remove the entry for the particle we just deleted.
	LLPointer<LLViewerPartSource> ps = mParticles[mParticles.count() - 1].mPartSourcep;

	mParticles[mParticles.count() - 1].mPartSourcep = NULL;
	mParticles.remove(part_num);
	if (part_num < mParticles.count())
	{
		mParticles[part_num].mPartSourcep = ps;
	}

	LLViewerPartSim::decPartCount(1);
}


void LLViewerPartGroup::updateParticles(const F32 dt)
{
	S32 i, count;


	LLVector3 gravity(0.f, 0.f, -9.8f);

	LLViewerRegion *regionp = getRegion();
	count = mParticles.count();
	for (i = 0; i < count; i++)
	{
		LLVector3 a(0.f, 0.f, 0.f);
		LLViewerPart &part = mParticles[i];

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
			//part.mPosAgent *= 1.f - to_target_frac;
			//part.mPosAgent += to_target_frac*part.mPartSourcep->mTargetPosAgent;
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
		else if (!posInGroup(part.mPosAgent))
		{
			// Transfer particles between groups
			gWorldPointer->mPartSim.put(part);
			removePart(i);
			i--;
			count--;
		}
	}

	// Kill the viewer object if this particle group is empty
	if (!mParticles.count())
	{
		gObjectList.killObject(mVOPartGroupp);
		mVOPartGroupp = NULL;
	}
}


void LLViewerPartGroup::shift(const LLVector3 &offset)
{
	mCenterAgent += offset;
	mMinObjPos += offset;
	mMaxObjPos += offset;

	S32 count = mParticles.count();
	S32 i;
	for (i = 0; i < count; i++)
	{
		mParticles[i].mPosAgent += offset;
	}
}


//////////////////////////////////
//
// LLViewerPartSim implementation
//
//


LLViewerPartSim::LLViewerPartSim()
{
	sMaxParticleCount = gSavedSettings.getS32("RenderMaxPartCount");
}


LLViewerPartSim::~LLViewerPartSim()
{
	S32 i;
	S32 count;

	// Kill all of the groups (and particles)
	count = mViewerPartGroups.count();
	for (i = 0; i < count; i++)
	{
		delete mViewerPartGroups[i];
	}
	mViewerPartGroups.reset();

	// Kill all of the sources 
	count = mViewerPartSources.count();
	for (i = 0; i < count; i++)
	{
		mViewerPartSources[i] = NULL;
	}
	mViewerPartSources.reset();
}

BOOL LLViewerPartSim::shouldAddPart()
{
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

void LLViewerPartSim::addPart(LLViewerPart &part)
{
	if (sParticleCount < MAX_PART_COUNT)
	{
		put(part);
	}
}

LLViewerPartGroup *LLViewerPartSim::put(LLViewerPart &part)
{
	const F32 MAX_MAG = 1000000.f*1000000.f; // 1 million
	if (part.mPosAgent.magVecSquared() > MAX_MAG)
	{
#ifndef LL_RELEASE_FOR_DOWNLOAD
		llwarns << "LLViewerPartSim::put Part out of range!" << llendl;
		llwarns << part.mPosAgent << llendl;
#endif
		return NULL;
	}

	S32 i;
	S32 count;

	count = mViewerPartGroups.count();
	for (i = 0; i < count; i++)
	{
		if (mViewerPartGroups[i]->addPart(part))
		{
			// We found a spatial group that we fit into, add us and exit
			return mViewerPartGroups[i];
		}
	}

	// Hmm, we didn't fit in any of the existing spatial groups
	// Create a new one...
	LLViewerPartGroup *groupp = createViewerPartGroup(part.mPosAgent);
	if (!groupp->addPart(part))
	{
		llwarns << "LLViewerPartSim::put - Particle didn't go into its box!" << llendl;
		llinfos << groupp->getCenterAgent() << llendl;
		llinfos << part.mPosAgent << llendl;
		return NULL;
	}
	return groupp;
}

LLViewerPartGroup *LLViewerPartSim::createViewerPartGroup(const LLVector3 &pos_agent)
{
	F32 x_origin = ((S32)(pos_agent.mV[VX]/PART_SIM_BOX_SIDE))*PART_SIM_BOX_SIDE;
	if (x_origin > pos_agent.mV[VX])
	{
		x_origin -= PART_SIM_BOX_SIDE;
	}

	F32 y_origin = ((S32)(pos_agent.mV[VY]/PART_SIM_BOX_SIDE))*PART_SIM_BOX_SIDE;
	if (y_origin > pos_agent.mV[VY])
	{
		y_origin -= PART_SIM_BOX_SIDE;
	}

	F32 z_origin = ((S32)(pos_agent.mV[VZ]/PART_SIM_BOX_SIDE))*PART_SIM_BOX_SIDE;
	if (z_origin > pos_agent.mV[VZ])
	{
		z_origin -= PART_SIM_BOX_SIDE;
	}

	LLVector3 group_center(x_origin + PART_SIM_BOX_OFFSET,
							y_origin + PART_SIM_BOX_OFFSET,
							z_origin + PART_SIM_BOX_OFFSET);

	LLViewerPartGroup *groupp = new LLViewerPartGroup(group_center, PART_SIM_BOX_SIDE);
	mViewerPartGroups.put(groupp);
	return groupp;
}


void LLViewerPartSim::shift(const LLVector3 &offset)
{
	S32 i;
	S32 count;

	count = mViewerPartSources.count();
	for (i = 0; i < count; i++)
	{
		mViewerPartSources[i]->mPosAgent += offset;
		mViewerPartSources[i]->mTargetPosAgent += offset;
		mViewerPartSources[i]->mLastUpdatePosAgent += offset;
	}

	count = mViewerPartGroups.count();
	for (i = 0; i < count; i++)
	{
		mViewerPartGroups[i]->shift(offset);
	}
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

	// Start at a random particle system so the same
	// particle system doesn't always get first pick at the
	// particles.  Theoretically we'd want to do this in distance
	// order or something, but sorting particle sources will be a big
	// pain.
	S32 i;
	S32 count = mViewerPartSources.count();
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
			mViewerPartSources[i]->update(dt);
		}

		if (mViewerPartSources[i]->isDead())
		{
			mViewerPartSources.remove(i);
			count--;
		}
		else
        {
			 i += dir;
        }
		num_updates++;
	}


	count = mViewerPartGroups.count();
	for (i = 0; i < count; i++)
	{
		mViewerPartGroups[i]->updateParticles(dt);
		if (!mViewerPartGroups[i]->getCount())
		{
			delete mViewerPartGroups[i];
			mViewerPartGroups.remove(i);
			i--;
			count--;
		}
	}
	//llinfos << "Particles: " << sParticleCount << llendl;
}


void LLViewerPartSim::addPartSource(LLViewerPartSource *sourcep)
{
	if (!sourcep)
	{
		llwarns << "Null part source!" << llendl;
		return;
	}
	mViewerPartSources.put(sourcep);
}


void LLViewerPartSim::cleanupRegion(LLViewerRegion *regionp)
{
	S32 i, count;
	count = mViewerPartGroups.count();
	for (i = 0; i < count; i++)
	{
		if (mViewerPartGroups[i]->getRegion() == regionp)
		{
			delete mViewerPartGroups[i];
			mViewerPartGroups.remove(i);
			i--;
			count--;
		}
	}
}

void LLViewerPartSim::cleanMutedParticles(const LLUUID& task_id)
{
	S32 i;
	S32 count = mViewerPartSources.count();
	for (i = 0; i < count; ++i)
	{
		if (mViewerPartSources[i]->getOwnerUUID() == task_id)
		{
			mViewerPartSources.remove(i);
			i--;
			count--;
		}
	}
}
