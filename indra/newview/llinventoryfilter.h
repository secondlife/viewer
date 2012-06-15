/** 
* @file llinventoryfilter.h
* @brief Support for filtering your inventory to only display a subset of the
* available items.
*
* $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#ifndef LLINVENTORYFILTER_H
#define LLINVENTORYFILTER_H

#include "llinventorytype.h"
#include "llpermissionsflags.h"

class LLFolderViewItem;
class LLFolderViewFolder;
class LLInventoryItem;

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

	enum EFilterType	{
		FILTERTYPE_NONE = 0,
		FILTERTYPE_OBJECT = 0x1 << 0,	// normal default search-by-object-type
		FILTERTYPE_CATEGORY = 0x1 << 1,	// search by folder type
		FILTERTYPE_UUID	= 0x1 << 2,		// find the object with UUID and any links to it
		FILTERTYPE_DATE = 0x1 << 3,		// search by date range
		FILTERTYPE_WEARABLE = 0x1 << 4,	// search by wearable type
		FILTERTYPE_EMPTYFOLDERS = 0x1 << 5	// pass if folder is not a system folder to be hidden if empty
	};

	enum EFilterLink
	{
		FILTERLINK_INCLUDE_LINKS,	// show links too
		FILTERLINK_EXCLUDE_LINKS,	// don't show links
		FILTERLINK_ONLY_LINKS		// only show links
	};

	enum ESortOrderType
	{
		SO_NAME = 0,						// Sort inventory by name
		SO_DATE = 0x1,						// Sort inventory by date
		SO_FOLDERS_BY_NAME = 0x1 << 1,		// Force folder sort by name
		SO_SYSTEM_FOLDERS_TO_TOP = 0x1 << 2	// Force system folders to be on top
	};

	LLInventoryFilter(const std::string& name);
	virtual ~LLInventoryFilter();

	// +-------------------------------------------------------------------+
	// + Parameters
	// +-------------------------------------------------------------------+
	void 				setFilterObjectTypes(U64 types);
	U64 				getFilterObjectTypes() const;
	U64					getFilterCategoryTypes() const;
	BOOL 				isFilterObjectTypesWith(LLInventoryType::EType t) const;
	void 				setFilterCategoryTypes(U64 types);
	void 				setFilterUUID(const LLUUID &object_id);
	void				setFilterWearableTypes(U64 types);
	void				setFilterEmptySystemFolders();
	void				updateFilterTypes(U64 types, U64& current_types);

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

	void 				setFilterLinks(U64 filter_link);
	U64					getFilterLinks() const;

	// +-------------------------------------------------------------------+
	// + Execution And Results
	// +-------------------------------------------------------------------+
	BOOL 				check(const LLFolderViewItem* item);
	bool				check(const LLInventoryItem* item);
	bool				checkFolder(const LLFolderViewFolder* folder) const;
	bool				checkFolder(const LLUUID& folder_id) const;
	BOOL 				checkAgainstFilterType(const LLFolderViewItem* item) const;
	bool 				checkAgainstFilterType(const LLInventoryItem* item) const;
	BOOL 				checkAgainstPermissions(const LLFolderViewItem* item) const;
	bool 				checkAgainstPermissions(const LLInventoryItem* item) const;
	BOOL 				checkAgainstFilterLinks(const LLFolderViewItem* item) const;
	bool				checkAgainstClipboard(const LLUUID& object_id) const;

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
	BOOL 				isDefault() const;
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
	bool				areDateLimitsSet();

	struct FilterOps
	{
		FilterOps();
		U32 			mFilterTypes;

		U64				mFilterObjectTypes; // For _OBJECT
		U64				mFilterWearableTypes;
		U64				mFilterCategoryTypes; // For _CATEGORY
		LLUUID      	mFilterUUID; // for UUID

		time_t			mMinDate;
		time_t			mMaxDate;
		U32				mHoursAgo;
		EFolderShow		mShowFolderState;
		PermissionMask	mPermissions;
		U64				mFilterLinks;
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
