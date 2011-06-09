/** 
 * @file llkeyframemotion.cpp
 * @brief Implementation of LLKeyframeMotion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llmath.h"
#include "llanimationstates.h"
#include "llassetstorage.h"
#include "lldatapacker.h"
#include "llcharacter.h"
#include "llcriticaldamp.h"
#include "lldir.h"
#include "llendianswizzle.h"
#include "llkeyframemotion.h"
#include "llquantize.h"
#include "llvfile.h"
#include "m3math.h"
#include "message.h"

//-----------------------------------------------------------------------------
// Static Definitions
//-----------------------------------------------------------------------------
LLVFS*				LLKeyframeMotion::sVFS = NULL;
LLKeyframeDataCache::keyframe_data_map_t	LLKeyframeDataCache::sKeyframeDataMap;

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
static F32 JOINT_LENGTH_K = 0.7f;
static S32 MAX_ITERATIONS = 20;
static S32 MIN_ITERATIONS = 1;
static S32 MIN_ITERATION_COUNT = 2;
static F32 MAX_PIXEL_AREA_CONSTRAINTS = 80000.f;
static F32 MIN_PIXEL_AREA_CONSTRAINTS = 1000.f;
static F32 MIN_ACCELERATION_SQUARED = 0.0005f * 0.0005f;

static F32 MAX_CONSTRAINTS = 10;

//-----------------------------------------------------------------------------
// JointMotionList
//-----------------------------------------------------------------------------
LLKeyframeMotion::JointMotionList::JointMotionList()
	: mDuration(0.f),
	  mLoop(FALSE),
	  mLoopInPoint(0.f),
	  mLoopOutPoint(0.f),
	  mEaseInDuration(0.f),
	  mEaseOutDuration(0.f),
	  mBasePriority(LLJoint::LOW_PRIORITY),
	  mHandPose(LLHandMotion::HAND_POSE_SPREAD),
	  mMaxPriority(LLJoint::LOW_PRIORITY)
{
}

LLKeyframeMotion::JointMotionList::~JointMotionList()
{
	for_each(mConstraints.begin(), mConstraints.end(), DeletePointer());
	for_each(mJointMotionArray.begin(), mJointMotionArray.end(), DeletePointer());
}

U32 LLKeyframeMotion::JointMotionList::dumpDiagInfo()
{
	S32	total_size = sizeof(JointMotionList);

	for (U32 i = 0; i < getNumJointMotions(); i++)
	{
		LLKeyframeMotion::JointMotion* joint_motion_p = mJointMotionArray[i];

		llinfos << "\tJoint " << joint_motion_p->mJointName << llendl;
		if (joint_motion_p->mUsage & LLJointState::SCALE)
		{
			llinfos << "\t" << joint_motion_p->mScaleCurve.mNumKeys << " scale keys at " 
			<< joint_motion_p->mScaleCurve.mNumKeys * sizeof(ScaleKey) << " bytes" << llendl;

			total_size += joint_motion_p->mScaleCurve.mNumKeys * sizeof(ScaleKey);
		}
		if (joint_motion_p->mUsage & LLJointState::ROT)
		{
			llinfos << "\t" << joint_motion_p->mRotationCurve.mNumKeys << " rotation keys at " 
			<< joint_motion_p->mRotationCurve.mNumKeys * sizeof(RotationKey) << " bytes" << llendl;

			total_size += joint_motion_p->mRotationCurve.mNumKeys * sizeof(RotationKey);
		}
		if (joint_motion_p->mUsage & LLJointState::POS)
		{
			llinfos << "\t" << joint_motion_p->mPositionCurve.mNumKeys << " position keys at " 
			<< joint_motion_p->mPositionCurve.mNumKeys * sizeof(PositionKey) << " bytes" << llendl;

			total_size += joint_motion_p->mPositionCurve.mNumKeys * sizeof(PositionKey);
		}
	}
	llinfos << "Size: " << total_size << " bytes" << llendl;

	return total_size;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// ****Curve classes
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// ScaleCurve::ScaleCurve()
//-----------------------------------------------------------------------------
LLKeyframeMotion::ScaleCurve::ScaleCurve()
{
	mInterpolationType = LLKeyframeMotion::IT_LINEAR;
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// ScaleCurve::~ScaleCurve()
//-----------------------------------------------------------------------------
LLKeyframeMotion::ScaleCurve::~ScaleCurve() 
{
	mKeys.clear();
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// getValue()
//-----------------------------------------------------------------------------
LLVector3 LLKeyframeMotion::ScaleCurve::getValue(F32 time, F32 duration)
{
	LLVector3 value;

	if (mKeys.empty())
	{
		value.clearVec();
		return value;
	}
	
	key_map_t::iterator right = mKeys.lower_bound(time);
	if (right == mKeys.end())
	{
		// Past last key
		--right;
		value = right->second.mScale;
	}
	else if (right == mKeys.begin() || right->first == time)
	{
		// Before first key or exactly on a key
		value = right->second.mScale;
	}
	else
	{
		// Between two keys
		key_map_t::iterator left = right; --left;
		F32 index_before = left->first;
		F32 index_after = right->first;
		ScaleKey& scale_before = left->second;
		ScaleKey& scale_after = right->second;
		if (right == mKeys.end())
		{
			scale_after = mLoopInKey;
			index_after = duration;
		}

		F32 u = (time - index_before) / (index_after - index_before);
		value = interp(u, scale_before, scale_after);
	}
	return value;
}

//-----------------------------------------------------------------------------
// interp()
//-----------------------------------------------------------------------------
LLVector3 LLKeyframeMotion::ScaleCurve::interp(F32 u, ScaleKey& before, ScaleKey& after)
{
	switch (mInterpolationType)
	{
	case IT_STEP:
		return before.mScale;

	default:
	case IT_LINEAR:
	case IT_SPLINE:
		return lerp(before.mScale, after.mScale, u);
	}
}

//-----------------------------------------------------------------------------
// RotationCurve::RotationCurve()
//-----------------------------------------------------------------------------
LLKeyframeMotion::RotationCurve::RotationCurve()
{
	mInterpolationType = LLKeyframeMotion::IT_LINEAR;
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// RotationCurve::~RotationCurve()
//-----------------------------------------------------------------------------
LLKeyframeMotion::RotationCurve::~RotationCurve()
{
	mKeys.clear();
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// RotationCurve::getValue()
//-----------------------------------------------------------------------------
LLQuaternion LLKeyframeMotion::RotationCurve::getValue(F32 time, F32 duration)
{
	LLQuaternion value;

	if (mKeys.empty())
	{
		value = LLQuaternion::DEFAULT;
		return value;
	}
	
	key_map_t::iterator right = mKeys.lower_bound(time);
	if (right == mKeys.end())
	{
		// Past last key
		--right;
		value = right->second.mRotation;
	}
	else if (right == mKeys.begin() || right->first == time)
	{
		// Before first key or exactly on a key
		value = right->second.mRotation;
	}
	else
	{
		// Between two keys
		key_map_t::iterator left = right; --left;
		F32 index_before = left->first;
		F32 index_after = right->first;
		RotationKey& rot_before = left->second;
		RotationKey& rot_after = right->second;
		if (right == mKeys.end())
		{
			rot_after = mLoopInKey;
			index_after = duration;
		}

		F32 u = (time - index_before) / (index_after - index_before);
		value = interp(u, rot_before, rot_after);
	}
	return value;
}

//-----------------------------------------------------------------------------
// interp()
//-----------------------------------------------------------------------------
LLQuaternion LLKeyframeMotion::RotationCurve::interp(F32 u, RotationKey& before, RotationKey& after)
{
	switch (mInterpolationType)
	{
	case IT_STEP:
		return before.mRotation;

	default:
	case IT_LINEAR:
	case IT_SPLINE:
		return nlerp(u, before.mRotation, after.mRotation);
	}
}


//-----------------------------------------------------------------------------
// PositionCurve::PositionCurve()
//-----------------------------------------------------------------------------
LLKeyframeMotion::PositionCurve::PositionCurve()
{
	mInterpolationType = LLKeyframeMotion::IT_LINEAR;
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// PositionCurve::~PositionCurve()
//-----------------------------------------------------------------------------
LLKeyframeMotion::PositionCurve::~PositionCurve()
{
	mKeys.clear();
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// PositionCurve::getValue()
//-----------------------------------------------------------------------------
LLVector3 LLKeyframeMotion::PositionCurve::getValue(F32 time, F32 duration)
{
	LLVector3 value;

	if (mKeys.empty())
	{
		value.clearVec();
		return value;
	}
	
	key_map_t::iterator right = mKeys.lower_bound(time);
	if (right == mKeys.end())
	{
		// Past last key
		--right;
		value = right->second.mPosition;
	}
	else if (right == mKeys.begin() || right->first == time)
	{
		// Before first key or exactly on a key
		value = right->second.mPosition;
	}
	else
	{
		// Between two keys
		key_map_t::iterator left = right; --left;
		F32 index_before = left->first;
		F32 index_after = right->first;
		PositionKey& pos_before = left->second;
		PositionKey& pos_after = right->second;
		if (right == mKeys.end())
		{
			pos_after = mLoopInKey;
			index_after = duration;
		}

		F32 u = (time - index_before) / (index_after - index_before);
		value = interp(u, pos_before, pos_after);
	}

	llassert(value.isFinite());
	
	return value;
}

//-----------------------------------------------------------------------------
// interp()
//-----------------------------------------------------------------------------
LLVector3 LLKeyframeMotion::PositionCurve::interp(F32 u, PositionKey& before, PositionKey& after)
{
	switch (mInterpolationType)
	{
	case IT_STEP:
		return before.mPosition;
	default:
	case IT_LINEAR:
	case IT_SPLINE:
		return lerp(before.mPosition, after.mPosition, u);
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// JointMotion class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// JointMotion::update()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::JointMotion::update(LLJointState* joint_state, F32 time, F32 duration)
{
	// this value being 0 is the cause of https://jira.lindenlab.com/browse/SL-22678 but I haven't 
	// managed to get a stack to see how it got here. Testing for 0 here will stop the crash.
	if ( joint_state == NULL )
	{
		return;
	}

	U32 usage = joint_state->getUsage();

	//-------------------------------------------------------------------------
	// update scale component of joint state
	//-------------------------------------------------------------------------
	if ((usage & LLJointState::SCALE) && mScaleCurve.mNumKeys)
	{
		joint_state->setScale( mScaleCurve.getValue( time, duration ) );
	}

	//-------------------------------------------------------------------------
	// update rotation component of joint state
	//-------------------------------------------------------------------------
	if ((usage & LLJointState::ROT) && mRotationCurve.mNumKeys)
	{
		joint_state->setRotation( mRotationCurve.getValue( time, duration ) );
	}

	//-------------------------------------------------------------------------
	// update position component of joint state
	//-------------------------------------------------------------------------
	if ((usage & LLJointState::POS) && mPositionCurve.mNumKeys)
	{
		joint_state->setPosition( mPositionCurve.getValue( time, duration ) );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLKeyframeMotion class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLKeyframeMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLKeyframeMotion::LLKeyframeMotion(const LLUUID &id) 
	: LLMotion(id),
		mJointMotionList(NULL),
		mPelvisp(NULL),
		mLastSkeletonSerialNum(0),
		mLastUpdateTime(0.f),
		mLastLoopedTime(0.f),
		mAssetStatus(ASSET_UNDEFINED)
{

}


//-----------------------------------------------------------------------------
// ~LLKeyframeMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLKeyframeMotion::~LLKeyframeMotion()
{
	for_each(mConstraints.begin(), mConstraints.end(), DeletePointer());
}

//-----------------------------------------------------------------------------
// create()
//-----------------------------------------------------------------------------
LLMotion *LLKeyframeMotion::create(const LLUUID &id)
{
	return new LLKeyframeMotion(id);
}

//-----------------------------------------------------------------------------
// getJointState()
//-----------------------------------------------------------------------------
LLPointer<LLJointState>& LLKeyframeMotion::getJointState(U32 index)
{
	llassert_always (index < mJointStates.size());
	return mJointStates[index];
}

//-----------------------------------------------------------------------------
// getJoin()
//-----------------------------------------------------------------------------
LLJoint* LLKeyframeMotion::getJoint(U32 index)
{
	llassert_always (index < mJointStates.size());
	LLJoint* joint = mJointStates[index]->getJoint();
	llassert_always (joint);
	return joint;
}

//-----------------------------------------------------------------------------
// LLKeyframeMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLKeyframeMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	
	LLUUID* character_id;

	// asset already loaded?
	switch(mAssetStatus)
	{
	case ASSET_NEEDS_FETCH:
		// request asset
		mAssetStatus = ASSET_FETCHED;

		character_id = new LLUUID(mCharacter->getID());
		gAssetStorage->getAssetData(mID,
						LLAssetType::AT_ANIMATION,
						onLoadComplete,
						(void *)character_id,
						FALSE);

		return STATUS_HOLD;
	case ASSET_FETCHED:
		return STATUS_HOLD;
	case ASSET_FETCH_FAILED:
		return STATUS_FAILURE;
	case ASSET_LOADED:
		return STATUS_SUCCESS;
	default:
		// we don't know what state the asset is in yet, so keep going
		// check keyframe cache first then static vfs then asset request
		break;
	}

	LLKeyframeMotion::JointMotionList* joint_motion_list = LLKeyframeDataCache::getKeyframeData(getID());

	if(joint_motion_list)
	{
		// motion already existed in cache, so grab it
		mJointMotionList = joint_motion_list;

		mJointStates.reserve(mJointMotionList->getNumJointMotions());
		
		// don't forget to allocate joint states
		// set up joint states to point to character joints
		for(U32 i = 0; i < mJointMotionList->getNumJointMotions(); i++)
		{
			JointMotion* joint_motion = mJointMotionList->getJointMotion(i);
			if (LLJoint *joint = mCharacter->getJoint(joint_motion->mJointName))
			{
				LLPointer<LLJointState> joint_state = new LLJointState;
				mJointStates.push_back(joint_state);
				joint_state->setJoint(joint);
				joint_state->setUsage(joint_motion->mUsage);
				joint_state->setPriority(joint_motion->mPriority);
			}
			else
			{
				// add dummy joint state with no associated joint
				mJointStates.push_back(new LLJointState);
			}
		}
		mAssetStatus = ASSET_LOADED;
		setupPose();
		return STATUS_SUCCESS;
	}

	//-------------------------------------------------------------------------
	// Load named file by concatenating the character prefix with the motion name.
	// Load data into a buffer to be parsed.
	//-------------------------------------------------------------------------
	U8 *anim_data;
	S32 anim_file_size;

	if (!sVFS)
	{
		llerrs << "Must call LLKeyframeMotion::setVFS() first before loading a keyframe file!" << llendl;
	}

	BOOL success = FALSE;
	LLVFile* anim_file = new LLVFile(sVFS, mID, LLAssetType::AT_ANIMATION);
	if (!anim_file || !anim_file->getSize())
	{
		delete anim_file;
		anim_file = NULL;
		
		// request asset over network on next call to load
		mAssetStatus = ASSET_NEEDS_FETCH;

		return STATUS_HOLD;
	}
	else
	{
		anim_file_size = anim_file->getSize();
		anim_data = new U8[anim_file_size];
		success = anim_file->read(anim_data, anim_file_size);	/*Flawfinder: ignore*/
		delete anim_file;
		anim_file = NULL;
	}

	if (!success)
	{
		llwarns << "Can't open animation file " << mID << llendl;
		mAssetStatus = ASSET_FETCH_FAILED;
		return STATUS_FAILURE;
	}

	lldebugs << "Loading keyframe data for: " << getName() << ":" << getID() << " (" << anim_file_size << " bytes)" << llendl;

	LLDataPackerBinaryBuffer dp(anim_data, anim_file_size);

	if (!deserialize(dp))
	{
		llwarns << "Failed to decode asset for animation " << getName() << ":" << getID() << llendl;
		mAssetStatus = ASSET_FETCH_FAILED;
		return STATUS_FAILURE;
	}

	delete []anim_data;

	mAssetStatus = ASSET_LOADED;
	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// setupPose()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotion::setupPose()
{
	// add all valid joint states to the pose
	for (U32 jm=0; jm<mJointMotionList->getNumJointMotions(); jm++)
	{
		LLPointer<LLJointState> joint_state = getJointState(jm);
		if ( joint_state->getJoint() )
		{
			addJointState( joint_state );
		}
	}

	// initialize joint constraints
	for (JointMotionList::constraint_list_t::iterator iter = mJointMotionList->mConstraints.begin();
		 iter != mJointMotionList->mConstraints.end(); ++iter)
	{
		JointConstraintSharedData* shared_constraintp = *iter;
		JointConstraint* constraintp = new JointConstraint(shared_constraintp);
		initializeConstraint(constraintp);
		mConstraints.push_front(constraintp);
	}

	if (mJointMotionList->mConstraints.size())
	{
		mPelvisp = mCharacter->getJoint("mPelvis");
		if (!mPelvisp)
		{
			return FALSE;
		}
	}

	// setup loop keys
	setLoopIn(mJointMotionList->mLoopInPoint);
	setLoopOut(mJointMotionList->mLoopOutPoint);

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLKeyframeMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotion::onActivate()
{
	// If the keyframe anim has an associated emote, trigger it. 
	if( mJointMotionList->mEmoteName.length() > 0 )
	{
		LLUUID emote_anim_id = gAnimLibrary.stringToAnimState(mJointMotionList->mEmoteName);
		// don't start emote if already active to avoid recursion
		if (!mCharacter->isMotionActive(emote_anim_id))
		{
			mCharacter->startMotion( emote_anim_id );
		}
	}

	mLastLoopedTime = 0.f;

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLKeyframeMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotion::onUpdate(F32 time, U8* joint_mask)
{
	llassert(time >= 0.f);

	if (mJointMotionList->mLoop)
	{
		if (mJointMotionList->mDuration == 0.0f)
		{
			time = 0.f;
			mLastLoopedTime = 0.0f;
		}
		else if (mStopped)
		{
			mLastLoopedTime = llmin(mJointMotionList->mDuration, mLastLoopedTime + time - mLastUpdateTime);
		}
		else if (time > mJointMotionList->mLoopOutPoint)
		{
			if ((mJointMotionList->mLoopOutPoint - mJointMotionList->mLoopInPoint) == 0.f)
			{
				mLastLoopedTime = mJointMotionList->mLoopOutPoint;
			}
			else
			{
				mLastLoopedTime = mJointMotionList->mLoopInPoint + 
					fmod(time - mJointMotionList->mLoopOutPoint, 
					mJointMotionList->mLoopOutPoint - mJointMotionList->mLoopInPoint);
			}
		}
		else
		{
			mLastLoopedTime = time;
		}
	}
	else
	{
		mLastLoopedTime = time;
	}

	applyKeyframes(mLastLoopedTime);

	applyConstraints(mLastLoopedTime, joint_mask);

	mLastUpdateTime = time;

	return mLastLoopedTime <= mJointMotionList->mDuration;
}

//-----------------------------------------------------------------------------
// applyKeyframes()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::applyKeyframes(F32 time)
{
	llassert_always (mJointMotionList->getNumJointMotions() <= mJointStates.size());
	for (U32 i=0; i<mJointMotionList->getNumJointMotions(); i++)
	{
		mJointMotionList->getJointMotion(i)->update(mJointStates[i],
													  time, 
													  mJointMotionList->mDuration );
	}

	LLJoint::JointPriority* pose_priority = (LLJoint::JointPriority* )mCharacter->getAnimationData("Hand Pose Priority");
	if (pose_priority)
	{
		if (mJointMotionList->mMaxPriority >= *pose_priority)
		{
			mCharacter->setAnimationData("Hand Pose", &mJointMotionList->mHandPose);
			mCharacter->setAnimationData("Hand Pose Priority", &mJointMotionList->mMaxPriority);
		}
	}
	else
	{
		mCharacter->setAnimationData("Hand Pose", &mJointMotionList->mHandPose);
		mCharacter->setAnimationData("Hand Pose Priority", &mJointMotionList->mMaxPriority);
	}
}

//-----------------------------------------------------------------------------
// applyConstraints()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::applyConstraints(F32 time, U8* joint_mask)
{
	//TODO: investigate replacing spring simulation with critically damped motion

	// re-init constraints if skeleton has changed
	if (mCharacter->getSkeletonSerialNum() != mLastSkeletonSerialNum)
	{
		mLastSkeletonSerialNum = mCharacter->getSkeletonSerialNum();
		for (constraint_list_t::iterator iter = mConstraints.begin();
			 iter != mConstraints.end(); ++iter)
		{
			JointConstraint* constraintp = *iter;
			initializeConstraint(constraintp);
		}
	}

	// apply constraints
	for (constraint_list_t::iterator iter = mConstraints.begin();
		 iter != mConstraints.end(); ++iter)
	{
		JointConstraint* constraintp = *iter;
		applyConstraint(constraintp, time, joint_mask);
	}
}

//-----------------------------------------------------------------------------
// LLKeyframeMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::onDeactivate()
{
	for (constraint_list_t::iterator iter = mConstraints.begin();
		 iter != mConstraints.end(); ++iter)
	{
		JointConstraint* constraintp = *iter;
		deactivateConstraint(constraintp);
	}
}

//-----------------------------------------------------------------------------
// setStopTime()
//-----------------------------------------------------------------------------
// time is in seconds since character creation
void LLKeyframeMotion::setStopTime(F32 time)
{
	LLMotion::setStopTime(time);

	if (mJointMotionList->mLoop && mJointMotionList->mLoopOutPoint != mJointMotionList->mDuration)
	{
		F32 start_loop_time = mActivationTimestamp + mJointMotionList->mLoopInPoint;
		F32 loop_fraction_time;
		if (mJointMotionList->mLoopOutPoint == mJointMotionList->mLoopInPoint)
		{
			loop_fraction_time = 0.f;
		}
		else
		{
			loop_fraction_time = fmod(time - start_loop_time, 
				mJointMotionList->mLoopOutPoint - mJointMotionList->mLoopInPoint);
		}
		mStopTimestamp = llmax(time, 
			(time - loop_fraction_time) + (mJointMotionList->mDuration - mJointMotionList->mLoopInPoint) - getEaseOutDuration());
	}
}

//-----------------------------------------------------------------------------
// initializeConstraint()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::initializeConstraint(JointConstraint* constraint)
{
	JointConstraintSharedData *shared_data = constraint->mSharedData;

	S32 joint_num;
	LLVector3 source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);
	LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[0]);

	F32 source_pos_offset = dist_vec(source_pos, cur_joint->getWorldPosition());

	constraint->mTotalLength = constraint->mJointLengths[0] = dist_vec(cur_joint->getParent()->getWorldPosition(), source_pos);
	
	// grab joint lengths
	for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
	{
		cur_joint = getJointState(shared_data->mJointStateIndices[joint_num])->getJoint();
		if (!cur_joint)
		{
			return;
		}
		constraint->mJointLengths[joint_num] = dist_vec(cur_joint->getWorldPosition(), cur_joint->getParent()->getWorldPosition());
		constraint->mTotalLength += constraint->mJointLengths[joint_num];
	}

	// store fraction of total chain length so we know how to shear the entire chain towards the goal position
	for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
	{
		constraint->mJointLengthFractions[joint_num] = constraint->mJointLengths[joint_num] / constraint->mTotalLength;
	}

	// add last step in chain, from final joint to constraint position
	constraint->mTotalLength += source_pos_offset;

	constraint->mSourceVolume = mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume);
	constraint->mTargetVolume = mCharacter->findCollisionVolume(shared_data->mTargetConstraintVolume);
}

