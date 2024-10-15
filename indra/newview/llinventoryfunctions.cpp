/**
 * @file llinventoryfunctions.cpp
 * @brief Implementation of the inventory view and associated stuff.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include <utility> // for std::pair<>

#include "llinventoryfunctions.h"

// library includes
#include "llagent.h"
#include "llagentwearables.h"
#include "llcallingcard.h"
#include "llfloaterreg.h"
#include "llinventorydefines.h"
#include "llsdserialize.h"
#include "llfiltereditor.h"
#include "llspinctrl.h"
#include "llui.h"
#include "message.h"

// newview includes
#include "llappearancemgr.h"
#include "llappviewer.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llclipboard.h"
#include "lldirpicker.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llfloatermarketplacelistings.h"
#include "llfloatersidepanelcontainer.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "llgiveinventory.h"
#include "lliconctrl.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "lllineeditor.h"
#include "llmarketplacenotifications.h"
#include "llmarketplacefunctions.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "llpanelmaininventory.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llselectmgr.h"
#include "llsidepanelinventory.h"
#include "lltabcontainer.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerfoldertype.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"

bool LLInventoryState::sWearNewClothing = false;
LLUUID LLInventoryState::sWearNewClothingTransactionID;
std::list<LLUUID> LLInventoryAction::sMarketplaceFolders;
bool LLInventoryAction::sDeleteConfirmationDisplayed = false;

// Helper function : callback to update a folder after inventory action happened in the background
void update_folder_cb(const LLUUID& dest_folder)
{
    LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);
    gInventory.updateCategory(dest_cat);
    gInventory.notifyObservers();
}

// Helper function : Count only the copyable items, i.e. skip the stock items (which are no copy)
S32 count_copyable_items(LLInventoryModel::item_array_t& items)
{
    S32 count = 0;
    for (LLInventoryModel::item_array_t::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        LLViewerInventoryItem* item = *it;
        if (item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            count++;
        }
    }
    return count;
}

// Helper function : Count only the non-copyable items, i.e. the stock items, skip the others
S32 count_stock_items(LLInventoryModel::item_array_t& items)
{
    S32 count = 0;
    for (LLInventoryModel::item_array_t::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        LLViewerInventoryItem* item = *it;
        if (!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            count++;
        }
    }
    return count;
}

// Helper function : Count the number of stock folders
S32 count_stock_folders(LLInventoryModel::cat_array_t& categories)
{
    S32 count = 0;
    for (LLInventoryModel::cat_array_t::const_iterator it = categories.begin(); it != categories.end(); ++it)
    {
        LLInventoryCategory* cat = *it;
        if (cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
        {
            count++;
        }
    }
    return count;
}

// Helper funtion : Count the number of items (not folders) in the descending hierarchy
S32 count_descendants_items(const LLUUID& cat_id)
{
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);

    S32 count = static_cast<S32>(item_array->size());

    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLViewerInventoryCategory* category = *iter;
        count += count_descendants_items(category->getUUID());
    }

    return count;
}

// Helper function : Returns true if the hierarchy contains nocopy items
bool contains_nocopy_items(const LLUUID& id)
{
    LLInventoryCategory* cat = gInventory.getCategory(id);

    if (cat)
    {
        // Get the content
        LLInventoryModel::cat_array_t* cat_array;
        LLInventoryModel::item_array_t* item_array;
        gInventory.getDirectDescendentsOf(id,cat_array,item_array);

        // Check all the items: returns true upon encountering a nocopy item
        for (LLInventoryModel::item_array_t::iterator iter = item_array->begin(); iter != item_array->end(); iter++)
        {
            LLInventoryItem* item = *iter;
            LLViewerInventoryItem * inv_item = (LLViewerInventoryItem *) item;
            if (!inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
            {
                return true;
            }
        }

        // Check all the sub folders recursively
        for (LLInventoryModel::cat_array_t::iterator iter = cat_array->begin(); iter != cat_array->end(); iter++)
        {
            LLViewerInventoryCategory* cat = *iter;
            if (contains_nocopy_items(cat->getUUID()))
            {
                return true;
            }
        }
    }
    else
    {
        LLInventoryItem* item = gInventory.getItem(id);
        LLViewerInventoryItem * inv_item = (LLViewerInventoryItem *) item;
        if (!inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            return true;
        }
    }

    // Exit without meeting a nocopy item
    return false;
}

// Generates a string containing the path to the item specified by id.
void append_path(const LLUUID& id, std::string& path)
{
    std::string temp;
    const LLInventoryObject* obj = gInventory.getObject(id);
    LLUUID parent_id;
    if(obj) parent_id = obj->getParentUUID();
    std::string forward_slash("/");
    while(obj)
    {
        obj = gInventory.getCategory(parent_id);
        if(obj)
        {
            temp.assign(forward_slash + obj->getName() + temp);
            parent_id = obj->getParentUUID();
        }
    }
    path.append(temp);
}

// Generates a string containing the path name of the object.
std::string make_path(const LLInventoryObject* object)
{
    std::string path;
    append_path(object->getUUID(), path);
    return path + "/" + object->getName();
}

// Generates a string containing the path name of the object specified by id.
std::string make_inventory_path(const LLUUID& id)
{
    if (LLInventoryObject* object = gInventory.getObject(id))
        return make_path(object);
    return "";
}

// Generates a string containing the path name and id of the object.
std::string make_info(const LLInventoryObject* object)
{
    return "'" + make_path(object) + "' (" + object->getUUID().asString() + ")";
}

// Generates a string containing the path name and id of the object specified by id.
std::string make_inventory_info(const LLUUID& id)
{
    if (LLInventoryObject* object = gInventory.getObject(id))
        return make_info(object);
    return "<Inventory object not found!> (" + id.asString() + ")";
}

void update_marketplace_folder_hierarchy(const LLUUID cat_id)
{
    // When changing the marketplace status of a folder, the only thing that needs to happen is
    // for all observers of the folder to, possibly, change the display label of the folder
    // so that's the only thing we change on the update mask.
    gInventory.addChangedMask(LLInventoryObserver::LABEL, cat_id);

    // Update all descendent folders down
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);

    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        update_marketplace_folder_hierarchy(category->getUUID());
    }
    return;
}

void update_marketplace_category(const LLUUID& cur_uuid, bool perform_consistency_enforcement, bool skip_clear_listing)
{
    // When changing the marketplace status of an item, we usually have to change the status of all
    // folders in the same listing. This is because the display of each folder is affected by the
    // overall status of the whole listing.
    // Consequently, the only way to correctly update an item anywhere in the marketplace is to
    // update the whole listing from its listing root.
    // This is not as bad as it seems as we only update folders, not items, and the folder nesting depth
    // is limited to 4.
    // We also take care of degenerated cases so we don't update all folders in the inventory by mistake.

    if (cur_uuid.isNull()
        || gInventory.getCategory(cur_uuid) == NULL
        || gInventory.getCategory(cur_uuid)->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
    {
        return;
    }

    // Grab marketplace listing data for this item
    S32 depth = depth_nesting_in_marketplace(cur_uuid);
    if (depth > 0)
    {
        // Retrieve the listing uuid this object is in
        LLUUID listing_uuid = nested_parent_id(cur_uuid, depth);
        LLViewerInventoryCategory* listing_cat = gInventory.getCategory(listing_uuid);
        bool listing_cat_loaded = listing_cat != NULL && listing_cat->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;

        // Verify marketplace data consistency for this listing
        if (perform_consistency_enforcement
            && listing_cat_loaded
            && LLMarketplaceData::instance().isListed(listing_uuid))
        {
            LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolder(listing_uuid);
            S32 version_depth = depth_nesting_in_marketplace(version_folder_uuid);
            if (version_folder_uuid.notNull() && (!gInventory.isObjectDescendentOf(version_folder_uuid, listing_uuid) || (version_depth != 2)))
            {
                LL_INFOS("SLM") << "Unlist and clear version folder as the version folder is not at the right place anymore!!" << LL_ENDL;
                LLMarketplaceData::instance().setVersionFolder(listing_uuid, LLUUID::null,1);
            }
            else if (version_folder_uuid.notNull()
                     && gInventory.isCategoryComplete(version_folder_uuid)
                     && LLMarketplaceData::instance().getActivationState(version_folder_uuid)
                     && (count_descendants_items(version_folder_uuid) == 0)
                     && !LLMarketplaceData::instance().isUpdating(version_folder_uuid,version_depth))
            {
                LL_INFOS("SLM") << "Unlist as the version folder is empty of any item!!" << LL_ENDL;
                LLNotificationsUtil::add("AlertMerchantVersionFolderEmpty");
                LLMarketplaceData::instance().activateListing(listing_uuid, false,1);
            }
        }

        // Check if the count on hand needs to be updated on SLM
        if (perform_consistency_enforcement
            && listing_cat_loaded
            && (compute_stock_count(listing_uuid) != LLMarketplaceData::instance().getCountOnHand(listing_uuid)))
        {
            LLMarketplaceData::instance().updateCountOnHand(listing_uuid,1);
        }
        // Update all descendents starting from the listing root
        update_marketplace_folder_hierarchy(listing_uuid);
    }
    else if (depth == 0)
    {
        // If this is the marketplace listings root itself, update all descendents
        if (gInventory.getCategory(cur_uuid))
        {
            update_marketplace_folder_hierarchy(cur_uuid);
        }
    }
    else
    {
        // If the folder is outside the marketplace listings root, clear its SLM data if needs be
        if (perform_consistency_enforcement && !skip_clear_listing && LLMarketplaceData::instance().isListed(cur_uuid))
        {
            LL_INFOS("SLM") << "Disassociate as the listing folder is not under the marketplace folder anymore!!" << LL_ENDL;
            LLMarketplaceData::instance().clearListing(cur_uuid);
        }
        // Update all descendents if this is a category
        if (gInventory.getCategory(cur_uuid))
        {
            update_marketplace_folder_hierarchy(cur_uuid);
        }
    }

    return;
}

// Iterate through the marketplace and flag for label change all categories that countain a stock folder (i.e. stock folders and embedding folders up the hierarchy)
void update_all_marketplace_count(const LLUUID& cat_id)
{
    // Get all descendent folders down
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);

    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        if (category->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
        {
            // Listing containing stock folders needs to be updated but not others
            // Note: we take advantage of the fact that stock folder *do not* contain sub folders to avoid a recursive call here
            update_marketplace_category(category->getUUID());
        }
        else
        {
            // Explore the contained folders recursively
            update_all_marketplace_count(category->getUUID());
        }
    }
}

void update_all_marketplace_count()
{
    // Get the marketplace root and launch the recursive exploration
    const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    if (!marketplace_listings_uuid.isNull())
    {
        update_all_marketplace_count(marketplace_listings_uuid);
    }
    return;
}

void rename_category(LLInventoryModel* model, const LLUUID& cat_id, const std::string& new_name)
{
    LLViewerInventoryCategory* cat;

    if (!model ||
        !get_is_category_renameable(model, cat_id) ||
        (cat = model->getCategory(cat_id)) == NULL ||
        cat->getName() == new_name)
    {
        return;
    }

    LLSD updates;
    updates["name"] = new_name;
    update_inventory_category(cat_id, updates, NULL);
}

void copy_inventory_category(LLInventoryModel* model,
                             LLViewerInventoryCategory* cat,
                             const LLUUID& parent_id,
                             const LLUUID& root_copy_id,
                             bool move_no_copy_items)
{
    // Create the initial folder
    inventory_func_type func = [model, cat, root_copy_id, move_no_copy_items](const LLUUID& new_id)
    {
        copy_inventory_category_content(new_id, model, cat, root_copy_id, move_no_copy_items);
    };
    gInventory.createNewCategory(parent_id, LLFolderType::FT_NONE, cat->getName(), func, cat->getThumbnailUUID());
}

void copy_inventory_category(LLInventoryModel* model,
                             LLViewerInventoryCategory* cat,
                             const LLUUID& parent_id,
                             const LLUUID& root_copy_id,
                             bool move_no_copy_items,
                             inventory_func_type callback)
{
    // Create the initial folder
    inventory_func_type func = [model, cat, root_copy_id, move_no_copy_items, callback](const LLUUID &new_id)
    {
        copy_inventory_category_content(new_id, model, cat, root_copy_id, move_no_copy_items);
        if (callback)
        {
            callback(new_id);
        }
    };
    gInventory.createNewCategory(parent_id, LLFolderType::FT_NONE, cat->getName(), func, cat->getThumbnailUUID());
}

void copy_cb(const LLUUID& dest_folder, const LLUUID& root_id)
{
    // Decrement the count in root_id since that one item won't be copied over
    LLMarketplaceData::instance().decrementValidationWaiting(root_id);
    update_folder_cb(dest_folder);
};

void copy_inventory_category_content(const LLUUID& new_cat_uuid, LLInventoryModel* model, LLViewerInventoryCategory* cat, const LLUUID& root_copy_id, bool move_no_copy_items)
{
    model->notifyObservers();

    // We need to exclude the initial root of the copy to avoid recursively copying the copy, etc...
    LLUUID root_id = (root_copy_id.isNull() ? new_cat_uuid : root_copy_id);

    // Get the content of the folder
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(cat->getUUID(), cat_array, item_array);

    // If root_copy_id is null, tell the marketplace model we'll be waiting for new items to be copied over for this folder
    if (root_copy_id.isNull())
    {
        LLMarketplaceData::instance().setValidationWaiting(root_id, count_descendants_items(cat->getUUID()));
    }

    LLPointer<LLInventoryCallback> cb;
    if (root_copy_id.isNull())
    {
        cb = new LLBoostFuncInventoryCallback(boost::bind(copy_cb, new_cat_uuid, root_id));
    }
    else
    {
        cb = new LLBoostFuncInventoryCallback(boost::bind(update_folder_cb, new_cat_uuid));
    }

    // Copy all the items
    LLInventoryModel::item_array_t item_array_copy = *item_array;
    for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
    {
        LLInventoryItem* item = *iter;

        if (item->getIsLinkType())
        {
            link_inventory_object(new_cat_uuid, item->getLinkedUUID(), cb);
        }
        else if (!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            // If the item is nocopy, we do nothing or, optionally, move it
            if (move_no_copy_items)
            {
                // Reparent the item
                LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *)item;
                gInventory.changeItemParent(viewer_inv_item, new_cat_uuid, true);
            }
            if (root_copy_id.isNull())
            {
                // Decrement the count in root_id since that one item won't be copied over
                LLMarketplaceData::instance().decrementValidationWaiting(root_id);
            }
        }
        else
        {
            copy_inventory_item(
                gAgent.getID(),
                item->getPermissions().getOwner(),
                item->getUUID(),
                new_cat_uuid,
                std::string(),
                cb);
        }
    }

    // Copy all the folders
    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLViewerInventoryCategory* category = *iter;
        if (category->getUUID() != root_id)
        {
            copy_inventory_category(model, category, new_cat_uuid, root_id, move_no_copy_items);
        }
    }
}

class LLInventoryCollectAllItems : public LLInventoryCollectFunctor
{
public:
    virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
    {
        return true;
    }
};

bool get_is_parent_to_worn_item(const LLUUID& id)
{
    const LLViewerInventoryCategory* cat = gInventory.getCategory(id);
    if (!cat)
    {
        return false;
    }

    LLInventoryModel::cat_array_t cats;
    LLInventoryModel::item_array_t items;
    LLInventoryCollectAllItems collect_all;
    gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(), cats, items, LLInventoryModel::EXCLUDE_TRASH, collect_all);

    for (LLInventoryModel::item_array_t::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        const LLViewerInventoryItem * const item = *it;

        llassert(item->getIsLinkType());

        LLUUID linked_id = item->getLinkedUUID();
        const LLViewerInventoryItem * const linked_item = gInventory.getItem(linked_id);

        if (linked_item)
        {
            LLUUID parent_id = linked_item->getParentUUID();

            while (!parent_id.isNull())
            {
                LLInventoryCategory * parent_cat = gInventory.getCategory(parent_id);

                if (cat == parent_cat)
                {
                    return true;
                }

                parent_id = parent_cat->getParentUUID();
            }
        }
    }

    return false;
}

bool get_is_item_worn(const LLUUID& id, const LLViewerInventoryItem* item)
{
    if (!item)
        return false;

    if (item->getIsLinkType() && !gInventory.getItem(item->getLinkedUUID()))
    {
        return false;
    }

    // Consider the item as worn if it has links in COF.
    if (LLAppearanceMgr::instance().isLinkedInCOF(id))
    {
        return true;
    }

    switch(item->getType())
    {
        case LLAssetType::AT_OBJECT:
        {
            if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
                return true;
            break;
        }
        case LLAssetType::AT_BODYPART:
        case LLAssetType::AT_CLOTHING:
            if(gAgentWearables.isWearingItem(item->getLinkedUUID()))
                return true;
            break;
        case LLAssetType::AT_GESTURE:
            if (LLGestureMgr::instance().isGestureActive(item->getLinkedUUID()))
                return true;
            break;
        default:
            break;
    }
    return false;
}

bool get_is_item_worn(const LLUUID& id)
{
    const LLViewerInventoryItem* item = gInventory.getItem(id);
    return get_is_item_worn(id, item);
}

bool get_is_item_worn(const LLViewerInventoryItem* item)
{
    if (!item)
    {
        return false;
    }
    return get_is_item_worn(item->getUUID(), item);
}

bool get_can_item_be_worn(const LLUUID& id)
{
    const LLViewerInventoryItem* item = gInventory.getItem(id);
    if (!item)
        return false;

    if (LLAppearanceMgr::instance().isLinkedInCOF(item->getLinkedUUID()))
    {
        // an item having links in COF (i.e. a worn item)
        return false;
    }

    if (gInventory.isObjectDescendentOf(id, LLAppearanceMgr::instance().getCOF()))
    {
        // a non-link object in COF (should not normally happen)
        return false;
    }

    const LLUUID trash_id = gInventory.findCategoryUUIDForType(
            LLFolderType::FT_TRASH);

    // item can't be worn if base obj in trash, see EXT-7015
    if (gInventory.isObjectDescendentOf(item->getLinkedUUID(),
            trash_id))
    {
        return false;
    }

    switch(item->getType())
    {
        case LLAssetType::AT_OBJECT:
        {
            if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
            {
                // Already being worn
                return false;
            }
            else
            {
                // Not being worn yet.
                return true;
            }
            break;
        }
        case LLAssetType::AT_BODYPART:
        case LLAssetType::AT_CLOTHING:
            if(gAgentWearables.isWearingItem(item->getLinkedUUID()))
            {
                // Already being worn
                return false;
            }
            else
            {
                // Not being worn yet.
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool get_is_item_removable(const LLInventoryModel* model, const LLUUID& id, bool check_worn)
{
    if (!model)
    {
        return false;
    }

    // Can't delete an item that's in the library.
    if (!model->isObjectDescendentOf(id, gInventory.getRootFolderID()))
    {
        return false;
    }

    // Disable delete from COF folder; have users explicitly choose "detach/take off",
    // unless the item is not worn but in the COF (i.e. is bugged).
    const LLViewerInventoryItem* obj = model->getItem(id);
    if (LLAppearanceMgr::instance().getIsProtectedCOFItem(obj))
    {
        if (get_is_item_worn(id, obj))
        {
            return false;
        }
    }

    if (obj && obj->getIsLinkType())
    {
        return true;
    }
    if (check_worn && get_is_item_worn(id, obj))
    {
        return false;
    }
    return true;
}

bool get_is_item_editable(const LLUUID& inv_item_id)
{
    if (const LLInventoryItem* inv_item = gInventory.getLinkedItem(inv_item_id))
    {
        switch (inv_item->getType())
        {
            case LLAssetType::AT_BODYPART:
            case LLAssetType::AT_CLOTHING:
                return gAgentWearables.isWearableModifiable(inv_item_id);
            case LLAssetType::AT_OBJECT:
                return true;
            default:
                return false;;
        }
    }
    return gAgentAvatarp->getWornAttachment(inv_item_id) != nullptr;
}

void handle_item_edit(const LLUUID& inv_item_id)
{
    if (get_is_item_editable(inv_item_id))
    {
        if (const LLInventoryItem* inv_item = gInventory.getLinkedItem(inv_item_id))
        {
            switch (inv_item->getType())
            {
                case LLAssetType::AT_BODYPART:
                case LLAssetType::AT_CLOTHING:
                    LLAgentWearables::editWearable(inv_item_id);
                    break;
                case LLAssetType::AT_OBJECT:
                    handle_attachment_edit(inv_item_id);
                    break;
                default:
                    break;
            }
        }
        else
        {
            handle_attachment_edit(inv_item_id);
        }
    }
}

bool get_is_category_removable(const LLInventoryModel* model, const LLUUID& id)
{
    // NOTE: This function doesn't check the folder's children.
    // See LLFolderBridge::isItemRemovable for a function that does
    // consider the children.

    if (!model)
    {
        return false;
    }

    if (!model->isObjectDescendentOf(id, gInventory.getRootFolderID()))
    {
        return false;
    }

    if (!isAgentAvatarValid()) return false;

    const LLInventoryCategory* category = model->getCategory(id);
    if (!category)
    {
        return false;
    }

    const LLFolderType::EType folder_type = category->getPreferredType();

    if (LLFolderType::lookupIsProtectedType(folder_type))
    {
        return false;
    }

    // Can't delete the outfit that is currently being worn.
    if (folder_type == LLFolderType::FT_OUTFIT)
    {
        const LLViewerInventoryItem *base_outfit_link = LLAppearanceMgr::instance().getBaseOutfitLink();
        if (base_outfit_link && (category == base_outfit_link->getLinkedCategory()))
        {
            return false;
        }
    }

    return true;
}

bool get_is_category_and_children_removable(LLInventoryModel* model, const LLUUID& folder_id, bool check_worn)
{
    if (!get_is_category_removable(model, folder_id))
    {
        return false;
    }

    LLInventoryModel::cat_array_t cat_array;
    LLInventoryModel::item_array_t item_array;
    model->collectDescendents(
        folder_id,
        cat_array,
        item_array,
        LLInventoryModel::EXCLUDE_TRASH);

    if (check_worn)
    {
        for (LLInventoryModel::item_array_t::value_type& item : item_array)
        {
            // Disable delete/cut from COF folder; have users explicitly choose "detach/take off",
            // unless the item is not worn but in the COF (i.e. is bugged).
            if (item)
            {
                if (LLAppearanceMgr::instance().getIsProtectedCOFItem(item))
                {
                    if (get_is_item_worn(item))
                    {
                        return false;
                    }
                }

                if (!item->getIsLinkType() && get_is_item_worn(item))
                {
                    return false;
                }
            }
        }
    }

    const LLViewerInventoryItem* base_outfit_link = LLAppearanceMgr::instance().getBaseOutfitLink();
    LLViewerInventoryCategory* outfit_linked_category = base_outfit_link ? base_outfit_link->getLinkedCategory() : nullptr;
    for (LLInventoryModel::cat_array_t::value_type& cat : cat_array)
    {
        const LLFolderType::EType folder_type = cat->getPreferredType();
        if (LLFolderType::lookupIsProtectedType(folder_type))
        {
            return false;
        }

        // Can't delete the outfit that is currently being worn.
        if (folder_type == LLFolderType::FT_OUTFIT)
        {
            if (cat == outfit_linked_category)
            {
                return false;
            }
        }
    }

    return true;
}

bool get_is_category_renameable(const LLInventoryModel* model, const LLUUID& id)
{
    if (!model)
    {
        return false;
    }

    LLViewerInventoryCategory* cat = model->getCategory(id);

    if (cat && !LLFolderType::lookupIsProtectedType(cat->getPreferredType()) &&
        cat->getOwnerID() == gAgent.getID())
    {
        return true;
    }
    return false;
}

void show_task_item_profile(const LLUUID& item_uuid, const LLUUID& object_id)
{
    LLSD params;
    params["id"] = item_uuid;
    params["object"] = object_id;

    LLFloaterReg::showInstance("item_properties", params);
}

void show_item_profile(const LLUUID& item_uuid)
{
    LLUUID linked_uuid = gInventory.getLinkedItemID(item_uuid);
    LLFloaterReg::showInstance("item_properties", LLSD().with("id", linked_uuid));
}

void show_item_original(const LLUUID& item_uuid)
{
    static LLUICachedControl<bool> find_original_new_floater("FindOriginalOpenWindow", false);

    //show in a new single-folder window
    if(find_original_new_floater)
    {
        const LLUUID& linked_item_uuid = gInventory.getLinkedItemID(item_uuid);
        const LLInventoryObject *obj = gInventory.getObject(linked_item_uuid);
        if (obj && obj->getParentUUID().notNull())
        {
            LLPanelMainInventory::newFolderWindow(obj->getParentUUID(), linked_item_uuid);
        }
    }
    //show in main Inventory
    else
    {
        LLFloater* floater_inventory = LLFloaterReg::getInstance("inventory");
    if (!floater_inventory)
    {
        LL_WARNS() << "Could not find My Inventory floater" << LL_ENDL;
        return;
    }
    LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
    if (sidepanel_inventory)
    {
        LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
        if (main_inventory)
        {
            if(main_inventory->isSingleFolderMode())
            {
                main_inventory->toggleViewMode();
            }
            main_inventory->resetAllItemsFilters();
        }
        reset_inventory_filter();

        if (!LLFloaterReg::getTypedInstance<LLFloaterSidePanelContainer>("inventory")->isInVisibleChain())
        {
            LLFloaterReg::toggleInstanceOrBringToFront("inventory");
        }

        const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX);
        if (gInventory.isObjectDescendentOf(gInventory.getLinkedItemID(item_uuid), inbox_id))
        {
            if (sidepanel_inventory->getInboxPanel())
            {
                sidepanel_inventory->openInbox();
                sidepanel_inventory->getInboxPanel()->setSelection(gInventory.getLinkedItemID(item_uuid), TAKE_FOCUS_YES);
            }
        }
        else
        {
            sidepanel_inventory->selectAllItemsPanel();
            if (sidepanel_inventory->getActivePanel())
            {
                sidepanel_inventory->getActivePanel()->setSelection(gInventory.getLinkedItemID(item_uuid), TAKE_FOCUS_YES);
            }
        }
    }
    }
}


void reset_inventory_filter()
{
    LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
    if (sidepanel_inventory)
    {
        LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
        if (main_inventory)
        {
            main_inventory->onFilterEdit("");
        }
    }
}

void open_marketplace_listings()
{
    LLFloaterReg::showInstance("marketplace_listings");
}

///----------------------------------------------------------------------------
// Marketplace functions
//
// Handles Copy and Move to or within the Marketplace listings folder.
// Handles creation of stock folders, nesting of listings and version folders,
// permission checking and listings validation.
///----------------------------------------------------------------------------

S32 depth_nesting_in_marketplace(LLUUID cur_uuid)
{
    // Get the marketplace listings root, exit with -1 (i.e. not under the marketplace listings root) if none
    // Todo: findCategoryUUIDForType is somewhat expensive with large
    // flat root folders yet we use depth_nesting_in_marketplace at
    // every turn, find a way to correctly cache this id.
    const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    if (marketplace_listings_uuid.isNull())
    {
        return -1;
    }
    // If not a descendant of the marketplace listings root, then the nesting depth is -1 by definition
    if (!gInventory.isObjectDescendentOf(cur_uuid, marketplace_listings_uuid))
    {
        return -1;
    }

    // Iterate through the parents till we hit the marketplace listings root
    // Note that the marketplace listings root itself will return 0
    S32 depth = 0;
    LLInventoryObject* cur_object = gInventory.getObject(cur_uuid);
    while (cur_uuid != marketplace_listings_uuid)
    {
        depth++;
        cur_uuid = cur_object->getParentUUID();
        cur_object = gInventory.getCategory(cur_uuid);
    }
    return depth;
}

// Returns the UUID of the marketplace listing this object is in
LLUUID nested_parent_id(LLUUID cur_uuid, S32 depth)
{
    if (depth < 1)
    {
        // For objects outside the marketplace listings root (or root itself), we return a NULL UUID
        return LLUUID::null;
    }
    else if (depth == 1)
    {
        // Just under the root, we return the passed UUID itself if it's a folder, NULL otherwise (not a listing)
        LLViewerInventoryCategory* cat = gInventory.getCategory(cur_uuid);
        return (cat ? cur_uuid : LLUUID::null);
    }

    // depth > 1
    LLInventoryObject* cur_object = gInventory.getObject(cur_uuid);
    while (depth > 1)
    {
        depth--;
        cur_uuid = cur_object->getParentUUID();
        cur_object = gInventory.getCategory(cur_uuid);
    }
    return cur_uuid;
}

S32 compute_stock_count(LLUUID cat_uuid, bool force_count /* false */)
{
    // Handle the case of the folder being a stock folder immediately
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_uuid);
    if (!cat)
    {
        // Not a category so no stock count to speak of
        return COMPUTE_STOCK_INFINITE;
    }
    if (cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
    {
        if (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
        {
            // If the folder is not completely fetched, we do not want to return any confusing value that could lead to unlisting
            // "COMPUTE_STOCK_NOT_EVALUATED" denotes that a stock folder has a count that cannot be evaluated at this time (folder not up to date)
            return COMPUTE_STOCK_NOT_EVALUATED;
        }
        // Note: stock folders are *not* supposed to have nested subfolders so we stop recursion here but we count only items (subfolders will be ignored)
        // Note: we *always* give a stock count for stock folders, it's useful even if the listing is unassociated
        LLInventoryModel::cat_array_t* cat_array;
        LLInventoryModel::item_array_t* item_array;
        gInventory.getDirectDescendentsOf(cat_uuid,cat_array,item_array);
        return static_cast<S32>(item_array->size());
    }

    // When force_count is true, we do not do any verification of the marketplace status and simply compute
    // the stock amount based on the descendent hierarchy. This is used specifically when creating a listing.
    if (!force_count)
    {
        // Grab marketplace data for this folder
        S32 depth = depth_nesting_in_marketplace(cat_uuid);
        LLUUID listing_uuid = nested_parent_id(cat_uuid, depth);
        if (!LLMarketplaceData::instance().isListed(listing_uuid))
        {
            // If not listed, the notion of stock is meaningless so it won't be computed for any level
            return COMPUTE_STOCK_INFINITE;
        }

        LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolder(listing_uuid);
        // Handle the case of the first 2 levels : listing and version folders
        if (depth == 1)
        {
            if (version_folder_uuid.notNull())
            {
                // If there is a version folder, the stock value for the listing is the version folder stock
                return compute_stock_count(version_folder_uuid, true);
            }
            else
            {
                // If there's no version folder associated, the notion of stock count has no meaning
                return COMPUTE_STOCK_INFINITE;
            }
        }
        else if (depth == 2)
        {
            if (version_folder_uuid.notNull() && (version_folder_uuid != cat_uuid))
            {
                // If there is a version folder but we're not it, our stock count is meaningless
                return COMPUTE_STOCK_INFINITE;
            }
        }
    }

    // In all other cases, the stock count is the min of stock folders count found in the descendents
    // "COMPUTE_STOCK_NOT_EVALUATED" denotes that a stock folder in the hierarchy has a count that cannot be evaluated at this time (folder not up to date)
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(cat_uuid,cat_array,item_array);

    // "COMPUTE_STOCK_INFINITE" denotes a folder that doesn't countain any stock folders in its descendents
    S32 curr_count = COMPUTE_STOCK_INFINITE;

    // Note: marketplace listings have a maximum depth nesting of 4
    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        S32 count = compute_stock_count(category->getUUID(), true);
        if ((curr_count == COMPUTE_STOCK_INFINITE) || ((count != COMPUTE_STOCK_INFINITE) && (count < curr_count)))
        {
            curr_count = count;
        }
    }

    return curr_count;
}

