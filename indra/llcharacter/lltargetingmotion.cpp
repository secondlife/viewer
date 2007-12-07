/** 
 * @file lltargetingmotion.cpp
 * @brief Implementation of LLTargetingMotion class.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "lltargetingmotion.h"
#include "llcharacter.h"
#include "v3dmath.h"
#include "llcriticaldamp.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const F32 TORSO_TARGET_HALF_LIFE = 0.25f;
const F32 MAX_TIME_DELTA = 2.f; //max two seconds a frame for calculating interpolation
const F32 TARGET_PLANE_THRESHOLD_DOT = 0.6f;
const F32 TORSO_ROT_FRACTION = 0.5f;

//-----------------------------------------------------------------------------
// LLTargetingMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLTargetingMotion::LLTargetingMotion(const LLUUID &id) : LLMotion(id)
{
	mCharacter = NULL;
	mName = "targeting";

	mTorsoState = new LLJointState;
}


//-----------------------------------------------------------------------------
// ~LLTargetingMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLTargetingMotion::~LLTargetingMotion()
{
}

//-----------------------------------------------------------------------------
// LLTargetingMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLTargetingMotion::onInitialize(LLCharacter *character)
{
	// save character for future use
	mCharacter = character;

	mPelvisJoint = mCharacter->getJoint("mPelvis");
	mTorsoJoint = mCharacter->getJoint("mTorso");
	mRightHandJoint = mCharacter->getJoint("mWristRight");

	// make sure character skeleton is copacetic
	if (!mPelvisJoint ||
		!mTorsoJoint ||
		!mRightHandJoint)
	{
		llwarns << "Invalid skeleton for targeting motion!" << llendl;
		return STATUS_FAILURE;
	}

	mTorsoState->setJoint( mTorsoJoint );

	// add joint states to the pose
	mTorsoState->setUsage(LLJointState::ROT);
	addJointState( mTorsoState );

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// LLTargetingMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLTargetingMotion::onActivate()
{
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLTargetingMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLTargetingMotion::onUpdate(F32 time, U8* joint_mask)
{
	F32 slerp_amt = LLCriticalDamp::getInterpolant(TORSO_TARGET_HALF_LIFE);

	LLVector3 target;
	LLVector3* lookAtPoint = (LLVector3*)mCharacter->getAnimationData("LookAtPoint");

	BOOL result = TRUE;

	if (!lookAtPoint)
	{
		return TRUE;
	}
	else
	{
		target = *lookAtPoint;
		target.normVec();
	}
	
	//LLVector3 target_plane_normal = LLVector3(1.f, 0.f, 0.f) * mPelvisJoint->getWorldRotation();
	//LLVector3 torso_dir = LLVector3(1.f, 0.f, 0.f) * (mTorsoJoint->getWorldRotation() * mTorsoState->getRotation());

	LLVector3 skyward(0.f, 0.f, 1.f);
	LLVector3 left(skyward % target);
	left.normVec();
	LLVector3 up(target % left);
	up.normVec();
	LLQuaternion target_aim_rot(target, left, up);

	LLQuaternion cur_torso_rot = mTorsoJoint->getWorldRotation();

	LLVector3 right_hand_at = LLVector3(0.f, -1.f, 0.f) * mRightHandJoint->getWorldRotation();
	left.setVec(skyward % right_hand_at);
	left.normVec();
	up.setVec(right_hand_at % left);
	up.normVec();
	LLQuaternion right_hand_rot(right_hand_at, left, up);

	LLQuaternion new_torso_rot = (cur_torso_rot * ~right_hand_rot) * target_aim_rot;

	// find ideal additive rotation to make torso point in correct direction
	new_torso_rot = new_torso_rot * ~cur_torso_rot;

	// slerp from current additive rotation to ideal additive rotation
	new_torso_rot = nlerp(slerp_amt, mTorsoState->getRotation(), new_torso_rot);

	// constraint overall torso rotation
	LLQuaternion total_rot = new_torso_rot * mTorsoJoint->getRotation();
	total_rot.constrain(F_PI_BY_TWO * 0.8f);
	new_torso_rot = total_rot * ~mTorsoJoint->getRotation();

	mTorsoState->setRotation(new_torso_rot);

	return result;
}

//-----------------------------------------------------------------------------
// LLTargetingMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLTargetingMotion::onDeactivate()
{
}


// End
