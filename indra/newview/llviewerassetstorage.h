/** 
 * @file llviewerassetstorage.h
 * @brief Class for loading asset data to/from an external source.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLVIEWERASSETSTORAGE_H
#define LLVIEWERASSETSTORAGE_H

#include "llassetstorage.h"
#include "llcorehttputil.h"

class LLFileSystem;

class LLViewerAssetRequest;

class LLViewerAssetStorage : public LLAssetStorage
{
public:
    LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer, const LLHost &upstream_host);

    LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer);

    ~LLViewerAssetStorage();

    void storeAssetData(
        const LLTransactionID& tid,
        LLAssetType::EType atype,
        LLStoreAssetCallback callback,
        void* user_data,
        bool temp_file = false,
        bool is_priority = false,
        bool store_local = false,
        bool user_waiting=FALSE,
        F64Seconds timeout=LL_ASSET_STORAGE_TIMEOUT) override;

    void storeAssetData(
        const std::string& filename,
        const LLTransactionID& tid,
        LLAssetType::EType type,
        LLStoreAssetCallback callback,
        void* user_data,
        bool temp_file = false,
        bool is_priority = false,
        bool user_waiting=FALSE,
        F64Seconds timeout=LL_ASSET_STORAGE_TIMEOUT) override;

    void checkForTimeouts() override;

protected:
    void _queueDataRequest(const LLUUID& uuid,
                           LLAssetType::EType type,
                           LLGetAssetCallback callback,
                           void *user_data,
                           BOOL duplicate,
                           BOOL is_priority) override;

    void queueRequestHttp(const LLUUID& uuid,
                          LLAssetType::EType type,
                          LLGetAssetCallback callback,
                          void *user_data,
                          BOOL duplicate,
                          BOOL is_priority);

    void capsRecvForRegion(const LLUUID& region_id, std::string pumpname);
    
    void assetRequestCoro(LLViewerAssetRequest *req,
                          const LLUUID uuid,
                          LLAssetType::EType atype,
                          LLGetAssetCallback callback,
                          void *user_data);

    std::string getAssetURL(const std::string& cap_url, const LLUUID& uuid, LLAssetType::EType atype);

    void logAssetStorageInfo() override;

    // Asset storage works through coroutines and coroutines have limited queue capacity
    // This class is meant to temporary store requests when fiber's queue is full
    class CoroWaitList
    {
    public:
        CoroWaitList(LLViewerAssetRequest *req,
            const LLUUID& uuid,
            LLAssetType::EType atype,
            LLGetAssetCallback &callback,
            void *user_data)
          : mRequest(req),
            mId(uuid),
            mType(atype),
            mCallback(callback),
            mUserData(user_data)
        {
        }

        LLViewerAssetRequest* mRequest;
        LLUUID mId;
        LLAssetType::EType mType;
        LLGetAssetCallback mCallback;
        void *mUserData;
    };
    typedef std::list<CoroWaitList> wait_list_t;
    wait_list_t mCoroWaitList;

    std::string mViewerAssetUrl;
    S32 mCountRequests;
    S32 mCountStarted;
    S32 mCountCompleted;
    S32 mCountSucceeded;
    S64 mTotalBytesFetched;

    static S32 sAssetCoroCount; // coroutine count, static since coroutines can outlive LLViewerAssetStorage
};

#endif
