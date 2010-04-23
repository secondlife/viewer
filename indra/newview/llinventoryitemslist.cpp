/**
 * @file llinventoryitemslist.cpp
 * @brief A list of inventory items represented by LLFlatListView.
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
#include "lltextutil.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

LLPanelInventoryItem::LLPanelInventoryItem(LLAssetType::EType asset_type,
										   LLInventoryType::EType inventory_type,
										   U32 wearable_type,
										   const std::string &item_name,
										   const std::string &hl)
:	 LLPanel()
	,mItemName(item_name)
	,mHighlightedText(hl)
	,mIcon(NULL)
	,mTitle(NULL)
{
	mItemIcon = get_item_icon(asset_type, inventory_type, wearable_type, FALSE);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_inventory_item.xml");
}

LLPanelInventoryItem::~LLPanelInventoryItem()
{}

//virtual
BOOL LLPanelInventoryItem::postBuild()
{
	mIcon = getChild<LLIconCtrl>("item_icon");
	mTitle = getChild<LLTextBox>("item_name");

	updateItem();

	return TRUE;
}

//virtual
void LLPanelInventoryItem::setValue(const LLSD& value)
{
	if (!value.isMap()) return;
	if (!value.has("selected")) return;
	childSetVisible("selected_icon", value["selected"]);
}

void LLPanelInventoryItem::updateItem()
{
	if (mItemIcon.notNull())
		mIcon->setImage(mItemIcon);

	LLTextUtil::textboxSetHighlightedVal(
		mTitle,
		LLStyle::Params(),
		mItemName,
		mHighlightedText);
}

void LLPanelInventoryItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLPanelInventoryItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", false);

	LLPanel::onMouseLeave(x, y, mask);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

LLInventoryItemsList::LLInventoryItemsList(const LLFlatListView::Params& p)
:	LLFlatListView(p)
,	mNeedsRefresh(false)
{}

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
	llassert(item);

	LLPanelInventoryItem *list_item = new LLPanelInventoryItem(item->getType(),
		item->getInventoryType(), item->getFlags(), item->getName(), LLStringUtil::null);

	if (!addItem(list_item, item->getUUID()))
	{
		llwarns << "Couldn't add flat list item." << llendl;
		llassert(!"Couldn't add flat list item.");
	}
}

// EOF
