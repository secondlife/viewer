/** 
 * @file lltargetingmotion.h
 * @brief Implementation of LLTargetingMotion class.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTARGETINGMOTION_H
#define LL_LLTARGETINGMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"

#define TARGETING_EASEIN_DURATION	0.3f
#define TARGETING_EASEOUT_DURATION 0.5f
#define TARGETING_PRIORITY LLJoint::HIGH_PRIORITY
#define MIN_REQUIRED_PIXEL_AREA_TARGETING 1000.f;


//-----------------------------------------------------------------------------
// class LLTargetingMotion
//-----------------------------------------------------------------------------
class LLTargetingMotion :
	public LLMotion
{
public:
	// Constructor
	LLTargetingMotion(const LLUUID &id);

	// Destructor
	virtual ~LLTargetingMotion();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLTargetingMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual BOOL getLoop() { return TRUE; }

	// motions must report their total duration
	virtual F32 getDuration() { return 0.0; }

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { return TARGETING_EASEIN_DURATION; }

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { return TARGETING_EASEOUT_DURATION; }

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { return TARGETING_PRIORITY; }

	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_TARGETING; }

	// run-time (post constructor) initialization,
	// called after parameters have been set
	// must return true to indicate success and be available for activation
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);

	// called when a motion is activated
	// must return TRUE to indicate success, or else
	// it will be deactivated
	virtual BOOL onActivate();

	// called per time step
	// must return TRUE while it is active, and
	// must return FALSE when the motion is completed.
	virtual BOOL onUpdate(F32 time, U8* joint_mask);

	// called when a motion is deactivated
	virtual void onDeactivate();

public:

	LLCharacter			*mCharacter;
	LLJointState		mTorsoState;
	LLJoint*			mPelvisJoint;
	LLJoint*			mTorsoJoint;
	LLJoint*			mRightHandJoint;
};

#endif // LL_LLTARGETINGMOTION_H

