/** 
 * @file llviewervisualparam.cpp
 * @brief Implementation of LLViewerVisualParam class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	
	LLString wearable;
	static LLStdStringHandle wearable_string = LLXmlTree::addAttributeString("wearable");
	if( node->getFastAttributeString( wearable_string, wearable) )
	{
		mWearableType = LLWearable::typeNameToType( wearable );
	}

	static LLStdStringHandle edit_group_string = LLXmlTree::addAttributeString("edit_group");
	if (!node->getFastAttributeString( edit_group_string, mEditGroup))
	{
		mEditGroup = "";
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

//-----------------------------------------------------------------------------
// LLViewerVisualParam()
//-----------------------------------------------------------------------------
LLViewerVisualParam::LLViewerVisualParam()
{
}

/*
//=============================================================================
// These virtual functions should always be overridden,
// but are included here for use as templates
//=============================================================================

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
