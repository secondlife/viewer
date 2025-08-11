/**
 * @file llinventoryobserver.cpp
 * @brief Implementation of the inventory observers used to track agent inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llinventoryobserver.h"

#include "llassetstorage.h"
#include "llcrc.h"
#include "lldir.h"
#include "llsys.h"
#include "llxfermanager.h"
#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llaisapi.h"
#include "llfloater.h"
#include "llfocusmgr.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llviewerregion.h"
#include "llappviewer.h"
#include "lldbstrings.h"
#include "llviewerstats.h"
#include "llnotificationsutil.h"
#include "llcallbacklist.h"
#include "llpreview.h"
#include "llviewercontrol.h"
#include "llvoavatarself.h"
#include "llsdutil.h"
#include <deque>

const S32 LLInventoryFetchItemsObserver::MAX_INDIVIDUAL_ITEM_REQUESTS = 7;
const F32 LLInventoryFetchItemsObserver::FETCH_TIMER_EXPIRY = 60.0f;


LLInventoryObserver::LLInventoryObserver()
{
}

// virtual
LLInventoryObserver::~LLInventoryObserver()
{
}

LLInventoryFetchObserver::LLInventoryFetchObserver(const LLUUID& id)
{
    mIDs.clear();
    if (id != LLUUID::null)
    {
        setFetchID(id);
    }
}

LLInventoryFetchObserver::LLInventoryFetchObserver(const uuid_vec_t& ids)
{
    setFetchIDs(ids);
}

bool LLInventoryFetchObserver::isFinished() const
{
    return mIncomplete.empty();
}

void LLInventoryFetchObserver::setFetchIDs(const uuid_vec_t& ids)
{
    mIDs = ids;
}
void LLInventoryFetchObserver::setFetchID(const LLUUID& id)
{
    mIDs.clear();
    mIDs.push_back(id);
}


void LLInventoryCompletionObserver::changed(U32 mask)
{
    // scan through the incomplete items and move or erase them as
    // appropriate.
    if (!mIncomplete.empty())
    {
        for (uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
        {
            const LLViewerInventoryItem* item = gInventory.getItem(*it);
            if (!item)
            {
                it = mIncomplete.erase(it);
                continue;
            }
            if (item->isFinished())
            {
                mComplete.push_back(*it);
                it = mIncomplete.erase(it);
                continue;
            }
            ++it;
        }
        if (mIncomplete.empty())
        {
            done();
        }
    }
}

void LLInventoryCompletionObserver::watchItem(const LLUUID& id)
{
    if (id.notNull())
    {
        mIncomplete.push_back(id);
    }
}

LLInventoryFetchItemsObserver::LLInventoryFetchItemsObserver(const LLUUID& item_id) :
    LLInventoryFetchObserver(item_id)
{
    mIDs.clear();
    mIDs.push_back(item_id);
}

LLInventoryFetchItemsObserver::LLInventoryFetchItemsObserver(const uuid_vec_t& item_ids) :
    LLInventoryFetchObserver(item_ids)
{
}

void LLInventoryFetchItemsObserver::changed(U32 mask)
{
    LL_DEBUGS("InventoryFetch") << this << " remaining incomplete " << mIncomplete.size()
             << " complete " << mComplete.size()
             << " wait period " << mFetchingPeriod.getRemainingTimeF32()
             << LL_ENDL;

    // scan through the incomplete items and move or erase them as
    // appropriate.
    if (!mIncomplete.empty())
    {
        if (!LLInventoryModelBackgroundFetch::getInstance()->isEverythingFetched())
        {
            // Folders have a priority over items and they download items as well
            // Wait untill initial folder fetch is done
            LL_DEBUGS("InventoryFetch") << "Folder fetch in progress, resetting fetch timer" << LL_ENDL;

            mFetchingPeriod.reset();
            mFetchingPeriod.setTimerExpirySec(FETCH_TIMER_EXPIRY);
        }

        // Have we exceeded max wait time?
        bool timeout_expired = mFetchingPeriod.hasExpired();

        for (uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
        {
            const LLUUID& item_id = (*it);
            LLViewerInventoryItem* item = gInventory.getItem(item_id);
            if (item && item->isFinished())
            {
                mComplete.push_back(item_id);
                it = mIncomplete.erase(it);
            }
            else
            {
                if (timeout_expired)
                {
                    // Just concede that this item hasn't arrived in reasonable time and continue on.
                    LL_WARNS("InventoryFetch") << "Fetcher timed out when fetching inventory item UUID: " << item_id << LL_ENDL;
                    it = mIncomplete.erase(it);
                }
                else
                {
                    // Keep trying.
                    ++it;
                }
            }
        }

    }

    if (mIncomplete.empty())
    {
        LL_DEBUGS("InventoryFetch") << this << " done at remaining incomplete "
                 << mIncomplete.size() << " complete " << mComplete.size() << LL_ENDL;
        done();
    }
    //LL_INFOS() << "LLInventoryFetchItemsObserver::changed() mComplete size " << mComplete.size() << LL_ENDL;
    //LL_INFOS() << "LLInventoryFetchItemsObserver::changed() mIncomplete size " << mIncomplete.size() << LL_ENDL;
}

void fetch_items_from_llsd(const LLSD& items_llsd)
{
    if (!items_llsd.size() || gDisconnected) return;

    LLSD body;
    body[0]["cap_name"] = "FetchInventory2";
    body[1]["cap_name"] = "FetchLib2";
    for (S32 i=0; i<items_llsd.size();i++)
    {
        if (items_llsd[i]["owner_id"].asString() == gAgent.getID().asString())
        {
            body[0]["items"].append(items_llsd[i]);
            continue;
        }
        else if (items_llsd[i]["owner_id"].asString() == ALEXANDRIA_LINDEN_ID.asString())
        {
            body[1]["items"].append(items_llsd[i]);
            continue;
        }
    }

    for (S32 i=0; i<body.size(); i++)
    {
        if (!gAgent.getRegion())
        {
            LL_WARNS() << "Agent's region is null" << LL_ENDL;
            break;
        }

        if (0 == body[i]["items"].size()) {
            LL_DEBUGS() << "Skipping body with no items to fetch" << LL_ENDL;
            continue;
        }

        std::string url = gAgent.getRegion()->getCapability(body[i]["cap_name"].asString());
        if (!url.empty())
        {
            body[i]["agent_id"] = gAgent.getID();
            LLCore::HttpHandler::ptr_t handler(new LLInventoryModel::FetchItemHttpHandler(body[i]));
            gInventory.requestPost(true, url, body[i], handler, (i ? "Library Item" : "Inventory Item"));
            continue;
        }
        else
        {
            LL_WARNS("INVENTORY") << "Failed to get capability." << LL_ENDL;
        }

    }
}

void LLInventoryFetchItemsObserver::startFetch()
{
    bool aisv3 = AISAPI::isAvailable();

    LLSD items_llsd;

    typedef std::map<LLUUID, uuid_vec_t> requests_by_folders_t;
    requests_by_folders_t requests;
    for (uuid_vec_t::const_iterator it = mIDs.begin(); it < mIDs.end(); ++it)
    {
        LLViewerInventoryItem* item = gInventory.getItem(*it);
        if (item && item->isFinished())
        {
            // It's complete, so put it on the complete container.
            mComplete.push_back(*it);
            continue;
        }

        // Ignore categories since they're not items.  We
        // could also just add this to mComplete but not sure what the
        // side-effects would be, so ignoring to be safe.
        LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
        if (cat)
        {
            continue;
        }

        if ((*it).isNull())
        {
            LL_WARNS("Inventory") << "Skip fetching for a NULL uuid" << LL_ENDL;
            continue;
        }

        // It's incomplete, so put it on the incomplete container, and
        // pack this on the message.
        mIncomplete.push_back(*it);

        if (aisv3)
        {
            if (item)
            {
                LLUUID parent_id = item->getParentUUID();
                requests[parent_id].push_back(*it);
            }
            else
            {
                // Can happen for gestures and calling cards if server notified us before they fetched
                // Request by id without checking for an item.
                LLInventoryModelBackgroundFetch::getInstance()->scheduleItemFetch(*it);
            }
        }
        else
        {
            // Prepare the data to fetch
            LLSD item_entry;
            if (item)
            {
                item_entry["owner_id"] = item->getPermissions().getOwner();
            }
            else
            {
                // assume it's agent inventory.
                item_entry["owner_id"] = gAgent.getID();
            }
            item_entry["item_id"] = (*it);
            items_llsd.append(item_entry);
        }
    }

    mFetchingPeriod.reset();
    mFetchingPeriod.setTimerExpirySec(FETCH_TIMER_EXPIRY);

    if (aisv3)
    {
        for (requests_by_folders_t::value_type &folder : requests)
        {
            LLViewerInventoryCategory* cat = gInventory.getCategory(folder.first);
            if (cat)
            {
                if (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
                {
                    // start fetching whole folder since it's not ready either way
                    cat->fetch();
                }
                else if (folder.second.size() > MAX_INDIVIDUAL_ITEM_REQUESTS)
                {
                    // requesting one by one will take a while
                    // do whole folder
                    LLInventoryModelBackgroundFetch::getInstance()->scheduleFolderFetch(folder.first, true);
                }
                else if (cat->getViewerDescendentCount() <= folder.second.size()
                         || cat->getDescendentCount() <= folder.second.size())
                {
                    // Start fetching whole folder since we need all items
                    LLInventoryModelBackgroundFetch::getInstance()->scheduleFolderFetch(folder.first, true);
                }
                else
                {
                    // get items one by one
                    for (LLUUID& item_id : folder.second)
                    {
                        LLInventoryModelBackgroundFetch::getInstance()->scheduleItemFetch(item_id);
                    }
                }
            }
            else
            {
                // Isn't supposed to happen? We should have all folders
                // and if item exists, folder is supposed to exist as well.
                llassert(false);
                LL_WARNS("Inventory") << "Missing folder: " << folder.first << " fetching items individually" << LL_ENDL;

                // get items one by one
                for (LLUUID& item_id : folder.second)
                {
                    LLInventoryModelBackgroundFetch::getInstance()->scheduleItemFetch(item_id);
                }
            }
        }
    }
    else
    {
        fetch_items_from_llsd(items_llsd);
    }

}

LLInventoryFetchDescendentsObserver::LLInventoryFetchDescendentsObserver(const LLUUID& cat_id) :
    LLInventoryFetchObserver(cat_id)
{
}

LLInventoryFetchDescendentsObserver::LLInventoryFetchDescendentsObserver(const uuid_vec_t& cat_ids) :
    LLInventoryFetchObserver(cat_ids)
{
}

// virtual
void LLInventoryFetchDescendentsObserver::changed(U32 mask)
{
    for (uuid_vec_t::iterator it = mIncomplete.begin(); it < mIncomplete.end();)
    {
        const LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
        if (!cat)
        {
            it = mIncomplete.erase(it);
            continue;
        }
        if (isCategoryComplete(cat))
        {
            mComplete.push_back(*it);
            it = mIncomplete.erase(it);
            continue;
        }
        ++it;
    }

    if (mIncomplete.empty())
    {
        done();
    }
    else
    {
        LLInventoryModelBackgroundFetch* fetcher = LLInventoryModelBackgroundFetch::getInstance();
        if (fetcher->isEverythingFetched()
            && !fetcher->folderFetchActive())
        {
            // If fetcher is done with folders yet we are waiting, fetch either
            // failed or version is somehow stuck at -1
            done();
        }
    }
}

void LLInventoryFetchDescendentsObserver::startFetch()
{
    for (uuid_vec_t::const_iterator it = mIDs.begin(); it != mIDs.end(); ++it)
    {
        LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
        if (!cat) continue;
        if (!isCategoryComplete(cat))
        {
            //blindly fetch it without seeing if anything else is fetching it.
            LLInventoryModelBackgroundFetch::getInstance()->scheduleFolderFetch(*it, true);
            mIncomplete.push_back(*it); //Add to list of things being downloaded for this observer.
        }
        else
        {
            mComplete.push_back(*it);
        }
    }
}

bool LLInventoryFetchDescendentsObserver::isCategoryComplete(const LLViewerInventoryCategory* cat) const
{
    const S32 version = cat->getVersion();
    const S32 expected_num_descendents = cat->getDescendentCount();
    if ((version == LLViewerInventoryCategory::VERSION_UNKNOWN) ||
        (expected_num_descendents == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN))
    {
        return false;
    }
    // it might be complete - check known descendents against
    // currently available.
    LLInventoryModel::cat_array_t* cats;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(cat->getUUID(), cats, items);
    if (!cats || !items)
    {
        LL_WARNS() << "Category '" << cat->getName() << "' descendents corrupted, fetch failed." << LL_ENDL;
        // NULL means the call failed -- cats/items map doesn't exist (note: this does NOT mean
        // that the cat just doesn't have any items or subfolders).
        // Unrecoverable, so just return done so that this observer can be cleared
        // from memory.
        return true;
    }
    const S32 current_num_known_descendents = static_cast<S32>(cats->size() + items->size());

    // Got the number of descendents that we were expecting, so we're done.
    if (current_num_known_descendents == expected_num_descendents)
    {
        return true;
    }

    // Error condition, but recoverable.  This happens if something was added to the
    // category before it was initialized, so accountForUpdate didn't update descendent
    // count and thus the category thinks it has fewer descendents than it actually has.
    if (current_num_known_descendents >= expected_num_descendents)
    {
        LL_WARNS() << "Category '" << cat->getName() << "' expected descendentcount:" << expected_num_descendents << " descendents but got descendentcount:" << current_num_known_descendents << LL_ENDL;
        const_cast<LLViewerInventoryCategory *>(cat)->setDescendentCount(current_num_known_descendents);
        return true;
    }
    return false;
}

LLInventoryFetchComboObserver::LLInventoryFetchComboObserver(const uuid_vec_t& folder_ids,
                                                             const uuid_vec_t& item_ids)
{
    mFetchDescendents = new LLInventoryFetchDescendentsObserver(folder_ids);

    uuid_vec_t pruned_item_ids;
    for (uuid_vec_t::const_iterator item_iter = item_ids.begin();
         item_iter != item_ids.end();
         ++item_iter)
    {
        const LLUUID& item_id = (*item_iter);
        const LLViewerInventoryItem* item = gInventory.getItem(item_id);
        if (item && std::find(folder_ids.begin(), folder_ids.end(), item->getParentUUID()) == folder_ids.end())
        {
            continue;
        }
        pruned_item_ids.push_back(item_id);
    }

    mFetchItems = new LLInventoryFetchItemsObserver(pruned_item_ids);
    mFetchDescendents = new LLInventoryFetchDescendentsObserver(folder_ids);
}

LLInventoryFetchComboObserver::~LLInventoryFetchComboObserver()
{
    mFetchItems->done();
    mFetchDescendents->done();
    delete mFetchItems;
    delete mFetchDescendents;
}

void LLInventoryFetchComboObserver::changed(U32 mask)
{
    mFetchItems->changed(mask);
    mFetchDescendents->changed(mask);
    if (mFetchItems->isFinished() && mFetchDescendents->isFinished())
    {
        done();
    }
}

void LLInventoryFetchComboObserver::startFetch()
{
    mFetchItems->startFetch();
    mFetchDescendents->startFetch();
}

// See comment preceding LLInventoryAddedObserver::changed() for some
// concerns that also apply to this observer.
void LLInventoryAddItemByAssetObserver::changed(U32 mask)
{
    if(!(mask & LLInventoryObserver::ADD) ||
       !(mask & LLInventoryObserver::CREATE) ||
       !(mask & LLInventoryObserver::UPDATE_CREATE))
    {
        return;
    }

    // nothing is watched
    if (mWatchedAssets.size() == 0)
    {
        return;
    }

    const uuid_set_t& added = gInventory.getAddedIDs();
    for (uuid_set_t::iterator it = added.begin(); it != added.end(); ++it)
    {
        LLInventoryItem *item = gInventory.getItem(*it);
        if (!item)
        {
            continue;
        }
        const LLUUID& asset_uuid = item->getAssetUUID();
        if (item->getUUID().notNull() && asset_uuid.notNull())
        {
            if (isAssetWatched(asset_uuid))
            {
                LL_DEBUGS("Inventory_Move") << "Found asset UUID: " << asset_uuid << LL_ENDL;
                mAddedItems.push_back(item->getUUID());
            }
        }
    }

    if (mAddedItems.size() == mWatchedAssets.size())
    {
        LL_DEBUGS("Inventory_Move") << "All watched items are added & processed." << LL_ENDL;
        done();
        mAddedItems.clear();

        // Unable to clean watched items here due to somebody can require to check them in current frame.
        // set dirty state to clean them while next watch cycle.
        mIsDirty = true;
    }
}

void LLInventoryAddItemByAssetObserver::watchAsset(const LLUUID& asset_id)
{
    if(asset_id.notNull())
    {
        if (mIsDirty)
        {
            LL_DEBUGS("Inventory_Move") << "Watched items are dirty. Clean them." << LL_ENDL;
            mWatchedAssets.clear();
            mIsDirty = false;
        }

        mWatchedAssets.push_back(asset_id);
        onAssetAdded(asset_id);
    }
}

bool LLInventoryAddItemByAssetObserver::isAssetWatched( const LLUUID& asset_id )
{
    return std::find(mWatchedAssets.begin(), mWatchedAssets.end(), asset_id) != mWatchedAssets.end();
}

// This observer used to explicitly check for whether it was being
// called as a result of an UpdateCreateInventoryItem message. It has
// now been decoupled enough that it's not actually checking the
// message system, but now we have the special UPDATE_CREATE flag
// being used for the same purpose. Fixing this, as we would need to
// do to get rid of the message, is somewhat subtle because there's no
// particular obvious criterion for when creating a new item should
// trigger this observer and when it shouldn't. For example, creating
// a new notecard with new->notecard causes a preview window to pop up
// via the derived class LLOpenTaskOffer, but creating a new notecard
// by copy and paste does not, solely because one goes through
// UpdateCreateInventoryItem and the other doesn't.
void LLInventoryAddedObserver::changed(U32 mask)
{
    if (!(mask & LLInventoryObserver::ADD) ||
        !(mask & LLInventoryObserver::CREATE) ||
        !(mask & LLInventoryObserver::UPDATE_CREATE))
    {
        return;
    }

    if (!gInventory.getAddedIDs().empty())
    {
        done();
    }
}

void LLInventoryCategoryAddedObserver::changed(U32 mask)
{
    if (!(mask & LLInventoryObserver::ADD))
    {
        return;
    }

    const LLInventoryModel::changed_items_t& added_ids = gInventory.getAddedIDs();

    for (LLInventoryModel::changed_items_t::const_iterator cit = added_ids.begin(); cit != added_ids.end(); ++cit)
    {
        LLViewerInventoryCategory* cat = gInventory.getCategory(*cit);

        if (cat)
        {
            mAddedCategories.push_back(cat);
        }
    }

    if (!mAddedCategories.empty())
    {
        done();

        mAddedCategories.clear();
    }
}

void LLInventoryCategoriesObserver::changed(U32 mask)
{
    if (!mCategoryMap.size())
        return;

    std::vector<LLUUID> deleted_categories_ids;

    for (category_map_t::iterator iter = mCategoryMap.begin();
         iter != mCategoryMap.end();
         ++iter)
    {
        const LLUUID& cat_id = (*iter).first;
        LLCategoryData& cat_data = (*iter).second;

        LLViewerInventoryCategory* category = gInventory.getCategory(cat_id);
        if (!category)
        {
            LL_WARNS() << "Category : Category id = " << cat_id << " disappeared" << LL_ENDL;
            cat_data.mCallback();
            // Keep track of those deleted categories so we can remove them
            deleted_categories_ids.push_back(cat_id);
            continue;
        }

        const S32 version = category->getVersion();
        const S32 expected_num_descendents = category->getDescendentCount();
        if ((version == LLViewerInventoryCategory::VERSION_UNKNOWN) ||
            (expected_num_descendents == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN))
        {
            continue;
        }

        // Check number of known descendents to find out whether it has changed.
        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf(cat_id, cats, items);
        if (!cats || !items)
        {
            LL_WARNS() << "Category '" << category->getName() << "' descendents corrupted, fetch failed." << LL_ENDL;
            // NULL means the call failed -- cats/items map doesn't exist (note: this does NOT mean
            // that the cat just doesn't have any items or subfolders).
            // Unrecoverable, so just skip this category.

            llassert(cats != NULL && items != NULL);

            continue;
        }

        const S32 current_num_known_descendents = static_cast<S32>(cats->size() + items->size());

        bool cat_changed = false;

        // If category version or descendents count has changed
        // update category data in mCategoryMap
        if (version != cat_data.mVersion || current_num_known_descendents != cat_data.mDescendentsCount)
        {
            cat_data.mVersion = version;
            cat_data.mDescendentsCount = current_num_known_descendents;
            cat_changed = true;
        }

        // If any item names have changed, update the name hash
        // Only need to check if (a) name hash has not previously been
        // computed, or (b) a name has changed.
        if (!cat_data.mIsNameHashInitialized || (mask & LLInventoryObserver::LABEL))
        {
            digest_t item_name_hash = gInventory.hashDirectDescendentNames(cat_id);
            if (cat_data.mItemNameHash != item_name_hash)
            {
                cat_data.mIsNameHashInitialized = true;
                cat_data.mItemNameHash = item_name_hash;
                cat_changed = true;
            }
        }

        const LLUUID thumbnail_id = category->getThumbnailUUID();
        if (cat_data.mThumbnailId != thumbnail_id)
        {
            cat_data.mThumbnailId = thumbnail_id;
            cat_changed = true;
        }

        bool is_favorite = category->getIsFavorite();
        if (cat_data.mIsFavorite != is_favorite)
        {
            cat_data.mIsFavorite = is_favorite;
            cat_changed = true;
        }

        // If anything has changed above, fire the callback.
        if (cat_changed)
            cat_data.mCallback();
    }

    // Remove deleted categories from the list
    for (std::vector<LLUUID>::iterator deleted_id = deleted_categories_ids.begin(); deleted_id != deleted_categories_ids.end(); ++deleted_id)
    {
        removeCategory(*deleted_id);
    }
}

bool LLInventoryCategoriesObserver::addCategory(const LLUUID& cat_id, callback_t cb, bool init_name_hash)
{
    S32 version = LLViewerInventoryCategory::VERSION_UNKNOWN;
    S32 current_num_known_descendents = LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN;
    bool can_be_added = true;
    bool favorite = false;
    LLUUID thumbnail_id;

    LLViewerInventoryCategory* category = gInventory.getCategory(cat_id);
    // If category could not be retrieved it might mean that
    // inventory is unusable at the moment so the category is
    // stored with VERSION_UNKNOWN and DESCENDENT_COUNT_UNKNOWN,
    // it may be updated later.
    if (category)
    {
        // Inventory category version is used to find out if some changes
        // to a category have been made.
        version = category->getVersion();
        thumbnail_id = category->getThumbnailUUID();
        favorite = category->getIsFavorite();

        LLInventoryModel::cat_array_t* cats;
        LLInventoryModel::item_array_t* items;
        gInventory.getDirectDescendentsOf(cat_id, cats, items);
        if (!cats || !items)
        {
            LL_WARNS() << "Category '" << category->getName() << "' descendents corrupted, fetch failed." << LL_ENDL;
            // NULL means the call failed -- cats/items map doesn't exist (note: this does NOT mean
            // that the cat just doesn't have any items or subfolders).
            // Unrecoverable, so just return "false" meaning that the category can't be observed.
            can_be_added = false;

            llassert(cats != NULL && items != NULL);
        }
        else
        {
            current_num_known_descendents = static_cast<S32>(cats->size() + items->size());
        }
    }

    if (can_be_added)
    {
        if(init_name_hash)
        {
            digest_t item_name_hash = gInventory.hashDirectDescendentNames(cat_id);
            mCategoryMap.insert(category_map_value_t(cat_id,LLCategoryData(cat_id, thumbnail_id, favorite, cb, version, current_num_known_descendents,item_name_hash)));
        }
        else
        {
            mCategoryMap.insert(category_map_value_t(cat_id,LLCategoryData(cat_id, thumbnail_id, favorite, cb, version, current_num_known_descendents)));
        }
    }

    return can_be_added;
}

void LLInventoryCategoriesObserver::removeCategory(const LLUUID& cat_id)
{
    mCategoryMap.erase(cat_id);
}

LLInventoryCategoriesObserver::LLCategoryData::LLCategoryData(
    const LLUUID& cat_id,
    const LLUUID& thumbnail_id,
    bool is_favorite,
    callback_t cb,
    S32 version,
    S32 num_descendents)

    : mCatID(cat_id)
    , mCallback(cb)
    , mVersion(version)
    , mDescendentsCount(num_descendents)
    , mThumbnailId(thumbnail_id)
    , mIsFavorite(is_favorite)
    , mIsNameHashInitialized(false)
{
}

LLInventoryCategoriesObserver::LLCategoryData::LLCategoryData(
    const LLUUID& cat_id,
    const LLUUID& thumbnail_id,
    bool is_favorite,
    callback_t cb, S32 version,
    S32 num_descendents,
    const digest_t& name_hash)

    : mCatID(cat_id)
    , mCallback(cb)
    , mVersion(version)
    , mDescendentsCount(num_descendents)
    , mThumbnailId(thumbnail_id)
    , mIsFavorite(is_favorite)
    , mIsNameHashInitialized(true)
    , mItemNameHash(name_hash)
{
}

void LLScrollOnRenameObserver::changed(U32 mask)
{
    if (mask & LLInventoryObserver::LABEL)
    {
        const uuid_set_t& changed_item_ids = gInventory.getChangedIDs();
        for (uuid_set_t::const_iterator it = changed_item_ids.begin(); it != changed_item_ids.end(); ++it)
        {
            const LLUUID& id = *it;
            if (id == mUUID)
            {
                mView->scrollToShowSelection();

                gInventory.removeObserver(this);
                delete this;
                return;
            }
        }
    }
}
