/** 
 * @file llvisualparam.h
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

#ifndef LL_LLVisualParam_H
#define LL_LLVisualParam_H

#include "v3math.h"
#include "llstring.h"
#include "llxmltree.h"
#include <boost/function.hpp>

class LLPolyMesh;
class LLXmlTreeNode;

enum ESex
{
	SEX_FEMALE =	0x01,
	SEX_MALE =		0x02,
	SEX_BOTH =		0x03  // values chosen to allow use as a bit field.
};

enum EVisualParamGroup
{
	VISUAL_PARAM_GROUP_TWEAKABLE,
	VISUAL_PARAM_GROUP_ANIMATABLE,
	VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT,
	NUM_VISUAL_PARAM_GROUPS
};

const S32 MAX_TRANSMITTED_VISUAL_PARAMS = 255;

//-----------------------------------------------------------------------------
// LLVisualParamInfo
// Contains shared data for VisualParams
//-----------------------------------------------------------------------------
class LLVisualParamInfo
{
	friend class LLVisualParam;
public:
	LLVisualParamInfo();
	virtual ~LLVisualParamInfo() {};

	virtual BOOL parseXml(LLXmlTreeNode *node);

	S32 getID() const { return mID; }

	virtual void toStream(std::ostream &out);
	
protected:
	S32					mID;				// ID associated with VisualParam
	
	std::string			mName;				// name (for internal purposes)
	std::string			mDisplayName;		// name displayed to the user
	std::string			mMinName;			// name associated with minimum value
	std::string			mMaxName;			// name associated with maximum value
	EVisualParamGroup	mGroup;				// morph group for separating UI controls
	F32					mMinWeight;			// minimum weight that can be assigned to this morph target
	F32					mMaxWeight;			// maximum weight that can be assigned to this morph target
	F32					mDefaultWeight;		
	ESex				mSex;				// Which gender(s) this param applies to.
};

//-----------------------------------------------------------------------------
// LLVisualParam
// VIRTUAL CLASS
// An interface class for a generalized parametric modification of the avatar mesh
// Contains data that is specific to each Avatar
//-----------------------------------------------------------------------------
LL_ALIGN_PREFIX(16)
class LLVisualParam
{
public:
	typedef	boost::function<LLVisualParam*(S32)> visual_param_mapper;

	LLVisualParam();
	virtual ~LLVisualParam();

	// Special: These functions are overridden by child classes
	// (They can not be virtual because they use specific derived Info classes)
	LLVisualParamInfo*		getInfo() const { return mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL					setInfo(LLVisualParamInfo *info);

	// Virtual functions
	//  Pure virtuals
	//virtual BOOL			parseData( LLXmlTreeNode *node ) = 0;
	virtual void			apply( ESex avatar_sex ) = 0;
	//  Default functions
	virtual void			setWeight(F32 weight, BOOL upload_bake);
	virtual void			setAnimationTarget( F32 target_value, BOOL upload_bake );
	virtual void			animate(F32 delta, BOOL upload_bake);
	virtual void			stopAnimating(BOOL upload_bake);

	virtual BOOL			linkDrivenParams(visual_param_mapper mapper, BOOL only_cross_params);
	virtual void			resetDrivenParams();

	// Interface methods
	S32						getID() const		{ return mID; }
	void					setID(S32 id) 		{ llassert(!mInfo); mID = id; }
	
	const std::string&		getName() const 			{ return mInfo->mName; }
	const std::string&		getDisplayName() const 		{ return mInfo->mDisplayName; }
	const std::string&		getMaxDisplayName() const	{ return mInfo->mMaxName; }
	const std::string&		getMinDisplayName() const	{ return mInfo->mMinName; }

	void					setDisplayName(const std::string& s) 	 { mInfo->mDisplayName = s; }
	void					setMaxDisplayName(const std::string& s) { mInfo->mMaxName = s; }
	void					setMinDisplayName(const std::string& s) { mInfo->mMinName = s; }

	EVisualParamGroup		getGroup() const 			{ return mInfo->mGroup; }
	F32						getMinWeight() const		{ return mInfo->mMinWeight; }
	F32						getMaxWeight() const		{ return mInfo->mMaxWeight; }
	F32						getDefaultWeight() const 	{ return mInfo->mDefaultWeight; }
	ESex					getSex() const			{ return mInfo->mSex; }

	F32						getWeight() const		{ return mIsAnimating ? mTargetWeight : mCurWeight; }
	F32						getCurrentWeight() const 	{ return mCurWeight; }
	F32						getLastWeight() const	{ return mLastWeight; }
	BOOL					isAnimating() const	{ return mIsAnimating; }
	BOOL					isTweakable() const { return (getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)  || (getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT); }

	LLVisualParam*			getNextParam()		{ return mNext; }
	void					setNextParam( LLVisualParam *next );
	
	virtual void			setAnimating(BOOL is_animating) { mIsAnimating = is_animating && !mIsDummy; }
	BOOL					getAnimating() const { return mIsAnimating; }

	void					setIsDummy(BOOL is_dummy) { mIsDummy = is_dummy; }

protected:
	F32					mCurWeight;			// current weight
	F32					mLastWeight;		// last weight
	LLVisualParam*		mNext;				// next param in a shared chain
	F32					mTargetWeight;		// interpolation target
	BOOL				mIsAnimating;	// this value has been given an interpolation target
	BOOL				mIsDummy;  // this is used to prevent dummy visual params from animating


	S32					mID;				// id for storing weight/morphtarget compares compactly
	LLVisualParamInfo	*mInfo;
} LL_ALIGN_POSTFIX(16);

#endif // LL_LLVisualParam_H
