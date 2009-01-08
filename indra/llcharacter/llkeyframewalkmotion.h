/** 
 * @file llkeyframewalkmotion.h
 * @brief Implementation of LLKeframeWalkMotion class.
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

#ifndef LL_LLKEYFRAMEWALKMOTION_H
#define LL_LLKEYFRAMEWALKMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llkeyframemotion.h"
#include "llcharacter.h"
#include "v3dmath.h"

#define MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST (20.f)
#define MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST (20.f)

//-----------------------------------------------------------------------------
// class LLKeyframeWalkMotion
//-----------------------------------------------------------------------------
class LLKeyframeWalkMotion :
	public LLKeyframeMotion
{
	friend class LLWalkAdjustMotion;
public:
	// Constructor
	LLKeyframeWalkMotion(const LLUUID &id);

	// Destructor
	virtual ~LLKeyframeWalkMotion();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------
	
	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLKeyframeWalkMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onActivate();
	virtual void onDeactivate();
	virtual BOOL onUpdate(F32 time, U8* joint_mask);

public:
	//-------------------------------------------------------------------------
	// Member Data
	//-------------------------------------------------------------------------
	LLCharacter	*mCharacter;
	F32			mCyclePhase;
	F32			mRealTimeLast;
	F32			mAdjTimeLast;
	S32			mDownFoot;
};

class LLWalkAdjustMotion : public LLMotion
{
public:
	// Constructor
	LLWalkAdjustMotion(const LLUUID &id);

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLWalkAdjustMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onActivate();
	virtual void onDeactivate();
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
	virtual LLJoint::JointPriority getPriority(){return LLJoint::HIGH_PRIORITY;}
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.f; }
	virtual F32 getEaseInDuration() { return 0.f; }
	virtual F32 getEaseOutDuration() { return 0.f; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_WALK_ADJUST; }
	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

public:
	//-------------------------------------------------------------------------
	// Member Data
	//-------------------------------------------------------------------------
	LLCharacter		*mCharacter;
	LLJoint*		mLeftAnkleJoint;
	LLJoint*		mRightAnkleJoint;
	LLPointer<LLJointState>	mPelvisState;
	LLJoint*		mPelvisJoint;
	LLVector3d		mLastLeftAnklePos;
	LLVector3d		mLastRightAnklePos;
	F32				mLastTime;
	F32				mAvgCorrection;
	F32				mSpeedAdjust;
	F32				mAnimSpeed;
	F32				mAvgSpeed;
	F32				mRelativeDir;
	LLVector3		mPelvisOffset;
	F32				mAnkleOffset;
};

class LLFlyAdjustMotion : public LLMotion
{
public:
	// Constructor
	LLFlyAdjustMotion(const LLUUID &id);

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLFlyAdjustMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onActivate();
	virtual void onDeactivate() {};
	virtual BOOL onUpdate(F32 time, U8* joint_mask);
	virtual LLJoint::JointPriority getPriority(){return LLJoint::HIGHER_PRIORITY;}
	virtual BOOL getLoop() { return TRUE; }
	virtual F32 getDuration() { return 0.f; }
	virtual F32 getEaseInDuration() { return 0.f; }
	virtual F32 getEaseOutDuration() { return 0.f; }
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_FLY_ADJUST; }
	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

protected:
	//-------------------------------------------------------------------------
	// Member Data
	//-------------------------------------------------------------------------
	LLCharacter		*mCharacter;
	LLPointer<LLJointState>	mPelvisState;
	F32				mRoll;
};

#endif // LL_LLKeyframeWalkMotion_H

