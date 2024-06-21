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
#include "stringize.h"

LLAppearanceListener::LLAppearanceListener()
  : LLEventAPI("LLAppearance",
               "API to wear a specified outfit and wear/remove individual items")
{
    add("wearOutfit",
        "Wear outfit by folder id: [folder_id]"
        "When [\"append\"] is true, outfit will be added to COF\n"
        "otherwise it will replace current oufit",
        &LLAppearanceListener::wearOutfit,
        llsd::map("folder_id", LLSD(), "append", LLSD()));

    add("wearOutfitByName",
        "Wear outfit by folder name: [folder_name]"
        "When [\"append\"] is true, outfit will be added to COF\n"
        "otherwise it will replace current oufit",
        &LLAppearanceListener::wearOutfitByName,
        llsd::map("folder_name", LLSD(), "append", LLSD()));

    add("getOutfitsList",
        "Return the table of Outfits(id and name) which are send to the script",
         &LLAppearanceListener::getOutfitsList);
}


void LLAppearanceListener::wearOutfit(LLSD const &data)
{
    Response response(LLSD(), data);
    LLViewerInventoryCategory* cat = gInventory.getCategory(data["folder_id"].asUUID());
    if (!cat)
    {
        response.error(stringize("Couldn't find outfit ", data["folder_id"].asUUID()));
        return;
    }
    if (LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
    {
        response.error(stringize("Can't wear system folder ", data["folder_id"].asUUID()));
        return;
    }
    bool append = data["append"].asBoolean();
    bool can_wear = append ? LLAppearanceMgr::instance().getCanAddToCOF(cat->getUUID()) : LLAppearanceMgr::instance().getCanReplaceCOF(cat->getUUID());
    if (!can_wear)
    {
        std::string msg = append ? "Can't add to COF outfit " : "Can't replace COF with outfit ";
        response.error(stringize(msg, std::quoted(cat->getName()), " , id: ", cat->getUUID()));
        return;
    }
    LLAppearanceMgr::instance().wearInventoryCategory(cat, false, append);
}

void LLAppearanceListener::wearOutfitByName(LLSD const &data)
{
    Response response(LLSD(), data);
    std::string error_msg;
    if (!LLAppearanceMgr::instance().wearOutfitByName(data["folder_name"].asString(), data["append"].asBoolean(), error_msg))
    {
        response.error(error_msg);
    }
}

void LLAppearanceListener::getOutfitsList(LLSD const &data)
{
    Response response(LLSD(), data);
    const LLUUID outfits_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);

    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;

    LLIsType is_category(LLAssetType::AT_CATEGORY);
    gInventory.collectDescendentsIf(outfits_id, cat_array, item_array, LLInventoryModel::EXCLUDE_TRASH, is_category);

    LLSD outfits_data;
    for (const LLPointer<LLViewerInventoryCategory> &cat : cat_array)
    {
        outfits_data[cat->getUUID().asString()] = cat->getName();
    }
    response["outfits"] = outfits_data;
}
