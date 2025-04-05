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

protected:
    BGItemHttpHandler(const BGItemHttpHandler&);               // Not defined
    void operator=(const BGItemHttpHandler&);                  // Not defined
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
    BGFolderHttpHandler(const BGFolderHttpHandler&);           // Not defined
    void operator=(const BGFolderHttpHandler&);                // Not defined

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
    mAllRecursiveFoldersFetched(false),
    mRecursiveInventoryFetchStarted(false),
    mRecursiveLibraryFetchStarted(false),
    mRecursiveMarketplaceFetchStarted(false),
    mMinTimeBetweenFetches(0.3f)
{}

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

void LLInventoryModelBackgroundFetch::start(const LLUUID& id, bool recursive)
{
    LLViewerInventoryCategory* cat(gInventory.getCategory(id));

    if (cat || (id.isNull() && ! isEverythingFetched()))
    {
        // it's a folder, do a bulk fetch
        LL_DEBUGS(LOG_INV) << "Start fetching category: " << id << ", recursive: " << recursive << LL_ENDL;

        mBackgroundFetchActive = true;
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
        mBackgroundFetchActive = true;
        mFolderFetchActive = true;

        if (forced)
        {
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
    if (mFetchItemQueue.empty() || mFetchItemQueue.front().mUUID != item_id)
    {
        mBackgroundFetchActive = true;
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
                                   LLInventoryModelBackgroundFetch::getInstance()->onAISFolderCalback(cat_id, id, FT_DEFAULT);
                               });

    // start idle loop to track completion
    mBackgroundFetchActive = true;
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
                         LLInventoryModelBackgroundFetch::getInstance()->onAISFolderCalback(cat_id, id, FT_DEFAULT);

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
    mBackgroundFetchActive = true;
    mFolderFetchActive = true;
    gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, nullptr);
}

void LLInventoryModelBackgroundFetch::findLostItems()
{
    mBackgroundFetchActive = true;
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
    }

    mFolderFetchActive = false;
    if (isBulkFetchProcessingComplete())
    {
        mBackgroundFetchActive = false;
    }

    // For now only informs about initial fetch being done
    mFoldersFetchedSignal();

    LL_INFOS(LOG_INV) << "Inventory background fetch completed" << LL_ENDL;
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
    const LLUUID& response_id,
    EFetchType fetch_type)
{
    // Don't emplace_front on failure - there is a chance it was fired from inside bulkFetchViaAis
    incrFetchFolderCount(-1);

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
void LLInventoryModelBackgroundFetch::onAISFolderCalback(const LLUUID& request_id, const LLUUID& response_id, EFetchType fetch_type)
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
    F64 curent_time = LLTimer::getTotalSeconds();
    const F64 max_time = LLStartUp::getStartupState() > STATE_WEARABLES_WAIT
        ? 0.006f // 6 ms
        : 1.f;
    const F64 end_time = curent_time + max_time;
    S32 last_fetch_count = mFetchCount;

    while (!mFetchFolderQueue.empty() && (U32)mFetchCount < max_concurrent_fetches && curent_time < end_time)
    {
        const FetchQueueInfo& fetch_info(mFetchFolderQueue.front());
        bulkFetchViaAis(fetch_info);
        mFetchFolderQueue.pop_front();
        curent_time = LLTimer::getTotalSeconds();
    }

    // Ideally we shouldn't fetch items if recursive fetch isn't done,
    // but there is a chance some request will start timeouting and recursive
    // fetch will get stuck on a single folder, don't block item fetch in such case
    while (!mFetchItemQueue.empty() && (U32)mFetchCount < max_concurrent_fetches && curent_time < end_time)
    {
        const FetchQueueInfo& fetch_info(mFetchItemQueue.front());
        bulkFetchViaAis(fetch_info);
        mFetchItemQueue.pop_front();
        curent_time = LLTimer::getTotalSeconds();
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
            const LLUUID& marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS);
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
        mBackgroundFetchActive = false;
    }
}

