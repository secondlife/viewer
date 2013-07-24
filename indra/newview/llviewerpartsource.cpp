/** 
 * @file llviewerpartsource.cpp
 * @brief LLViewerPartSource class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#include "llviewerpartsource.h"

#include "llviewercontrol.h"
#include "llrender.h"

#include "llagent.h"
#include "lldrawable.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llworld.h"
#include "pipeline.h"

LLViewerPartSource::LLViewerPartSource(const U32 type) :
	mType(type),
	mOwnerUUID(LLUUID::null),
	mPartFlags(0)
{
	mLastUpdateTime = 0.f;
	mLastPartTime = 0.f;
	mIsDead = FALSE;
	mIsSuspended = FALSE;
	static U32 id_seed = 0;
	mID = ++id_seed;

	mDelay = 0 ;
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

LLUUID LLViewerPartSource::getImageUUID() const
{
	LLViewerTexture* imagep = mImagep;
	if(imagep)
	{
		return imagep->getID();
	}
	return LLUUID::null;
}
void LLViewerPartSource::setStart()
{
	//cancel delaying to start a new added particle source, because some particle source just emits for a short time.
	//however, canceling this might cause overall particle emmitting fluctuate for a while because the new added source jumps to 
	//the current particle emmitting settings instantly. -->bao
	mDelay = 0 ; //99
}

LLViewerPartSourceScript::LLViewerPartSourceScript(LLViewerObject *source_objp) :
	LLViewerPartSource(LL_PART_SOURCE_SCRIPT)
{
	llassert(source_objp);
	mSourceObjectp = source_objp;
	mPosAgent = mSourceObjectp->getPositionAgent();
	mImagep = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
	
	mImagep->setAddressMode(LLTexUnit::TAM_CLAMP);
}


void LLViewerPartSourceScript::setDead()
{
	mIsDead = TRUE;
	mSourceObjectp = NULL;
	mTargetObjectp = NULL;
}

void LLViewerPartSourceScript::update(const F32 dt)
{
	if( mIsSuspended )
		return;

	F32 old_update_time = mLastUpdateTime;
	mLastUpdateTime += dt;

	F32 ref_rate_travelspeed = llmin(LLViewerPartSim::getInstance()->getRefRate(), 1.f);
	
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


	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PARTICLES))
	{
		if (mSourceObjectp.notNull())
		{
			std::ostringstream ostr;
			ostr << mPartSysData;
			mSourceObjectp->setDebugText(ostr.str());
		}
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
		
		if (LLViewerPartSim::getInstance()->aboveParticleLimit())
		{
			// Don't bother doing any more updates if we're above the particle limit,
			// just give up.
			mLastPartTime = mLastUpdateTime;
            break;

		}
		
		// find the greatest length that the shortest side of a system
		// particle is expected to have
		F32 max_short_side =
			llmax(
			      llmax(llmin(mPartSysData.mPartData.mStartScale[0],
					  mPartSysData.mPartData.mStartScale[1]),
				    llmin(mPartSysData.mPartData.mEndScale[0],
					  mPartSysData.mPartData.mEndScale[1])),
			      llmin((mPartSysData.mPartData.mStartScale[0]
				     + mPartSysData.mPartData.mEndScale[0])/2,
				    (mPartSysData.mPartData.mStartScale[1]
				     + mPartSysData.mPartData.mEndScale[1])/2));
		
		F32 pixel_meter_ratio = LLViewerCamera::getInstance()->getPixelMeterRatio();

		// Maximum distance at which spawned particles will be viewable
		F32 max_dist = max_short_side * pixel_meter_ratio; 

		if (max_dist < 0.25f)
		{
			// < 1 pixel wide at a distance of >=25cm.  Particles
			// this tiny are useless and mostly spawned by buggy
			// sources
			mLastPartTime = mLastUpdateTime;
			break;
		}

		// Distance from camera
		F32 dist = (mPosAgent - LLViewerCamera::getInstance()->getOrigin()).magVec();

		// Particle size vs distance vs maxage throttling

		F32 limited_rate=0.f;
		if (dist - max_dist > 0.f)
		{
			if((dist - max_dist) * ref_rate_travelspeed > mPartSysData.mPartData.mMaxAge - 0.2f )
			{
				// You need to travel faster than 1 divided by reference rate m/s directly towards these particles to see them at least 0.2s
				mLastPartTime = mLastUpdateTime;
				break;
			}
			limited_rate = ((dist - max_dist) * ref_rate_travelspeed) / mPartSysData.mPartData.mMaxAge;
		}
		
		if(mDelay)
		{			
			limited_rate = llmax(limited_rate, 0.01f * mDelay--) ;
		}

		S32 i;
		for (i = 0; i < mPartSysData.mBurstPartCount; i++)
		{
			if (ll_frand() < llmax(1.0f - LLViewerPartSim::getInstance()->getBurstRate(), limited_rate))
			{
				// Limit particle generation
				continue;
			}

			LLViewerPart* part = new LLViewerPart();

			part->init(this, mImagep, NULL);
			part->mFlags = mPartSysData.mPartData.mFlags;
			if (!mSourceObjectp.isNull() && mSourceObjectp->isHUDAttachment())
			{
				part->mFlags |= LLPartData::LL_PART_HUD;
			}
			part->mMaxAge = mPartSysData.mPartData.mMaxAge;
			part->mStartColor = mPartSysData.mPartData.mStartColor;
			part->mEndColor = mPartSysData.mPartData.mEndColor;
			part->mColor = part->mStartColor;

			part->mStartScale = mPartSysData.mPartData.mStartScale;
			part->mEndScale = mPartSysData.mPartData.mEndScale;
			part->mScale = part->mStartScale;

			part->mAccel = mPartSysData.mPartAccel;

			if (mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_DROP)
			{
				part->mPosAgent = mPosAgent;
				part->mVelocity.setVec(0.f, 0.f, 0.f);
			}
			else if (mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_EXPLODE)
			{
				part->mPosAgent = mPosAgent;
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
				part->mPosAgent += mPartSysData.mBurstRadius*part_dir_vector;
				part->mVelocity = part_dir_vector;
				F32 speed = mPartSysData.mBurstSpeedMin + ll_frand(mPartSysData.mBurstSpeedMax - mPartSysData.mBurstSpeedMin);
				part->mVelocity *= speed;
			}
			else if (mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_ANGLE
				|| mPartSysData.mPattern & LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE)
			{				
				part->mPosAgent = mPosAgent;
				
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
								
				part->mPosAgent += mPartSysData.mBurstRadius*part_dir_vector;

				part->mVelocity = part_dir_vector;

				F32 speed = mPartSysData.mBurstSpeedMin + ll_frand(mPartSysData.mBurstSpeedMax - mPartSysData.mBurstSpeedMin);
				part->mVelocity *= speed;
			}
			else
			{
				part->mPosAgent = mPosAgent;
				part->mVelocity.setVec(0.f, 0.f, 0.f);
				//llwarns << "Unknown source pattern " << (S32)mPartSysData.mPattern << llendl;
			}

			if (part->mFlags & LLPartData::LL_PART_FOLLOW_SRC_MASK ||	// SVC-193, VWR-717
				part->mFlags & LLPartData::LL_PART_TARGET_LINEAR_MASK) 
			{
				mPartSysData.mBurstRadius = 0; 
			}

			LLViewerPartSim::getInstance()->addPart(part);
		}

		mLastPartTime = mLastUpdateTime;
		dt_update -= mPartSysData.mBurstRate;
	}
}

// static
LLPointer<LLViewerPartSourceScript> LLViewerPartSourceScript::unpackPSS(LLViewerObject *source_objp, LLPointer<LLViewerPartSourceScript> pssp, const S32 block_num)
{
	if (!pssp)
	{
		if (LLPartSysData::isNullPS(block_num))
		{
			return NULL;
		}
		LLPointer<LLViewerPartSourceScript> new_pssp = new LLViewerPartSourceScript(source_objp);
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


LLPointer<LLViewerPartSourceScript> LLViewerPartSourceScript::unpackPSS(LLViewerObject *source_objp, LLPointer<LLViewerPartSourceScript> pssp, LLDataPacker &dp)
{
	if (!pssp)
	{
		LLPointer<LLViewerPartSourceScript> new_pssp = new LLViewerPartSourceScript(source_objp);
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


/* static */
LLPointer<LLViewerPartSourceScript> LLViewerPartSourceScript::createPSS(LLViewerObject *source_objp, const LLPartSysData& particle_parameters)
{
	LLPointer<LLViewerPartSourceScript> new_pssp = new LLViewerPartSourceScript(source_objp);

	new_pssp->mPartSysData = particle_parameters;

	if (new_pssp->mPartSysData.mTargetUUID.notNull())
	{
		LLViewerObject *target_objp = gObjectList.findObject(new_pssp->mPartSysData.mTargetUUID);
		new_pssp->setTargetObject(target_objp);
	}
	return new_pssp;
}


