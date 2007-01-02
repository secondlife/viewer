/** 
 * @file llhandmotion.cpp
 * @brief Implementation of LLHandMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
			mCharacter->setVisualParamWeight(gHandPoseNames[mNewPose], 0.f);
		}
		mNewPose = HAND_POSE_RELAXED;
	}
	else
	{
		// this is a new morph we didn't know about before
		if (*requestedHandPose != mNewPose && mNewPose != mCurrentPose && mNewPose != HAND_POSE_SPREAD)
		{
			mCharacter->setVisualParamWeight(gHandPoseNames[mNewPose], 0.f);
		}
		mNewPose = *requestedHandPose;
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
LLString LLHandMotion::getHandPoseName(eHandPose pose)
{
	if ((S32)pose < LLHandMotion::NUM_HAND_POSES && (S32)pose >= 0)
	{
		return gHandPoseNames[pose];
	}
	return "";
}

LLHandMotion::eHandPose LLHandMotion::getHandPose(LLString posename)
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
