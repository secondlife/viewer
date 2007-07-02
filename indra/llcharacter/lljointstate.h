/** 
 * @file lljointstate.h
 * @brief Implementation of LLJointState class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLJOINTSTATE_H
#define LL_LLJOINTSTATE_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "lljoint.h"

//-----------------------------------------------------------------------------
// class LLJointState
//-----------------------------------------------------------------------------
class LLJointState
{
public:
	enum BlendPhase
	{
		INACTIVE,
		EASE_IN,
		ACTIVE,
		EASE_OUT
	};
protected:
	// associated joint
	LLJoint	*mJoint;

	// indicates which members are used
	U32		mUsage;

	// indicates weighted effect of this joint 
	F32		mWeight;

	// transformation members
	LLVector3		mPosition;	// position relative to parent joint
	LLQuaternion	mRotation;	// joint rotation relative to parent joint
	LLVector3		mScale;		// scale relative to rotated frame
	LLJoint::JointPriority	mPriority;  // how important this joint state is relative to others
public:
	// Constructor
	LLJointState()
	{
		mUsage = 0;
		mJoint = NULL;
		mUsage = 0;
		mWeight = 0.f;
		mPriority = LLJoint::USE_MOTION_PRIORITY;
	}

	LLJointState(LLJoint* joint)
	{
		mUsage = 0;
		mJoint = joint;
		mUsage = 0;
		mWeight = 0.f;
		mPriority = LLJoint::USE_MOTION_PRIORITY;
	}

	// Destructor
	virtual ~LLJointState()
	{
	}

	// joint that this state is applied to
	LLJoint *getJoint()				{ return mJoint; }
	BOOL setJoint( LLJoint *joint )	{ mJoint = joint; return mJoint != NULL; }

	// transform type (bitwise flags can be combined)
	// Note that these are set automatically when various
	// member setPos/setRot/setScale functions are called.
	enum Usage
	{
		POS		= 1,
		ROT		= 2,
		SCALE	= 4,
	};
	U32 getUsage()			{ return mUsage; }
	void setUsage( U32 usage )	{ mUsage = usage; }
	F32 getWeight()			{ return mWeight; }
	void setWeight( F32 weight )	{ mWeight = weight; }

	// get/set position
	const LLVector3& getPosition()				{ return mPosition; }
	void setPosition( const LLVector3& pos )	{ llassert(mUsage & POS); mPosition = pos; }

	// get/set rotation
	const LLQuaternion& getRotation()			{ return mRotation; }
	void setRotation( const LLQuaternion& rot )	{ llassert(mUsage & ROT); mRotation = rot; }

	// get/set scale
	const LLVector3& getScale()				{ return mScale; }
	void setScale( const LLVector3& scale )	{ llassert(mUsage & SCALE); mScale = scale; }

	// get/set priority
	LLJoint::JointPriority getPriority()		{ return mPriority; }
	void setPriority( const LLJoint::JointPriority priority ) { mPriority = priority; }
};

#endif // LL_LLJOINTSTATE_H

