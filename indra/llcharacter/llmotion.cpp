/**
 * @file llmotion.cpp
 * @brief Implementation of LLMotion class.
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
    mStopped(true),
    mActive(false),
    mID(id),
    mActivationTimestamp(0.f),
    mStopTimestamp(0.f),
    mSendStopTimestamp(F32_MAX),
    mResidualWeight(0.f),
    mFadeWeight(1.f),
    mDeactivateCallback(NULL),
    mDeactivateCallbackUserData(NULL)
{
    for (S32 i=0; i<3; ++i)
        memset(&mJointSignature[i][0], 0, sizeof(U8) * LL_CHARACTER_MAX_ANIMATED_JOINTS);
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
        mFadeWeight = lerp(mFadeWeight, 0.f, LLSmoothInterpolation::getInterpolant(0.15f));
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
        mFadeWeight = lerp(mFadeWeight, 1.f, LLSmoothInterpolation::getInterpolant(0.15f));
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
    S32 joint_num = jointState->getJoint()->getJointNum();
    if ((joint_num >= (S32)LL_CHARACTER_MAX_ANIMATED_JOINTS) || (joint_num < 0))
    {
        LL_WARNS() << "joint_num " << joint_num << " is outside of legal range [0-" << LL_CHARACTER_MAX_ANIMATED_JOINTS << ") for joint " << jointState->getJoint()->getName() << LL_ENDL;
        return;
    }
    mJointSignature[0][joint_num] = (usage & LLJointState::POS) ? (0xff >> (7 - priority)) : 0;
    mJointSignature[1][joint_num] = (usage & LLJointState::ROT) ? (0xff >> (7 - priority)) : 0;
    mJointSignature[2][joint_num] = (usage & LLJointState::SCALE) ? (0xff >> (7 - priority)) : 0;
}

void LLMotion::setDeactivateCallback( void (*cb)(void *), void* userdata )
{
    mDeactivateCallback = cb;
    mDeactivateCallbackUserData = userdata;
}

//virtual
void LLMotion::setStopTime(F32 time)
{
    mStopTimestamp = time;
    mStopped = true;
}

bool LLMotion::isBlending()
{
    return mPose.getWeight() < 1.f;
}

//-----------------------------------------------------------------------------
// activate()
//-----------------------------------------------------------------------------
void LLMotion::activate(F32 time)
{
    mActivationTimestamp = time;
    mStopped = false;
    mActive = true;
    onActivate();
}

//-----------------------------------------------------------------------------
// deactivate()
//-----------------------------------------------------------------------------
void LLMotion::deactivate()
{
    mActive = false;
    mPose.setWeight(0.f);

    if (mDeactivateCallback)
    {
        (*mDeactivateCallback)(mDeactivateCallbackUserData);
        mDeactivateCallback = NULL; // only call callback once
        mDeactivateCallbackUserData = NULL;
    }

    onDeactivate();
}

bool LLMotion::canDeprecate()
{
    return true;
}

// End

