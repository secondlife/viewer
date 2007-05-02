/** 
 * @file llviewerassetstorage.cpp
 * @brief Subclass capable of loading asset data to/from an external source.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */


#include "linden_common.h"
#include "llviewerprecompiledheaders.h"

#include "llagent.h"
#include "llviewerassetstorage.h"
#include "llviewerbuild.h"
#include "llvfile.h"
#include "llvfs.h"

LLViewerAssetStorage::LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
										   LLVFS *vfs, const LLHost &upstream_host)
		: LLAssetStorage(msg, xfer, vfs, upstream_host)
{
}


LLViewerAssetStorage::LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
										   LLVFS *vfs)
		: LLAssetStorage(msg, xfer, vfs)
{
}

// virtual 
void LLViewerAssetStorage::storeAssetData(
	const LLTransactionID& tid,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority,
	bool store_local)
{
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	llinfos << "LLViewerAssetStorage::storeAssetData (legacy) " << tid << ":" << LLAssetType::lookup(asset_type)
			<< " ASSET_ID: " << asset_id << llendl;
	
	if (mUpstreamHost.isOk())
	{
		if (mVFS->getExists(asset_id, asset_type))
		{
			// Pack data into this packet if we can fit it.
			U8 buffer[MTUBYTES];
			buffer[0] = 0;

			LLVFile vfile(mVFS, asset_id, asset_type, LLVFile::READ);
			S32 asset_size = vfile.getSize();

			LLAssetRequest *req = new LLAssetRequest(asset_id, asset_type);
			req->mUpCallback = callback;
			req->mUserData = user_data;

			if (asset_size < 1)
			{
				// This can happen if there's a bug in our code or if the VFS has been corrupted.
				llwarns << "LLViewerAssetStorage::storeAssetData()  Data _should_ already be in the VFS, but it's not! " << asset_id << llendl;

				delete req;
				if (callback)
				{
					callback(asset_id, user_data, LL_ERR_ASSET_REQUEST_FAILED);
				}
				return;
			}
			else if(is_priority)
			{
				mPendingUploads.push_front(req);
			}
			else
			{
				mPendingUploads.push_back(req);
			}

			// Read the data from the VFS if it'll fit in this packet.
			if (asset_size + 100 < MTUBYTES)
			{
				BOOL res = vfile.read(buffer, asset_size);		/* Flawfinder: ignore */
				S32 bytes_read = res ? vfile.getLastBytesRead() : 0;
				
				if( bytes_read == asset_size )
				{
					req->mDataSentInFirstPacket = TRUE;
					//llinfos << "LLViewerAssetStorage::createAsset sending data in first packet" << llendl;
				}
				else
				{
					llwarns << "Probable corruption in VFS file, aborting store asset data" << llendl;
					if (callback)
					{
						callback(asset_id, user_data,  LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE);
					}
					return;
				}
			}
			else
			{
				// Too big, do an xfer
				buffer[0] = 0;
				asset_size = 0;
			}
			mMessageSys->newMessageFast(_PREHASH_AssetUploadRequest);
			mMessageSys->nextBlockFast(_PREHASH_AssetBlock);
			mMessageSys->addUUIDFast(_PREHASH_TransactionID, tid);
			mMessageSys->addS8Fast(_PREHASH_Type, (S8)asset_type);
			mMessageSys->addBOOLFast(_PREHASH_Tempfile, temp_file);
			mMessageSys->addBOOLFast(_PREHASH_StoreLocal, store_local);
			mMessageSys->addBinaryDataFast( _PREHASH_AssetData, buffer, asset_size );
			mMessageSys->sendReliable(mUpstreamHost);
		}
		else
		{
			llwarns << "AssetStorage: attempt to upload non-existent vfile " << asset_id << ":" << LLAssetType::lookup(asset_type) << llendl;
			if (callback)
			{
				callback(asset_id, user_data,  LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE);
			}
		}
	}
	else
	{
		llwarns << "Attempt to move asset store request upstream w/o valid upstream provider" << llendl;
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CIRCUIT_GONE);
		}
	}
}

void LLViewerAssetStorage::storeAssetData(
	const char* filename,
	const LLTransactionID& tid,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority)
{
	if(!filename)
	{
		llerrs << "No filename specified" << llendl;
		return;
	}
	
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	llinfos << "LLViewerAssetStorage::storeAssetData (legacy)" << asset_id << ":" << LLAssetType::lookup(asset_type) << llendl;

	llinfos << "ASSET_ID: " << asset_id << llendl;

	FILE* fp = LLFile::fopen(filename, "rb");
	if (fp)
	{
		LLLegacyAssetRequest *legacy = new LLLegacyAssetRequest;
		
		legacy->mUpCallback = callback;
		legacy->mUserData = user_data;

		LLVFile file(mVFS, asset_id, asset_type, LLVFile::WRITE);

		fseek(fp, 0, SEEK_END);
		S32 size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		file.setMaxSize(size);

		const S32 buf_size = 65536;
		U8 copy_buf[buf_size];
		while ((size = (S32)fread(copy_buf, 1, buf_size, fp)))
		{
			file.write(copy_buf, size);
		}
		fclose(fp);

		// if this upload fails, the caller needs to setup a new tempfile for us
		if (temp_file)
		{
			LLFile::remove(filename);
		}
		
		LLViewerAssetStorage::storeAssetData(
			tid,
			asset_type,
			legacyStoreDataCallback,
			(void**)legacy,
			temp_file,
			is_priority);
	}
	else
	{
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CANNOT_OPEN_FILE);
		}
	}
}