// local helper
bool can_move_to_marketplace(LLInventoryItem* inv_item, std::string& tooltip_msg, bool resolve_links)
{
    // Collapse links directly to items/folders
    LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
    LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
    LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();

    // Linked items and folders cannot be put for sale
    if (linked_category || linked_item)
    {
        tooltip_msg = LLTrans::getString("TooltipOutboxLinked");
        return false;
    }

    // A category is always considered as passing...
    if (linked_category != NULL)
    {
        return true;
    }

    // Take the linked item if necessary
    if (linked_item != NULL)
    {
        inv_item = linked_item;
    }

    // Check that the agent has transfer permission on the item: this is required as a resident cannot
    // put on sale items she cannot transfer. Proceed with move if we have permission.
    bool allow_transfer = inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());
    if (!allow_transfer)
    {
        tooltip_msg = LLTrans::getString("TooltipOutboxNoTransfer");
        return false;
    }

    // Check worn/not worn status: worn items cannot be put on the marketplace
    bool worn = get_is_item_worn(inv_item->getUUID());
    if (worn)
    {
        tooltip_msg = LLTrans::getString("TooltipOutboxWorn");
        return false;
    }

    // Check library status: library items cannot be put on the marketplace
    if (!gInventory.isObjectDescendentOf(inv_item->getUUID(), gInventory.getRootFolderID()))
    {
        tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
        return false;
    }

    // Check type: for the moment, calling cards cannot be put on the marketplace
    bool calling_card = (LLAssetType::AT_CALLINGCARD == inv_item->getType());
    if (calling_card)
    {
        tooltip_msg = LLTrans::getString("TooltipOutboxCallingCard");
        return false;
    }

    return true;
}

