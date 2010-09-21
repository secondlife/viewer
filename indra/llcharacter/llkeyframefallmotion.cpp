/** 
 * @file llkeyframefallmotion.cpp
 * @brief Implementation of LLKeyframeFallMotion class.
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

	for (U32 jm=0; jm<mJointMotionList->getNumJointMotions(); jm++)
	{
		if (!mJointStates[jm]->getJoint())
			continue;
		if (mJointStates[jm]->getJoint()->getName() == std::string("mPelvis"))
		{
			mPelvisState = mJointStates[jm];
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

	if (mPelvisState.notNull())
	{
		mPelvisState->setRotation(mPelvisState->getRotation() * slerp(slerp_amt, mRotationToGroundNormal, LLQuaternion()));
	}
	
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
