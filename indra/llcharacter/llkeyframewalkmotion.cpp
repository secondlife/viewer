/** 
 * @file llkeyframewalkmotion.cpp
 * @brief Implementation of LLKeyframeWalkMotion class.
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
const F32 MAX_WALK_PLAYBACK_SPEED = 8.f;		// max m/s for which we adjust walk cycle speed

const F32 MIN_WALK_SPEED = 0.1f;				// minimum speed at which we use velocity for down foot detection
const F32 TIME_EPSILON = 0.001f;				// minumum frame time
const F32 MAX_TIME_DELTA = 2.f;					// max two seconds a frame for calculating interpolation
F32 SPEED_ADJUST_MAX_SEC = 2.f;					// maximum adjustment to walk animation playback speed for a second
F32 ANIM_SPEED_MAX = 1.5f;						// absolute upper limit on animation speed
const F32 DRIFT_COMP_MAX_TOTAL = 0.1f;			// maximum drift compensation overall, in any direction 
const F32 DRIFT_COMP_MAX_SPEED = 4.f;			// speed at which drift compensation total maxes out
const F32 MAX_ROLL = 0.6f;
const F32 PELVIS_COMPENSATION_WIEGHT = 0.7f; 	// proportion of foot drift that is compensated by moving the avatar directly
const F32 SPEED_ADJUST_TIME_CONSTANT = 0.1f; 	// time constant for speed adjustment interpolation

//-----------------------------------------------------------------------------
// LLKeyframeWalkMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLKeyframeWalkMotion::LLKeyframeWalkMotion(const LLUUID &id)
:	LLKeyframeMotion(id),
    mCharacter(NULL),
    mCyclePhase(0.0f),
    mRealTimeLast(0.0f),
    mAdjTimeLast(0.0f),
    mDownFoot(0)
{}


//-----------------------------------------------------------------------------
// ~LLKeyframeWalkMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLKeyframeWalkMotion::~LLKeyframeWalkMotion()
{}


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
	mAnimSpeed(0.f),
	mAdjustedSpeed(0.f),
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
	mAnimSpeed = 0.f;
	mAdjustedSpeed = 0.f;
	mRelativeDir = 1.f;
	mPelvisState->setPosition(LLVector3::zero);
	// store ankle positions for next frame
	mLastLeftFootGlobalPos = mCharacter->getPosGlobalFromAgent(mLeftAnkleJoint->getWorldPosition());
	mLastLeftFootGlobalPos.mdV[VZ] = 0.0;

	mLastRightFootGlobalPos = mCharacter->getPosGlobalFromAgent(mRightAnkleJoint->getWorldPosition());
	mLastRightFootGlobalPos.mdV[VZ] = 0.0;

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
	// delta_time is guaranteed to be non zero
	F32 delta_time = llclamp(time - mLastTime, TIME_EPSILON, MAX_TIME_DELTA);
	mLastTime = time;

	// find the avatar motion vector in the XY plane
	LLVector3 avatar_velocity = mCharacter->getCharacterVelocity() * mCharacter->getTimeDilation();
	avatar_velocity.mV[VZ] = 0.f;

	F32 speed = llclamp(avatar_velocity.magVec(), 0.f, MAX_WALK_PLAYBACK_SPEED);

	// grab avatar->world transforms
	LLQuaternion avatar_to_world_rot = mCharacter->getRootJoint()->getWorldRotation();

	LLQuaternion world_to_avatar_rot(avatar_to_world_rot);
	world_to_avatar_rot.conjugate();

	LLVector3 foot_slip_vector;

	// find foot drift along velocity vector
	if (speed > MIN_WALK_SPEED)
	{	// walking/running

		// calculate world-space foot drift
		// use global coordinates to seamlessly handle region crossings
		LLVector3d leftFootGlobalPosition = mCharacter->getPosGlobalFromAgent(mLeftAnkleJoint->getWorldPosition());
		leftFootGlobalPosition.mdV[VZ] = 0.0;
		LLVector3 leftFootDelta(leftFootGlobalPosition - mLastLeftFootGlobalPos);
		mLastLeftFootGlobalPos = leftFootGlobalPosition;

		LLVector3d rightFootGlobalPosition = mCharacter->getPosGlobalFromAgent(mRightAnkleJoint->getWorldPosition());
		rightFootGlobalPosition.mdV[VZ] = 0.0;
		LLVector3 rightFootDelta(rightFootGlobalPosition - mLastRightFootGlobalPos);
		mLastRightFootGlobalPos = rightFootGlobalPosition;

		// get foot drift along avatar direction of motion
		F32 left_foot_slip_amt = leftFootDelta * avatar_velocity;
		F32 right_foot_slip_amt = rightFootDelta * avatar_velocity;

		// if right foot is pushing back faster than left foot...
		if (right_foot_slip_amt < left_foot_slip_amt)
		{	//...use it to calculate optimal animation speed
			foot_slip_vector = rightFootDelta;
		} 
		else
		{	// otherwise use the left foot
			foot_slip_vector = leftFootDelta;
		}

		// calculate ideal pelvis offset so that foot is glued to ground and damp towards it
		// this will soak up transient slippage
		//
		// FIXME: this interacts poorly with speed adjustment
		// mPelvisOffset compensates for foot drift by moving the avatar pelvis in the opposite
		// direction of the drift, up to a certain limited distance
		// but this will cause the animation playback rate calculation below to 
		// kick in too slowly and sometimes start playing the animation in reverse.

		//mPelvisOffset -= PELVIS_COMPENSATION_WIEGHT * (foot_slip_vector * world_to_avatar_rot);//lerp(LLVector3::zero, -1.f * (foot_slip_vector * world_to_avatar_rot), LLCriticalDamp::getInterpolant(0.1f));

		////F32 drift_comp_max = DRIFT_COMP_MAX_TOTAL * (llclamp(speed, 0.f, DRIFT_COMP_MAX_SPEED) / DRIFT_COMP_MAX_SPEED);
		//F32 drift_comp_max = DRIFT_COMP_MAX_TOTAL;

		//// clamp pelvis offset to a 90 degree arc behind the nominal position
		//// NB: this is an ADDITIVE amount that is accumulated every frame, so clamping it alone won't do the trick
		//// must clamp with absolute position of pelvis in mind
		//LLVector3 currentPelvisPos = mPelvisState->getJoint()->getPosition();
		//mPelvisOffset.mV[VX] = llclamp( mPelvisOffset.mV[VX], -drift_comp_max, drift_comp_max );
		//mPelvisOffset.mV[VY] = llclamp( mPelvisOffset.mV[VY], -drift_comp_max, drift_comp_max );
		//mPelvisOffset.mV[VZ] = 0.f;
		//
		//mLastRightFootGlobalPos += LLVector3d(mPelvisOffset * avatar_to_world_rot);
		//mLastLeftFootGlobalPos += LLVector3d(mPelvisOffset * avatar_to_world_rot);

		//foot_slip_vector -= mPelvisOffset;

		LLVector3 avatar_movement_dir = avatar_velocity;
		avatar_movement_dir.normalize();

		// planted foot speed is avatar velocity - foot slip amount along avatar movement direction
		F32 foot_speed = speed - ((foot_slip_vector * avatar_movement_dir) / delta_time);

		// multiply animation playback rate so that foot speed matches avatar speed
		F32 min_speed_multiplier = clamp_rescale(speed, 0.f, 1.f, 0.f, 0.1f);
		F32 desired_speed_multiplier = llclamp(speed / foot_speed, min_speed_multiplier, ANIM_SPEED_MAX);

		// blend towards new speed adjustment value
		F32 new_speed_adjust = lerp(mAdjustedSpeed, desired_speed_multiplier, LLCriticalDamp::getInterpolant(SPEED_ADJUST_TIME_CONSTANT));

		// limit that rate at which the speed adjustment changes
		F32 speedDelta = llclamp(new_speed_adjust - mAdjustedSpeed, -SPEED_ADJUST_MAX_SEC * delta_time, SPEED_ADJUST_MAX_SEC * delta_time);
		mAdjustedSpeed += speedDelta;

		// modulate speed by dot products of facing and velocity
		// so that if we are moving sideways, we slow down the animation
		// and if we're moving backward, we walk backward
		// do this at the end to be more responsive to direction changes instead of in the above speed calculations
		F32 directional_factor = (avatar_movement_dir * world_to_avatar_rot).mV[VX];

		mAnimSpeed = mAdjustedSpeed * directional_factor;
	}
	else
	{	// standing/turning

		// damp out speed adjustment to 0
		mAnimSpeed = lerp(mAnimSpeed, 1.f, LLCriticalDamp::getInterpolant(0.2f));
		//mPelvisOffset = lerp(mPelvisOffset, LLVector3::zero, LLCriticalDamp::getInterpolant(0.2f));
	}

	// broadcast walk speed change
 	mCharacter->setAnimationData("Walk Speed", &mAnimSpeed);

	// set position
	// need to update *some* joint to keep this animation active
	mPelvisState->setPosition(mPelvisOffset);

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

	LLQuaternion roll(mRoll, LLVector3(0.f, 0.f, 1.f));
	mPelvisState->setRotation(roll);

	return TRUE;
}
