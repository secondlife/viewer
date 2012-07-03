/** 
 * @file llfolderviewmodelinventory.h
 * @brief view model implementation specific to inventory
 * class definition
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

#ifndef LL_LLFOLDERVIEWMODELINVENTORY_H
#define LL_LLFOLDERVIEWMODELINVENTORY_H

#include "llinventoryfilter.h"

class LLFolderViewModelItemInventory
	:	public LLFolderViewModelItemCommon
{
public:
	LLFolderViewModelItemInventory()
		:	mRootViewModel(NULL)
	{}
	void setRootViewModel(class LLFolderViewModelInventory* root_view_model)
	{
		mRootViewModel = root_view_model;
	}
	virtual const LLUUID& getUUID() const = 0;
	virtual time_t getCreationDate() const = 0;	// UTC seconds
	virtual void setCreationDate(time_t creation_date_utc) = 0;
	virtual PermissionMask getPermissionMask() const = 0;
	virtual LLFolderType::EType getPreferredType() const = 0;
	virtual void showProperties(void) = 0;
	virtual BOOL isItemInTrash( void) const { return FALSE; } // TODO: make   into pure virtual.
	virtual BOOL isUpToDate() const = 0;
	virtual bool hasChildren() const = 0;
	virtual LLInventoryType::EType getInventoryType() const = 0;
	virtual void performAction(LLInventoryModel* model, std::string action)   = 0;
	virtual LLWearableType::EType getWearableType() const = 0;
	virtual EInventorySortGroup getSortGroup() const = 0;
	virtual LLInventoryObject* getInventoryObject() const = 0;
	virtual void requestSort();
	virtual bool potentiallyVisible();
	virtual bool passedFilter(S32 filter_generation = -1);
	virtual bool descendantsPassedFilter(S32 filter_generation = -1);
	virtual void setPassedFilter(bool filtered, bool filtered_folder, S32 filter_generation);
	virtual bool filter( LLFolderViewFilter& filter);
	virtual bool filterChildItem( LLFolderViewModelItem* item, LLFolderViewFilter& filter);
protected:
	class LLFolderViewModelInventory* mRootViewModel;
};

class LLInventorySort
{
public:
	LLInventorySort(U32 order = 0)
		:	mSortOrder(order),
		mByDate(false),
		mSystemToTop(false),
		mFoldersByName(false)
	{
		mByDate = (order & LLInventoryFilter::SO_DATE);
		mSystemToTop = (order & LLInventoryFilter::SO_SYSTEM_FOLDERS_TO_TOP);
		mFoldersByName = (order & LLInventoryFilter::SO_FOLDERS_BY_NAME);
	}

	bool isByDate() const { return mByDate; }
	U32 getSortOrder() const { return mSortOrder; }

	bool operator()(const LLFolderViewModelItemInventory* const& a, const LLFolderViewModelItemInventory* const& b) const;
private:
	U32  mSortOrder;
	bool mByDate;
	bool mSystemToTop;
	bool mFoldersByName;
};

class LLFolderViewModelInventory
	:	public LLFolderViewModel<LLInventorySort,   LLFolderViewModelItemInventory, LLFolderViewModelItemInventory,   LLInventoryFilter>
{
public:
	typedef LLFolderViewModel<LLInventorySort,   LLFolderViewModelItemInventory, LLFolderViewModelItemInventory,   LLInventoryFilter> base_t;

	virtual ~LLFolderViewModelInventory() {}

	void sort(LLFolderViewFolder* folder);

	bool contentsReady();

};
#endif // LL_LLFOLDERVIEWMODELINVENTORY_H
