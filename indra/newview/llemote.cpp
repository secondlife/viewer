/** 
 * @file llemote.cpp
 * @brief Implementation of LLEmote class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
