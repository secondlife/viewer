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
#include "m3math.h"
#include "message.h"
#include "llfilesystem.h"

//-----------------------------------------------------------------------------
// Static Definitions
//-----------------------------------------------------------------------------
LLKeyframeDataCache::keyframe_data_map_t    LLKeyframeDataCache::sKeyframeDataMap;

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
      mLoop(false),
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
    mConstraints.clear();
    for_each(mJointMotionArray.begin(), mJointMotionArray.end(), DeletePointer());
    mJointMotionArray.clear();
}

U32 LLKeyframeMotion::JointMotionList::dumpDiagInfo()
{
    S32 total_size = sizeof(JointMotionList);

    for (U32 i = 0; i < getNumJointMotions(); i++)
    {
        LLKeyframeMotion::JointMotion* joint_motion_p = mJointMotionArray[i];

        LL_INFOS() << "\tJoint " << joint_motion_p->mJointName << LL_ENDL;
        if (joint_motion_p->mUsage & LLJointState::SCALE)
        {
            LL_INFOS() << "\t" << joint_motion_p->mScaleCurve.mNumKeys << " scale keys at "
            << joint_motion_p->mScaleCurve.mNumKeys * sizeof(ScaleKey) << " bytes" << LL_ENDL;

            total_size += joint_motion_p->mScaleCurve.mNumKeys * sizeof(ScaleKey);
        }
        if (joint_motion_p->mUsage & LLJointState::ROT)
        {
            LL_INFOS() << "\t" << joint_motion_p->mRotationCurve.mNumKeys << " rotation keys at "
            << joint_motion_p->mRotationCurve.mNumKeys * sizeof(RotationKey) << " bytes" << LL_ENDL;

            total_size += joint_motion_p->mRotationCurve.mNumKeys * sizeof(RotationKey);
        }
        if (joint_motion_p->mUsage & LLJointState::POS)
        {
            LL_INFOS() << "\t" << joint_motion_p->mPositionCurve.mNumKeys << " position keys at "
            << joint_motion_p->mPositionCurve.mNumKeys * sizeof(PositionKey) << " bytes" << LL_ENDL;

            total_size += joint_motion_p->mPositionCurve.mNumKeys * sizeof(PositionKey);
        }
    }
    LL_INFOS() << "Size: " << total_size << " bytes" << LL_ENDL;

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
        mJointMotionList(nullptr),
        mPelvisp(nullptr),
        mCharacter(nullptr),
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
    mConstraints.clear();
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
// getJoint()
//-----------------------------------------------------------------------------
LLJoint* LLKeyframeMotion::getJoint(U32 index)
{
    llassert_always (index < mJointStates.size());
    LLJoint* joint = mJointStates[index]->getJoint();

    //Commented out 06-28-11 by Aura.
    //llassert_always (joint);
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

        if (mID.notNull())
        {
            LL_DEBUGS("Animation") << "Requesting data fetch for: " << mID << LL_ENDL;
            character_id = new LLUUID(mCharacter->getID());
            gAssetStorage->getAssetData(mID,
                                        LLAssetType::AT_ANIMATION,
                                        onLoadComplete,
                                        (void*)character_id,
                                        false);
        }
        else
        {
            LL_INFOS("Animation") << "Attempted to fetch animation '" << mName << "' with null id"
                << " for character " << mCharacter->getID() << LL_ENDL;
        }

        return STATUS_HOLD;
    case ASSET_FETCHED:
        return STATUS_HOLD;
    case ASSET_FETCH_FAILED:
        return STATUS_FAILURE;
    case ASSET_LOADED:
        return STATUS_SUCCESS;
    default:
        // we don't know what state the asset is in yet, so keep going
        // check keyframe cache first then file cache then asset request
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

    bool success = false;
    LLFileSystem* anim_file = new LLFileSystem(mID, LLAssetType::AT_ANIMATION);
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
        anim_data = new(std::nothrow) U8[anim_file_size];
        if (anim_data)
        {
            success = anim_file->read(anim_data, anim_file_size);   /*Flawfinder: ignore*/
        }
        else
        {
            LL_WARNS() << "Failed to allocate buffer: " << anim_file_size << mID << LL_ENDL;
        }
        delete anim_file;
        anim_file = NULL;
    }

    if (!success)
    {
        LL_WARNS() << "Can't open animation file " << mID << LL_ENDL;
        mAssetStatus = ASSET_FETCH_FAILED;
        return STATUS_FAILURE;
    }

    LL_DEBUGS() << "Loading keyframe data for: " << getName() << ":" << getID() << " (" << anim_file_size << " bytes)" << LL_ENDL;

    LLDataPackerBinaryBuffer dp(anim_data, anim_file_size);

    if (!deserialize(dp, getID()))
    {
        LL_WARNS() << "Failed to decode asset for animation " << getName() << ":" << getID() << LL_ENDL;
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
bool LLKeyframeMotion::setupPose()
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
    for (JointConstraintSharedData* shared_constraintp : mJointMotionList->mConstraints)
    {
        JointConstraint* constraintp = new JointConstraint(shared_constraintp);
        initializeConstraint(constraintp);
        mConstraints.push_front(constraintp);
    }

    if (mJointMotionList->mConstraints.size())
    {
        mPelvisp = mCharacter->getJoint("mPelvis");
        if (!mPelvisp)
        {
            return false;
        }
    }

    // setup loop keys
    setLoopIn(mJointMotionList->mLoopInPoint);
    setLoopOut(mJointMotionList->mLoopOutPoint);

    return true;
}

//-----------------------------------------------------------------------------
// LLKeyframeMotion::onActivate()
//-----------------------------------------------------------------------------
bool LLKeyframeMotion::onActivate()
{
    // If the keyframe anim has an associated emote, trigger it.
    if (mJointMotionList->mEmoteID.notNull())
    {
        // don't start emote if already active to avoid recursion
        if (!mCharacter->isMotionActive(mJointMotionList->mEmoteID))
        {
            mCharacter->startMotion(mJointMotionList->mEmoteID);
        }
    }

    mLastLoopedTime = 0.f;

    return true;
}

//-----------------------------------------------------------------------------
// LLKeyframeMotion::onUpdate()
//-----------------------------------------------------------------------------
bool LLKeyframeMotion::onUpdate(F32 time, U8* joint_mask)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    // llassert(time >= 0.f);       // This will fire
    time = llmax(0.f, time);

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
        for (JointConstraint* constraintp : mConstraints)
        {
            initializeConstraint(constraintp);
        }
    }

    // apply constraints
    for (JointConstraint* constraintp : mConstraints)
    {
        applyConstraint(constraintp, time, joint_mask);
    }
}

