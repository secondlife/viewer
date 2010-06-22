/**
 * @file llinventoryitemslist.h
 * @brief A list of inventory items represented by LLFlatListView.
 *
 * Class LLInventoryItemsList implements a flat list of inventory items.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
	bool mNeedsRefresh;

	bool mForceRefresh;

	commit_signal_t mRefreshCompleteSignal;
};

#endif //LL_LLINVENTORYITEMSLIST_H