// local helper
// Returns the max tree length (in folder nodes) down from the argument folder
int get_folder_levels(LLInventoryCategory* inv_cat)
{
    LLInventoryModel::cat_array_t* cats;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(inv_cat->getUUID(), cats, items);

    int max_child_levels = 0;

    for (S32 i=0; i < cats->size(); ++i)
    {
        LLInventoryCategory* category = cats->at(i);
        max_child_levels = llmax(max_child_levels, get_folder_levels(category));
    }

    return 1 + max_child_levels;
}

// local helper
// Returns the distance (in folder nodes) between the ancestor and its descendant. Returns -1 if not related.
int get_folder_path_length(const LLUUID& ancestor_id, const LLUUID& descendant_id)
{
    int depth = 0;

    if (ancestor_id == descendant_id) return depth;

    const LLInventoryCategory* category = gInventory.getCategory(descendant_id);

    while (category)
    {
        LLUUID parent_id = category->getParentUUID();

        if (parent_id.isNull()) break;

        depth++;

        if (parent_id == ancestor_id) return depth;

        category = gInventory.getCategory(parent_id);
    }

    LL_WARNS("SLM") << "get_folder_path_length() couldn't trace a path from the descendant to the ancestor" << LL_ENDL;
    return -1;
}

// local helper
// Returns true if all items within the argument folder are fit for sale, false otherwise
bool has_correct_permissions_for_sale(LLInventoryCategory* cat, std::string& error_msg)
{
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);

    LLInventoryModel::item_array_t item_array_copy = *item_array;

    for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
    {
        LLInventoryItem* item = *iter;
        if (!can_move_to_marketplace(item, error_msg, false))
        {
            return false;
        }
    }

    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;

    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        if (!has_correct_permissions_for_sale(category, error_msg))
        {
            return false;
        }
    }
    return true;
}

// Returns true if inv_item can be dropped in dest_folder, a folder nested in marketplace listings (or merchant inventory) under the root_folder root
// If returns is false, tooltip_msg contains an error message to display to the user (localized and all).
// bundle_size is the amount of sibling items that are getting moved to the marketplace at the same time.
bool can_move_item_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryItem* inv_item, std::string& tooltip_msg, S32 bundle_size, bool from_paste)
{
    // Check stock folder type matches item type in marketplace listings or merchant outbox (even if of no use there for the moment)
    LLViewerInventoryCategory* view_folder = dynamic_cast<LLViewerInventoryCategory*>(dest_folder);
    bool move_in_stock = (view_folder && (view_folder->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK));
    bool accept = (view_folder && view_folder->acceptItem(inv_item));
    if (!accept)
    {
        tooltip_msg = LLTrans::getString("TooltipOutboxMixedStock");
    }

    // Check that the item has the right type and permissions to be sold on the marketplace
    if (accept)
    {
        accept = can_move_to_marketplace(inv_item, tooltip_msg, true);
    }

    // Check that the total amount of items won't violate the max limit on the marketplace
    if (accept)
    {
        // If the dest folder is a stock folder, we do not count the incoming items toward the total (stock items are seen as one)
        unsigned int existing_item_count = (move_in_stock ? 0 : bundle_size);

        // If the dest folder is a stock folder, we do assume that the incoming items are also stock items (they should anyway)
        unsigned int existing_stock_count = (move_in_stock ? bundle_size : 0);

        unsigned int existing_folder_count = 0;

        // Get the version folder: that's where the counts start from
        const LLViewerInventoryCategory * version_folder = ((root_folder && (root_folder != dest_folder)) ? gInventory.getFirstDescendantOf(root_folder->getUUID(), dest_folder->getUUID()) : NULL);

        if (version_folder)
        {
            if (!from_paste && gInventory.isObjectDescendentOf(inv_item->getUUID(), version_folder->getUUID()))
            {
                // Clear those counts or they will be counted twice because we're already inside the version category
                existing_item_count = 0;
            }

            LLInventoryModel::cat_array_t existing_categories;
            LLInventoryModel::item_array_t existing_items;

            gInventory.collectDescendents(version_folder->getUUID(), existing_categories, existing_items, false);

            existing_item_count += count_copyable_items(existing_items) + count_stock_folders(existing_categories);
            existing_stock_count += count_stock_items(existing_items);
            existing_folder_count += static_cast<S32>(existing_categories.size());

            // If the incoming item is a nocopy (stock) item, we need to consider that it will create a stock folder
            if (!inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()) && !move_in_stock)
            {
                // Note : we do not assume that all incoming items are nocopy of different kinds...
                existing_folder_count += 1;
            }
        }

        if (existing_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxItemCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects", args);
            accept = false;
        }
        else if (existing_stock_count > gSavedSettings.getU32("InventoryOutboxMaxStockItemCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxStockItemCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyStockItems", args);
            accept = false;
        }
        else if (existing_folder_count > gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyFolders", args);
            accept = false;
        }
    }

    return accept;
}

// Returns true if inv_cat can be dropped in dest_folder, a folder nested in marketplace listings (or merchant inventory) under the root_folder root
// If returns is false, tooltip_msg contains an error message to display to the user (localized and all).
// bundle_size is the amount of sibling items that are getting moved to the marketplace at the same time.
bool can_move_folder_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryCategory* inv_cat, std::string& tooltip_msg, S32 bundle_size, bool check_items, bool from_paste)
{
    bool accept = true;

    // Compute the nested folders level we'll add into with that incoming folder
    int incoming_folder_depth = get_folder_levels(inv_cat);
    // Compute the nested folders level we're inserting ourselves in
    // Note: add 1 when inserting under a listing folder as we need to take the root listing folder in the count
    int insertion_point_folder_depth = (root_folder ? get_folder_path_length(root_folder->getUUID(), dest_folder->getUUID()) + 1 : 1);

    // Get the version folder: that's where the folders and items counts start from
    const LLViewerInventoryCategory * version_folder = (insertion_point_folder_depth >= 2 ? gInventory.getFirstDescendantOf(root_folder->getUUID(), dest_folder->getUUID()) : NULL);

    // Compare the whole with the nested folders depth limit
    // Note: substract 2 as we leave root and version folder out of the count threshold
    if ((incoming_folder_depth + insertion_point_folder_depth - 2) > (S32)(gSavedSettings.getU32("InventoryOutboxMaxFolderDepth")))
    {
        LLStringUtil::format_map_t args;
        U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderDepth");
        args["[AMOUNT]"] = llformat("%d",amount);
        tooltip_msg = LLTrans::getString("TooltipOutboxFolderLevels", args);
        accept = false;
    }

    if (accept)
    {
        LLInventoryModel::cat_array_t descendent_categories;
        LLInventoryModel::item_array_t descendent_items;
        gInventory.collectDescendents(inv_cat->getUUID(), descendent_categories, descendent_items, false);

        int dragged_folder_count = static_cast<int>(descendent_categories.size()) + bundle_size;  // Note: We assume that we're moving a bunch of folders in. That might be wrong...
        int dragged_item_count = count_copyable_items(descendent_items) + count_stock_folders(descendent_categories);
        int dragged_stock_count = count_stock_items(descendent_items);
        int existing_item_count = 0;
        int existing_stock_count = 0;
        int existing_folder_count = 0;

        if (version_folder)
        {
            if (!from_paste && gInventory.isObjectDescendentOf(inv_cat->getUUID(), version_folder->getUUID()))
            {
                // Clear those counts or they will be counted twice because we're already inside the version category
                dragged_folder_count = 0;
                dragged_item_count = 0;
                dragged_stock_count = 0;
            }

            // Tally the total number of categories and items inside the root folder
            LLInventoryModel::cat_array_t existing_categories;
            LLInventoryModel::item_array_t existing_items;
            gInventory.collectDescendents(version_folder->getUUID(), existing_categories, existing_items, false);

            existing_folder_count += static_cast<int>(existing_categories.size());
            existing_item_count += count_copyable_items(existing_items) + count_stock_folders(existing_categories);
            existing_stock_count += count_stock_items(existing_items);
        }

        const unsigned int total_folder_count = existing_folder_count + dragged_folder_count;
        const unsigned int total_item_count = existing_item_count + dragged_item_count;
        const unsigned int total_stock_count = existing_stock_count + dragged_stock_count;

        if (total_folder_count > gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyFolders", args);
            accept = false;
        }
        else if (total_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxItemCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects", args);
            accept = false;
        }
        else if (total_stock_count > gSavedSettings.getU32("InventoryOutboxMaxStockItemCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxStockItemCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyStockItems", args);
            accept = false;
        }

        // Now check that each item in the folder can be moved in the marketplace
        if (accept && check_items)
        {
            for (S32 i=0; i < descendent_items.size(); ++i)
            {
                LLInventoryItem* item = descendent_items[i];
                if (!can_move_to_marketplace(item, tooltip_msg, false))
                {
                    accept = false;
                    break;
                }
            }
        }
    }

    return accept;
}

