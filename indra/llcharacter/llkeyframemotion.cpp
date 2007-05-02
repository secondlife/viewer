/** 
 * @file llkeyframemotion.cpp
 * @brief Implementation of LLKeyframeMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	: mNumJointMotions(0),
	  mJointMotionArray(NULL)
{
}

LLKeyframeMotion::JointMotionList::~JointMotionList()
{
	for_each(mConstraints.begin(), mConstraints.end(), DeletePointer());
	delete [] mJointMotionArray;
}

U32 LLKeyframeMotion::JointMotionList::dumpDiagInfo()
{
	S32	total_size = sizeof(JointMotionList);

	for (U32 i = 0; i < mNumJointMotions; i++)
	{
		LLKeyframeMotion::JointMotion* joint_motion_p = &mJointMotionArray[i];

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
	mKeys.deleteAllData();
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// getValue()
//-----------------------------------------------------------------------------
LLVector3 LLKeyframeMotion::ScaleCurve::getValue(F32 time, F32 duration)
{
	LLVector3 value;
	F32 index_before, index_after;
	ScaleKey* scale_before;
	ScaleKey* scale_after;

	mKeys.getInterval(time, index_before, index_after, scale_before, scale_after);
	if (scale_before)
	{
		if (!scale_after)
		{
			scale_after = &mLoopInKey;
			index_after = duration;
		}

		if (index_after == index_before)
		{
			value = scale_after->mScale;
		}
		else
		{
			F32 u = (time - index_before) / (index_after - index_before);
			value = interp(u, *scale_before, *scale_after);
		}
	}
	else
	{
		// before first key
		if (scale_after)
		{
			value = scale_after->mScale;
		}
		// no keys?
		else
		{
			value.clearVec();
		}
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
	mKeys.deleteAllData();
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// RotationCurve::getValue()
//-----------------------------------------------------------------------------
LLQuaternion LLKeyframeMotion::RotationCurve::getValue(F32 time, F32 duration)
{
	LLQuaternion value;
	F32 index_before, index_after;
	RotationKey* rot_before;
	RotationKey* rot_after;

	mKeys.getInterval(time, index_before, index_after, rot_before, rot_after);

	if (rot_before)
	{
		if (!rot_after)
		{
			rot_after = &mLoopInKey;
			index_after = duration;
		}

		if (index_after == index_before)
		{
			value = rot_after->mRotation;
		}
		else
		{
			F32 u = (time - index_before) / (index_after - index_before);
			value = interp(u, *rot_before, *rot_after);
		}
	}
	else
	{
		// before first key
		if (rot_after)
		{
			value = rot_after->mRotation;
		}
		// no keys?
		else
		{
			value = LLQuaternion::DEFAULT;
		}
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
	mKeys.deleteAllData();
	mNumKeys = 0;
}

//-----------------------------------------------------------------------------
// PositionCurve::getValue()
//-----------------------------------------------------------------------------
LLVector3 LLKeyframeMotion::PositionCurve::getValue(F32 time, F32 duration)
{
	LLVector3 value;
	F32 index_before, index_after;
	PositionKey* pos_before;
	PositionKey* pos_after;

	mKeys.getInterval(time, index_before, index_after, pos_before, pos_after);

	if (pos_before)
	{
		if (!pos_after)
		{
			pos_after = &mLoopInKey;
			index_after = duration;
		}

		if (index_after == index_before)
		{
			value = pos_after->mPosition;
		}
		else
		{
			F32 u = (time - index_before) / (index_after - index_before);
			value = interp(u, *pos_before, *pos_after);
		}
	}
	else
	{
		// before first key
		if (pos_after)
		{
			value = pos_after->mPosition;
		}
		// no keys?
		else
		{
			value.clearVec();
		}
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
	if ( joint_state == 0 )
	{
		return;
	};

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
LLKeyframeMotion::LLKeyframeMotion( const LLUUID &id) : LLMotion(id)
{
	mJointMotionList = NULL;
	mJointStates = NULL;
	mLastSkeletonSerialNum = 0;
	mLastLoopedTime = 0.f;
	mLastUpdateTime = 0.f;
	mAssetStatus = ASSET_UNDEFINED;
	mPelvisp = NULL;
}


//-----------------------------------------------------------------------------
// ~LLKeyframeMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLKeyframeMotion::~LLKeyframeMotion()
{
	if (mJointStates)
	{
		delete [] mJointStates;
	}
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

		// don't forget to allocate joint states
		mJointStates = new LLJointState[mJointMotionList->mNumJointMotions];

		// set up joint states to point to character joints
		for(U32 i = 0; i < mJointMotionList->mNumJointMotions; i++)
		{
			if (LLJoint *jointp = mCharacter->getJoint(mJointMotionList->mJointMotionArray[i].mJointName))
			{
				mJointStates[i].setJoint(jointp);
				mJointStates[i].setUsage(mJointMotionList->mJointMotionArray[i].mUsage);
				mJointStates[i].setPriority(joint_motion_list->mJointMotionArray[i].mPriority);
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
	for (U32 jm=0; jm<mJointMotionList->mNumJointMotions; jm++)
	{
		if ( mJointStates[jm].getJoint() )
		{
			addJointState( &mJointStates[jm] );
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
	if( mEmoteName.length() > 0 )
	{
		mCharacter->startMotion( gAnimLibrary.stringToAnimState(mEmoteName.c_str()) );
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
	U32 i;
	for (i=0; i<mJointMotionList->mNumJointMotions; i++)
	{
		mJointMotionList->mJointMotionArray[i].update(
			&mJointStates[i], 
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
	LLJoint* cur_joint = mJointStates[shared_data->mJointStateIndices[0]].getJoint();

	F32 source_pos_offset = dist_vec(source_pos, cur_joint->getWorldPosition());

	constraint->mTotalLength = constraint->mJointLengths[0] = dist_vec(cur_joint->getParent()->getWorldPosition(), source_pos);
	
	// grab joint lengths
	for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
	{
		cur_joint = mJointStates[shared_data->mJointStateIndices[joint_num]].getJoint();
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
	if (shared_data->mConstraintTargetType == TYPE_GROUND)
	{
		LLVector3 source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);
		LLVector3 ground_pos_agent;
		mCharacter->getGround(source_pos, ground_pos_agent, constraint->mGroundNorm);
		constraint->mGroundPos = mCharacter->getPosGlobalFromAgent(ground_pos_agent + shared_data->mTargetConstraintOffset);
	}

	for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
	{
		LLJoint* cur_joint = mJointStates[shared_data->mJointStateIndices[joint_num]].getJoint();
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

	if (!constraintp->mSharedData->mConstraintTargetType == TYPE_GROUND)
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
	LLJoint*		cur_joint;

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

	LLJoint* root_joint = mJointStates[shared_data->mJointStateIndices[shared_data->mChainLength]].getJoint();
	LLVector3 root_pos = root_joint->getWorldPosition();
//	LLQuaternion root_rot = 
	root_joint->getParent()->getWorldRotation();
//	LLQuaternion inv_root_rot = ~root_rot;

//	LLVector3 current_source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);

	//apply underlying keyframe animation to get nominal "kinematic" joint positions
	for (joint_num = 0; joint_num <= shared_data->mChainLength; joint_num++)
	{
		cur_joint = mJointStates[shared_data->mJointStateIndices[joint_num]].getJoint();
		if (joint_mask[cur_joint->getJointNum()] >= (0xff >> (7 - getPriority())))
		{
			// skip constraint
			return;
		}
		old_rots[joint_num] = cur_joint->getRotation();
		cur_joint->setRotation(mJointStates[shared_data->mJointStateIndices[joint_num]].getRotation());
	}


	LLVector3 keyframe_source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);
	LLVector3 target_pos;

	switch(shared_data->mConstraintTargetType)
	{
	case TYPE_GROUND:
		target_pos = mCharacter->getPosAgentFromGlobal(constraint->mGroundPos);
//		llinfos << "Target Pos " << constraint->mGroundPos << " on " << mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume)->getName() << llendl;
		break;
	case TYPE_BODY:
		target_pos = mCharacter->getVolumePos(shared_data->mTargetConstraintVolume, shared_data->mTargetConstraintOffset);
		break;
	default:
		break;
	}
	
	LLVector3 norm;
	LLJoint *source_jointp = NULL;
	LLJoint *target_jointp = NULL;

	if (shared_data->mConstraintType == TYPE_PLANE)
	{
		switch(shared_data->mConstraintTargetType)
		{
		case TYPE_GROUND:
			norm = constraint->mGroundNorm;
			break;
		case TYPE_BODY:
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
		LLQuaternion end_rot = mJointStates[shared_data->mJointStateIndices[0]].getJoint()->getWorldRotation();

		// slam start and end of chain to the proper positions (rest of chain stays put)
		positions[0] = lerp(keyframe_source_pos, target_pos, weight);
		positions[shared_data->mChainLength] = root_pos;

		// grab keyframe-specified positions of joints	
		for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
		{
			LLVector3 kinematic_position = mJointStates[shared_data->mJointStateIndices[joint_num]].getJoint()->getWorldPosition() + 
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
			LLQuaternion parent_rot = mJointStates[shared_data->mJointStateIndices[joint_num]].getJoint()->getParent()->getWorldRotation();
			cur_joint = mJointStates[shared_data->mJointStateIndices[joint_num]].getJoint();
			LLJoint* child_joint = mJointStates[shared_data->mJointStateIndices[joint_num - 1]].getJoint();

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
				LLQuaternion cur_rot = mJointStates[shared_data->mJointStateIndices[joint_num]].getRotation();
				target_rot = nlerp(weight, cur_rot, target_rot);
			}

			mJointStates[shared_data->mJointStateIndices[joint_num]].setRotation(target_rot);
			cur_joint->setRotation(target_rot);
		}

		LLJoint* end_joint = mJointStates[shared_data->mJointStateIndices[0]].getJoint();
		LLQuaternion end_local_rot = end_rot * ~end_joint->getParent()->getWorldRotation();

		if (weight == 1.f)
		{
			mJointStates[shared_data->mJointStateIndices[0]].setRotation(end_local_rot);
		}
		else
		{
			LLQuaternion cur_rot = mJointStates[shared_data->mJointStateIndices[0]].getRotation();
			mJointStates[shared_data->mJointStateIndices[0]].setRotation(nlerp(weight, cur_rot, end_local_rot));
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
		constraint->mFixupDistanceRMS = fsqrtf(constraint->mFixupDistanceRMS);

		//reset old joint rots
		for (joint_num = 0; joint_num <= shared_data->mChainLength; joint_num++)
		{
			mJointStates[shared_data->mJointStateIndices[joint_num]].getJoint()->setRotation(old_rots[joint_num]);
		}
	}
	// simple positional constraint (pelvis only)
	else if (mJointStates[shared_data->mJointStateIndices[0]].getUsage() & LLJointState::POS)
	{
		LLVector3 delta = source_to_target * weight;
		LLJointState* current_joint_statep = &mJointStates[shared_data->mJointStateIndices[0]];
		LLQuaternion parent_rot = current_joint_statep->getJoint()->getParent()->getWorldRotation();
		delta = delta * ~parent_rot;
		current_joint_statep->setPosition(current_joint_statep->getJoint()->getPosition() + delta);
	}
}

//-----------------------------------------------------------------------------
// deserialize()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotion::deserialize(LLDataPacker& dp)
{
	BOOL old_version = FALSE;
	mJointMotionList = new LLKeyframeMotion::JointMotionList;
	mJointMotionList->mNumJointMotions = 0;

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
		llwarns << "can't read priority" << llendl;
		return FALSE;
	}
	mJointMotionList->mBasePriority = (LLJoint::JointPriority) temp_priority;

	if (mJointMotionList->mBasePriority >= LLJoint::ADDITIVE_PRIORITY)
	{
		mJointMotionList->mBasePriority = (LLJoint::JointPriority)((int)LLJoint::ADDITIVE_PRIORITY-1);
		mJointMotionList->mMaxPriority = mJointMotionList->mBasePriority;
	}

	//-------------------------------------------------------------------------
	// get duration
	//-------------------------------------------------------------------------
	if (!dp.unpackF32(mJointMotionList->mDuration, "duration"))
	{
		llwarns << "can't read duration" << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// get emote (optional)
	//-------------------------------------------------------------------------
	if (!dp.unpackString(mEmoteName, "emote_name"))
	{
		llwarns << "can't read optional_emote_animation" << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// get loop
	//-------------------------------------------------------------------------
	if (!dp.unpackF32(mJointMotionList->mLoopInPoint, "loop_in_point"))
	{
		llwarns << "can't read loop point" << llendl;
		return FALSE;
	}

	if (!dp.unpackF32(mJointMotionList->mLoopOutPoint, "loop_out_point"))
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
	if (!dp.unpackF32(mJointMotionList->mEaseInDuration, "ease_in_duration"))
	{
		llwarns << "can't read easeIn" << llendl;
		return FALSE;
	}

	if (!dp.unpackF32(mJointMotionList->mEaseOutDuration, "ease_out_duration"))
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
	mJointMotionList->mHandPose = (LLHandMotion::eHandPose)word;

	//-------------------------------------------------------------------------
	// get number of joint motions
	//-------------------------------------------------------------------------
	if (!dp.unpackU32(mJointMotionList->mNumJointMotions, "num_joints"))
	{
		llwarns << "can't read number of joints" << llendl;
		return FALSE;
	}

	if (mJointMotionList->mNumJointMotions == 0)
	{
		llwarns << "no joints in animation" << llendl;
		return FALSE;
	}
	else if (mJointMotionList->mNumJointMotions > LL_CHARACTER_MAX_JOINTS)
	{
		llwarns << "too many joints in animation" << llendl;
		return FALSE;
	}

	mJointMotionList->mJointMotionArray = new JointMotion[mJointMotionList->mNumJointMotions];
	mJointStates = new LLJointState[mJointMotionList->mNumJointMotions];

	if (!mJointMotionList->mJointMotionArray)
	{
		mJointMotionList->mDuration = 0.0f;
		mJointMotionList->mEaseInDuration = 0.0f;
		mJointMotionList->mEaseOutDuration = 0.0f;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// initialize joint motions
	//-------------------------------------------------------------------------
	S32 k;
	for(U32 i=0; i<mJointMotionList->mNumJointMotions; ++i)
	{
		std::string joint_name;
		if (!dp.unpackString(joint_name, "joint_name"))
		{
			llwarns << "can't read joint name" << llendl;
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

		mJointMotionList->mJointMotionArray[i].mJointName = joint_name;
		mJointStates[i].setJoint( joint );
		mJointStates[i].setUsage( 0 );

		//---------------------------------------------------------------------
		// get joint priority
		//---------------------------------------------------------------------
		S32 joint_priority;
		if (!dp.unpackS32(joint_priority, "joint_priority"))
		{
			llwarns << "can't read joint priority." << llendl;
			return FALSE;
		}
		
		mJointMotionList->mJointMotionArray[i].mPriority = (LLJoint::JointPriority)joint_priority;
		if (joint_priority != LLJoint::USE_MOTION_PRIORITY &&
			joint_priority > mJointMotionList->mMaxPriority)
		{
			mJointMotionList->mMaxPriority = (LLJoint::JointPriority)joint_priority;
		}

		mJointStates[i].setPriority((LLJoint::JointPriority)joint_priority);

		//---------------------------------------------------------------------
		// scan rotation curve header
		//---------------------------------------------------------------------
		if (!dp.unpackS32(mJointMotionList->mJointMotionArray[i].mRotationCurve.mNumKeys, "num_rot_keys"))
		{
			llwarns << "can't read number of rotation keys" << llendl;
			return FALSE;
		}

		mJointMotionList->mJointMotionArray[i].mRotationCurve.mInterpolationType = IT_LINEAR;
		if (mJointMotionList->mJointMotionArray[i].mRotationCurve.mNumKeys != 0)
		{
			mJointStates[i].setUsage(mJointStates[i].getUsage() | LLJointState::ROT );
		}

		//---------------------------------------------------------------------
		// scan rotation curve keys
		//---------------------------------------------------------------------
		RotationCurve *rCurve = &mJointMotionList->mJointMotionArray[i].mRotationCurve;

		for (k = 0; k < mJointMotionList->mJointMotionArray[i].mRotationCurve.mNumKeys; k++)
		{
			F32 time;
			U16 time_short;

			if (old_version)
			{
				if (!dp.unpackF32(time, "time"))
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
			}
			
			RotationKey *rot_key = new RotationKey;
			rot_key->mTime = time;
			LLVector3 rot_angles;
			U16 x, y, z;

			BOOL success = TRUE;

			if (old_version)
			{
				success = dp.unpackVector3(rot_angles, "rot_angles");

				LLQuaternion::Order ro = StringToOrder("ZYX");
				rot_key->mRotation = mayaQ(rot_angles.mV[VX], rot_angles.mV[VY], rot_angles.mV[VZ], ro);
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
				rot_key->mRotation.unpackFromVector3(rot_vec);
			}

			if (!success)
			{
				llwarns << "can't read rotation key (" << k << ")" << llendl;
				delete rot_key;
				return FALSE;
			}

			rCurve->mKeys[time] = rot_key;
		}

		//---------------------------------------------------------------------
		// scan position curve header
		//---------------------------------------------------------------------
		if (!dp.unpackS32(mJointMotionList->mJointMotionArray[i].mPositionCurve.mNumKeys, "num_pos_keys"))
		{
			llwarns << "can't read number of position keys" << llendl;
			return FALSE;
		}

		mJointMotionList->mJointMotionArray[i].mPositionCurve.mInterpolationType = IT_LINEAR;
		if (mJointMotionList->mJointMotionArray[i].mPositionCurve.mNumKeys != 0)
		{
			mJointStates[i].setUsage(mJointStates[i].getUsage() | LLJointState::POS );
		}

		//---------------------------------------------------------------------
		// scan position curve keys
		//---------------------------------------------------------------------
		PositionCurve *pCurve = &mJointMotionList->mJointMotionArray[i].mPositionCurve;
		BOOL is_pelvis = mJointMotionList->mJointMotionArray[i].mJointName == "mPelvis";
		for (k = 0; k < mJointMotionList->mJointMotionArray[i].mPositionCurve.mNumKeys; k++)
		{
			U16 time_short;
			PositionKey* pos_key = new PositionKey;

			if (old_version)
			{
				if (!dp.unpackF32(pos_key->mTime, "time"))
				{
					llwarns << "can't read position key (" << k << ")" << llendl;
					delete pos_key;
					return FALSE;
				}
			}
			else
			{
				if (!dp.unpackU16(time_short, "time"))
				{
					llwarns << "can't read position key (" << k << ")" << llendl;
					delete pos_key;
					return FALSE;
				}

				pos_key->mTime = U16_to_F32(time_short, 0.f, mJointMotionList->mDuration);
			}

			BOOL success = TRUE;

			if (old_version)
			{
				success = dp.unpackVector3(pos_key->mPosition, "pos");
			}
			else
			{
				U16 x, y, z;

				success &= dp.unpackU16(x, "pos_x");
				success &= dp.unpackU16(y, "pos_y");
				success &= dp.unpackU16(z, "pos_z");

				pos_key->mPosition.mV[VX] = U16_to_F32(x, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				pos_key->mPosition.mV[VY] = U16_to_F32(y, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				pos_key->mPosition.mV[VZ] = U16_to_F32(z, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			}

			if (!success)
			{
				llwarns << "can't read position key (" << k << ")" << llendl;
				delete pos_key;
				return FALSE;
			}
			
			pCurve->mKeys[pos_key->mTime] = pos_key;

			if (is_pelvis)
			{
				mJointMotionList->mPelvisBBox.addPoint(pos_key->mPosition);
			}
		}

		mJointMotionList->mJointMotionArray[i].mUsage = mJointStates[i].getUsage();
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

	if (num_constraints > MAX_CONSTRAINTS)
	{
		llwarns << "Too many constraints...ignoring" << llendl;
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

			if (!dp.unpackU8(byte, "constraint_type"))
			{
				llwarns << "can't read constraint type" << llendl;
				delete constraintp;
				return FALSE;
			}
			constraintp->mConstraintType = (EConstraintType)byte;

			const S32 BIN_DATA_LENGTH = 16;
			U8 bin_data[BIN_DATA_LENGTH];
			if (!dp.unpackBinaryDataFixed(bin_data, BIN_DATA_LENGTH, "source_volume"))
			{
				llwarns << "can't read source volume name" << llendl;
				delete constraintp;
				return FALSE;
			}

			bin_data[BIN_DATA_LENGTH-1] = 0; // Ensure null termination
			str = (char*)bin_data;
			constraintp->mSourceConstraintVolume = mCharacter->getCollisionVolumeID(str);

			if (!dp.unpackVector3(constraintp->mSourceConstraintOffset, "source_offset"))
			{
				llwarns << "can't read constraint source offset" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackBinaryDataFixed(bin_data, BIN_DATA_LENGTH, "target_volume"))
			{
				llwarns << "can't read target volume name" << llendl;
				delete constraintp;
				return FALSE;
			}

			bin_data[BIN_DATA_LENGTH-1] = 0; // Ensure null termination
			str = (char*)bin_data;
			if (str == "GROUND")
			{
				// constrain to ground
				constraintp->mConstraintTargetType = TYPE_GROUND;
			}
			else
			{
				constraintp->mConstraintTargetType = TYPE_BODY;
				constraintp->mTargetConstraintVolume = mCharacter->getCollisionVolumeID(str);
			}

			if (!dp.unpackVector3(constraintp->mTargetConstraintOffset, "target_offset"))
			{
				llwarns << "can't read constraint target offset" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackVector3(constraintp->mTargetConstraintDir, "target_dir"))
			{
				llwarns << "can't read constraint target direction" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!constraintp->mTargetConstraintDir.isExactlyZero())
			{
				constraintp->mUseTargetOffset = TRUE;
	//			constraintp->mTargetConstraintDir *= constraintp->mSourceConstraintOffset.magVec();
			}

			if (!dp.unpackF32(constraintp->mEaseInStartTime, "ease_in_start"))
			{
				llwarns << "can't read constraint ease in start time" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackF32(constraintp->mEaseInStopTime, "ease_in_stop"))
			{
				llwarns << "can't read constraint ease in stop time" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackF32(constraintp->mEaseOutStartTime, "ease_out_start"))
			{
				llwarns << "can't read constraint ease out start time" << llendl;
				delete constraintp;
				return FALSE;
			}

			if (!dp.unpackF32(constraintp->mEaseOutStopTime, "ease_out_stop"))
			{
				llwarns << "can't read constraint ease out stop time" << llendl;
				delete constraintp;
				return FALSE;
			}

			mJointMotionList->mConstraints.push_front(constraintp);

			constraintp->mJointStateIndices = new S32[constraintp->mChainLength + 1];
			
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
							<< " Emote: " << mEmoteName << llendl;
					return FALSE;
				}
				joint = parent;
				constraintp->mJointStateIndices[i] = -1;
				for (U32 j = 0; j < mJointMotionList->mNumJointMotions; j++)
				{
					if(mJointStates[j].getJoint() == joint)
					{
						constraintp->mJointStateIndices[i] = (S32)j;
						break;
					}
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
	success &= dp.packString(mEmoteName.c_str(), "emote_name");
	success &= dp.packF32(mJointMotionList->mLoopInPoint, "loop_in_point");
	success &= dp.packF32(mJointMotionList->mLoopOutPoint, "loop_out_point");
	success &= dp.packS32(mJointMotionList->mLoop, "loop");
	success &= dp.packF32(mJointMotionList->mEaseInDuration, "ease_in_duration");
	success &= dp.packF32(mJointMotionList->mEaseOutDuration, "ease_out_duration");
	success &= dp.packU32(mJointMotionList->mHandPose, "hand_pose");
	success &= dp.packU32(mJointMotionList->mNumJointMotions, "num_joints");

	for (U32 i = 0; i < mJointMotionList->mNumJointMotions; i++)
	{
		JointMotion* joint_motionp = &mJointMotionList->mJointMotionArray[i];
		success &= dp.packString(joint_motionp->mJointName.c_str(), "joint_name");
		success &= dp.packS32(joint_motionp->mPriority, "joint_priority");
		success &= dp.packS32(joint_motionp->mRotationCurve.mNumKeys, "num_rot_keys");

		for (RotationKey* rot_keyp = joint_motionp->mRotationCurve.mKeys.getFirstData();
			rot_keyp;
			rot_keyp = joint_motionp->mRotationCurve.mKeys.getNextData())
		{
			U16 time_short = F32_to_U16(rot_keyp->mTime, 0.f, mJointMotionList->mDuration);
			success &= dp.packU16(time_short, "time");

			LLVector3 rot_angles = rot_keyp->mRotation.packToVector3();
			
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
		for (PositionKey* pos_keyp = joint_motionp->mPositionCurve.mKeys.getFirstData();
			pos_keyp;
			pos_keyp = joint_motionp->mPositionCurve.mKeys.getNextData())
		{
			U16 time_short = F32_to_U16(pos_keyp->mTime, 0.f, mJointMotionList->mDuration);
			success &= dp.packU16(time_short, "time");

			U16 x, y, z;
			pos_keyp->mPosition.quantize16(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			x = F32_to_U16(pos_keyp->mPosition.mV[VX], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			y = F32_to_U16(pos_keyp->mPosition.mV[VY], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			z = F32_to_U16(pos_keyp->mPosition.mV[VZ], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
			success &= dp.packU16(x, "pos_x");
			success &= dp.packU16(y, "pos_y");
			success &= dp.packU16(z, "pos_z");
		}
	}	

	success &= dp.packS32(mJointMotionList->mConstraints.size(), "num_constraints");
	for (JointMotionList::constraint_list_t::iterator iter = mJointMotionList->mConstraints.begin();
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
		if (shared_constraintp->mConstraintTargetType == TYPE_GROUND)
		{
			success &= dp.packU8(shared_constraintp->mChainLength, "chain_length");
			success &= dp.packU8(shared_constraintp->mConstraintType, "constraint_type");
			char volume_name[16];	/* Flawfinder: ignore */
			snprintf(volume_name, sizeof(volume_name), "%s",	/* Flawfinder: ignore */
				mCharacter->findCollisionVolume(shared_constraintp->mSourceConstraintVolume)->getName().c_str()); 
			success &= dp.packBinaryDataFixed((U8*)volume_name, 16, "source_volume");
			success &= dp.packVector3(shared_constraintp->mSourceConstraintOffset, "source_offset");
			if (shared_constraintp->mConstraintTargetType == TYPE_GROUND)
			{
				snprintf(volume_name,sizeof(volume_name), "%s", "GROUND");	/* Flawfinder: ignore */
			}
			else
			{
				snprintf(volume_name, sizeof(volume_name),"%s", 	/* Flawfinder: ignore */
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

		for (U32 i = 0; i < mJointMotionList->mNumJointMotions; i++)
		{
			mJointMotionList->mJointMotionArray[i].mPriority = (LLJoint::JointPriority)llclamp(
				(S32)mJointMotionList->mJointMotionArray[i].mPriority + priority_delta,
				(S32)LLJoint::LOW_PRIORITY, 
				(S32)LLJoint::HIGHEST_PRIORITY);
			mJointStates[i].setPriority(mJointMotionList->mJointMotionArray[i].mPriority);
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
		mEmoteName = emote_name;
	}
	else
	{
		mEmoteName = "";
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
		for (U32 i = 0; i < mJointMotionList->mNumJointMotions; i++)
		{
			PositionCurve* pos_curve = &mJointMotionList->mJointMotionArray[i].mPositionCurve;
			RotationCurve* rot_curve = &mJointMotionList->mJointMotionArray[i].mRotationCurve;
			ScaleCurve* scale_curve = &mJointMotionList->mJointMotionArray[i].mScaleCurve;
			
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
		for (U32 i = 0; i < mJointMotionList->mNumJointMotions; i++)
		{
			PositionCurve* pos_curve = &mJointMotionList->mJointMotionArray[i].mPositionCurve;
			RotationCurve* rot_curve = &mJointMotionList->mJointMotionArray[i].mRotationCurve;
			ScaleCurve* scale_curve = &mJointMotionList->mJointMotionArray[i].mScaleCurve;
			
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
									   void* user_data, S32 status)
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

	// create an instance of this motion (it may or may not already exist)
	LLKeyframeMotion* motionp = (LLKeyframeMotion*) character->createMotion(asset_uuid);

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
			
			delete []buffer;
		}
		else
		{
			llwarns << "Failed to load asset for animation " << motionp->getName() << ":" << motionp->getID() << llendl;
			motionp->mAssetStatus = ASSET_FETCH_FAILED;
		}
	}
	else
	{
		// motionp is NULL
		llwarns << "Failed to createMotion() for asset UUID " << asset_uuid << llendl;
	}
}


//-----------------------------------------------------------------------------
// writeCAL3D()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::writeCAL3D(apr_file_t* fp)
{
//	<ANIMATION VERSION="1000" DURATION="1.03333" NUMTRACKS="58">
//		<TRACK BONEID="0" NUMKEYFRAMES="31">
//			<KEYFRAME TIME="0">
//				<TRANSLATION>0 0 48.8332</TRANSLATION>
//				<ROTATION>0.0512905 0.05657 0.66973 0.738668</ROTATION>
//			</KEYFRAME>
//			</TRACK>
//	</ANIMATION>

	apr_file_printf(fp, "<ANIMATION VERSION=\"1000\" DURATION=\"%.5f\" NUMTRACKS=\"%d\">\n",  getDuration(), mJointMotionList->mNumJointMotions);
	for (U32 joint_index = 0; joint_index < mJointMotionList->mNumJointMotions; joint_index++)
	{
		JointMotion* joint_motionp = &mJointMotionList->mJointMotionArray[joint_index];
		LLJoint* animated_joint = mCharacter->getJoint(joint_motionp->mJointName);
		S32 joint_num = animated_joint->mJointNum + 1;

		apr_file_printf(fp, "	<TRACK BONEID=\"%d\" NUMKEYFRAMES=\"%d\">\n",  joint_num, joint_motionp->mRotationCurve.mNumKeys );
		PositionKey* pos_keyp = joint_motionp->mPositionCurve.mKeys.getFirstData();
		for (RotationKey* rot_keyp = joint_motionp->mRotationCurve.mKeys.getFirstData();
			rot_keyp;
			rot_keyp = joint_motionp->mRotationCurve.mKeys.getNextData())
		{
			apr_file_printf(fp, "		<KEYFRAME TIME=\"%0.3f\">\n", rot_keyp->mTime);
			LLVector3 nominal_pos = animated_joint->getPosition();
			if (animated_joint->getParent())
			{
				nominal_pos.scaleVec(animated_joint->getParent()->getScale());
			}
			nominal_pos = nominal_pos * 100.f;

			if (joint_motionp->mUsage & LLJointState::POS && pos_keyp)
			{
				LLVector3 pos_val = pos_keyp->mPosition;
				pos_val = pos_val * 100.f;
				pos_val += nominal_pos;
				apr_file_printf(fp, "			<TRANSLATION>%0.4f %0.4f %0.4f</TRANSLATION>\n", pos_val.mV[VX], pos_val.mV[VY], pos_val.mV[VZ]);
				pos_keyp = joint_motionp->mPositionCurve.mKeys.getNextData();
			}
			else
			{
				apr_file_printf(fp, "			<TRANSLATION>%0.4f %0.4f %0.4f</TRANSLATION>\n", nominal_pos.mV[VX], nominal_pos.mV[VY], nominal_pos.mV[VZ]);
			}

			LLQuaternion rot_val = ~rot_keyp->mRotation;
			apr_file_printf(fp, "			<ROTATION>%0.4f %0.4f %0.4f %0.4f</ROTATION>\n", rot_val.mQ[VX], rot_val.mQ[VY], rot_val.mQ[VZ], rot_val.mQ[VW]);
			apr_file_printf(fp, "		</KEYFRAME>\n");
		}
		apr_file_printf(fp, "	</TRACK>\n");
	}
	apr_file_printf(fp, "</ANIMATION>\n");
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
	mTotalLength = 0.f;
	mActive = FALSE;
	mSourceVolume = NULL;
	mTargetVolume = NULL;
	mFixupDistanceRMS = 0.f;
}

//-----------------------------------------------------------------------------
// ~JointConstraint()
//-----------------------------------------------------------------------------
LLKeyframeMotion::JointConstraint::~JointConstraint()
{
}

// End
