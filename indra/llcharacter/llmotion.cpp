/** 
 * @file llmotion.cpp
 * @brief Implementation of LLMotion class.
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

#include "llmotion.h"
#include "llcriticaldamp.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLMotion class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLMotion::LLMotion( const LLUUID &id ) :
	mStopped(TRUE),
	mActive(FALSE),
	mID(id),
	mActivationTimestamp(0.f),
	mStopTimestamp(0.f),
	mSendStopTimestamp(F32_MAX),
	mResidualWeight(0.f),
	mFadeWeight(1.f),
	mDeactivateCallback(NULL),
	mDeactivateCallbackUserData(NULL)
{
	for (int i=0; i<3; ++i)
		memset(&mJointSignature[i][0], 0, sizeof(U8) * LL_CHARACTER_MAX_JOINTS);
}

//-----------------------------------------------------------------------------
// ~LLMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLMotion::~LLMotion()
{
}

//-----------------------------------------------------------------------------
// fadeOut()
//-----------------------------------------------------------------------------
void LLMotion::fadeOut()
{
	if (mFadeWeight > 0.01f)
	{
		mFadeWeight = lerp(mFadeWeight, 0.f, LLCriticalDamp::getInterpolant(0.15f));
	}
	else
	{
		mFadeWeight = 0.f;
	}
}

//-----------------------------------------------------------------------------
// fadeIn()
//-----------------------------------------------------------------------------
void LLMotion::fadeIn()
{
	if (mFadeWeight < 0.99f)
	{
		mFadeWeight = lerp(mFadeWeight, 1.f, LLCriticalDamp::getInterpolant(0.15f));
	}
	else
	{
		mFadeWeight = 1.f;
	}
}

//-----------------------------------------------------------------------------
// addJointState()
//-----------------------------------------------------------------------------
void LLMotion::addJointState(const LLPointer<LLJointState>& jointState)
{
	mPose.addJointState(jointState);
	S32 priority = jointState->getPriority();
	if (priority == LLJoint::USE_MOTION_PRIORITY)
	{
		priority = getPriority();	
	}

	U32 usage = jointState->getUsage();

	// for now, usage is everything
	mJointSignature[0][jointState->getJoint()->getJointNum()] = (usage & LLJointState::POS) ? (0xff >> (7 - priority)) : 0;
	mJointSignature[1][jointState->getJoint()->getJointNum()] = (usage & LLJointState::ROT) ? (0xff >> (7 - priority)) : 0;
	mJointSignature[2][jointState->getJoint()->getJointNum()] = (usage & LLJointState::SCALE) ? (0xff >> (7 - priority)) : 0;
}

void LLMotion::setDeactivateCallback( void (*cb)(void *), void* userdata )
{
	mDeactivateCallback = cb;
	mDeactivateCallbackUserData = userdata;
}

BOOL LLMotion::isBlending()
{
	return mPose.getWeight() < 1.f;
}

//-----------------------------------------------------------------------------
// activate()
//-----------------------------------------------------------------------------
void LLMotion::activate()
{
	mStopped = FALSE;
	mActive = TRUE;
	onActivate();
}

//-----------------------------------------------------------------------------
// deactivate()
//-----------------------------------------------------------------------------
void LLMotion::deactivate()
{
	mActive = FALSE;
	mPose.setWeight(0.f);

	if (mDeactivateCallback) (*mDeactivateCallback)(mDeactivateCallbackUserData);

	onDeactivate();
}

BOOL LLMotion::canDeprecate()
{
	return TRUE;
}

// End
