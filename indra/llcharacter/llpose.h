/** 
 * @file llpose.h
 * @brief Implementation of LLPose class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

const S32 JSB_NUM_JOINT_STATES = 4;

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
	LLMap<LLJoint*,LLJointStateBlender*> mJointStateBlenderPool;
	LLLinkedList<LLJointStateBlender> mActiveBlenders;

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

