/** 
 * @file llviewervisualparam.h
 * @brief viewer side visual params (with data file parsing)
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
LL_ALIGN_PREFIX(16)
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
	virtual const LLVector4a&	getAvgDistortion() = 0;
	virtual F32					getMaxDistortion() = 0;
	virtual LLVector4a			getVertexDistortion(S32 index, LLPolyMesh *mesh) = 0;
	virtual const LLVector4a*	getFirstDistortion(U32 *index, LLPolyMesh **mesh) = 0;
	virtual const LLVector4a*	getNextDistortion(U32 *index, LLPolyMesh **mesh) = 0;
	
	// interface methods
	F32					getDisplayOrder() const		{ return getInfo()->mEditGroupDisplayOrder; }
	S32					getWearableType() const		{ return getInfo()->mWearableType; }
	const std::string&	getEditGroup() const		{ return getInfo()->mEditGroup; }

	F32					getCameraDistance()	const	{ return getInfo()->mCamDist; } 
	F32					getCameraAngle() const		{ return getInfo()->mCamAngle; }  // degrees
	F32					getCameraElevation() const	{ return getInfo()->mCamElevation; } 
	
	BOOL				getShowSimple() const		{ return getInfo()->mShowSimple; }
	F32					getSimpleMin() const		{ return getInfo()->mSimpleMin; }
	F32					getSimpleMax() const		{ return getInfo()->mSimpleMax; }

	BOOL				getCrossWearable() const 	{ return getInfo()->mCrossWearable; }

} LL_ALIGN_POSTFIX(16);

#endif // LL_LLViewerVisualParam_H
