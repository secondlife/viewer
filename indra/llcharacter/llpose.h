/** 
 * @file llpose.h
 * @brief Implementation of LLPose class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLPOSE_H
#define LL_LLPOSE_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include <string>

#include "linked_lists.h"
#include "llmap.h"
#include "lljointstate.h"
#include "llassoclist.h"
#include "lljoint.h"
#include <map>


//-----------------------------------------------------------------------------
// class LLPose
//-----------------------------------------------------------------------------
class LLPose
{
	friend class LLPoseBlender;
protected:
	typedef std::map<std::string, LLJointState*> joint_map;
	typedef joint_map::iterator joint_map_iterator;
	typedef joint_map::value_type joint_map_value_type;
	
	joint_map					mJointMap;
	F32							mWeight;
	joint_map_iterator			mListIter;
public:
	// Iterate through jointStates
	LLJointState *getFirstJointState();
	LLJointState *getNextJointState();
	LLJointState *findJointState(LLJoint *joint);
	LLJointState *findJointState(const std::string &name);
public:
	// Constructor
	LLPose() : mWeight(0.f) {}
	// Destructor
	~LLPose();
	// add a joint state in this pose
	BOOL addJointState(LLJointState *jointState);
	// remove a joint state from this pose
	BOOL removeJointState(LLJointState *jointState);
	// removes all joint states from this pose
	BOOL removeAllJointStates();
	// set weight for all joint states in this pose
	void setWeight(F32 weight);
	// get weight for this pose
	F32 getWeight() const;
	// returns number of joint states stored in this pose
	S32 getNumJointStates() const;
};

const S32 JSB_NUM_JOINT_STATES = 6;

class LLJointStateBlender
{
protected:
	LLJointState*	mJointStates[JSB_NUM_JOINT_STATES];
	S32				mPriorities[JSB_NUM_JOINT_STATES];
	BOOL			mAdditiveBlends[JSB_NUM_JOINT_STATES];
public:
	LLJointStateBlender();
	~LLJointStateBlender();
	void blendJointStates(BOOL apply_now = TRUE);
	BOOL addJointState(LLJointState *joint_state, S32 priority, BOOL additive_blend);
	void interpolate(F32 u);
	void clear();
	void resetCachedJoint();

public:
	LLJoint mJointCache;
};

class LLMotion;

class LLPoseBlender
{
protected:
	typedef std::list<LLJointStateBlender*> blender_list_t;
	typedef std::map<LLJoint*,LLJointStateBlender*> blender_map_t;
	blender_map_t mJointStateBlenderPool;
	blender_list_t mActiveBlenders;

	S32			mNextPoseSlot;
	LLPose		mBlendedPose;
public:
	// Constructor
	LLPoseBlender();
	// Destructor
	~LLPoseBlender();
	
	// request motion joint states to be added to pose blender joint state records
	BOOL addMotion(LLMotion* motion);

	// blend all joint states and apply to skeleton
	void blendAndApply();

	// removes all joint state blenders from last time
	void clearBlenders();

	// blend all joint states and cache results
	void blendAndCache(BOOL reset_cached_joints);

	// interpolate all joints towards cached values
	void interpolate(F32 u);

	LLPose* getBlendedPose() { return &mBlendedPose; }
};

#endif // LL_LLPOSE_H