void LLViewerPartSourceScript::setImage(LLViewerTexture *imagep)
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
	LLPointer<LLViewerPartSource>& ps = part.mPartSourcep;
	LLViewerPartSourceSpiral *pss = (LLViewerPartSourceSpiral *)ps.get();
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
		mImagep = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
	}

	const F32 RATE = 0.025f;

	mLastUpdateTime += dt;

	F32 dt_update = mLastUpdateTime - mLastPartTime;
	F32 max_time = llmax(1.f, 10.f*RATE);
	dt_update = llmin(max_time, dt_update);

	if (dt_update > RATE)
	{
		mLastPartTime = mLastUpdateTime;
		if (!LLViewerPartSim::getInstance()->shouldAddPart())
		{
			// Particle simulation says we have too many particles, skip all this
			return;
		}

		if (!mSourceObjectp.isNull() && !mSourceObjectp->mDrawable.isNull())
		{
			mPosAgent = mSourceObjectp->getRenderPosition();
		}
		LLViewerPart* part = new LLViewerPart();
		part->init(this, mImagep, updatePart);
		part->mStartColor = mColor;
		part->mEndColor = mColor;
		part->mEndColor.mV[3] = 0.f;
		part->mPosAgent = mPosAgent;
		part->mMaxAge = 1.f;
		part->mFlags = LLViewerPart::LL_PART_INTERP_COLOR_MASK;
		part->mLastUpdateTime = 0.f;
		part->mScale.mV[0] = 0.25f;
		part->mScale.mV[1] = 0.25f;
		part->mParameter = ll_frand(F_TWO_PI);

		LLViewerPartSim::getInstance()->addPart(part);
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
		if (!LLViewerPartSim::getInstance()->shouldAddPart())
		{
			// Particle simulation says we have too many particles, skip all this
			return;
		}

		if (!mImagep)
		{
			mImagep = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
		}

		LLViewerPart* part = new LLViewerPart();
		part->init(this, mImagep, NULL);

		part->mFlags = LLPartData::LL_PART_INTERP_COLOR_MASK |
						LLPartData::LL_PART_INTERP_SCALE_MASK |
						LLPartData::LL_PART_TARGET_POS_MASK |
						LLPartData::LL_PART_FOLLOW_VELOCITY_MASK;
		part->mMaxAge = 0.5f;
		part->mStartColor = mColor;
		part->mEndColor = part->mStartColor;
		part->mEndColor.mV[3] = 0.4f;
		part->mColor = part->mStartColor;

		part->mStartScale = LLVector2(0.1f, 0.1f);
		part->mEndScale = LLVector2(0.1f, 0.1f);
		part->mScale = part->mStartScale;

		part->mPosAgent = mPosAgent;
		part->mVelocity = mTargetPosAgent - mPosAgent;

		LLViewerPartSim::getInstance()->addPart(part);
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
		mImagep = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
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
		if (!LLViewerPartSim::getInstance()->shouldAddPart())
		{
			// Particle simulation says we have too many particles, skip all this
			return;
		}

		if (!mSourceObjectp.isNull() && !mSourceObjectp->mDrawable.isNull())
		{
			mPosAgent = mSourceObjectp->getRenderPosition();
		}
		LLViewerPart* part = new LLViewerPart();
		part->init(this, mImagep, updatePart);
		part->mStartColor = mColor;
		part->mEndColor = mColor;
		part->mEndColor.mV[3] = 0.f;
		part->mPosAgent = mPosAgent;
		part->mMaxAge = 1.f;
		part->mFlags = LLViewerPart::LL_PART_INTERP_COLOR_MASK;
		part->mLastUpdateTime = 0.f;
		part->mScale.mV[0] = 0.25f;
		part->mScale.mV[1] = 0.25f;
		part->mParameter = ll_frand(F_TWO_PI);

		LLViewerPartSim::getInstance()->addPart(part);
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


