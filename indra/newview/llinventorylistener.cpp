/**
 * @file llinventorylistener.cpp
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llinventorylistener.h"

#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "lltransutil.h"
#include "llwearableitemslist.h"
#include "stringize.h"

LLInventoryListener::LLInventoryListener()
  : LLEventAPI("LLInventory",
               "API for interactions with viewer Inventory items")
{
    add("getItemsInfo",
        "Return information about items or folders defined in [\"items_id\"]:\n"
        "reply will contain [\"items\"] and [\"categories\"] tables accordingly",
        &LLInventoryListener::getItemsInfo,
        llsd::map("items_id", LLSD(), "reply", LLSD()));

    add("getFolderTypeNames",
        "Return the table of folder type names, contained in [\"type_names\"]\n",
        &LLInventoryListener::getFolderTypeNames,
        llsd::map("reply", LLSD()));

    add("getBasicFolderID",
        "Return the UUID of the folder by specified folder type name, for example:\n"
        "\"Textures\", \"My outfits\", \"Sounds\" and other basic folders which have associated type",
        &LLInventoryListener::getBasicFolderID,
        llsd::map("ft_name", LLSD(), "reply", LLSD()));

    add("getDirectDescendents",
        "Return the direct descendents(both items and folders) of the [\"folder_id\"]",
        &LLInventoryListener::getDirectDescendents,
        llsd::map("folder_id", LLSD(), "reply", LLSD()));
 }


void add_item_info(LLEventAPI::Response& response, LLViewerInventoryItem* item)
{
    response["items"].insert(item->getUUID().asString(),
                             llsd::map("name", item->getName(), "parent_id", item->getParentUUID(), "desc", item->getDescription(),
                                       "inv_type", LLInventoryType::lookup(item->getInventoryType()), "creation_date",
                                       (S32) item->getCreationDate(), "asset_id", item->getAssetUUID(), "is_link", item->getIsLinkType(),
                                       "linked_id", item->getLinkedUUID()));
}

void add_cat_info(LLEventAPI::Response &response, LLViewerInventoryCategory *cat)
{
    response["categories"].insert(cat->getUUID().asString(),
                                  llsd::map("name", cat->getName(), "parent_id", cat->getParentUUID(), "type",
                                            LLFolderType::lookup(cat->getPreferredType()), "creation_date", (S32) cat->getCreationDate(),
                                            "is_link", cat->getIsLinkType(), "linked_id", cat->getLinkedUUID()));
}

void LLInventoryListener::getItemsInfo(LLSD const &data)
{
    Response response(LLSD(), data);

    uuid_vec_t ids = LLSDParam<uuid_vec_t>(data["items_id"]);
    for (auto &it : ids)
    {
        LLViewerInventoryItem* item = gInventory.getItem(it);
        if (item)
        {
            add_item_info(response, item);
        }
        LLViewerInventoryCategory* cat = gInventory.getCategory(it);
        if (cat)
        {
            add_cat_info(response, cat);
        }
    }
}

void LLInventoryListener::getFolderTypeNames(LLSD const &data)
{
    Response response(llsd::map("type_names", LLFolderType::getTypeNames()), data);
}

void LLInventoryListener::getBasicFolderID(LLSD const &data)
{
    Response response(llsd::map("id", gInventory.findCategoryUUIDForType(LLFolderType::lookup(data["ft_name"].asString()))), data);
}


void LLInventoryListener::getDirectDescendents(LLSD const &data)
{
    Response response(LLSD(), data);
    LLInventoryModel::cat_array_t* cats; 
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(data["folder_id"], cats, items);

    LLInventoryModel::item_array_t items_copy = *items;
    for (LLInventoryModel::item_array_t::iterator iter = items_copy.begin(); iter != items_copy.end(); iter++)
    {
        add_item_info(response, *iter);
    }
    LLInventoryModel::cat_array_t cats_copy = *cats;
    for (LLInventoryModel::cat_array_t::iterator iter = cats_copy.begin(); iter != cats_copy.end(); iter++)
    {
        add_cat_info(response, *iter);
    }
}

