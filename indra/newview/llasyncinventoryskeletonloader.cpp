/**
 * @file llasyncinventoryskeletonloader.cpp
 * @brief Async inventory skeleton loading helper implementation.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#include "llasyncinventoryskeletonloader.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llcallbacklist.h"
#include "llfoldertype.h"
#include "llinventorymodel.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"

#include <algorithm>

#include <boost/bind.hpp>

LLAsyncInventorySkeletonLoader gAsyncInventorySkeletonLoader;
bool gAsyncAgentCacheHydrated = false;
bool gAsyncLibraryCacheHydrated = false;
bool gAsyncParentChildMapPrimed = false;

void LLAsyncInventorySkeletonLoader::reset()
{
    disconnectCapsCallback();
    removeIdleCallback();
    mPhase = Phase::Idle;
    mForceAsync = false;
    mEssentialReady = false;
    mFetchQueue.clear();
    mActiveFetches.clear();
    mQueuedCategories.clear();
    mFetchedCategories.clear();
    mEssentialPending.clear();
    mFailureReason.clear();
    mCapsTimer.stop();
    mFetchTimer.stop();
    mTotalTimer.stop();
    mEssentialTimer.stop();

    const U32 requested = gSavedSettings.getU32("AsyncInventoryMaxConcurrentFetches");
    mMaxConcurrentFetches = std::clamp(requested, 1U, 8U);

    mCapsTimeoutSec = gSavedSettings.getF32("AsyncInventoryCapsTimeout");
    mFetchTimeoutSec = gSavedSettings.getF32("AsyncInventoryFetchTimeout");
    mEssentialTimeoutSec = gSavedSettings.getF32("AsyncInventoryEssentialTimeout");

    mSawCurrentOutfitFolder = false;
}

bool LLAsyncInventorySkeletonLoader::isRunning() const
{
    return mPhase == Phase::WaitingForCaps || mPhase == Phase::Fetching;
}

void LLAsyncInventorySkeletonLoader::start(bool force_async)
{
    reset();
    mForceAsync = force_async;
    mPhase = Phase::WaitingForCaps;
    mTotalTimer.start();
    mCapsTimer.start();

    ensureCapsCallback();
    ensureIdleCallback();

    LL_DEBUGS("AppInit") << "Async skeleton loader concurrency limit set to "
                         << mMaxConcurrentFetches << LL_ENDL;

    if (AISAPI::isAvailable())
    {
        LL_DEBUGS("AppInit") << "Async skeleton loader detected AIS available at start; beginning fetch." << LL_ENDL;
        startFetches();
    }
    else
    {
        LL_DEBUGS("AppInit") << "Async skeleton loader awaiting AIS availability." << LL_ENDL;
    }
}

void LLAsyncInventorySkeletonLoader::ensureCapsCallback()
{
    disconnectCapsCallback();

    LLViewerRegion* regionp = gAgent.getRegion();
    if (regionp)
    {
        mCapsConnection = regionp->setCapabilitiesReceivedCallback(
            boost::bind(&LLAsyncInventorySkeletonLoader::onCapsReceived, this, _1, _2));
        LL_DEBUGS("AppInit") << "Async skeleton loader registered caps callback for region "
                             << regionp->getRegionID() << LL_ENDL;
    }
}

void LLAsyncInventorySkeletonLoader::disconnectCapsCallback()
{
    if (mCapsConnection.connected())
    {
        mCapsConnection.disconnect();
        LL_DEBUGS("AppInit") << "Async skeleton loader disconnected caps callback." << LL_ENDL;
    }
}

void LLAsyncInventorySkeletonLoader::ensureIdleCallback()
{
    if (!mIdleRegistered)
    {
        gIdleCallbacks.addFunction(&LLAsyncInventorySkeletonLoader::idleCallback, this);
        mIdleRegistered = true;
    }
}

void LLAsyncInventorySkeletonLoader::removeIdleCallback()
{
    if (mIdleRegistered)
    {
        gIdleCallbacks.deleteFunction(&LLAsyncInventorySkeletonLoader::idleCallback, this);
        mIdleRegistered = false;
    }
}

void LLAsyncInventorySkeletonLoader::idleCallback(void* userdata)
{
    if (userdata)
    {
        static_cast<LLAsyncInventorySkeletonLoader*>(userdata)->update();
    }
}

void LLAsyncInventorySkeletonLoader::onCapsReceived(const LLUUID&, LLViewerRegion* regionp)
{
    if (regionp && AISAPI::isAvailable())
    {
        LL_DEBUGS("AppInit") << "Async skeleton loader received capabilities for region "
                             << regionp->getRegionID() << ", starting fetch." << LL_ENDL;
        startFetches();
    }
}

void LLAsyncInventorySkeletonLoader::startFetches()
{
    if (mPhase == Phase::Complete || mPhase == Phase::Failed)
    {
        LL_DEBUGS("AppInit") << "Async skeleton loader received startFetches after terminal state; ignoring." << LL_ENDL;
        return;
    }

    if (!AISAPI::isAvailable())
    {
        LL_DEBUGS("AppInit") << "Async skeleton loader startFetches called but AIS still unavailable." << LL_ENDL;
        return;
    }

    if (mPhase == Phase::WaitingForCaps)
    {
        const LLUUID agent_root = gInventory.getRootFolderID();
        const LLUUID library_root = gInventory.getLibraryRootFolderID();

        LL_INFOS("AppInit") << "Async inventory skeleton loader primed. force_async="
                             << (mForceAsync ? "true" : "false")
                             << " agent_root=" << agent_root
                             << " library_root=" << library_root
                             << LL_ENDL;

        mPhase = Phase::Fetching;
        mFetchTimer.start();
        scheduleInitialFetches();
    }

    processQueue();
}

void LLAsyncInventorySkeletonLoader::scheduleInitialFetches()
{
    const LLUUID agent_root = gInventory.getRootFolderID();
    if (agent_root.notNull())
    {
        enqueueFetch(agent_root, false, true, gInventory.getCachedCategoryVersion(agent_root));
        mEssentialPending.insert(agent_root);
    }

    const LLUUID library_root = gInventory.getLibraryRootFolderID();
    if (library_root.notNull())
    {
        enqueueFetch(library_root, true, true, gInventory.getCachedCategoryVersion(library_root));
        mEssentialPending.insert(library_root);
    }

    mEssentialTimer.reset();
    mEssentialTimer.start();
}

void LLAsyncInventorySkeletonLoader::processQueue()
{
    if (mPhase != Phase::Fetching)
    {
        return;
    }

    gInventory.handleResponses(false);

    while (!mFetchQueue.empty() && mActiveFetches.size() < mMaxConcurrentFetches)
    {
        FetchRequest request = mFetchQueue.front();
        mFetchQueue.pop_front();

        AISAPI::completion_t cb = [this, request](const LLUUID& response_id)
        {
            handleFetchComplete(request.mCategoryId, response_id);
        };

        LL_DEBUGS("AppInit") << "Async skeleton loader requesting AIS children for "
                             << request.mCategoryId << " (library="
                             << (request.mIsLibrary ? "true" : "false")
                             << ", essential=" << (request.mEssential ? "true" : "false")
                             << ", cached_version=" << request.mCachedVersion
                             << ")" << LL_ENDL;

        AISAPI::FetchCategoryChildren(request.mCategoryId,
                                      requestType(request.mIsLibrary),
                                      false,
                                      cb,
                                      1);
        mActiveFetches.emplace(request.mCategoryId, request);
    }
}

void LLAsyncInventorySkeletonLoader::handleFetchComplete(const LLUUID& request_id, const LLUUID& response_id)
{
    auto active_it = mActiveFetches.find(request_id);
    if (active_it == mActiveFetches.end())
    {
        LL_WARNS("AppInit") << "Async skeleton loader received unexpected completion for " << request_id << LL_ENDL;
        return;
    }

    FetchRequest request = active_it->second;
    mActiveFetches.erase(active_it);
    mQueuedCategories.erase(request_id);
    mFetchedCategories.insert(request_id);

    if (request.mEssential)
    {
        mEssentialPending.erase(request_id);
    }

    if (response_id.isNull())
    {
        LL_WARNS("AppInit") << "Async inventory skeleton loader failed to fetch "
                             << request_id << " (library="
                             << (request.mIsLibrary ? "true" : "false") << ")" << LL_ENDL;
        markFailed("AIS skeleton fetch returned no data for category " + request_id.asString());
        return;
    }

    LLViewerInventoryCategory* category = gInventory.getCategory(request_id);
    S32 server_version = LLViewerInventoryCategory::VERSION_UNKNOWN;
    if (category)
    {
        server_version = category->getVersion();
        if (server_version != LLViewerInventoryCategory::VERSION_UNKNOWN)
        {
            gInventory.rememberCachedCategoryVersion(request_id, server_version);
        }
    }

    const bool version_changed = (server_version == LLViewerInventoryCategory::VERSION_UNKNOWN)
        || (request.mCachedVersion == LLViewerInventoryCategory::VERSION_UNKNOWN)
        || (server_version != request.mCachedVersion);

    if (request_id == gInventory.getRootFolderID())
    {
        discoverEssentialFolders();
    }

    evaluateChildren(request, version_changed);

    processQueue();
}

void LLAsyncInventorySkeletonLoader::evaluateChildren(const FetchRequest& request, bool force_changed_scan)
{
    LLInventoryModel::cat_array_t* categories = nullptr;
    LLInventoryModel::item_array_t* items = nullptr;
    gInventory.getDirectDescendentsOf(request.mCategoryId, categories, items);

    if (!categories)
    {
        return;
    }

    for (const auto& child_ptr : *categories)
    {
        LLViewerInventoryCategory* child = child_ptr.get();
        if (!child)
        {
            continue;
        }

        const LLUUID& child_id = child->getUUID();
        if (child_id.isNull())
        {
            continue;
        }

        const bool already_processed = mFetchedCategories.count(child_id) > 0
            || mActiveFetches.count(child_id) > 0;

        if (already_processed)
        {
            continue;
        }

        const S32 cached_child_version = gInventory.getCachedCategoryVersion(child_id);
        const S32 current_child_version = child->getVersion();
        const bool child_version_unknown = (current_child_version == LLViewerInventoryCategory::VERSION_UNKNOWN);
        const bool child_changed = child_version_unknown
            || (cached_child_version == LLViewerInventoryCategory::VERSION_UNKNOWN)
            || (current_child_version != cached_child_version);
        const bool child_cache_valid = isCategoryUpToDate(child, cached_child_version);

        const bool child_is_library = request.mIsLibrary
            || (child->getOwnerID() == gInventory.getLibraryOwnerID());

        if (child->getPreferredType() == LLFolderType::FT_CURRENT_OUTFIT)
        {
            mSawCurrentOutfitFolder = true;
        }

        bool child_essential = false;
        if (child->getUUID() == LLAppearanceMgr::instance().getCOF())
        {
            child_essential = true;
        }
        else if (LLFolderType::lookupIsEssentialType(child->getPreferredType()))
        {
            child_essential = true;
        }

        bool should_fetch = child_changed || force_changed_scan;
        if (child_essential)
        {
            if (!should_fetch && child_cache_valid)
            {
                LL_INFOS("AsyncInventory") << "Async skeleton loader trusting cached essential folder"
                                  << " cat_id=" << child_id
                                  << " name=\"" << child->getName() << "\""
                                  << " cached_version=" << cached_child_version
                                  << " current_version=" << current_child_version
                                  << " descendents=" << child->getDescendentCount()
                                  << LL_ENDL;
                mFetchedCategories.insert(child_id);
                continue;
            }

            if (!child_cache_valid)
            {
                should_fetch = true;
            }
        }

        if (should_fetch && mQueuedCategories.count(child_id) == 0)
        {
            if (child_essential)
            {
                mEssentialPending.insert(child_id);
            }
            enqueueFetch(child_id, child_is_library, child_essential, cached_child_version);
            LL_INFOS("AsyncInventory") << "Async skeleton loader enqueued fetch"
                              << " cat_id=" << child_id
                              << " name=\"" << child->getName() << "\""
                              << " essential=" << (child_essential ? "true" : "false")
                              << " cache_valid=" << (child_cache_valid ? "true" : "false")
                              << " cached_version=" << cached_child_version
                              << " current_version=" << current_child_version
                              << LL_ENDL;
        }
        else if (child_essential && child_cache_valid)
        {
            LL_INFOS("AsyncInventory") << "Async skeleton loader treating essential folder as fetched"
                              << " cat_id=" << child_id
                              << " name=\"" << child->getName() << "\""
                              << " cached_version=" << cached_child_version
                              << " current_version=" << current_child_version
                              << LL_ENDL;
            mFetchedCategories.insert(child_id);
        }
    }
}

void LLAsyncInventorySkeletonLoader::discoverEssentialFolders()
{
    static const LLFolderType::EType essential_types[] = {
        LLFolderType::FT_CURRENT_OUTFIT,
        LLFolderType::FT_MY_OUTFITS,
        LLFolderType::FT_LOST_AND_FOUND,
        LLFolderType::FT_TRASH,
        LLFolderType::FT_INBOX,
        LLFolderType::FT_OUTBOX
    };

    for (LLFolderType::EType type : essential_types)
    {
        LLUUID cat_id = gInventory.findCategoryUUIDForType(type);
        if (cat_id.isNull())
        {
            continue;
        }

        if (type == LLFolderType::FT_CURRENT_OUTFIT)
        {
            mSawCurrentOutfitFolder = true;
        }

        LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
        bool is_library = false;
        if (cat)
        {
            is_library = (cat->getOwnerID() == gInventory.getLibraryOwnerID());
        }

        const S32 cached_version = gInventory.getCachedCategoryVersion(cat_id);
        if (cat && isCategoryUpToDate(cat, cached_version))
        {
            mFetchedCategories.insert(cat_id);
            LL_INFOS("AsyncInventory") << "Essential folder up to date from cache"
                              << " cat_id=" << cat_id
                              << " name=\"" << cat->getName() << "\""
                              << " cached_version=" << cached_version
                              << " current_version=" << cat->getVersion()
                              << " descendents=" << cat->getDescendentCount()
                              << LL_ENDL;
            continue;
        }

        if (mFetchedCategories.count(cat_id) == 0 && mQueuedCategories.count(cat_id) == 0 && mActiveFetches.count(cat_id) == 0)
        {
            enqueueFetch(cat_id, is_library, true, cached_version);
            mEssentialPending.insert(cat_id);
            LL_INFOS("AsyncInventory") << "Essential folder queued for fetch"
                              << " cat_id=" << cat_id
                              << " cached_version=" << cached_version
                              << " current_version=" << (cat ? cat->getVersion() : LLViewerInventoryCategory::VERSION_UNKNOWN)
                              << LL_ENDL;
        }
    }

    LLUUID cof_id = LLAppearanceMgr::instance().getCOF();
    if (cof_id.notNull()
        && mFetchedCategories.count(cof_id) == 0
        && mQueuedCategories.count(cof_id) == 0
        && mActiveFetches.count(cof_id) == 0)
    {
        mSawCurrentOutfitFolder = true;
        LLViewerInventoryCategory* cof = gInventory.getCategory(cof_id);
        const S32 cached_version = gInventory.getCachedCategoryVersion(cof_id);
        if (isCategoryUpToDate(cof, cached_version))
        {
            mFetchedCategories.insert(cof_id);
            LL_INFOS("Inventory") << "COF up to date from cache"
                              << " cat_id=" << cof_id
                              << " name=\"" << (cof ? cof->getName() : std::string("<null>")) << "\""
                              << " cached_version=" << cached_version
                              << " current_version=" << (cof ? cof->getVersion() : LLViewerInventoryCategory::VERSION_UNKNOWN)
                              << LL_ENDL;
        }
        else
        {
            enqueueFetch(cof_id, false, true, cached_version);
            mEssentialPending.insert(cof_id);
            LL_INFOS("Inventory") << "COF queued for fetch"
                              << " cached_version=" << cached_version
                              << " current_version=" << (cof ? cof->getVersion() : LLViewerInventoryCategory::VERSION_UNKNOWN)
                              << LL_ENDL;
        }
    }
}

void LLAsyncInventorySkeletonLoader::enqueueFetch(const LLUUID& category_id,
                                                  bool is_library,
                                                  bool essential,
                                                  S32 cached_version)
{
    if (category_id.isNull())
    {
        return;
    }

    if (mQueuedCategories.count(category_id) > 0 || mActiveFetches.count(category_id) > 0)
    {
        return;
    }

    FetchRequest request;
    request.mCategoryId = category_id;
    request.mIsLibrary = is_library;
    request.mEssential = essential;
    request.mCachedVersion = cached_version;

    mFetchQueue.push_back(request);
    mQueuedCategories.insert(category_id);
}

AISAPI::ITEM_TYPE LLAsyncInventorySkeletonLoader::requestType(bool is_library) const
{
    return is_library ? AISAPI::LIBRARY : AISAPI::INVENTORY;
}

void LLAsyncInventorySkeletonLoader::markEssentialReady()
{
    if (mEssentialReady)
    {
        return;
    }

    mEssentialReady = true;
    LL_INFOS("AppInit") << "Async inventory skeleton loader has fetched essential folders after "
                         << mTotalTimer.getElapsedTimeF32() << " seconds." << LL_ENDL;
}

void LLAsyncInventorySkeletonLoader::markComplete()
{
    if (mPhase == Phase::Complete)
    {
        return;
    }

    disconnectCapsCallback();
    removeIdleCallback();
    mPhase = Phase::Complete;
    mFetchTimer.stop();
    mTotalTimer.stop();
    LL_DEBUGS("AppInit") << "Async inventory skeleton loader finished in "
                         << mTotalTimer.getElapsedTimeF32() << " seconds." << LL_ENDL;
}

void LLAsyncInventorySkeletonLoader::markFailed(const std::string& reason)
{
    disconnectCapsCallback();
    removeIdleCallback();
    mFailureReason = reason;
    mPhase = Phase::Failed;
    mFetchTimer.stop();
    mTotalTimer.stop();
    LL_WARNS("AppInit") << "Async inventory skeleton loader failed: " << mFailureReason << LL_ENDL;
}

bool LLAsyncInventorySkeletonLoader::hasFetchedCurrentOutfit() const
{
    if (!mSawCurrentOutfitFolder)
    {
        return true;
    }

    LLUUID cof_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
    if (cof_id.isNull())
    {
        return false;
    }

    if (mFetchedCategories.count(cof_id) == 0)
    {
        return false;
    }

    const LLViewerInventoryCategory* cof = gInventory.getCategory(cof_id);
    if (!cof)
    {
        return false;
    }

    return cof->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;
}

void LLAsyncInventorySkeletonLoader::update()
{
    if (mPhase == Phase::Idle || mPhase == Phase::Complete || mPhase == Phase::Failed)
    {
        return;
    }

    if (mPhase == Phase::WaitingForCaps)
    {
        if (AISAPI::isAvailable())
        {
            startFetches();
            return;
        }

        if (mCapsTimer.getElapsedTimeF32() > mCapsTimeoutSec)
        {
            markFailed("Timed out waiting for inventory capabilities");
        }
        return;
    }

    processQueue();

    const bool current_outfit_ready = hasFetchedCurrentOutfit();

    if (!mEssentialReady && mEssentialPending.empty() && current_outfit_ready)
    {
        markEssentialReady();
    }

    if (!mEssentialReady && mEssentialTimer.getElapsedTimeF32() > mEssentialTimeoutSec)
    {
        markFailed("Timed out loading essential inventory folders");
        return;
    }

    if (mFetchTimer.getElapsedTimeF32() > mFetchTimeoutSec)
    {
        markFailed("Timed out while fetching inventory skeleton via AIS");
        return;
    }

    if (mFetchQueue.empty() && mActiveFetches.empty())
    {
        markComplete();
    }
}

bool LLAsyncInventorySkeletonLoader::isCategoryUpToDate(const LLViewerInventoryCategory* cat, S32 cached_version) const
{
    if (!cat)
    {
        return false;
    }

    if (cached_version == LLViewerInventoryCategory::VERSION_UNKNOWN)
    {
        return false;
    }

    if (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
    {
        return false;
    }

    if (cat->getDescendentCount() == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN)
    {
        return false;
    }

    return cat->getVersion() == cached_version;
}
