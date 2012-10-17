/** 
 * @file lldriverparam.h
 * @brief A visual parameter that drives (controls) other visual parameters.
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

#ifndef LL_LLDRIVERPARAM_H
#define LL_LLDRIVERPARAM_H

#include "llviewervisualparam.h"
#include "llwearabletype.h"

class LLPhysicsMotion;
class LLVOAvatar;
class LLWearable;

//-----------------------------------------------------------------------------

struct LLDrivenEntryInfo
{
	LLDrivenEntryInfo( S32 id, F32 min1, F32 max1, F32 max2, F32 min2 )
		: mDrivenID( id ), mMin1( min1 ), mMax1( max1 ), mMax2( max2 ), mMin2( min2 ) {}
	S32					mDrivenID;
	F32					mMin1;
	F32					mMax1;
	F32					mMax2;
	F32					mMin2;
};

struct LLDrivenEntry
{
	LLDrivenEntry( LLViewerVisualParam* param, LLDrivenEntryInfo *info )
		: mParam( param ), mInfo( info ) {}
	LLViewerVisualParam* mParam;
	LLDrivenEntryInfo*	 mInfo;
};

//-----------------------------------------------------------------------------

class LLDriverParamInfo : public LLViewerVisualParamInfo
{
	friend class LLDriverParam;
public:
	LLDriverParamInfo();
	/*virtual*/ ~LLDriverParamInfo() {};
	
	/*virtual*/ BOOL parseXml(LLXmlTreeNode* node);

	/*virtual*/ void toStream(std::ostream &out);	

protected:
	typedef std::deque<LLDrivenEntryInfo> entry_info_list_t;
	entry_info_list_t mDrivenInfoList;
};

//-----------------------------------------------------------------------------

class LLDriverParam : public LLViewerVisualParam
{
	friend class LLPhysicsMotion; // physics motion needs to access driven params directly.
public:
	LLDriverParam(LLVOAvatar *avatarp);
	LLDriverParam(LLWearable *wearablep);
	~LLDriverParam();

	// Special: These functions are overridden by child classes
	LLDriverParamInfo*		getInfo() const { return (LLDriverParamInfo*)mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLDriverParamInfo *info);

	void					setWearable(LLWearable *wearablep);
	void					setAvatar(LLVOAvatar *avatarp);
	void					updateCrossDrivenParams(LLWearableType::EType driven_type);

	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable) const;

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL				parseData(LLXmlTreeNode* node);
	/*virtual*/ void				apply( ESex sex ) {} // apply is called separately for each driven param.
	/*virtual*/ void				setWeight(F32 weight, BOOL upload_bake);
	/*virtual*/ void				setAnimationTarget( F32 target_value, BOOL upload_bake );
	/*virtual*/ void				stopAnimating(BOOL upload_bake);
	/*virtual*/ BOOL				linkDrivenParams(visual_param_mapper mapper, BOOL only_cross_params);
	/*virtual*/ void				resetDrivenParams();
	
	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion();
	/*virtual*/ const LLVector4a&	getAvgDistortion();
	/*virtual*/ F32					getMaxDistortion();
	/*virtual*/ LLVector4a			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh);
	/*virtual*/ const LLVector4a*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh);
	/*virtual*/ const LLVector4a*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh);

protected:
	F32 getDrivenWeight(const LLDrivenEntry* driven, F32 input_weight);
	void setDrivenWeight(LLDrivenEntry *driven, F32 driven_weight, bool upload_bake);


	LLVector4a	mDefaultVec; // temp holder
	typedef std::vector<LLDrivenEntry> entry_list_t;
	entry_list_t mDriven;
	LLViewerVisualParam* mCurrentDistortionParam;
	// Backlink only; don't make this an LLPointer.
	LLVOAvatar* mAvatarp;
	LLWearable* mWearablep;
};

#endif  // LL_LLDRIVERPARAM_H
