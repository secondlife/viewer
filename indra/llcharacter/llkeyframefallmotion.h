/** 
 * @file llkeyframefallmotion.h
 * @brief Implementation of LLKeframeWalkMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLKeyframeFallMotion_H
#define LL_LLKeyframeFallMotion_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llkeyframemotion.h"
#include "llcharacter.h"

//-----------------------------------------------------------------------------
// class LLKeyframeFallMotion
//-----------------------------------------------------------------------------
class LLKeyframeFallMotion :
	public LLKeyframeMotion
{
public:
	// Constructor
	LLKeyframeFallMotion(const LLUUID &id);

	// Destructor
	virtual ~LLKeyframeFallMotion();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLKeyframeFallMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);
	virtual BOOL onActivate();
	virtual F32 getEaseInDuration();
	virtual BOOL onUpdate(F32 activeTime, U8* joint_mask);

protected:
	//-------------------------------------------------------------------------
	// Member Data
	//-------------------------------------------------------------------------
	LLCharacter*	mCharacter;
	F32				mVelocityZ;
	LLJointState*	mPelvisStatep;
	LLQuaternion	mRotationToGroundNormal;
};

#endif // LL_LLKeyframeFallMotion_H

