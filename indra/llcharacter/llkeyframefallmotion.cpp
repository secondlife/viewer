/** 
 * @file llkeyframefallmotion.cpp
 * @brief Implementation of LLKeyframeFallMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llkeyframefallmotion.h"
#include "llcharacter.h"
#include "m3math.h"

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
#define GO_TO_KEY_POSE	1
#define MIN_TRACK_SPEED 0.01f

//-----------------------------------------------------------------------------
// LLKeyframeFallMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLKeyframeFallMotion::LLKeyframeFallMotion(const LLUUID &id) : LLKeyframeMotion(id)
{
	mVelocityZ = 0.f;
	mCharacter = NULL;
}


//-----------------------------------------------------------------------------
// ~LLKeyframeFallMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLKeyframeFallMotion::~LLKeyframeFallMotion()
{
}


//-----------------------------------------------------------------------------
// LLKeyframeFallMotion::onInitialize()
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLKeyframeFallMotion::onInitialize(LLCharacter *character)
{
	// save character pointer for later use
	mCharacter = character;

	// load keyframe data, setup pose and joint states
	LLMotion::LLMotionInitStatus result = LLKeyframeMotion::onInitialize(character);

	for (U32 jm=0; jm<mJointMotionList->mNumJointMotions; jm++)
	{
		if (!mJointStates[jm].getJoint())
			continue;
		if (mJointStates[jm].getJoint()->getName() == std::string("mPelvis"))
		{
			mPelvisStatep = &mJointStates[jm];
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// LLKeyframeFallMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeFallMotion::onActivate()
{
	LLVector3 ground_pos;
	LLVector3 ground_normal;
	LLQuaternion inverse_pelvis_rot;
	LLVector3 fwd_axis(1.f, 0.f, 0.f);

	mVelocityZ = -mCharacter->getCharacterVelocity().mV[VZ];
	mCharacter->getGround( mCharacter->getCharacterPosition(), ground_pos, ground_normal);
	ground_normal.normVec();

	inverse_pelvis_rot = mCharacter->getCharacterRotation();
	inverse_pelvis_rot.transQuat();

	// find ground normal in pelvis space
	ground_normal = ground_normal * inverse_pelvis_rot;

	// calculate new foward axis
	fwd_axis = fwd_axis - (ground_normal * (ground_normal * fwd_axis));
	fwd_axis.normVec();
	mRotationToGroundNormal = LLQuaternion(fwd_axis, ground_normal % fwd_axis, ground_normal);

	return LLKeyframeMotion::onActivate();
}

//-----------------------------------------------------------------------------
// LLKeyframeFallMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeFallMotion::onUpdate(F32 activeTime, U8* joint_mask)
{
	BOOL result = LLKeyframeMotion::onUpdate(activeTime, joint_mask);
	F32  slerp_amt = clamp_rescale(activeTime / getDuration(), 0.5f, 0.75f, 0.f, 1.f);

	mPelvisStatep->setRotation(mPelvisStatep->getRotation() * slerp(slerp_amt, mRotationToGroundNormal, LLQuaternion()));

	return result;
}

//-----------------------------------------------------------------------------
// LLKeyframeFallMotion::getEaseInDuration()
//-----------------------------------------------------------------------------
F32 LLKeyframeFallMotion::getEaseInDuration()
{
	if (mVelocityZ == 0.f)
	{
		// we've already hit the ground
		return 0.4f;
	}

	return mCharacter->getPreferredPelvisHeight() / mVelocityZ;
}

// End