// Can happen asynhroneously!!!
bool move_item_to_marketplacelistings(LLInventoryItem* inv_item, LLUUID dest_folder, bool copy)
{
    // Get the marketplace listings depth of the destination folder, exit with error if not under marketplace
    S32 depth = depth_nesting_in_marketplace(dest_folder);
    if (depth < 0)
    {
        LLSD subs;
        subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Merchant");
        LLNotificationsUtil::add("MerchantPasteFailed", subs);
        return false;
    }

    // We will collapse links into items/folders
    LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
    LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();

    if (linked_category != NULL)
    {
        // Move the linked folder directly
        return move_folder_to_marketplacelistings(linked_category, dest_folder, copy);
    }
    else
    {
        // Grab the linked item if any
        LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
        viewer_inv_item = (linked_item != NULL ? linked_item : viewer_inv_item);

        // If we want to copy but the item is no copy, fail silently (this is a common case that doesn't warrant notification)
        if (copy && !viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            return false;
        }

        // Check that the agent has transfer permission on the item: this is required as a resident cannot
        // put on sale items she cannot transfer. Proceed with move if we have permission.
        std::string error_msg;
        if (can_move_to_marketplace(inv_item, error_msg, true))
        {
            // When moving an isolated item, we might need to create the folder structure to support it

            LLUUID item_id = inv_item->getUUID();
            std::function<void(const LLUUID&)> callback_create_stock = [copy, item_id](const LLUUID& new_cat_id)
            {
                if (new_cat_id.isNull())
                {
                    LL_WARNS() << "Failed to create category" << LL_ENDL;
                    LLSD subs;
                    subs["[ERROR_CODE]"] =
                        LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
                    LLNotificationsUtil::add("MerchantPasteFailed", subs);
                    return;
                }

                // Verify we can have this item in that destination category
                LLViewerInventoryCategory* dest_cat = gInventory.getCategory(new_cat_id);
                LLViewerInventoryItem * viewer_inv_item = gInventory.getItem(item_id);
                if (!dest_cat || !viewer_inv_item)
                {
                    LL_WARNS() << "Move to marketplace: item or folder do not exist" << LL_ENDL;

                    LLSD subs;
                    subs["[ERROR_CODE]"] =
                        LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
                    LLNotificationsUtil::add("MerchantPasteFailed", subs);
                    return;
                }
                if (!dest_cat->acceptItem(viewer_inv_item))
                {
                    LLSD subs;
                    subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
                    LLNotificationsUtil::add("MerchantPasteFailed", subs);
                }

                if (copy)
                {
                    // Copy the item
                    LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(update_folder_cb, new_cat_id));
                    copy_inventory_item(
                        gAgent.getID(),
                        viewer_inv_item->getPermissions().getOwner(),
                        viewer_inv_item->getUUID(),
                        new_cat_id,
                        std::string(),
                        cb);
                }
                else
                {
                    // Reparent the item
                    gInventory.changeItemParent(viewer_inv_item, new_cat_id, true);
                }
            };

            std::function<void(const LLUUID&)> callback_dest_create = [item_id, callback_create_stock](const LLUUID& new_cat_id)
            {
                if (new_cat_id.isNull())
                {
                    LL_WARNS() << "Failed to create category" << LL_ENDL;
                    LLSD subs;
                    subs["[ERROR_CODE]"] =
                        LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
                    LLNotificationsUtil::add("MerchantPasteFailed", subs);
                    return;
                }

                LLViewerInventoryCategory* dest_cat = gInventory.getCategory(new_cat_id);
                LLViewerInventoryItem * viewer_inv_item = gInventory.getItem(item_id);
                if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()) &&
                    (dest_cat->getPreferredType() != LLFolderType::FT_MARKETPLACE_STOCK))
                {
                    // We need to create a stock folder to move a no copy item
                    gInventory.createNewCategory(new_cat_id, LLFolderType::FT_MARKETPLACE_STOCK, viewer_inv_item->getName(), callback_create_stock);
                }
                else
                {
                    callback_create_stock(new_cat_id);
                }
            };

            if (depth == 0)
            {
                // We need a listing folder
               gInventory.createNewCategory(dest_folder,
                                            LLFolderType::FT_NONE,
                                            viewer_inv_item->getName(),
                                            [callback_dest_create](const LLUUID &new_cat_id)
                                            {
                                                if (new_cat_id.isNull())
                                                {
                                                    LL_WARNS() << "Failed to create listing folder for marketpace" << LL_ENDL;
                                                    return;
                                                }
                                                LLViewerInventoryCategory *dest_cat = gInventory.getCategory(new_cat_id);
                                                if (!dest_cat)
                                                {
                                                    LL_WARNS() << "Failed to find freshly created listing folder" << LL_ENDL;
                                                    return;
                                                }
                                                // version folder
                                                gInventory.createNewCategory(new_cat_id,
                                                                             LLFolderType::FT_NONE,
                                                                             dest_cat->getName(),
                                                                             callback_dest_create);
                                            });
            }
            else if (depth == 1)
            {
                // We need a version folder
                gInventory.createNewCategory(dest_folder, LLFolderType::FT_NONE, viewer_inv_item->getName(), callback_dest_create);
            }
            else
            {
                callback_dest_create(dest_folder);
            }
        }
        else
        {
            LLSD subs;
            subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + error_msg;
            LLNotificationsUtil::add("MerchantPasteFailed", subs);
            return false;
        }
    }

    open_marketplace_listings();
    return true;
}

bool move_folder_to_marketplacelistings(LLInventoryCategory* inv_cat, const LLUUID& dest_folder, bool copy, bool move_no_copy_items)
{
    // Check that we have adequate permission on all items being moved. Proceed if we do.
    std::string error_msg;
    if (has_correct_permissions_for_sale(inv_cat, error_msg))
    {
        // Get the destination folder
        LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);

        // Check it's not a stock folder
        if (dest_cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
        {
            LLSD subs;
            subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
            LLNotificationsUtil::add("MerchantPasteFailed", subs);
            return false;
        }

        // Get the parent folder of the moved item : we may have to update it
        LLUUID src_folder = inv_cat->getParentUUID();

        LLViewerInventoryCategory * viewer_inv_cat = (LLViewerInventoryCategory *) inv_cat;
        if (copy)
        {
            // Copy the folder
            copy_inventory_category(&gInventory, viewer_inv_cat, dest_folder, LLUUID::null, move_no_copy_items);
        }
        else
        {
            LL_INFOS("SLM") << "Move category " << make_info(viewer_inv_cat) << " to '" << make_inventory_path(dest_folder) << "'" << LL_ENDL;
            // Reparent the folder
            gInventory.changeCategoryParent(viewer_inv_cat, dest_folder, false);
            // Check the destination folder recursively for no copy items and promote the including folders if any
            LLMarketplaceValidator::getInstance()->validateMarketplaceListings(dest_folder);
        }

        // Update the modified folders
        update_marketplace_category(src_folder);
        update_marketplace_category(dest_folder);
        gInventory.notifyObservers();
    }
    else
    {
        LLSD subs;
        subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + error_msg;
        LLNotificationsUtil::add("MerchantPasteFailed", subs);
        return false;
    }

    open_marketplace_listings();
    return true;
}

bool sort_alpha(const LLViewerInventoryCategory* cat1, const LLViewerInventoryCategory* cat2)
{
    return cat1->getName().compare(cat2->getName()) < 0;
}

