/** 
 * @file llviewervisualparam.h
 * @brief viewer side visual params (with data file parsing)
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

#ifndef LL_LLViewerVisualParam_H
#define LL_LLViewerVisualParam_H

#include "v3math.h"
#include "llstring.h"
#include "llvisualparam.h"

class LLWearable;

//-----------------------------------------------------------------------------
// LLViewerVisualParamInfo
//-----------------------------------------------------------------------------
class LLViewerVisualParamInfo : public LLVisualParamInfo
{
	friend class LLViewerVisualParam;
public:
	LLViewerVisualParamInfo();
	/*virtual*/ ~LLViewerVisualParamInfo();
	
	/*virtual*/ BOOL parseXml(LLXmlTreeNode* node);

	/*virtual*/ void toStream(std::ostream &out);

protected:
	S32			mWearableType;
	BOOL		mCrossWearable;
	std::string	mEditGroup;
	F32			mCamDist;
	F32			mCamAngle;		// degrees
	F32			mCamElevation;
	std::string	mCamTargetName;
	F32			mEditGroupDisplayOrder;
	BOOL		mShowSimple;	// show edit controls when in "simple ui" mode?
	F32			mSimpleMin;		// when in simple UI, apply this minimum, range 0.f to 100.f
	F32			mSimpleMax;		// when in simple UI, apply this maximum, range 0.f to 100.f
};

//-----------------------------------------------------------------------------
// LLViewerVisualParam
// VIRTUAL CLASS
// a viewer side interface class for a generalized parametric modification of the avatar mesh
//-----------------------------------------------------------------------------
class LLViewerVisualParam : public LLVisualParam
{
public:
	LLViewerVisualParam();
	/*virtual*/ ~LLViewerVisualParam(){};

	// Special: These functions are overridden by child classes
	LLViewerVisualParamInfo 	*getInfo() const { return (LLViewerVisualParamInfo*)mInfo; };
	//   This sets mInfo and calls initialization functions
	BOOL						setInfo(LLViewerVisualParamInfo *info);

	virtual LLViewerVisualParam* cloneParam(LLWearable* wearable) const = 0;
	
	// LLVisualParam Virtual functions
	///*virtual*/ BOOL			parseData(LLXmlTreeNode* node);

	// New Virtual functions
	virtual F32					getTotalDistortion() = 0;
	virtual const LLVector3&	getAvgDistortion() = 0;
	virtual F32					getMaxDistortion() = 0;
	virtual LLVector3			getVertexDistortion(S32 index, LLPolyMesh *mesh) = 0;
	virtual const LLVector3*	getFirstDistortion(U32 *index, LLPolyMesh **mesh) = 0;
	virtual const LLVector3*	getNextDistortion(U32 *index, LLPolyMesh **mesh) = 0;
	
	// interface methods
	F32					getDisplayOrder() const		{ return getInfo()->mEditGroupDisplayOrder; }
	S32					getWearableType() const		{ return getInfo()->mWearableType; }
	const std::string&	getEditGroup() const		{ return getInfo()->mEditGroup; }

	F32					getCameraDistance()	const	{ return getInfo()->mCamDist; } 
	F32					getCameraAngle() const		{ return getInfo()->mCamAngle; }  // degrees
	F32					getCameraElevation() const	{ return getInfo()->mCamElevation; } 
	const std::string&	getCameraTargetName() const { return getInfo()->mCamTargetName; }
	
	BOOL				getShowSimple() const		{ return getInfo()->mShowSimple; }
	F32					getSimpleMin() const		{ return getInfo()->mSimpleMin; }
	F32					getSimpleMax() const		{ return getInfo()->mSimpleMax; }

	BOOL				getCrossWearable() const 	{ return getInfo()->mCrossWearable; }

};

#endif // LL_LLViewerVisualParam_H
