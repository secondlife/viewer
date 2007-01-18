/** 
 * @file llviewerpartsource.cpp
 * @brief LLViewerPartSource class implementation
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include "llviewerpartsource.h"

#include "llviewercontrol.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llviewerimagelist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llworld.h"

LLViewerPartSource::LLViewerPartSource(const U32 type) :
	mType(type),
	mOwnerUUID(LLUUID::null)
{
	mLastUpdateTime = 0.f;
	mLastPartTime = 0.f;
	mIsDead = FALSE;
}

void LLViewerPartSource::setDead()
{
	mIsDead = TRUE;
}


void LLViewerPartSource::updatePart(LLViewerPart &part, const F32 dt)
{
}

void LLViewerPartSource::update(const F32 dt)
{
	llerrs << "Creating default part source!" << llendl;
}



LLViewerPartSourceScript::LLViewerPartSourceScript(LLViewerObject *source_objp) :
	LLViewerPartSource(LL_PART_SOURCE_SCRIPT)
{
	llassert(source_objp);
	mSourceObjectp = source_objp;
	mPosAgent = mSourceObjectp->getPositionAgent();
	LLUUID id;
	id.set( gViewerArt.getString("pixiesmall.tga") );
	mImagep = gImageList.getImage(id);
	mImagep->bind();
	mImagep->setClamp(TRUE, TRUE);
}


void LLViewerPartSourceScript::setDead()
{
	mIsDead = TRUE;
	mSourceObjectp = NULL;
	mTargetObjectp = NULL;
}



void LLViewerPartSourceScript::update(const F32 dt)
{
	F32 old_update_time = mLastUpdateTime;
	mLastUpdateTime += dt;

	F32 dt_update = mLastUpdateTime - mLastPartTime;

	// Update this for objects which have the follow flag set...
	if (!mSourceObjectp.isNull())
	{
		if (mSourceObjectp->isDead())
		{
			mSourceObjectp = NULL;
		}
		else if (mSourceObjectp->mDrawable.notNull())
		{
			mPosAgent = mSourceObjectp->getRenderPosition();
		}
	}

	if (mTargetObjectp.isNull() 
		&& mPartSysData.mTargetUUID.notNull())
	{
		//
		// Hmm, missing object, let's see if we can find it...
		//

		LLViewerObject *target_objp = gObjectList.findObject(mPartSysData.mTargetUUID);
		setTargetObject(target_objp);
	}

	if (!mTargetObjectp.isNull())
	{
		if (mTargetObjectp->isDead())
		{
			mTargetObjectp = NULL;
		}
		else if (mTargetObjectp->mDrawable.notNull())
		{
			mTargetPosAgent = mTargetObjectp->getRenderPosition();
		}
	}

	if (!mTargetObjectp)
	{
		mTargetPosAgent = mPosAgent;
	}

	if (mPartSysData.mMaxAge && ((mPartSysData.mStartAge + mLastUpdateTime + dt_update) > mPartSysData.mMaxAge))
	{
		// Kill particle source because it has outlived its max age...
		setDead();
		return;
	}

	BOOL first_run = FALSE;
	if (old_update_time <= 0.f)
	{
		first_run = TRUE;
	}

	F32 max_time = llmax(1.f, 10.f*mPartSysData.mBurstRate);
	dt_update = llmin(max_time, dt_update);
	while ((dt_update > mPartSysData.mBurstRate) || first_run)
	{
		first_run = FALSE;
		LLViewerPart part;

		part.init(this, mImagep, NULL);
		part.mFlags = mPartSysData.mPartData.mFlags;
		part.mMaxAge = mPartSysData.mPartData.mMaxAge;
		part.mStartColor = mPartSysData.mPartData.mStartColor;
		part.mEndColor = mPartSysData.mPartData.mEndColor;
		part.mColor = part.mStartColor;

		part.mStartScale = mPartSysData.mPartData.mStartScale;
		part.mEndScale = mPartSysData.mPartData.mEndScale;
		part.mScale = part.mStartScale;

		part.mAccel = mPartSysData.mPartAccel;

		// Update the rotation of the particle source by the angular velocity
		// First check to see if there is still an angular velocity.
		F32 angular_velocity_mag = mPartSysData.mAngularVelocity.magVec();
		if (angular_velocity_mag != 0.0f)
		{
			F32 av_angle = dt * angular_velocity_mag;
			LLQuaternion dquat(av_angle, mPartSysData.mAngularVelocity);
			mRotation *= dquat;
		}
		else
		{
			// No angular velocity.  Reset our rotation.
			mRotation.setQuat(0, 0, 0);
		}

		if (gWorldPointer->mPartSim.aboveParticleLimit())
		{
			// Don't bother doing any more updates if we're above the particle limit,
			// just give up.
			mLastPartTime = mLastUpdateTime;
			break;
		}

		S32 i;
		for (i = 0; i < mPartSysData.mBurstPartCount; i++)
		{
			if (!gWorldPointer->mPartSim.shouldAddPart())
			{
				// Particle simulation says we have too many particles, skip all this
				continue;
			}

			if (mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_DROP)
			{
				part.mPosAgent = mPosAgent;
				part.mVelocity.setVec(0.f, 0.f, 0.f);
			}
			else if (mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_EXPLODE)
			{
				part.mPosAgent = mPosAgent;
				LLVector3 part_dir_vector;

				F32 mvs;
				do
				{
					part_dir_vector.mV[VX] = ll_frand(2.f) - 1.f;
					part_dir_vector.mV[VY] = ll_frand(2.f) - 1.f;
					part_dir_vector.mV[VZ] = ll_frand(2.f) - 1.f;
					mvs = part_dir_vector.magVecSquared();
				}
				while ((mvs > 1.f) || (mvs < 0.01f));

				part_dir_vector.normVec();
				part.mPosAgent += mPartSysData.mBurstRadius*part_dir_vector;
				part.mVelocity = part_dir_vector;
				F32 speed = mPartSysData.mBurstSpeedMin + ll_frand(mPartSysData.mBurstSpeedMax - mPartSysData.mBurstSpeedMin);
				part.mVelocity *= speed;
			}
			else if (mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_ANGLE
				|| mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE)
			{
				part.mPosAgent = mPosAgent;
				
				// original implemenetation for part_dir_vector was just:					
				LLVector3 part_dir_vector(0.0, 0.0, 1.0);

				// params from the script...
				// outer = outer cone angle
				// inner = inner cone angle
				//		between outer and inner there will be particles
				F32 innerAngle = mPartSysData.mInnerAngle;
				F32 outerAngle = mPartSysData.mOuterAngle;

				// generate a random angle within the given space...
				F32 angle = innerAngle + ll_frand(outerAngle - innerAngle);

				// split which side it will go on randomly...
				if (ll_frand() < 0.5) 
				{
					angle = -angle;
				}
				
				// Both patterns rotate around the x-axis first:
				part_dir_vector.rotVec(angle, 1.0, 0.0, 0.0);

				// If this is a cone pattern, rotate again to create the cone.
				if (mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE)
				{
					part_dir_vector.rotVec(ll_frand(4*F_PI), 0.0, 0.0, 1.0);
				}
					
				// Only apply this rotation if using the deprecated angles. 
				if (! (mPartSysData.mFlags & LLPartSysData::LL_PART_USE_NEW_ANGLE))
				{
					// Deprecated...
					part_dir_vector.rotVec(outerAngle, 1.0, 0.0, 0.0);
				}

				if (mSourceObjectp)
				{
					part_dir_vector = part_dir_vector * mSourceObjectp->getRenderRotation();
				}
				part_dir_vector = part_dir_vector * mRotation;

				part.mPosAgent += mPartSysData.mBurstRadius*part_dir_vector;

				part.mVelocity = part_dir_vector;

				F32 speed = mPartSysData.mBurstSpeedMin + ll_frand(mPartSysData.mBurstSpeedMax - mPartSysData.mBurstSpeedMin);
				part.mVelocity *= speed;
			}
			else
			{
				part.mPosAgent = mPosAgent;
				part.mVelocity.setVec(0.f, 0.f, 0.f);
				//llwarns << "Unknown source pattern " << (S32)mPartSysData.mPattern << llendl;
			}

			gWorldPointer->mPartSim.addPart(part);
		}

		mLastPartTime = mLastUpdateTime;
		dt_update -= mPartSysData.mBurstRate;
	}
}

// static
LLViewerPartSourceScript *LLViewerPartSourceScript::unpackPSS(LLViewerObject *source_objp, LLViewerPartSourceScript *pssp, const S32 block_num)
{
	if (!pssp)
	{
		if (LLPartSysData::isNullPS(block_num))
		{
			return NULL;
		}
		LLViewerPartSourceScript *new_pssp = new LLViewerPartSourceScript(source_objp);
		if (!new_pssp->mPartSysData.unpackBlock(block_num))
		{
			return NULL;
		}
		if (new_pssp->mPartSysData.mTargetUUID.notNull())
		{
			LLViewerObject *target_objp = gObjectList.findObject(new_pssp->mPartSysData.mTargetUUID);
			new_pssp->setTargetObject(target_objp);
		}
		return new_pssp;
	}
	else
	{
		if (LLPartSysData::isNullPS(block_num))
		{
			return NULL;
		}

		if (!pssp->mPartSysData.unpackBlock(block_num))
		{
			return NULL;
		}
		if (pssp->mPartSysData.mTargetUUID.notNull())
		{
			LLViewerObject *target_objp = gObjectList.findObject(pssp->mPartSysData.mTargetUUID);
			pssp->setTargetObject(target_objp);
		}
		return pssp;
	}
}


LLViewerPartSourceScript *LLViewerPartSourceScript::unpackPSS(LLViewerObject *source_objp, LLViewerPartSourceScript *pssp, LLDataPacker &dp)
{
	if (!pssp)
	{
		LLViewerPartSourceScript *new_pssp = new LLViewerPartSourceScript(source_objp);
		if (!new_pssp->mPartSysData.unpack(dp))
		{
			return NULL;
		}
		if (new_pssp->mPartSysData.mTargetUUID.notNull())
		{
			LLViewerObject *target_objp = gObjectList.findObject(new_pssp->mPartSysData.mTargetUUID);
			new_pssp->setTargetObject(target_objp);
		}
		return new_pssp;
	}
	else
	{
		if (!pssp->mPartSysData.unpack(dp))
		{
			return NULL;
		}
		if (pssp->mPartSysData.mTargetUUID.notNull())
		{
			LLViewerObject *target_objp = gObjectList.findObject(pssp->mPartSysData.mTargetUUID);
			pssp->setTargetObject(target_objp);
		}
		return pssp;
	}
}

void LLViewerPartSourceScript::setImage(LLViewerImage *imagep)
{
	mImagep = imagep;
}

void LLViewerPartSourceScript::setTargetObject(LLViewerObject *objp)
{
	mTargetObjectp = objp;
}





LLViewerPartSourceSpiral::LLViewerPartSourceSpiral(const LLVector3 &pos) :
	LLViewerPartSource(LL_PART_SOURCE_CHAT)
{
	mPosAgent = pos;
}

void LLViewerPartSourceSpiral::setDead()
{
	mIsDead = TRUE;
	mSourceObjectp = NULL;
}


void LLViewerPartSourceSpiral::updatePart(LLViewerPart &part, const F32 dt)
{
	F32 frac = part.mLastUpdateTime/part.mMaxAge;

	LLVector3 center_pos;
	LLViewerPartSource *ps = (LLViewerPartSource*)part.mPartSourcep;
	LLViewerPartSourceSpiral *pss = (LLViewerPartSourceSpiral *)ps;
	if (!pss->mSourceObjectp.isNull() && !pss->mSourceObjectp->mDrawable.isNull())
	{
		part.mPosAgent = pss->mSourceObjectp->getRenderPosition();
	}
	else
	{
		part.mPosAgent = pss->mPosAgent;
	}
	F32 x = sin(F_TWO_PI*frac + part.mParameter);
	F32 y = cos(F_TWO_PI*frac + part.mParameter);

	part.mPosAgent.mV[VX] += x;
	part.mPosAgent.mV[VY] += y;
	part.mPosAgent.mV[VZ] += -0.5f + frac;
}


void LLViewerPartSourceSpiral::update(const F32 dt)
{
	if (!mImagep)
	{
		LLUUID id;
		id.set( gViewerArt.getString("pixiesmall.tga") );
		mImagep = gImageList.getImage(id);
	}

	const F32 RATE = 0.025f;

	mLastUpdateTime += dt;

	F32 dt_update = mLastUpdateTime - mLastPartTime;
	F32 max_time = llmax(1.f, 10.f*RATE);
	dt_update = llmin(max_time, dt_update);

	if (dt_update > RATE)
	{
		mLastPartTime = mLastUpdateTime;
		if (!gWorldPointer->mPartSim.shouldAddPart())
		{
			// Particle simulation says we have too many particles, skip all this
			return;
		}

		if (!mSourceObjectp.isNull() && !mSourceObjectp->mDrawable.isNull())
		{
			mPosAgent = mSourceObjectp->getRenderPosition();
		}
		LLViewerPart part;
		part.init(this, mImagep, updatePart);
		part.mStartColor = mColor;
		part.mEndColor = mColor;
		part.mEndColor.mV[3] = 0.f;
		part.mPosAgent = mPosAgent;
		part.mMaxAge = 1.f;
		part.mFlags = LLViewerPart::LL_PART_INTERP_COLOR_MASK;
		part.mLastUpdateTime = 0.f;
		part.mScale.mV[0] = 0.25f;
		part.mScale.mV[1] = 0.25f;
		part.mParameter = ll_frand(F_TWO_PI);

		gWorldPointer->mPartSim.addPart(part);
	}
}

void LLViewerPartSourceSpiral::setSourceObject(LLViewerObject *objp)
{
	mSourceObjectp = objp;
}

void LLViewerPartSourceSpiral::setColor(const LLColor4 &color)
{
	mColor = color;
}





LLViewerPartSourceBeam::LLViewerPartSourceBeam() :
	LLViewerPartSource(LL_PART_SOURCE_BEAM)
{
}

LLViewerPartSourceBeam::~LLViewerPartSourceBeam()
{
}

void LLViewerPartSourceBeam::setDead()
{
	mIsDead = TRUE;
	mSourceObjectp = NULL;
	mTargetObjectp = NULL;
}

void LLViewerPartSourceBeam::setColor(const LLColor4 &color)
{
	mColor = color;
}


void LLViewerPartSourceBeam::updatePart(LLViewerPart &part, const F32 dt)
{
	F32 frac = part.mLastUpdateTime/part.mMaxAge;

	LLViewerPartSource *ps = (LLViewerPartSource*)part.mPartSourcep;
	LLViewerPartSourceBeam *psb = (LLViewerPartSourceBeam *)ps;
	if (psb->mSourceObjectp.isNull())
	{
		part.mFlags = LLPartData::LL_PART_DEAD_MASK;
		return;
	}

	LLVector3 source_pos_agent;
	LLVector3 target_pos_agent;
	if (!psb->mSourceObjectp.isNull() && !psb->mSourceObjectp->mDrawable.isNull())
	{
		if (psb->mSourceObjectp->isAvatar())
		{
			LLViewerObject *objp = psb->mSourceObjectp;
			LLVOAvatar *avp = (LLVOAvatar *)objp;
			source_pos_agent = avp->mWristLeftp->getWorldPosition();
		}
		else
		{
			source_pos_agent = psb->mSourceObjectp->getRenderPosition();
		}
	}
	if (!psb->mTargetObjectp.isNull() && !psb->mTargetObjectp->mDrawable.isNull())
	{
		target_pos_agent = psb->mTargetObjectp->getRenderPosition();
	}

	part.mPosAgent = (1.f - frac) * source_pos_agent;
	if (psb->mTargetObjectp.isNull())
	{
		part.mPosAgent += frac * (gAgent.getPosAgentFromGlobal(psb->mLKGTargetPosGlobal));
	}
	else
	{
		part.mPosAgent += frac * target_pos_agent;
	}
}


void LLViewerPartSourceBeam::update(const F32 dt)
{

	const F32 RATE = 0.025f;

	mLastUpdateTime += dt;

	if (!mSourceObjectp.isNull() && !mSourceObjectp->mDrawable.isNull())
	{
		if (mSourceObjectp->isAvatar())
		{
			LLViewerObject *objp = mSourceObjectp;
			LLVOAvatar *avp = (LLVOAvatar *)objp;
			mPosAgent = avp->mWristLeftp->getWorldPosition();
		}
		else
		{
			mPosAgent = mSourceObjectp->getRenderPosition();
		}
	}

	if (!mTargetObjectp.isNull() && !mTargetObjectp->mDrawable.isNull())
	{
		mTargetPosAgent = mTargetObjectp->getRenderPosition();
	}
	else if (!mLKGTargetPosGlobal.isExactlyZero())
	{
		mTargetPosAgent = gAgent.getPosAgentFromGlobal(mLKGTargetPosGlobal);
	}

	F32 dt_update = mLastUpdateTime - mLastPartTime;
	F32 max_time = llmax(1.f, 10.f*RATE);
	dt_update = llmin(max_time, dt_update);

	if (dt_update > RATE)
	{
		mLastPartTime = mLastUpdateTime;
		if (!gWorldPointer->mPartSim.shouldAddPart())
		{
			// Particle simulation says we have too many particles, skip all this
			return;
		}

		if (!mImagep)
		{
			LLUUID id;
			id.set( gViewerArt.getString("pixiesmall.tga") );
			mImagep = gImageList.getImage(id);
		}

		LLViewerPart part;
		part.init(this, mImagep, NULL);

		part.mFlags = LLPartData::LL_PART_INTERP_COLOR_MASK |
						LLPartData::LL_PART_INTERP_SCALE_MASK |
						LLPartData::LL_PART_TARGET_POS_MASK |
						LLPartData::LL_PART_FOLLOW_VELOCITY_MASK;
		part.mMaxAge = 0.5f;
		part.mStartColor = mColor;
		part.mEndColor = part.mStartColor;
		part.mEndColor.mV[3] = 0.4f;
		part.mColor = part.mStartColor;

		part.mStartScale = LLVector2(0.1f, 0.1f);
		part.mEndScale = LLVector2(0.1f, 0.1f);
		part.mScale = part.mStartScale;

		part.mPosAgent = mPosAgent;
		part.mVelocity = mTargetPosAgent - mPosAgent;

		gWorldPointer->mPartSim.addPart(part);
	}
}

void LLViewerPartSourceBeam::setSourceObject(LLViewerObject* objp)
{
	mSourceObjectp = objp;
}

void LLViewerPartSourceBeam::setTargetObject(LLViewerObject* objp)
{
	mTargetObjectp = objp;
}




LLViewerPartSourceChat::LLViewerPartSourceChat(const LLVector3 &pos) :
	LLViewerPartSource(LL_PART_SOURCE_SPIRAL)
{
	mPosAgent = pos;
}

void LLViewerPartSourceChat::setDead()
{
	mIsDead = TRUE;
	mSourceObjectp = NULL;
}


void LLViewerPartSourceChat::updatePart(LLViewerPart &part, const F32 dt)
{
	F32 frac = part.mLastUpdateTime/part.mMaxAge;

	LLVector3 center_pos;
	LLViewerPartSource *ps = (LLViewerPartSource*)part.mPartSourcep;
	LLViewerPartSourceChat *pss = (LLViewerPartSourceChat *)ps;
	if (!pss->mSourceObjectp.isNull() && !pss->mSourceObjectp->mDrawable.isNull())
	{
		part.mPosAgent = pss->mSourceObjectp->getRenderPosition();
	}
	else
	{
		part.mPosAgent = pss->mPosAgent;
	}
	F32 x = sin(F_TWO_PI*frac + part.mParameter);
	F32 y = cos(F_TWO_PI*frac + part.mParameter);

	part.mPosAgent.mV[VX] += x;
	part.mPosAgent.mV[VY] += y;
	part.mPosAgent.mV[VZ] += -0.5f + frac;
}


void LLViewerPartSourceChat::update(const F32 dt)
{
	if (!mImagep)
	{
		LLUUID id;
		id.set( gViewerArt.getString("pixiesmall.tga") );
		mImagep = gImageList.getImage(id);
	}


	const F32 RATE = 0.025f;

	mLastUpdateTime += dt;

	if (mLastUpdateTime > 2.f)
	{
		// Kill particle source because it has outlived its max age...
		setDead();
		return;
	}

	F32 dt_update = mLastUpdateTime - mLastPartTime;

	// Clamp us to generating at most one second's worth of particles on a frame.
	F32 max_time = llmax(1.f, 10.f*RATE);
	dt_update = llmin(max_time, dt_update);

	if (dt_update > RATE)
	{
		mLastPartTime = mLastUpdateTime;
		if (!gWorldPointer->mPartSim.shouldAddPart())
		{
			// Particle simulation says we have too many particles, skip all this
			return;
		}

		if (!mSourceObjectp.isNull() && !mSourceObjectp->mDrawable.isNull())
		{
			mPosAgent = mSourceObjectp->getRenderPosition();
		}
		LLViewerPart part;
		part.init(this, mImagep, updatePart);
		part.mStartColor = mColor;
		part.mEndColor = mColor;
		part.mEndColor.mV[3] = 0.f;
		part.mPosAgent = mPosAgent;
		part.mMaxAge = 1.f;
		part.mFlags = LLViewerPart::LL_PART_INTERP_COLOR_MASK;
		part.mLastUpdateTime = 0.f;
		part.mScale.mV[0] = 0.25f;
		part.mScale.mV[1] = 0.25f;
		part.mParameter = ll_frand(F_TWO_PI);

		gWorldPointer->mPartSim.addPart(part);
	}
}

void LLViewerPartSourceChat::setSourceObject(LLViewerObject *objp)
{
	mSourceObjectp = objp;
}

void LLViewerPartSourceChat::setColor(const LLColor4 &color)
{
	mColor = color;
}