// Make all relevant business logic checks on the marketplace listings starting with the folder as argument.
// This function does no deletion of listings but a mere audit and raises issues to the user (through the
// optional callback cb).
// The only inventory changes that are done is to move and sort folders containing no-copy items to stock folders.
// @pending_callbacks - how many callbacks we are waiting for, must be inited before use
// @result - true if things validate, false if issues are raised, must be inited before use
typedef boost::function<void(S32 pending_callbacks, bool result)> validation_result_callback_t;
void validate_marketplacelistings(
    LLInventoryCategory* cat,
    validation_result_callback_t cb_result,
    LLMarketplaceValidator::validation_msg_callback_t cb_msg,
    bool fix_hierarchy,
    S32 depth,
    bool notify_observers,
    S32 &pending_callbacks,
    bool &result)
{
    // Get the type and the depth of the folder
    LLViewerInventoryCategory * viewer_cat = (LLViewerInventoryCategory *) (cat);
    const LLFolderType::EType folder_type = cat->getPreferredType();
    if (depth < 0)
    {
        // If the depth argument was not provided, evaluate the depth directly
        depth = depth_nesting_in_marketplace(cat->getUUID());
    }
    if (depth < 0)
    {
        // If the folder is not under the marketplace listings root, we run validation as if it was a listing folder and prevent any hierarchy fix
        // This allows the function to be used to pre-validate a folder anywhere in the inventory
        depth = 1;
        fix_hierarchy = false;
    }

    // Set the indentation for print output (typically, audit button in marketplace folder floater)
    std::string indent;
    for (int i = 1; i < depth; i++)
    {
        indent += "    ";
    }

    // Check out that version folders are marketplace ready
    if (depth == 2)
    {
        std::string message;
        // Note: if we fix the hierarchy, we want to check the items individually, hence the last argument here
        if (!can_move_folder_to_marketplace(cat, cat, cat, message, 0, fix_hierarchy))
        {
            result = false;
            if (cb_msg)
            {
                message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error") + " " + message;
                cb_msg(message,depth,LLError::LEVEL_ERROR);
            }
        }
    }

    // Check out that stock folders are at the right level
    if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth <= 2))
    {
        if (fix_hierarchy)
        {
            if (cb_msg)
            {
                std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Warning") + " " + LLTrans::getString("Marketplace Validation Warning Stock");
                cb_msg(message,depth,LLError::LEVEL_WARN);
            }

            // Nest the stock folder one level deeper in a normal folder and restart from there
            pending_callbacks++;
            LLUUID parent_uuid = cat->getParentUUID();
            LLUUID cat_uuid = cat->getUUID();
            gInventory.createNewCategory(parent_uuid,
                LLFolderType::FT_NONE,
                cat->getName(),
                [cat_uuid, cb_result, cb_msg, fix_hierarchy, depth](const LLUUID &new_cat_id)
            {
                if (new_cat_id.isNull())
                {
                    cb_result(0, false);
                    return;
                }
                LLInventoryCategory * move_cat = gInventory.getCategory(cat_uuid);
                LLViewerInventoryCategory * viewer_cat = (LLViewerInventoryCategory *)(move_cat);
                LLInventoryCategory * new_cat = gInventory.getCategory(new_cat_id);
                gInventory.changeCategoryParent(viewer_cat, new_cat_id, false);
                S32 pending = 0;
                bool result = true;
                validate_marketplacelistings(new_cat, cb_result, cb_msg, fix_hierarchy, depth + 1, true, pending, result);
                cb_result(pending, result);
            }
            );
            result = false;
            return;
        }
        else
        {
            result = false;
            if (cb_msg)
            {
                std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error") + " " + LLTrans::getString("Marketplace Validation Warning Stock");
                cb_msg(message,depth,LLError::LEVEL_ERROR);
            }
        }
    }

    // Item sorting and validation : sorting and moving the various stock items is complicated as the set of constraints is high
    // We need to:
    // * separate non stock items, stock items per types in different folders
    // * have stock items nested at depth 2 at least
    // * never ever move the non-stock items

    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);

    // We use a composite (type,permission) key on that map to store UUIDs of items of same (type,permissions)
    std::map<U32, std::vector<LLUUID> > items_vector;

    // Parse the items and create vectors of item UUIDs sorting copyable items and stock items of various types
    bool has_bad_items = false;
    LLInventoryModel::item_array_t item_array_copy = *item_array;
    for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
    {
        LLInventoryItem* item = *iter;
        LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) item;

        // Test but skip items that shouldn't be there to start with, raise an error message for those
        std::string error_msg;
        if (!can_move_to_marketplace(item, error_msg, false))
        {
            has_bad_items = true;
            if (cb_msg && fix_hierarchy)
            {
                std::string message = indent + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Error") + " " + error_msg;
                cb_msg(message,depth,LLError::LEVEL_ERROR);
            }
            continue;
        }
        // Update the appropriate vector item for that type
        LLInventoryType::EType type = LLInventoryType::IT_COUNT;    // Default value for non stock items
        U32 perms = 0;
        if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            // Get the item type for stock items
            type = viewer_inv_item->getInventoryType();
            perms = viewer_inv_item->getPermissions().getMaskNextOwner();
        }
        U32 key = (((U32)(type) & 0xFF) << 24) | (perms & 0xFFFFFF);
        items_vector[key].push_back(viewer_inv_item->getUUID());
    }

    // How many types of items? Which type is it if only one?
    auto count = items_vector.size();
    U32 default_key = (U32)(LLInventoryType::IT_COUNT) << 24; // This is the key for any normal copyable item
    U32 unique_key = (count == 1 ? items_vector.begin()->first : default_key); // The key in the case of one item type only

    // If we have no items in there (only folders or empty), analyze a bit further
    if ((count == 0) && !has_bad_items)
    {
        if (cat_array->size() == 0)
        {
            // So we have no item and no folder. That's at least a warning.
            if (depth == 2)
            {
                // If this is an empty version folder, warn only (listing won't be delivered by AIS, but only AIS should unlist)
                if (cb_msg)
                {
                    std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Empty Version");
                    cb_msg(message,depth,LLError::LEVEL_WARN);
                }
            }
            else if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth > 2))
            {
                // If this is a legit but empty stock folder, warn only (listing must stay searchable when out of stock)
                if (cb_msg)
                {
                    std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Empty Stock");
                    cb_msg(message,depth,LLError::LEVEL_WARN);
                }
            }
            else if (cb_msg)
            {
                // We warn if there's nothing in a regular folder (may be it's an under construction listing)
                std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Warning Empty");
                cb_msg(message,depth,LLError::LEVEL_WARN);
            }
        }
        else
        {
            // Done with that folder : Print out the folder name unless we already found an error here
            if (cb_msg && result && (depth >= 1))
            {
                std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Log");
                cb_msg(message,depth,LLError::LEVEL_INFO);
            }
        }
    }
    // If we have a single type of items of the right type in the right place, we're done
    else if ((count == 1) && !has_bad_items && (((unique_key == default_key) && (depth > 1)) || ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth > 2) && (cat_array->size() == 0))))
    {
        // Done with that folder : Print out the folder name unless we already found an error here
        if (cb_msg && result && (depth >= 1))
        {
            std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Log");
            cb_msg(message,depth,LLError::LEVEL_INFO);
        }
    }
    else
    {
        if (fix_hierarchy && !has_bad_items)
        {
            // Alert the user when an existing stock folder has to be split
            if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && ((count >= 2) || (cat_array->size() > 0)))
            {
                LLNotificationsUtil::add("AlertMerchantStockFolderSplit");
            }
            // If we have more than 1 type of items or we are at the listing level or we have stock/no stock type mismatch, wrap the items in subfolders
            if ((count > 1) || (depth == 1) ||
                ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (unique_key == default_key)) ||
                ((folder_type != LLFolderType::FT_MARKETPLACE_STOCK) && (unique_key != default_key)))
            {
                // Create one folder per vector at the right depth and of the right type
                std::map<U32, std::vector<LLUUID> >::iterator items_vector_it = items_vector.begin();
                while (items_vector_it != items_vector.end())
                {
                    // Create a new folder
                    const LLUUID parent_uuid = (depth > 2 ? viewer_cat->getParentUUID() : viewer_cat->getUUID());
                    const LLUUID origin_uuid = viewer_cat->getUUID();
                    LLViewerInventoryItem* viewer_inv_item = gInventory.getItem(items_vector_it->second.back());
                    std::string folder_name = (depth >= 1 ? viewer_cat->getName() : viewer_inv_item->getName());
                    LLFolderType::EType new_folder_type = (items_vector_it->first == default_key ? LLFolderType::FT_NONE : LLFolderType::FT_MARKETPLACE_STOCK);
                    if (cb_msg)
                    {
                        std::string message = "";
                        if (new_folder_type == LLFolderType::FT_MARKETPLACE_STOCK)
                        {
                            message = indent + folder_name + LLTrans::getString("Marketplace Validation Warning Create Stock");
                        }
                        else
                        {
                            message = indent + folder_name + LLTrans::getString("Marketplace Validation Warning Create Version");
                        }
                        cb_msg(message,depth,LLError::LEVEL_WARN);
                    }

                    pending_callbacks++;
                    std::vector<LLUUID> uuid_vector = items_vector_it->second; // needs to be a copy for lambda
                    gInventory.createNewCategory(
                        parent_uuid,
                        new_folder_type,
                        folder_name,
                        [uuid_vector, cb_result, cb_msg, depth, parent_uuid, origin_uuid, notify_observers](const LLUUID &new_category_id)
                    {
                        // Move each item to the new folder
                        std::vector<LLUUID>::const_reverse_iterator iter = uuid_vector.rbegin();
                        while (iter != uuid_vector.rend())
                        {
                            LLViewerInventoryItem* viewer_inv_item = gInventory.getItem(*iter);
                            if (cb_msg)
                            {
                                std::string indent;
                                for (int i = 1; i < depth; i++)
                                {
                                    indent += "    ";
                                }
                                std::string message = indent + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Warning Move");
                                cb_msg(message, depth, LLError::LEVEL_WARN);
                            }
                            gInventory.changeItemParent(viewer_inv_item, new_category_id, true);
                            iter++;
                        }

                        if (origin_uuid != parent_uuid)
                        {
                            // We might have moved last item from a folder, check if it needs to be removed
                            LLViewerInventoryCategory* cat = gInventory.getCategory(origin_uuid);
                            if (cat->getDescendentCount() == 0)
                            {
                                // Remove previous folder if it ends up empty
                                if (cb_msg)
                                {
                                    std::string indent;
                                    for (int i = 1; i < depth; i++)
                                    {
                                        indent += "    ";
                                    }
                                    std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Warning Delete");
                                    cb_msg(message, depth, LLError::LEVEL_WARN);
                                }
                                gInventory.removeCategory(cat->getUUID());
                                if (notify_observers)
                                {
                                    gInventory.notifyObservers();
                                }
                            }
                        }

                        // Next type
                        update_marketplace_category(parent_uuid);
                        update_marketplace_category(new_category_id);
                        if (notify_observers)
                        {
                            gInventory.notifyObservers();
                        }
                        cb_result(0, true);
                    }
                    );
                    items_vector_it++;
                }
            }
            // Stock folder should have no sub folder so reparent those up
            if (folder_type == LLFolderType::FT_MARKETPLACE_STOCK)
            {
                LLUUID parent_uuid = cat->getParentUUID();
                gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
                LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
                for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
                {
                    LLViewerInventoryCategory * viewer_cat = (LLViewerInventoryCategory *) (*iter);
                    gInventory.changeCategoryParent(viewer_cat, parent_uuid, false);
                    validate_marketplacelistings(viewer_cat, cb_result, cb_msg, fix_hierarchy, depth, false, pending_callbacks, result);
                }
            }
        }
        else if (cb_msg)
        {
            // We are not fixing the hierarchy but reporting problems, report everything we can find
            // Print the folder name
            if (result && (depth >= 1))
            {
                if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (count >= 2))
                {
                    // Report if a stock folder contains a mix of items
                    result = false;
                    std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Mixed Stock");
                    cb_msg(message,depth,LLError::LEVEL_ERROR);
                }
                else if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (cat_array->size() != 0))
                {
                    // Report if a stock folder contains subfolders
                    result = false;
                    std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Error Subfolder In Stock");
                    cb_msg(message,depth,LLError::LEVEL_ERROR);
                }
                else
                {
                    // Simply print the folder name
                    std::string message = indent + cat->getName() + LLTrans::getString("Marketplace Validation Log");
                    cb_msg(message,depth,LLError::LEVEL_INFO);
                }
            }
            // Scan each item and report if there's a problem
            LLInventoryModel::item_array_t item_array_copy = *item_array;
            for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
            {
                LLInventoryItem* item = *iter;
                LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) item;
                std::string error_msg;
                if (!can_move_to_marketplace(item, error_msg, false))
                {
                    // Report items that shouldn't be there to start with
                    result = false;
                    std::string message = indent + "    " + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Error") + " " + error_msg;
                    cb_msg(message,depth,LLError::LEVEL_ERROR);
                }
                else if ((!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID())) && (folder_type != LLFolderType::FT_MARKETPLACE_STOCK))
                {
                    // Report stock items that are misplaced
                    result = false;
                    std::string message = indent + "    " + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Error Stock Item");
                    cb_msg(message,depth,LLError::LEVEL_ERROR);
                }
                else if (depth == 1)
                {
                    // Report items not wrapped in version folder
                    result = false;
                    std::string message = indent + "    " + viewer_inv_item->getName() + LLTrans::getString("Marketplace Validation Warning Unwrapped Item");
                    cb_msg(message,depth,LLError::LEVEL_ERROR);
                }
            }
        }

        // Clean up
        if (viewer_cat->getDescendentCount() == 0)
        {
            // Remove the current folder if it ends up empty
            if (cb_msg)
            {
                std::string message = indent + viewer_cat->getName() + LLTrans::getString("Marketplace Validation Warning Delete");
                cb_msg(message,depth,LLError::LEVEL_WARN);
            }
            gInventory.removeCategory(cat->getUUID());
            if (notify_observers)
            {
                gInventory.notifyObservers();
            }
            result &=!has_bad_items;
            return;
        }
    }

    // Recursion : Perform the same validation on each nested folder
    gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    // Sort the folders in alphabetical order first
    std::sort(cat_array_copy.begin(), cat_array_copy.end(), sort_alpha);

    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        validate_marketplacelistings(category, cb_result, cb_msg, fix_hierarchy, depth + 1, false, pending_callbacks, result);
    }

    update_marketplace_category(cat->getUUID(), true, true);
    if (notify_observers)
    {
        gInventory.notifyObservers();
    }
    result &= !has_bad_items;
}

void change_item_parent(const LLUUID& item_id, const LLUUID& new_parent_id)
{
    LLInventoryItem* inv_item = gInventory.getItem(item_id);
    if (inv_item)
    {
        LLInventoryModel::update_list_t update;
        LLInventoryModel::LLCategoryUpdate old_folder(inv_item->getParentUUID(), -1);
        update.push_back(old_folder);
        LLInventoryModel::LLCategoryUpdate new_folder(new_parent_id, 1);
        update.push_back(new_folder);
        gInventory.accountForUpdate(update);

        LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(inv_item);
        new_item->setParent(new_parent_id);
        new_item->updateParentOnServer(false);
        gInventory.updateItem(new_item);
        gInventory.notifyObservers();
    }
}

void move_items_to_folder(const LLUUID& new_cat_uuid, const uuid_vec_t& selected_uuids)
{
    for (uuid_vec_t::const_iterator it = selected_uuids.begin(); it != selected_uuids.end(); ++it)
    {
        LLInventoryItem* inv_item = gInventory.getItem(*it);
        if (inv_item)
        {
            change_item_parent(*it, new_cat_uuid);
        }
        else
        {
            LLInventoryCategory* inv_cat = gInventory.getCategory(*it);
            if (inv_cat && !LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()))
            {
                gInventory.changeCategoryParent((LLViewerInventoryCategory*)inv_cat, new_cat_uuid, false);
            }
        }
    }

    LLFloater* floater_inventory = LLFloaterReg::getInstance("inventory");
    if (!floater_inventory)
    {
        LL_WARNS() << "Could not find My Inventory floater" << LL_ENDL;
        return;
    }
    LLSidepanelInventory *sidepanel_inventory = LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
    if (sidepanel_inventory)
    {
        if (sidepanel_inventory->getActivePanel())
        {
            sidepanel_inventory->getActivePanel()->setSelection(new_cat_uuid, TAKE_FOCUS_YES);
            LLFolderViewItem* fv_folder = sidepanel_inventory->getActivePanel()->getItemByID(new_cat_uuid);
            if (fv_folder)
            {
                fv_folder->setOpen(true);
            }
        }
    }
}

bool is_only_cats_selected(const uuid_vec_t& selected_uuids)
{
    for (uuid_vec_t::const_iterator it = selected_uuids.begin(); it != selected_uuids.end(); ++it)
    {
        LLInventoryCategory* inv_cat = gInventory.getCategory(*it);
        if (!inv_cat)
        {
            return false;
        }
    }
    return true;
}

bool is_only_items_selected(const uuid_vec_t& selected_uuids)
{
    for (uuid_vec_t::const_iterator it = selected_uuids.begin(); it != selected_uuids.end(); ++it)
    {
        LLViewerInventoryItem* inv_item = gInventory.getItem(*it);
        if (!inv_item)
        {
            return false;
        }
    }
    return true;
}


void move_items_to_new_subfolder(const uuid_vec_t& selected_uuids, const std::string& folder_name)
{
    LLInventoryObject* first_item = gInventory.getObject(*selected_uuids.begin());
    if (!first_item)
    {
        return;
    }

    inventory_func_type func = boost::bind(&move_items_to_folder, _1, selected_uuids);
    gInventory.createNewCategory(first_item->getParentUUID(), LLFolderType::FT_NONE, folder_name, func);
}

