/** 
 * @file llviewervisualparam.cpp
 * @brief Implementation of LLViewerVisualParam class
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
#include "llviewerprecompiledheaders.h"

#include "llviewervisualparam.h"
#include "llxmltree.h"
#include "llui.h"
#include "llwearable.h"

//-----------------------------------------------------------------------------
// LLViewerVisualParamInfo()
//-----------------------------------------------------------------------------
LLViewerVisualParamInfo::LLViewerVisualParamInfo()
	:
	mWearableType( WT_INVALID ),
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
		mWearableType = LLWearableDictionary::typeNameToType( wearable );
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
	static LLStdStringHandle camera_target_string = LLXmlTree::addAttributeString("camera_target");
	node->getFastAttributeString( camera_target_string, mCamTargetName );

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
