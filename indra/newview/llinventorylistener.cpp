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

    add("getAssetTypeNames",
        "Return the table of asset type names, contained in [\"type_names\"]\n",
        &LLInventoryListener::getAssetTypeNames,
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

    add("collectDescendentsIf",
        "Return the descendents(both items and folders) of the [\"folder_id\"], if it passes specified filters:\n"
        "[\"name\"] is a substring of object's name,\n"
        "[\"desc\"] is a substring of object's description,\n"
        "asset [\"type\"] corresponds to the object's asset type\n" 
        "[\"filter_links\"]: EXCLUDE_LINKS - don't show links, ONLY_LINKS - only show links, INCLUDE_LINKS - show links too (default)",
        &LLInventoryListener::collectDescendentsIf,
        llsd::map("folder_id", LLSD(), "reply", LLSD()));
 }


void add_item_info(LLEventAPI::Response& response, LLViewerInventoryItem* item)
{
    response["items"].insert(item->getUUID().asString(),
                             llsd::map("name", item->getName(), "parent_id", item->getParentUUID(), "desc", item->getDescription(),
                                       "inv_type", LLInventoryType::lookup(item->getInventoryType()), "asset_type", LLAssetType::lookup(item->getType()),
                                       "creation_date", (S32) item->getCreationDate(), "asset_id", item->getAssetUUID(), 
                                       "is_link", item->getIsLinkType(), "linked_id", item->getLinkedUUID()));
}

void add_cat_info(LLEventAPI::Response &response, LLViewerInventoryCategory *cat)
{
    response["categories"].insert(cat->getUUID().asString(),
                                  llsd::map("name", cat->getName(), "parent_id", cat->getParentUUID(), "type", LLFolderType::lookup(cat->getPreferredType())));
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

void LLInventoryListener::getAssetTypeNames(LLSD const &data)
{
    Response response(llsd::map("type_names", LLAssetType::getTypeNames()), data);
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

void LLInventoryListener::collectDescendentsIf(LLSD const &data)
{
    Response response(LLSD(), data);
    LLUUID folder_id(data["folder_id"].asUUID());
    LLViewerInventoryCategory *cat = gInventory.getCategory(folder_id);
    if (!cat)
    {
        return response.error(stringize("Folder ", std::quoted(data["folder_id"].asString()), " was not found"));
    }
    LLInventoryModel::cat_array_t  cat_array;
    LLInventoryModel::item_array_t item_array;

    LLFilteredCollector collector = LLFilteredCollector(data);

    gInventory.collectDescendentsIf(folder_id, cat_array, item_array, LLInventoryModel::EXCLUDE_TRASH, collector);

    for (LLInventoryModel::item_array_t::iterator iter = item_array.begin(); iter != item_array.end(); iter++)
    {
        add_item_info(response, *iter);
    }
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array.begin(); iter != cat_array.end(); iter++)
    {
        add_cat_info(response, *iter);
    }
}

LLFilteredCollector::LLFilteredCollector(LLSD const &data)
    : mType(LLAssetType::EType::AT_UNKNOWN),
      mLinkFilter(INCLUDE_LINKS)
{
    if (data.has("name"))
    {
        mName = data["name"];
    }
    if (data.has("desc"))
    {
        mDesc = data["desc"];
    }
    if (data.has("type"))
    {
        mType = LLAssetType::lookup(data["type"]);
    }
    if (data.has("filter_links"))
    {
        if (data["filter_links"] == "EXCLUDE_LINKS")
        {
            mLinkFilter = EXCLUDE_LINKS;
        }
        else if (data["filter_links"] == "ONLY_LINKS")
        {
            mLinkFilter = ONLY_LINKS;
        }
    }
}

bool LLFilteredCollector::operator()(LLInventoryCategory *cat, LLInventoryItem *item)
{
    bool passed = checkagainstType(cat, item);
    passed = passed && checkagainstNameDesc(cat, item);
    passed = passed && checkagainstLinks(cat, item);

    return passed;
}

bool LLFilteredCollector::checkagainstNameDesc(LLInventoryCategory *cat, LLInventoryItem *item)
{
    std::string name, desc;
    bool passed(true);
    if (cat)
    {
        if (!mDesc.empty()) return false;
        name = cat->getName();
    }
    if (item)
    {
        name = item->getName();
        passed = (mDesc.size() ? item->getDescription().find(mDesc) != std::string::npos : true);
    }

    return passed && (mName.size() ? name.find(mName) != std::string::npos : true);
}

bool LLFilteredCollector::checkagainstType(LLInventoryCategory *cat, LLInventoryItem *item)
{
    if (mType == LLAssetType::AT_UNKNOWN)
    {
        return true;
    }
    if (mType == LLAssetType::AT_CATEGORY)
    {
        if (cat)
        {
            return true;
        }
    }
    if (item && item->getType() == mType)
    {
        return true;
    }
    return false;
}

bool LLFilteredCollector::checkagainstLinks(LLInventoryCategory *cat, LLInventoryItem *item)
{
    bool is_link = cat ? cat->getIsLinkType() : item->getIsLinkType();
    if (is_link && (mLinkFilter == EXCLUDE_LINKS))
        return false;
    if (!is_link && (mLinkFilter == ONLY_LINKS))
        return false;
    return true;
}
