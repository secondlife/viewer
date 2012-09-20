/** 
 * @file lleditingmotion.cpp
 * @brief Implementation of LLEditingMotion class.
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

#include "lleditingmotion.h"
#include "llcharacter.h"
#include "llhandmotion.h"
#include "llcriticaldamp.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const LLQuaternion EDIT_MOTION_WRIST_ROTATION(F_PI_BY_TWO * 0.7f, LLVector3(1.0f, 0.0f, 0.0f));
const F32 TARGET_LAG_HALF_LIFE	= 0.1f;		// half-life of IK targeting
const F32 TORSO_LAG_HALF_LIFE = 0.2f;
const F32 MAX_TIME_DELTA = 2.f; //max two seconds a frame for calculating interpolation

S32 LLEditingMotion::sHandPose = LLHandMotion::HAND_POSE_RELAXED_R;
S32 LLEditingMotion::sHandPosePriority = 3;

//-----------------------------------------------------------------------------
// LLEditingMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLEditingMotion::LLEditingMotion( const LLUUID &id) : LLMotion(id)
{
	mCharacter = NULL;

	// create kinematic chain
	mParentJoint.addChild( &mShoulderJoint );
	mShoulderJoint.addChild( &mElbowJoint );
	mElbowJoint.addChild( &mWristJoint );

	mName = "editing";

	mParentState = new LLJointState;
	mShoulderState = new LLJointState;
	mElbowState = new LLJointState;
	mWristState = new LLJointState;
	mTorsoState = new LLJointState;
}


//-----------------------------------------------------------------------------
// ~LLEditingMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLEditingMotion::~LLEditingMotion()
{
}

//-----------------------------------------------------------------------------
// LLEditingMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLEditingMotion::onInitialize(LLCharacter *character)
{
	// save character for future use
	mCharacter = character;

	// make sure character skeleton is copacetic
	if (!mCharacter->getJoint("mShoulderLeft") ||
		!mCharacter->getJoint("mElbowLeft") ||
		!mCharacter->getJoint("mWristLeft"))
	{
		llwarns << "Invalid skeleton for editing motion!" << llendl;
		return STATUS_FAILURE;
	}

	// get the shoulder, elbow, wrist joints from the character
	mParentState->setJoint( mCharacter->getJoint("mShoulderLeft")->getParent() );
	mShoulderState->setJoint( mCharacter->getJoint("mShoulderLeft") );
	mElbowState->setJoint( mCharacter->getJoint("mElbowLeft") );
	mWristState->setJoint( mCharacter->getJoint("mWristLeft") );
	mTorsoState->setJoint( mCharacter->getJoint("mTorso"));

	if ( ! mParentState->getJoint() )
	{
		llinfos << getName() << ": Can't get parent joint." << llendl;
		return STATUS_FAILURE;
	}

	mWristOffset = LLVector3(0.0f, 0.2f, 0.0f);

	// add joint states to the pose
	mShoulderState->setUsage(LLJointState::ROT);
	mElbowState->setUsage(LLJointState::ROT);
	mTorsoState->setUsage(LLJointState::ROT);
	mWristState->setUsage(LLJointState::ROT);
	addJointState( mShoulderState );
	addJointState( mElbowState );
	addJointState( mTorsoState );
	addJointState( mWristState );

	// propagate joint positions to kinematic chain
	mParentJoint.setPosition(	mParentState->getJoint()->getWorldPosition() );
	mShoulderJoint.setPosition(	mShoulderState->getJoint()->getPosition() );
	mElbowJoint.setPosition(	mElbowState->getJoint()->getPosition() );
	mWristJoint.setPosition(	mWristState->getJoint()->getPosition() + mWristOffset );

	// propagate current joint rotations to kinematic chain
	mParentJoint.setRotation(	mParentState->getJoint()->getWorldRotation() );
	mShoulderJoint.setRotation(	mShoulderState->getJoint()->getRotation() );
	mElbowJoint.setRotation(	mElbowState->getJoint()->getRotation() );

	// connect the ikSolver to the chain
	mIKSolver.setPoleVector( LLVector3( -1.0f, 1.0f, 0.0f ) );
	// specifying the elbow's axis will prevent bad IK for the more
	// singular configurations, but the axis is limb-specific -- Leviathan 
	mIKSolver.setBAxis( LLVector3( -0.682683f, 0.0f, -0.730714f ) );
	mIKSolver.setupJoints( &mShoulderJoint, &mElbowJoint, &mWristJoint, &mTarget );

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// LLEditingMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLEditingMotion::onActivate()
{
	// propagate joint positions to kinematic chain
	mParentJoint.setPosition(	mParentState->getJoint()->getWorldPosition() );
	mShoulderJoint.setPosition(	mShoulderState->getJoint()->getPosition() );
	mElbowJoint.setPosition(	mElbowState->getJoint()->getPosition() );
	mWristJoint.setPosition(	mWristState->getJoint()->getPosition() + mWristOffset );

	// propagate current joint rotations to kinematic chain
	mParentJoint.setRotation(	mParentState->getJoint()->getWorldRotation() );
	mShoulderJoint.setRotation(	mShoulderState->getJoint()->getRotation() );
	mElbowJoint.setRotation(	mElbowState->getJoint()->getRotation() );

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLEditingMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLEditingMotion::onUpdate(F32 time, U8* joint_mask)
{
	LLVector3 focus_pt;
	LLVector3* pointAtPt = (LLVector3*)mCharacter->getAnimationData("PointAtPoint");


	BOOL result = TRUE;

	if (!pointAtPt)
	{
		focus_pt = mLastSelectPt;
		result = FALSE;
	}
	else
	{
		focus_pt = *pointAtPt;
		mLastSelectPt = focus_pt;
	}

	focus_pt += mCharacter->getCharacterPosition();

	// propagate joint positions to kinematic chain
	mParentJoint.setPosition(	mParentState->getJoint()->getWorldPosition() );
	mShoulderJoint.setPosition(	mShoulderState->getJoint()->getPosition() );
	mElbowJoint.setPosition(	mElbowState->getJoint()->getPosition() );
	mWristJoint.setPosition(	mWristState->getJoint()->getPosition() + mWristOffset );

	// propagate current joint rotations to kinematic chain
	mParentJoint.setRotation(	mParentState->getJoint()->getWorldRotation() );
	mShoulderJoint.setRotation(	mShoulderState->getJoint()->getRotation() );
	mElbowJoint.setRotation(	mElbowState->getJoint()->getRotation() );

	// update target position from character
	LLVector3 target = focus_pt - mParentJoint.getPosition();
	F32 target_dist = target.normVec();
	
	LLVector3 edit_plane_normal(1.f / F_SQRT2, 1.f / F_SQRT2, 0.f);
	edit_plane_normal.normVec();

	edit_plane_normal.rotVec(mTorsoState->getJoint()->getWorldRotation());
	
	F32 dot = edit_plane_normal * target;

	if (dot < 0.f)
	{
		target = target + (edit_plane_normal * (dot * 2.f));
		target.mV[VZ] += clamp_rescale(dot, 0.f, -1.f, 0.f, 5.f);
		target.normVec();
	}

	target = target * target_dist;
	if (!target.isFinite())
	{
		// Don't error out here, set a fail-safe target vector
		llwarns << "Non finite target in editing motion with target distance of " << target_dist << 
			" and focus point " << focus_pt << llendl;
		target.setVec(1.f, 1.f, 1.f);
	}
	
	mTarget.setPosition( target + mParentJoint.getPosition());

//	llinfos << "Point At: " << mTarget.getPosition() << llendl;

	// update the ikSolver
	if (!mTarget.getPosition().isExactlyZero())
	{
		LLQuaternion shoulderRot = mShoulderJoint.getRotation();
		LLQuaternion elbowRot = mElbowJoint.getRotation();
		mIKSolver.solve();

		// use blending...
		F32 slerp_amt = LLCriticalDamp::getInterpolant(TARGET_LAG_HALF_LIFE);
		shoulderRot = slerp(slerp_amt, mShoulderJoint.getRotation(), shoulderRot);
		elbowRot = slerp(slerp_amt, mElbowJoint.getRotation(), elbowRot);

		// now put blended values back into joints
		llassert(shoulderRot.isFinite());
		llassert(elbowRot.isFinite());
		mShoulderState->setRotation(shoulderRot);
		mElbowState->setRotation(elbowRot);
		mWristState->setRotation(LLQuaternion::DEFAULT);
	}

	mCharacter->setAnimationData("Hand Pose", &sHandPose);
	mCharacter->setAnimationData("Hand Pose Priority", &sHandPosePriority);
	return result;
}

//-----------------------------------------------------------------------------
// LLEditingMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLEditingMotion::onDeactivate()
{
}


// End
