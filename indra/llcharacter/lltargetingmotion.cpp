/** 
 * @file lltargetingmotion.cpp
 * @brief Implementation of LLTargetingMotion class.
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

#include "lltargetingmotion.h"
#include "llcharacter.h"
#include "v3dmath.h"
#include "llcriticaldamp.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const F32 TORSO_TARGET_HALF_LIFE = 0.25f;

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
        LL_WARNS() << "Invalid skeleton for targeting motion!" << LL_ENDL;
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
    LL_PROFILE_ZONE_SCOPED;
    F32 slerp_amt = LLSmoothInterpolation::getInterpolant(TORSO_TARGET_HALF_LIFE);

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

