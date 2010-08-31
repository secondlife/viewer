/** 
 * @file llemote.cpp
 * @brief Implementation of LLEmote class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"

#include "llemote.h"
#include "llcharacter.h"
#include "m3math.h"
#include "llvoavatar.h"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLEmote()
// Class Constructor
//-----------------------------------------------------------------------------
LLEmote::LLEmote(const LLUUID &id) : LLMotion(id)
{
	mCharacter = NULL;

	//RN: flag face joint as highest priority for now, until we implement a proper animation track
	mJointSignature[0][LL_FACE_JOINT_NUM] = 0xff;
	mJointSignature[1][LL_FACE_JOINT_NUM] = 0xff;
	mJointSignature[2][LL_FACE_JOINT_NUM] = 0xff;
}


//-----------------------------------------------------------------------------
// ~LLEmote()
// Class Destructor
//-----------------------------------------------------------------------------
LLEmote::~LLEmote()
{
}

//-----------------------------------------------------------------------------
// LLEmote::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLEmote::onInitialize(LLCharacter *character)
{
	mCharacter = character;
	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// LLEmote::onActivate()
//-----------------------------------------------------------------------------
BOOL LLEmote::onActivate()
{
	LLVisualParam* default_param = mCharacter->getVisualParam( "Express_Closed_Mouth" );
	if( default_param )
	{
		default_param->setWeight( default_param->getMaxWeight(), FALSE );
	}

	mParam = mCharacter->getVisualParam(mName.c_str());
	if (mParam)
	{
		mParam->setWeight(0.f, FALSE);
		mCharacter->updateVisualParams();
	}
	
	return TRUE;
}


//-----------------------------------------------------------------------------
// LLEmote::onUpdate()
//-----------------------------------------------------------------------------
BOOL LLEmote::onUpdate(F32 time, U8* joint_mask)
{
	if( mParam )
	{
		F32 weight = mParam->getMinWeight() + mPose.getWeight() * (mParam->getMaxWeight() - mParam->getMinWeight());
		mParam->setWeight(weight, FALSE);

		// Cross fade against the default parameter
		LLVisualParam* default_param = mCharacter->getVisualParam( "Express_Closed_Mouth" );
		if( default_param )
		{
			F32 default_param_weight = default_param->getMinWeight() + 
				(1.f - mPose.getWeight()) * ( default_param->getMaxWeight() - default_param->getMinWeight() );
			
			default_param->setWeight( default_param_weight, FALSE );
		}

		mCharacter->updateVisualParams();
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// LLEmote::onDeactivate()
//-----------------------------------------------------------------------------
void LLEmote::onDeactivate()
{
	if( mParam )
	{
		mParam->setWeight( mParam->getDefaultWeight(), FALSE );
	}

	LLVisualParam* default_param = mCharacter->getVisualParam( "Express_Closed_Mouth" );
	if( default_param )
	{
		default_param->setWeight( default_param->getMaxWeight(), FALSE );
	}

	mCharacter->updateVisualParams();
}


// End
