/**
 * @file llplacesinventorybridge.cpp
 * @brief Implementation of the Inventory-Folder-View-Bridge classes for Places Panel.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llmenugl.h"

#include "llplacesinventorybridge.h"

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
        if (!isItemRemovable() || (gInventory.getCategory(mUUID) && !gInventory.isCategoryComplete(mUUID)))
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
    std::vector<std::string> items;
    std::vector<std::string> disabled_items;

    LLInventoryPanel* inv_panel = mInventoryPanel.get();
    bool is_open = false;
    if (inv_panel)
    {
        LLFolderViewFolder* folder =  dynamic_cast<LLFolderViewFolder*>(inv_panel->getItemByID(mUUID));
        is_open = (NULL != folder) && folder->isOpen();
    }

    // collect all items' names
    fill_items_with_menu_items(items, menu);

    // remove expand or collapse menu item depend on folder state
    std::string collapse_expand_item_to_hide(is_open ? "expand" :  "collapse");
    std::vector<std::string>::iterator it = std::find(items.begin(),  items.end(), collapse_expand_item_to_hide);
    if (it != items.end())  items.erase(it);


    // Disabled items are processed via LLLandmarksPanel::isActionEnabled()
    // they should be synchronized with Places/My Landmarks/Gear menu. See EXT-1601

    // repeat parent functionality
    sSelf = getHandle(); // necessary for "New Folder" functionality

    hide_context_entries(menu, items, disabled_items);
}

//virtual
void LLPlacesFolderBridge::performAction(LLInventoryModel* model, std::string action)
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
        LLFolderBridge::performAction(model, action);
    }
}

LLFolderViewFolder* LLPlacesFolderBridge::getFolder()
{
    LLFolderViewFolder* folder = NULL;
    LLInventoryPanel* inv_panel = mInventoryPanel.get();
    if (inv_panel)
    {
        folder =    dynamic_cast<LLFolderViewFolder*>(inv_panel->getItemByID(mUUID));
    }

    return folder;
}

// virtual
LLInvFVBridge* LLPlacesInventoryBridgeBuilder::createBridge(
    LLAssetType::EType asset_type,
    LLAssetType::EType actual_asset_type,
    LLInventoryType::EType inv_type,
    LLInventoryPanel* inventory,
    LLFolderViewModelInventory* view_model,
    LLFolderView* root,
    const LLUUID& uuid,
    U32 flags/* = 0x00*/) const
{
    LLInvFVBridge* new_listener = NULL;
    switch(asset_type)
    {
    case LLAssetType::AT_LANDMARK:
        if(!(inv_type == LLInventoryType::IT_LANDMARK))
        {
            LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
        }
        new_listener = new LLPlacesLandmarkBridge(inv_type, inventory, root, uuid, flags);
        break;
    case LLAssetType::AT_CATEGORY:
        if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
        {
            // *TODO: Create a link folder handler instead if it is necessary
            new_listener = LLInventoryFolderViewModelBuilder::createBridge(
                asset_type,
                actual_asset_type,
                inv_type,
                inventory,
                view_model,
                root,
                uuid,
                flags);
            break;
        }
        new_listener = new LLPlacesFolderBridge(inv_type, inventory, root, uuid);
        break;
    default:
        new_listener = LLInventoryFolderViewModelBuilder::createBridge(
            asset_type,
            actual_asset_type,
            inv_type,
            inventory,
            view_model,
            root,
            uuid,
            flags);
    }
    return new_listener;
}

// EOF
