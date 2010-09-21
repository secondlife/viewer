/** 
 * @file llkeyframestandmotion.h
 * @brief Implementation of LLKeyframeStandMotion class.
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

	LLPointer<LLJointState>	mPelvisState;

	LLPointer<LLJointState>	mHipLeftState;
	LLPointer<LLJointState>	mKneeLeftState;
	LLPointer<LLJointState>	mAnkleLeftState;

	LLPointer<LLJointState>	mHipRightState;
	LLPointer<LLJointState>	mKneeRightState;
	LLPointer<LLJointState>	mAnkleRightState;

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

