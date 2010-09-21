/** 
 * @file llviewerassetstorage.cpp
 * @brief Subclass capable of loading asset data to/from an external source.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewerassetstorage.h"

#include "llvfile.h"
#include "llvfs.h"
#include "message.h"

#include "llagent.h"

LLViewerAssetStorage::LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
										   LLVFS *vfs, LLVFS *static_vfs, 
										   const LLHost &upstream_host)
		: LLAssetStorage(msg, xfer, vfs, static_vfs, upstream_host)
{
}


LLViewerAssetStorage::LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
										   LLVFS *vfs, LLVFS *static_vfs)
		: LLAssetStorage(msg, xfer, vfs, static_vfs)
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
	bool store_local,
	bool user_waiting,
	F64 timeout)
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
				// LLAssetStorage metric: Zero size VFS
				reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file didn't exist or was zero length (VFS - can't tell which)" );

				delete req;
				if (callback)
				{
					callback(asset_id, user_data, LL_ERR_ASSET_REQUEST_FAILED, LL_EXSTAT_VFS_CORRUPT);
				}
				return;
			}
			else
			{
				// LLAssetStorage metric: Successful Request
				S32 size = mVFS->getSize(asset_id, asset_type);
				const char *message = "Added to upload queue";
				reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, size, MR_OKAY, __FILE__, __LINE__, message );

				if(is_priority)
				{
					mPendingUploads.push_front(req);
				}
				else
				{
					mPendingUploads.push_back(req);
				}
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

					// LLAssetStorage metric: VFS corrupt - bogus size
					reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, asset_size, MR_VFS_CORRUPTION, __FILE__, __LINE__, "VFS corruption" );

					if (callback)
					{
						callback(asset_id, user_data, LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE, LL_EXSTAT_VFS_CORRUPT);
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
			// LLAssetStorage metric: Zero size VFS
			reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file didn't exist or was zero length (VFS - can't tell which)" );
			if (callback)
			{
				callback(asset_id, user_data,  LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE, LL_EXSTAT_NONEXISTENT_FILE);
			}
		}
	}
	else
	{
		llwarns << "Attempt to move asset store request upstream w/o valid upstream provider" << llendl;
		// LLAssetStorage metric: Upstream provider dead
		reportMetric( asset_id, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_NO_UPSTREAM, __FILE__, __LINE__, "No upstream provider" );
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CIRCUIT_GONE, LL_EXSTAT_NO_UPSTREAM);
		}
	}
}

void LLViewerAssetStorage::storeAssetData(
	const std::string& filename,
	const LLTransactionID& tid,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority,
	bool user_waiting,
	F64 timeout)
{
	if(filename.empty())
	{
		// LLAssetStorage metric: no filename
		reportMetric( LLUUID::null, asset_type, LLStringUtil::null, LLUUID::null, 0, MR_VFS_CORRUPTION, __FILE__, __LINE__, "Filename missing" );
		llerrs << "No filename specified" << llendl;
		return;
	}
	
	LLAssetID asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	llinfos << "LLViewerAssetStorage::storeAssetData (legacy)" << asset_id << ":" << LLAssetType::lookup(asset_type) << llendl;

	llinfos << "ASSET_ID: " << asset_id << llendl;

	S32 size = 0;
	LLFILE* fp = LLFile::fopen(filename, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}
	if( size )
	{
		LLLegacyAssetRequest *legacy = new LLLegacyAssetRequest;
		
		legacy->mUpCallback = callback;
		legacy->mUserData = user_data;

		LLVFile file(mVFS, asset_id, asset_type, LLVFile::WRITE);

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

		// LLAssetStorage metric: Success not needed; handled in the overloaded method here:

		LLViewerAssetStorage::storeAssetData(
			tid,
			asset_type,
			legacyStoreDataCallback,
			(void**)legacy,
			temp_file,
			is_priority);
	}
	else // size == 0 (but previous block changes size)
	{
		if( fp )
		{
			// LLAssetStorage metric: Zero size
			reportMetric( asset_id, asset_type, filename, LLUUID::null, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file was zero length" );
		}
		else
		{
			// LLAssetStorage metric: Missing File
			reportMetric( asset_id, asset_type, filename, LLUUID::null, 0, MR_FILE_NONEXIST, __FILE__, __LINE__, "The file didn't exist" );
		}
		if (callback)
		{
			callback(asset_id, user_data, LL_ERR_CANNOT_OPEN_FILE, LL_EXSTAT_BLOCKED_FILE);
		}
	}
}
