/** 
 * @file lltransfersourceasset.cpp
 * @brief Transfer system for sending an asset.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "linden_common.h"

#include "lltransfersourceasset.h"

#include "llerror.h"
#include "message.h"
#include "lldatapacker.h"
#include "lldir.h"
#include "llvfile.h"

LLTransferSourceAsset::LLTransferSourceAsset(const LLUUID &request_id, const F32 priority) :
	LLTransferSource(LLTST_ASSET, request_id, priority),
	mGotResponse(FALSE),
	mCurPos(0)
{
}

LLTransferSourceAsset::~LLTransferSourceAsset()
{
}


void LLTransferSourceAsset::initTransfer()
{
	if (gAssetStorage)
	{
		// *HACK: asset transfers will only be coming from the viewer
		// to the simulator. This is subset of assets we allow to be
		// simply pulled straight from the asset system.
		LLUUID* tidp;
		if(LLAssetType::lookupIsAssetFetchByIDAllowed(mParams.getAssetType()))
		{
			tidp = new LLUUID(getID());
			gAssetStorage->getAssetData(
				mParams.getAssetID(),
				mParams.getAssetType(),
				LLTransferSourceAsset::responderCallback,
				tidp,
				FALSE);
		}
		else
		{
			llwarns << "Attempted to request blocked asset "
				<< mParams.getAssetID() << ":"
				<< LLAssetType::lookupHumanReadable(mParams.getAssetType())
				<< llendl;
			sendTransferStatus(LLTS_ERROR);
		}
	}
	else
	{
		llwarns << "Attempted to request asset " << mParams.getAssetID()
			<< ":" << LLAssetType::lookupHumanReadable(mParams.getAssetType())
			<< " without an asset system!" << llendl;
		sendTransferStatus(LLTS_ERROR);
	}
}

F32 LLTransferSourceAsset::updatePriority()
{
	return 0.f;
}

LLTSCode LLTransferSourceAsset::dataCallback(const S32 packet_id,
											const S32 max_bytes,
											U8 **data_handle,
											S32 &returned_bytes,
											BOOL &delete_returned)
{
	//llinfos << "LLTransferSourceAsset::dataCallback" << llendl;
	if (!mGotResponse)
	{
		return LLTS_SKIP;
	}

	LLVFile vf(gAssetStorage->mVFS, mParams.getAssetID(), mParams.getAssetType(), LLVFile::READ);

	if (!vf.getSize())
	{
		// Something bad happened with the asset request!
		return LLTS_ERROR;
	}

	if (packet_id != mLastPacketID + 1)
	{
		llerrs << "Can't handle out of order file transfer yet!" << llendl;
	}

	// grab a buffer from the right place in the file
	if (!vf.seek(mCurPos, 0))
	{
		llwarns << "LLTransferSourceAsset Can't seek to " << mCurPos << " length " << vf.getSize() << llendl;
		llwarns << "While sending " << mParams.getAssetID() << llendl;
		return LLTS_ERROR;
	}
	
	delete_returned = TRUE;
	U8 *tmpp = new U8[max_bytes];
	*data_handle = tmpp;
	if (!vf.read(tmpp, max_bytes))		/* Flawfinder: Ignore */
	{
		// Read failure, need to deal with it.
		delete[] tmpp;
		*data_handle = NULL;
		returned_bytes = 0;
		delete_returned = FALSE;
		return LLTS_ERROR;
	}

	returned_bytes = vf.getLastBytesRead();
	mCurPos += returned_bytes;


	if (vf.eof())
	{
		if (!returned_bytes)
		{
			delete[] tmpp;
			*data_handle = NULL;
			returned_bytes = 0;
			delete_returned = FALSE;
		}
		return LLTS_DONE;
	}

	return LLTS_OK;
}

void LLTransferSourceAsset::completionCallback(const LLTSCode status)
{
	// No matter what happens, all we want to do is close the vfile if
	// we've got it open.
}

void LLTransferSourceAsset::packParams(LLDataPacker& dp) const
{
	//llinfos << "LLTransferSourceAsset::packParams" << llendl;
	mParams.packParams(dp);
}

BOOL LLTransferSourceAsset::unpackParams(LLDataPacker &dp)
{
	//llinfos << "LLTransferSourceAsset::unpackParams" << llendl;
	return mParams.unpackParams(dp);
}


void LLTransferSourceAsset::responderCallback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type,
											  void *user_data, S32 result, LLExtStat ext_status )
{
	LLUUID *tidp = ((LLUUID*) user_data);
	LLUUID transfer_id = *(tidp);
	delete tidp;
	tidp = NULL;

	LLTransferSourceAsset *tsap = (LLTransferSourceAsset *)	gTransferManager.findTransferSource(transfer_id);

	if (!tsap)
	{
		llinfos << "Aborting transfer " << transfer_id << " callback, transfer source went away" << llendl;
		return;
	}

	if (result)
	{
		llinfos << "AssetStorage: Error " << gAssetStorage->getErrorString(result) << " downloading uuid " << uuid << llendl;
	}

	LLTSCode status;

	tsap->mGotResponse = TRUE;
	if (LL_ERR_NOERR == result)
	{
		// Everything's OK.
		LLVFile vf(gAssetStorage->mVFS, uuid, type, LLVFile::READ);
		tsap->mSize = vf.getSize();
		status = LLTS_OK;
	}
	else
	{
		// Uh oh, something bad happened when we tried to get this asset!
		switch (result)
		{
		case LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE:
			status = LLTS_UNKNOWN_SOURCE;
			break;
		default:
			status = LLTS_ERROR;
		}
	}

	tsap->sendTransferStatus(status);
}



LLTransferSourceParamsAsset::LLTransferSourceParamsAsset()
	: LLTransferSourceParams(LLTST_ASSET),

	  mAssetType(LLAssetType::AT_NONE)
{
}

void LLTransferSourceParamsAsset::setAsset(const LLUUID &asset_id, const LLAssetType::EType asset_type)
{
	mAssetID = asset_id;
	mAssetType = asset_type;
}

void LLTransferSourceParamsAsset::packParams(LLDataPacker &dp) const
{
	dp.packUUID(mAssetID, "AssetID");
	dp.packS32(mAssetType, "AssetType");
}


BOOL LLTransferSourceParamsAsset::unpackParams(LLDataPacker &dp)
{
	S32 tmp_at;

	dp.unpackUUID(mAssetID, "AssetID");
	dp.unpackS32(tmp_at, "AssetType");

	mAssetType = (LLAssetType::EType)tmp_at;

	return TRUE;
}

