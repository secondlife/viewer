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

/**
 * @class LLPanelInventoryListItemBase
 *
 * Base class for Inventory flat list item. Panel consists of inventory icon
 * and inventory item name.
 * This class is able to display widgets(buttons) on left(before icon) and right(after text-box) sides 
 * of panel.
 *
 * How to use (see LLPanelClothingListItem for example):
 * - implement init() to build panel from xml
 * - create new xml file, fill it with widgets you want to dynamically show/hide/reshape on left/right sides
 * - redefine postBuild()(call base implementation) and add needed widgets to needed sides,
 *
 */
class LLPanelInventoryListItemBase : public LLPanel
{
public:

	static LLPanelInventoryListItemBase* create(LLViewerInventoryItem* item);

	/**
	 * Called after inventory item was updated, update panel widgets to reflect inventory changes.
	 */
	virtual void updateItem();

	/**
	 * Add widget to left side
	 */
	void addWidgetToLeftSide(const std::string& name, bool show_widget = true);
	void addWidgetToLeftSide(LLUICtrl* ctrl, bool show_widget = true);

	/**
	 * Add widget to right side, widget is supposed to be child of calling panel
	 */
	void addWidgetToRightSide(const std::string& name, bool show_widget = true);
	void addWidgetToRightSide(LLUICtrl* ctrl, bool show_widget = true);

	/**
	 * Mark widgets as visible. Only visible widgets take part in reshaping children
	 */
	void setShowWidget(const std::string& name, bool show);
	void setShowWidget(LLUICtrl* ctrl, bool show);

	/**
	 * Set spacing between widgets during reshape
	 */
	void setWidgetSpacing(S32 spacing) { mWidgetSpacing = spacing; }

	S32 getWidgetSpacing() { return mWidgetSpacing; }

	/**
	 * Inheritors need to call base implementation of postBuild()
	 */
	/*virtual*/ BOOL postBuild();

	/**
	 * Handles item selection
	 */
	/*virtual*/ void setValue(const LLSD& value);

	 /* Highlights item */
	/*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);
	/* Removes item highlight */
	/*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);

	virtual ~LLPanelInventoryListItemBase(){}

protected:

	LLPanelInventoryListItemBase(LLViewerInventoryItem* item);

	typedef std::vector<LLUICtrl*> widget_array_t;

	/**
	 * Use it from a factory function to build panel, do not build panel in constructor
	 */
	virtual void init();

	void setLeftWidgetsWidth(S32 width) { mLeftWidgetsWidth = width; }
	void setRightWidgetsWidth(S32 width) { mRightWidgetsWidth = width; }

	/**
	 * Set all widgets from both side visible/invisible. Only enabled widgets
	 * (see setShowWidget()) can become visible
	 */
	virtual void setWidgetsVisible(bool visible);

	/**
	 * Reshape all child widgets - icon, text-box and side widgets
	 */
	virtual void reshapeWidgets();

private:

	/** reshape left side widgets
	 * Deprecated for now. Disabled reshape left for now to reserve space for 'delete' 
	 * button in LLPanelClothingListItem according to Neal's comment (https://codereview.productengine.com/secondlife/r/325/)
	 */
	void reshapeLeftWidgets();

	/** reshape right side widgets */
	void reshapeRightWidgets();

	/** reshape remaining widgets */
	void reshapeMiddleWidgets();

	LLViewerInventoryItem* mItem;

	LLIconCtrl*		mIcon;
	LLTextBox*		mTitle;

	LLUIImagePtr	mItemIcon;
	std::string		mHighlightedText;

	widget_array_t	mLeftSideWidgets;
	widget_array_t	mRightSideWidgets;
	S32				mWidgetSpacing;

	S32				mLeftWidgetsWidth;
	S32				mRightWidgetsWidth;
};

//////////////////////////////////////////////////////////////////////////

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
	virtual void addNewItem(LLViewerInventoryItem* item);

private:
	uuid_vec_t mIDs; // IDs of items that were added in refreshList().
					 // Will be used in refresh() to determine added and removed ids
	bool mNeedsRefresh;
};

#endif //LL_LLINVENTORYITEMSLIST_H
