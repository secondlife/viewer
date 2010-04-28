/**
 * @file llinventoryitemslist.h
 * @brief A list of inventory items represented by LLFlatListView.
 *
 * Class LLInventoryItemsList implements a flat list of inventory items.
 * Class LLPanelInventoryListItem displays inventory item as an element
 * of LLInventoryItemsList.
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

#include "llpanel.h"

// newview
#include "llflatlistview.h"

class LLIconCtrl;
class LLTextBox;
class LLViewerInventoryItem;

class LLPanelInventoryListItem : public LLPanel
{
public:
	static LLPanelInventoryListItem* createItemPanel(const LLViewerInventoryItem* item);

	virtual ~LLPanelInventoryListItem();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setValue(const LLSD& value);

	void updateItem();

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);

protected:
	LLPanelInventoryListItem(const LLViewerInventoryItem* item);

private:
	LLIconCtrl*		mIcon;
	LLTextBox*		mTitle;

	LLUIImagePtr	mItemIcon;
	std::string		mItemName;
	std::string		mHighlightedText;
};

class LLInventoryItemsList : public LLFlatListView
{
public:
	struct Params : public LLInitParam::Block<Params, LLFlatListView::Params>
	{
		Params();
	};

	virtual ~LLInventoryItemsList();

	void refreshList(const LLDynamicArray<LLPointer<LLViewerInventoryItem> > item_array);

	/**
	 * Let list know items need to be refreshed in next draw()
	 */
	void setNeedsRefresh(bool needs_refresh){ mNeedsRefresh = needs_refresh; }

	bool getNeedsRefresh(){ return mNeedsRefresh; }

	/*virtual*/ void draw();

protected:
	friend class LLUICtrlFactory;
	LLInventoryItemsList(const LLInventoryItemsList::Params& p);

	uuid_vec_t& getIDs() { return mIDs; }

	/**
	 * Refreshes list items, adds new items and removes deleted items. 
	 * Called from draw() until all new items are added, ,
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
	void addNewItem(LLViewerInventoryItem* item);

private:
	uuid_vec_t mIDs; // IDs of items that were added in refreshList().
					 // Will be used in refresh() to determine added and removed ids
	bool mNeedsRefresh;
};

#endif //LL_LLINVENTORYITEMSLIST_H
