/**
 * @file llinventoryitemslist.h
 * @brief A list of inventory items represented by LLFlatListView.
 *
 * Class LLInventoryItemsList implements a flat list of inventory items.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLINVENTORYITEMSLIST_H
#define LL_LLINVENTORYITEMSLIST_H

#include "lldarray.h"

// newview
#include "llflatlistview.h"

class LLViewerInventoryItem;

class LLInventoryItemsList : public LLFlatListViewEx
{
public:
	struct Params : public LLInitParam::Block<Params, LLFlatListViewEx::Params>
	{
		Params();
	};

	virtual ~LLInventoryItemsList();

	void refreshList(const LLDynamicArray<LLPointer<LLViewerInventoryItem> > item_array);

	boost::signals2::connection setRefreshCompleteCallback(const commit_signal_t::slot_type& cb);

	/**
	 * Let list know items need to be refreshed in next doIdle()
	 */
	void setNeedsRefresh(bool needs_refresh){ mNeedsRefresh = needs_refresh; }

	bool getNeedsRefresh(){ return mNeedsRefresh; }

	/**
	 * Sets the flag indicating that the list needs to be refreshed even if it is
	 * not currently visible.
	 */
	void setForceRefresh(bool force_refresh){ mForceRefresh = force_refresh; }

	virtual bool selectItemByValue(const LLSD& value, bool select = true);

	void updateSelection();

	/**
	 * Idle routine used to refresh the list regardless of the current list
	 * visibility, unlike draw() which is called only for the visible list.
	 * This is needed for example to filter items of the list hidden by closed
	 * accordion tab.
	 */
	void	doIdle();						// Real idle routine
	static void idle(void* user_data);		// static glue to doIdle()

protected:
	friend class LLUICtrlFactory;
	LLInventoryItemsList(const LLInventoryItemsList::Params& p);

	uuid_vec_t& getIDs() { return mIDs; }

	/**
	 * Refreshes list items, adds new items and removes deleted items. 
	 * Called from doIdle() until all new items are added,
	 * maximum 50 items can be added during single call.
	 */
	void refresh();

	/**
	 * Compute difference between new items and current items, fills 'vadded' with added items,
	 * 'vremoved' with removed items. See LLCommonUtils::computeDifference
	 */
	void computeDifference(const uuid_vec_t& vnew, uuid_vec_t& vadded, uuid_vec_t& vremoved);

	/**
	 * Add an item to the list
	 */
	virtual void addNewItem(LLViewerInventoryItem* item, bool rearrange = true);

private:
	uuid_vec_t mIDs; // IDs of items that were added in refreshList().
					 // Will be used in refresh() to determine added and removed ids

	uuid_vec_t mSelectTheseIDs; // IDs that will be selected if list is not loaded till now

	bool mNeedsRefresh;

	bool mForceRefresh;

	commit_signal_t mRefreshCompleteSignal;
};

#endif //LL_LLINVENTORYITEMSLIST_H