//-----------------------------------------------------------------------------
// activateConstraint()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::activateConstraint(JointConstraint* constraint)
{
	JointConstraintSharedData *shared_data = constraint->mSharedData;
	constraint->mActive = TRUE;
	S32 joint_num;

	// grab ground position if we need to
	if (shared_data->mConstraintTargetType == CONSTRAINT_TARGET_TYPE_GROUND)
	{
		LLVector3 source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);
		LLVector3 ground_pos_agent;
		mCharacter->getGround(source_pos, ground_pos_agent, constraint->mGroundNorm);
		constraint->mGroundPos = mCharacter->getPosGlobalFromAgent(ground_pos_agent + shared_data->mTargetConstraintOffset);
	}

	for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
	{
		LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[joint_num]);
		constraint->mPositions[joint_num] = (cur_joint->getWorldPosition() - mPelvisp->getWorldPosition()) * ~mPelvisp->getWorldRotation();
	}

	constraint->mWeight = 1.f;
}

//-----------------------------------------------------------------------------
// deactivateConstraint()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::deactivateConstraint(JointConstraint *constraintp)
{
	if (constraintp->mSourceVolume)
	{
		constraintp->mSourceVolume->mUpdateXform = FALSE;
	}

	if (!constraintp->mSharedData->mConstraintTargetType == CONSTRAINT_TARGET_TYPE_GROUND)
	{
		if (constraintp->mTargetVolume)
		{
			constraintp->mTargetVolume->mUpdateXform = FALSE;
		}
	}
	constraintp->mActive = FALSE;
}

