/**
 * @file llinventorymodelbackgroundfetch.h
 * @brief LLInventoryModelBackgroundFetch class header file
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

#ifndef LL_LLINVENTORYMODELBACKGROUNDFETCH_H
#define LL_LLINVENTORYMODELBACKGROUNDFETCH_H

#include "llsingleton.h"
#include "lluuid.h"
#include "httpcommon.h"
#include "httprequest.h"
#include "httpoptions.h"
#include "httpheaders.h"
#include "httphandler.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryModelBackgroundFetch
//
// This class handles background fetches, which are fetches of
// inventory folder.  Fetches can be recursive or not.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryModelBackgroundFetch : public LLSingleton<LLInventoryModelBackgroundFetch>
{
    LLSINGLETON(LLInventoryModelBackgroundFetch);
    ~LLInventoryModelBackgroundFetch();
public:

    // Start background breadth-first fetching of inventory contents.
    // This gets triggered when performing a filter-search.
    void start(const LLUUID& cat_id = LLUUID::null, bool recursive = true);
    void scheduleFolderFetch(const LLUUID& cat_id, bool forced = false);
    void scheduleItemFetch(const LLUUID& item_id, bool forced = false);

    typedef boost::function<void()> nullary_func_t;
    // AIS3 only, Fetches folder and everything links inside the folder point to
    // Intended for outfits
    void fetchFolderAndLinks(const LLUUID& cat_id, nullary_func_t callback);
    // AIS3 only
    void fetchCOF(nullary_func_t callback);

    bool folderFetchActive() const;
    bool isEverythingFetched() const; // completing the fetch once per session should be sufficient

    bool libraryFetchStarted() const;
    bool libraryFetchCompleted() const;
    bool libraryFetchInProgress() const;

    bool inventoryFetchStarted() const;
    bool inventoryFetchCompleted() const;
    bool inventoryFetchInProgress() const;

    void findLostItems();
    void incrFetchCount(S32 fetching);
    void incrFetchFolderCount(S32 fetching);

    bool isBulkFetchProcessingComplete() const;
    void setAllFoldersFetched();

    typedef boost::function<void()> folders_fetched_callback_t;
    boost::signals2::connection setFetchCompletionCallback(folders_fetched_callback_t cb);

    void addRequestAtFront(const LLUUID& id, bool recursive, bool is_category);
    void addRequestAtBack(const LLUUID& id, bool recursive, bool is_category);

protected:
    bool isFolderFetchProcessingComplete() const;

    typedef enum {
        FT_DEFAULT = 0,
        FT_FORCED, // request non-recursively even if already loaded
        FT_CONTENT_RECURSIVE, // request content recursively
        FT_FOLDER_AND_CONTENT, // request folder, then content recursively
        FT_RECURSIVE, // request everything recursively
    } EFetchType;
    struct FetchQueueInfo
    {
        FetchQueueInfo(const LLUUID& id, EFetchType recursive, bool is_category = true)
            : mUUID(id),
            mIsCategory(is_category),
            mFetchType(recursive)
        {}

        LLUUID mUUID;
        bool mIsCategory;
        EFetchType mFetchType;
    };
    typedef std::deque<FetchQueueInfo> fetch_queue_t;

    void onAISContentCalback(const LLUUID& request_id, const uuid_vec_t& content_ids, const LLUUID& response_id, EFetchType fetch_type);
    void onAISFolderCalback(const LLUUID& request_id, const LLUUID& response_id, EFetchType fetch_type);
    void bulkFetchViaAis();
    void bulkFetchViaAis(const FetchQueueInfo& fetch_info);
    void bulkFetch();

    void backgroundFetch();
    static void backgroundFetchCB(void*); // background fetch idle function

    bool fetchQueueContainsNoDescendentsOf(const LLUUID& cat_id) const;

private:
    bool mRecursiveInventoryFetchStarted;
    bool mRecursiveLibraryFetchStarted;
    bool mRecursiveMarketplaceFetchStarted; // AIS3 specific
    bool mAllRecursiveFoldersFetched;
    typedef boost::signals2::signal<void()> folders_fetched_signal_t;
    folders_fetched_signal_t mFoldersFetchedSignal;

    bool mBackgroundFetchActive;
    bool mFolderFetchActive;
    S32 mFetchCount;
    S32 mLastFetchCount; // for debug
    S32 mFetchFolderCount;

    LLFrameTimer mFetchTimer;
    F32 mMinTimeBetweenFetches;
    fetch_queue_t mFetchFolderQueue;
    fetch_queue_t mFetchItemQueue;
    uuid_set_t mForceFetchSet;
    std::list<LLUUID> mExpectedFolderIds; // for debug, should this track time?
};

#endif // LL_LLINVENTORYMODELBACKGROUNDFETCH_H

