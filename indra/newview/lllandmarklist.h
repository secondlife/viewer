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

#ifndef LL_LLLANDMARKLIST_H
#define LL_LLLANDMARKLIST_H

#include <boost/function.hpp>
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
    typedef boost::function<void(LLLandmark*)> loaded_callback_t;

    LLLandmarkList() {}
    ~LLLandmarkList();

    //S32                   getLength() { return mList.getLength(); }
    //const LLLandmark* getFirst()  { return mList.getFirstData(); }
    //const LLLandmark* getNext()   { return mList.getNextData(); }

    BOOL assetExists(const LLUUID& asset_uuid);
    LLLandmark* getAsset(const LLUUID& asset_uuid, loaded_callback_t cb = NULL);
    static void processGetAssetReply(
        const LLUUID& uuid,
        LLAssetType::EType type,
        void* user_data,
        S32 status,
        LLExtStat ext_status );

    // Returns TRUE if loading the landmark with given asset_uuid has been requested
    // but is not complete yet.
    BOOL isAssetInLoadedCallbackMap(const LLUUID& asset_uuid);

protected:
    void onRegionHandle(const LLUUID& landmark_id);
    void eraseCallbacks(const LLUUID& landmark_id);
    void makeCallbacks(const LLUUID& landmark_id);

    typedef std::map<LLUUID, LLLandmark*> landmark_list_t;
    landmark_list_t mList;

    typedef std::set<LLUUID> landmark_uuid_list_t;
    landmark_uuid_list_t mBadList;
    landmark_uuid_list_t mWaitList;

    typedef std::map<LLUUID,F32> landmark_requested_list_t;
    landmark_requested_list_t mRequestedList;
    
    // *TODO: make the callback multimap a template class and make use of it
    // here and in LLLandmark.
    typedef std::multimap<LLUUID, loaded_callback_t> loaded_callback_map_t;
    loaded_callback_map_t mLoadedCallbackMap;
};


extern LLLandmarkList gLandmarkList;

#endif  // LL_LLLANDMARKLIST_H
