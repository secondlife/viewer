/** 
 * @file llkeyframemotionparam.cpp
 * @brief Implementation of LLKeyframeMotion class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
// sortFunc()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotionParam::sortFunc(ParameterizedMotion *new_motion, ParameterizedMotion *tested_motion)
{
	return (new_motion->second < tested_motion->second);
}

//-----------------------------------------------------------------------------
// LLKeyframeMotionParam()
// Class Constructor
//-----------------------------------------------------------------------------
LLKeyframeMotionParam::LLKeyframeMotionParam( const LLUUID &id) : LLMotion(id)
{
	mJointStates = NULL;
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
	for (U32 i = 0; i < mParameterizedMotions.length(); i++)
	{
		LLLinkedList< ParameterizedMotion > *motionList = *mParameterizedMotions.getValueAt(i);
		for (ParameterizedMotion* paramMotion = motionList->getFirstData(); paramMotion; paramMotion = motionList->getNextData())
		{
			delete paramMotion->first;
		}
		delete motionList;
	}

	mParameterizedMotions.removeAll();
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
	
	for (U32 i = 0; i < mParameterizedMotions.length(); i++)
	{
		LLLinkedList< ParameterizedMotion > *motionList = *mParameterizedMotions.getValueAt(i);
		for (ParameterizedMotion* paramMotion = motionList->getFirstData(); paramMotion; paramMotion = motionList->getNextData())
		{
			paramMotion->first->onInitialize(character);

			if (paramMotion->first->getDuration() > mEaseInDuration)
			{
				mEaseInDuration = paramMotion->first->getEaseInDuration();
			}

			if (paramMotion->first->getEaseOutDuration() > mEaseOutDuration)
			{
				mEaseOutDuration = paramMotion->first->getEaseOutDuration();
			}

			if (paramMotion->first->getDuration() > mDuration)
			{
				mDuration = paramMotion->first->getDuration();
			}

			if (paramMotion->first->getPriority() > mPriority)
			{
				mPriority = paramMotion->first->getPriority();
			}

			LLPose *pose = paramMotion->first->getPose();

			mPoseBlender.addMotion(paramMotion->first);
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
	for (U32 i = 0; i < mParameterizedMotions.length(); i++)
	{
		LLLinkedList< ParameterizedMotion > *motionList = *mParameterizedMotions.getValueAt(i);
		for (ParameterizedMotion* paramMotion = motionList->getFirstData(); paramMotion; paramMotion = motionList->getNextData())
		{
			paramMotion->first->activate();
		}
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLKeyframeMotionParam::onUpdate(F32 time, U8* joint_mask)
{
	F32 weightFactor = 1.f / (F32)mParameterizedMotions.length();
	U32 i;

	// zero out all pose weights
	for (i = 0; i < mParameterizedMotions.length(); i++)
	{
		LLLinkedList< ParameterizedMotion > *motionList = *mParameterizedMotions.getValueAt(i);

		for (ParameterizedMotion* paramMotion = motionList->getFirstData(); paramMotion; paramMotion = motionList->getNextData())
		{
//			llinfos << "Weight for pose " << paramMotion->first->getName() << " is " << paramMotion->first->getPose()->getWeight() << llendl;
			paramMotion->first->getPose()->setWeight(0.f);
		}
	}


	for (i = 0; i < mParameterizedMotions.length(); i++)
	{
		LLLinkedList< ParameterizedMotion > *motionList = *mParameterizedMotions.getValueAt(i);
		std::string *paramName = mParameterizedMotions.getIndexAt(i);
		F32* paramValue = (F32 *)mCharacter->getAnimationData(*paramName);
		ParameterizedMotion* firstMotion = NULL;
		ParameterizedMotion* secondMotion = NULL;

		for (ParameterizedMotion* paramMotion = motionList->getFirstData(); paramMotion; paramMotion = motionList->getNextData())
		{
			paramMotion->first->onUpdate(time, joint_mask);
			
			F32 distToParam = paramMotion->second - *paramValue;
			
			if ( distToParam <= 0.f)
			{
				// keep track of the motion closest to the parameter value
				firstMotion = paramMotion;
			}
			else
			{
				// we've passed the parameter value
				// so store the first motion we find as the second one we want to blend...
				if (firstMotion && !secondMotion )
				{
					secondMotion = paramMotion;
				}
				//...or, if we've seen no other motion so far, make sure we blend to this only
				else if (!firstMotion)
				{
					firstMotion = paramMotion;
					secondMotion = paramMotion;
				}
			}
		}

		LLPose *firstPose;
		LLPose *secondPose;

		if (firstMotion)
			firstPose = firstMotion->first->getPose();
		else
			firstPose = NULL;

		if (secondMotion)
			secondPose = secondMotion->first->getPose();
		else
			secondPose = NULL;
		
		// now modify weight of the subanim (only if we are blending between two motions)
		if (firstMotion && secondMotion)
		{
			if (firstMotion == secondMotion)
			{
				firstPose->setWeight(weightFactor);
			}
			else if (firstMotion->second == secondMotion->second)
			{
				firstPose->setWeight(0.5f * weightFactor);
				secondPose->setWeight(0.5f * weightFactor);
			}
			else
			{
				F32 first_weight = 1.f - 
					((llclamp(*paramValue - firstMotion->second, 0.f, (secondMotion->second - firstMotion->second))) / 
						(secondMotion->second - firstMotion->second));
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
	for (U32 i = 0; i < mParameterizedMotions.length(); i++)
	{
		LLLinkedList< ParameterizedMotion > *motionList = *mParameterizedMotions.getValueAt(i);
		for (ParameterizedMotion* paramMotion = motionList->getFirstData(); paramMotion; paramMotion = motionList->getNextData())
		{
			paramMotion->first->onDeactivate();
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

	// make sure a list of motions exists for this parameter
	LLLinkedList< ParameterizedMotion > *motionList;
	if (mParameterizedMotions.getValue(param))
	{
		motionList = *mParameterizedMotions.getValue(param);
	}
	else
	{
		motionList = new LLLinkedList< ParameterizedMotion >;
		motionList->setInsertBefore(sortFunc);
		mParameterizedMotions.addToHead(param, motionList);
	}

	// now add motion to this list
	ParameterizedMotion *parameterizedMotion = new ParameterizedMotion(newMotion, value);

	motionList->addDataSorted(parameterizedMotion);

	return TRUE;
}


//-----------------------------------------------------------------------------
// LLKeyframeMotionParam::setDefaultKeyframeMotion()
//-----------------------------------------------------------------------------
void LLKeyframeMotionParam::setDefaultKeyframeMotion(char *name)
{
	for (U32 i = 0; i < mParameterizedMotions.length(); i++)
	{
		LLLinkedList< ParameterizedMotion > *motionList = *mParameterizedMotions.getValueAt(i);
		for (ParameterizedMotion* paramMotion = motionList->getFirstData(); paramMotion; paramMotion = motionList->getNextData())
		{
			if (paramMotion->first->getName() == name)
			{
				mDefaultKeyframeMotion = paramMotion->first;
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
	char path[LL_MAX_PATH];		/* Flawfinder: ignore */
	snprintf( path,sizeof(path), "%s_%s.llp",	/* Flawfinder: ignore */
		gDirUtilp->getExpandedFilename(LL_PATH_MOTIONS,mCharacter->getAnimationPrefix()).c_str(),
		getName().c_str() );	

	//-------------------------------------------------------------------------
	// open the file
	//-------------------------------------------------------------------------
	S32 fileSize = 0;
	apr_file_t* fp = ll_apr_file_open(path, LL_APR_R, &fileSize);
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
		apr_file_close(fp);
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
	apr_file_close( fp );

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

		addKeyframeMotion(strA, gAnimLibrary.stringToAnimState(strA), strB, floatA);
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