//-----------------------------------------------------------------------------
// LLKeyframeMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::onDeactivate()
{
    for (JointConstraint* constraintp : mConstraints)
    {
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
    if ( !cur_joint )
    {
        return;
    }

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
    constraint->mActive = true;
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
        if ( !cur_joint )
        {
            return;
        }
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
        constraintp->mSourceVolume->mUpdateXform = false;
    }

    if (constraintp->mSharedData->mConstraintTargetType != CONSTRAINT_TARGET_TYPE_GROUND)
    {
        if (constraintp->mTargetVolume)
        {
            constraintp->mTargetVolume->mUpdateXform = false;
        }
    }
    constraintp->mActive = false;
}

//-----------------------------------------------------------------------------
// applyConstraint()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::applyConstraint(JointConstraint* constraint, F32 time, U8* joint_mask)
{
    JointConstraintSharedData *shared_data = constraint->mSharedData;
    if (!shared_data) return;

    LLVector3       positions[MAX_CHAIN_LENGTH];
    const F32*      joint_lengths = constraint->mJointLengths;
    LLVector3       velocities[MAX_CHAIN_LENGTH - 1];
    LLQuaternion    old_rots[MAX_CHAIN_LENGTH];
    S32             joint_num;

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
    if (! root_joint)
    {
        return;
    }

    LLVector3 root_pos = root_joint->getWorldPosition();
//  LLQuaternion root_rot =
    root_joint->getParent()->getWorldRotation();
//  LLQuaternion inv_root_rot = ~root_rot;

//  LLVector3 current_source_pos = mCharacter->getVolumePos(shared_data->mSourceConstraintVolume, shared_data->mSourceConstraintOffset);

    //apply underlying keyframe animation to get nominal "kinematic" joint positions
    for (joint_num = 0; joint_num <= shared_data->mChainLength; joint_num++)
    {
        LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[joint_num]);
        if (!cur_joint)
        {
            return;
        }

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
//      LL_INFOS() << "Target Pos " << constraint->mGroundPos << " on " << mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume)->getName() << LL_ENDL;
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
        constraint->mWeight = LLSmoothInterpolation::lerp(constraint->mWeight, 0.f, 0.1f);
    }
    else
    {
        constraint->mWeight = LLSmoothInterpolation::lerp(constraint->mWeight, 1.f, 0.3f);
    }

    F32 weight = constraint->mWeight * ((shared_data->mEaseOutStopTime == 0.f) ? 1.f :
        llmin(clamp_rescale(time, shared_data->mEaseInStartTime, shared_data->mEaseInStopTime, 0.f, 1.f),
        clamp_rescale(time, shared_data->mEaseOutStartTime, shared_data->mEaseOutStopTime, 1.f, 0.f)));

    LLVector3 source_to_target = target_pos - keyframe_source_pos;

    S32 max_iteration_count = ll_round(clamp_rescale(
                                          mCharacter->getPixelArea(),
                                          MAX_PIXEL_AREA_CONSTRAINTS,
                                          MIN_PIXEL_AREA_CONSTRAINTS,
                                          (F32)MAX_ITERATIONS,
                                          (F32)MIN_ITERATIONS));

    if (shared_data->mChainLength)
    {
        LLJoint* end_joint = getJoint(shared_data->mJointStateIndices[0]);

        if (!end_joint)
        {
            return;
        }

        LLQuaternion end_rot = end_joint->getWorldRotation();

        // slam start and end of chain to the proper positions (rest of chain stays put)
        positions[0] = lerp(keyframe_source_pos, target_pos, weight);
        positions[shared_data->mChainLength] = root_pos;

        // grab keyframe-specified positions of joints
        for (joint_num = 1; joint_num < shared_data->mChainLength; joint_num++)
        {
            LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[joint_num]);

            if (!cur_joint)
            {
                return;
            }

            LLVector3 kinematic_position = cur_joint->getWorldPosition() +
                (source_to_target * constraint->mJointLengthFractions[joint_num]);

            // convert intermediate joint positions to world coordinates
            positions[joint_num] = ( constraint->mPositions[joint_num] * mPelvisp->getWorldRotation()) + mPelvisp->getWorldPosition();
            F32 time_constant = 1.f / clamp_rescale(constraint->mFixupDistanceRMS, 0.f, 0.5f, 0.2f, 8.f);
//          LL_INFOS() << "Interpolant " << LLSmoothInterpolation::getInterpolant(time_constant, false) << " and fixup distance " << constraint->mFixupDistanceRMS << " on " << mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume)->getName() << LL_ENDL;
            positions[joint_num] = lerp(positions[joint_num], kinematic_position,
                LLSmoothInterpolation::getInterpolant(time_constant, false));
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
//              LL_INFOS() << iteration_count << " iterations on " <<
//                  mCharacter->findCollisionVolume(shared_data->mSourceConstraintVolume)->getName() << LL_ENDL;
                break;
            }
        }

        for (joint_num = shared_data->mChainLength; joint_num > 0; joint_num--)
        {
            LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[joint_num]);

            if (!cur_joint)
            {
                return;
            }
            LLJoint* child_joint = getJoint(shared_data->mJointStateIndices[joint_num - 1]);
            if (!child_joint)
            {
                return;
            }

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
            LLJoint* cur_joint = getJoint(shared_data->mJointStateIndices[joint_num]);
            if (!cur_joint)
            {
                return;
            }

            cur_joint->setRotation(old_rots[joint_num]);
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
//
// allow_invalid_joints should be true when handling existing content, to avoid breakage.
// During upload, we should be more restrictive and reject such animations.
//-----------------------------------------------------------------------------
bool LLKeyframeMotion::deserialize(LLDataPacker& dp, const LLUUID& asset_id, bool allow_invalid_joints)
{
    bool old_version = false;
    std::unique_ptr<LLKeyframeMotion::JointMotionList> joint_motion_list(new LLKeyframeMotion::JointMotionList);

    //-------------------------------------------------------------------------
    // get base priority
    //-------------------------------------------------------------------------
    S32 temp_priority;
    U16 version;
    U16 sub_version;

    // Amimation identifier for log messages
    auto asset = [&]() -> std::string
        {
            return asset_id.asString() + ", char " + mCharacter->getID().asString();
        };

    if (!dp.unpackU16(version, "version"))
    {
        LL_WARNS() << "can't read version number"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (!dp.unpackU16(sub_version, "sub_version"))
    {
        LL_WARNS() << "can't read sub version number"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (version == 0 && sub_version == 1)
    {
        old_version = true;
    }
    else if (version != KEYFRAME_MOTION_VERSION || sub_version != KEYFRAME_MOTION_SUBVERSION)
    {
#if LL_RELEASE
        LL_WARNS() << "Bad animation version " << version << "." << sub_version
                   << " for animation " << asset() << LL_ENDL;
        return false;
#else
        LL_ERRS() << "Bad animation version " << version << "." << sub_version
                  << " for animation " << asset() << LL_ENDL;
#endif
    }

    if (!dp.unpackS32(temp_priority, "base_priority"))
    {
        LL_WARNS() << "can't read animation base_priority"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }
    joint_motion_list->mBasePriority = (LLJoint::JointPriority) temp_priority;

    if (joint_motion_list->mBasePriority >= LLJoint::ADDITIVE_PRIORITY)
    {
        joint_motion_list->mBasePriority = (LLJoint::JointPriority)((S32)LLJoint::ADDITIVE_PRIORITY-1);
        joint_motion_list->mMaxPriority = joint_motion_list->mBasePriority;
    }
    else if (joint_motion_list->mBasePriority < LLJoint::USE_MOTION_PRIORITY)
    {
        LL_WARNS() << "bad animation base_priority " << joint_motion_list->mBasePriority
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    //-------------------------------------------------------------------------
    // get duration
    //-------------------------------------------------------------------------
    if (!dp.unpackF32(joint_motion_list->mDuration, "duration"))
    {
        LL_WARNS() << "can't read duration"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (joint_motion_list->mDuration > MAX_ANIM_DURATION ||
        !llfinite(joint_motion_list->mDuration))
    {
        LL_WARNS() << "invalid animation duration"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    //-------------------------------------------------------------------------
    // get emote (optional)
    //-------------------------------------------------------------------------
    if (!dp.unpackString(joint_motion_list->mEmoteName, "emote_name"))
    {
        LL_WARNS() << "can't read emote_name"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (!joint_motion_list->mEmoteName.empty())
    {
        if (joint_motion_list->mEmoteName == mID.asString())
        {
            LL_WARNS() << "Malformed animation mEmoteName==mID"
                       << " for animation " << asset() << LL_ENDL;
            return false;
        }
        // "Closed_Mouth" is a very popular emote name we should ignore
        if (joint_motion_list->mEmoteName == "Closed_Mouth")
        {
            joint_motion_list->mEmoteName.clear();
        }
        else
        {
            joint_motion_list->mEmoteID = gAnimLibrary.stringToAnimState(joint_motion_list->mEmoteName);
            if (joint_motion_list->mEmoteID.isNull())
            {
                LL_WARNS() << "unknown emote_name '" << joint_motion_list->mEmoteName << "'"
                           << " for animation " << asset() << LL_ENDL;
                joint_motion_list->mEmoteName.clear();
            }
        }
    }

    //-------------------------------------------------------------------------
    // get loop
    //-------------------------------------------------------------------------
    if (!dp.unpackF32(joint_motion_list->mLoopInPoint, "loop_in_point") ||
        !llfinite(joint_motion_list->mLoopInPoint))
    {
        LL_WARNS() << "can't read loop point"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (!dp.unpackF32(joint_motion_list->mLoopOutPoint, "loop_out_point") ||
        !llfinite(joint_motion_list->mLoopOutPoint))
    {
        LL_WARNS() << "can't read loop point"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    S32 loop{ 0 };
    if (!dp.unpackS32(loop, "loop"))
    {
        LL_WARNS() << "can't read loop"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }
    joint_motion_list->mLoop = static_cast<bool>(loop);

    //SL-17206 hack to alter Female_land loop setting, while current behavior won't be changed serverside
    LLUUID const female_land_anim("ca1baf4d-0a18-5a1f-0330-e4bd1e71f09e");
    LLUUID const formal_female_land_anim("6a9a173b-61fa-3ad5-01fa-a851cfc5f66a");
    if (female_land_anim == asset_id || formal_female_land_anim == asset_id)
    {
        LL_WARNS() << "Animation " << asset() << " won't be looped." << LL_ENDL;
        joint_motion_list->mLoop = false;
    }

    //-------------------------------------------------------------------------
    // get easeIn and easeOut
    //-------------------------------------------------------------------------
    if (!dp.unpackF32(joint_motion_list->mEaseInDuration, "ease_in_duration") ||
        !llfinite(joint_motion_list->mEaseInDuration))
    {
        LL_WARNS() << "can't read easeIn"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (!dp.unpackF32(joint_motion_list->mEaseOutDuration, "ease_out_duration") ||
        !llfinite(joint_motion_list->mEaseOutDuration))
    {
        LL_WARNS() << "can't read easeOut"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    //-------------------------------------------------------------------------
    // get hand pose
    //-------------------------------------------------------------------------
    U32 word;
    if (!dp.unpackU32(word, "hand_pose"))
    {
        LL_WARNS() << "can't read hand pose"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (word > LLHandMotion::NUM_HAND_POSES)
    {
        LL_WARNS() << "invalid LLHandMotion::eHandPose index: " << word
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    joint_motion_list->mHandPose = (LLHandMotion::eHandPose)word;

    //-------------------------------------------------------------------------
    // get number of joint motions
    //-------------------------------------------------------------------------
    U32 num_motions = 0;
    S32 rotation_duplicates = 0;
    S32 position_duplicates = 0;
    if (!dp.unpackU32(num_motions, "num_joints"))
    {
        LL_WARNS() << "can't read number of joints"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (num_motions == 0)
    {
        LL_WARNS() << "no joints"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }
    else if (num_motions > LL_CHARACTER_MAX_ANIMATED_JOINTS)
    {
        LL_WARNS() << "too many joints"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    joint_motion_list->mJointMotionArray.clear();
    joint_motion_list->mJointMotionArray.reserve(num_motions);
    mJointStates.clear();
    mJointStates.reserve(num_motions);

    //-------------------------------------------------------------------------
    // initialize joint motions
    //-------------------------------------------------------------------------

    for (U32 i = 0; i < num_motions; ++i)
    {
        JointMotion* joint_motion = new JointMotion;
        joint_motion_list->mJointMotionArray.push_back(joint_motion);

        std::string joint_name;
        if (!dp.unpackString(joint_name, "joint_name"))
        {
            LL_WARNS() << "can't read joint name"
                       << " for animation " << asset() << LL_ENDL;
            return false;
        }

        if (joint_name == "mScreen" || joint_name == "mRoot")
        {
            LL_WARNS() << "attempted to animate special " << joint_name << " joint"
                       << " for animation " << asset() << LL_ENDL;
            return false;
        }

        //---------------------------------------------------------------------
        // find the corresponding joint
        //---------------------------------------------------------------------
        LLJoint *joint = mCharacter->getJoint( joint_name );
        if (joint)
        {
            S32 joint_num = joint->getJointNum();
            joint_name = joint->getName(); // canonical name in case this is an alias.
//          LL_INFOS() << "  joint: " << joint_name << LL_ENDL;
            if ((joint_num >= (S32)LL_CHARACTER_MAX_ANIMATED_JOINTS) || (joint_num < 0))
            {
                LL_WARNS() << "Joint will be omitted from animation: joint_num " << joint_num
                           << " is outside of legal range [0-"
                           << LL_CHARACTER_MAX_ANIMATED_JOINTS << ") for joint " << joint->getName()
                           << " for animation " << asset() << LL_ENDL;
                joint = NULL;
            }
        }
        else
        {
            LL_WARNS() << "invalid joint name: " << joint_name
                       << " for animation " << asset() << LL_ENDL;
            if (!allow_invalid_joints)
            {
                return false;
            }
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
            LL_WARNS() << "can't read joint priority."
                       << " for animation " << asset() << LL_ENDL;
            return false;
        }

        if (joint_priority < LLJoint::USE_MOTION_PRIORITY)
        {
            LL_WARNS() << "joint priority unknown - too low."
                       << " for animation " << asset() << LL_ENDL;
            return false;
        }

        joint_motion->mPriority = (LLJoint::JointPriority)joint_priority;
        if (joint_priority != LLJoint::USE_MOTION_PRIORITY &&
            joint_priority > joint_motion_list->mMaxPriority)
        {
            joint_motion_list->mMaxPriority = (LLJoint::JointPriority)joint_priority;
        }

        joint_state->setPriority((LLJoint::JointPriority)joint_priority);

        //---------------------------------------------------------------------
        // scan rotation curve header
        //---------------------------------------------------------------------
        if (!dp.unpackS32(joint_motion->mRotationCurve.mNumKeys, "num_rot_keys") || joint_motion->mRotationCurve.mNumKeys < 0)
        {
            LL_WARNS() << "can't read number of rotation keys"
                       << " for animation " << asset() << LL_ENDL;
            return false;
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
                    LL_WARNS() << "can't read rotation key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }

            }
            else
            {
                if (!dp.unpackU16(time_short, "time"))
                {
                    LL_WARNS() << "can't read rotation key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }

                time = U16_to_F32(time_short, 0.f, joint_motion_list->mDuration);

                if (time < 0 || time > joint_motion_list->mDuration)
                {
                    LL_WARNS() << "invalid frame time"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }
            }

            RotationKey rot_key;
            rot_key.mTime = time;
            LLVector3 rot_angles;
            U16 x, y, z;

            if (old_version)
            {
                if (!dp.unpackVector3(rot_angles, "rot_angles"))
                {
                    LL_WARNS() << "can't read rot_angles in rotation key (" << k << ")"
                        << " for animation " << asset() << LL_ENDL;
                    return false;
                }
                if (!rot_angles.isFinite())
                {
                    LL_WARNS() << "non-finite angle in rotation key (" << k << ")"
                        << " for animation " << asset() << LL_ENDL;
                    return false;
                }

                LLQuaternion::Order ro = StringToOrder("ZYX");
                rot_key.mRotation = mayaQ(rot_angles.mV[VX], rot_angles.mV[VY], rot_angles.mV[VZ], ro);
            }
            else
            {
                if (!dp.unpackU16(x, "rot_angle_x"))
                {
                    LL_WARNS() << "can't read rot_angle_x in rotation key (" << k << ")"
                        << " for animation " << asset() << LL_ENDL;
                    return false;
                }
                if (!dp.unpackU16(y, "rot_angle_y"))
                {
                    LL_WARNS() << "can't read rot_angle_y in rotation key (" << k << ")"
                        << " for animation " << asset() << LL_ENDL;
                    return false;
                }
                if (!dp.unpackU16(z, "rot_angle_z"))
                {
                    LL_WARNS() << "can't read rot_angle_z in rotation key (" << k << ")"
                        << " for animation " << asset() << LL_ENDL;
                    return false;
                }

                LLVector3 rot_vec;
                rot_vec.mV[VX] = U16_to_F32(x, -1.f, 1.f);
                rot_vec.mV[VY] = U16_to_F32(y, -1.f, 1.f);
                rot_vec.mV[VZ] = U16_to_F32(z, -1.f, 1.f);

                if (!rot_vec.isFinite())
                {
                    LL_WARNS() << "non-finite angle in rotation key (" << k << ")"
                        << " for animation " << asset() << LL_ENDL;
                    return false;
                }
                rot_key.mRotation.unpackFromVector3(rot_vec);
            }

            if (!rot_key.mRotation.isFinite())
            {
                LL_WARNS() << "non-finite angle in rotation key (" << k << ")"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            rCurve->mKeys[time] = rot_key;
        }

        if (joint_motion->mRotationCurve.mNumKeys > joint_motion->mRotationCurve.mKeys.size())
        {
            rotation_duplicates++;
            LL_INFOS() << "Motion " << asset() << " had duplicated rotation keys that were removed: "
                << joint_motion->mRotationCurve.mNumKeys << " > " << joint_motion->mRotationCurve.mKeys.size()
                << " (" << rotation_duplicates << ")" << LL_ENDL;
        }

        //---------------------------------------------------------------------
        // scan position curve header
        //---------------------------------------------------------------------
        if (!dp.unpackS32(joint_motion->mPositionCurve.mNumKeys, "num_pos_keys") || joint_motion->mPositionCurve.mNumKeys < 0)
        {
            LL_WARNS() << "can't read number of position keys"
                       << " for animation " << asset() << LL_ENDL;
            return false;
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
        bool is_pelvis = joint_motion->mJointName == "mPelvis";
        for (S32 k = 0; k < joint_motion->mPositionCurve.mNumKeys; k++)
        {
            U16 time_short;
            PositionKey pos_key;

            if (old_version)
            {
                if (!dp.unpackF32(pos_key.mTime, "time") ||
                    !llfinite(pos_key.mTime))
                {
                    LL_WARNS() << "can't read position key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }
            }
            else
            {
                if (!dp.unpackU16(time_short, "time"))
                {
                    LL_WARNS() << "can't read position key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }

                pos_key.mTime = U16_to_F32(time_short, 0.f, joint_motion_list->mDuration);
            }

            if (old_version)
            {
                if (!dp.unpackVector3(pos_key.mPosition, "pos"))
                {
                    LL_WARNS() << "can't read pos in position key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }

                //MAINT-6162
                pos_key.mPosition.mV[VX] = llclamp( pos_key.mPosition.mV[VX], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
                pos_key.mPosition.mV[VY] = llclamp( pos_key.mPosition.mV[VY], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
                pos_key.mPosition.mV[VZ] = llclamp( pos_key.mPosition.mV[VZ], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);

            }
            else
            {
                U16 x, y, z;

                if (!dp.unpackU16(x, "pos_x"))
                {
                    LL_WARNS() << "can't read pos_x in position key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }
                if (!dp.unpackU16(y, "pos_y"))
                {
                    LL_WARNS() << "can't read pos_y in position key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }
                if (!dp.unpackU16(z, "pos_z"))
                {
                    LL_WARNS() << "can't read pos_z in position key (" << k << ")"
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }

                pos_key.mPosition.mV[VX] = U16_to_F32(x, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
                pos_key.mPosition.mV[VY] = U16_to_F32(y, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
                pos_key.mPosition.mV[VZ] = U16_to_F32(z, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
            }

            if (!pos_key.mPosition.isFinite())
            {
                LL_WARNS() << "non-finite position in key"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            pCurve->mKeys[pos_key.mTime] = pos_key;

            if (is_pelvis)
            {
                joint_motion_list->mPelvisBBox.addPoint(pos_key.mPosition);
            }
        }

        if (joint_motion->mPositionCurve.mNumKeys > joint_motion->mPositionCurve.mKeys.size())
        {
            position_duplicates++;
            LL_INFOS() << "Motion " << asset() << " had duplicated position keys that were removed: "
                << joint_motion->mPositionCurve.mNumKeys << " > " << joint_motion->mPositionCurve.mKeys.size()
                << " (" << position_duplicates << ")" << LL_ENDL;
        }

        joint_motion->mUsage = joint_state->getUsage();
    }

    if (rotation_duplicates > 0)
    {
        LL_INFOS() << "Motion " << asset() << " had " << rotation_duplicates
            << " duplicated rotation keys that were removed" << LL_ENDL;
    }

    if (position_duplicates > 0)
    {
        LL_INFOS() << "Motion " << asset() << " had " << position_duplicates
            << " duplicated position keys that were removed" << LL_ENDL;
    }

    //-------------------------------------------------------------------------
    // get number of constraints
    //-------------------------------------------------------------------------
    S32 num_constraints = 0;
    if (!dp.unpackS32(num_constraints, "num_constraints"))
    {
        LL_WARNS() << "can't read number of constraints"
                   << " for animation " << asset() << LL_ENDL;
        return false;
    }

    if (num_constraints > MAX_CONSTRAINTS || num_constraints < 0)
    {
        LL_WARNS() << "Bad number of constraints... ignoring: " << num_constraints
                   << " for animation " << asset() << LL_ENDL;
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
            std::unique_ptr<JointConstraintSharedData> constraintp(new JointConstraintSharedData);
            U8 byte = 0;

            if (!dp.unpackU8(byte, "chain_length"))
            {
                LL_WARNS() << "can't read constraint chain length"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }
            constraintp->mChainLength = (S32) byte;

            if((U32)constraintp->mChainLength > joint_motion_list->getNumJointMotions())
            {
                LL_WARNS() << "invalid constraint chain length"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!dp.unpackU8(byte, "constraint_type"))
            {
                LL_WARNS() << "can't read constraint type"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if( byte >= NUM_CONSTRAINT_TYPES )
            {
                LL_WARNS() << "invalid constraint type"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }
            constraintp->mConstraintType = (EConstraintType)byte;

            const S32 BIN_DATA_LENGTH = 16;
            U8 bin_data[BIN_DATA_LENGTH+1];
            if (!dp.unpackBinaryDataFixed(bin_data, BIN_DATA_LENGTH, "source_volume"))
            {
                LL_WARNS() << "can't read source volume name"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            bin_data[BIN_DATA_LENGTH] = 0; // Ensure null termination
            str = (char*)bin_data;
            constraintp->mSourceConstraintVolume = mCharacter->getCollisionVolumeID(str);
            if (constraintp->mSourceConstraintVolume == -1)
            {
                LL_WARNS() << "not a valid source constraint volume " << str
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!dp.unpackVector3(constraintp->mSourceConstraintOffset, "source_offset"))
            {
                LL_WARNS() << "can't read constraint source offset"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if( !(constraintp->mSourceConstraintOffset.isFinite()) )
            {
                LL_WARNS() << "non-finite constraint source offset"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!dp.unpackBinaryDataFixed(bin_data, BIN_DATA_LENGTH, "target_volume"))
            {
                LL_WARNS() << "can't read target volume name"
                           << " for animation " << asset() << LL_ENDL;
                return false;
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
                if (constraintp->mTargetConstraintVolume == -1)
                {
                    LL_WARNS() << "not a valid target constraint volume " << str
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }
            }

            if (!dp.unpackVector3(constraintp->mTargetConstraintOffset, "target_offset"))
            {
                LL_WARNS() << "can't read constraint target offset"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if( !(constraintp->mTargetConstraintOffset.isFinite()) )
            {
                LL_WARNS() << "non-finite constraint target offset"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!dp.unpackVector3(constraintp->mTargetConstraintDir, "target_dir"))
            {
                LL_WARNS() << "can't read constraint target direction"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if( !(constraintp->mTargetConstraintDir.isFinite()) )
            {
                LL_WARNS() << "non-finite constraint target direction"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!constraintp->mTargetConstraintDir.isExactlyZero())
            {
                constraintp->mUseTargetOffset = true;
    //          constraintp->mTargetConstraintDir *= constraintp->mSourceConstraintOffset.magVec();
            }

            if (!dp.unpackF32(constraintp->mEaseInStartTime, "ease_in_start") || !llfinite(constraintp->mEaseInStartTime))
            {
                LL_WARNS() << "can't read constraint ease in start time"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!dp.unpackF32(constraintp->mEaseInStopTime, "ease_in_stop") || !llfinite(constraintp->mEaseInStopTime))
            {
                LL_WARNS() << "can't read constraint ease in stop time"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!dp.unpackF32(constraintp->mEaseOutStartTime, "ease_out_start") || !llfinite(constraintp->mEaseOutStartTime))
            {
                LL_WARNS() << "can't read constraint ease out start time"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            if (!dp.unpackF32(constraintp->mEaseOutStopTime, "ease_out_stop") || !llfinite(constraintp->mEaseOutStopTime))
            {
                LL_WARNS() << "can't read constraint ease out stop time"
                           << " for animation " << asset() << LL_ENDL;
                return false;
            }

            LLJoint* joint = mCharacter->findCollisionVolume(constraintp->mSourceConstraintVolume);
            // get joint to which this collision volume is attached
            if (!joint)
            {
                return false;
            }

            constraintp->mJointStateIndices = new S32[constraintp->mChainLength + 1]; // note: mChainLength is size-limited - comes from a byte

            for (S32 i = 0; i < constraintp->mChainLength + 1; i++)
            {
                LLJoint* parent = joint->getParent();
                if (!parent)
                {
                    LL_WARNS() << "Joint with no parent: " << joint->getName()
                               << " Emote: " << joint_motion_list->mEmoteName
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }
                joint = parent;
                constraintp->mJointStateIndices[i] = -1;
                for (U32 j = 0; j < joint_motion_list->getNumJointMotions(); j++)
                {
                    LLJoint* constraint_joint = getJoint(j);

                    if ( !constraint_joint )
                    {
                        LL_WARNS() << "Invalid joint " << j
                                   << " for animation " << asset() << LL_ENDL;
                        return false;
                    }

                    if(constraint_joint == joint)
                    {
                        constraintp->mJointStateIndices[i] = (S32)j;
                        break;
                    }
                }
                if (constraintp->mJointStateIndices[i] < 0 )
                {
                    LL_WARNS() << "No joint index for constraint " << i
                               << " for animation " << asset() << LL_ENDL;
                    return false;
                }
            }

            joint_motion_list->mConstraints.push_front(constraintp.release());
        }
    }

    // *FIX: support cleanup of old keyframe data
    mJointMotionList = joint_motion_list.release(); // release from unique_ptr to member;
    LLKeyframeDataCache::addKeyframeData(getID(),  mJointMotionList);
    mAssetStatus = ASSET_LOADED;

    setupPose();

    return true;
}

//-----------------------------------------------------------------------------
// serialize()
//-----------------------------------------------------------------------------
bool LLKeyframeMotion::serialize(LLDataPacker& dp) const
{
    bool success = true;

    LL_DEBUGS("BVH") << "serializing" << LL_ENDL;

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

    LL_DEBUGS("BVH") << "version " << KEYFRAME_MOTION_VERSION << LL_ENDL;
    LL_DEBUGS("BVH") << "sub_version " << KEYFRAME_MOTION_SUBVERSION << LL_ENDL;
    LL_DEBUGS("BVH") << "base_priority " << mJointMotionList->mBasePriority << LL_ENDL;
    LL_DEBUGS("BVH") << "duration " << mJointMotionList->mDuration << LL_ENDL;
    LL_DEBUGS("BVH") << "emote_name " << mJointMotionList->mEmoteName << LL_ENDL;
    LL_DEBUGS("BVH") << "loop_in_point " << mJointMotionList->mLoopInPoint << LL_ENDL;
    LL_DEBUGS("BVH") << "loop_out_point " << mJointMotionList->mLoopOutPoint << LL_ENDL;
    LL_DEBUGS("BVH") << "loop " << mJointMotionList->mLoop << LL_ENDL;
    LL_DEBUGS("BVH") << "ease_in_duration " << mJointMotionList->mEaseInDuration << LL_ENDL;
    LL_DEBUGS("BVH") << "ease_out_duration " << mJointMotionList->mEaseOutDuration << LL_ENDL;
    LL_DEBUGS("BVH") << "hand_pose " << mJointMotionList->mHandPose << LL_ENDL;
    LL_DEBUGS("BVH") << "num_joints " << mJointMotionList->getNumJointMotions() << LL_ENDL;

    for (U32 i = 0; i < mJointMotionList->getNumJointMotions(); i++)
    {
        JointMotion* joint_motionp = mJointMotionList->getJointMotion(i);
        success &= dp.packString(joint_motionp->mJointName, "joint_name");
        success &= dp.packS32(joint_motionp->mPriority, "joint_priority");
        success &= dp.packS32(static_cast<S32>(joint_motionp->mRotationCurve.mKeys.size()), "num_rot_keys");

        LL_DEBUGS("BVH") << "Joint " << i
            << " name: " << joint_motionp->mJointName
            << " Rotation keys: " << joint_motionp->mRotationCurve.mKeys.size()
            << " Position keys: " << joint_motionp->mPositionCurve.mKeys.size() << LL_ENDL;
        for (RotationCurve::key_map_t::value_type& rot_pair : joint_motionp->mRotationCurve.mKeys)
        {
            RotationKey& rot_key = rot_pair.second;
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

            LL_DEBUGS("BVH") << "  rot: t " << rot_key.mTime << " angles " << rot_angles.mV[VX] <<","<< rot_angles.mV[VY] <<","<< rot_angles.mV[VZ] << LL_ENDL;
        }

        success &= dp.packS32(static_cast<S32>(joint_motionp->mPositionCurve.mKeys.size()), "num_pos_keys");
        for (PositionCurve::key_map_t::value_type& pos_pair : joint_motionp->mPositionCurve.mKeys)
        {
            PositionKey& pos_key = pos_pair.second;
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

            LL_DEBUGS("BVH") << "  pos: t " << pos_key.mTime << " pos " << pos_key.mPosition.mV[VX] <<","<< pos_key.mPosition.mV[VY] <<","<< pos_key.mPosition.mV[VZ] << LL_ENDL;
        }
    }

    success &= dp.packS32(static_cast<S32>(mJointMotionList->mConstraints.size()), "num_constraints");
    LL_DEBUGS("BVH") << "num_constraints " << mJointMotionList->mConstraints.size() << LL_ENDL;
    for (JointConstraintSharedData* shared_constraintp : mJointMotionList->mConstraints)
    {
        success &= dp.packU8(shared_constraintp->mChainLength, "chain_length");
        success &= dp.packU8(shared_constraintp->mConstraintType, "constraint_type");
        char source_volume[16]; /* Flawfinder: ignore */
        snprintf(source_volume, sizeof(source_volume), "%s",    /* Flawfinder: ignore */
                 mCharacter->findCollisionVolume(shared_constraintp->mSourceConstraintVolume)->getName().c_str());

        success &= dp.packBinaryDataFixed((U8*)source_volume, 16, "source_volume");
        success &= dp.packVector3(shared_constraintp->mSourceConstraintOffset, "source_offset");
        char target_volume[16]; /* Flawfinder: ignore */
        if (shared_constraintp->mConstraintTargetType == CONSTRAINT_TARGET_TYPE_GROUND)
        {
            snprintf(target_volume,sizeof(target_volume), "%s", "GROUND");  /* Flawfinder: ignore */
        }
        else
        {
            snprintf(target_volume, sizeof(target_volume),"%s", /* Flawfinder: ignore */
                     mCharacter->findCollisionVolume(shared_constraintp->mTargetConstraintVolume)->getName().c_str());
        }
        success &= dp.packBinaryDataFixed((U8*)target_volume, 16, "target_volume");
        success &= dp.packVector3(shared_constraintp->mTargetConstraintOffset, "target_offset");
        success &= dp.packVector3(shared_constraintp->mTargetConstraintDir, "target_dir");
        success &= dp.packF32(shared_constraintp->mEaseInStartTime, "ease_in_start");
        success &= dp.packF32(shared_constraintp->mEaseInStopTime, "ease_in_stop");
        success &= dp.packF32(shared_constraintp->mEaseOutStartTime, "ease_out_start");
        success &= dp.packF32(shared_constraintp->mEaseOutStopTime, "ease_out_stop");

        LL_DEBUGS("BVH") << "  chain_length " << shared_constraintp->mChainLength << LL_ENDL;
        LL_DEBUGS("BVH") << "  constraint_type " << (S32)shared_constraintp->mConstraintType << LL_ENDL;
        LL_DEBUGS("BVH") << "  source_volume " << source_volume << LL_ENDL;
        LL_DEBUGS("BVH") << "  source_offset " << shared_constraintp->mSourceConstraintOffset << LL_ENDL;
        LL_DEBUGS("BVH") << "  target_volume " << target_volume << LL_ENDL;
        LL_DEBUGS("BVH") << "  target_offset " << shared_constraintp->mTargetConstraintOffset << LL_ENDL;
        LL_DEBUGS("BVH") << "  target_dir " << shared_constraintp->mTargetConstraintDir << LL_ENDL;
        LL_DEBUGS("BVH") << "  ease_in_start " << shared_constraintp->mEaseInStartTime << LL_ENDL;
        LL_DEBUGS("BVH") << "  ease_in_stop " << shared_constraintp->mEaseInStopTime << LL_ENDL;
        LL_DEBUGS("BVH") << "  ease_out_start " << shared_constraintp->mEaseOutStartTime << LL_ENDL;
        LL_DEBUGS("BVH") << "  ease_out_stop " << shared_constraintp->mEaseOutStopTime << LL_ENDL;
    }

    return success;
}

//-----------------------------------------------------------------------------
// getFileSize()
//-----------------------------------------------------------------------------
U32 LLKeyframeMotion::getFileSize()
{
    // serialize into a dummy buffer to calculate required size
    LLDataPackerBinaryBuffer dp;
    serialize(dp);

    return dp.getCurrentSize();
}

//-----------------------------------------------------------------------------
// dumpToFile()
//-----------------------------------------------------------------------------
bool LLKeyframeMotion::dumpToFile(const std::string& name)
{
    bool succ = false;
    if (isLoaded())
    {
        std::string outfile_base;
        if (!name.empty())
        {
            outfile_base = name;
        }
        else if (!getName().empty())
        {
            outfile_base = getName();
        }
        else
        {
            const LLUUID& id = getID();
            outfile_base = id.asString();
        }

        if (gDirUtilp->getExtension(outfile_base).empty())
        {
            outfile_base += ".anim";
        }
        std::string outfilename;
        if (gDirUtilp->getDirName(outfile_base).empty())
        {
            outfilename = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfile_base);
        }
        else
        {
            outfilename = outfile_base;
        }
        if (LLFile::isfile(outfilename))
        {
            LL_WARNS() << outfilename << " already exists, write failed" << LL_ENDL;
            return false;
        }

        S32 file_size = getFileSize();
        U8* buffer = new(std::nothrow) U8[file_size];
        if (!buffer)
        {
            LLError::LLUserWarningMsg::showOutOfMemory();
            LL_ERRS() << "Bad memory allocation for buffer, file: " << name << " " << file_size << LL_ENDL;
        }

        LL_DEBUGS("BVH") << "Dumping " << outfilename << LL_ENDL;
        LLDataPackerBinaryBuffer dp(buffer, file_size);
        if (serialize(dp))
        {
            LLAPRFile outfile;
            outfile.open(outfilename, LL_APR_WPB);
            if (outfile.getFileHandle())
            {
                S32 wrote_bytes = outfile.write(buffer, file_size);
                succ = (wrote_bytes == file_size);
            }
        }
        delete [] buffer;
    }
    return succ;
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
        mJointMotionList->mEmoteID = emote_id;
    }
    else
    {
        mJointMotionList->mEmoteName.clear();
        mJointMotionList->mEmoteID.setNull();
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
//  LLKeyframeDataCache::clear();
}

//-----------------------------------------------------------------------------
// setLoop()
//-----------------------------------------------------------------------------
void LLKeyframeMotion::setLoop(bool loop)
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
void LLKeyframeMotion::onLoadComplete(const LLUUID& asset_uuid,
                                      LLAssetType::EType type,
                                      void* user_data, S32 status, LLExtStat ext_status)
{
    LLUUID* id = (LLUUID*)user_data;

    auto char_iter = std::find_if(LLCharacter::sInstances.begin(), LLCharacter::sInstances.end(), [&](LLCharacter* c)
        {
            return c->getID() == *id;
        });

    delete id;

    if (char_iter == LLCharacter::sInstances.end())
    {
        return;
    }

    LLCharacter* character = *char_iter;

    // look for an existing instance of this motion
    if (LLMotion* asset = character->findMotion(asset_uuid))
    {
        LLKeyframeMotion* motionp = dynamic_cast<LLKeyframeMotion*>(asset);
        if (!motionp)
        {
            // This motion is not LLKeyframeMotion (e.g., LLEmote)
            return;
        }

        if (0 == status)
        {
            if (motionp->mAssetStatus == ASSET_LOADED)
            {
                // asset already loaded
                return;
            }
            LLFileSystem file(asset_uuid, type, LLFileSystem::READ);
            S32 size = file.getSize();

            U8* buffer = new(std::nothrow) U8[size];
            if (!buffer)
            {
                LLError::LLUserWarningMsg::showOutOfMemory();
                LL_ERRS() << "Bad memory allocation for buffer of size: " << size << LL_ENDL;
            }
            file.read((U8*)buffer, size);   /*Flawfinder: ignore*/

            LL_DEBUGS("Animation") << "Loading keyframe data for: " << motionp->getName() << ":" << motionp->getID() << " (" << size << " bytes)" << LL_ENDL;

            LLDataPackerBinaryBuffer dp(buffer, size);
            if (motionp->deserialize(dp, asset_uuid))
            {
                motionp->mAssetStatus = ASSET_LOADED;
            }
            else
            {
                LL_WARNS() << "Failed to decode asset for animation " << motionp->getName() << ":" << motionp->getID() << LL_ENDL;
                motionp->mAssetStatus = ASSET_FETCH_FAILED;
            }

            delete[] buffer;
        }
        else
        {
            LL_WARNS() << "Failed to load asset for animation " << motionp->getName() << ":" << motionp->getID() << LL_ENDL;
            motionp->mAssetStatus = ASSET_FETCH_FAILED;
        }
    }
    else
    {
        LL_WARNS() << "No existing motion for asset data. UUID: " << asset_uuid << LL_ENDL;
    }
}

//--------------------------------------------------------------------
// LLKeyframeDataCache::dumpDiagInfo()
//--------------------------------------------------------------------
void LLKeyframeDataCache::dumpDiagInfo()
{
    // keep track of totals
    U32 total_size = 0;

    char buf[1024];     /* Flawfinder: ignore */

    LL_INFOS() << "-----------------------------------------------------" << LL_ENDL;
    LL_INFOS() << "       Global Motion Table (DEBUG only)" << LL_ENDL;
    LL_INFOS() << "-----------------------------------------------------" << LL_ENDL;

    // print each loaded mesh, and it's memory usage
    for (keyframe_data_map_t::value_type& data_pair : sKeyframeDataMap)
    {
        U32 joint_motion_kb;

        LLKeyframeMotion::JointMotionList *motion_list_p = data_pair.second;

        LL_INFOS() << "Motion: " << data_pair.first << LL_ENDL;

        joint_motion_kb = motion_list_p->dumpDiagInfo();

        total_size += joint_motion_kb;
    }

    LL_INFOS() << "-----------------------------------------------------" << LL_ENDL;
    LL_INFOS() << "Motions\tTotal Size" << LL_ENDL;
    snprintf(buf, sizeof(buf), "%d\t\t%d bytes", (S32)sKeyframeDataMap.size(), total_size );        /* Flawfinder: ignore */
    LL_INFOS() << buf << LL_ENDL;
    LL_INFOS() << "-----------------------------------------------------" << LL_ENDL;
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
    mActive = false;
    mSourceVolume = NULL;
    mTargetVolume = NULL;
    mFixupDistanceRMS = 0.f;

    for (S32 i=0; i<MAX_CHAIN_LENGTH; ++i)
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

