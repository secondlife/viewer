/** 
 * @file llplacesinventorybridge.cpp
 * @brief Implementation of the Inventory-Folder-View-Bridge classes for Places Panel.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llmenugl.h"

#include "llplacesinventorybridge.h"

#include "llfloaterinventory.h" // for LLInventoryPanel
#include "llfolderview.h" // for FIRST_SELECTED_ITEM
#include "llinventorypanel.h"


static const std::string LANDMARKS_INVENTORY_LIST_NAME("landmarks_list");

bool is_landmarks_panel(const LLInventoryPanel* inv_panel)
{
	if (NULL == inv_panel)
		return false;
	return inv_panel->getName() == LANDMARKS_INVENTORY_LIST_NAME;
}

void fill_items_with_menu_items(std::vector<std::string>& items, LLMenuGL& menu)
{
	LLView::child_list_const_iter_t itor;
	for (itor = menu.beginChild(); itor != menu.endChild(); ++itor)
	{
		std::string name = (*itor)->getName();
		items.push_back(name);
	}
}

// virtual
void LLPlacesLandmarkBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(isItemInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		fill_items_with_menu_items(items, menu);

		// Disabled items are processed via LLLandmarksPanel::isActionEnabled()
		// they should be synchronized with Places/My Landmarks/Gear menu. See EXT-1601 
	}

	hide_context_entries(menu, items, disabled_items);
}



void LLPlacesFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	{
		std::vector<std::string> items;
		std::vector<std::string> disabled_items;

		LLInventoryPanel* inv_panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
		bool is_open = false;
		if (inv_panel)
		{
			LLFolderViewFolder* folder = dynamic_cast<LLFolderViewFolder*>(inv_panel->getRootFolder()->getItemByID(mUUID));
			is_open = (NULL != folder) && folder->isOpen();
		}

		// collect all items' names
		fill_items_with_menu_items(items, menu);

		// remove expand or collapse menu item depend on folder state
		std::string collapse_expand_item_to_hide(is_open ? "expand" : "collapse");
		std::vector<std::string>::iterator it = std::find(items.begin(), items.end(), collapse_expand_item_to_hide);
		if (it != items.end())	items.erase(it);

		// Disabled items are processed via LLLandmarksPanel::isActionEnabled()
		// they should be synchronized with Places/My Landmarks/Gear menu. See EXT-1601 

		// repeat parent functionality
 		sSelf = this; // necessary for "New Folder" functionality

		hide_context_entries(menu, items, disabled_items);
	}
}

//virtual
void LLPlacesFolderBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("expand" == action)
	{
		LLFolderViewFolder* act_folder = getFolder();
		act_folder->toggleOpen();
	}
	else if ("collapse" == action)
	{
		LLFolderViewFolder* act_folder = getFolder();
		act_folder->toggleOpen();
	}
	else
	{
		LLFolderBridge::performAction(folder, model, action);
	}
}

LLFolderViewFolder* LLPlacesFolderBridge::getFolder()
{
	LLFolderViewFolder* folder = NULL;
	LLInventoryPanel* inv_panel = dynamic_cast<LLInventoryPanel*>(mInventoryPanel.get());
	if (inv_panel)
	{
		folder = dynamic_cast<LLFolderViewFolder*>(inv_panel->getRootFolder()->getItemByID(mUUID));
	}

	return folder;
}

// virtual
LLInvFVBridge* LLPlacesInventoryBridgeBuilder::createBridge(
	LLAssetType::EType asset_type,
	LLAssetType::EType actual_asset_type,
	LLInventoryType::EType inv_type,
	LLInventoryPanel* inventory,
	const LLUUID& uuid,
	U32 flags/* = 0x00*/) const
{
	LLInvFVBridge* new_listener = NULL;
	switch(asset_type)
	{
	case LLAssetType::AT_LANDMARK:
		if(!(inv_type == LLInventoryType::IT_LANDMARK))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLPlacesLandmarkBridge(inv_type, inventory, uuid, flags);
		break;
	case LLAssetType::AT_CATEGORY:
		if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
		{
			// *TODO: Create a link folder handler instead if it is necessary
			new_listener = LLInventoryFVBridgeBuilder::createBridge(
				asset_type,
				actual_asset_type,
				inv_type,
				inventory,
				uuid,
				flags);
			break;
		}
		new_listener = new LLPlacesFolderBridge(inv_type, inventory, uuid);
		break;
	default:
		new_listener = LLInventoryFVBridgeBuilder::createBridge(
			asset_type,
			actual_asset_type,
			inv_type,
			inventory,
			uuid,
			flags);
	}
	return new_listener;
}

// EOF
