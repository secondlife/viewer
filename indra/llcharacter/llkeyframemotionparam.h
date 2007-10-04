/** 
 * @file llkeyframemotionparam.h
 * @brief Implementation of LLKeframeMotionParam class.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLKEYFRAMEMOTIONPARAM_H
#define LL_LLKEYFRAMEMOTIONPARAM_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------

#include <string>

#include "llmotion.h"
#include "lljointstate.h"
#include "v3math.h"
#include "llquaternion.h"
#include "linked_lists.h"
#include "llkeyframemotion.h"

//-----------------------------------------------------------------------------
// class LLKeyframeMotionParam
//-----------------------------------------------------------------------------
class LLKeyframeMotionParam :
	public LLMotion
{
public:
	// Constructor
	LLKeyframeMotionParam(const LLUUID &id);

	// Destructor
	virtual ~LLKeyframeMotionParam();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLKeyframeMotionParam(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual BOOL getLoop() {
		return TRUE;
	}

	// motions must report their total duration
	virtual F32 getDuration() { 
		return mDuration;
	}

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { 
		return mEaseInDuration;
	}

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { 
		return mEaseOutDuration;
	}

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { 
		return mPriority;
	}

	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_KEYFRAME; }

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

	virtual LLPose* getPose() { return mPoseBlender.getBlendedPose();}

protected:
	//-------------------------------------------------------------------------
	// new functions defined by this subclass
	//-------------------------------------------------------------------------
	typedef std::pair<LLMotion*, F32> ParameterizedMotion;
	
	// add a motion and associated parameter triplet
	BOOL addKeyframeMotion(char *name, const LLUUID &id, char *param, F32 value);
	
	// set default motion for LOD and retrieving blend constants
	void setDefaultKeyframeMotion(char *);

	static BOOL sortFunc(ParameterizedMotion *new_motion, ParameterizedMotion *tested_motion);

	BOOL loadMotions();

protected:
	//-------------------------------------------------------------------------
	// Member Data
	//-------------------------------------------------------------------------

	typedef LLLinkedList < ParameterizedMotion >	motion_list_t;
	LLAssocList <std::string, motion_list_t* > mParameterizedMotions;
	LLJointState*		mJointStates;
	LLMotion*			mDefaultKeyframeMotion;
	LLCharacter*		mCharacter;
	LLPoseBlender		mPoseBlender;

	F32					mEaseInDuration;
	F32					mEaseOutDuration;
	F32					mDuration;
	LLJoint::JointPriority	mPriority;

	LLUUID				mTransactionID;
};

#endif // LL_LLKEYFRAMEMOTIONPARAM_H
