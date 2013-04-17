/** 
 * @file llvisualparam.cpp
 * @brief Implementation of LLPolyMesh class.
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
	std::string sex = "both";
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
	LLStringUtil::toLower(mName);

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

//virtual
void LLVisualParamInfo::toStream(std::ostream &out)
{
	out <<  mID << "\t";
	out << mName << "\t";
	out <<  mDisplayName << "\t";
	out <<  mMinName << "\t";
	out <<  mMaxName << "\t";
	out <<  mGroup << "\t";
	out <<  mMinWeight << "\t";
	out <<  mMaxWeight << "\t";
	out <<  mDefaultWeight << "\t";
	out <<  mSex << "\t";
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
	mInfo( 0 ),
	mIsDummy(FALSE),
	mParamLocation(LOC_UNKNOWN)
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
void LLVisualParam::setWeight(F32 weight, BOOL upload_bake)
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
		mNext->setWeight(weight, upload_bake);
	}
}

//-----------------------------------------------------------------------------
// setAnimationTarget()
//-----------------------------------------------------------------------------
void LLVisualParam::setAnimationTarget(F32 target_value, BOOL upload_bake)
{
	// don't animate dummy parameters
	if (mIsDummy)
	{
		setWeight(target_value, upload_bake);
		mTargetWeight = mCurWeight;
		return;
	}

	if (mInfo)
	{
		if (isTweakable())
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
		mNext->setAnimationTarget(target_value, upload_bake);
	}
}

//-----------------------------------------------------------------------------
// setNextParam()
//-----------------------------------------------------------------------------
void LLVisualParam::setNextParam( LLVisualParam *next )
{
	llassert(!mNext);
	llassert(getWeight() == getDefaultWeight()); // need to establish mNext before we start changing values on this, else initial value won't get mirrored (we can fix that, but better to forbid this pattern)
	mNext = next;
}

//-----------------------------------------------------------------------------
// animate()
//-----------------------------------------------------------------------------
void LLVisualParam::animate( F32 delta, BOOL upload_bake )
{
	if (mIsAnimating)
	{
		F32 new_weight = ((mTargetWeight - mCurWeight) * delta) + mCurWeight;
		setWeight(new_weight, upload_bake);
	}
}

//-----------------------------------------------------------------------------
// stopAnimating()
//-----------------------------------------------------------------------------
void LLVisualParam::stopAnimating(BOOL upload_bake)
{ 
	if (mIsAnimating && isTweakable())
	{
		mIsAnimating = FALSE; 
		setWeight(mTargetWeight, upload_bake);
	}
}

//virtual
BOOL LLVisualParam::linkDrivenParams(visual_param_mapper mapper, BOOL only_cross_params)
{
	// nothing to do for non-driver parameters
	return TRUE;
}

//virtual 
void LLVisualParam::resetDrivenParams()
{
	// nothing to do for non-driver parameters
	return;
}

const std::string param_location_name(const EParamLocation& loc)
{
	switch (loc)
	{
		case LOC_UNKNOWN: return "unknown";
		case LOC_AV_SELF: return "self";
		case LOC_AV_OTHER: return "other";
		case LOC_WEARABLE: return "wearable";
		default: return "error";
	}
}

void LLVisualParam::setParamLocation(EParamLocation loc)
{
	if (mParamLocation == LOC_UNKNOWN || loc == LOC_UNKNOWN)
	{
		mParamLocation = loc;
	}
	else if (mParamLocation == loc)
	{
		// no action
	}
	else
	{
		lldebugs << "param location is already " << mParamLocation << ", not slamming to " << loc << llendl;
	}
}