std::string get_category_path(LLUUID cat_id)
{
    LLViewerInventoryCategory *cat = gInventory.getCategory(cat_id);
    std::string localized_cat_name;
    if (!LLTrans::findString(localized_cat_name, "InvFolder " + cat->getName()))
    {
        localized_cat_name = cat->getName();
    }

    if (cat->getParentUUID().notNull())
    {
        return get_category_path(cat->getParentUUID()) + " > " + localized_cat_name;
    }
    else
    {
        return localized_cat_name;
    }
}
// Returns true if the item can be moved to Current Outfit or any outfit folder.
bool can_move_to_outfit(LLInventoryItem* inv_item, bool move_is_into_current_outfit)
{
    LLInventoryType::EType inv_type = inv_item->getInventoryType();
    if ((inv_type != LLInventoryType::IT_WEARABLE) &&
        (inv_type != LLInventoryType::IT_GESTURE) &&
        (inv_type != LLInventoryType::IT_ATTACHMENT) &&
        (inv_type != LLInventoryType::IT_OBJECT) &&
        (inv_type != LLInventoryType::IT_SNAPSHOT) &&
        (inv_type != LLInventoryType::IT_TEXTURE))
    {
        return false;
    }

    U32 flags = inv_item->getFlags();
    if(flags & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
    {
        return false;
    }

    if((inv_type == LLInventoryType::IT_TEXTURE) || (inv_type == LLInventoryType::IT_SNAPSHOT))
    {
        return !move_is_into_current_outfit;
    }

    if (move_is_into_current_outfit && get_is_item_worn(inv_item->getUUID()))
    {
        return false;
    }

    return true;
}

// Returns true if item is a landmark or a link to a landmark
// and can be moved to Favorites or Landmarks folder.
bool can_move_to_landmarks(LLInventoryItem* inv_item)
{
    // Need to get the linked item to know its type because LLInventoryItem::getType()
    // returns actual type AT_LINK for links, not the asset type of a linked item.
    if (LLAssetType::AT_LINK == inv_item->getType())
    {
        LLInventoryItem* linked_item = gInventory.getItem(inv_item->getLinkedUUID());
        if (linked_item)
        {
            return LLAssetType::AT_LANDMARK == linked_item->getType();
        }
    }

    return LLAssetType::AT_LANDMARK == inv_item->getType();
}

// Returns true if folder's content can be moved to Current Outfit or any outfit folder.
bool can_move_to_my_outfits(LLInventoryModel* model, LLInventoryCategory* inv_cat, U32 wear_limit)
{
    LLInventoryModel::cat_array_t *cats;
    LLInventoryModel::item_array_t *items;
    model->getDirectDescendentsOf(inv_cat->getUUID(), cats, items);

    if (items->size() > wear_limit)
    {
        return false;
    }

    if (items->size() == 0)
    {
        // Nothing to move(create)
        return false;
    }

    if (cats->size() > 0)
    {
        // We do not allow subfolders in outfits of "My Outfits" yet
        return false;
    }

    LLInventoryModel::item_array_t::iterator iter = items->begin();
    LLInventoryModel::item_array_t::iterator end = items->end();

    while (iter != end)
    {
        LLViewerInventoryItem *item = *iter;
        if (!can_move_to_outfit(item, false))
        {
            return false;
        }
        iter++;
    }

    return true;
}

std::string get_localized_folder_name(LLUUID cat_uuid)
{
    std::string localized_root_name;
    const LLViewerInventoryCategory* cat = gInventory.getCategory(cat_uuid);
    if (cat)
    {
        LLFolderType::EType preferred_type = cat->getPreferredType();

        // Translation of Accessories folder in Library inventory folder
        bool accessories = false;
        if(cat->getName() == "Accessories")
        {
            const LLUUID& parent_folder_id = cat->getParentUUID();
            accessories = (parent_folder_id == gInventory.getLibraryRootFolderID());
        }

        //"Accessories" inventory category has folder type FT_NONE. So, this folder
        //can not be detected as protected with LLFolderType::lookupIsProtectedType
        localized_root_name.assign(cat->getName());
        if (accessories || LLFolderType::lookupIsProtectedType(preferred_type))
        {
            LLTrans::findString(localized_root_name, std::string("InvFolder ") + cat->getName(), LLSD());
        }
    }

    return localized_root_name;
}

void new_folder_window(const LLUUID& folder_id)
{
    LLPanelMainInventory::newFolderWindow(folder_id);
}

void ungroup_folder_items(const LLUUID& folder_id)
{
    LLInventoryCategory* inv_cat = gInventory.getCategory(folder_id);
    if (!inv_cat || LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()))
    {
        return;
    }
    const LLUUID &new_cat_uuid = inv_cat->getParentUUID();
    LLInventoryModel::cat_array_t* cat_array;
    LLInventoryModel::item_array_t* item_array;
    gInventory.getDirectDescendentsOf(inv_cat->getUUID(), cat_array, item_array);
    LLInventoryModel::cat_array_t cats = *cat_array;
    LLInventoryModel::item_array_t items = *item_array;

    for (const LLPointer<LLViewerInventoryCategory>& cat : cats)
    {
        if (cat)
        {
            gInventory.changeCategoryParent(cat, new_cat_uuid, false);
        }
    }
    for (const LLPointer<LLViewerInventoryItem>& item : items)
    {
        if (item)
        {
            gInventory.changeItemParent(item, new_cat_uuid, false);
        }
    }
    gInventory.removeCategory(inv_cat->getUUID());
    gInventory.notifyObservers();
}

std::string get_searchable_description(LLInventoryModel* model, const LLUUID& item_id)
{
    if (model)
    {
        if (const LLInventoryItem* item = model->getItem(item_id))
        {
            std::string desc = item->getDescription();
            LLStringUtil::toUpper(desc);
            return desc;
        }
    }
    return LLStringUtil::null;
}

std::string get_searchable_creator_name(LLInventoryModel* model, const LLUUID& item_id)
{
    if (model)
    {
        if (const LLInventoryItem* item = model->getItem(item_id))
        {
            LLAvatarName av_name;
            if (LLAvatarNameCache::get(item->getCreatorUUID(), &av_name))
            {
                std::string username = av_name.getUserName();
                LLStringUtil::toUpper(username);
                return username;
            }
        }
    }
    return LLStringUtil::null;
}

std::string get_searchable_UUID(LLInventoryModel* model, const LLUUID& item_id)
{
    if (model)
    {
        const LLViewerInventoryItem *item = model->getItem(item_id);
        if(item && (item->getIsFullPerm() || gAgent.isGodlikeWithoutAdminMenuFakery()))
        {
            std::string uuid = item->getAssetUUID().asString();
            LLStringUtil::toUpper(uuid);
            return uuid;
        }
    }
    return LLStringUtil::null;
}

bool can_share_item(const LLUUID& item_id)
{
    bool can_share = false;

    if (gInventory.isObjectDescendentOf(item_id, gInventory.getRootFolderID()))
    {
            const LLViewerInventoryItem *item = gInventory.getItem(item_id);
            if (item)
            {
                if (LLInventoryCollectFunctor::itemTransferCommonlyAllowed(item))
                {
                    can_share = LLGiveInventory::isInventoryGiveAcceptable(item);
                }
            }
            else
            {
                can_share = (gInventory.getCategory(item_id) != NULL);
            }

            const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
            if ((item_id == trash_id) || gInventory.isObjectDescendentOf(item_id, trash_id))
            {
                can_share = false;
            }
    }

    return can_share;
}
///----------------------------------------------------------------------------
/// LLMarketplaceValidator implementations
///----------------------------------------------------------------------------


LLMarketplaceValidator::LLMarketplaceValidator()
    : mPendingCallbacks(0)
    , mValidationInProgress(false)
{
}

LLMarketplaceValidator::~LLMarketplaceValidator()
{
}

void LLMarketplaceValidator::validateMarketplaceListings(
    const LLUUID &category_id,
    LLMarketplaceValidator::validation_done_callback_t cb_done,
    LLMarketplaceValidator::validation_msg_callback_t cb_msg,
    bool fix_hierarchy,
    S32 depth)
{

    mValidationQueue.emplace(category_id, cb_done, cb_msg, fix_hierarchy, depth);
    if (!mValidationInProgress)
    {
        start();
    }
}

void LLMarketplaceValidator::start()
{
    if (mValidationQueue.empty())
    {
        mValidationInProgress = false;
        return;
    }
    mValidationInProgress = true;

    const ValidationRequest &first = mValidationQueue.front();
    LLViewerInventoryCategory* cat = gInventory.getCategory(first.mCategoryId);
    if (!cat)
    {
        LL_WARNS() << "Tried to validate a folder that doesn't exist" << LL_ENDL;
        if (first.mCbDone)
        {
            first.mCbDone(false);
        }
        mValidationQueue.pop();
        start();
        return;
    }

    validation_result_callback_t result_callback = [](S32 pending, bool result)
    {
        LLMarketplaceValidator* validator = LLMarketplaceValidator::getInstance();
        validator->mPendingCallbacks--; // we just got a callback
        validator->mPendingCallbacks += pending;
        validator->mPendingResult &= result;
        if (validator->mPendingCallbacks <= 0)
        {
            llassert(validator->mPendingCallbacks == 0); // shouldn't be below 0
            const ValidationRequest &first = validator->mValidationQueue.front();
            if (first.mCbDone)
            {
                first.mCbDone(validator->mPendingResult);
            }
            validator->mValidationQueue.pop(); // done;
            validator->start();
        }
    };

    mPendingResult = true;
    mPendingCallbacks = 1; // do '1' in case something decides to callback immediately

    S32 pending_calbacks = 0;
    bool result = true;
    validate_marketplacelistings(
        cat,
        result_callback,
        first.mCbMsg,
        first.mFixHierarchy,
        first.mDepth,
        true,
        pending_calbacks,
        result);

    result_callback(pending_calbacks, result);
}

LLMarketplaceValidator::ValidationRequest::ValidationRequest(
    LLUUID category_id,
    validation_done_callback_t cb_done,
    validation_msg_callback_t cb_msg,
    bool fix_hierarchy,
    S32 depth)
: mCategoryId(category_id)
, mCbDone(cb_done)
, mCbMsg(cb_msg)
, mFixHierarchy(fix_hierarchy)
, mDepth(depth)
{}

///----------------------------------------------------------------------------
/// LLInventoryCollectFunctor implementations
///----------------------------------------------------------------------------

// static
bool LLInventoryCollectFunctor::itemTransferCommonlyAllowed(const LLInventoryItem* item)
{
    if (!item)
        return false;

    switch(item->getType())
    {
        case LLAssetType::AT_OBJECT:
        case LLAssetType::AT_BODYPART:
        case LLAssetType::AT_CLOTHING:
            if (!get_is_item_worn(item->getUUID()))
                return true;
            break;
        default:
            return true;
            break;
    }
    return false;
}

bool LLIsType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    if(mType == LLAssetType::AT_CATEGORY)
    {
        if(cat) return true;
    }
    if(item)
    {
        if(item->getType() == mType) return true;
    }
    return false;
}

bool LLIsNotType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    if(mType == LLAssetType::AT_CATEGORY)
    {
        if(cat) return false;
    }
    if(item)
    {
        if(item->getType() == mType) return false;
        else return true;
    }
    return true;
}

bool LLIsOfAssetType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    if(mType == LLAssetType::AT_CATEGORY)
    {
        if(cat) return true;
    }
    if(item)
    {
        if(item->getActualType() == mType) return true;
    }
    return false;
}

bool LLAssetIDAndTypeMatches::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    if (!item) return false;
    return (item->getActualType() == mType && item->getAssetUUID() == mAssetID);
}

bool LLIsValidItemLink::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
    if (!vitem) return false;
    return (vitem->getActualType() == LLAssetType::AT_LINK  && !vitem->getIsBrokenLink());
}

bool LLIsTypeWithPermissions::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    if(mType == LLAssetType::AT_CATEGORY)
    {
        if(cat)
        {
            return true;
        }
    }
    if(item)
    {
        if(item->getType() == mType)
        {
            LLPermissions perm = item->getPermissions();
            if ((perm.getMaskBase() & mPerm) == mPerm)
            {
                return true;
            }
        }
    }
    return false;
}

bool LLBuddyCollector::operator()(LLInventoryCategory* cat,
                                  LLInventoryItem* item)
{
    if(item)
    {
        if((LLAssetType::AT_CALLINGCARD == item->getType())
           && (!item->getCreatorUUID().isNull())
           && (item->getCreatorUUID() != gAgent.getID()))
        {
            return true;
        }
    }
    return false;
}


bool LLUniqueBuddyCollector::operator()(LLInventoryCategory* cat,
                                        LLInventoryItem* item)
{
    if(item)
    {
        if((LLAssetType::AT_CALLINGCARD == item->getType())
           && (item->getCreatorUUID().notNull())
           && (item->getCreatorUUID() != gAgent.getID()))
        {
            mSeen.insert(item->getCreatorUUID());
            return true;
        }
    }
    return false;
}


bool LLParticularBuddyCollector::operator()(LLInventoryCategory* cat,
                                            LLInventoryItem* item)
{
    if(item)
    {
        if((LLAssetType::AT_CALLINGCARD == item->getType())
           && (item->getCreatorUUID() == mBuddyID))
        {
            return true;
        }
    }
    return false;
}


bool LLNameCategoryCollector::operator()(
    LLInventoryCategory* cat, LLInventoryItem* item)
{
    if(cat)
    {
        if (!LLStringUtil::compareInsensitive(mName, cat->getName()))
        {
            return true;
        }
    }
    return false;
}

bool LLNameItemCollector::operator()(
    LLInventoryCategory* cat, LLInventoryItem* item)
{
    if(item)
    {
        if (!LLStringUtil::compareInsensitive(mName, item->getName()))
        {
            return true;
        }
    }
    return false;
}

bool LLFindCOFValidItems::operator()(LLInventoryCategory* cat,
                                     LLInventoryItem* item)
{
    // Valid COF items are:
    // - links to wearables (body parts or clothing)
    // - links to attachments
    // - links to gestures
    // - links to ensemble folders
    LLViewerInventoryItem *linked_item = ((LLViewerInventoryItem*)item)->getLinkedItem();
    if (linked_item)
    {
        LLAssetType::EType type = linked_item->getType();
        return (type == LLAssetType::AT_CLOTHING ||
                type == LLAssetType::AT_BODYPART ||
                type == LLAssetType::AT_GESTURE ||
                type == LLAssetType::AT_OBJECT);
    }
    else
    {
        LLViewerInventoryCategory *linked_category = ((LLViewerInventoryItem*)item)->getLinkedCategory();
        // BAP remove AT_NONE support after ensembles are fully working?
        return (linked_category &&
                ((linked_category->getPreferredType() == LLFolderType::FT_NONE) ||
                 (LLFolderType::lookupIsEnsembleType(linked_category->getPreferredType()))));
    }
}

bool LLFindBrokenLinks::operator()(LLInventoryCategory* cat,
    LLInventoryItem* item)
{
    // only for broken links getType will be a link
    // otherwise it's supposed to have the type of an item
    // it is linked too
    if (item && LLAssetType::lookupIsLinkType(item->getType()))
    {
        return true;
    }
    return false;
}

bool LLFindWearables::operator()(LLInventoryCategory* cat,
                                 LLInventoryItem* item)
{
    if(item)
    {
        if((item->getType() == LLAssetType::AT_CLOTHING)
           || (item->getType() == LLAssetType::AT_BODYPART))
        {
            return true;
        }
    }
    return false;
}

LLFindWearablesEx::LLFindWearablesEx(bool is_worn, bool include_body_parts)
:   mIsWorn(is_worn)
,   mIncludeBodyParts(include_body_parts)
{}

bool LLFindWearablesEx::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
    if (!vitem) return false;

    // Skip non-wearables.
    if (!vitem->isWearableType() && vitem->getType() != LLAssetType::AT_OBJECT && vitem->getType() != LLAssetType::AT_GESTURE)
    {
        return false;
    }

    // Skip body parts if requested.
    if (!mIncludeBodyParts && vitem->getType() == LLAssetType::AT_BODYPART)
    {
        return false;
    }

    // Skip broken links.
    if (vitem->getIsBrokenLink())
    {
        return false;
    }

    return (bool) get_is_item_worn(item->getUUID()) == mIsWorn;
}

bool LLFindWearablesOfType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    if (!item) return false;
    if (item->getType() != LLAssetType::AT_CLOTHING &&
        item->getType() != LLAssetType::AT_BODYPART)
    {
        return false;
    }

    LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
    if (!vitem || vitem->getWearableType() != mWearableType) return false;

    return true;
}

