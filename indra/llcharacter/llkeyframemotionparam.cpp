/** 
 * @file llkeyframemotionparam.cpp
 * @brief Implementation of LLKeyframeMotion class.
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

#include "llkeyframemotionparam.h"
#include "llcharacter.h"
#include "llmath.h"
#include "m3math.h"
#include "lldir.h"
#include "llanimationstates.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLKeyframeMotionParam class
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLKeyframeMotionParam()
// Class Constructor
//-----------------------------------------------------------------------------
LLKeyframeMotionParam::LLKeyframeMotionParam( const LLUUID &id) : LLMotion(id)
{
	mDefaultKeyframeMotion = NULL;
	mCharacter = NULL;

	mEaseInDuration = 0.f;
	mEaseOutDuration = 0.f;
	mDuration = 0.f;
	mPriority = LLJoint::LOW_PRIORITY;
}


//-----------------------------------------------------------------------------
// ~LLKeyframeMotionParam()
// Class Destructor
//-----------------------------------------------------------------------------
LLKeyframeMotionParam::~LLKeyframeMotionParam()
{
	for (motion_map_t::iterator iter = mParameterizedMotions.begin();
		 iter != mParameterizedMotions.end(); ++iter)
	{
		motion_list_t& motionList = iter->second;
		for (motion_list_t::iterator iter2 = motionList.begin(); iter2 != motionList.end(); ++iter2)
		{
			const ParameterizedMotion& paramMotion = *iter2;
			delete paramMotion.mMotion;
		}
		motionList.clear();
	}
	mParameterizedMotions.clear();
}

//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLKeyframeMotionParam::onInitialize(LLCharacter *character)
{
	mCharacter = character;

	if (!loadMotions())
	{
		return STATUS_FAILURE;	
	}
	
	for (motion_map_t::iterator iter = mParameterizedMotions.begin();
		 iter != mParameterizedMotions.end(); ++iter)
	{
		motion_list_t& motionList = iter->second;
		for (motion_list_t::iterator iter2 = motionList.begin(); iter2 != motionList.end(); ++iter2)
		{
			const ParameterizedMotion& paramMotion = *iter2;
			LLMotion* motion = paramMotion.mMotion;
			motion->onInitialize(character);

			if (motion->getDuration() > mEaseInDuration)
			{
				mEaseInDuration = motion->getEaseInDuration();
			}

			if (motion->getEaseOutDuration() > mEaseOutDuration)
			{
				mEaseOutDuration = motion->getEaseOutDuration();
			}

			if (motion->getDuration() > mDuration)
			{
				mDuration = motion->getDuration();
			}

			if (motion->getPriority() > mPriority)
			{
				mPriority = motion->getPriority();
			}

			LLPose *pose = motion->getPose();

			mPoseBlender.addMotion(motion);
			for (LLJointState *jsp = pose->getFirstJointState(); jsp; jsp = pose->getNextJointState())
			{
				LLPose *blendedPose = mPoseBlender.getBlendedPose();
				blendedPose->addJointState(jsp);
			}
		}
	}

	return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::onActivate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotionParam::onActivate()
{
	for (motion_map_t::iterator iter = mParameterizedMotions.begin();
		 iter != mParameterizedMotions.end(); ++iter)
	{
		motion_list_t& motionList = iter->second;
		for (motion_list_t::iterator iter2 = motionList.begin(); iter2 != motionList.end(); ++iter2)
		{
			const ParameterizedMotion& paramMotion = *iter2;
			paramMotion.mMotion->activate(mActivationTimestamp);
		}
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotionParam::onUpdate(F32 time, U8* joint_mask)
{
	F32 weightFactor = 1.f / (F32)mParameterizedMotions.size();

	// zero out all pose weights
	for (motion_map_t::iterator iter = mParameterizedMotions.begin();
		 iter != mParameterizedMotions.end(); ++iter)
	{
		motion_list_t& motionList = iter->second;
		for (motion_list_t::iterator iter2 = motionList.begin(); iter2 != motionList.end(); ++iter2)
		{
			const ParameterizedMotion& paramMotion = *iter2;
//			llinfos << "Weight for pose " << paramMotion.mMotion->getName() << " is " << paramMotion.mMotion->getPose()->getWeight() << llendl;
			paramMotion.mMotion->getPose()->setWeight(0.f);
		}
	}


	for (motion_map_t::iterator iter = mParameterizedMotions.begin();
		 iter != mParameterizedMotions.end(); ++iter)
	{
		const std::string& paramName = iter->first;
		F32* paramValue = (F32 *)mCharacter->getAnimationData(paramName);
		if (NULL == paramValue) // unexpected, but...
		{
			llwarns << "paramValue == NULL" << llendl;
			continue;
		}

		// DANGER! Do not modify mParameterizedMotions while using these pointers!
		const ParameterizedMotion* firstMotion = NULL;
		const ParameterizedMotion* secondMotion = NULL;

		motion_list_t& motionList = iter->second;
		for (motion_list_t::iterator iter2 = motionList.begin(); iter2 != motionList.end(); ++iter2)
		{
			const ParameterizedMotion& paramMotion = *iter2;
			paramMotion.mMotion->onUpdate(time, joint_mask);
			
			F32 distToParam = paramMotion.mParam - *paramValue;
			
			if ( distToParam <= 0.f)
			{
				// keep track of the motion closest to the parameter value
				firstMotion = &paramMotion;
			}
			else
			{
				// we've passed the parameter value
				// so store the first motion we find as the second one we want to blend...
				if (firstMotion && !secondMotion )
				{
					secondMotion = &paramMotion;
				}
				//...or, if we've seen no other motion so far, make sure we blend to this only
				else if (!firstMotion)
				{
					firstMotion = &paramMotion;
					secondMotion = &paramMotion;
				}
			}
		}

		LLPose *firstPose;
		LLPose *secondPose;

		if (firstMotion)
			firstPose = firstMotion->mMotion->getPose();
		else
			firstPose = NULL;

		if (secondMotion)
			secondPose = secondMotion->mMotion->getPose();
		else
			secondPose = NULL;
		
		// now modify weight of the subanim (only if we are blending between two motions)
		if (firstMotion && secondMotion)
		{
			if (firstMotion == secondMotion)
			{
				firstPose->setWeight(weightFactor);
			}
			else if (firstMotion->mParam == secondMotion->mParam)
			{
				firstPose->setWeight(0.5f * weightFactor);
				secondPose->setWeight(0.5f * weightFactor);
			}
			else
			{
				F32 first_weight = 1.f - 
					((llclamp(*paramValue - firstMotion->mParam, 0.f, (secondMotion->mParam - firstMotion->mParam))) / 
						(secondMotion->mParam - firstMotion->mParam));
				first_weight = llclamp(first_weight, 0.f, 1.f);
				
				F32 second_weight = 1.f - first_weight;
				
				firstPose->setWeight(first_weight * weightFactor);
				secondPose->setWeight(second_weight * weightFactor);

//				llinfos << "Parameter " << *paramName << ": " << *paramValue << llendl;
//				llinfos << "Weights " << firstPose->getWeight() << " " << secondPose->getWeight() << llendl;
			}
		}
		else if (firstMotion && !secondMotion)
		{
			firstPose->setWeight(weightFactor);
		}
	}

	// blend poses
	mPoseBlender.blendAndApply();

	llinfos << "Param Motion weight " << mPoseBlender.getBlendedPose()->getWeight() << llendl;

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::onDeactivate()
//-----------------------------------------------------------------------------
void LLKeyframeMotionParam::onDeactivate()
{
	for (motion_map_t::iterator iter = mParameterizedMotions.begin();
		 iter != mParameterizedMotions.end(); ++iter)
	{
		motion_list_t& motionList = iter->second;
		for (motion_list_t::iterator iter2 = motionList.begin(); iter2 != motionList.end(); ++iter2)
		{
			const ParameterizedMotion& paramMotion = *iter2;
			paramMotion.mMotion->onDeactivate();
		}
	}
}

//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::addKeyframeMotion()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotionParam::addKeyframeMotion(char *name, const LLUUID &id, char *param, F32 value)
{
	LLMotion *newMotion = mCharacter->createMotion( id );
	
	if (!newMotion)
	{
		return FALSE;
	}
	
	newMotion->setName(name);

	// now add motion to this list
	mParameterizedMotions[param].insert(ParameterizedMotion(newMotion, value));

	return TRUE;
}


//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::setDefaultKeyframeMotion()
//-----------------------------------------------------------------------------
void LLKeyframeMotionParam::setDefaultKeyframeMotion(char *name)
{
	for (motion_map_t::iterator iter = mParameterizedMotions.begin();
		 iter != mParameterizedMotions.end(); ++iter)
	{
		motion_list_t& motionList = iter->second;
		for (motion_list_t::iterator iter2 = motionList.begin(); iter2 != motionList.end(); ++iter2)
		{
			const ParameterizedMotion& paramMotion = *iter2;
			if (paramMotion.mMotion->getName() == name)
			{
				mDefaultKeyframeMotion = paramMotion.mMotion;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// loadMotions()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotionParam::loadMotions()
{
	//-------------------------------------------------------------------------
	// Load named file by concatenating the character prefix with the motion name.
	// Load data into a buffer to be parsed.
	//-------------------------------------------------------------------------
	std::string path = gDirUtilp->getExpandedFilename(LL_PATH_MOTIONS,mCharacter->getAnimationPrefix())
		+ "_" + getName() + ".llp";

	//-------------------------------------------------------------------------
	// open the file
	//-------------------------------------------------------------------------
	S32 fileSize = 0;
	LLAPRFile infile ;
	infile.open(path, LL_APR_R, NULL, &fileSize);
	apr_file_t* fp = infile.getFileHandle() ;
	if (!fp || fileSize == 0)
	{
		llinfos << "ERROR: can't open: " << path << llendl;
		return FALSE;
	}

	// allocate a text buffer
	char *text = new char[ fileSize+1 ];
	if ( !text )
	{
		llinfos << "ERROR: can't allocated keyframe text buffer." << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// load data from file into buffer
	//-------------------------------------------------------------------------
	bool error = false;
	char *p = text;
	while ( 1 )
	{
		if (apr_file_eof(fp) == APR_EOF)
		{
			break;
		}
		if (apr_file_gets(p, 1024, fp) != APR_SUCCESS)
		{
			error = true;
			break;
		}
		while ( *(++p) )
			;
	}

	//-------------------------------------------------------------------------
	// close the file
	//-------------------------------------------------------------------------
	infile.close();

	//-------------------------------------------------------------------------
	// check for error
	//-------------------------------------------------------------------------
	llassert( p <= (text+fileSize) );

	if ( error )
	{
		llinfos << "ERROR: error while reading from " << path << llendl;
		delete [] text;
		return FALSE;
	}

	llinfos << "Loading parametric keyframe data for: " << getName() << llendl;

	//-------------------------------------------------------------------------
	// parse the text and build keyframe data structures
	//-------------------------------------------------------------------------
	p = text;
	S32 num;
	char strA[80]; /* Flawfinder: ignore */
	char strB[80]; /* Flawfinder: ignore */
	F32 floatA = 0.0f;


	//-------------------------------------------------------------------------
	// get priority
	//-------------------------------------------------------------------------
	BOOL isFirstMotion = TRUE;
	num = sscanf(p, "%79s %79s %f", strA, strB, &floatA);	/* Flawfinder: ignore */

	while(1)
	{
		if (num == 0 || num == EOF) break;
		if ((num != 3))
		{
			llinfos << "WARNING: can't read parametric motion" << llendl;
			delete [] text;
			return FALSE;
		}

		addKeyframeMotion(strA, gAnimLibrary.stringToAnimState(std::string(strA)), strB, floatA);
		if (isFirstMotion)
		{
			isFirstMotion = FALSE;
			setDefaultKeyframeMotion(strA);
		}
		
		p = strstr(p, "\n");
		if (!p)
		{
			break;
		}
			
		p++;
		num = sscanf(p, "%79s %79s %f", strA, strB, &floatA);	/* Flawfinder: ignore */
	}

	delete [] text;
	return TRUE;
}

// End
