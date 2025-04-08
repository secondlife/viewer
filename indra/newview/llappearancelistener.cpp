/**
 * @file llappearancelistener.cpp
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

#include "llappearancelistener.h"

#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "lltransutil.h"
#include "llwearableitemslist.h"
#include "stringize.h"

LLAppearanceListener::LLAppearanceListener()
  : LLEventAPI("LLAppearance",
               "API to wear a specified outfit and wear/remove individual items")
{
    add("wearOutfit",
        "Wear outfit by folder id: [\"folder_id\"] OR by folder name: [\"folder_name\"]\n"
        "When [\"append\"] is true, outfit will be added to COF\n"
        "otherwise it will replace current oufit",
        &LLAppearanceListener::wearOutfit);

    add("wearItems",
        "Wear items by id: [items_id]",
        &LLAppearanceListener::wearItems,
        llsd::map("items_id", LLSD(), "replace", LLSD()));

    add("detachItems",
        "Detach items by id: [items_id]",
        &LLAppearanceListener::detachItems,
        llsd::map("items_id", LLSD()));

    add("getOutfitsList",
        "Return the table with Outfits info(id and name)",
         &LLAppearanceListener::getOutfitsList);

    add("getOutfitItems",
        "Return the table of items with info(id : name, wearable_type, is_worn) inside specified outfit folder",
         &LLAppearanceListener::getOutfitItems);
}


void LLAppearanceListener::wearOutfit(LLSD const &data)
{
    Response response(LLSD(), data);
    if (!data.has("folder_id") && !data.has("folder_name"))
    {
        return response.error("Either [folder_id] or [folder_name] is required");
    }

    bool append = data.has("append") ? data["append"].asBoolean() : false;
    if (!LLAppearanceMgr::instance().wearOutfit(data, append))
    {
        response.error("Failed to wear outfit");
    }
}

void LLAppearanceListener::wearItems(LLSD const &data)
{
    const LLSD& items_id{ data["items_id"] };
    uuid_vec_t  ids;
    if (!items_id.isArray())
    {
        ids.push_back(items_id.asUUID());
    }
    else // array
    {
        for (const auto& id : llsd::inArray(items_id))
        {
            ids.push_back(id);
        }
    }
    LLAppearanceMgr::instance().wearItemsOnAvatar(ids, true, data["replace"].asBoolean());
}

void LLAppearanceListener::detachItems(LLSD const &data)
{
    const LLSD& items_id{ data["items_id"] };
    uuid_vec_t  ids;
    if (!items_id.isArray())
    {
        ids.push_back(items_id.asUUID());
    }
    else // array
    {
        for (const auto& id : llsd::inArray(items_id))
        {
            ids.push_back(id);
        }
    }
    LLAppearanceMgr::instance().removeItemsFromAvatar(ids);
}

void LLAppearanceListener::getOutfitsList(LLSD const &data)
{
    Response response(LLSD(), data);
    const LLUUID outfits_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;

    LLIsFolderType is_category(LLFolderType::FT_OUTFIT);
    gInventory.collectDescendentsIf(outfits_id, cat_array, item_array, LLInventoryModel::EXCLUDE_TRASH, is_category);

    response["outfits"] = llsd::toMap(cat_array,
        [](const LLPointer<LLViewerInventoryCategory> &cat)
        { return std::make_pair(cat->getUUID().asString(), cat->getName()); });
}

void LLAppearanceListener::getOutfitItems(LLSD const &data)
{
    Response response(LLSD(), data);
    LLUUID outfit_id(data["outfit_id"].asUUID());
    LLViewerInventoryCategory *cat = gInventory.getCategory(outfit_id);
    if (!cat || cat->getPreferredType() != LLFolderType::FT_OUTFIT)
    {
        return response.error(stringize("Couldn't find outfit ", outfit_id.asString()));
    }
    LLInventoryModel::cat_array_t  cat_array;
    LLInventoryModel::item_array_t item_array;

    LLFindOutfitItems collector = LLFindOutfitItems();
    gInventory.collectDescendentsIf(outfit_id, cat_array, item_array, LLInventoryModel::EXCLUDE_TRASH, collector);

    response["items"] = llsd::toMap(item_array,
        [](const LLPointer<LLViewerInventoryItem> &it)
        {
            return std::make_pair(
                it->getUUID().asString(),
                llsd::map(
                    "name", it->getName(),
                    "wearable_type", LLWearableType::getInstance()->getTypeName(it->isWearableType() ? it->getWearableType() : LLWearableType::WT_NONE),
                    "is_worn", get_is_item_worn(it)));
        });
}
