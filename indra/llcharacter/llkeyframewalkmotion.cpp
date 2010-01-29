/** 
 * @file llkeyframewalkmotion.cpp
 * @brief Implementation of LLKeyframeWalkMotion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llkeyframewalkmotion.h"
#include "llcharacter.h"
#include "llmath.h"
#include "m3math.h"
#include "llcriticaldamp.h"

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
const F32 MAX_WALK_PLAYBACK_SPEED = 8.f;	// max m/s for which we adjust walk cycle speed

const F32 MIN_WALK_SPEED = 0.1f;	// minimum speed at which we use velocity for down foot detection
const F32 MAX_TIME_DELTA = 2.f;		//max two seconds a frame for calculating interpolation
const F32 SPEED_ADJUST_MAX = 2.5f; // maximum adjustment of walk animation playback speed
const F32 SPEED_ADJUST_MAX_SEC = 3.f;	// maximum adjustment to walk animation playback speed for a second
const F32 DRIFT_COMP_MAX_TOTAL = 0.07f;//0.55f; // maximum drift compensation overall, in any direction 
const F32 DRIFT_COMP_MAX_SPEED = 4.f; // speed at which drift compensation total maxes out
const F32 MAX_ROLL = 0.6f;

//-----------------------------------------------------------------------------
// LLKeyframeWalkMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLKeyframeWalkMotion::LLKeyframeWalkMotion(const LLUUID &id)
  : LLKeyframeMotion(id),
    
    mCharacter(NULL),
    mCyclePhase(0.0f),
    mRealTimeLast(0.0f),
    mAdjTimeLast(0.0f),
    mDownFoot(0)
{
}


//-----------------------------------------------------------------------------
// ~LLKeyframeWalkMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLKeyframeWalkMotion::~LLKeyframeWalkMotion()
{
}


//-----------------------------------------------------------------------------
// LLKeyframeWalkMotion::onInitialize()
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLKeyframeWalkMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;

	return LLKeyframeMotion::onInitialize(character);
}

//-----------------------------------------------------------------------------
// LLKeyframeWalkMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeWalkMotion::onActivate()
{
	mRealTimeLast = 0.0f;
	mAdjTimeLast = 0.0f;

	return LLKeyframeMotion::onActivate();
}

//-----------------------------------------------------------------------------
// LLKeyframeWalkMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLKeyframeWalkMotion::onDeactivate()
{
	mCharacter->removeAnimationData("Down Foot");		
	LLKeyframeMotion::onDeactivate();
}

//-----------------------------------------------------------------------------
// LLKeyframeWalkMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeWalkMotion::onUpdate(F32 time, U8* joint_mask)
{
	// compute time since last update
	F32 deltaTime = time - mRealTimeLast;

	void* speed_ptr = mCharacter->getAnimationData("Walk Speed");
	F32 speed = (speed_ptr) ? *((F32 *)speed_ptr) : 1.f;

	// adjust the passage of time accordingly
	F32 adjusted_time = mAdjTimeLast + (deltaTime * speed);

	// save time for next update
	mRealTimeLast = time;
	mAdjTimeLast = adjusted_time;

	// handle wrap around
	if (adjusted_time < 0.0f)
	{
		adjusted_time = getDuration() + fmod(adjusted_time, getDuration());
	}

	// let the base class update the cycle
	return LLKeyframeMotion::onUpdate( adjusted_time, joint_mask );
}

// End


//-----------------------------------------------------------------------------
// LLWalkAdjustMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLWalkAdjustMotion::LLWalkAdjustMotion(const LLUUID &id) :
	LLMotion(id),
	mLastTime(0.f),
	mAvgCorrection(0.f),
	mSpeedAdjust(0.f),
	mAnimSpeed(0.f),
	mAvgSpeed(0.f),
	mRelativeDir(0.f),
	mAnkleOffset(0.f)
{
	mName = "walk_adjust";

	mPelvisState = new LLJointState;
}

//-----------------------------------------------------------------------------
// LLWalkAdjustMotion::onInitialize()
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLWalkAdjustMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	mLeftAnkleJoint = mCharacter->getJoint("mAnkleLeft");
	mRightAnkleJoint = mCharacter->getJoint("mAnkleRight");

	mPelvisJoint = mCharacter->getJoint("mPelvis");
	mPelvisState->setJoint( mPelvisJoint );
	if ( !mPelvisJoint )
	{
		llwarns << getName() << ": Can't get pelvis joint." << llendl;
		return STATUS_FAILURE;
	}

	mPelvisState->setUsage(LLJointState::POS);
	addJointState( mPelvisState );

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// LLWalkAdjustMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLWalkAdjustMotion::onActivate()
{
	mAvgCorrection = 0.f;
	mSpeedAdjust = 0.f;
	mAnimSpeed = 0.f;
	mAvgSpeed = 0.f;
	mRelativeDir = 1.f;
	mPelvisState->setPosition(LLVector3::zero);
	// store ankle positions for next frame
	mLastLeftAnklePos = mCharacter->getPosGlobalFromAgent(mLeftAnkleJoint->getWorldPosition());
	mLastRightAnklePos = mCharacter->getPosGlobalFromAgent(mRightAnkleJoint->getWorldPosition());

	F32 leftAnkleOffset = (mLeftAnkleJoint->getWorldPosition() - mCharacter->getCharacterPosition()).magVec();
	F32 rightAnkleOffset = (mRightAnkleJoint->getWorldPosition() - mCharacter->getCharacterPosition()).magVec();
	mAnkleOffset = llmax(leftAnkleOffset, rightAnkleOffset);

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLWalkAdjustMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLWalkAdjustMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLVector3 footCorrection;
	LLVector3 vel = mCharacter->getCharacterVelocity() * mCharacter->getTimeDilation();
	F32 deltaTime = llclamp(time - mLastTime, 0.f, MAX_TIME_DELTA);
	mLastTime = time;

	LLQuaternion inv_rotation = ~mPelvisJoint->getWorldRotation();

	// get speed and normalize velocity vector
	LLVector3 ang_vel = mCharacter->getCharacterAngularVelocity() * mCharacter->getTimeDilation();
	F32 speed = llmin(vel.normVec(), MAX_WALK_PLAYBACK_SPEED);
	mAvgSpeed = lerp(mAvgSpeed, speed, LLCriticalDamp::getInterpolant(0.2f));

	// calculate facing vector in pelvis-local space 
	// (either straight forward or back, depending on velocity)
	LLVector3 localVel = vel * inv_rotation;
	if (localVel.mV[VX] > 0.f)
	{
		mRelativeDir = 1.f;
	}
	else if (localVel.mV[VX] < 0.f)
	{
		mRelativeDir = -1.f;
	}

	// calculate world-space foot drift
	LLVector3 leftFootDelta;
	LLVector3 leftFootWorldPosition = mLeftAnkleJoint->getWorldPosition();
	LLVector3d leftFootGlobalPosition = mCharacter->getPosGlobalFromAgent(leftFootWorldPosition);
	leftFootDelta.setVec(mLastLeftAnklePos - leftFootGlobalPosition);
	mLastLeftAnklePos = leftFootGlobalPosition;

	LLVector3 rightFootDelta;
	LLVector3 rightFootWorldPosition = mRightAnkleJoint->getWorldPosition();
	LLVector3d rightFootGlobalPosition = mCharacter->getPosGlobalFromAgent(rightFootWorldPosition);
	rightFootDelta.setVec(mLastRightAnklePos - rightFootGlobalPosition);
	mLastRightAnklePos = rightFootGlobalPosition;

	// find foot drift along velocity vector
	if (mAvgSpeed > 0.1)
	{
		// walking/running
		F32 leftFootDriftAmt = leftFootDelta * vel;
		F32 rightFootDriftAmt = rightFootDelta * vel;

		if (rightFootDriftAmt > leftFootDriftAmt)
		{
			footCorrection = rightFootDelta;
		} else
		{
			footCorrection = leftFootDelta;
		}
	}
	else
	{
		mAvgSpeed = ang_vel.magVec() * mAnkleOffset;
		mRelativeDir = 1.f;

		// standing/turning
		// find the lower foot
		if (leftFootWorldPosition.mV[VZ] < rightFootWorldPosition.mV[VZ])
		{
			// pivot on left foot
			footCorrection = leftFootDelta;
		}
		else
		{
			// pivot on right foot
			footCorrection = rightFootDelta;
		}
	}

	// rotate into avatar coordinates
	footCorrection = footCorrection * inv_rotation;

	// calculate ideal pelvis offset so that foot is glued to ground and damp towards it
	// the amount of foot slippage this frame + the offset applied last frame
	mPelvisOffset = mPelvisState->getPosition() + lerp(LLVector3::zero, footCorrection, LLCriticalDamp::getInterpolant(0.2f));

	// pelvis drift (along walk direction)
	mAvgCorrection = lerp(mAvgCorrection, footCorrection.mV[VX] * mRelativeDir, LLCriticalDamp::getInterpolant(0.1f));

	// calculate average velocity of foot slippage
	F32 footSlipVelocity = (deltaTime != 0.f) ? (-mAvgCorrection / deltaTime) : 0.f;

	F32 newSpeedAdjust = 0.f;
	
	// modulate speed by dot products of facing and velocity
	// so that if we are moving sideways, we slow down the animation
	// and if we're moving backward, we walk backward

	F32 directional_factor = localVel.mV[VX] * mRelativeDir;
	if (speed > 0.1f)
	{
		// calculate ratio of desired foot velocity to detected foot velocity
		newSpeedAdjust = llclamp(footSlipVelocity - mAvgSpeed * (1.f - directional_factor), 
								-SPEED_ADJUST_MAX, SPEED_ADJUST_MAX);
		newSpeedAdjust = lerp(mSpeedAdjust, newSpeedAdjust, LLCriticalDamp::getInterpolant(0.2f));

		F32 speedDelta = newSpeedAdjust - mSpeedAdjust;
		speedDelta = llclamp(speedDelta, -SPEED_ADJUST_MAX_SEC * deltaTime, SPEED_ADJUST_MAX_SEC * deltaTime);

		mSpeedAdjust = mSpeedAdjust + speedDelta;
	}
	else
	{
		mSpeedAdjust = lerp(mSpeedAdjust, 0.f, LLCriticalDamp::getInterpolant(0.2f));
	}

	mAnimSpeed = (mAvgSpeed + mSpeedAdjust) * mRelativeDir;
//	char debug_text[64];
//	sprintf(debug_text, "Foot slip vel: %.2f", footSlipVelocity);
//	mCharacter->addDebugText(debug_text);
//	sprintf(debug_text, "Speed: %.2f", mAvgSpeed);
//	mCharacter->addDebugText(debug_text);
//	sprintf(debug_text, "Speed Adjust: %.2f", mSpeedAdjust);
//	mCharacter->addDebugText(debug_text);
//	sprintf(debug_text, "Animation Playback Speed: %.2f", mAnimSpeed);
//	mCharacter->addDebugText(debug_text);
	mCharacter->setAnimationData("Walk Speed", &mAnimSpeed);

	// clamp pelvis offset to a 90 degree arc behind the nominal position
	F32 drift_comp_max = llclamp(speed, 0.f, DRIFT_COMP_MAX_SPEED) / DRIFT_COMP_MAX_SPEED;
	drift_comp_max *= DRIFT_COMP_MAX_TOTAL;

	LLVector3 currentPelvisPos = mPelvisState->getJoint()->getPosition();

	// NB: this is an ADDITIVE amount that is accumulated every frame, so clamping it alone won't do the trick
	// must clamp with absolute position of pelvis in mind
	mPelvisOffset.mV[VX] = llclamp( mPelvisOffset.mV[VX], -drift_comp_max - currentPelvisPos.mV[VX], drift_comp_max - currentPelvisPos.mV[VX]  );
	mPelvisOffset.mV[VY] = llclamp( mPelvisOffset.mV[VY], -drift_comp_max - currentPelvisPos.mV[VY], drift_comp_max - currentPelvisPos.mV[VY]);
	mPelvisOffset.mV[VZ] = 0.f;

	// set position
	mPelvisState->setPosition(mPelvisOffset);

	mCharacter->setAnimationData("Pelvis Offset", &mPelvisOffset);

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLWalkAdjustMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLWalkAdjustMotion::onDeactivate()
{
	mCharacter->removeAnimationData("Walk Speed");
}

//-----------------------------------------------------------------------------
// LLFlyAdjustMotion::LLFlyAdjustMotion()
//-----------------------------------------------------------------------------
LLFlyAdjustMotion::LLFlyAdjustMotion(const LLUUID &id)
	: LLMotion(id),
	  mRoll(0.f)
{
	mName = "fly_adjust";

	mPelvisState = new LLJointState;
}

//-----------------------------------------------------------------------------
// LLFlyAdjustMotion::onInitialize()
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLFlyAdjustMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;

	LLJoint* pelvisJoint = mCharacter->getJoint("mPelvis");
	mPelvisState->setJoint( pelvisJoint );
	if ( !pelvisJoint )
	{
		llwarns << getName() << ": Can't get pelvis joint." << llendl;
		return STATUS_FAILURE;
	}

	mPelvisState->setUsage(LLJointState::POS | LLJointState::ROT);
	addJointState( mPelvisState );

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// LLFlyAdjustMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLFlyAdjustMotion::onActivate()
{
	mPelvisState->setPosition(LLVector3::zero);
	mPelvisState->setRotation(LLQuaternion::DEFAULT);
	mRoll = 0.f;
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLFlyAdjustMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLFlyAdjustMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLVector3 ang_vel = mCharacter->getCharacterAngularVelocity() * mCharacter->getTimeDilation();
	F32 speed = mCharacter->getCharacterVelocity().magVec();

	F32 roll_factor = clamp_rescale(speed, 7.f, 15.f, 0.f, -MAX_ROLL);
	F32 target_roll = llclamp(ang_vel.mV[VZ], -4.f, 4.f) * roll_factor;

	// roll is critically damped interpolation between current roll and angular velocity-derived target roll
	mRoll = lerp(mRoll, target_roll, LLCriticalDamp::getInterpolant(0.1f));

//	llinfos << mRoll << llendl;

	LLQuaternion roll(mRoll, LLVector3(0.f, 0.f, 1.f));
	mPelvisState->setRotation(roll);

//	F32 lerp_amt = LLCriticalDamp::getInterpolant(0.2f);
//	
//	LLVector3 pelvis_correction = mPelvisState->getPosition() - lerp(LLVector3::zero, mPelvisState->getJoint()->getPosition() + mPelvisState->getPosition(), lerp_amt);
//	mPelvisState->setPosition(pelvis_correction);
	return TRUE;
}
