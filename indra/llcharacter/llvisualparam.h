/** 
 * @file llvisualparam.h
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

#ifndef LL_LLVisualParam_H
#define LL_LLVisualParam_H

#include "v3math.h"
#include "llstring.h"
#include "llxmltree.h"

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
	
protected:
	S32					mID;				// ID associated with VisualParam
	
	LLString			mName;				// name (for internal purposes)
	LLString			mDisplayName;		// name displayed to the user
	LLString			mMinName;			// name associated with minimum value
	LLString			mMaxName;			// name associated with maximum value
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
class LLVisualParam
{
public:
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
	virtual void			setWeight(F32 weight, BOOL set_by_user);
	virtual void			setAnimationTarget( F32 target_value, BOOL set_by_user );
	virtual void			animate(F32 delta, BOOL set_by_user);
	virtual void			stopAnimating(BOOL set_by_user);

	// Interface methods
	S32						getID() 			{ return mID; }
	void					setID(S32 id) 		{ llassert(!mInfo); mID = id; }
	
	const LLString&			getName() const 			{ return mInfo->mName; }
	const LLString&			getDisplayName() const 		{ return mInfo->mDisplayName; }
	const LLString&			getMaxDisplayName() const	{ return mInfo->mMaxName; }
	const LLString&			getMinDisplayName() const	{ return mInfo->mMinName; }

	void					setDisplayName(const LLString& s) 	 { mInfo->mDisplayName = s; }
	void					setMaxDisplayName(const LLString& s) { mInfo->mMaxName = s; }
	void					setMinDisplayName(const LLString& s) { mInfo->mMinName = s; }

	EVisualParamGroup		getGroup() 			{ return mInfo->mGroup; }
	F32						getMinWeight() 		{ return mInfo->mMinWeight; }
	F32						getMaxWeight() 		{ return mInfo->mMaxWeight; }
	F32						getDefaultWeight() 	{ return mInfo->mDefaultWeight; }
	ESex					getSex()			{ return mInfo->mSex; }

	F32						getWeight() 		{ return mIsAnimating ? mTargetWeight : mCurWeight; }
	F32						getCurrentWeight() 	{ return mCurWeight; }
	F32						getLastWeight() 	{ return mLastWeight; }
	BOOL					isAnimating() 	{ return mIsAnimating; }

	LLVisualParam*			getNextParam()		{ return mNext; }
	void					setNextParam( LLVisualParam *next );
	
	virtual void			setAnimating(BOOL is_animating) { mIsAnimating = is_animating; }
	BOOL					getAnimating() { return mIsAnimating; }

protected:
	F32					mCurWeight;			// current weight
	F32					mLastWeight;		// last weight
	LLVisualParam*		mNext;				// next param in a shared chain
	F32					mTargetWeight;		// interpolation target
	BOOL				mIsAnimating;	// this value has been given an interpolation target

	S32					mID;				// id for storing weight/morphtarget compares compactly
	LLVisualParamInfo	*mInfo;
};

#endif // LL_LLVisualParam_H