//-----------------------------------------------------------------------------
// applyConstraint()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::applyConstraint(JointConstraint* constraint, F32 time, U8* joint_mask)
{
	JointConstraintSharedData *shared_data = constraint->mSharedData;
	if (!shared_data) return;

	LLVector3		positions[MAX_CHAIN_LENGTH];
	const F32*		joint_lengths = constraint->mJointLengths;
	LLVector3		velocities[MAX_CHAIN_LENGTH - 1];
	LLQuaternion	old_rots[MAX_CHAIN_LENGTH];
	S32				joint_num;

	if (time < shared_data->mEaseInStartTime)
	{
		return;
	}

	if (time > shared_data->mEaseOutStopTime)
	{
		if (constraint->mActive)
		{
			deactivateConstraint(constraint);
		}
		return;
	}

	if (!constraint->mActive || time < shared_data->mEaseInStopTime)
	{
		activateConstraint(constraint);
	}

	LLJoint* root_joint = getJoint(shared_data->mJointStateIndices[shared_data->mChainLength]);
	LLVector3 root_pos = root_joint->getWorldPosition();
//	LLQuaternion root_rot = 
	root_joint->getParent()->getWorldRotation();
//	LLQuaternion inv_root_rot = ~root_rot;

//	LLVector3 current_source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);

	//apply underlying keyframe animation to get nominal "kinematic" joint positions
	for (joint_num = 0; joint_num <= shared_data->mChainLength; joint_num++)
	{
		LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[joint_num]);
		if (joint_mask[cur_joint->getJointNum()] >= (0xff >> (7 - getPriority())))
		{
			// skip constraint
			return;
		}
		old_rots[joint_num] = cur_joint->getRotation();
		cur_joint->setRotation(getJointState(shared_data->mJointStateIndices[joint_num])->getRotation());
	}


	LLVector3 keyframe_source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);
	LLVector3 target_pos;

	switch(shared_data->mConstraintTargetType)
	{
	case CONSTRAINT_TARGET_TYPE_GROUND:
		target_pos = mCharacter->getPosAgentFromGlobal(constraint->mGroundPos);
//		llinfos << "Target Pos " << constraint->mGroundPos << " on " << mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume)->getName() << llendl;
		break;
	case CONSTRAINT_TARGET_TYPE_BODY:
		target_pos = mCharacter->getVolumePos(shared_data->mTargetConstraintVolume, shared_data->mTargetConstraintOffset);
		break;
	default:
		break;
	}
	
	LLVector3 norm;
	LLJoint *source_jointp = NULL;
	LLJoint *target_jointp = NULL;

	if (shared_data->mConstraintType == CONSTRAINT_TYPE_PLANE)
	{
		switch(shared_data->mConstraintTargetType)
		{
		case CONSTRAINT_TARGET_TYPE_GROUND:
			norm = constraint->mGroundNorm;
			break;
		case CONSTRAINT_TARGET_TYPE_BODY:
			target_jointp = mCharacter->findCollisionVolume(shared_data->mTargetConstraintVolume);
			if (target_jointp)
			{
				// *FIX: do proper normal calculation for stretched
				// spheres (inverse transpose)
				norm = target_pos - target_jointp->getWorldPosition();
			}

			if (norm.isExactlyZero())
			{
				source_jointp = mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume);
				norm = -1.f * shared_data->mSourceConstraintOffset;
				if (source_jointp)	
				{
					norm = norm * source_jointp->getWorldRotation();
				}
			}
			norm.normVec();
			break;
		default:
			norm.clearVec();
			break;
		}
		
		target_pos = keyframe_source_pos + (norm * ((target_pos - keyframe_source_pos) * norm));
	}

	if (constraint->mSharedData->mChainLength != 0 &&
		dist_vec_squared(root_pos, target_pos) * 0.95f > constraint->mTotalLength * constraint->mTotalLength)
	{
		constraint->mWeight = lerp(constraint->mWeight, 0.f, LLCriticalDamp::getInterpolant(0.1f));
	}
	else
	{
		constraint->mWeight = lerp(constraint->mWeight, 1.f, LLCriticalDamp::getInterpolant(0.3f));
	}

	F32 weight = constraint->mWeight * ((shared_data->mEaseOutStopTime == 0.f) ? 1.f : 
		llmin(clamp_rescale(time, shared_data->mEaseInStartTime, shared_data->mEaseInStopTime, 0.f, 1.f), 
		clamp_rescale(time, shared_data->mEaseOutStartTime, shared_data->mEaseOutStopTime, 1.f, 0.f)));

	LLVector3 source_to_target = target_pos - keyframe_source_pos;
	
	S32 max_iteration_count = llround(clamp_rescale(
										  mCharacter->getPixelArea(),
										  MAX_PIXEL_AREA_CONSTRAINTS,
										  MIN_PIXEL_AREA_CONSTRAINTS,
										  (F32)MAX_ITERATIONS,
										  (F32)MIN_ITERATIONS));

	if (shared_data->mChainLength)
	{
		LLQuaternion end_rot = getJoint(shared_data->mJointStateIndices[0])->getWorldRotation();

		// slam start and end of chain to the proper positions (rest of chain stays put)
		positions[0] = lerp(keyframe_source_pos, target_pos, weight);
		positions[shared_data->mChainLength] = root_pos;

		// grab keyframe-specified positions of joints	
		for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
		{
			LLVector3 kinematic_position = getJoint(shared_data->mJointStateIndices[joint_num])->getWorldPosition() + 
				(source_to_target * constraint->mJointLengthFractions[joint_num]);

			// convert intermediate joint positions to world coordinates
			positions[joint_num] = ( constraint->mPositions[joint_num] * mPelvisp->getWorldRotation()) + mPelvisp->getWorldPosition();
			F32 time_constant = 1.f / clamp_rescale(constraint->mFixupDistanceRMS, 0.f, 0.5f, 0.2f, 8.f);
//			llinfos << "Interpolant " << LLCriticalDamp::getInterpolant(time_constant, FALSE) << " and fixup distance " << constraint->mFixupDistanceRMS << " on " << mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume)->getName() << llendl;
			positions[joint_num] = lerp(positions[joint_num], kinematic_position, 
				LLCriticalDamp::getInterpolant(time_constant, FALSE));
		}

		S32 iteration_count;
		for (iteration_count = 0; iteration_count < max_iteration_count; iteration_count++)
		{
			S32 num_joints_finished = 0;
			for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
			{
				// constraint to child
				LLVector3 acceleration = (positions[joint_num - 1] - positions[joint_num]) * 
					(dist_vec(positions[joint_num], positions[joint_num - 1]) - joint_lengths[joint_num - 1]) * JOINT_LENGTH_K;
				// constraint to parent
				acceleration  += (positions[joint_num + 1] - positions[joint_num]) * 
					(dist_vec(positions[joint_num + 1], positions[joint_num]) - joint_lengths[joint_num]) * JOINT_LENGTH_K;

				if (acceleration.magVecSquared() < MIN_ACCELERATION_SQUARED)
				{
					num_joints_finished++;
				}

				velocities[joint_num - 1] = velocities[joint_num - 1] * 0.7f;
				positions[joint_num] += velocities[joint_num - 1] + (acceleration * 0.5f);
				velocities[joint_num - 1] += acceleration;
			}

			if ((iteration_count >= MIN_ITERATION_COUNT) && 
				(num_joints_finished == shared_data->mChainLength - 1))
			{
//				llinfos << iteration_count << " iterations on " << 
//					mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume)->getName() << llendl;
				break;
			}
		}

		for (joint_num = shared_data->mChainLength; joint_num > 0; joint_num--)
		{
			LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[joint_num]);
			LLJoint* child_joint = getJoint(shared_data->mJointStateIndices[joint_num - 1]);
			LLQuaternion parent_rot = cur_joint->getParent()->getWorldRotation();

			LLQuaternion cur_rot = cur_joint->getWorldRotation();
			LLQuaternion fixup_rot;
			
			LLVector3 target_at = positions[joint_num - 1] - positions[joint_num];
			LLVector3 current_at;

			// at bottom of chain, use point on collision volume, not joint position
			if (joint_num == 1)
			{
				current_at = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset) -
					cur_joint->getWorldPosition();
			}
			else
			{
				current_at = child_joint->getPosition() * cur_rot;
			}
			fixup_rot.shortestArc(current_at, target_at);

			LLQuaternion target_rot = cur_rot * fixup_rot;
			target_rot = target_rot * ~parent_rot;

			if (weight != 1.f)
			{
				LLQuaternion cur_rot = getJointState(shared_data->mJointStateIndices[joint_num])->getRotation();
				target_rot = nlerp(weight, cur_rot, target_rot);
			}

			getJointState(shared_data->mJointStateIndices[joint_num])->setRotation(target_rot);
			cur_joint->setRotation(target_rot);
		}

		LLJoint* end_joint = getJoint(shared_data->mJointStateIndices[0]);
		LLQuaternion end_local_rot = end_rot * ~end_joint->getParent()->getWorldRotation();

		if (weight == 1.f)
		{
			getJointState(shared_data->mJointStateIndices[0])->setRotation(end_local_rot);
		}
		else
		{
			LLQuaternion cur_rot = getJointState(shared_data->mJointStateIndices[0])->getRotation();
			getJointState(shared_data->mJointStateIndices[0])->setRotation(nlerp(weight, cur_rot, end_local_rot));
		}

		// save simulated positions in pelvis-space and calculate total fixup distance
		constraint->mFixupDistanceRMS = 0.f;
		F32 delta_time = llmax(0.02f, llabs(time - mLastUpdateTime));
		for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
		{
			LLVector3 new_pos = (positions[joint_num] - mPelvisp->getWorldPosition()) * ~mPelvisp->getWorldRotation();
			constraint->mFixupDistanceRMS += dist_vec_squared(new_pos, constraint->mPositions[joint_num]) / delta_time;
			constraint->mPositions[joint_num] = new_pos;
		}
		constraint->mFixupDistanceRMS *= 1.f / (constraint->mTotalLength * (F32)(shared_data->mChainLength - 1));
		constraint->mFixupDistanceRMS = (F32) sqrt(constraint->mFixupDistanceRMS);

		//reset old joint rots
		for (joint_num = 0; joint_num <= shared_data->mChainLength; joint_num++)
		{
			getJoint(shared_data->mJointStateIndices[joint_num])->setRotation(old_rots[joint_num]);
		}
	}
	// simple positional constraint (pelvis only)
	else if (getJointState(shared_data->mJointStateIndices[0])->getUsage() & LLJointState::POS)
	{
		LLVector3 delta = source_to_target * weight;
		LLPointer<LLJointState> current_joint_state = getJointState(shared_data->mJointStateIndices[0]);
		LLQuaternion parent_rot = current_joint_state->getJoint()->getParent()->getWorldRotation();
		delta = delta * ~parent_rot;
		current_joint_state->setPosition(current_joint_state->getJoint()->getPosition() + delta);
	}
}

