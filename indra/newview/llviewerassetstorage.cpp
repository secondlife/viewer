/** 
 * @file llviewerassetstorage.cpp
 * @brief Subclass capable of loading asset data to/from an external source.
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

#include "llviewerprecompiledheaders.h"

#include "llviewerassetstorage.h"

#include "llvfile.h"
#include "llvfs.h"
#include "message.h"

#include "llagent.h"
#include "lltransfersourceasset.h"
#include "lltransfertargetvfile.h"
#include "llviewerassetstats.h"

///----------------------------------------------------------------------------
/// LLViewerAssetRequest
///----------------------------------------------------------------------------

/**
 * @brief Local class to encapsulate asset fetch requests with a timestamp.
 *
 * Derived from the common LLAssetRequest class, this is currently used
 * only for fetch/get operations and its only function is to wrap remote
 * asset fetch requests so that they can be timed.
 */
class LLViewerAssetRequest : public LLAssetRequest
{
public:
	LLViewerAssetRequest(const LLUUID &uuid, const LLAssetType::EType type)
		: LLAssetRequest(uuid, type),
		  mMetricsStartTime(0)
		{
		}
	
	LLViewerAssetRequest & operator=(const LLViewerAssetRequest &);	// Not defined
	// Default assignment operator valid
	
	// virtual
	~LLViewerAssetRequest()
		{
			recordMetrics();
		}

protected:
	void recordMetrics()
		{
			if (mMetricsStartTime)
			{
				// Okay, it appears this request was used for useful things.  Record
				// the expected dequeue and duration of request processing.
				LLViewerAssetStatsFF::record_dequeue_main(mType, false, false);
				LLViewerAssetStatsFF::record_response_main(mType, false, false,
														   (LLViewerAssetStatsFF::get_timestamp()
															- mMetricsStartTime));
				mMetricsStartTime = 0;
			}
		}
	
public:
	LLViewerAssetStats::duration_t		mMetricsStartTime;
};

///----------------------------------------------------------------------------
/// LLViewerAssetStorage
///----------------------------------------------------------------------------

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


/**
 * @brief Allocate and queue an asset fetch request for the viewer
 *
 * This is a nearly-verbatim copy of the base class's implementation
 * with the following changes:
 *  -  Use a locally-derived request class
 *  -  Start timing for metrics when request is queued
 *
 * This is an unfortunate implementation choice but it's forced by
 * current conditions.  A refactoring that might clean up the layers
 * of responsibility or introduce factories or more virtualization
 * of methods would enable a more attractive solution.
 *
 * If LLAssetStorage::_queueDataRequest changes, this must change
 * as well.
 */

// virtual
void LLViewerAssetStorage::_queueDataRequest(
	const LLUUID& uuid,
	LLAssetType::EType atype,
	LLGetAssetCallback callback,
	void *user_data,
	BOOL duplicate,
	BOOL is_priority)
{
	if (mUpstreamHost.isOk())
	{
		// stash the callback info so we can find it after we get the response message
		LLViewerAssetRequest *req = new LLViewerAssetRequest(uuid, atype);
		req->mDownCallback = callback;
		req->mUserData = user_data;
		req->mIsPriority = is_priority;
		req->mMetricsStartTime = LLViewerAssetStatsFF::get_timestamp();
		
		mPendingDownloads.push_back(req);
	
		if (!duplicate)
		{
			// send request message to our upstream data provider
			// Create a new asset transfer.
			LLTransferSourceParamsAsset spa;
			spa.setAsset(uuid, atype);

			// Set our destination file, and the completion callback.
			LLTransferTargetParamsVFile tpvf;
			tpvf.setAsset(uuid, atype);
			tpvf.setCallback(downloadCompleteCallback, req);

			llinfos << "Starting transfer for " << uuid << llendl;
			LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(mUpstreamHost, LLTCT_ASSET);
			ttcp->requestTransfer(spa, tpvf, 100.f + (is_priority ? 1.f : 0.f));

			LLViewerAssetStatsFF::record_enqueue_main(atype, false, false);
		}
	}
	else
	{
		// uh-oh, we shouldn't have gotten here
		llwarns << "Attempt to move asset data request upstream w/o valid upstream provider" << llendl;
		if (callback)
		{
			callback(mVFS, uuid, atype, user_data, LL_ERR_CIRCUIT_GONE, LL_EXSTAT_NO_UPSTREAM);
		}
	}
}

