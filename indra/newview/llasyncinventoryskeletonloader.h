/**
 * @file llasyncinventoryskeletonloader.h
 * @brief Async inventory skeleton loading helper.
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

#ifndef LL_LLASYNCINVENTORYSKELETONLOADER_H
#define LL_LLASYNCINVENTORYSKELETONLOADER_H

#include "llframetimer.h"
#include "lluuid.h"
#include "llaisapi.h"
#include "llinventorymodel.h"

#include <deque>
#include <map>
#include <set>
#include <string>

#include <boost/signals2/connection.hpp>

class LLViewerInventoryCategory;
class LLViewerRegion;

class LLAsyncInventorySkeletonLoader
{
public:
    void start(bool force_async);
    void update();
    bool isRunning() const;
    bool isComplete() const { return mPhase == Phase::Complete; }
    bool hasFailed() const { return mPhase == Phase::Failed; }
    bool isEssentialReady() const { return mEssentialReady; }
    const std::string& failureReason() const { return mFailureReason; }
    F32 elapsedSeconds() const { return mTotalTimer.getElapsedTimeF32(); }
    void reset();

private:
    struct FetchRequest
    {
        LLUUID mCategoryId;
        bool mIsLibrary = false;
        bool mEssential = false;
    S32 mCachedVersion = LLViewerInventoryCategory::VERSION_UNKNOWN;
    };

    enum class Phase
    {
        Idle,
        WaitingForCaps,
        Fetching,
        Complete,
        Failed
    };

    void ensureCapsCallback();
    void disconnectCapsCallback();
    void onCapsReceived(const LLUUID& region_id, LLViewerRegion* regionp);
    void ensureIdleCallback();
    void removeIdleCallback();
    static void idleCallback(void* userdata);

    void startFetches();
    void scheduleInitialFetches();
    void processQueue();
    void handleFetchComplete(const LLUUID& request_id, const LLUUID& response_id);
    void evaluateChildren(const FetchRequest& request, bool force_changed_scan);
    void discoverEssentialFolders();
    void enqueueFetch(const LLUUID& category_id, bool is_library, bool essential, S32 cached_version);
    bool isCategoryUpToDate(const LLViewerInventoryCategory* cat, S32 cached_version) const;
    AISAPI::ITEM_TYPE requestType(bool is_library) const;
    void markEssentialReady();
    void markComplete();
    void markFailed(const std::string& reason);
    bool hasFetchedCurrentOutfit() const;

    Phase mPhase = Phase::Idle;
    bool mForceAsync = false;
    bool mEssentialReady = false;

    std::deque<FetchRequest> mFetchQueue;
    std::map<LLUUID, FetchRequest> mActiveFetches;
    std::set<LLUUID> mQueuedCategories;
    std::set<LLUUID> mFetchedCategories;
    std::set<LLUUID> mEssentialPending;

    U32 mMaxConcurrentFetches = 4;
    bool mSawCurrentOutfitFolder = false;

    LLFrameTimer mCapsTimer;
    LLFrameTimer mFetchTimer;
    LLFrameTimer mTotalTimer;
    LLFrameTimer mEssentialTimer;

    F32 mCapsTimeoutSec = 0.f;
    F32 mFetchTimeoutSec = 0.f;
    F32 mEssentialTimeoutSec = 0.f;

    boost::signals2::connection mCapsConnection;
    bool mIdleRegistered = false;
    std::string mFailureReason;
};

extern LLAsyncInventorySkeletonLoader gAsyncInventorySkeletonLoader;
extern bool gAsyncAgentCacheHydrated;
extern bool gAsyncLibraryCacheHydrated;
extern bool gAsyncParentChildMapPrimed;

#endif // LL_LLASYNCINVENTORYSKELETONLOADER_H
