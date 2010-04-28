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

#include "llinventorytype.h"
#include "llpermissionsflags.h"

class LLFolderViewItem;

class LLInventoryFilter
{
public:
	enum EFolderShow
	{
		SHOW_ALL_FOLDERS,
		SHOW_NON_EMPTY_FOLDERS,
		SHOW_NO_FOLDERS
	};

	enum EFilterBehavior
	{
		FILTER_NONE,				// nothing to do, already filtered
		FILTER_RESTART,				// restart filtering from scratch
		FILTER_LESS_RESTRICTIVE,	// existing filtered items will certainly pass this filter
		FILTER_MORE_RESTRICTIVE		// if you didn't pass the previous filter, you definitely won't pass this one
	};

	enum EFilterType
	{
		FILTERTYPE_NONE = 0,
		FILTERTYPE_OBJECT = 1,		// normal default search-by-object-type
		FILTERTYPE_CATEGORY = 2,	// search by folder type
		FILTERTYPE_UUID	= 4,		// find the object with UUID and any links to it
		FILTERTYPE_DATE = 8			// search by date range
	};

	// REFACTOR: Change this to an enum.
	static const U32 SO_DATE = 1;
	static const U32 SO_FOLDERS_BY_NAME = 2;
	static const U32 SO_SYSTEM_FOLDERS_TO_TOP = 4;

	LLInventoryFilter(const std::string& name);
	virtual ~LLInventoryFilter();

	// +-------------------------------------------------------------------+
	// + Parameters
	// +-------------------------------------------------------------------+
	void 				setFilterObjectTypes(U64 types);
	U32 				getFilterObjectTypes() const;
	BOOL 				isFilterObjectTypesWith(LLInventoryType::EType t) const;
	void 				setFilterCategoryTypes(U64 types);
	void 				setFilterUUID(const LLUUID &object_id);

	void 				setFilterSubString(const std::string& string);
	const std::string& 	getFilterSubString(BOOL trim = FALSE) const;
	const std::string& 	getFilterSubStringOrig() const { return mFilterSubStringOrig; } 
	BOOL 				hasFilterString() const;

	void 				setFilterPermissions(PermissionMask perms);
	PermissionMask 		getFilterPermissions() const;

	void 				setDateRange(time_t min_date, time_t max_date);
	void 				setDateRangeLastLogoff(BOOL sl);
	time_t 				getMinDate() const;
	time_t 				getMaxDate() const;

	void 				setHoursAgo(U32 hours);
	U32 				getHoursAgo() const;

	// +-------------------------------------------------------------------+
	// + Execution And Results
	// +-------------------------------------------------------------------+
	BOOL 				check(const LLFolderViewItem* item);
	BOOL 				checkAgainstFilterType(const LLFolderViewItem* item);
	std::string::size_type getStringMatchOffset() const;

	// +-------------------------------------------------------------------+
	// + Presentation
	// +-------------------------------------------------------------------+
	void 				setShowFolderState( EFolderShow state);
	EFolderShow 		getShowFolderState() const;

	void 				setSortOrder(U32 order);
	U32 				getSortOrder() const;

	void 				setEmptyLookupMessage(const std::string& message);
	const std::string&	getEmptyLookupMessage() const;

	// +-------------------------------------------------------------------+
	// + Status
	// +-------------------------------------------------------------------+
	BOOL 				isActive() const;
	BOOL 				isModified() const;
	BOOL 				isModifiedAndClear();
	BOOL 				isSinceLogoff() const;
	void 				clearModified();
	const std::string& 	getName() const;
	const std::string& 	getFilterText();
	//RN: this is public to allow system to externally force a global refilter
	void 				setModified(EFilterBehavior behavior = FILTER_RESTART);

	// +-------------------------------------------------------------------+
	// + Count
	// +-------------------------------------------------------------------+
	void 				setFilterCount(S32 count);
	S32 				getFilterCount() const;
	void 				decrementFilterCount();

	// +-------------------------------------------------------------------+
	// + Default
	// +-------------------------------------------------------------------+
	BOOL 				isNotDefault() const;
	void 				markDefault();
	void 				resetDefault();

	// +-------------------------------------------------------------------+
	// + Generation
	// +-------------------------------------------------------------------+
	S32 				getCurrentGeneration() const;
	S32 				getMinRequiredGeneration() const;
	S32 				getMustPassGeneration() const;

	// +-------------------------------------------------------------------+
	// + Conversion
	// +-------------------------------------------------------------------+
	void 				toLLSD(LLSD& data) const;
	void 				fromLLSD(LLSD& data);

private:
	struct FilterOps
	{
		FilterOps();
		U32 			mFilterTypes;

		U64				mFilterObjectTypes; // For _OBJECT
		U64				mFilterCategoryTypes; // For _CATEGORY
		LLUUID      	mFilterUUID; // for UUID

		time_t			mMinDate;
		time_t			mMaxDate;
		U32				mHoursAgo;
		EFolderShow		mShowFolderState;
		PermissionMask	mPermissions;
	};

	U32						mOrder;
	U32 					mLastLogoff;

	FilterOps				mFilterOps;
	FilterOps				mDefaultFilterOps;

	std::string::size_type	mSubStringMatchOffset;
	std::string				mFilterSubString;
	std::string				mFilterSubStringOrig;
	const std::string		mName;

	S32						mFilterGeneration;
	S32						mMustPassGeneration;
	S32						mMinRequiredGeneration;
	S32						mNextFilterGeneration;

	S32						mFilterCount;
	EFilterBehavior 		mFilterBehavior;

	BOOL 					mModified;
	BOOL 					mNeedTextRebuild;
	std::string 			mFilterText;
	std::string 			mEmptyLookupMessage;
};

#endif
