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
{}

// virtual
LLInventoryItemsList::~LLInventoryItemsList()
{}

void LLInventoryItemsList::refreshList(const LLInventoryModel::item_array_t item_array)
{
	clear();

	for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		LLViewerInventoryItem *item = (*iter);

		LLPanelInventoryItem *list_item = new LLPanelInventoryItem(item->getType(),
																   item->getInventoryType(),
																   item->getFlags(),
																   item->getName(),
																   LLStringUtil::null);
		if (!addItem(list_item, item->getUUID()))
		{
			llerrs << "Couldn't add flat item." << llendl;
		}
	}
}
