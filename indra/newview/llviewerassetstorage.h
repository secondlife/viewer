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
//#include "curl/curl.h"

class LLVFile;

class LLViewerAssetStorage : public LLAssetStorage
{
public:
	LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs, const LLHost &upstream_host);

	LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs);

	using LLAssetStorage::storeAssetData;
	virtual void storeAssetData(
		const LLTransactionID& tid,
		LLAssetType::EType atype,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool store_local = false,
		bool user_waiting=FALSE,
		F64 timeout=LL_ASSET_STORAGE_TIMEOUT);
	
	virtual void storeAssetData(
		const std::string& filename,
		const LLTransactionID& tid,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool user_waiting=FALSE,
		F64 timeout=LL_ASSET_STORAGE_TIMEOUT);

protected:
	using LLAssetStorage::_queueDataRequest;

	// virtual
	void _queueDataRequest(const LLUUID& uuid,
						   LLAssetType::EType type,
						   void (*callback) (LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32, LLExtStat),
						   void *user_data,
						   BOOL duplicate,
						   BOOL is_priority);
};

#endif