//-----------------------------------------------------------------------------
// deserialize()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotion::deserialize(LLDataPacker& dp)
{
	BOOL old_version = FALSE;
	mJointMotionList = new LLKeyframeMotion::JointMotionList;

	//-------------------------------------------------------------------------
	// get base priority
	//-------------------------------------------------------------------------
	S32 temp_priority;
	U16 version;
	U16 sub_version;

	if (!dp.unpackU16(version, "version"))
	{
		llwarns << "can't read version number" << llendl;
		return FALSE;
	}

	if (!dp.unpackU16(sub_version, "sub_version"))
	{
		llwarns << "can't read sub version number" << llendl;
		return FALSE;
	}

	if (version == 0 && sub_version == 1)
	{
		old_version = TRUE;
	}
	else if (version != KEYFRAME_MOTION_VERSION || sub_version != KEYFRAME_MOTION_SUBVERSION)
	{
#if LL_RELEASE
		llwarns << "Bad animation version " << version << "." << sub_version << llendl;
		return FALSE;
#else
		llerrs << "Bad animation version " << version << "." << sub_version << llendl;
#endif
	}

	if (!dp.unpackS32(temp_priority, "base_priority"))
	{
		llwarns << "can't read animation base_priority" << llendl;
		return FALSE;
	}
	mJointMotionList->mBasePriority = (LLJoint::JointPriority) temp_priority;

	if (mJointMotionList->mBasePriority >= LLJoint::ADDITIVE_PRIORITY)
	{
		mJointMotionList->mBasePriority = (LLJoint::JointPriority)((int)LLJoint::ADDITIVE_PRIORITY-1);
		mJointMotionList->mMaxPriority = mJointMotionList->mBasePriority;
	}
	else if (mJointMotionList->mBasePriority < LLJoint::USE_MOTION_PRIORITY)
	{
		llwarns << "bad animation base_priority " << mJointMotionList->mBasePriority << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// get duration
	//-------------------------------------------------------------------------
	if (!dp.unpackF32(mJointMotionList->mDuration, "duration"))
	{
		llwarns << "can't read duration" << llendl;
		return FALSE;
	}
	
	if (mJointMotionList->mDuration > MAX_ANIM_DURATION ||
	    !llfinite(mJointMotionList->mDuration))
	{
		llwarns << "invalid animation duration" << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// get emote (optional)
	//-------------------------------------------------------------------------
	if (!dp.unpackString(mJointMotionList->mEmoteName, "emote_name"))
	{
		llwarns << "can't read optional_emote_animation" << llendl;
		return FALSE;
	}

	if(mJointMotionList->mEmoteName==mID.asString())
	{
		llwarns << "Malformed animation mEmoteName==mID" << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// get loop
	//-------------------------------------------------------------------------
	if (!dp.unpackF32(mJointMotionList->mLoopInPoint, "loop_in_point") ||
	    !llfinite(mJointMotionList->mLoopInPoint))
	{
		llwarns << "can't read loop point" << llendl;
		return FALSE;
	}

	if (!dp.unpackF32(mJointMotionList->mLoopOutPoint, "loop_out_point") ||
	    !llfinite(mJointMotionList->mLoopOutPoint))
	{
		llwarns << "can't read loop point" << llendl;
		return FALSE;
	}

	if (!dp.unpackS32(mJointMotionList->mLoop, "loop"))
	{
		llwarns << "can't read loop" << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// get easeIn and easeOut
	//-------------------------------------------------------------------------
	if (!dp.unpackF32(mJointMotionList->mEaseInDuration, "ease_in_duration") ||
	    !llfinite(mJointMotionList->mEaseInDuration))
	{
		llwarns << "can't read easeIn" << llendl;
		return FALSE;
	}

	if (!dp.unpackF32(mJointMotionList->mEaseOutDuration, "ease_out_duration") ||
	    !llfinite(mJointMotionList->mEaseOutDuration))
	{
		llwarns << "can't read easeOut" << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// get hand pose
	//-------------------------------------------------------------------------
	U32 word;
	if (!dp.unpackU32(word, "hand_pose"))
	{
		llwarns << "can't read hand pose" << llendl;
		return FALSE;
	}
	
	if(word > LLHandMotion::NUM_HAND_POSES)
	{
		llwarns << "invalid LLHandMotion::eHandPose index: " << word << llendl;
		return FALSE;
	}
	
	mJointMotionList->mHandPose = (LLHandMotion::eHandPose)word;

	//-------------------------------------------------------------------------
	// get number of joint motions
	//-------------------------------------------------------------------------
	U32 num_motions = 0;
	if (!dp.unpackU32(num_motions, "num_joints"))
	{
		llwarns << "can't read number of joints" << llendl;
		return FALSE;
	}

	if (num_motions == 0)
	{
		llwarns << "no joints in animation" << llendl;
		return FALSE;
	}
	else if (num_motions > LL_CHARACTER_MAX_JOINTS)
	{
		llwarns << "too many joints in animation" << llendl;
		return FALSE;
	}

	mJointMotionList->mJointMotionArray.clear();
	mJointMotionList->mJointMotionArray.reserve(num_motions);
	mJointStates.clear();
	mJointStates.reserve(num_motions);

	//-------------------------------------------------------------------------
	// initialize joint motions
	//-------------------------------------------------------------------------

	for(U32 i=0; i<num_motions; ++i)
	{
		JointMotion* joint_motion = new JointMotion;		
		mJointMotionList->mJointMotionArray.push_back(joint_motion);
		
		std::string joint_name;
		if (!dp.unpackString(joint_name, "joint_name"))
		{
			llwarns << "can't read joint name" << llendl;
			return FALSE;
		}

		if (joint_name == "mScreen" || joint_name == "mRoot")
		{
			llwarns << "attempted to animate special " << joint_name << " joint" << llendl;
			return FALSE;
		}
				
		//---------------------------------------------------------------------
		// find the corresponding joint
		//---------------------------------------------------------------------
		LLJoint *joint = mCharacter->getJoint( joint_name );
		if (joint)
		{
//			llinfos << "  joint: " << joint_name << llendl;
		}
		else
		{
			llwarns << "joint not found: " << joint_name << llendl;
			//return FALSE;
		}

		joint_motion->mJointName = joint_name;
		
		LLPointer<LLJointState> joint_state = new LLJointState;
		mJointStates.push_back(joint_state);
		joint_state->setJoint( joint ); // note: can accept NULL
		joint_state->setUsage( 0 );

		//---------------------------------------------------------------------
		// get joint priority
		//---------------------------------------------------------------------
		S32 joint_priority;
		if (!dp.unpackS32(joint_priority, "joint_priority"))
		{
			llwarns << "can't read joint priority." << llendl;
			return FALSE;
		}

		if (joint_priority < LLJoint::USE_MOTION_PRIORITY)
		{
			llwarns << "joint priority unknown - too low." << llendl;
			return FALSE;
		}
		
		joint_motion->mPriority = (LLJoint::JointPriority)joint_priority;
		if (joint_priority != LLJoint::USE_MOTION_PRIORITY &&
		    joint_priority > mJointMotionList->mMaxPriority)
		{
			mJointMotionList->mMaxPriority = (LLJoint::JointPriority)joint_priority;
		}

		joint_state->setPriority((LLJoint::JointPriority)joint_priority);

		//---------------------------------------------------------------------
		// scan rotation curve header
		//---------------------------------------------------------------------
		if (!dp.unpackS32(joint_motion->mRotationCurve.mNumKeys, "num_rot_keys") || joint_motion->mRotationCurve.mNumKeys < 0)
		{
			llwarns << "can't read number of rotation keys" << llendl;
			return FALSE;
		}

		joint_motion->mRotationCurve.mInterpolationType = IT_LINEAR;
		if (joint_motion->mRotationCurve.mNumKeys != 0)
		{
			joint_state->setUsage(joint_state->getUsage() | LLJointState::ROT );
		}

		//---------------------------------------------------------------------
		// scan rotation curve keys
		//---------------------------------------------------------------------
		RotationCurve *rCurve = &joint_motion->mRotationCurve;

		for (S32 k = 0; k < joint_motion->mRotationCurve.mNumKeys; k++)
		{
			F32 time;
			U16 time_short;

			if (old_version)
			{
				if (!dp.unpackF32(time, "time") ||
				    !llfinite(time))
				{
					llwarns << "can't read rotation key (" << k << ")" << llendl;
					return FALSE;
				}

			}
			else
			{
				if (!dp.unpackU16(time_short, "time"))
				{
					llwarns << "can't read rotation key (" << k << ")" << llendl;
					return FALSE;
				}

				time = U16_to_F32(time_short, 0.f, mJointMotionList->mDuration);
				
				if (time < 0 || time > mJointMotionList->mDuration)
				{
					llwarns << "invalid frame time" << llendl;
					return FALSE;
				}
			}
			
			RotationKey rot_key;
			rot_key.mTime = time;
			LLVector3 rot_angles;
			U16 x, y, z;

			BOOL success = TRUE;

			if (old_version)
			{
				success = dp.unpackVector3(rot_angles, "rot_angles") && rot_angles.isFinite();

				LLQuaternion::Order ro = StringToOrder("ZYX");
				rot_key.mRotation = mayaQ(rot_angles.mV[VX], rot_angles.mV[VY], rot_angles.mV[VZ], ro);
			}
			else
			{
				success &= dp.unpackU16(x, "rot_angle_x");
				success &= dp.unpackU16(y, "rot_angle_y");
				success &= dp.unpackU16(z, "rot_angle_z");

				LLVector3 rot_vec;
				rot_vec.mV[VX] = U16_to_F32(x, -1.f, 1.f);
				rot_vec.mV[VY] = U16_to_F32(y, -1.f, 1.f);
				rot_vec.mV[VZ] = U16_to_F32(z, -1.f, 1.f);
				rot_key.mRotation.unpackFromVector3(rot_vec);
			}

			if( !(rot_key.mRotation.isFinite()) )
			{
				llwarns << "non-finite angle in rotation key" << llendl;
				success = FALSE;
			}
			
			if (!success)
			{
				llwarns << "can't read rotation key (" << k << ")" << llendl;
				return FALSE;
			}

			rCurve->mKeys[time] = rot_key;
		}

		//---------------------------------------------------------------------
		// scan position curve header
		//---------------------------------------------------------------------
		if (!dp.unpackS32(joint_motion->mPositionCurve.mNumKeys, "num_pos_keys") || joint_motion->mPositionCurve.mNumKeys < 0)
		{
			llwarns << "can't read number of position keys" << llendl;
			return FALSE;
		}

		joint_motion->mPositionCurve.mInterpolationType = IT_LINEAR;
		if (joint_motion->mPositionCurve.mNumKeys != 0)
		{
			joint_state->setUsage(joint_state->getUsage() | LLJointState::POS );
		}

		//---------------------------------------------------------------------
		// scan position curve keys
		//---------------------------------------------------------------------
		PositionCurve *pCurve = &joint_motion->mPositionCurve;
		BOOL is_pelvis = joint_motion->mJointName == "mPelvis";
		for (S32 k = 0; k < joint_motion->mPositionCurve.mNumKeys; k++)
		{
			U16 time_short;
			PositionKey pos_key;

			if (old_version)
			{
				if (!dp.unpackF32(pos_key.mTime, "time") ||
				    !llfinite(pos_key.mTime))
				{
					llwarns << "can't read position key (" << k << ")" << llendl;
					return FALSE;
				}
			}
			else
			{
				if (!dp.unpackU16(time_short, "time"))
				{
					llwarns << "can't read position key (" << k << ")" << llendl;
					return FALSE;
				}

				pos_key.mTime = U16_to_F32(time_short, 0.f, mJointMotionList->mDuration);
			}

			BOOL success = TRUE;

			if (old_version)
			{
				success = dp.unpackVector3(pos_key.mPosition, "pos");
			}
			else
			{
				U16 x, y, z;

				success &= dp.unpackU16(x, "pos_x");
				success &= dp.unpackU16(y, "pos_y");
				success &= dp.unpackU16(z, "pos_z");

				pos_key.mPosition.mV[VX] = U16_to_F32(x, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				pos_key.mPosition.mV[VY] = U16_to_F32(y, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				pos_key.mPosition.mV[VZ] = U16_to_F32(z, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			}
			
			if( !(pos_key.mPosition.isFinite()) )
			{
				llwarns << "non-finite position in key" << llendl;
				success = FALSE;
			}
			
			if (!success)
			{
				llwarns << "can't read position key (" << k << ")" << llendl;
				return FALSE;
			}
			
			pCurve->mKeys[pos_key.mTime] = pos_key;

			if (is_pelvis)
			{
				mJointMotionList->mPelvisBBox.addPoint(pos_key.mPosition);
			}
		}

		joint_motion->mUsage = joint_state->getUsage();
	}

	//-------------------------------------------------------------------------
	// get number of constraints
	//-------------------------------------------------------------------------
	S32 num_constraints = 0;
	if (!dp.unpackS32(num_constraints, "num_constraints"))
	{
		llwarns << "can't read number of constraints" << llendl;
		return FALSE;
	}

	if (num_constraints > MAX_CONSTRAINTS || num_constraints < 0)
	{
		llwarns << "Bad number of constraints... ignoring: " << num_constraints << llendl;
	}
	else
	{
		//-------------------------------------------------------------------------
		// get constraints
		//-------------------------------------------------------------------------
		std::string str;
		for(S32 i = 0; i < num_constraints; ++i)
		{
			// read in constraint data
			JointConstraintSharedData* constraintp = new JointConstraintSharedData;
			U8 byte = 0;

			if (!dp.unpackU8(byte, "chain_length"))
			{
				llwarns << "can't read constraint chain length" << llendl;
				delete constraintp;
				return FALSE;
			}
			constraintp->mChainLength = (S32) byte;

			if((U32)constraintp->mChainLength > mJointMotionList->getNumJointMotions())
			{
				llwarns << "invalid constraint chain length" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackU8(byte, "constraint_type"))
			{
				llwarns << "can't read constraint type" << llendl;
				delete constraintp;
				return FALSE;
			}
			
			if( byte >= NUM_CONSTRAINT_TYPES )
			{
				llwarns << "invalid constraint type" << llendl;
				delete constraintp;
				return FALSE;
			}
			constraintp->mConstraintType = (EConstraintType)byte;

			const S32 BIN_DATA_LENGTH = 16;
			U8 bin_data[BIN_DATA_LENGTH+1];
			if (!dp.unpackBinaryDataFixed(bin_data, BIN_DATA_LENGTH, "source_volume"))
			{
				llwarns << "can't read source volume name" << llendl;
				delete constraintp;
				return FALSE;
			}

			bin_data[BIN_DATA_LENGTH] = 0; // Ensure null termination
			str = (char*)bin_data;
			constraintp->mSourceConstraintVolume = mCharacter->getCollisionVolumeID(str);

			if (!dp.unpackVector3(constraintp->mSourceConstraintOffset, "source_offset"))
			{
				llwarns << "can't read constraint source offset" << llendl;
				delete constraintp;
				return FALSE;
			}
			
			if( !(constraintp->mSourceConstraintOffset.isFinite()) )
			{
				llwarns << "non-finite constraint source offset" << llendl;
				delete constraintp;
				return FALSE;
			}
			
			if (!dp.unpackBinaryDataFixed(bin_data, BIN_DATA_LENGTH, "target_volume"))
			{
				llwarns << "can't read target volume name" << llendl;
				delete constraintp;
				return FALSE;
			}

			bin_data[BIN_DATA_LENGTH] = 0; // Ensure null termination
			str = (char*)bin_data;
			if (str == "GROUND")
			{
				// constrain to ground
				constraintp->mConstraintTargetType = CONSTRAINT_TARGET_TYPE_GROUND;
			}
			else
			{
				constraintp->mConstraintTargetType = CONSTRAINT_TARGET_TYPE_BODY;
				constraintp->mTargetConstraintVolume = mCharacter->getCollisionVolumeID(str);
			}

			if (!dp.unpackVector3(constraintp->mTargetConstraintOffset, "target_offset"))
			{
				llwarns << "can't read constraint target offset" << llendl;
				delete constraintp;
				return FALSE;
			}

			if( !(constraintp->mTargetConstraintOffset.isFinite()) )
			{
				llwarns << "non-finite constraint target offset" << llendl;
				delete constraintp;
				return FALSE;
			}
			
			if (!dp.unpackVector3(constraintp->mTargetConstraintDir, "target_dir"))
			{
				llwarns << "can't read constraint target direction" << llendl;
				delete constraintp;
				return FALSE;
			}

			if( !(constraintp->mTargetConstraintDir.isFinite()) )
			{
				llwarns << "non-finite constraint target direction" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!constraintp->mTargetConstraintDir.isExactlyZero())
			{
				constraintp->mUseTargetOffset = TRUE;
	//			constraintp->mTargetConstraintDir *= constraintp->mSourceConstraintOffset.magVec();
			}

			if (!dp.unpackF32(constraintp->mEaseInStartTime, "ease_in_start") || !llfinite(constraintp->mEaseInStartTime))
			{
				llwarns << "can't read constraint ease in start time" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackF32(constraintp->mEaseInStopTime, "ease_in_stop") || !llfinite(constraintp->mEaseInStopTime))
			{
				llwarns << "can't read constraint ease in stop time" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackF32(constraintp->mEaseOutStartTime, "ease_out_start") || !llfinite(constraintp->mEaseOutStartTime))
			{
				llwarns << "can't read constraint ease out start time" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackF32(constraintp->mEaseOutStopTime, "ease_out_stop") || !llfinite(constraintp->mEaseOutStopTime))
			{
				llwarns << "can't read constraint ease out stop time" << llendl;
				delete constraintp;
				return FALSE;
			}

			mJointMotionList->mConstraints.push_front(constraintp);

			constraintp->mJointStateIndices = new S32[constraintp->mChainLength + 1]; // note: mChainLength is size-limited - comes from a byte
			
			LLJoint* joint = mCharacter->findCollisionVolume(constraintp->mSourceConstraintVolume);
			// get joint to which this collision volume is attached
			if (!joint)
			{
				return FALSE;
			}
			for (S32 i = 0; i < constraintp->mChainLength + 1; i++)
			{
				LLJoint* parent = joint->getParent();
				if (!parent)
				{
					llwarns << "Joint with no parent: " << joint->getName()
							<< " Emote: " << mJointMotionList->mEmoteName << llendl;
					return FALSE;
				}
				joint = parent;
				constraintp->mJointStateIndices[i] = -1;
				for (U32 j = 0; j < mJointMotionList->getNumJointMotions(); j++)
				{
					if(getJoint(j) == joint)
					{
						constraintp->mJointStateIndices[i] = (S32)j;
						break;
					}
				}
				if (constraintp->mJointStateIndices[i] < 0 )
				{
					llwarns << "No joint index for constraint " << i << llendl;
					delete constraintp;
					return FALSE;
				}
			}
		}
	}

	// *FIX: support cleanup of old keyframe data
	LLKeyframeDataCache::addKeyframeData(getID(),  mJointMotionList);
	mAssetStatus = ASSET_LOADED;

	setupPose();

	return TRUE;
}

//-----------------------------------------------------------------------------
// serialize()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotion::serialize(LLDataPacker& dp) const
{
	BOOL success = TRUE;

	success &= dp.packU16(KEYFRAME_MOTION_VERSION, "version");
	success &= dp.packU16(KEYFRAME_MOTION_SUBVERSION, "sub_version");
	success &= dp.packS32(mJointMotionList->mBasePriority, "base_priority");
	success &= dp.packF32(mJointMotionList->mDuration, "duration");
	success &= dp.packString(mJointMotionList->mEmoteName, "emote_name");
	success &= dp.packF32(mJointMotionList->mLoopInPoint, "loop_in_point");
	success &= dp.packF32(mJointMotionList->mLoopOutPoint, "loop_out_point");
	success &= dp.packS32(mJointMotionList->mLoop, "loop");
	success &= dp.packF32(mJointMotionList->mEaseInDuration, "ease_in_duration");
	success &= dp.packF32(mJointMotionList->mEaseOutDuration, "ease_out_duration");
	success &= dp.packU32(mJointMotionList->mHandPose, "hand_pose");
	success &= dp.packU32(mJointMotionList->getNumJointMotions(), "num_joints");

	for (U32 i = 0; i < mJointMotionList->getNumJointMotions(); i++)
	{
		JointMotion* joint_motionp = mJointMotionList->getJointMotion(i);
		success &= dp.packString(joint_motionp->mJointName, "joint_name");
		success &= dp.packS32(joint_motionp->mPriority, "joint_priority");
		success &= dp.packS32(joint_motionp->mRotationCurve.mNumKeys, "num_rot_keys");

		for (RotationCurve::key_map_t::iterator iter = joint_motionp->mRotationCurve.mKeys.begin();
			 iter != joint_motionp->mRotationCurve.mKeys.end(); ++iter)
		{
			RotationKey& rot_key = iter->second;
			U16 time_short = F32_to_U16(rot_key.mTime, 0.f, mJointMotionList->mDuration);
			success &= dp.packU16(time_short, "time");

			LLVector3 rot_angles = rot_key.mRotation.packToVector3();
			
			U16 x, y, z;
			rot_angles.quantize16(-1.f, 1.f, -1.f, 1.f);
			x = F32_to_U16(rot_angles.mV[VX], -1.f, 1.f);
			y = F32_to_U16(rot_angles.mV[VY], -1.f, 1.f);
			z = F32_to_U16(rot_angles.mV[VZ], -1.f, 1.f);
			success &= dp.packU16(x, "rot_angle_x");
			success &= dp.packU16(y, "rot_angle_y");
			success &= dp.packU16(z, "rot_angle_z");
		}

		success &= dp.packS32(joint_motionp->mPositionCurve.mNumKeys, "num_pos_keys");
		for (PositionCurve::key_map_t::iterator iter = joint_motionp->mPositionCurve.mKeys.begin();
			 iter != joint_motionp->mPositionCurve.mKeys.end(); ++iter)
		{
			PositionKey& pos_key = iter->second;
			U16 time_short = F32_to_U16(pos_key.mTime, 0.f, mJointMotionList->mDuration);
			success &= dp.packU16(time_short, "time");

			U16 x, y, z;
			pos_key.mPosition.quantize16(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			x = F32_to_U16(pos_key.mPosition.mV[VX], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			y = F32_to_U16(pos_key.mPosition.mV[VY], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			z = F32_to_U16(pos_key.mPosition.mV[VZ], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			success &= dp.packU16(x, "pos_x");
			success &= dp.packU16(y, "pos_y");
			success &= dp.packU16(z, "pos_z");
		}
	}	

	success &= dp.packS32(mJointMotionList->mConstraints.size(), "num_constraints");
	for (JointMotionList::constraint_list_t::const_iterator iter = mJointMotionList->mConstraints.begin();
		 iter != mJointMotionList->mConstraints.end(); ++iter)
	{
		JointConstraintSharedData* shared_constraintp = *iter;
		success &= dp.packU8(shared_constraintp->mChainLength, "chain_length");
		success &= dp.packU8(shared_constraintp->mConstraintType, "constraint_type");
		char volume_name[16];	/* Flawfinder: ignore */
		snprintf(volume_name, sizeof(volume_name), "%s",	/* Flawfinder: ignore */
				 mCharacter->findCollisionVolume(shared_constraintp->mSourceConstraintVolume)->getName().c_str()); 
		success &= dp.packBinaryDataFixed((U8*)volume_name, 16, "source_volume");
		success &= dp.packVector3(shared_constraintp->mSourceConstraintOffset, "source_offset");
		if (shared_constraintp->mConstraintTargetType == CONSTRAINT_TARGET_TYPE_GROUND)
		{
			snprintf(volume_name,sizeof(volume_name), "%s", "GROUND");	/* Flawfinder: ignore */
		}
		else
		{
			snprintf(volume_name, sizeof(volume_name),"%s", /* Flawfinder: ignore */
					 mCharacter->findCollisionVolume(shared_constraintp->mTargetConstraintVolume)->getName().c_str());	
		}
		success &= dp.packBinaryDataFixed((U8*)volume_name, 16, "target_volume");
		success &= dp.packVector3(shared_constraintp->mTargetConstraintOffset, "target_offset");
		success &= dp.packVector3(shared_constraintp->mTargetConstraintDir, "target_dir");
		success &= dp.packF32(shared_constraintp->mEaseInStartTime, "ease_in_start");
		success &= dp.packF32(shared_constraintp->mEaseInStopTime, "ease_in_stop");
		success &= dp.packF32(shared_constraintp->mEaseOutStartTime, "ease_out_start");
		success &= dp.packF32(shared_constraintp->mEaseOutStopTime, "ease_out_stop");
	}

	return success;
}

//-----------------------------------------------------------------------------
// getFileSize()
//-----------------------------------------------------------------------------
U32	LLKeyframeMotion::getFileSize()
{
	// serialize into a dummy buffer to calculate required size
	LLDataPackerBinaryBuffer dp;
	serialize(dp);

	return dp.getCurrentSize();
}

//-----------------------------------------------------------------------------
// getPelvisBBox()
//-----------------------------------------------------------------------------
const LLBBoxLocal &LLKeyframeMotion::getPelvisBBox()
{
	return mJointMotionList->mPelvisBBox;
}

//-----------------------------------------------------------------------------
// setPriority()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setPriority(S32 priority)
{
	if (mJointMotionList) 
	{
		S32 priority_delta = priority - mJointMotionList->mBasePriority;
		mJointMotionList->mBasePriority = (LLJoint::JointPriority)priority;
		mJointMotionList->mMaxPriority = mJointMotionList->mBasePriority;

		for (U32 i = 0; i < mJointMotionList->getNumJointMotions(); i++)
		{
			JointMotion* joint_motion = mJointMotionList->getJointMotion(i);			
			joint_motion->mPriority = (LLJoint::JointPriority)llclamp(
				(S32)joint_motion->mPriority + priority_delta,
				(S32)LLJoint::LOW_PRIORITY, 
				(S32)LLJoint::HIGHEST_PRIORITY);
			getJointState(i)->setPriority(joint_motion->mPriority);
		}
	}
}

//-----------------------------------------------------------------------------
// setEmote()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setEmote(const LLUUID& emote_id)
{
	const char* emote_name = gAnimLibrary.animStateToString(emote_id);
	if (emote_name)
	{
		mJointMotionList->mEmoteName = emote_name;
	}
	else
	{
		mJointMotionList->mEmoteName = "";
	}
}

//-----------------------------------------------------------------------------
// setEaseIn()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setEaseIn(F32 ease_in)
{
	if (mJointMotionList)
	{
		mJointMotionList->mEaseInDuration = llmax(ease_in, 0.f);
	}
}

//-----------------------------------------------------------------------------
// setEaseOut()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setEaseOut(F32 ease_in)
{
	if (mJointMotionList)
	{
		mJointMotionList->mEaseOutDuration = llmax(ease_in, 0.f);
	}
}


//-----------------------------------------------------------------------------
// flushKeyframeCache()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::flushKeyframeCache()
{
	// TODO: Make this safe to do
// 	LLKeyframeDataCache::clear();
}

//-----------------------------------------------------------------------------
// setLoop()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setLoop(BOOL loop)
{
	if (mJointMotionList) 
	{
		mJointMotionList->mLoop = loop; 
		mSendStopTimestamp = F32_MAX;
	}
}


//-----------------------------------------------------------------------------
// setLoopIn()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setLoopIn(F32 in_point)
{
	if (mJointMotionList)
	{
		mJointMotionList->mLoopInPoint = in_point; 
		
		// set up loop keys
		for (U32 i = 0; i < mJointMotionList->getNumJointMotions(); i++)
		{
			JointMotion* joint_motion = mJointMotionList->getJointMotion(i);
			
			PositionCurve* pos_curve = &joint_motion->mPositionCurve;
			RotationCurve* rot_curve = &joint_motion->mRotationCurve;
			ScaleCurve* scale_curve = &joint_motion->mScaleCurve;
			
			pos_curve->mLoopInKey.mTime = mJointMotionList->mLoopInPoint;
			rot_curve->mLoopInKey.mTime = mJointMotionList->mLoopInPoint;
			scale_curve->mLoopInKey.mTime = mJointMotionList->mLoopInPoint;

			pos_curve->mLoopInKey.mPosition = pos_curve->getValue(mJointMotionList->mLoopInPoint, mJointMotionList->mDuration);
			rot_curve->mLoopInKey.mRotation = rot_curve->getValue(mJointMotionList->mLoopInPoint, mJointMotionList->mDuration);
			scale_curve->mLoopInKey.mScale = scale_curve->getValue(mJointMotionList->mLoopInPoint, mJointMotionList->mDuration);
		}
	}
}

//-----------------------------------------------------------------------------
// setLoopOut()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setLoopOut(F32 out_point)
{
	if (mJointMotionList)
	{
		mJointMotionList->mLoopOutPoint = out_point; 
		
		// set up loop keys
		for (U32 i = 0; i < mJointMotionList->getNumJointMotions(); i++)
		{
			JointMotion* joint_motion = mJointMotionList->getJointMotion(i);
			
			PositionCurve* pos_curve = &joint_motion->mPositionCurve;
			RotationCurve* rot_curve = &joint_motion->mRotationCurve;
			ScaleCurve* scale_curve = &joint_motion->mScaleCurve;
			
			pos_curve->mLoopOutKey.mTime = mJointMotionList->mLoopOutPoint;
			rot_curve->mLoopOutKey.mTime = mJointMotionList->mLoopOutPoint;
			scale_curve->mLoopOutKey.mTime = mJointMotionList->mLoopOutPoint;

			pos_curve->mLoopOutKey.mPosition = pos_curve->getValue(mJointMotionList->mLoopOutPoint, mJointMotionList->mDuration);
			rot_curve->mLoopOutKey.mRotation = rot_curve->getValue(mJointMotionList->mLoopOutPoint, mJointMotionList->mDuration);
			scale_curve->mLoopOutKey.mScale = scale_curve->getValue(mJointMotionList->mLoopOutPoint, mJointMotionList->mDuration);
		}
	}
}

//-----------------------------------------------------------------------------
// onLoadComplete()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::onLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LLUUID* id = (LLUUID*)user_data;
		
	std::vector<LLCharacter* >::iterator char_iter = LLCharacter::sInstances.begin();

	while(char_iter != LLCharacter::sInstances.end() &&
			(*char_iter)->getID() != *id)
	{
		++char_iter;
	}
	
	delete id;

	if (char_iter == LLCharacter::sInstances.end())
	{
		return;
	}

	LLCharacter* character = *char_iter;

	// look for an existing instance of this motion
	LLKeyframeMotion* motionp = (LLKeyframeMotion*) character->findMotion(asset_uuid);
	if (motionp)
	{
		if (0 == status)
		{
			if (motionp->mAssetStatus == ASSET_LOADED)
			{
				// asset already loaded
				return;
			}
			LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
			S32 size = file.getSize();
			
			U8* buffer = new U8[size];
			file.read((U8*)buffer, size);	/*Flawfinder: ignore*/
			
			lldebugs << "Loading keyframe data for: " << motionp->getName() << ":" << motionp->getID() << " (" << size << " bytes)" << llendl;
			
			LLDataPackerBinaryBuffer dp(buffer, size);
			if (motionp->deserialize(dp))
			{
				motionp->mAssetStatus = ASSET_LOADED;
			}
			else
			{
				llwarns << "Failed to decode asset for animation " << motionp->getName() << ":" << motionp->getID() << llendl;
				motionp->mAssetStatus = ASSET_FETCH_FAILED;
			}
			
			delete[] buffer;
		}
		else
		{
			llwarns << "Failed to load asset for animation " << motionp->getName() << ":" << motionp->getID() << llendl;
			motionp->mAssetStatus = ASSET_FETCH_FAILED;
		}
	}
	else
	{
		llwarns << "No existing motion for asset data. UUID: " << asset_uuid << llendl;
	}
}

//--------------------------------------------------------------------
// LLKeyframeDataCache::dumpDiagInfo()
//--------------------------------------------------------------------
void LLKeyframeDataCache::dumpDiagInfo()
{
	// keep track of totals
	U32 total_size = 0;

	char buf[1024];		/* Flawfinder: ignore */

	llinfos << "-----------------------------------------------------" << llendl;
	llinfos << "       Global Motion Table (DEBUG only)" << llendl;
	llinfos << "-----------------------------------------------------" << llendl;

	// print each loaded mesh, and it's memory usage
	for (keyframe_data_map_t::iterator map_it = sKeyframeDataMap.begin();
		 map_it != sKeyframeDataMap.end(); ++map_it)
	{
		U32 joint_motion_kb;

		LLKeyframeMotion::JointMotionList *motion_list_p = map_it->second;

		llinfos << "Motion: " << map_it->first << llendl;

		joint_motion_kb = motion_list_p->dumpDiagInfo();

		total_size += joint_motion_kb;
	}

	llinfos << "-----------------------------------------------------" << llendl;
	llinfos << "Motions\tTotal Size" << llendl;
	snprintf(buf, sizeof(buf), "%d\t\t%d bytes", (S32)sKeyframeDataMap.size(), total_size );		/* Flawfinder: ignore */
	llinfos << buf << llendl;
	llinfos << "-----------------------------------------------------" << llendl;
}


//--------------------------------------------------------------------
// LLKeyframeDataCache::addKeyframeData()
//--------------------------------------------------------------------
void LLKeyframeDataCache::addKeyframeData(const LLUUID& id, LLKeyframeMotion::JointMotionList* joint_motion_listp)
{
	sKeyframeDataMap[id] = joint_motion_listp;
}

//--------------------------------------------------------------------
// LLKeyframeDataCache::removeKeyframeData()
//--------------------------------------------------------------------
void LLKeyframeDataCache::removeKeyframeData(const LLUUID& id)
{
	keyframe_data_map_t::iterator found_data = sKeyframeDataMap.find(id);
	if (found_data != sKeyframeDataMap.end())
	{
		delete found_data->second;
		sKeyframeDataMap.erase(found_data);
	}
}

//--------------------------------------------------------------------
// LLKeyframeDataCache::getKeyframeData()
//--------------------------------------------------------------------
LLKeyframeMotion::JointMotionList* LLKeyframeDataCache::getKeyframeData(const LLUUID& id)
{
	keyframe_data_map_t::iterator found_data = sKeyframeDataMap.find(id);
	if (found_data == sKeyframeDataMap.end())
	{
		return NULL;
	}
	return found_data->second;
}

//--------------------------------------------------------------------
// ~LLKeyframeDataCache::LLKeyframeDataCache()
//--------------------------------------------------------------------
LLKeyframeDataCache::~LLKeyframeDataCache()
{
	clear();
}

//-----------------------------------------------------------------------------
// clear()
//-----------------------------------------------------------------------------
void LLKeyframeDataCache::clear()
{
	for_each(sKeyframeDataMap.begin(), sKeyframeDataMap.end(), DeletePairedPointer());
	sKeyframeDataMap.clear();
}

//-----------------------------------------------------------------------------
// JointConstraint()
//-----------------------------------------------------------------------------
LLKeyframeMotion::JointConstraint::JointConstraint(JointConstraintSharedData* shared_data) : mSharedData(shared_data) 
{
	mWeight = 0.f;
	mTotalLength = 0.f;
	mActive = FALSE;
	mSourceVolume = NULL;
	mTargetVolume = NULL;
	mFixupDistanceRMS = 0.f;

	int i;
	for (i=0; i<MAX_CHAIN_LENGTH; ++i)
	{
		mJointLengths[i] = 0.f;
		mJointLengthFractions[i] = 0.f;
	}
}

//-----------------------------------------------------------------------------
// ~JointConstraint()
//-----------------------------------------------------------------------------
LLKeyframeMotion::JointConstraint::~JointConstraint()
{
}

// End