void LLFindWearablesOfType::setType(LLWearableType::EType type)
{
    mWearableType = type;
}

bool LLIsTextureType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    return item && (item->getType() == LLAssetType::AT_TEXTURE);
}

bool LLFindNonRemovableObjects::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    if (item)
    {
        return !get_is_item_removable(&gInventory, item->getUUID(), true);
    }
    if (cat)
    {
        return !get_is_category_removable(&gInventory, cat->getUUID());
    }

    LL_WARNS() << "Not a category and not an item?" << LL_ENDL;
    return false;
}

///----------------------------------------------------------------------------
/// LLAssetIDMatches
///----------------------------------------------------------------------------
bool LLAssetIDMatches::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    return (item && item->getAssetUUID() == mAssetID);
}

///----------------------------------------------------------------------------
/// LLLinkedItemIDMatches
///----------------------------------------------------------------------------
bool LLLinkedItemIDMatches::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
    return (item &&
            (item->getIsLinkType()) &&
            (item->getLinkedUUID() == mBaseItemID)); // A linked item's assetID will be the compared-to item's itemID.
}

void LLSaveFolderState::setApply(bool apply)
{
    mApply = apply;
    // before generating new list of open folders, clear the old one
    if(!apply)
    {
        clearOpenFolders();
    }
}

void LLSaveFolderState::doFolder(LLFolderViewFolder* folder)
{
    LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getViewModelItem();
    if(!bridge) return;

    if(mApply)
    {
        // we're applying the open state
        LLUUID id(bridge->getUUID());
        if(mOpenFolders.find(id) != mOpenFolders.end())
        {
            if (!folder->isOpen())
            {
                folder->setOpen(true);
            }
        }
        else
        {
            // keep selected filter in its current state, this is less jarring to user
            if (!folder->isSelected() && folder->isOpen())
            {
                folder->setOpen(false);
            }
        }
    }
    else
    {
        // we're recording state at this point
        if(folder->isOpen())
        {
            mOpenFolders.insert(bridge->getUUID());
        }
    }
}

void LLOpenFilteredFolders::doItem(LLFolderViewItem *item)
{
    if (item->passedFilter())
    {
        item->getParentFolder()->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_UP);
    }
}

void LLOpenFilteredFolders::doFolder(LLFolderViewFolder* folder)
{
    if (folder->LLFolderViewItem::passedFilter() && folder->getParentFolder())
    {
        folder->getParentFolder()->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_UP);
    }
    // if this folder didn't pass the filter, and none of its descendants did
    else if (!folder->getViewModelItem()->passedFilter() && !folder->getViewModelItem()->descendantsPassedFilter())
    {
        folder->setOpenArrangeRecursively(false, LLFolderViewFolder::RECURSE_NO);
    }
}

void LLSelectFirstFilteredItem::doItem(LLFolderViewItem *item)
{
    if (item->passedFilter() && !mItemSelected)
    {
        item->getRoot()->setSelection(item, false, false);
        if (item->getParentFolder())
        {
            item->getParentFolder()->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_UP);
        }
        mItemSelected = true;
    }
}

void LLSelectFirstFilteredItem::doFolder(LLFolderViewFolder* folder)
{
    // Skip if folder or item already found, if not filtered or if no parent (root folder is not selectable)
    if (!mFolderSelected && !mItemSelected && folder->LLFolderViewItem::passedFilter() && folder->getParentFolder())
    {
        folder->getRoot()->setSelection(folder, false, false);
        folder->getParentFolder()->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_UP);
        mFolderSelected = true;
    }
}

void LLOpenFoldersWithSelection::doItem(LLFolderViewItem *item)
{
    if (item->getParentFolder() && item->isSelected())
    {
        item->getParentFolder()->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_UP);
    }
}

void LLOpenFoldersWithSelection::doFolder(LLFolderViewFolder* folder)
{
    if (folder->getParentFolder() && folder->isSelected())
    {
        folder->getParentFolder()->setOpenArrangeRecursively(true, LLFolderViewFolder::RECURSE_UP);
    }
}

// Callback for doToSelected if DAMA required...
void LLInventoryAction::callback_doToSelected(const LLSD& notification, const LLSD& response, class LLInventoryModel* model, class LLFolderView* root, const std::string& action)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
        doToSelected(model, root, action, false);
    }
}

void LLInventoryAction::callback_copySelected(const LLSD& notification, const LLSD& response, class LLInventoryModel* model, class LLFolderView* root, const std::string& action)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES, Move no copy item(s)
    {
        doToSelected(model, root, "copy_or_move_to_marketplace_listings", false);
    }
    else if (option == 1) // NO, Don't move no copy item(s) (leave them behind)
    {
        doToSelected(model, root, "copy_to_marketplace_listings", false);
    }
}

// Succeeds iff all selected items are bridges to objects, in which
// case returns their corresponding uuids.
bool get_selection_object_uuids(LLFolderView *root, uuid_vec_t& ids)
{
    uuid_vec_t results;
    S32 non_object = 0;
    LLFolderView::selected_items_t selectedItems = root->getSelectedItems();
    for(LLFolderView::selected_items_t::iterator it = selectedItems.begin(); it != selectedItems.end(); ++it)
    {
        LLObjectBridge *view_model = dynamic_cast<LLObjectBridge *>((*it)->getViewModelItem());

        if(view_model && view_model->getUUID().notNull())
        {
            results.push_back(view_model->getUUID());
        }
        else
        {
            non_object++;
        }
    }
    if (non_object == 0)
    {
        ids = results;
        return true;
    }
    return false;
}


void LLInventoryAction::doToSelected(LLInventoryModel* model, LLFolderView* root, const std::string& action, bool user_confirm)
{
    std::set<LLFolderViewItem*> selected_items = root->getSelectionList();
    if (selected_items.empty()
        && action != "wear"
        && action != "wear_add"
        && !isRemoveAction(action))
    {
        // Was item removed while user was checking menu?
        // "wear" and removal exlusions are due to use of
        // getInventorySelectedUUIDs() below
        LL_WARNS("Inventory") << "Menu tried to operate on empty selection" << LL_ENDL;

        if (("copy" == action) || ("cut" == action))
        {
            LLClipboard::instance().reset();
        }

        return;
    }

    // Prompt the user and check for authorization for some marketplace active listing edits
    if (user_confirm && (("delete" == action) || ("cut" == action) || ("rename" == action) || ("properties" == action) || ("task_properties" == action) || ("open" == action)))
    {
        std::set<LLFolderViewItem*>::iterator set_iter = selected_items.begin();
        LLFolderViewModelItemInventory * viewModel = NULL;
        for (; set_iter != selected_items.end(); ++set_iter)
        {
            viewModel = dynamic_cast<LLFolderViewModelItemInventory *>((*set_iter)->getViewModelItem());
            if (viewModel && (depth_nesting_in_marketplace(viewModel->getUUID()) >= 0))
            {
                break;
            }
        }
        if (set_iter != selected_items.end())
        {
            if ("open" == action)
            {
                if (get_can_item_be_worn(viewModel->getUUID()))
                {
                    // Wearing an object from any listing, active or not, is verbotten
                    LLNotificationsUtil::add("AlertMerchantListingCannotWear");
                    return;
                }
                // Note: we do not prompt for change when opening items (e.g. textures or note cards) on the marketplace...
            }
            else if (LLMarketplaceData::instance().isInActiveFolder(viewModel->getUUID()) ||
                     LLMarketplaceData::instance().isListedAndActive(viewModel->getUUID()))
            {
                // If item is in active listing, further confirmation is required
                if ((("cut" == action) || ("delete" == action)) && (LLMarketplaceData::instance().isListed(viewModel->getUUID()) || LLMarketplaceData::instance().isVersionFolder(viewModel->getUUID())))
                {
                    // Cut or delete of the active version folder or listing folder itself will unlist the listing so ask that question specifically
                    LLNotificationsUtil::add("ConfirmMerchantUnlist", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, model, root, action));
                    return;
                }
                // Any other case will simply modify but not unlist a listing
                LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, model, root, action));
                return;
            }
            // Cutting or deleting a whole listing needs confirmation as SLM will be archived and inaccessible to the user
            else if (LLMarketplaceData::instance().isListed(viewModel->getUUID()) && (("cut" == action) || ("delete" == action)))
            {
                LLNotificationsUtil::add("ConfirmListingCutOrDelete", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, model, root, action));
                return;
            }
        }
    }
    // Copying to the marketplace needs confirmation if nocopy items are involved
    if (user_confirm && ("copy_to_marketplace_listings" == action))
    {
        std::set<LLFolderViewItem*>::iterator set_iter = selected_items.begin();
        LLFolderViewModelItemInventory * viewModel = dynamic_cast<LLFolderViewModelItemInventory *>((*set_iter)->getViewModelItem());
        if (contains_nocopy_items(viewModel->getUUID()))
        {
            LLNotificationsUtil::add("ConfirmCopyToMarketplace", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_copySelected, _1, _2, model, root, action));
            return;
        }
    }

    // Keep track of the marketplace folders that will need update of their status/name after the operation is performed
    buildMarketplaceFolders(root);

    if ("rename" == action)
    {
        root->startRenamingSelectedItem();
        // Update the marketplace listings that have been affected by the operation
        updateMarketplaceFolders();
        return;
    }

    if ("delete" == action)
    {
        const LLUUID &marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
        bool marketplacelistings_item = false;
        bool has_worn = false;
        bool needs_replacement = false;
        LLAllDescendentsPassedFilter f;
        for (std::set<LLFolderViewItem*>::iterator it = selected_items.begin(); (it != selected_items.end()) && (f.allDescendentsPassedFilter()); ++it)
        {
            if (LLFolderViewFolder* folder = dynamic_cast<LLFolderViewFolder*>(*it))
            {
                folder->applyFunctorRecursively(f);
            }
            LLFolderViewModelItemInventory * viewModel = dynamic_cast<LLFolderViewModelItemInventory *>((*it)->getViewModelItem());
            LLUUID obj_id = viewModel->getUUID();
            if (viewModel && gInventory.isObjectDescendentOf(obj_id, marketplacelistings_id))
            {
                marketplacelistings_item = true;
                break;
            }

            LLViewerInventoryCategory* cat = gInventory.getCategory(obj_id);
            if (cat)
            {
                LLInventoryModel::cat_array_t categories;
                LLInventoryModel::item_array_t items;

                gInventory.collectDescendents(obj_id, categories, items, false);

                for (LLInventoryModel::item_array_t::value_type& item : items)
                {
                    if (get_is_item_worn(item))
                    {
                        has_worn = true;
                        LLWearableType::EType type = item->getWearableType();
                        if (type == LLWearableType::WT_SHAPE
                            || type == LLWearableType::WT_SKIN
                            || type == LLWearableType::WT_HAIR
                            || type == LLWearableType::WT_EYES)
                        {
                            needs_replacement = true;
                            break;
                        }
                    }
                }
                if (needs_replacement)
                {
                    break;
                }
            }
            LLViewerInventoryItem* item = gInventory.getItem(obj_id);
            if (item && get_is_item_worn(item))
            {
                has_worn = true;
                LLWearableType::EType type = item->getWearableType();
                if (type == LLWearableType::WT_SHAPE
                    || type == LLWearableType::WT_SKIN
                    || type == LLWearableType::WT_HAIR
                    || type == LLWearableType::WT_EYES)
                {
                    needs_replacement = true;
                    break;
                }
            }
        }
        // Fall through to the generic confirmation if the user choose to ignore the specialized one
        if (needs_replacement)
        {
            LLNotificationsUtil::add("CantDeleteRequiredClothing");
        }
        else if (has_worn)
        {
            LLSD payload;
            payload["has_worn"] = true;
            LLNotificationsUtil::add("DeleteWornItems", LLSD(), payload, boost::bind(&LLInventoryAction::onItemsRemovalConfirmation, _1, _2, root->getHandle()));
        }
        else if ( (!f.allDescendentsPassedFilter()) && !marketplacelistings_item && (!LLNotifications::instance().getIgnored("DeleteFilteredItems")) )
        {
            LLNotificationsUtil::add("DeleteFilteredItems", LLSD(), LLSD(), boost::bind(&LLInventoryAction::onItemsRemovalConfirmation, _1, _2, root->getHandle()));
        }
        else
        {
            if (!sDeleteConfirmationDisplayed) // ask for the confirmation at least once per session
            {
                LLNotifications::instance().setIgnored("DeleteItems", false);
                sDeleteConfirmationDisplayed = true;
            }

            LLSD args;
            args["QUESTION"] = LLTrans::getString(root->getSelectedCount() > 1 ? "DeleteItems" :  "DeleteItem");
            LLNotificationsUtil::add("DeleteItems", args, LLSD(), boost::bind(&LLInventoryAction::onItemsRemovalConfirmation, _1, _2, root->getHandle()));
        }
        // Note: marketplace listings will be updated in the callback if delete confirmed
        return;
    }
    if (("copy" == action) || ("cut" == action))
    {
        // Clear the clipboard before we start adding things on it
        LLClipboard::instance().reset();
    }
    if ("replace_links" == action)
    {
        LLSD params;
        if (root->getSelectedCount() == 1)
        {
            LLFolderViewItem* folder_item = root->getSelectedItems().front();
            LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getViewModelItem();

            if (bridge)
            {
                LLInventoryObject* obj = bridge->getInventoryObject();
                if (obj && obj->getType() != LLAssetType::AT_CATEGORY && obj->getActualType() != LLAssetType::AT_LINK_FOLDER)
                {
                    params = LLSD(obj->getUUID());
                }
            }
        }
        LLFloaterReg::showInstance("linkreplace", params);
        return;
    }

    static const std::string change_folder_string = "change_folder_type_";
    if (action.length() > change_folder_string.length() &&
        (action.compare(0,change_folder_string.length(),"change_folder_type_") == 0))
    {
        LLFolderType::EType new_folder_type = LLViewerFolderType::lookupTypeFromXUIName(action.substr(change_folder_string.length()));
        LLFolderViewModelItemInventory* inventory_item = static_cast<LLFolderViewModelItemInventory*>(root->getViewModelItem());
        LLViewerInventoryCategory *cat = model->getCategory(inventory_item->getUUID());
        if (!cat) return;
        cat->changeType(new_folder_type);
        // Update the marketplace listings that have been affected by the operation
        updateMarketplaceFolders();
        return;
    }


    LLMultiPreview* multi_previewp = NULL;
    LLMultiItemProperties* multi_itempropertiesp = nullptr;

    if (("task_open" == action  || "open" == action) && selected_items.size() > 1)
    {
        bool open_multi_preview = true;

        if ("open" == action)
        {
            for (std::set<LLFolderViewItem*>::iterator set_iter = selected_items.begin(); set_iter != selected_items.end(); ++set_iter)
            {
                LLFolderViewItem* folder_item = *set_iter;
                if (folder_item)
                {
                    LLInvFVBridge* bridge = dynamic_cast<LLInvFVBridge*>(folder_item->getViewModelItem());
                    if (!bridge || !bridge->isMultiPreviewAllowed())
                    {
                        open_multi_preview = false;
                        break;
                    }
                }
            }
        }

        if (open_multi_preview)
        {
            multi_previewp = new LLMultiPreview();
            gFloaterView->addChild(multi_previewp);

            LLFloater::setFloaterHost(multi_previewp);
        }

    }
    else if (("task_properties" == action || "properties" == action) && selected_items.size() > 1)
    {
        multi_itempropertiesp = new LLMultiItemProperties("item_properties");
        gFloaterView->addChild(multi_itempropertiesp);
        LLFloater::setFloaterHost(multi_itempropertiesp);
    }

    std::set<LLUUID> selected_uuid_set = LLAvatarActions::getInventorySelectedUUIDs();

    // copy list of applicable items into a vector for bulk handling
    uuid_vec_t ids;
    if (action == "wear" || action == "wear_add")
    {
        const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
        const LLUUID mp_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
        std::copy_if(selected_uuid_set.begin(),
            selected_uuid_set.end(),
            std::back_inserter(ids),
            [trash_id, mp_id](LLUUID id)
        {
            if (get_is_item_worn(id)
                || LLAppearanceMgr::instance().getIsInCOF(id)
                || gInventory.isObjectDescendentOf(id, trash_id))
            {
                return false;
            }
            if (mp_id.notNull() && gInventory.isObjectDescendentOf(id, mp_id))
            {
                return false;
            }
            LLInventoryObject* obj = (LLInventoryObject*)gInventory.getObject(id);
            if (!obj)
            {
                return false;
            }
            if (obj->getIsLinkType() && gInventory.isObjectDescendentOf(obj->getLinkedUUID(), trash_id))
            {
                return false;
            }
            if (obj->getIsLinkType() && LLAssetType::lookupIsLinkType(obj->getType()))
            {
                // missing
                return false;
            }
            return true;
        }
        );
    }
    else if (isRemoveAction(action))
    {
        std::copy_if(selected_uuid_set.begin(),
            selected_uuid_set.end(),
            std::back_inserter(ids),
            [](LLUUID id)
        {
            return get_is_item_worn(id);
        }
        );
    }
    else
    {
        for (std::set<LLFolderViewItem*>::iterator it = selected_items.begin(), end_it = selected_items.end();
            it != end_it;
            ++it)
        {
            ids.push_back(static_cast<LLFolderViewModelItemInventory*>((*it)->getViewModelItem())->getUUID());
        }
    }

    // Check for actions that get handled in bulk
    if (action == "wear")
    {
        wear_multiple(ids, true);
    }
    else if (action == "wear_add")
    {
        wear_multiple(ids, false);
    }
    else if (isRemoveAction(action))
    {
        LLAppearanceMgr::instance().removeItemsFromAvatar(ids);
    }
    else if ("save_selected_as" == action)
    {
        (new LLDirPickerThread(boost::bind(&LLInventoryAction::saveMultipleTextures, _1, selected_items, model), std::string()))->getFile();
    }
    else if ("new_folder_from_selected" == action)
    {

        LLInventoryObject* first_item = gInventory.getObject(*ids.begin());
        if (!first_item)
        {
            return;
        }
        const LLUUID& parent_uuid = first_item->getParentUUID();
        for (uuid_vec_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
        {
            LLInventoryObject *item = gInventory.getObject(*it);
            if (!item || item->getParentUUID() != parent_uuid)
            {
                LLNotificationsUtil::add("SameFolderRequired");
                return;
            }
        }

        LLSD args;
        args["DESC"] = LLTrans::getString("New Folder");

        LLNotificationsUtil::add("CreateSubfolder", args, LLSD(),
            [ids](const LLSD& notification, const LLSD& response)
        {
            S32 opt = LLNotificationsUtil::getSelectedOption(notification, response);
            if (opt == 0)
            {
                std::string settings_name = response["message"].asString();

                LLInventoryObject::correctInventoryName(settings_name);
                if (settings_name.empty())
                {
                    settings_name = LLTrans::getString("New Folder");
                }
                move_items_to_new_subfolder(ids, settings_name);
            }
        });
    }
    else if ("ungroup_folder_items" == action)
    {
        if (ids.size() == 1)
        {
            ungroup_folder_items(*ids.begin());
        }
    }
    else if ("thumbnail" == action)
    {
        if (selected_items.size() > 0)
        {
            LLSD data;
            std::set<LLFolderViewItem*>::iterator set_iter;
            for (set_iter = selected_items.begin(); set_iter != selected_items.end(); ++set_iter)
            {
                LLFolderViewItem* folder_item = *set_iter;
                if (!folder_item) continue;
                LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getViewModelItem();
                if (!bridge) continue;
                data.append(bridge->getUUID());
            }
            LLFloaterReg::showInstance("change_item_thumbnail", data);
        }
    }
    else
    {
        std::set<LLFolderViewItem*>::iterator set_iter;
        for (set_iter = selected_items.begin(); set_iter != selected_items.end(); ++set_iter)
        {
            LLFolderViewItem* folder_item = *set_iter;
            if(!folder_item) continue;
            LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getViewModelItem();
            if(!bridge) continue;
            bridge->performAction(model, action);
        }
        if(root->isSingleFolderMode() && selected_items.empty())
        {
            LLInvFVBridge* bridge = (LLInvFVBridge*)root->getViewModelItem();
            if(bridge)
            {
                bridge->performAction(model, action);
            }
        }
    }

    // Update the marketplace listings that have been affected by the operation
    updateMarketplaceFolders();

    LLFloater::setFloaterHost(NULL);
    if (multi_previewp)
    {
        multi_previewp->openFloater(LLSD());
    }
    else if (multi_itempropertiesp)
    {
        multi_itempropertiesp->openFloater(LLSD());
    }
}

