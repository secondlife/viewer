/**
 * @file llinventoryitemslist.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llinventoryitemslist.h"

// llcommon
#include "llcommonutils.h"

#include "lltrans.h"

#include "llcallbacklist.h"
#include "llinventorylistitem.h"
#include "llinventorymodel.h"
#include "llviewerinventory.h"

LLInventoryItemsList::Params::Params()
{}

LLInventoryItemsList::LLInventoryItemsList(const LLInventoryItemsList::Params& p)
:	LLFlatListViewEx(p)
,	mNeedsRefresh(false)
,	mForceRefresh(false)
{
	// TODO: mCommitOnSelectionChange is set to "false" in LLFlatListView
	// but reset to true in all derived classes. This settings might need to
	// be added to LLFlatListView::Params() and/or set to "true" by default.
	setCommitOnSelectionChange(true);

	setNoFilteredItemsMsg(LLTrans::getString("InventoryNoMatchingItems"));

	gIdleCallbacks.addFunction(idle, this);
}

// virtual
LLInventoryItemsList::~LLInventoryItemsList()
{
	gIdleCallbacks.deleteFunction(idle, this);
}

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

boost::signals2::connection LLInventoryItemsList::setRefreshCompleteCallback(const commit_signal_t::slot_type& cb)
{
	return mRefreshCompleteSignal.connect(cb);
}

void LLInventoryItemsList::doIdle()
{
	if (!mNeedsRefresh) return;

	if (isInVisibleChain() || mForceRefresh)
	{
		refresh();

		mRefreshCompleteSignal(this, LLSD());
	}
}

//static
void LLInventoryItemsList::idle(void* user_data)
{
	LLInventoryItemsList* self = static_cast<LLInventoryItemsList*>(user_data);
	if ( self )
	{	// Do the real idle
		self->doIdle();
	}
}

LLFastTimer::DeclareTimer FTM_INVENTORY_ITEMS_REFRESH("Inventory List Refresh");

void LLInventoryItemsList::refresh()
{
	LLFastTimer _(FTM_INVENTORY_ITEMS_REFRESH);
	static const unsigned ADD_LIMIT = 20;

	uuid_vec_t added_items;
	uuid_vec_t removed_items;

	computeDifference(getIDs(), added_items, removed_items);

	bool add_limit_exceeded = false;
	unsigned int nadded = 0;

	uuid_vec_t::const_iterator it = added_items.begin();
	for( ; added_items.end() != it; ++it)
	{
		if(nadded >= ADD_LIMIT)
		{
			add_limit_exceeded = true;
			break;
		}
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		// Do not rearrange items on each adding, let's do that on filter call
		llassert(item);
		if (item)
		{
			addNewItem(item, false);
			++nadded;
		}
	}

	it = removed_items.begin();
	for( ; removed_items.end() != it; ++it)
	{
		// don't filter items right away
		removeItemByUUID(*it, false);
	}

	// Filter, rearrange and notify parent about shape changes
	filterItems();

	bool needs_refresh = add_limit_exceeded;
	setNeedsRefresh(needs_refresh);
	setForceRefresh(needs_refresh);
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

void LLInventoryItemsList::addNewItem(LLViewerInventoryItem* item, bool rearrange /*= true*/)
{
	if (!item)
	{
		llwarns << "No inventory item. Couldn't create flat list item." << llendl;
		llassert(item != NULL);
	}

	LLPanelInventoryListItemBase *list_item = LLPanelInventoryListItemBase::create(item);
	if (!list_item)
		return;

	bool is_item_added = addItem(list_item, item->getUUID(), ADD_BOTTOM, rearrange);
	if (!is_item_added)
	{
		llwarns << "Couldn't add flat list item." << llendl;
		llassert(is_item_added);
	}
}

// EOF
