/** 
 * @file llbreastmotion.h
 * @brief Implementation of LLBreastMotion class.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLBREASTMOTION_H
#define LL_LLBREASTMOTION_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "llframetimer.h"

#define BREAST_MOTION_FADEIN_TIME 1.0f
#define BREAST_MOTION_FADEOUT_TIME 1.0f

class LLViewerVisualParam;

//-----------------------------------------------------------------------------
// class LLBreastMotion
//-----------------------------------------------------------------------------
class LLBreastMotion :
	public LLMotion
{
public:
	// Constructor
	LLBreastMotion(const LLUUID &id);

	// Destructor
	virtual ~LLBreastMotion();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLBreastMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual BOOL getLoop() { return TRUE; }

	// motions must report their total duration
	virtual F32 getDuration() { return 0.0; }

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { return BREAST_MOTION_FADEIN_TIME; }

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { return BREAST_MOTION_FADEOUT_TIME; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea();

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }

	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

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

protected:
	LLVector3 toLocal(const LLVector3 &world_vector);
	LLVector3 calculateVelocity_local(const F32 time_delta);
	LLVector3 calculateAcceleration_local(const LLVector3 &new_char_velocity_local_vec,
										  const F32 time_delta);
	F32 calculateTimeDelta();
private:
	//-------------------------------------------------------------------------
	// joint states to be animated
	//-------------------------------------------------------------------------
	LLPointer<LLJointState> mChestState;
	LLCharacter*		mCharacter;


	//-------------------------------------------------------------------------
	// miscellaneous parameters
	//-------------------------------------------------------------------------
	LLViewerVisualParam *mBreastParamsUser[3];
	LLViewerVisualParam *mBreastParamsDriven[3];
	LLVector3           mBreastParamsMin;
	LLVector3           mBreastParamsMax;

	LLVector3           mCharLastPosition_world_pt; // Last position of the avatar
	LLVector3			mCharLastVelocity_local_vec; // How fast the character is moving
	LLVector3           mCharLastAcceleration_local_vec; // Change in character velocity

	LLVector3           mBreastLastPosition_local_pt; // Last parameters for breast
	LLVector3           mBreastVelocity_local_vec; // How fast the breast params are moving
	LLVector3           mBreastLastUpdatePosition_local_pt; // Last parameters when visual update was sent


	F32 mBreastMassParam;
	F32 mBreastGravityParam;
	U32 mBreastSmoothingParam;

	LLVector3 mBreastSpringParam;
	LLVector3 mBreastDampingParam;
	LLVector3 mBreastGainParam;
	LLVector3 mBreastMaxVelocityParam;
	LLVector3 mBreastDragParam;

	LLFrameTimer	mTimer;
	F32             mLastTime;
	
	U32            mFileTicks;
};

#endif // LL_LLBREASTMOTION_H

