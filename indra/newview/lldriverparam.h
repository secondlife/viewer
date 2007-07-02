/** 
 * @file lldriverparam.h
 * @brief A visual parameter that drives (controls) other visual parameters.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRIVERPARAM_H
#define LL_LLDRIVERPARAM_H

#include "llvoavatar.h"
#include "llviewervisualparam.h"

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

protected:
	typedef std::deque<LLDrivenEntryInfo> entry_info_list_t;
	entry_info_list_t mDrivenInfoList;
};

//-----------------------------------------------------------------------------

class LLDriverParam : public LLViewerVisualParam
{
public:
	LLDriverParam(LLVOAvatar *avatarp);
	~LLDriverParam();

	// Special: These functions are overridden by child classes
	LLDriverParamInfo*		getInfo() const { return (LLDriverParamInfo*)mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLDriverParamInfo *info);

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL				parseData(LLXmlTreeNode* node);
	/*virtual*/ void				apply( ESex sex ) {} // apply is called separately for each driven param.
	/*virtual*/ void				setWeight(F32 weight, BOOL set_by_user);
	/*virtual*/ void				setAnimationTarget( F32 target_value, BOOL set_by_user );
	/*virtual*/ void				stopAnimating(BOOL set_by_user);
	
	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion();
	/*virtual*/ const LLVector3&	getAvgDistortion();
	/*virtual*/ F32					getMaxDistortion();
	/*virtual*/ LLVector3			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh);
	/*virtual*/ const LLVector3*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh);
	/*virtual*/ const LLVector3*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh);
protected:
	F32 getDrivenWeight(const LLDrivenEntry* driven, F32 input_weight);


	LLVector3	mDefaultVec; // temp holder
	typedef std::vector<LLDrivenEntry> entry_list_t;
	entry_list_t mDriven;
	LLViewerVisualParam* mCurrentDistortionParam;
	// Backlink only; don't make this an LLPointer.
	LLVOAvatar* mAvatarp;
};

#endif  // LL_LLDRIVERPARAM_H
