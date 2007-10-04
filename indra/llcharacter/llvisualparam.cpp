/** 
 * @file llvisualparam.cpp
 * @brief Implementation of LLPolyMesh class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llvisualparam.h"

//-----------------------------------------------------------------------------
// LLVisualParamInfo()
//-----------------------------------------------------------------------------
LLVisualParamInfo::LLVisualParamInfo()
	:
	mID( -1 ),
	mGroup( VISUAL_PARAM_GROUP_TWEAKABLE ),
	mMinWeight( 0.f ),
	mMaxWeight( 1.f ),
	mDefaultWeight( 0.f ),
	mSex( SEX_BOTH )
{
}

//-----------------------------------------------------------------------------
// parseXml()
//-----------------------------------------------------------------------------
BOOL LLVisualParamInfo::parseXml(LLXmlTreeNode *node)
{
	// attribute: id
	static LLStdStringHandle id_string = LLXmlTree::addAttributeString("id");
	node->getFastAttributeS32( id_string, mID );
	
	// attribute: group
	U32 group = 0;
	static LLStdStringHandle group_string = LLXmlTree::addAttributeString("group");
	if( node->getFastAttributeU32( group_string, group ) )
	{
		if( group < NUM_VISUAL_PARAM_GROUPS )
		{
			mGroup = (EVisualParamGroup)group;
		}
	}

	// attribute: value_min, value_max
	static LLStdStringHandle value_min_string = LLXmlTree::addAttributeString("value_min");
	static LLStdStringHandle value_max_string = LLXmlTree::addAttributeString("value_max");
	node->getFastAttributeF32( value_min_string, mMinWeight );
	node->getFastAttributeF32( value_max_string, mMaxWeight );

	// attribute: value_default
	F32 default_weight = 0;
	static LLStdStringHandle value_default_string = LLXmlTree::addAttributeString("value_default");
	if( node->getFastAttributeF32( value_default_string, default_weight ) )
	{
		mDefaultWeight = llclamp( default_weight, mMinWeight, mMaxWeight );
		if( default_weight != mDefaultWeight )
		{
			llwarns << "value_default attribute is out of range in node " << mName << " " << default_weight << llendl;
		}
	}
	
	// attribute: sex
	LLString sex = "both";
	static LLStdStringHandle sex_string = LLXmlTree::addAttributeString("sex");
	node->getFastAttributeString( sex_string, sex ); // optional
	if( sex == "both" )
	{
		mSex = SEX_BOTH;
	}
	else if( sex == "male" )
	{
		mSex = SEX_MALE;
	}
	else if( sex == "female" )
	{
		mSex = SEX_FEMALE;
	}
	else
	{
		llwarns << "Avatar file: <param> has invalid sex attribute: " << sex << llendl;
		return FALSE;
	}
	
	// attribute: name
	static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
	if( !node->getFastAttributeString( name_string, mName ) )
	{
		llwarns << "Avatar file: <param> is missing name attribute" << llendl;
		return FALSE;
	}

	// attribute: label
	static LLStdStringHandle label_string = LLXmlTree::addAttributeString("label");
	if( !node->getFastAttributeString( label_string, mDisplayName ) )
	{
		mDisplayName = mName;
	}

	// JC - make sure the display name includes the capitalization in the XML file,
	// not the lowercased version.
	LLString::toLower(mName);

	// attribute: label_min
	static LLStdStringHandle label_min_string = LLXmlTree::addAttributeString("label_min");
	if( !node->getFastAttributeString( label_min_string, mMinName ) )
	{
		mMinName = "Less";
	}

	// attribute: label_max
	static LLStdStringHandle label_max_string = LLXmlTree::addAttributeString("label_max");
	if( !node->getFastAttributeString( label_max_string, mMaxName ) )
	{
		mMaxName = "More";
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLVisualParam()
//-----------------------------------------------------------------------------
LLVisualParam::LLVisualParam()	
	:
	mCurWeight( 0.f ),
	mLastWeight( 0.f ),
	mNext( NULL ),
	mTargetWeight( 0.f ),
	mIsAnimating( FALSE ),
	mID( -1 ),
	mInfo( 0 )
{
}

//-----------------------------------------------------------------------------
// ~LLVisualParam()
//-----------------------------------------------------------------------------
LLVisualParam::~LLVisualParam()
{
	delete mNext;
}

/*
//=============================================================================
// These virtual functions should always be overridden,
// but are included here for use as templates
//=============================================================================

//-----------------------------------------------------------------------------
// setInfo()
//-----------------------------------------------------------------------------

BOOL LLVisualParam::setInfo(LLVisualParamInfo *info)
{
	llassert(mInfo == NULL);
	if (info->mID < 0)
		return FALSE;
	mInfo = info;
	mID = info->mID;
	setWeight(getDefaultWeight(), FALSE );
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseData()
//-----------------------------------------------------------------------------
BOOL LLVisualParam::parseData(LLXmlTreeNode *node)
{
	LLVisualParamInfo *info = new LLVisualParamInfo;

	info->parseXml(node);
	if (!setInfo(info))
		return FALSE;
	
	return TRUE;
}
*/

//-----------------------------------------------------------------------------
// setWeight()
//-----------------------------------------------------------------------------
void LLVisualParam::setWeight(F32 weight, BOOL set_by_user)
{
	if (mIsAnimating)
	{
		//RN: allow overshoot
		mCurWeight = weight;
	}
	else if (mInfo)
	{
		mCurWeight = llclamp(weight, mInfo->mMinWeight, mInfo->mMaxWeight);
	}
	else
	{
		mCurWeight = weight;
	}
	
	if (mNext)
	{
		mNext->setWeight(weight, set_by_user);
	}
}

//-----------------------------------------------------------------------------
// setAnimationTarget()
//-----------------------------------------------------------------------------
void LLVisualParam::setAnimationTarget(F32 target_value, BOOL set_by_user)
{
	if (mInfo)
	{
		if (getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)
		{
			mTargetWeight = llclamp(target_value, mInfo->mMinWeight, mInfo->mMaxWeight);
		}
	}
	else
	{
		mTargetWeight = target_value;
	}
	mIsAnimating = TRUE;

	if (mNext)
	{
		mNext->setAnimationTarget(target_value, set_by_user);
	}
}

//-----------------------------------------------------------------------------
// setNextParam()
//-----------------------------------------------------------------------------
void LLVisualParam::setNextParam( LLVisualParam *next )
{
	llassert(!mNext);

	mNext = next;
}

//-----------------------------------------------------------------------------
// animate()
//-----------------------------------------------------------------------------
void LLVisualParam::animate( F32 delta, BOOL set_by_user )
{
	if (mIsAnimating)
	{
		F32 new_weight = ((mTargetWeight - mCurWeight) * delta) + mCurWeight;
		setWeight(new_weight, set_by_user);
	}
}

//-----------------------------------------------------------------------------
// stopAnimating()
//-----------------------------------------------------------------------------
void LLVisualParam::stopAnimating(BOOL set_by_user)
{ 
	if (mIsAnimating && getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)
	{
		mIsAnimating = FALSE; 
		setWeight(mTargetWeight, set_by_user);
	}
}
