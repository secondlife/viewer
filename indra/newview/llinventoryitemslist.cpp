/**
 * @file llinventoryitemslist.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llinventoryitemslist.h"

// llcommon
#include "llcommonutils.h"

#include "lliconctrl.h"

#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lltextutil.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// static
LLPanelInventoryListItem* LLPanelInventoryListItem::createItemPanel(const LLViewerInventoryItem* item)
{
	if (item)
	{
		return new LLPanelInventoryListItem(item);
	}
	else
	{
		return NULL;
	}
}

LLPanelInventoryListItem::~LLPanelInventoryListItem()
{}

//virtual
BOOL LLPanelInventoryListItem::postBuild()
{
	mIcon = getChild<LLIconCtrl>("item_icon");
	mTitle = getChild<LLTextBox>("item_name");

	updateItem();

	return TRUE;
}

//virtual
void LLPanelInventoryListItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;
	if (!value.has("selected")) return;
	childSetVisible("selected_icon", value["selected"]);
}

void LLPanelInventoryListItem::updateItem()
{
	if (mItemIcon.notNull())
		mIcon->setImage(mItemIcon);

	LLTextUtil::textboxSetHighlightedVal(
		mTitle,
		LLStyle::Params(),
		mItemName,
		mHighlightedText);
}

void LLPanelInventoryListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLPanelInventoryListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", false);

	LLPanel::onMouseLeave(x, y, mask);
}

LLPanelInventoryListItem::LLPanelInventoryListItem(const LLViewerInventoryItem* item)
:	 LLPanel()
	,mIcon(NULL)
	,mTitle(NULL)
{
	mItemName = item->getName();
	mItemIcon = get_item_icon(item->getType(), item->getInventoryType(), item->getFlags(), FALSE);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_inventory_item.xml");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

LLInventoryItemsList::Params::Params()
{}

LLInventoryItemsList::LLInventoryItemsList(const LLInventoryItemsList::Params& p)
:	LLFlatListView(p)
,	mNeedsRefresh(false)
{
	// TODO: mCommitOnSelectionChange is set to "false" in LLFlatListView
	// but reset to true in all derived classes. This settings might need to
	// be added to LLFlatListView::Params() and/or set to "true" by default.
	setCommitOnSelectionChange(true);
}

// virtual
LLInventoryItemsList::~LLInventoryItemsList()
{}

void LLInventoryItemsList::refreshList(const LLInventoryModel::item_array_t item_array)
{
	getIDs().clear();
	LLInventoryModel::item_array_t::const_iterator it = item_array.begin();
	for( ; item_array.end() != it; ++it)
	{
		getIDs().push_back((*it)->getUUID());
	}
	mNeedsRefresh = true;
}

void LLInventoryItemsList::draw()
{
	LLFlatListView::draw();
	if(mNeedsRefresh)
	{
		refresh();
	}
}

void LLInventoryItemsList::refresh()
{
	static const unsigned ADD_LIMIT = 50;

	uuid_vec_t added_items;
	uuid_vec_t removed_items;

	computeDifference(getIDs(), added_items, removed_items);

	bool add_limit_exceeded = false;
	unsigned nadded = 0;

	uuid_vec_t::const_iterator it = added_items.begin();
	for( ; added_items.end() != it; ++it)
	{
		if(nadded >= ADD_LIMIT)
		{
			add_limit_exceeded = true;
			break;
		}
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		addNewItem(item);
		++nadded;
	}

	it = removed_items.begin();
	for( ; removed_items.end() != it; ++it)
	{
		removeItemByUUID(*it);
	}

	bool needs_refresh = add_limit_exceeded;
	setNeedsRefresh(needs_refresh);
}

void LLInventoryItemsList::computeDifference(
	const uuid_vec_t& vnew,
	uuid_vec_t& vadded,
	uuid_vec_t& vremoved)
{
	uuid_vec_t vcur;
	{
		std::vector<LLSD> vcur_values;
		getValues(vcur_values);

		for (size_t i=0; i<vcur_values.size(); i++)
			vcur.push_back(vcur_values[i].asUUID());
	}

	LLCommonUtils::computeDifference(vnew, vcur, vadded, vremoved);
}

void LLInventoryItemsList::addNewItem(LLViewerInventoryItem* item)
{
	if (!item)
	{
		llwarns << "No inventory item. Couldn't create flat list item." << llendl;
		llassert(!"No inventory item. Couldn't create flat list item.");
	}

	LLPanelInventoryListItem *list_item = LLPanelInventoryListItem::createItemPanel(item);
	if (!list_item)
		return;

	if (!addItem(list_item, item->getUUID()))
	{
		llwarns << "Couldn't add flat list item." << llendl;
		llassert(!"Couldn't add flat list item.");
	}
}

// EOF
