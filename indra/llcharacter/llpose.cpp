/** 
 * @file llpose.cpp
 * @brief Implementation of LLPose class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "llpose.h"

#include "llmotion.h"
#include "llmath.h"
#include "llstl.h"

//-----------------------------------------------------------------------------
// Static
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLPose
//-----------------------------------------------------------------------------
LLPose::~LLPose()
{
}

//-----------------------------------------------------------------------------
// getFirstJointState()
//-----------------------------------------------------------------------------
LLJointState* LLPose::getFirstJointState()
{
	mListIter = mJointMap.begin();
	if (mListIter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return mListIter->second;
	}
}

//-----------------------------------------------------------------------------
// getNextJointState()
//-----------------------------------------------------------------------------
LLJointState *LLPose::getNextJointState()
{
	mListIter++;
	if (mListIter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return mListIter->second;
	}
}

//-----------------------------------------------------------------------------
// addJointState()
//-----------------------------------------------------------------------------
BOOL LLPose::addJointState(const LLPointer<LLJointState>& jointState)
{
	if (mJointMap.find(jointState->getJoint()->getName()) == mJointMap.end())
	{
		mJointMap[jointState->getJoint()->getName()] = jointState;
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// removeJointState()
//-----------------------------------------------------------------------------
BOOL LLPose::removeJointState(const LLPointer<LLJointState>& jointState)
{
	mJointMap.erase(jointState->getJoint()->getName());
	return TRUE;
}

//-----------------------------------------------------------------------------
// removeAllJointStates()
//-----------------------------------------------------------------------------
BOOL LLPose::removeAllJointStates()
{
	mJointMap.clear();
	return TRUE;
}

//-----------------------------------------------------------------------------
// findJointState()
//-----------------------------------------------------------------------------
LLJointState* LLPose::findJointState(LLJoint *joint)
{
	joint_map_iterator iter = mJointMap.find(joint->getName());

	if (iter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return iter->second;
	}
}

//-----------------------------------------------------------------------------
// findJointState()
//-----------------------------------------------------------------------------
LLJointState* LLPose::findJointState(const std::string &name)
{
	joint_map_iterator iter = mJointMap.find(name);

	if (iter == mJointMap.end())
	{
		return NULL;
	}
	else
	{
		return iter->second;
	}
}

//-----------------------------------------------------------------------------
// setWeight()
//-----------------------------------------------------------------------------
void LLPose::setWeight(F32 weight)
{
	joint_map_iterator iter;
	for(iter = mJointMap.begin(); 
		iter != mJointMap.end();
		++iter)
	{
		iter->second->setWeight(weight);
	}
	mWeight = weight;
}

//-----------------------------------------------------------------------------
// getWeight()
//-----------------------------------------------------------------------------
F32 LLPose::getWeight() const
{
	return mWeight;
}

//-----------------------------------------------------------------------------
// getNumJointStates()
//-----------------------------------------------------------------------------
S32 LLPose::getNumJointStates() const
{
	return (S32)mJointMap.size();
}

//-----------------------------------------------------------------------------
// LLJointStateBlender
//-----------------------------------------------------------------------------

LLJointStateBlender::LLJointStateBlender()
{
	for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
	{
		mJointStates[i] = NULL;
		mPriorities[i] = S32_MIN;
		mAdditiveBlends[i] = FALSE;
	}
}

LLJointStateBlender::~LLJointStateBlender()
{
	
}

//-----------------------------------------------------------------------------
// addJointState()
//-----------------------------------------------------------------------------
BOOL LLJointStateBlender::addJointState(const LLPointer<LLJointState>& joint_state, S32 priority, BOOL additive_blend)
{
	llassert(joint_state);

	if (!joint_state->getJoint())
		// this joint state doesn't point to an actual joint, so we don't care about applying it
		return FALSE;

	for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
	{
		if (mJointStates[i].isNull())
		{
			mJointStates[i] = joint_state;
			mPriorities[i] = priority;
			mAdditiveBlends[i] = additive_blend;
			return TRUE;
		} 
		else if (priority > mPriorities[i])
		{
			// we're at a higher priority than the current joint state in this slot
			// so shift everyone over
			// previous joint states (newer motions) with same priority should stay in place
			for (S32 j = JSB_NUM_JOINT_STATES - 1; j > i; j--)
			{
				mJointStates[j] = mJointStates[j - 1];
				mPriorities[j] = mPriorities[j - 1];
				mAdditiveBlends[j] = mAdditiveBlends[j - 1];
			}
			// now store ourselves in this slot
			mJointStates[i] = joint_state;
			mPriorities[i] = priority;
			mAdditiveBlends[i] = additive_blend;
			return TRUE;
		}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// blendJointStates()
//-----------------------------------------------------------------------------
void LLJointStateBlender::blendJointStates(BOOL apply_now)
{
	// we need at least one joint to blend
	// if there is one, it will be in slot zero according to insertion logic
	// instead of resetting joint state to default, just leave it unchanged from last frame
	if (mJointStates[0].isNull())
	{
		return;
	}

	LLJoint* target_joint = apply_now ? mJointStates[0]->getJoint() : &mJointCache;

	const S32 POS_WEIGHT = 0;
	const S32 ROT_WEIGHT = 1;
	const S32 SCALE_WEIGHT = 2;

	F32				sum_weights[3];
	U32				sum_usage = 0;

	LLVector3		blended_pos = target_joint->getPosition();
	LLQuaternion	blended_rot = target_joint->getRotation();
	LLVector3		blended_scale = target_joint->getScale();

	LLVector3		added_pos;
	LLQuaternion	added_rot;
	LLVector3		added_scale;

	//S32				joint_state_index;

	sum_weights[POS_WEIGHT] = 0.f;
	sum_weights[ROT_WEIGHT] = 0.f;
	sum_weights[SCALE_WEIGHT] = 0.f;

	for(S32 joint_state_index = 0; 
		joint_state_index < JSB_NUM_JOINT_STATES && mJointStates[joint_state_index].notNull();
		joint_state_index++)
	{
		LLJointState* jsp = mJointStates[joint_state_index];
		U32 current_usage = jsp->getUsage();
		F32 current_weight = jsp->getWeight();

		if (current_weight == 0.f)
		{
			continue;
		}

		if (mAdditiveBlends[joint_state_index])
		{
			if(current_usage & LLJointState::POS)
			{
				F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[POS_WEIGHT]);

				// add in pos for this jointstate modulated by weight
				added_pos += jsp->getPosition() * (new_weight_sum - sum_weights[POS_WEIGHT]);
			}

			if(current_usage & LLJointState::SCALE)
			{
				F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[SCALE_WEIGHT]);

				// add in scale for this jointstate modulated by weight
				added_scale += jsp->getScale() * (new_weight_sum - sum_weights[SCALE_WEIGHT]);
			}

			if (current_usage & LLJointState::ROT)
			{
				F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[ROT_WEIGHT]);

				// add in rotation for this jointstate modulated by weight
				added_rot = nlerp((new_weight_sum - sum_weights[ROT_WEIGHT]), added_rot, jsp->getRotation()) * added_rot;
			}
		}
		else
		{
			// blend two jointstates together
		
			// blend position
			if(current_usage & LLJointState::POS)
			{
				if(sum_usage & LLJointState::POS)
				{
					F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[POS_WEIGHT]);

					// blend positions from both
					blended_pos = lerp(jsp->getPosition(), blended_pos, sum_weights[POS_WEIGHT] / new_weight_sum);
					sum_weights[POS_WEIGHT] = new_weight_sum;
				} 
				else
				{
					// copy position from current
					blended_pos = jsp->getPosition();
					sum_weights[POS_WEIGHT] = current_weight;
				}
			}
			
			// now do scale
			if(current_usage & LLJointState::SCALE)
			{
				if(sum_usage & LLJointState::SCALE)
				{
					F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[SCALE_WEIGHT]);

					// blend scales from both
					blended_scale = lerp(jsp->getScale(), blended_scale, sum_weights[SCALE_WEIGHT] / new_weight_sum);
					sum_weights[SCALE_WEIGHT] = new_weight_sum;
				} 
				else
				{
					// copy scale from current
					blended_scale = jsp->getScale();
					sum_weights[SCALE_WEIGHT] = current_weight;
				}
			}

			// rotation
			if (current_usage & LLJointState::ROT)
			{
				if(sum_usage & LLJointState::ROT)
				{
					F32 new_weight_sum = llmin(1.f, current_weight + sum_weights[ROT_WEIGHT]);

					// blend rotations from both
					blended_rot = nlerp(sum_weights[ROT_WEIGHT] / new_weight_sum, jsp->getRotation(), blended_rot);
					sum_weights[ROT_WEIGHT] = new_weight_sum;
				} 
				else
				{
					// copy rotation from current
					blended_rot = jsp->getRotation();
					sum_weights[ROT_WEIGHT] = current_weight;
				}
			}

			// update resulting usage mask
			sum_usage = sum_usage | current_usage;
		}
	}

	if (!added_scale.isFinite())
	{
		added_scale.clearVec();
	}

	if (!blended_scale.isFinite())
	{
		blended_scale.setVec(1,1,1);
	}

	// apply transforms
	target_joint->setPosition(blended_pos + added_pos);
	target_joint->setScale(blended_scale + added_scale);
	target_joint->setRotation(added_rot * blended_rot);

	if (apply_now)
	{
		// now clear joint states
		for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
		{
			mJointStates[i] = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// interpolate()
//-----------------------------------------------------------------------------
void LLJointStateBlender::interpolate(F32 u)
{
	// only interpolate if we have a joint state
	if (!mJointStates[0])
	{
		return;
	}
	LLJoint* target_joint = mJointStates[0]->getJoint();

	if (!target_joint)
	{
		return;
	}

	target_joint->setPosition(lerp(target_joint->getPosition(), mJointCache.getPosition(), u));
	target_joint->setScale(lerp(target_joint->getScale(), mJointCache.getScale(), u));
	target_joint->setRotation(nlerp(u, target_joint->getRotation(), mJointCache.getRotation()));
}

//-----------------------------------------------------------------------------
// clear()
//-----------------------------------------------------------------------------
void LLJointStateBlender::clear()
{
	// now clear joint states
	for(S32 i = 0; i < JSB_NUM_JOINT_STATES; i++)
	{
		mJointStates[i] = NULL;
	}
}

//-----------------------------------------------------------------------------
// resetCachedJoint()
//-----------------------------------------------------------------------------
void LLJointStateBlender::resetCachedJoint()
{
	if (!mJointStates[0])
	{
		return;
	}
	LLJoint* source_joint = mJointStates[0]->getJoint();
	mJointCache.setPosition(source_joint->getPosition());
	mJointCache.setScale(source_joint->getScale());
	mJointCache.setRotation(source_joint->getRotation());
}

//-----------------------------------------------------------------------------
// LLPoseBlender
//-----------------------------------------------------------------------------

LLPoseBlender::LLPoseBlender()
	: mNextPoseSlot(0)
{
}

LLPoseBlender::~LLPoseBlender()
{
	for_each(mJointStateBlenderPool.begin(), mJointStateBlenderPool.end(), DeletePairedPointer());
}

//-----------------------------------------------------------------------------
// addMotion()
//-----------------------------------------------------------------------------
BOOL LLPoseBlender::addMotion(LLMotion* motion)
{
	LLPose* pose = motion->getPose();

	for(LLJointState* jsp = pose->getFirstJointState(); jsp; jsp = pose->getNextJointState())
	{
		LLJoint *jointp = jsp->getJoint();
		LLJointStateBlender* joint_blender;
		if (mJointStateBlenderPool.find(jointp) == mJointStateBlenderPool.end())
		{
			// this is the first time we are animating this joint
			// so create new jointblender and add it to our pool
			joint_blender = new LLJointStateBlender();
			mJointStateBlenderPool[jointp] = joint_blender;
		}
		else
		{
			joint_blender = mJointStateBlenderPool[jointp];
		}

		if (jsp->getPriority() == LLJoint::USE_MOTION_PRIORITY)
		{
			joint_blender->addJointState(jsp, motion->getPriority(), motion->getBlendType() == LLMotion::ADDITIVE_BLEND);
		}
		else
		{
			joint_blender->addJointState(jsp, jsp->getPriority(), motion->getBlendType() == LLMotion::ADDITIVE_BLEND);
		}

		// add it to our list of active blenders
		if (std::find(mActiveBlenders.begin(), mActiveBlenders.end(), joint_blender) == mActiveBlenders.end())
		{
			mActiveBlenders.push_front(joint_blender);
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// blendAndApply()
//-----------------------------------------------------------------------------
void LLPoseBlender::blendAndApply()
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); )
	{
		LLJointStateBlender* jsbp = *iter++;
		jsbp->blendJointStates();
	}

	// we're done now so there are no more active blenders for this frame
	mActiveBlenders.clear();
}

//-----------------------------------------------------------------------------
// blendAndCache()
//-----------------------------------------------------------------------------
void LLPoseBlender::blendAndCache(BOOL reset_cached_joints)
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); ++iter)
	{
		LLJointStateBlender* jsbp = *iter;
		if (reset_cached_joints)
		{
			jsbp->resetCachedJoint();
		}
		jsbp->blendJointStates(FALSE);
	}
}

//-----------------------------------------------------------------------------
// interpolate()
//-----------------------------------------------------------------------------
void LLPoseBlender::interpolate(F32 u)
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); ++iter)
	{
		LLJointStateBlender* jsbp = *iter;
		jsbp->interpolate(u);
	}
}

//-----------------------------------------------------------------------------
// clearBlenders()
//-----------------------------------------------------------------------------
void LLPoseBlender::clearBlenders()
{
	for (blender_list_t::iterator iter = mActiveBlenders.begin();
		 iter != mActiveBlenders.end(); ++iter)
	{
		LLJointStateBlender* jsbp = *iter;
		jsbp->clear();
	}

	mActiveBlenders.clear();
}

