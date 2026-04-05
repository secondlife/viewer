/**
 * @file lllandmarklist.h
 * @brief Landmark asset list class
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

#pragma once

#include <functional>
#include <map>
#include "lllandmark.h"
#include "lluuid.h"
#include "llassetstorage.h"

class LLMessageSystem;
class LLLineEditor;
class LLInventoryItem;

class LLLandmarkList
{
public:
    using loaded_callback_t = std::function<void(LLLandmark*)>;

    LLLandmarkList() = default;
    ~LLLandmarkList();

    //S32                   getLength() { return mList.getLength(); }
    //const LLLandmark* getFirst()  { return mList.getFirstData(); }
    //const LLLandmark* getNext()   { return mList.getNextData(); }

    bool assetExists(const LLUUID& asset_uuid);
    LLLandmark* getAsset(const LLUUID& asset_uuid, loaded_callback_t cb = nullptr);
    static void processGetAssetReply(
        const LLUUID& uuid,
        LLAssetType::EType type,
        void* user_data,
        S32 status,
        LLExtStat ext_status );

    // Returns true if loading the landmark with given asset_uuid has been requested
    // but is not complete yet.
    bool isAssetInLoadedCallbackMap(const LLUUID& asset_uuid);

protected:
    void onRegionHandle(const LLUUID& landmark_id);
    void eraseCallbacks(const LLUUID& landmark_id);
    void makeCallbacks(const LLUUID& landmark_id);

    using landmark_list_t = std::map<LLUUID, LLLandmark*>;
    landmark_list_t mList;

    using landmark_uuid_list_t = std::set<LLUUID>;
    landmark_uuid_list_t mBadList;
    landmark_uuid_list_t mRetryList;

    using landmark_requested_list_t = std::map<LLUUID,F32>;
    landmark_requested_list_t mRequestedList;

    // *TODO: make the callback multimap a template class and make use of it
    // here and in LLLandmark.
    using loaded_callback_map_t = std::multimap<LLUUID, loaded_callback_t>;
    loaded_callback_map_t mLoadedCallbackMap;
};


extern LLLandmarkList gLandmarkList;

