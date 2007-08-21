/** 
 * @file llviewerassetstorage.h
 * @brief Class for loading asset data to/from an external source.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
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
				   LLVFS *vfs, const LLHost &upstream_host);

	LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs);

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
		const char* filename,
		const LLTransactionID& tid,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool user_waiting=FALSE,
		F64 timeout=LL_ASSET_STORAGE_TIMEOUT);
};

#endif