void LLInventoryAction::saveMultipleTextures(const std::vector<std::string>& filenames, std::set<LLFolderViewItem*> selected_items, LLInventoryModel* model)
{
    gSavedSettings.setString("TextureSaveLocation", filenames[0]);

    LLMultiPreview* multi_previewp = new LLMultiPreview();
    gFloaterView->addChild(multi_previewp);

    LLFloater::setFloaterHost(multi_previewp);

    std::map<std::string, S32> tex_names_map;
    std::set<LLFolderViewItem*>::iterator set_iter;

    for (set_iter = selected_items.begin(); set_iter != selected_items.end(); ++set_iter)
    {
        LLFolderViewItem* folder_item = *set_iter;
        if(!folder_item) continue;
        LLTextureBridge* bridge = (LLTextureBridge*)folder_item->getViewModelItem();
        if(!bridge) continue;

        std::string tex_name = bridge->getName();
        if(!tex_names_map.insert(std::pair<std::string, S32>(tex_name, 0)).second)
        {
            tex_names_map[tex_name]++;
            bridge->setFileName(tex_name + llformat("_%.3d", tex_names_map[tex_name]));
        }
        bridge->performAction(model, "save_selected_as");
    }

    LLFloater::setFloaterHost(NULL);
    if (multi_previewp)
    {
        multi_previewp->openFloater(LLSD());
    }
}

void LLInventoryAction::removeItemFromDND(LLFolderView* root)
{
    if(gAgent.isDoNotDisturb())
    {
        //Get selected items
        LLFolderView::selected_items_t selectedItems = root->getSelectedItems();
        LLFolderViewModelItemInventory * viewModel = NULL;

        //If user is in DND and deletes item, make sure the notification is not displayed by removing the notification
        //from DND history and .xml file. Once this is done, upon exit of DND mode the item deleted will not show a notification.
        for(LLFolderView::selected_items_t::iterator it = selectedItems.begin(); it != selectedItems.end(); ++it)
        {
            viewModel = dynamic_cast<LLFolderViewModelItemInventory *>((*it)->getViewModelItem());

            if(viewModel && viewModel->getUUID().notNull())
            {
                //Will remove the item offer notification
                LLDoNotDisturbNotificationStorage::instance().removeNotification(LLDoNotDisturbNotificationStorage::offerName, viewModel->getUUID());
            }
        }
    }
}

void LLInventoryAction::onItemsRemovalConfirmation(const LLSD& notification, const LLSD& response, LLHandle<LLFolderView> root)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0 && !root.isDead() && !root.get()->isDead())
    {
        bool has_worn = notification["payload"]["has_worn"].asBoolean();
        LLFolderView* folder_root = root.get();
        //Need to remove item from DND before item is removed from root folder view
        //because once removed from root folder view the item is no longer a selected item
        removeItemFromDND(folder_root);

        // removeSelectedItems will change selection, collect worn items beforehand
        uuid_vec_t worn;
        uuid_vec_t item_deletion_list;
        uuid_vec_t cat_deletion_list;
        if (has_worn)
        {
            //Get selected items
            LLFolderView::selected_items_t selectedItems = folder_root->getSelectedItems();

            //If user is in DND and deletes item, make sure the notification is not displayed by removing the notification
            //from DND history and .xml file. Once this is done, upon exit of DND mode the item deleted will not show a notification.
            for (LLFolderView::selected_items_t::iterator it = selectedItems.begin(); it != selectedItems.end(); ++it)
            {
                LLFolderViewModelItemInventory* viewModel = dynamic_cast<LLFolderViewModelItemInventory*>((*it)->getViewModelItem());

                LLUUID obj_id = viewModel->getUUID();
                LLViewerInventoryCategory* cat = gInventory.getCategory(obj_id);
                bool cat_has_worn = false;
                if (cat)
                {
                    LLInventoryModel::cat_array_t categories;
                    LLInventoryModel::item_array_t items;

                    gInventory.collectDescendents(obj_id, categories, items, false);

                    for (LLInventoryModel::item_array_t::value_type& item : items)
                    {
                        if (get_is_item_worn(item))
                        {
                            worn.push_back(item->getUUID());
                            cat_has_worn = true;
                        }
                    }
                    if (cat_has_worn)
                    {
                        cat_deletion_list.push_back(obj_id);
                    }
                }
                LLViewerInventoryItem* item = gInventory.getItem(obj_id);
                if (item && get_is_item_worn(item))
                {
                    worn.push_back(obj_id);
                    item_deletion_list.push_back(obj_id);
                }
            }
        }

        // removeSelectedItems will check if items are worn before deletion,
        // don't 'unwear' yet to prevent race conditions from unwearing
        // and removing simultaneously
        folder_root->removeSelectedItems();

        // unwear then delete the rest
        if (!worn.empty())
        {
            // should fire once after every item gets detached
            LLAppearanceMgr::instance().removeItemsFromAvatar(worn,
                                                              [item_deletion_list, cat_deletion_list]()
                                                              {
                                                                  for (const LLUUID& id : item_deletion_list)
                                                                  {
                                                                      gInventory.removeItem(id);
                                                                  }
                                                                  for (const LLUUID& id : cat_deletion_list)
                                                                  {
                                                                      gInventory.removeCategory(id);
                                                                  }
                                                              });
        }

        // Update the marketplace listings that have been affected by the operation
        updateMarketplaceFolders();
    }
}

void LLInventoryAction::buildMarketplaceFolders(LLFolderView* root)
{
    // Make a list of all marketplace folders containing the elements in the selected list
    // as well as the elements themselves.
    // Once those elements are updated (cut, delete in particular but potentially any action), their
    // containing folder will need to be updated as well as their initially containing folder. For
    // instance, moving a stock folder from a listed folder to another will require an update of the
    // target listing *and* the original listing. So we need to keep track of both.
    // Note: do not however put the marketplace listings root itself in this list or the whole marketplace data will be rebuilt.
    sMarketplaceFolders.clear();
    const LLUUID &marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
    if (marketplacelistings_id.isNull())
    {
        return;
    }

    std::set<LLFolderViewItem*> selected_items = root->getSelectionList();
    std::set<LLFolderViewItem*>::iterator set_iter = selected_items.begin();
    LLFolderViewModelItemInventory * viewModel = NULL;
    for (; set_iter != selected_items.end(); ++set_iter)
    {
        viewModel = dynamic_cast<LLFolderViewModelItemInventory *>((*set_iter)->getViewModelItem());
        if (!viewModel || !viewModel->getInventoryObject()) continue;
        if (gInventory.isObjectDescendentOf(viewModel->getInventoryObject()->getParentUUID(), marketplacelistings_id))
        {
            const LLUUID &parent_id = viewModel->getInventoryObject()->getParentUUID();
            if (parent_id != marketplacelistings_id)
            {
                sMarketplaceFolders.push_back(parent_id);
            }
            const LLUUID &curr_id = viewModel->getInventoryObject()->getUUID();
            if (curr_id != marketplacelistings_id)
            {
                sMarketplaceFolders.push_back(curr_id);
            }
        }
    }
    // Suppress dupes in the list so we won't update listings twice
    sMarketplaceFolders.sort();
    sMarketplaceFolders.unique();
}

void LLInventoryAction::updateMarketplaceFolders()
{
    while (!sMarketplaceFolders.empty())
    {
        update_marketplace_category(sMarketplaceFolders.back());
        sMarketplaceFolders.pop_back();
    }
}


