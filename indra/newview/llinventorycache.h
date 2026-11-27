/**
 * @file llinventorycache.h
 * @brief contains LLCacheLoadResult for async inventory
 * cache loading.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#ifndef LL_LLINVENTORYCACHE_H
#define LL_LLINVENTORYCACHE_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "lluuid.h"
#include "llviewerinventory.h"

typedef std::vector<LLPointer<LLViewerInventoryCategory> > cat_array_t;
typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
typedef std::unordered_map<LLUUID, LLPointer<LLViewerInventoryCategory> > cat_map_t;
typedef std::unordered_map<LLUUID, LLPointer<LLViewerInventoryItem>>     item_map_t;
typedef std::set<LLUUID> changed_items_t;

// Async cache loading parses cache file on a worker thread into
// LLCacheLoadResult, then merges priority results into the live
// inventory model on the main thread. Non-priority results get
// passed to background fetch.
struct LLCacheLoadResult
{
    // Actual cache fields
    cat_map_t       mCategories;
    item_map_t      mItems;
    uuid_set_t      mPriorityCats;
    changed_items_t mCategoriesToUpdate;
    std::unordered_map<LLUUID, uuid_vec_t> mChildItemIds; // parent category id -> item ids
    bool            mIsCacheObsolete = true;

    // Where inventory cache will be or was loaded from.
    std::string     mInventoryFilename;
    std::string     mGzipFilename;

    // Indicates whether temporary file was created during cache load.
    // Cache load should be the one removing the file.
    bool            mRemoveInventoryFile = false;

    // Cats that will be loaded from cache by background fetcher.
    uuid_set_t      mValidDeferredCats;

    // Todo: might be a good idea to store favorited folders and items here for prioritized load.

    // Debug information
    S32 mNumCatsLoaded = 0;
    S32 mNumItemsLoaded = 0;
};

#endif // LL_LLINVENTORYCACHE_H

