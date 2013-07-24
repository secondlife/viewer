/** 
 * @file llviewervisualparam.cpp
 * @brief Implementation of LLViewerVisualParam class
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

#include "llviewervisualparam.h"
#include "llxmltree.h"
#include "llwearable.h"

//-----------------------------------------------------------------------------
// LLViewerVisualParamInfo()
//-----------------------------------------------------------------------------
LLViewerVisualParamInfo::LLViewerVisualParamInfo()
	:
	mWearableType( LLWearableType::WT_INVALID ),
	mCrossWearable(FALSE),
	mCamDist( 0.5f ),
	mCamAngle( 0.f ),
	mCamElevation( 0.f ),
	mEditGroupDisplayOrder( 0 ),
	mShowSimple(FALSE),
	mSimpleMin(0.f),
	mSimpleMax(100.f)
{
}

LLViewerVisualParamInfo::~LLViewerVisualParamInfo()
{
}

//-----------------------------------------------------------------------------
// parseXml()
//-----------------------------------------------------------------------------
BOOL LLViewerVisualParamInfo::parseXml(LLXmlTreeNode *node)
{
	llassert( node->hasName( "param" ) );

	if (!LLVisualParamInfo::parseXml(node))
		return FALSE;
	
	// VIEWER SPECIFIC PARAMS
	
	std::string wearable;
	static LLStdStringHandle wearable_string = LLXmlTree::addAttributeString("wearable");
	if( node->getFastAttributeString( wearable_string, wearable) )
	{
		mWearableType = LLWearableType::typeNameToType( wearable );
	}

	static LLStdStringHandle edit_group_string = LLXmlTree::addAttributeString("edit_group");
	if (!node->getFastAttributeString( edit_group_string, mEditGroup))
	{
		mEditGroup = "";
	}

	static LLStdStringHandle cross_wearable_string = LLXmlTree::addAttributeString("cross_wearable");
	if (!node->getFastAttributeBOOL(cross_wearable_string, mCrossWearable))
	{
		mCrossWearable = FALSE;
	}

	// Optional camera offsets from the current joint center.  Used for generating "hints" (thumbnails).
	static LLStdStringHandle camera_distance_string = LLXmlTree::addAttributeString("camera_distance");
	node->getFastAttributeF32( camera_distance_string, mCamDist );
	static LLStdStringHandle camera_angle_string = LLXmlTree::addAttributeString("camera_angle");
	node->getFastAttributeF32( camera_angle_string, mCamAngle );	// in degrees
	static LLStdStringHandle camera_elevation_string = LLXmlTree::addAttributeString("camera_elevation");
	node->getFastAttributeF32( camera_elevation_string, mCamElevation );

	mCamAngle += 180;

	static S32 params_loaded = 0;

	// By default, parameters are displayed in the order in which they appear in the xml file.
	// "edit_group_order" overriddes.
	static LLStdStringHandle edit_group_order_string = LLXmlTree::addAttributeString("edit_group_order");
	if( !node->getFastAttributeF32( edit_group_order_string, mEditGroupDisplayOrder ) )
	{
		mEditGroupDisplayOrder = (F32)params_loaded;
	}

	params_loaded++;
	
	return TRUE;
}

/*virtual*/ void LLViewerVisualParamInfo::toStream(std::ostream &out)
{
	LLVisualParamInfo::toStream(out);

	out << mWearableType << "\t";
	out << mEditGroup << "\t";
	out << mEditGroupDisplayOrder << "\t";
}

//-----------------------------------------------------------------------------
// LLViewerVisualParam()
//-----------------------------------------------------------------------------
LLViewerVisualParam::LLViewerVisualParam()
{
}

//-----------------------------------------------------------------------------
// setInfo()
//-----------------------------------------------------------------------------

BOOL LLViewerVisualParam::setInfo(LLViewerVisualParamInfo *info)
{
	llassert(mInfo == NULL);
	if (info->mID < 0)
		return FALSE;
	mInfo = info;
	mID = info->mID;
	setWeight(getDefaultWeight(), FALSE );
	return TRUE;
}

/*
//=============================================================================
// These virtual functions should always be overridden,
// but are included here for use as templates
//=============================================================================

//-----------------------------------------------------------------------------
// parseData()
//-----------------------------------------------------------------------------
BOOL LLViewerVisualParam::parseData(LLXmlTreeNode *node)
{
	LLViewerVisualParamInfo* info = new LLViewerVisualParamInfo;

	info->parseXml(node);
	if (!setInfo(info))
		return FALSE;
	
	return TRUE;
}
*/