void LLInventoryModelBackgroundFetch::bulkFetchViaAis(const FetchQueueInfo& fetch_info)
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

                    // Top limit is 'as many as you can put into url'
                    static LLCachedControl<S32> ais_batch(gSavedSettings, "BatchSizeAIS3", 20);
                    S32 batch_limit = llclamp(ais_batch(), 1, 40);

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

                        children.emplace_back(child_cat->getUUID());
                        mExpectedFolderIds.emplace_back(child_cat->getUUID());
                        child_cat->setFetching(target_state);

                        if (children.size() >= batch_limit)
                        {
                            content_done = false;
                            break;
                        }
                    }

                    if (!children.empty())
                    {
                        // increment before call in case of immediate callback
                        incrFetchFolderCount(1);

                        EFetchType type = fetch_info.mFetchType;
                        LLUUID cat_id = cat->getUUID(); // need a copy for lambda
                        AISAPI::completion_t cb = [cat_id, children, type](const LLUUID& response_id)
                        {
                            LLInventoryModelBackgroundFetch::instance().onAISContentCalback(cat_id, children, response_id, type);
                        };

                        AISAPI::ITEM_TYPE item_type = AISAPI::INVENTORY;
                        if (ALEXANDRIA_LINDEN_ID == cat->getOwnerID())
                        {
                            item_type = AISAPI::LIBRARY;
                        }

                        AISAPI::FetchCategorySubset(cat_id, children, item_type, true, cb, 0);
                    }

                    if (content_done)
                    {
                        // This will have a bit of overlap with onAISContentCalback,
                        // but something else might have downloaded folders, so verify
                        // every child that is complete has it's children done as well
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
                    else
                    {
                        // send it back to get the rest
                        mFetchFolderQueue.emplace_back(cat_id, FT_CONTENT_RECURSIVE);
                    }
                }
                else if (LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion()
                         || fetch_info.mFetchType == FT_FORCED)
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

                        EFetchType type = fetch_info.mFetchType;
                        LLUUID cat_cb_id = cat_id;
                        AISAPI::completion_t cb = [cat_cb_id, type](const LLUUID& response_id)
                        {
                            LLInventoryModelBackgroundFetch::instance().onAISFolderCalback(cat_cb_id, response_id , type);
                        };

                        AISAPI::ITEM_TYPE item_type = AISAPI::INVENTORY;
                        if (ALEXANDRIA_LINDEN_ID == cat->getOwnerID())
                        {
                            item_type = AISAPI::LIBRARY;
                        }

                        AISAPI::FetchCategoryChildren(cat_id , item_type , type == FT_RECURSIVE , cb, 0);
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
                        for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
                            it != categories->end();
                            ++it)
                        {
                            // not emplace_front to not cause an infinite loop
                            mFetchFolderQueue.emplace_back((*it)->getUUID(), FT_RECURSIVE);
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
            && (item_count + folder_count) < max_batch_size)
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
                        if (std::find(all_cats.begin(), all_cats.end(), cat_id) == all_cats.end())
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
                    LLCore::HttpHandler::ptr_t  handler(new BGFolderHttpHandler(folder_request_body, recursive_cats));
                    gInventory.requestPost(false, url, folder_request_body, handler, "Inventory Folder");
                }
            }

            if (folder_request_body_lib["folders"].size())
            {
                const std::string url(region->getCapability("FetchLibDescendents2"));

                if (! url.empty())
                {
                    LLCore::HttpHandler::ptr_t  handler(new BGFolderHttpHandler(folder_request_body_lib, recursive_cats));
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
                    LLCore::HttpHandler::ptr_t  handler(new BGItemHttpHandler(body));
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
                    LLCore::HttpHandler::ptr_t handler(new BGItemHttpHandler(body));
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
                    LLCore::HttpHandler::ptr_t  handler(new BGFolderHttpHandler(request_body, recursive_cats));
                    gInventory.requestPost(false, url, request_body, handler, "Inventory Folder");
                    recursive_cats.clear();
                    folders.clear();
                }
                iter++;
            }

            LLSD request_body;
            request_body["folders"] = folders;
            LLCore::HttpHandler::ptr_t  handler(new BGFolderHttpHandler(request_body, recursive_cats));
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
