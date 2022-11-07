/** 
 * @file llkeyframestandmotion.cpp
 * @brief Implementation of LLKeyframeStandMotion class.
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

#include "llkeyframestandmotion.h"
#include "llcharacter.h"

//-----------------------------------------------------------------------------
// Macros and consts
//-----------------------------------------------------------------------------
#define GO_TO_KEY_POSE  1
#define MIN_TRACK_SPEED 0.01f
const F32 ROTATION_THRESHOLD = 0.6f;
const F32 POSITION_THRESHOLD = 0.1f;

//-----------------------------------------------------------------------------
// LLKeyframeStandMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLKeyframeStandMotion::LLKeyframeStandMotion(const LLUUID &id) : LLKeyframeMotion(id)
{
    mFlipFeet = FALSE;
    mCharacter = NULL;

    // create kinematic hierarchy
    mPelvisJoint.addChild( &mHipLeftJoint );
        mHipLeftJoint.addChild( &mKneeLeftJoint );
            mKneeLeftJoint.addChild( &mAnkleLeftJoint );
    mPelvisJoint.addChild( &mHipRightJoint );
        mHipRightJoint.addChild( &mKneeRightJoint );
            mKneeRightJoint.addChild( &mAnkleRightJoint );

    mPelvisState = NULL;

    mHipLeftState =  NULL;
    mKneeLeftState =  NULL;
    mAnkleLeftState =  NULL;

    mHipRightState =  NULL;
    mKneeRightState =  NULL;
    mAnkleRightState =  NULL;

    mTrackAnkles = TRUE;

    mFrameNum = 0;
}


//-----------------------------------------------------------------------------
// ~LLKeyframeStandMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLKeyframeStandMotion::~LLKeyframeStandMotion()
{
}


//-----------------------------------------------------------------------------
// LLKeyframeStandMotion::onInitialize()
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLKeyframeStandMotion::onInitialize(LLCharacter *character)
{
    // save character pointer for later use
    mCharacter = character;

    mFlipFeet = FALSE;

    // load keyframe data, setup pose and joint states
    LLMotion::LLMotionInitStatus status = LLKeyframeMotion::onInitialize(character);
    if ( status == STATUS_FAILURE )
    {
        return status;
    }

    // find the necessary joint states
    LLPose *pose = getPose();
    mPelvisState = pose->findJointState("mPelvis");
    
    mHipLeftState = pose->findJointState("mHipLeft");
    mKneeLeftState = pose->findJointState("mKneeLeft");
    mAnkleLeftState = pose->findJointState("mAnkleLeft");

    mHipRightState = pose->findJointState("mHipRight");
    mKneeRightState = pose->findJointState("mKneeRight");
    mAnkleRightState = pose->findJointState("mAnkleRight");

    if (    !mPelvisState ||
            !mHipLeftState ||
            !mKneeLeftState ||
            !mAnkleLeftState ||
            !mHipRightState ||
            !mKneeRightState ||
            !mAnkleRightState )
    {
        LL_INFOS() << getName() << ": Can't find necessary joint states" << LL_ENDL;
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// LLKeyframeStandMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeStandMotion::onActivate()
{
    //-------------------------------------------------------------------------
    // setup the IK solvers
    //-------------------------------------------------------------------------
    mIKLeft.setPoleVector( LLVector3(1.0f, 0.0f, 0.0f));
    mIKRight.setPoleVector( LLVector3(1.0f, 0.0f, 0.0f));
    mIKLeft.setBAxis( LLVector3(0.05f, 1.0f, 0.0f));
    mIKRight.setBAxis( LLVector3(-0.05f, 1.0f, 0.0f));

    mLastGoodPelvisRotation.loadIdentity();
    mLastGoodPosition.clearVec();

    mFrameNum = 0;

    return LLKeyframeMotion::onActivate();
}

//-----------------------------------------------------------------------------
// LLKeyframeStandMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLKeyframeStandMotion::onDeactivate()
{
    LLKeyframeMotion::onDeactivate();
}

//-----------------------------------------------------------------------------
// LLKeyframeStandMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeStandMotion::onUpdate(F32 time, U8* joint_mask)
{
    //-------------------------------------------------------------------------
    // let the base class update the cycle
    //-------------------------------------------------------------------------
    BOOL status = LLKeyframeMotion::onUpdate(time, joint_mask);
    if (!status)
    {
        return FALSE;
    }

    LLVector3 root_world_pos = mPelvisState->getJoint()->getParent()->getWorldPosition();

    // have we received a valid world position for this avatar?
    if (root_world_pos.isExactlyZero())
    {
        return TRUE;
    }

    //-------------------------------------------------------------------------
    // Stop tracking (start locking) ankles once ease in is done.
    // Setting this here ensures we track until we get valid foot position.
    //-------------------------------------------------------------------------
    if (dot(mPelvisState->getJoint()->getWorldRotation(), mLastGoodPelvisRotation) < ROTATION_THRESHOLD)
    {
        mLastGoodPelvisRotation = mPelvisState->getJoint()->getWorldRotation();
        mLastGoodPelvisRotation.normalize();
        mTrackAnkles = TRUE;
    }
    else if ((mCharacter->getCharacterPosition() - mLastGoodPosition).magVecSquared() > POSITION_THRESHOLD)
    {
        mLastGoodPosition = mCharacter->getCharacterPosition();
        mTrackAnkles = TRUE;
    }
    else if (mPose.getWeight() < 1.f)
    {
        mTrackAnkles = TRUE;
    }


    //-------------------------------------------------------------------------
    // propagate joint positions to internal versions
    //-------------------------------------------------------------------------
    // SL-315
    mPelvisJoint.setPosition(
            root_world_pos +
            mPelvisState->getPosition() );

    // SL-315
    mHipLeftJoint.setPosition( mHipLeftState->getJoint()->getPosition() );
    mKneeLeftJoint.setPosition( mKneeLeftState->getJoint()->getPosition() );
    mAnkleLeftJoint.setPosition( mAnkleLeftState->getJoint()->getPosition() );

    mHipLeftJoint.setScale( mHipLeftState->getJoint()->getScale() );
    mKneeLeftJoint.setScale( mKneeLeftState->getJoint()->getScale() );
    mAnkleLeftJoint.setScale( mAnkleLeftState->getJoint()->getScale() );

    // SL-315
    mHipRightJoint.setPosition( mHipRightState->getJoint()->getPosition() );
    mKneeRightJoint.setPosition( mKneeRightState->getJoint()->getPosition() );
    mAnkleRightJoint.setPosition( mAnkleRightState->getJoint()->getPosition() );

    mHipRightJoint.setScale( mHipRightState->getJoint()->getScale() );
    mKneeRightJoint.setScale( mKneeRightState->getJoint()->getScale() );
    mAnkleRightJoint.setScale( mAnkleRightState->getJoint()->getScale() );
    //-------------------------------------------------------------------------
    // propagate joint rotations to internal versions
    //-------------------------------------------------------------------------
    mPelvisJoint.setRotation( mPelvisState->getJoint()->getWorldRotation() );

#if GO_TO_KEY_POSE
    mHipLeftJoint.setRotation( mHipLeftState->getRotation() );
    mKneeLeftJoint.setRotation( mKneeLeftState->getRotation() );
    mAnkleLeftJoint.setRotation( mAnkleLeftState->getRotation() );

    mHipRightJoint.setRotation( mHipRightState->getRotation() );
    mKneeRightJoint.setRotation( mKneeRightState->getRotation() );
    mAnkleRightJoint.setRotation( mAnkleRightState->getRotation() );
#else
    mHipLeftJoint.setRotation( mHipLeftState->getJoint()->getRotation() );
    mKneeLeftJoint.setRotation( mKneeLeftState->getJoint()->getRotation() );
    mAnkleLeftJoint.setRotation( mAnkleLeftState->getJoint()->getRotation() );

    mHipRightJoint.setRotation( mHipRightState->getJoint()->getRotation() );
    mKneeRightJoint.setRotation( mKneeRightState->getJoint()->getRotation() );
    mAnkleRightJoint.setRotation( mAnkleRightState->getJoint()->getRotation() );
#endif

    // need to wait for underlying keyframe motion to affect the skeleton
    if (mFrameNum == 2)
    {
        mIKLeft.setupJoints( &mHipLeftJoint, &mKneeLeftJoint, &mAnkleLeftJoint, &mTargetLeft );
        mIKRight.setupJoints( &mHipRightJoint, &mKneeRightJoint, &mAnkleRightJoint, &mTargetRight );
    }
    else if (mFrameNum < 2)
    {
        mFrameNum++;
        return TRUE;
    }

    mFrameNum++;

    //-------------------------------------------------------------------------
    // compute target position by projecting ankles to the ground
    //-------------------------------------------------------------------------
    if ( mTrackAnkles )
    {
        mCharacter->getGround( mAnkleLeftJoint.getWorldPosition(), mPositionLeft, mNormalLeft);
        mCharacter->getGround( mAnkleRightJoint.getWorldPosition(), mPositionRight, mNormalRight);

        // SL-315
        mTargetLeft.setPosition( mPositionLeft );
        mTargetRight.setPosition( mPositionRight );
    }

    //-------------------------------------------------------------------------
    // update solvers
    //-------------------------------------------------------------------------
    mIKLeft.solve();
    mIKRight.solve();

    //-------------------------------------------------------------------------
    // make ankle rotation conform to the ground
    //-------------------------------------------------------------------------
    if ( mTrackAnkles )
    {
        LLVector4 dirLeft4 = mAnkleLeftJoint.getWorldMatrix().getFwdRow4();
        LLVector4 dirRight4 = mAnkleRightJoint.getWorldMatrix().getFwdRow4();
        LLVector3 dirLeft = vec4to3( dirLeft4 );
        LLVector3 dirRight = vec4to3( dirRight4 );

        LLVector3 up;
        LLVector3 dir;
        LLVector3 left;

        up = mNormalLeft;
        up.normVec();
        if (mFlipFeet)
        {
            up *= -1.0f;
        }
        dir = dirLeft;
        dir.normVec();
        left = up % dir;
        left.normVec();
        dir = left % up;
        mRotationLeft = LLQuaternion( dir, left, up );

        up = mNormalRight;
        up.normVec();
        if (mFlipFeet)
        {
            up *= -1.0f;
        }
        dir = dirRight;
        dir.normVec();
        left = up % dir;
        left.normVec();
        dir = left % up;
        mRotationRight = LLQuaternion( dir, left, up );
    }
    mAnkleLeftJoint.setWorldRotation( mRotationLeft );
    mAnkleRightJoint.setWorldRotation( mRotationRight );

    //-------------------------------------------------------------------------
    // propagate joint rotations to joint states
    //-------------------------------------------------------------------------
    mHipLeftState->setRotation( mHipLeftJoint.getRotation() );
    mKneeLeftState->setRotation( mKneeLeftJoint.getRotation() );
    mAnkleLeftState->setRotation( mAnkleLeftJoint.getRotation() );

    mHipRightState->setRotation( mHipRightJoint.getRotation() );
    mKneeRightState->setRotation( mKneeRightJoint.getRotation() );
    mAnkleRightState->setRotation( mAnkleRightJoint.getRotation() );

    //LL_INFOS() << "Stand drift amount " << (mCharacter->getCharacterPosition() - mLastGoodPosition).magVec() << LL_ENDL;

//  LL_INFOS() << "DEBUG: " << speed << " : " << mTrackAnkles << LL_ENDL;
    return TRUE;
}

// End
