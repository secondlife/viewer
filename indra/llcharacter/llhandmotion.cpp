/** 
 * @file llhandmotion.cpp
 * @brief Implementation of LLHandMotion class.
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

#include "llhandmotion.h"
#include "llcharacter.h"
#include "m3math.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

const char *gHandPoseNames[LLHandMotion::NUM_HAND_POSES] =  /* Flawfinder: ignore */
{
	"",
	"Hands_Relaxed",
	"Hands_Point",
	"Hands_Fist",
	"Hands_Relaxed_L",
	"Hands_Point_L",
	"Hands_Fist_L",
	"Hands_Relaxed_R",
	"Hands_Point_R",
	"Hands_Fist_R",
	"Hands_Salute_R",
	"Hands_Typing",
	"Hands_Peace_R",
	"Hands_Spread_R"
};

const F32 HAND_MORPH_BLEND_TIME = 0.2f;

//-----------------------------------------------------------------------------
// LLHandMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLHandMotion::LLHandMotion(const LLUUID &id) : LLMotion(id)
{
	mCharacter = NULL;
	mLastTime = 0.f;
	mCurrentPose = HAND_POSE_RELAXED;
	mNewPose = HAND_POSE_RELAXED;
	mName = "hand_motion";

	//RN: flag hand joint as highest priority for now, until we implement a proper animation track
	mJointSignature[0][LL_HAND_JOINT_NUM] = 0xff;
	mJointSignature[1][LL_HAND_JOINT_NUM] = 0xff;
	mJointSignature[2][LL_HAND_JOINT_NUM] = 0xff;
}


//-----------------------------------------------------------------------------
// ~LLHandMotion()
// Class Destructor
//-----------------------------------------------------------------------------
LLHandMotion::~LLHandMotion()
{
}

//-----------------------------------------------------------------------------
// LLHandMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLHandMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;

	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// LLHandMotion::onActivate()
//-----------------------------------------------------------------------------
BOOL LLHandMotion::onActivate()
{
	LLPolyMesh *upperBodyMesh = mCharacter->getUpperBodyMesh();

	if (upperBodyMesh)
	{
		// Note: 0 is the default
		for (S32 i = 1; i < LLHandMotion::NUM_HAND_POSES; i++)
		{
			mCharacter->setVisualParamWeight(gHandPoseNames[i], 0.f);
		}
		mCharacter->setVisualParamWeight(gHandPoseNames[mCurrentPose], 1.f);
		mCharacter->updateVisualParams();
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// LLHandMotion::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLHandMotion::onUpdate(F32 time, U8* joint_mask)
{
	eHandPose *requestedHandPose;

	F32 timeDelta = time - mLastTime;
	mLastTime = time;

	requestedHandPose = (eHandPose *)mCharacter->getAnimationData("Hand Pose");
	// check to see if requested pose has changed
	if (!requestedHandPose)
	{
		if (mNewPose != HAND_POSE_RELAXED && mNewPose != mCurrentPose)
		{
			// Only set param weight for poses other than
			// default (HAND_POSE_SPREAD); HAND_POSE_SPREAD
			// is not an animatable morph!
			if (mNewPose != HAND_POSE_SPREAD)
			{
				mCharacter->setVisualParamWeight(gHandPoseNames[mNewPose], 0.f);
			}

			// Reset morph weight for current pose back to its
			// full extend or it might be stuck somewhere in the middle if a
			// pose is requested and the old pose is requested again shortly
			// after while still blending to the other pose!
			if (mCurrentPose != HAND_POSE_SPREAD)
			{
				mCharacter->setVisualParamWeight(gHandPoseNames[mCurrentPose], 1.f);
			}

			// Update visual params now if we won't blend
			if (mCurrentPose == HAND_POSE_RELAXED)
			{
				mCharacter->updateVisualParams();
			}
		}
		mNewPose = HAND_POSE_RELAXED;
	}
	else
	{
		// Sometimes we seem to get garbage here, with poses that are out of bounds.
		// So check for a valid pose first.
		if (*requestedHandPose >= 0 && *requestedHandPose < NUM_HAND_POSES)
		{
			// This is a new morph we didn't know about before:
			// Reset morph weight for both current and new pose
			// back their starting values while still blending.
			if (*requestedHandPose != mNewPose && mNewPose != mCurrentPose)
			{
				if (mNewPose != HAND_POSE_SPREAD)
				{
					mCharacter->setVisualParamWeight(gHandPoseNames[mNewPose], 0.f);
				}

				// Reset morph weight for current pose back to its full extend
				// or it might be stuck somewhere in the middle if a pose is
				// requested and the old pose is requested again shortly after
				// while still blending to the other pose!
				if (mCurrentPose != HAND_POSE_SPREAD)
				{
					mCharacter->setVisualParamWeight(gHandPoseNames[mCurrentPose], 1.f);
				}

				// Update visual params now if we won't blend
				if (mCurrentPose == *requestedHandPose)
				{
					mCharacter->updateVisualParams();
				}
			}
			mNewPose = *requestedHandPose;
		}
		else
		{
			llwarns << "Requested hand pose out of range. Ignoring requested pose." << llendl;
		}
	}

	mCharacter->removeAnimationData("Hand Pose");
	mCharacter->removeAnimationData("Hand Pose Priority");

//	if (requestedHandPose)
//		llinfos << "Hand Pose " << *requestedHandPose << llendl;

	// if we are still blending...
	if (mCurrentPose != mNewPose)
	{
		F32 incomingWeight = 1.f;
		F32 outgoingWeight = 0.f;

		if (mNewPose != HAND_POSE_SPREAD)
		{
			incomingWeight = mCharacter->getVisualParamWeight(gHandPoseNames[mNewPose]);
			incomingWeight += (timeDelta / HAND_MORPH_BLEND_TIME);
			incomingWeight = llclamp(incomingWeight, 0.f, 1.f);
			mCharacter->setVisualParamWeight(gHandPoseNames[mNewPose], incomingWeight);
		}

		if (mCurrentPose != HAND_POSE_SPREAD)
		{
			outgoingWeight = mCharacter->getVisualParamWeight(gHandPoseNames[mCurrentPose]);
			outgoingWeight -= (timeDelta / HAND_MORPH_BLEND_TIME);
			outgoingWeight = llclamp(outgoingWeight, 0.f, 1.f);
			mCharacter->setVisualParamWeight(gHandPoseNames[mCurrentPose], outgoingWeight);
		}

		mCharacter->updateVisualParams();
		
		if (incomingWeight == 1.f && outgoingWeight == 0.f)
		{
			mCurrentPose = mNewPose;
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// LLHandMotion::onDeactivate()
//-----------------------------------------------------------------------------
void LLHandMotion::onDeactivate()
{
}

//-----------------------------------------------------------------------------
// LLHandMotion::getHandPoseName()
//-----------------------------------------------------------------------------
std::string LLHandMotion::getHandPoseName(eHandPose pose)
{
	if ((S32)pose < LLHandMotion::NUM_HAND_POSES && (S32)pose >= 0)
	{
		return std::string(gHandPoseNames[pose]);
	}
	return LLStringUtil::null;
}

LLHandMotion::eHandPose LLHandMotion::getHandPose(std::string posename)
{
	for (S32 pose = 0; pose < LLHandMotion::NUM_HAND_POSES; ++pose)
	{
		if (gHandPoseNames[pose] == posename)
		{
			return (eHandPose)pose;
		}
	}
	return (eHandPose)0;
}

// End
