/** 
* @file llinventoryfilter.h
* @brief Support for filtering your inventory to only display a subset of the
* available items.
*
* $LicenseInfo:firstyear=2005&license=viewergpl$
* 
* Copyright (c) 2005-2009, Linden Research, Inc.
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
#ifndef LLINVENTORYFILTER_H
#define LLINVENTORYFILTER_H

// lots of includes here
#include "llinventorytype.h"
#include "llpermissionsflags.h"		// PermissionsMask

class LLFolderViewItem;

class LLInventoryFilter
{
public:
	typedef enum e_folder_show
	{
		SHOW_ALL_FOLDERS,
		SHOW_NON_EMPTY_FOLDERS,
		SHOW_NO_FOLDERS
	} EFolderShow;

	typedef enum e_filter_behavior
	{
		FILTER_NONE,				// nothing to do, already filtered
		FILTER_RESTART,				// restart filtering from scratch
		FILTER_LESS_RESTRICTIVE,	// existing filtered items will certainly pass this filter
		FILTER_MORE_RESTRICTIVE		// if you didn't pass the previous filter, you definitely won't pass this one
	} EFilterBehavior;

	static const U32 SO_DATE = 1;
	static const U32 SO_FOLDERS_BY_NAME = 2;
	static const U32 SO_SYSTEM_FOLDERS_TO_TOP = 4;

	LLInventoryFilter(const std::string& name);
	virtual ~LLInventoryFilter();

	void setFilterTypes(U64 types, BOOL filter_for_categories = FALSE); // if filter_for_categories is true, operate on folder preferred asset type
	U32 getFilterTypes() const { return mFilterOps.mFilterTypes; }

	void setFilterSubString(const std::string& string);
	const std::string getFilterSubString(BOOL trim = FALSE);

	void setFilterPermissions(PermissionMask perms);
	PermissionMask getFilterPermissions() const { return mFilterOps.mPermissions; }

	void setDateRange(time_t min_date, time_t max_date);
	void setDateRangeLastLogoff(BOOL sl);
	time_t getMinDate() const { return mFilterOps.mMinDate; }
	time_t getMaxDate() const { return mFilterOps.mMaxDate; }

	void setHoursAgo(U32 hours);
	U32 getHoursAgo() const { return mFilterOps.mHoursAgo; }

	void setShowFolderState( EFolderShow state);
	EFolderShow getShowFolderState() { return mFilterOps.mShowFolderState; }

	void setSortOrder(U32 order);
	U32 getSortOrder() { return mOrder; }

	BOOL check(LLFolderViewItem* item);
	std::string::size_type getStringMatchOffset() const;
	BOOL isActive();
	BOOL isNotDefault();
	BOOL isModified();
	BOOL isModifiedAndClear();
	BOOL isSinceLogoff();
	bool hasFilterString() { return mFilterSubString.size() > 0; }
	void clearModified() { mModified = FALSE; mFilterBehavior = FILTER_NONE; }
	const std::string getName() const { return mName; }
	std::string getFilterText();

	void setFilterCount(S32 count) { mFilterCount = count; }
	S32 getFilterCount() { return mFilterCount; }
	void decrementFilterCount() { mFilterCount--; }

	void markDefault();
	void resetDefault();

	BOOL isFilterWith(LLInventoryType::EType t);

	S32 getCurrentGeneration() const { return mFilterGeneration; }
	S32 getMinRequiredGeneration() const { return mMinRequiredGeneration; }
	S32 getMustPassGeneration() const { return mMustPassGeneration; }

	//RN: this is public to allow system to externally force a global refilter
	void setModified(EFilterBehavior behavior = FILTER_RESTART);

	void toLLSD(LLSD& data);
	void fromLLSD(LLSD& data);

protected:
	struct filter_ops
	{
		U64			mFilterTypes;
		BOOL        mFilterForCategories;
		time_t		mMinDate;
		time_t		mMaxDate;
		U32			mHoursAgo;
		EFolderShow	mShowFolderState;
		PermissionMask	mPermissions;
	};
	filter_ops		mFilterOps;
	filter_ops		mDefaultFilterOps;
	std::string::size_type	mSubStringMatchOffset;
	std::string		mFilterSubString;
	U32				mOrder;
	const std::string	mName;
	S32				mFilterGeneration;
	S32				mMustPassGeneration;
	S32				mMinRequiredGeneration;
	S32				mFilterCount;
	S32				mNextFilterGeneration;
	EFilterBehavior mFilterBehavior;

private:
	U32 mLastLogoff;
	BOOL mModified;
	BOOL mNeedTextRebuild;
	std::string mFilterText;
};

#endif
