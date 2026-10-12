/**
 * @file llinventorymodelbackgroundfetch.cpp
 * @brief Implementation of background fetching of inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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
#include "llinventorymodelbackgroundfetch.h"

#include "llaisapi.h"
#include "llagent.h"
#include "llappviewer.h"
#include "llappearancemgr.h"
#include "llcallbacklist.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "llnotificationsutil.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermessage.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llhttpconstants.h"
#include "bufferarray.h"
#include "bufferstream.h"
#include "llcorehttputil.h"

// History (may be apocryphal)
//
// Around V2, an HTTP inventory download mechanism was added
// along with inventory LINK items referencing other inventory
// items.  As part of this, at login, the entire inventory
// structure is downloaded 'in the background' using the
// backgroundFetch()/bulkFetch() methods.  The UDP path can
// still be used and is found in the 'DEPRECATED OLD CODE'
// section.
//
// The old UDP path implemented a throttle that adapted
// itself during running.  The mechanism survived info HTTP
// somewhat but was pinned to poll the HTTP plumbing at
// 0.5S intervals.  The reasons for this particular value
// have been lost.  It's possible to switch between UDP
// and HTTP while this is happening but there may be
// surprises in what happens in that case.
//
// Conversion to llcorehttp reduced the number of connections
// used but batches more data and queues more requests (but
// doesn't due pipelining due to libcurl restrictions).  The
// poll interval above was re-examined and reduced to get
// inventory into the viewer more quickly.
//
// Possible future work:
//
// * Don't download the entire heirarchy in one go (which
//   might have been how V1 worked).  Implications for
//   links (which may not have a valid target) and search
//   which would then be missing data.
//
// * Review the download rate throttling.  Slow then fast?
//   Detect bandwidth usage and speed up when it drops?
//
// * An error on a fetch could be due to one item in the batch.
//   If the batch were broken up, perhaps more of the inventory
//   would download.  (Handwave here, not certain this is an
//   issue in practice.)
//
// * Conversion to AISv3.
//


namespace
{

///----------------------------------------------------------------------------
/// Class <anonymous>::BGItemHttpHandler
///----------------------------------------------------------------------------

//
// Http request handler class for single inventory item requests.
//
// We'll use a handler-per-request pattern here rather than
// a shared handler.  Mainly convenient as this was converted
// from a Responder class model.
//
// Derives from and is identical to the normal FetchItemHttpHandler
// except that:  1) it uses the background request object which is
// updated more slowly than the foreground and 2) keeps a count of
// active requests on the LLInventoryModelBackgroundFetch object
// to indicate outstanding operations are in-flight.
//
class BGItemHttpHandler : public LLInventoryModel::FetchItemHttpHandler
{
    LOG_CLASS(BGItemHttpHandler);

public:
    BGItemHttpHandler(const LLSD& request_sd)
        : LLInventoryModel::FetchItemHttpHandler(request_sd)
        {
            LLInventoryModelBackgroundFetch::instance().incrFetchCount(1);
        }

    virtual ~BGItemHttpHandler()
        {
            LLInventoryModelBackgroundFetch::instance().incrFetchCount(-1);
        }

    BGItemHttpHandler(const BGItemHttpHandler&) = delete;
    BGItemHttpHandler& operator=(const BGItemHttpHandler&) = delete;
};


///----------------------------------------------------------------------------
/// Class <anonymous>::BGFolderHttpHandler
///----------------------------------------------------------------------------

// Http request handler class for folders.
//
// Handler for FetchInventoryDescendents2 and FetchLibDescendents2
// caps requests for folders.
//
class BGFolderHttpHandler : public LLCore::HttpHandler
{
    LOG_CLASS(BGFolderHttpHandler);

public:
    BGFolderHttpHandler(const LLSD& request_sd, const uuid_vec_t& recursive_cats)
        : LLCore::HttpHandler(),
          mRequestSD(request_sd),
          mRecursiveCatUUIDs(recursive_cats)
        {
            LLInventoryModelBackgroundFetch::instance().incrFetchCount(1);
        }

    virtual ~BGFolderHttpHandler()
        {
            LLInventoryModelBackgroundFetch::instance().incrFetchCount(-1);
        }

protected:
    BGFolderHttpHandler(const BGFolderHttpHandler&) = delete;
    BGFolderHttpHandler& operator=(const BGFolderHttpHandler&) = delete;

public:
    virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse* response);

    bool getIsRecursive(const LLUUID& cat_id) const;

private:
    void processData(LLSD& body, LLCore::HttpResponse* response);
    void processFailure(LLCore::HttpStatus status, LLCore::HttpResponse* response);
    void processFailure(const char* const reason, LLCore::HttpResponse* response);

private:
    LLSD mRequestSD;
    const uuid_vec_t mRecursiveCatUUIDs; // hack for storing away which cat fetches are recursive
};


const char* const LOG_INV("Inventory");

} // end of namespace anonymous


///----------------------------------------------------------------------------
/// Class LLInventoryModelBackgroundFetch
///----------------------------------------------------------------------------

LLInventoryModelBackgroundFetch::LLInventoryModelBackgroundFetch():
    mBackgroundFetchActive(false),
    mFolderFetchActive(false),
    mFetchCount(0),
    mLastFetchCount(0),
    mFetchFolderCount(0),
    mInitialFetchDuration(0.f),
    mInitialFetchDurationCaptured(false),
    mAllRecursiveFoldersFetched(false),
    mRecursiveInventoryFetchStarted(false),
    mRecursiveLibraryFetchStarted(false),
    mRecursiveMarketplaceFetchStarted(false),
    mMinTimeBetweenFetches(0.3f)
{}

void LLInventoryModelBackgroundFetch::markFetchStarted()
{
    if (!mBackgroundFetchActive)
    {
        mFetchStartTimer.reset();
    }
    mBackgroundFetchActive = true;
}

LLInventoryModelBackgroundFetch::~LLInventoryModelBackgroundFetch()
{
    gIdleCallbacks.deleteFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
}

bool LLInventoryModelBackgroundFetch::isBulkFetchProcessingComplete() const
{
    return mFetchFolderQueue.empty() && mFetchItemQueue.empty() && mFetchCount <= 0;
}

bool LLInventoryModelBackgroundFetch::isFolderFetchProcessingComplete() const
{
    return mFetchFolderQueue.empty() && mFetchFolderCount <= 0;
}

bool LLInventoryModelBackgroundFetch::libraryFetchStarted() const
{
    return mRecursiveLibraryFetchStarted;
}

bool LLInventoryModelBackgroundFetch::libraryFetchCompleted() const
{
    return libraryFetchStarted() && fetchQueueContainsNoDescendentsOf(gInventory.getLibraryRootFolderID());
}

bool LLInventoryModelBackgroundFetch::libraryFetchInProgress() const
{
    return libraryFetchStarted() && !libraryFetchCompleted();
}

bool LLInventoryModelBackgroundFetch::inventoryFetchStarted() const
{
    return mRecursiveInventoryFetchStarted;
}

bool LLInventoryModelBackgroundFetch::inventoryFetchCompleted() const
{
    return inventoryFetchStarted() && fetchQueueContainsNoDescendentsOf(gInventory.getRootFolderID());
}

bool LLInventoryModelBackgroundFetch::inventoryFetchInProgress() const
{
    return inventoryFetchStarted() && !inventoryFetchCompleted();
}

bool LLInventoryModelBackgroundFetch::isEverythingFetched() const
{
    return mAllRecursiveFoldersFetched;
}

bool LLInventoryModelBackgroundFetch::folderFetchActive() const
{
    return mFolderFetchActive;
}

void LLInventoryModelBackgroundFetch::addRequestAtFront(const LLUUID& id, bool recursive, bool is_category)
{
    EFetchType recursion_type = recursive ? FT_RECURSIVE : FT_DEFAULT;
    if (is_category)
    {
        mFetchFolderQueue.emplace_front(id, recursion_type, is_category);
    }
    else
    {
        mFetchItemQueue.emplace_front(id, recursion_type, is_category);
    }
}

void LLInventoryModelBackgroundFetch::addRequestAtBack(const LLUUID& id, bool recursive, bool is_category)
{
    EFetchType recursion_type = recursive ? FT_RECURSIVE : FT_DEFAULT;
    if (is_category)
    {
        mFetchFolderQueue.emplace_back(id, recursion_type, is_category);
    }
    else
    {
        mFetchItemQueue.emplace_back(id, recursion_type, is_category);
    }
}

void LLInventoryModelBackgroundFetch::setDeferredCache(const LLUUID& owner_id, std::shared_ptr<LLCacheLoadResult> cache_result)
{
    if (!cache_result)
    {
        return;
    }
    mDeferredCaches[owner_id] = cache_result;
}

bool LLInventoryModelBackgroundFetch::applyDeferredCache(const LLUUID& cat_id)
{
    if (mDeferredCaches.empty())
    {
        return false;
    }

    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
    if (!cat)
    {
        LL_DEBUGS(LOG_INV) << "applyDeferredCache: category " << cat_id
            << " not found in model, skipping." << LL_ENDL;
        return false;
    }

    // Either owned by the library or the agent
    LLUUID owner_id = cat->getOwnerID();
    llassert(owner_id == gAgent.getID() || owner_id == gInventory.getLibraryOwnerID());
    auto cache_it = mDeferredCaches.find(owner_id);
    if (cache_it == mDeferredCaches.end())
    {
        return false;
    }

    LLCacheLoadResult* cache_result = cache_it->second.get();
    if (!cache_result || cache_result->mValidDeferredCats.find(cat_id) == cache_result->mValidDeferredCats.end())
    {
        return false;
    }

    if (!cat || cat->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN)
    {
        // Already fetched/merged (e.g. was a priority folder), nothing to do.
        LL_DEBUGS(LOG_INV) << "applyDeferredCache: category " << cat_id
            << " already has a known version (" << cat->getVersion()
            << "), dropping from mValidDeferredCats without applying." << LL_ENDL;
        cache_result->mValidDeferredCats.erase(cat_id);
        return false;
    }

    // Find the cached category to pick up its confirmed version.
    auto cached_cat_it = cache_result->mCategories.find(cat_id);
    if (cached_cat_it == cache_result->mCategories.end())
    {
        LL_WARNS(LOG_INV) << "applyDeferredCache: category " << cat_id
            << " was in mValidDeferredCats but missing from mCategories - "
            << "inconsistent cache state." << LL_ENDL;
        return false;
    }
    S32 cached_version = cached_cat_it->second->getVersion();
    if (cached_version == LLViewerInventoryCategory::VERSION_UNKNOWN)
    {
        LL_WARNS(LOG_INV) << "applyDeferredCache: category " << cat_id
            << " was in mValidDeferredCats but has VERSION_UNKNOWN in cache - "
            << "inconsistent cache state." << LL_ENDL;
        return false;
    }

    // Done with this category - drop it so a later forced re-fetch (e.g.
    // something resets the category back to VERSION_UNKNOWN) does not try
    // to reapply the same cached items a second time.
    cache_result->mValidDeferredCats.erase(cat_id);

    // Follow links to not leave behind broken ones.
    auto resolve_link = [cache_result](LLViewerInventoryItem* link_item, std::vector<LLViewerInventoryItem*>& chain) -> bool
    {
        uuid_set_t seen;
        LLViewerInventoryItem* current = link_item;
        while (current)
        {
            if (gInventory.getItem(current->getUUID()))
            {
                // Already committed to the live model.
                return true;
            }

            auto parent_it = cache_result->mCategories.find(current->getParentUUID());
            if (parent_it == cache_result->mCategories.end())
            {
                // An orphan, can't resolve.
                return false;
            }

            S32 parent_version = parent_it->second->getVersion();
            if (parent_version == LLViewerInventoryCategory::VERSION_UNKNOWN)
            {
                // Parent folder's version isn't confirmed - we can't trust
                // that this link's target still belongs there, don't follow.
                return false;
            }

            LLViewerInventoryCategory* live_parent = gInventory.getCategory(current->getParentUUID());
            if (!live_parent)
            {
                // Folder was removed already. Don't follow.
                return false;
            }

            if (live_parent->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN)
            {
                // Parent folder is already in the live model, getItem failing
                // means target was moved or removed, so don't follow the link.
                //
                // Or could have checked mValidDeferredCats.
                return false;
            }

            if (current->getActualType() == LLAssetType::AT_LINK_FOLDER)
            {
                // Folder links point at a category, since we are dealing with cache,
                // all folders cache is aware off should already be 'live' from the
                // skeleton load.
                LLViewerInventoryCategory* linked_cat = gInventory.getCategory(current->getLinkedUUID());
                if (linked_cat)
                {
                    // Target folder is already live - the link itself is the
                    // final thing that needs to be added.
                    chain.push_back(current);
                    return true;
                }

                // Target folder isn't live yet. Link is dead.
                return false;
            }

            if (current->getActualType() != LLAssetType::AT_LINK)
            {
                // Reached a non-link item that isn't in the model yet -
                // final target, needs to be added.
                chain.push_back(current);
                return true;
            }

            if (!seen.insert(current->getUUID()).second)
            {
                // Cycle - can't resolve.
                return false;
            }
            chain.push_back(current);

            auto next_it = cache_result->mItems.find(current->getLinkedUUID());
            if (next_it == cache_result->mItems.end())
            {
                // Next link/target isn't present in cache at all. It may
                // still be resolvable later (e.g. via network fetch, or a
                // different deferred category being applied first), so
                // don't treat this as fully broken here.
                return false;
            }
            current = next_it->second.get();
        }
        return false;
    };

    // Emulate a completed fetch: feed items through the normal
    // create/update path (same as AISUpdate::doUpdate()'s "CREATE ITEMS"
    // loop) so observers/listeners waiting on these specific items get
    // notified.
    S32 added_items = 0;
    S32 skipped_items = 0;
    S32 missing_items = 0;
    S32 unresolved_links = 0;
    auto child_it = cache_result->mChildItemIds.find(cat_id);
    if (child_it != cache_result->mChildItemIds.end())
    {
        for (const LLUUID& item_id : child_it->second)
        {
            if (gInventory.getItem(item_id))
            {
                skipped_items++;
                continue; // already present (somehow)was followed by a link or requested directly)
            }
            auto found_item = cache_result->mItems.find(item_id);
            if (found_item == cache_result->mItems.end())
            {
                missing_items++;
                continue;
            }

            LLViewerInventoryItem* item = found_item->second.get();
            if (item->getActualType() == LLAssetType::AT_LINK
                || item->getActualType() == LLAssetType::AT_LINK_FOLDER)
            {
                std::vector<LLViewerInventoryItem*> chain;
                if (resolve_link(item, chain))
                {
                    for (auto chain_it = chain.rbegin(); chain_it != chain.rend(); ++chain_it)
                    {
                        LLViewerInventoryItem* chain_item = *chain_it;
                        if (!gInventory.getItem(chain_item->getUUID()))
                        {
                            chain_item->setComplete(true); // warning supression
                            gInventory.updateItem(chain_item, LLInventoryObserver::CREATE);
                            added_items++;
                        }
                    }
                }
                else
                {
                    // Could not resolve the link's target from this cache
                    // right now (target not cached, or a cycle). Leave it
                    // out - it'll come through normal fetch/validation
                    // instead of being added as a broken link.
                    unresolved_links++;
                }
            }
            else
            {
                item->setComplete(true); // warning supression, consider cache to be complete
                gInventory.updateItem(item, LLInventoryObserver::CREATE);
                added_items++;
            }
        }
    }

    // Category already exists (created during loadSkeleton()'s skeleton
    // merge) but is still at VERSION_UNKNOWN. Mirror AISUpdate::parseCategory()'s
    // "curr_cat exists" path: stage an updated copy with the confirmed
    // version/descendant count and commit it via updateCategory(), so the
    // change goes through addChangedMask() like a real fetch response -
    // rather than mutating cat in place, which observers wouldn't see.
    LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);

    // At the moment skeleton download does not provide a thumbnail uuid or favorite,
    // use values from cache. Mirrors loadSkeleton()'s handling of the same metadata.
    new_cat->setThumbnailUUID(cached_cat_it->second->getThumbnailUUID());
    new_cat->setFavorite(cached_cat_it->second->getIsFavorite());

    if (unresolved_links > 0)
    {
        // Mirror loadSkeleton()'s handling of broken links: don't confirm
        // this folder's version/descendant count if some of its links
        // couldn't be resolved, so a subsequent real fetch of this folder
        // is issued and will re-fetch/resolve the links normally. Items
        // that did resolve above remain in the live model, same as
        // loadSkeleton().
        LL_DEBUGS(LOG_INV) << "applyDeferredCache: category " << cat_id
            << " ('" << cat->getName() << "')"
            << " has " << unresolved_links
            << " unresolved link(s), not confirming version - will be re-fetched."
            << LL_ENDL;

        gInventory.updateCategory(new_cat);
        // Note that observers aren't notified here,
        // Caller can load multiple folders fron cache,
        // so it's caller's job to call notifyObservers()
        // for bulk processing of loaded folders.

        cache_result->mNumItemsLoaded += added_items;
        return false;
    }

    new_cat->setVersion(cached_version);
    new_cat->setDescendentCount(cat->getViewerDescendentCount());

    gInventory.updateCategory(new_cat);
    // Note that observers aren't notified here,
    // Caller can load multiple folders fron cache,
    // so it's caller's job to call notifyObservers()
    // for bulk processing of loaded folders.

    LL_DEBUGS(LOG_INV) << "Applied deferred cache for category " << cat_id
        << " ('" << cat->getName() << "')"
        << ", version " << cached_version
        << ", added " << added_items << " items"
        << ", skipped (already present) " << skipped_items
        << ", missing from cache " << missing_items
        << ", unresolved links " << unresolved_links
        << ", remaining valid deferred cats " << cache_result->mValidDeferredCats.size()
        << LL_ENDL;

    cache_result->mNumCatsLoaded++;
    cache_result->mNumItemsLoaded += added_items;
    return true;
}

void LLInventoryModelBackgroundFetch::invalidateDeferredCache(const LLUUID& cat_id)
{
    for (auto& cache_pair : mDeferredCaches)
    {
        size_t erased = cache_pair.second->mValidDeferredCats.erase(cat_id);
        if (erased)
        {
            LL_DEBUGS(LOG_INV) << "invalidateDeferredCache: dropped category " << cat_id
                << " from owner " << cache_pair.first
                << "'s mValidDeferredCats (remaining size "
                << cache_pair.second->mValidDeferredCats.size() << ")." << LL_ENDL;
        }
    }
}

void LLInventoryModelBackgroundFetch::invalidateDeferredCache(const LLUUID& owner_id, const LLUUID& cat_id)
{
    if (mDeferredCaches.empty())
    {
        return;
    }

    auto cache_it = mDeferredCaches.find(owner_id);
    if (cache_it == mDeferredCaches.end() || !cache_it->second)
    {
        return;
    }

    size_t erased = cache_it->second->mValidDeferredCats.erase(cat_id);
    if (erased)
    {
        LL_DEBUGS(LOG_INV) << "invalidateDeferredCache: dropped category " << cat_id
            << " from owner " << owner_id
            << "'s mValidDeferredCats (remaining size "
            << cache_it->second->mValidDeferredCats.size() << ")." << LL_ENDL;
    }
}

void LLInventoryModelBackgroundFetch::invalidateDeferredCache(const LLUUID& owner_id, const uuid_vec_t& cat_ids)
{
    if (mDeferredCaches.empty() || cat_ids.empty())
    {
        return;
    }

    auto cache_it = mDeferredCaches.find(owner_id);
    if (cache_it == mDeferredCaches.end() || !cache_it->second)
    {
        return;
    }

    uuid_set_t& valid_cats = cache_it->second->mValidDeferredCats;
    size_t erased = 0;
    for (const LLUUID& cat_id : cat_ids)
    {
        erased += valid_cats.erase(cat_id);
    }
    if (erased)
    {
        LL_DEBUGS(LOG_INV) << "invalidateDeferredCache: dropped " << erased
            << " categories from owner " << owner_id
            << "'s mValidDeferredCats (remaining size "
            << valid_cats.size() << ")." << LL_ENDL;
    }
}

void LLInventoryModelBackgroundFetch::start(const LLUUID& id, bool recursive)
{
    LLViewerInventoryCategory* cat(gInventory.getCategory(id));

    if (cat || (id.isNull() && ! isEverythingFetched()))
    {
        // it's a folder, do a bulk fetch
        LL_DEBUGS(LOG_INV) << "Start fetching category: " << id << ", recursive: " << recursive << LL_ENDL;

        markFetchStarted();
        mFolderFetchActive = true;
        EFetchType recursion_type = recursive ? FT_RECURSIVE : FT_DEFAULT;
        if (id.isNull())
        {
            if (!mRecursiveInventoryFetchStarted)
            {
                mRecursiveInventoryFetchStarted |= recursive;
                if (recursive && AISAPI::isAvailable())
                {
                    // Not only root folder can be massive, but
                    // most system folders will be requested independently
                    // so request root folder and content separately
                    mFetchFolderQueue.emplace_front(gInventory.getRootFolderID(), FT_FOLDER_AND_CONTENT);
                }
                else
                {
                    mFetchFolderQueue.emplace_back(gInventory.getRootFolderID(), recursion_type);
                }
                gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
            }
            if (!mRecursiveLibraryFetchStarted)
            {
                mRecursiveLibraryFetchStarted |= recursive;
                mFetchFolderQueue.emplace_back(gInventory.getLibraryRootFolderID(), recursion_type);
                gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
            }
        }
        else if (recursive && cat && cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_LISTINGS)
        {
            if (mFetchFolderQueue.empty() || mFetchFolderQueue.back().mUUID != id)
            {
                if (recursive && AISAPI::isAvailable())
                {
                    // Request marketplace folder and content separately
                    mFetchFolderQueue.emplace_front(id, FT_FOLDER_AND_CONTENT);
                }
                else
                {
                    mFetchFolderQueue.emplace_front(id, recursion_type);
                }
                gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
                mRecursiveMarketplaceFetchStarted = true;
            }
        }
        else
        {
            if (AISAPI::isAvailable())
            {
                if (mFetchFolderQueue.empty() || mFetchFolderQueue.back().mUUID != id)
                {
                    // On AIS make sure root goes to the top and follow up recursive
                    // fetches, not individual requests
                    mFetchFolderQueue.emplace_back(id, recursion_type);
                    gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
                }
            }
            else if (mFetchFolderQueue.empty() || mFetchFolderQueue.front().mUUID != id)
            {
                // Specific folder requests go to front of queue.
                mFetchFolderQueue.emplace_front(id, recursion_type);
                gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
            }

            if (id == gInventory.getLibraryRootFolderID())
            {
                mRecursiveLibraryFetchStarted |= recursive;
            }
            if (id == gInventory.getRootFolderID())
            {
                mRecursiveInventoryFetchStarted |= recursive;
            }
        }
    }
    else if (LLViewerInventoryItem* itemp = gInventory.getItem(id))
    {
        if (!itemp->mIsComplete)
        {
            scheduleItemFetch(id);
        }
    }
}

void LLInventoryModelBackgroundFetch::scheduleFolderFetch(const LLUUID& cat_id, bool forced)
{
    if (mFetchFolderQueue.empty() || mFetchFolderQueue.front().mUUID != cat_id)
    {
        markFetchStarted();
        mFolderFetchActive = true;

        if (forced)
        {
            // A forced fetch explicitly wants fresh network data - any cached
            // version we might have been prepared to reuse for this folder is
            // no longer trustworthy, so stop background fetch from
            // considering it later.
            invalidateDeferredCache(cat_id);

            // check if already requested
            if (mForceFetchSet.find(cat_id) == mForceFetchSet.end())
            {
                mForceFetchSet.emplace(cat_id);
                mFetchFolderQueue.emplace_front(cat_id, FT_FORCED);
            }
        }
        else
        {
            // Specific folder requests go to front of queue.
            // version presence acts as duplicate prevention for normal fetches
            mFetchFolderQueue.emplace_front(cat_id, FT_DEFAULT);
        }

        gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
    }
}

void LLInventoryModelBackgroundFetch::scheduleItemFetch(const LLUUID& item_id, bool forced)
{
    // Do not load item requests from cache, they have no version control,
    // and checking parent won't tell much. Just assume cache is stale.

    if (mFetchItemQueue.empty() || mFetchItemQueue.front().mUUID != item_id)
    {
        markFetchStarted();
        if (forced)
        {
            // check if already requested
            if (mForceFetchSet.find(item_id) == mForceFetchSet.end())
            {
                mForceFetchSet.emplace(item_id);
                mFetchItemQueue.emplace_front(item_id, FT_FORCED, false);
            }
        }
        else
        {
            // 'isFinished' being set acts as duplicate prevention for normal fetches
            mFetchItemQueue.emplace_front(item_id, FT_DEFAULT, false);
        }

        gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
    }
}

void LLInventoryModelBackgroundFetch::fetchFolderAndLinks(const LLUUID& cat_id, nullary_func_t callback)
{
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
    if (cat)
    {
        // Mark folder (update timer) so that background fetch won't request it
        cat->setFetching(LLViewerInventoryCategory::FETCH_RECURSIVE);
    }
    incrFetchFolderCount(1);
    mExpectedFolderIds.emplace_back(cat_id);

    // Assume that we have no relevant cache. Fetch folder, and items folder's links point to.
    AISAPI::FetchCategoryLinks(cat_id,
                               [callback, cat_id](const LLUUID& id)
                               {
                                   callback();
                                   if (id.isNull())
                                   {
                                       LL_WARNS() << "Failed to fetch category links " << cat_id << LL_ENDL;
                                   }
                                   LLInventoryModelBackgroundFetch::getInstance()->onAISFolderCalback(cat_id, gAgent.getID(), id, FT_DEFAULT);
                               });

    // start idle loop to track completion
    markFetchStarted();
    mFolderFetchActive = true;
    gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
}

void LLInventoryModelBackgroundFetch::fetchCOF(nullary_func_t callback)
{
    LLUUID cat_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
    if (cat)
    {
        // Mark cof (update timer) so that background fetch won't request it
        cat->setFetching(LLViewerInventoryCategory::FETCH_RECURSIVE);
    }
    incrFetchFolderCount(1);
    mExpectedFolderIds.emplace_back(cat_id);
    // For reliability assume that we have no relevant cache, so
    // fetch cof along with items cof's links point to.
    AISAPI::FetchCOF([callback](const LLUUID& id)
                     {
                         callback();
                         LLUUID cat_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
                         LLInventoryModelBackgroundFetch::getInstance()->onAISFolderCalback(cat_id, gAgent.getID(), id, FT_DEFAULT);

                         if (id.notNull())
                         {
                             // COF might have fetched base outfit folder through a link, but it hasn't
                             // fetched base outfit's content, which doesn't nessesary match COF,
                             // so make sure it's up to date
                             LLUUID baseoutfit_id = LLAppearanceMgr::getInstance()->getBaseOutfitUUID();
                             if (baseoutfit_id.notNull())
                             {
                                 LLViewerInventoryCategory* cat = gInventory.getCategory(baseoutfit_id);
                                 if (!cat || cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
                                 {
                                     LLInventoryModelBackgroundFetch::getInstance()->fetchFolderAndLinks(baseoutfit_id, no_op);
                                 }
                             }
                         }
                     });

    // start idle loop to track completion
    markFetchStarted();
    mFolderFetchActive = true;
    gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
}

void LLInventoryModelBackgroundFetch::findLostItems()
{
    markFetchStarted();
    mFolderFetchActive = true;
    mFetchFolderQueue.emplace_back(LLUUID::null, FT_RECURSIVE);
    gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
}

void LLInventoryModelBackgroundFetch::setAllFoldersFetched()
{
    if (mRecursiveInventoryFetchStarted &&
        mRecursiveLibraryFetchStarted)
    {
        mAllRecursiveFoldersFetched = true;
        //LL_INFOS(LOG_INV) << "All folders fetched, validating" << LL_ENDL;
        //gInventory.validate();

        for (const auto& cache_pair : mDeferredCaches)
        {
            const LLUUID& owner_id = cache_pair.first;
            const auto& cache_result = cache_pair.second;
            if (!cache_result)
            {
                continue;
            }

            if (cache_result->mValidDeferredCats.size() > 0)
            {
                // We use recursive fetch, everything should have been fetched.
                // But given that ais can fetch with 'depth', it might have gotten
                // something that we had in cache.
                LL_WARNS(LOG_INV) << "Inventory " << owner_id
                    << ": loaded " << cache_result->mNumCatsLoaded << " categories and "
                    << cache_result->mNumItemsLoaded << " items from cache"
                    << ", " << cache_result->mValidDeferredCats.size()
                    << " valid deferred categories left unapplied"
                    << LL_ENDL;
            }
            else
            {
                LL_INFOS(LOG_INV) << "Inventory " << owner_id
                    << ": loaded " << cache_result->mNumCatsLoaded << " categories and "
                    << cache_result->mNumItemsLoaded << " items from cache."
                    << LL_ENDL;
            }
        }

        // The deferred on-disk cache has done its job
        mDeferredCaches.clear();
    }

    mFolderFetchActive = false;
    if (isBulkFetchProcessingComplete())
    {
        if (mAllRecursiveFoldersFetched && !mInitialFetchDurationCaptured)
        {
            mInitialFetchDuration = mFetchStartTimer.getElapsedTimeF32();
            mInitialFetchDurationCaptured = true;
        }
        mBackgroundFetchActive = false;
    }

    // For now only informs about initial fetch being done
    mFoldersFetchedSignal();

    LL_INFOS(LOG_INV) << "Inventory background fetch completed after: " << mFetchStartTimer.getElapsedTimeF32() << LL_ENDL;
}

boost::signals2::connection LLInventoryModelBackgroundFetch::setFetchCompletionCallback(folders_fetched_callback_t cb)
{
    return mFoldersFetchedSignal.connect(cb);
}

void LLInventoryModelBackgroundFetch::backgroundFetchCB(void*)
{
    LLInventoryModelBackgroundFetch::instance().backgroundFetch();
}

void LLInventoryModelBackgroundFetch::backgroundFetch()
{
    if (mBackgroundFetchActive)
    {
        if (AISAPI::isAvailable())
        {
            bulkFetchViaAis();
        }
        else if (gAgent.getRegion() && gAgent.getRegion()->capabilitiesReceived())
        {
            // If we'll be using the capability, we'll be sending batches and the background thing isn't as important.
            bulkFetch();
        }
    }
}

void LLInventoryModelBackgroundFetch::incrFetchCount(S32 fetching)
{
    mFetchCount += fetching;
    if (mFetchCount < 0)
    {
        LL_WARNS_ONCE(LOG_INV) << "Inventory fetch count fell below zero (0)." << LL_ENDL;
        mFetchCount = 0;
    }
}
void LLInventoryModelBackgroundFetch::incrFetchFolderCount(S32 fetching)
{
    incrFetchCount(fetching);
    mFetchFolderCount += fetching;
    if (mFetchCount < 0)
    {
        LL_WARNS_ONCE(LOG_INV) << "Inventory fetch count fell below zero (0)." << LL_ENDL;
        mFetchFolderCount = 0;
    }
}

void ais_simple_item_callback(const LLUUID& inv_id)
{
    LL_DEBUGS(LOG_INV , "AIS3") << "Response for " << inv_id << LL_ENDL;
    LLInventoryModelBackgroundFetch::instance().incrFetchCount(-1);
}

void LLInventoryModelBackgroundFetch::onAISContentCalback(
    const LLUUID& request_id,
    const uuid_vec_t& content_ids,
    const LLUUID& owner_id,
    const LLUUID& response_id,
    EFetchType fetch_type)
{
    // Don't emplace_front on failure - there is a chance it was fired from inside bulkFetchViaAis
    incrFetchFolderCount(-1);

    if (response_id.notNull())
    {
        // FetchCategorySubset() is always issued with recursive=true, so a
        // successful response means every folder in content_ids had its
        // full subtree fetched from network, not just itself. Any deferred
        // on-disk cache entry for those folders (and their descendants) is
        // now superseded - make sure applyDeferredCache() can't later
        // clobber the freshly-fetched data with a stale cached version.
        invalidateDeferredCache(owner_id, content_ids);
    }

    uuid_vec_t::const_iterator folder_iter = content_ids.begin();
    uuid_vec_t::const_iterator folder_end = content_ids.end();
    while (folder_iter != folder_end)
    {
        std::list<LLUUID>::const_iterator found = std::find(mExpectedFolderIds.begin(), mExpectedFolderIds.end(), *folder_iter);
        if (found != mExpectedFolderIds.end())
        {
            mExpectedFolderIds.erase(found);
        }

        LLViewerInventoryCategory* cat(gInventory.getCategory(*folder_iter));
        if (cat)
        {
            cat->setFetching(LLViewerInventoryCategory::FETCH_NONE);
        }
        if (response_id.isNull())
        {
            // Failed to fetch, get it individually
            mFetchFolderQueue.emplace_back(*folder_iter, FT_RECURSIVE);
        }
        else
        {
            // push descendant back to verify they are fetched fully (ex: didn't encounter depth limit)
            LLInventoryModel::cat_array_t* categories(NULL);
            LLInventoryModel::item_array_t* items(NULL);
            gInventory.getDirectDescendentsOf(*folder_iter, categories, items);
            if (categories)
            {
                for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
                     it != categories->end();
                     ++it)
                {
                    mFetchFolderQueue.emplace_back((*it)->getUUID(), FT_RECURSIVE);
                }
            }
        }

        folder_iter++;
    }

    if (!mFetchFolderQueue.empty())
    {
        mBackgroundFetchActive = true;
        mFolderFetchActive = true;
        gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
    }
}
void LLInventoryModelBackgroundFetch::onAISFolderCalback(const LLUUID& request_id, const LLUUID& owner_id, const LLUUID& response_id, EFetchType fetch_type)
{
    // Don't emplace_front on failure - there is a chance it was fired from inside bulkFetchViaAis
    incrFetchFolderCount(-1);
    std::list<LLUUID>::const_iterator found = std::find(mExpectedFolderIds.begin(), mExpectedFolderIds.end(), request_id);
    if (found != mExpectedFolderIds.end())
    {
        mExpectedFolderIds.erase(found);
    }
    else
    {
        // ais shouldn't respond twice
        llassert(false);
        LL_WARNS() << "Unexpected folder response for " << request_id << LL_ENDL;
    }

    if (request_id.isNull())
    {
        // orphans, no other actions needed
        return;
    }

    LLViewerInventoryCategory::EFetchType new_state = LLViewerInventoryCategory::FETCH_NONE;
    bool request_descendants = false;
    if (response_id.isNull()) // Failure
    {
        LL_DEBUGS(LOG_INV , "AIS3") << "Failure response for folder " << request_id << LL_ENDL;
        if (fetch_type == FT_RECURSIVE)
        {
            // A full recursive request failed.
            // Try requesting folder and nested content separately
            mFetchFolderQueue.emplace_back(request_id, FT_FOLDER_AND_CONTENT);
        }
        else if (fetch_type == FT_FOLDER_AND_CONTENT)
        {
            LL_WARNS() << "Failed to download folder: " << request_id << " Requesting known content separately" << LL_ENDL;
            mFetchFolderQueue.emplace_back(request_id, FT_CONTENT_RECURSIVE);

            // set folder's version to prevent viewer from trying to request folder indefinetely
            LLViewerInventoryCategory* cat(gInventory.getCategory(request_id));
            if (cat && cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
            {
                cat->setVersion(0);
            }
            // back off for a bit in case something tries to force-request immediately
            new_state = LLViewerInventoryCategory::FETCH_FAILED;
        }
    }
    else
    {
        if (fetch_type == FT_RECURSIVE)
        {
            // Got the folder and content, now verify content
            // Request content even for FT_RECURSIVE in case of changes, failures
            // or if depth limit gets implemented.
            // This shouldn't redownload folders if they already have version
            request_descendants = true;
            LL_DEBUGS(LOG_INV, "AIS3") << "Got folder " << request_id << ". Requesting content" << LL_ENDL;
        }
        else if (fetch_type == FT_FOLDER_AND_CONTENT)
        {
            // read folder for content request
            mFetchFolderQueue.emplace_front(request_id, FT_CONTENT_RECURSIVE);
        }
        else
        {
            LL_DEBUGS(LOG_INV, "AIS3") << "Got folder " << request_id << "." << LL_ENDL;
        }

        invalidateDeferredCache(owner_id, request_id);
    }

    if (request_descendants)
    {
        LLInventoryModel::cat_array_t* categories(NULL);
        LLInventoryModel::item_array_t* items(NULL);
        gInventory.getDirectDescendentsOf(request_id, categories, items);
        if (categories)
        {
            for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
                 it != categories->end();
                 ++it)
            {
                mFetchFolderQueue.emplace_back((*it)->getUUID(), FT_RECURSIVE);
            }
        }
    }

    if (!mFetchFolderQueue.empty())
    {
        mBackgroundFetchActive = true;
        mFolderFetchActive = true;
        gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
    }

    // done
    LLViewerInventoryCategory* cat(gInventory.getCategory(request_id));
    if (cat)
    {
        cat->setFetching(new_state);
    }
}

static LLTrace::BlockTimerStatHandle FTM_BULK_FETCH("Bulk Fetch");

void LLInventoryModelBackgroundFetch::bulkFetchViaAis()
{
    LL_RECORD_BLOCK_TIME(FTM_BULK_FETCH);
    //Background fetch is called from gIdleCallbacks in a loop until background fetch is stopped.
    if (gDisconnected)
    {
        return;
    }

    static LLCachedControl<U32> ais_pool(gSavedSettings, "PoolSizeAIS", 20);
    // Don't have too many requests at once, AIS throttles
    // Reserve one request for actions outside of fetch (like renames)
    const U32 max_concurrent_fetches = llclamp(ais_pool - 1, 1, 50);

    if ((U32)mFetchCount >= max_concurrent_fetches)
    {
        return;
    }

    // Don't loop for too long (in case of large, fully loaded inventory)
    const F64 max_time = LLStartUp::getStartupState() > STATE_WEARABLES_WAIT
        ? 0.006f // 6 ms
        : 1.f;
    const F64 end_time = gIdleCallbacks.getStartTime() + max_time;
    S32 last_fetch_count = mFetchCount;

    // Network can handle a limited number of requests at once,
    // cache can handle more, but can be slow to process, so
    // limit it separately, without using network's budget.
    // Limit was choosen from logic:
    // 30 frames per second, 30K folders should load under 10 seconds.
    // But is somewhat arbitrary, and can be adjusted as needed.
    constexpr S32 FECTH_CACHE_TRESHOLD = 100;
    S32 loaded_from_cache = 0;

    while (!mFetchFolderQueue.empty()
        && (U32)mFetchCount < max_concurrent_fetches
        && loaded_from_cache < FECTH_CACHE_TRESHOLD
        && LLTimer::getTotalSeconds() < end_time)
    {
        const FetchQueueInfo& fetch_info(mFetchFolderQueue.front());
        bulkFetchViaAis(fetch_info, loaded_from_cache);
        mFetchFolderQueue.pop_front();
    }

    // Ideally we shouldn't fetch items if recursive fetch isn't done,
    // but there is a chance some request will start timeouting and recursive
    // fetch will get stuck on a single folder, don't block item fetch in such case
    while (!mFetchItemQueue.empty()
        && (U32)mFetchCount < max_concurrent_fetches
        && LLTimer::getTotalSeconds() < end_time)
    {
        const FetchQueueInfo& fetch_info(mFetchItemQueue.front());
        bulkFetchViaAis(fetch_info, loaded_from_cache);
        mFetchItemQueue.pop_front();
    }

    if (loaded_from_cache > 0)
    {
        // Some folders were loaded from cache. Notify in bulk.
        gInventory.notifyObservers();
    }

    if (last_fetch_count != mFetchCount // if anything was added
        || mLastFetchCount != mFetchCount) // if anything was substracted
    {
        LL_DEBUGS(LOG_INV , "AIS3") << "Total active fetches: " << mLastFetchCount << "->" << last_fetch_count << "->" << mFetchCount
            << ", scheduled folder fetches: " << (S32)mFetchFolderQueue.size()
            << ", scheduled item fetches: " << (S32)mFetchItemQueue.size()
            << LL_ENDL;
        mLastFetchCount = mFetchCount;

        if (!mExpectedFolderIds.empty())
        {
            // A folder seem to be stack fetching on QA account, print oldest folder out
            LL_DEBUGS(LOG_INV , "AIS3") << "Oldest expected folder: ";
            std::list<LLUUID>::const_iterator iter = mExpectedFolderIds.begin();
            LL_CONT << *iter;
            if ((*iter).notNull())
            {
                LLViewerInventoryCategory* cat(gInventory.getCategory(*iter));
                if (cat)
                {
                    LL_CONT << " Folder name: " << cat->getName() << " Parent: " << cat->getParentUUID();
                }
                else
                {
                    LL_CONT << " This folder doesn't exist";
                }
            }
            else
            {
                LL_CONT << " Orphans request";
            }
            LL_CONT << LL_ENDL;
        }
    }

    if (isFolderFetchProcessingComplete() && mFolderFetchActive)
    {
        if (!mRecursiveInventoryFetchStarted || mRecursiveMarketplaceFetchStarted)
        {
            setAllFoldersFetched();
        }
        else
        {
            // Intent is for marketplace request to happen after
            // main inventory is done, unless requested by floater
            mRecursiveMarketplaceFetchStarted = true;
            const LLUUID& marketplacelistings_id = gInventory.getMarketplaceListingsUUID();
            if (marketplacelistings_id.notNull())
            {
                mFetchFolderQueue.emplace_front(marketplacelistings_id, FT_FOLDER_AND_CONTENT);
            }
            else
            {
                setAllFoldersFetched();
            }
        }

    }

    if (isBulkFetchProcessingComplete())
    {
        if (mAllRecursiveFoldersFetched && !mInitialFetchDurationCaptured)
        {
            mInitialFetchDuration = mFetchStartTimer.getElapsedTimeF32();
            mInitialFetchDurationCaptured = true;
        }
        mBackgroundFetchActive = false;
    }
}

void LLInventoryModelBackgroundFetch::bulkFetchViaAis(const FetchQueueInfo& fetch_info, S32& loaded_from_cache_count)
{
    if (fetch_info.mIsCategory)
    {
        const LLUUID& cat_id(fetch_info.mUUID);
        if (cat_id.isNull())
        {
            incrFetchFolderCount(1);
            mExpectedFolderIds.emplace_back(cat_id);
            // Lost and found
            // Should it actually be recursive?
            AISAPI::FetchOrphans([](const LLUUID& response_id)
                                 {
                                     LLInventoryModelBackgroundFetch::instance().onAISFolderCalback(LLUUID::null,
                                         gAgent.getID(),
                                         response_id,
                                         FT_DEFAULT);
                                 });
        }
        else
        {
            LLViewerInventoryCategory* cat(gInventory.getCategory(cat_id));
            if (cat)
            {
                if (fetch_info.mFetchType == FT_CONTENT_RECURSIVE)
                {
                    // fetch content only, ignore cat itself
                    uuid_vec_t children;
                    LLInventoryModel::cat_array_t* categories(NULL);
                    LLInventoryModel::item_array_t* items(NULL);
                    gInventory.getDirectDescendentsOf(cat_id, categories, items);

                    LLViewerInventoryCategory::EFetchType target_state = LLViewerInventoryCategory::FETCH_RECURSIVE;
                    bool content_done = true;
                    bool notify_observers = false;

                    // Cache invokes a price on main thread, don't process too many at once.
                    // Todo: probably should be a time limit instead.
                    S32 cache_limit = 100;
                    // Top limit is 'as many as you can put into url'
                    static LLCachedControl<S32> ais_batch(gSavedSettings, "BatchSizeAIS3", 20);
                    S32 batch_limit = llclamp(ais_batch(), 1, 40);

                    if (categories)
                    {
                        for (LLInventoryModel::cat_array_t::iterator it = categories->begin();
                             it != categories->end();
                             ++it)
                        {
                            LLViewerInventoryCategory* child_cat = (*it);
                            if (LLViewerInventoryCategory::VERSION_UNKNOWN != child_cat->getVersion()
                                || child_cat->getFetching() >= target_state)
                            {
                                continue;
                            }

                            if (child_cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_LISTINGS)
                            {
                                // special case, marketplace will fetch that as needed
                                continue;
                            }

                            LLUUID child_id = child_cat->getUUID();
                            // applyDeferredCache replaces folder in place, so this shouldn't
                            // invalidate the iterator.
                            // Postpone notifying observers to avoid mutating categories array
                            // and to trigger callbacks in bulk.
                            // Observers will be notified after bulkFetchViaAis()'s loop.
                            if (applyDeferredCache(child_id))
                            {
                                // Note that applyDeferredCache does not count for batch limit.
                                // network is the limiting factor, not cache.
                                // But this does mean that if we have a lot of folders in cache,
                                // this place can stall main thread, so instead use independent
                                // cache_limit.

                                // Schedule children verification on the next pass.
                                mFetchFolderQueue.emplace_back(child_id, FT_RECURSIVE);
                                loaded_from_cache_count++;

                                if (cache_limit-- < 0)
                                {
                                    content_done = false;
                                    break;
                                }
                                // Resolved from cache - no network request needed
                                continue;
                            }

                            children.emplace_back(child_cat->getUUID());
                            mExpectedFolderIds.emplace_back(child_cat->getUUID());
                            child_cat->setFetching(target_state);

                            if (children.size() >= batch_limit)
                            {
                                content_done = false;
                                break;
                            }
                        }
                    }

                    if (!children.empty())
                    {
                        // increment before call in case of immediate callback
                        incrFetchFolderCount(1);

                        AISAPI::ITEM_TYPE item_type = AISAPI::INVENTORY;
                        LLUUID owner_id = cat->getOwnerID();
                        if (ALEXANDRIA_LINDEN_ID == owner_id)
                        {
                            item_type = AISAPI::LIBRARY;
                        }
                        else
                        {
                            llassert(owner_id == gAgent.getID());
                        }

                        EFetchType type = fetch_info.mFetchType;
                        LLUUID cat_id = cat->getUUID(); // need a copy for lambda
                        AISAPI::completion_t cb = [cat_id, children, owner_id, type](const LLUUID& response_id)
                        {
                            LLInventoryModelBackgroundFetch::instance().onAISContentCalback(cat_id, children, owner_id, response_id, type);
                        };

                        AISAPI::FetchCategorySubset(cat_id, children, item_type, true, cb, 0);
                    }

                    if (content_done)
                    {
                        // This will have a bit of overlap with onAISContentCalback,
                        // but something else might have downloaded folders, so verify
                        // every child that is complete has it's children done as well
                        if (categories)
                        {
                            for (LLInventoryModel::cat_array_t::iterator it = categories->begin();
                                 it != categories->end();
                                 ++it)
                            {
                                LLViewerInventoryCategory* child_cat = (*it);
                                if (LLViewerInventoryCategory::VERSION_UNKNOWN != child_cat->getVersion())
                                {
                                    mFetchFolderQueue.emplace_back(child_cat->getUUID(), FT_RECURSIVE);
                                }
                            }
                        }
                    }
                    else
                    {
                        // send it back to get the rest
                        mFetchFolderQueue.emplace_back(cat_id, FT_CONTENT_RECURSIVE);
                    }
                }
                else if (LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion()
                         || fetch_info.mFetchType == FT_FORCED)
                {
                    bool resolved_from_cache = fetch_info.mFetchType != FT_FORCED && applyDeferredCache(cat_id);
                    if (!resolved_from_cache)
                    {
                        LLViewerInventoryCategory::EFetchType target_state =
                            fetch_info.mFetchType > FT_CONTENT_RECURSIVE
                            ? LLViewerInventoryCategory::FETCH_RECURSIVE
                            : LLViewerInventoryCategory::FETCH_NORMAL;
                        // start again if we did a non-recursive fetch before
                        // to get all children in a single request
                        if (cat->getFetching() < target_state)
                        {
                            // increment before call in case of immediate callback
                            incrFetchFolderCount(1);
                            cat->setFetching(target_state);
                            mExpectedFolderIds.emplace_back(cat_id);

                            LLUUID owner_id  = cat->getOwnerID();
                            AISAPI::ITEM_TYPE item_type = AISAPI::INVENTORY;
                            if (ALEXANDRIA_LINDEN_ID == owner_id)
                            {
                                item_type = AISAPI::LIBRARY;
                            }
                            else
                            {
                                // Assuming that there are only two owners. Agent and library.
                                llassert(owner_id == gAgent.getID());
                            }

                            EFetchType type = fetch_info.mFetchType;
                            LLUUID cat_cb_id = cat_id;
                            AISAPI::completion_t cb = [cat_cb_id, owner_id, type](const LLUUID& response_id)
                            {
                                LLInventoryModelBackgroundFetch::instance().onAISFolderCalback(cat_cb_id, owner_id, response_id, type);
                            };

                            AISAPI::FetchCategoryChildren(cat_id, item_type, type == FT_RECURSIVE, cb, 0);
                        }
                    }
                    else
                    {
                        loaded_from_cache_count++;
                        if (fetch_info.mFetchType == FT_RECURSIVE
                            || fetch_info.mFetchType == FT_FOLDER_AND_CONTENT)
                        {
                            // Resolved from cache - cat now has a confirmed version.
                            // Treat as if it is fetched and verify folder's children.
                            LLInventoryModel::cat_array_t* child_categories(NULL);
                            LLInventoryModel::item_array_t* child_items(NULL);
                            gInventory.getDirectDescendentsOf(cat_id, child_categories, child_items);
                            if (child_categories)
                            {
                                for (LLInventoryModel::cat_array_t::const_iterator it = child_categories->begin();
                                    it != child_categories->end();
                                    ++it)
                                {
                                    mFetchFolderQueue.emplace_back((*it)->getUUID(), FT_RECURSIVE);
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Already fetched, check if anything inside needs fetching
                    if (fetch_info.mFetchType == FT_RECURSIVE
                        || fetch_info.mFetchType == FT_FOLDER_AND_CONTENT)
                    {
                        LLInventoryModel::cat_array_t* categories(NULL);
                        LLInventoryModel::item_array_t* items(NULL);
                        gInventory.getDirectDescendentsOf(cat_id, categories, items);
                        if (categories)
                        {
                            for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
                                 it != categories->end();
                                 ++it)
                            {
                                // not emplace_front to not cause an infinite loop
                                mFetchFolderQueue.emplace_back((*it)->getUUID(), FT_RECURSIVE);
                            }
                        }
                    }
                }
            } // else try to fetch folder either way?
        }
    }
    else
    {
        LLViewerInventoryItem* itemp(gInventory.getItem(fetch_info.mUUID));

        if (itemp)
        {
            if (!itemp->isFinished() || fetch_info.mFetchType == FT_FORCED)
            {
                mFetchCount++;
                if (itemp->getPermissions().getOwner() == gAgent.getID())
                {
                    AISAPI::FetchItem(fetch_info.mUUID, AISAPI::INVENTORY, ais_simple_item_callback);
                }
                else
                {
                    AISAPI::FetchItem(fetch_info.mUUID, AISAPI::LIBRARY, ais_simple_item_callback);
                }
            }
        }
        else // We don't know it, assume incomplete
        {
            // Assume agent's inventory, library wouldn't have gotten here
            mFetchCount++;
            AISAPI::FetchItem(fetch_info.mUUID, AISAPI::INVENTORY, ais_simple_item_callback);
        }
    }

    if (fetch_info.mFetchType == FT_FORCED)
    {
        mForceFetchSet.erase(fetch_info.mUUID);
    }
}

// Bundle up a bunch of requests to send all at once.
void LLInventoryModelBackgroundFetch::bulkFetch()
{
    LL_RECORD_BLOCK_TIME(FTM_BULK_FETCH);
    //Background fetch is called from gIdleCallbacks in a loop until background fetch is stopped.
    //If there are items in mFetchQueue, we want to check the time since the last bulkFetch was
    //sent.  If it exceeds our retry time, go ahead and fire off another batch.
    LLViewerRegion* region(gAgent.getRegion());
    if (! region || gDisconnected || LLApp::isExiting())
    {
        return;
    }

    // *TODO:  These values could be tweaked at runtime to effect
    // a fast/slow fetch throttle.  Once login is complete and the scene
    // is mostly loaded, we could turn up the throttle and fill missing
    // inventory more quickly.
    static const U32 max_batch_size(10);
    static const S32 max_concurrent_fetches(12);        // Outstanding requests, not connections

    if (mFetchCount)
    {
        // Process completed background HTTP requests
        gInventory.handleResponses(false);
        // Just processed a bunch of items.
        // Note: do we really need notifyObservers() here?
        // OnIdle it will be called anyway due to Add flag for processed item.
        // It seems like in some cases we are updating on fail (no flag),
        // but is there anything to update?
        gInventory.notifyObservers();
    }

    if (mFetchCount > max_concurrent_fetches)
    {
        return;
    }

    U32 item_count(0);
    U32 folder_count(0);

    constexpr S32 FECTH_CACHE_TRESHOLD = 100;
    S32 folders_from_cache(0);

    const U32 sort_order(gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER) & 0x1);

    // *TODO:  Think I'd like to get a shared pointer to this and share it
    // among all the folder requests.
    uuid_vec_t recursive_cats;
    uuid_vec_t all_cats; // duplicate avoidance

    LLSD folder_request_body;
    LLSD folder_request_body_lib;
    LLSD item_request_body;
    LLSD item_request_body_lib;

    while (! mFetchFolderQueue.empty()
            && (item_count + folder_count) < max_batch_size
            && folders_from_cache < FECTH_CACHE_TRESHOLD)
    {
        const FetchQueueInfo& fetch_info(mFetchFolderQueue.front());
        if (fetch_info.mIsCategory)
        {
            const LLUUID& cat_id(fetch_info.mUUID);
            if (cat_id.isNull()) //DEV-17797 Lost and found
            {
                LLSD folder_sd;
                folder_sd["folder_id"]      = LLUUID::null.asString();
                folder_sd["owner_id"]       = gAgent.getID();
                folder_sd["sort_order"]     = LLSD::Integer(sort_order);
                folder_sd["fetch_folders"]  = LLSD::Boolean(false);
                folder_sd["fetch_items"]    = LLSD::Boolean(true);
                folder_request_body["folders"].append(folder_sd);
                folder_count++;
            }
            else
            {
                const LLViewerInventoryCategory* cat(gInventory.getCategory(cat_id));
                if (cat)
                {
                    if (LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion())
                    {
                        if (applyDeferredCache(cat_id))
                        {
                            folders_from_cache++;
                            // Resolved from cache, but may need children.
                            if (fetch_info.mFetchType >= FT_CONTENT_RECURSIVE)
                            {
                                LLInventoryModel::cat_array_t* categories(NULL);
                                LLInventoryModel::item_array_t* items(NULL);
                                gInventory.getDirectDescendentsOf(cat_id, categories, items);
                                if (categories)
                                {
                                    for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
                                        it != categories->end();
                                        ++it)
                                    {
                                        mFetchFolderQueue.emplace_back((*it)->getUUID(), fetch_info.mFetchType);
                                    }
                                }
                            }
                        }
                        else if (std::find(all_cats.begin(), all_cats.end(), cat_id) == all_cats.end())
                        {
                            LLSD folder_sd;
                            folder_sd["folder_id"] = cat->getUUID();
                            folder_sd["owner_id"] = cat->getOwnerID();
                            folder_sd["sort_order"] = LLSD::Integer(sort_order);
                            folder_sd["fetch_folders"] = LLSD::Boolean(true); //(LLSD::Boolean)sFullFetchStarted;
                            folder_sd["fetch_items"] = LLSD::Boolean(true);

                            if (ALEXANDRIA_LINDEN_ID == cat->getOwnerID())
                            {
                                folder_request_body_lib["folders"].append(folder_sd);
                            }
                            else
                            {
                                folder_request_body["folders"].append(folder_sd);
                            }
                            folder_count++;
                        }
                    }
                    else
                    {
                        // May already have this folder, but append child folders to list.
                        if (fetch_info.mFetchType >= FT_CONTENT_RECURSIVE)
                        {
                            LLInventoryModel::cat_array_t* categories(NULL);
                            LLInventoryModel::item_array_t* items(NULL);
                            gInventory.getDirectDescendentsOf(cat_id, categories, items);
                            for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
                                it != categories->end();
                                ++it)
                            {
                                mFetchFolderQueue.emplace_back((*it)->getUUID(), fetch_info.mFetchType);
                            }
                        }
                    }
                }
            }
            if (fetch_info.mFetchType >= FT_CONTENT_RECURSIVE)
            {
                recursive_cats.emplace_back(cat_id);
            }
            all_cats.emplace_back(cat_id);
        }

        mFetchFolderQueue.pop_front();
    }

    if (folders_from_cache > 0)
    {
        // Some folders were loaded from cache. Notify in bulk.
        gInventory.notifyObservers();
    }


    while (!mFetchItemQueue.empty()
        && (item_count + folder_count) < max_batch_size)
    {
        const FetchQueueInfo& fetch_info(mFetchItemQueue.front());

        LLViewerInventoryItem* itemp(gInventory.getItem(fetch_info.mUUID));

        if (itemp)
        {
            LLSD item_sd;
            item_sd["owner_id"] = itemp->getPermissions().getOwner();
            item_sd["item_id"] = itemp->getUUID();
            if (itemp->getPermissions().getOwner() == gAgent.getID())
            {
                item_request_body.append(item_sd);
            }
            else
            {
                item_request_body_lib.append(item_sd);
            }
            //itemp->fetchFromServer();
            item_count++;
        }

        mFetchItemQueue.pop_front();
    }

    // Issue HTTP POST requests to fetch folders and items

    if (item_count + folder_count > 0)
    {
        if (folder_count)
        {
            if (folder_request_body["folders"].size())
            {
                const std::string url(region->getCapability("FetchInventoryDescendents2"));

                if (! url.empty())
                {
                    LLCore::HttpHandler::ptr_t handler = std::make_shared<BGFolderHttpHandler>(folder_request_body, recursive_cats);
                    gInventory.requestPost(false, url, folder_request_body, handler, "Inventory Folder");
                }
            }

            if (folder_request_body_lib["folders"].size())
            {
                const std::string url(region->getCapability("FetchLibDescendents2"));

                if (! url.empty())
                {
                    LLCore::HttpHandler::ptr_t handler = std::make_shared<BGFolderHttpHandler>(folder_request_body_lib, recursive_cats);
                    gInventory.requestPost(false, url, folder_request_body_lib, handler, "Library Folder");
                }
            }
        } // if (folder_count)

        if (item_count)
        {
            if (item_request_body.size())
            {
                const std::string url(region->getCapability("FetchInventory2"));

                if (! url.empty())
                {
                    LLSD body;
                    body["items"] = item_request_body;
                    LLCore::HttpHandler::ptr_t handler = std::make_shared<BGItemHttpHandler>(body);
                    gInventory.requestPost(false, url, body, handler, "Inventory Item");
                }
            }

            if (item_request_body_lib.size())
            {
                const std::string url(region->getCapability("FetchLib2"));

                if (! url.empty())
                {
                    LLSD body;
                    body["items"] = item_request_body_lib;
                    LLCore::HttpHandler::ptr_t handler = std::make_shared<BGItemHttpHandler>(body);
                    gInventory.requestPost(false, url, body, handler, "Library Item");
                }
            }
        } // if (item_count)

        mFetchTimer.reset();
    }
    else if (isBulkFetchProcessingComplete())
    {
        setAllFoldersFetched();
    }
}

bool LLInventoryModelBackgroundFetch::fetchQueueContainsNoDescendentsOf(const LLUUID& cat_id) const
{
    for (fetch_queue_t::const_iterator it = mFetchFolderQueue.begin();
         it != mFetchFolderQueue.end();
         ++it)
    {
        const LLUUID& fetch_id = (*it).mUUID;
        if (gInventory.isObjectDescendentOf(fetch_id, cat_id))
            return false;
    }
    for (fetch_queue_t::const_iterator it = mFetchItemQueue.begin();
        it != mFetchItemQueue.end();
        ++it)
    {
        const LLUUID& fetch_id = (*it).mUUID;
        if (gInventory.isObjectDescendentOf(fetch_id, cat_id))
            return false;
    }
    return true;
}


namespace
{

///----------------------------------------------------------------------------
/// Class <anonymous>::BGFolderHttpHandler
///----------------------------------------------------------------------------

void BGFolderHttpHandler::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse* response)
{
    do      // Single-pass do-while used for common exit handling
    {
        LLCore::HttpStatus status(response->getStatus());
        // status = LLCore::HttpStatus(404);                // Dev tool to force error handling
        if (! status)
        {
            processFailure(status, response);
            break;          // Goto common exit
        }

        // Response body should be present.
        LLCore::BufferArray* body(response->getBody());
        // body = NULL;                                 // Dev tool to force error handling
        if (! body || ! body->size())
        {
            LL_WARNS(LOG_INV) << "Missing data in inventory folder query." << LL_ENDL;
            processFailure("HTTP response missing expected body", response);
            break;          // Goto common exit
        }

        // Could test 'Content-Type' header but probably unreliable.

        // Convert response to LLSD
        // body->write(0, "Garbage Response", 16);      // Dev tool to force error handling
        LLSD body_llsd;
        if (! LLCoreHttpUtil::responseToLLSD(response, true, body_llsd))
        {
            // INFOS-level logging will occur on the parsed failure
            processFailure("HTTP response contained malformed LLSD", response);
            break;          // goto common exit
        }

        // Expect top-level structure to be a map
        // body_llsd = LLSD::emptyArray();              // Dev tool to force error handling
        if (! body_llsd.isMap())
        {
            processFailure("LLSD response not a map", response);
            break;          // goto common exit
        }

        // Check for 200-with-error failures
        //
        // See comments in llinventorymodel.cpp about this mode of error.
        //
        // body_llsd["error"] = LLSD::emptyMap();       // Dev tool to force error handling
        // body_llsd["error"]["identifier"] = "Development";
        // body_llsd["error"]["message"] = "You left development code in the viewer";
        if (body_llsd.has("error"))
        {
            processFailure("Inventory application error (200-with-error)", response);
            break;          // goto common exit
        }

        // Okay, process data if possible
        processData(body_llsd, response);
    }
    while (false);
}


void BGFolderHttpHandler::processData(LLSD& content, LLCore::HttpResponse* response)
{
    LLInventoryModelBackgroundFetch* fetcher(LLInventoryModelBackgroundFetch::getInstance());

    // API V2 and earlier should probably be testing for "error" map
    // in response as an application-level error.

    // Instead, we assume success and attempt to extract information.
    if (content.has("folders"))
    {
        LLSD folders(content["folders"]);

        for (LLSD::array_const_iterator folder_it = folders.beginArray();
            folder_it != folders.endArray();
            ++folder_it)
        {
            LLSD folder_sd(*folder_it);

            //LLUUID agent_id = folder_sd["agent_id"];

            //if (agent_id != gAgent.getID())    //This should never happen.
            //{
            //  LL_WARNS(LOG_INV) << "Got a UpdateInventoryItem for the wrong agent."
            //          << LL_ENDL;
            //  break;
            //}

            LLUUID parent_id(folder_sd["folder_id"].asUUID());
            LLUUID owner_id(folder_sd["owner_id"].asUUID());
            S32    version(folder_sd["version"].asInteger());
            S32    descendents(folder_sd["descendents"].asInteger());
            LLPointer<LLViewerInventoryCategory> tcategory = new LLViewerInventoryCategory(owner_id);

            if (parent_id.isNull())
            {
                LLSD items(folder_sd["items"]);
                LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;

                for (LLSD::array_const_iterator item_it = items.beginArray();
                    item_it != items.endArray();
                    ++item_it)
                {
                    const LLUUID lost_uuid(gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));

                    if (lost_uuid.notNull())
                    {
                        LLSD item(*item_it);

                        titem->unpackMessage(item);

                        LLInventoryModel::update_list_t update;
                        LLInventoryModel::LLCategoryUpdate new_folder(lost_uuid, 1);
                        update.emplace_back(new_folder);
                        gInventory.accountForUpdate(update);

                        titem->setParent(lost_uuid);
                        titem->updateParentOnServer(false);
                        gInventory.updateItem(titem);
                    }
                }
            }

            LLViewerInventoryCategory* pcat(gInventory.getCategory(parent_id));
            if (! pcat)
            {
                continue;
            }

            LLSD categories(folder_sd["categories"]);
            for (LLSD::array_const_iterator category_it = categories.beginArray();
                category_it != categories.endArray();
                ++category_it)
            {
                LLSD category(*category_it);
                tcategory->fromLLSD(category);

                const bool recursive(getIsRecursive(tcategory->getUUID()));
                if (recursive)
                {
                    fetcher->addRequestAtBack(tcategory->getUUID(), recursive, true);
                }
                else if (! gInventory.isCategoryComplete(tcategory->getUUID()))
                {
                    gInventory.updateCategory(tcategory);
                }
            }

            LLSD items(folder_sd["items"]);
            LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
            for (LLSD::array_const_iterator item_it = items.beginArray();
                 item_it != items.endArray();
                 ++item_it)
            {
                LLSD item(*item_it);
                titem->unpackMessage(item);

                gInventory.updateItem(titem);
            }

            // Set version and descendentcount according to message.
            LLViewerInventoryCategory* cat(gInventory.getCategory(parent_id));
            if (cat)
            {
                cat->setVersion(version);
                cat->setDescendentCount(descendents);
                cat->determineFolderType();
            }
        }
    }

    if (content.has("bad_folders"))
    {
        LLSD bad_folders(content["bad_folders"]);
        for (LLSD::array_const_iterator folder_it = bad_folders.beginArray();
             folder_it != bad_folders.endArray();
             ++folder_it)
        {
            // *TODO: Stop copying data [ed:  this isn't copying data]
            LLSD folder_sd(*folder_it);

            // These folders failed on the dataserver.  We probably don't want to retry them.
            LL_WARNS(LOG_INV) << "Folder " << folder_sd["folder_id"].asString()
                              << "Error: " << folder_sd["error"].asString() << LL_ENDL;
        }
    }

    if (fetcher->isBulkFetchProcessingComplete())
    {
        fetcher->setAllFoldersFetched();
    }
}


void BGFolderHttpHandler::processFailure(LLCore::HttpStatus status, LLCore::HttpResponse* response)
{
    const std::string& ct(response->getContentType());
    LL_WARNS(LOG_INV) << "Inventory folder fetch failure\n"
                      << "[Status: " << status.toTerseString() << "]\n"
                      << "[Reason: " << status.toString() << "]\n"
                      << "[Content-type: " << ct << "]\n"
                      << "[Content (abridged): "
                      << LLCoreHttpUtil::responseToString(response) << "]" << LL_ENDL;

    // Could use a 404 test here to try to detect revoked caps...

    if (status == LLCore::HttpStatus(HTTP_FORBIDDEN))
    {
        // Too large, split into two if possible
        if (gDisconnected || LLApp::isExiting())
        {
            return;
        }

        const std::string url(gAgent.getRegionCapability("FetchInventoryDescendents2"));
        if (url.empty())
        {
            LL_WARNS(LOG_INV) << "Failed to get AIS2 cap" << LL_ENDL;
            return;
        }

        auto size = mRequestSD["folders"].size();

        if (size > 1)
        {
            // Can split, assume that this isn't the library
            LLSD folders;
            uuid_vec_t recursive_cats;
            LLSD::array_iterator iter = mRequestSD["folders"].beginArray();
            LLSD::array_iterator end = mRequestSD["folders"].endArray();
            while (iter != end)
            {
                folders.append(*iter);
                LLUUID folder_id = iter->get("folder_id").asUUID();
                if (std::find(mRecursiveCatUUIDs.begin(), mRecursiveCatUUIDs.end(), folder_id) != mRecursiveCatUUIDs.end())
                {
                    recursive_cats.emplace_back(folder_id);
                }
                if (folders.size() == (size / 2))
                {
                    LLSD request_body;
                    request_body["folders"] = folders;
                    LLCore::HttpHandler::ptr_t handler = std::make_shared<BGFolderHttpHandler>(request_body, recursive_cats);
                    gInventory.requestPost(false, url, request_body, handler, "Inventory Folder");
                    recursive_cats.clear();
                    folders.clear();
                }
                iter++;
            }

            LLSD request_body;
            request_body["folders"] = folders;
            LLCore::HttpHandler::ptr_t handler = std::make_shared<BGFolderHttpHandler>(request_body, recursive_cats);
            gInventory.requestPost(false, url, request_body, handler, "Inventory Folder");
            return;
        }
        else
        {
            // Can't split
            LLNotificationsUtil::add("InventoryLimitReachedAIS");
        }
    }

    // This was originally the request retry logic for the inventory
    // request which tested on HTTP_INTERNAL_ERROR status.  This
    // retry logic was unbounded and lacked discrimination as to the
    // cause of the retry.  The new http library should be doing
    // adequately on retries but I want to keep the structure of a
    // retry for reference.
    LLInventoryModelBackgroundFetch* fetcher = LLInventoryModelBackgroundFetch::getInstance();
    if (false)
    {
        // timed out or curl failure
        for (LLSD::array_const_iterator folder_it = mRequestSD["folders"].beginArray();
             folder_it != mRequestSD["folders"].endArray();
             ++folder_it)
        {
            LLSD folder_sd(*folder_it);
            LLUUID folder_id(folder_sd["folder_id"].asUUID());
            const bool recursive = getIsRecursive(folder_id);
            fetcher->addRequestAtFront(folder_id, recursive, true);
        }
    }
    else
    {
        if (fetcher->isBulkFetchProcessingComplete())
        {
            fetcher->setAllFoldersFetched();
        }
    }
}


void BGFolderHttpHandler::processFailure(const char* const reason, LLCore::HttpResponse* response)
{
    LL_WARNS(LOG_INV) << "Inventory folder fetch failure\n"
                      << "[Status: internal error]\n"
                      << "[Reason: " << reason << "]\n"
                      << "[Content (abridged): "
                      << LLCoreHttpUtil::responseToString(response) << "]" << LL_ENDL;

    // Reverse of previous processFailure() method, this is invoked
    // when response structure is found to be invalid.  Original
    // always re-issued the request (without limit).  This does
    // the same but be aware that this may be a source of problems.
    // Philosophy is that inventory folders are so essential to
    // operation that this is a reasonable action.
    LLInventoryModelBackgroundFetch* fetcher = LLInventoryModelBackgroundFetch::getInstance();
    if (true)
    {
        for (LLSD::array_const_iterator folder_it = mRequestSD["folders"].beginArray();
             folder_it != mRequestSD["folders"].endArray();
             ++folder_it)
        {
            LLSD folder_sd(*folder_it);
            LLUUID folder_id(folder_sd["folder_id"].asUUID());
            const bool recursive = getIsRecursive(folder_id);
            fetcher->addRequestAtFront(folder_id, recursive, true);
        }
    }
    else
    {
        if (fetcher->isBulkFetchProcessingComplete())
        {
            fetcher->setAllFoldersFetched();
        }
    }
}


bool BGFolderHttpHandler::getIsRecursive(const LLUUID& cat_id) const
{
    return std::find(mRecursiveCatUUIDs.begin(), mRecursiveCatUUIDs.end(), cat_id) != mRecursiveCatUUIDs.end();
}

///----------------------------------------------------------------------------
/// Class <anonymous>::BGItemHttpHandler
///----------------------------------------------------------------------------

// Nothing to implement here.  All ctor/dtor changes.

} // end namespace anonymous
