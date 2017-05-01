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

class LLVFile;

class LLViewerAssetRequest;

class LLViewerAssetStorage : public LLAssetStorage
{
public:
	LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs, const LLHost &upstream_host);

	LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs);

	virtual void storeAssetData(
		const LLTransactionID& tid,
		LLAssetType::EType atype,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool store_local = false,
		bool user_waiting=FALSE,
		F64Seconds timeout=LL_ASSET_STORAGE_TIMEOUT);
	
	virtual void storeAssetData(
		const std::string& filename,
		const LLTransactionID& tid,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool user_waiting=FALSE,
		F64Seconds timeout=LL_ASSET_STORAGE_TIMEOUT);

protected:
	// virtual
	void _queueDataRequest(const LLUUID& uuid,
						   LLAssetType::EType type,
						   void (*callback) (LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32, LLExtStat),
						   void *user_data,
						   BOOL duplicate,
						   BOOL is_priority);

    void queueRequestHttp(const LLUUID& uuid,
                          LLAssetType::EType type,
                          void (*callback) (LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32, LLExtStat),
                          void *user_data,
                          BOOL duplicate,
                          BOOL is_priority);

    void capsRecvForRegion(const LLUUID& region_id, std::string pumpname);
    
    void assetRequestCoro(LLViewerAssetRequest *req,
                          const LLUUID uuid,
                          LLAssetType::EType atype,
                          void (*callback) (LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32, LLExtStat),
                          void *user_data);

    std::string getAssetURL(const std::string& cap_url, const LLUUID& uuid, LLAssetType::EType atype);

    void logAssetStorageInfo();
    
    std::string mViewerAssetUrl;
    S32 mAssetCoroCount;
    S32 mCountRequests;
    S32 mCountStarted;
    S32 mCountCompleted;
    S32 mCountSucceeded;
    S64 mTotalBytesFetched;
};

#endif
