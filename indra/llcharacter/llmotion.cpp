/** 
 * @file llmotion.cpp
 * @brief Implementation of LLMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
LLMotion::LLMotion( const LLUUID &id )
{
	mActivationTimestamp = 0.f;
	mStopTimestamp = 0.f;
	mSendStopTimestamp = F32_MAX;
	mResidualWeight = 0.f;
	mFadeWeight = 1.f;
	mStopped = TRUE;
	mActive = FALSE;
	mDeactivateCallback = NULL;

	memset(&mJointSignature[0][0], 0, sizeof(U8) * LL_CHARACTER_MAX_JOINTS);
	memset(&mJointSignature[1][0], 0, sizeof(U8) * LL_CHARACTER_MAX_JOINTS);
	memset(&mJointSignature[2][0], 0, sizeof(U8) * LL_CHARACTER_MAX_JOINTS);	

	mID = id;
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
void LLMotion::addJointState(LLJointState* jointState)
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

	if (mDeactivateCallback) (*mDeactivateCallback)(mDeactivateCallbackUserData);

	onDeactivate();
}

// End
