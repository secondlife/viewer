/** 
 * @file llkeyframestandmotion.h
 * @brief Implementation of LLKeyframeStandMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLKEYFRAMESTANDMOTION_H
#define LL_LLKEYFRAMESTANDMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llkeyframemotion.h"
#include "lljointsolverrp3.h"


//-----------------------------------------------------------------------------
// class LLKeyframeStandMotion
//-----------------------------------------------------------------------------
class LLKeyframeStandMotion :
	public LLKeyframeMotion
{
public:
	// Constructor
	LLKeyframeStandMotion(const LLUUID &id);

	// Destructor
	virtual ~LLKeyframeStandMotion();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLKeyframeStandMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onActivate();
	void	onDeactivate();
	virtual BOOL onUpdate(F32 time, U8* joint_mask);

public:
	//-------------------------------------------------------------------------
	// Member Data
	//-------------------------------------------------------------------------
	LLCharacter	*mCharacter;

	BOOL				mFlipFeet;

	LLJointState		*mPelvisState;

	LLJointState		*mHipLeftState;
	LLJointState		*mKneeLeftState;
	LLJointState		*mAnkleLeftState;

	LLJointState		*mHipRightState;
	LLJointState		*mKneeRightState;
	LLJointState		*mAnkleRightState;

	LLJoint				mPelvisJoint;

	LLJoint				mHipLeftJoint;
	LLJoint				mKneeLeftJoint;
	LLJoint				mAnkleLeftJoint;
	LLJoint				mTargetLeft;

	LLJoint				mHipRightJoint;
	LLJoint				mKneeRightJoint;
	LLJoint				mAnkleRightJoint;
	LLJoint				mTargetRight;

	LLJointSolverRP3	mIKLeft;
	LLJointSolverRP3	mIKRight;

	LLVector3			mPositionLeft;
	LLVector3			mPositionRight;
	LLVector3			mNormalLeft;
	LLVector3			mNormalRight;
	LLQuaternion		mRotationLeft;
	LLQuaternion		mRotationRight;

	LLQuaternion		mLastGoodPelvisRotation;
	LLVector3			mLastGoodPosition;
	BOOL				mTrackAnkles;

	S32					mFrameNum;
};

#endif // LL_LLKEYFRAMESTANDMOTION_H

