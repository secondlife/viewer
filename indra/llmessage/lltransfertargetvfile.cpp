/** 
 * @file lltransfertargetvfile.cpp
 * @brief Transfer system for receiving a vfile.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lltransfertargetvfile.h"

#include "lldatapacker.h"
#include "llerror.h"
#include "llvfile.h"

//static
void LLTransferTargetVFile::updateQueue(bool shutdown)
{
}


LLTransferTargetParamsVFile::LLTransferTargetParamsVFile() :
	LLTransferTargetParams(LLTTT_VFILE),
	mAssetType(LLAssetType::AT_NONE),
	mCompleteCallback(NULL),
	mUserDatap(NULL),
	mErrCode(0)
{
}

void LLTransferTargetParamsVFile::setAsset(
	const LLUUID& asset_id,
	LLAssetType::EType asset_type)
{
	mAssetID = asset_id;
	mAssetType = asset_type;
}

void LLTransferTargetParamsVFile::setCallback(LLTTVFCompleteCallback cb, void *user_data)
{
	mCompleteCallback = cb;
	mUserDatap = user_data;
}

bool LLTransferTargetParamsVFile::unpackParams(LLDataPacker& dp)
{
	// if the source provided a new key, assign that to the asset id.
	if(dp.hasNext())
	{
		LLUUID dummy_id;
		dp.unpackUUID(dummy_id, "AgentID");
		dp.unpackUUID(dummy_id, "SessionID");
		dp.unpackUUID(dummy_id, "OwnerID");
		dp.unpackUUID(dummy_id, "TaskID");
		dp.unpackUUID(dummy_id, "ItemID");
		dp.unpackUUID(mAssetID, "AssetID");
		S32 dummy_type;
		dp.unpackS32(dummy_type, "AssetType");
	}

	// if we never got an asset id, this will always fail.
	if(mAssetID.isNull())
	{
		return false;
	}
	return true;
}


LLTransferTargetVFile::LLTransferTargetVFile(
	const LLUUID& uuid,
	LLTransferSourceType src_type) :
	LLTransferTarget(LLTTT_VFILE, uuid, src_type),
	mNeedsCreate(TRUE)
{
	mTempID.generate();
}


LLTransferTargetVFile::~LLTransferTargetVFile()
{
}


// virtual
bool LLTransferTargetVFile::unpackParams(LLDataPacker& dp)
{
	if(LLTST_SIM_INV_ITEM == mSourceType)
	{
		return mParams.unpackParams(dp);
	}
	return true;
}

void LLTransferTargetVFile::applyParams(const LLTransferTargetParams &params)
{
	if (params.getType() != mType)
	{
		llwarns << "Target parameter type doesn't match!" << llendl;
		return;
	}
	
	mParams = (LLTransferTargetParamsVFile &)params;
}


LLTSCode LLTransferTargetVFile::dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size)
{
	//llinfos << "LLTransferTargetFile::dataCallback" << llendl;
	//llinfos << "Packet: " << packet_id << llendl;

	LLVFile vf(gAssetStorage->mVFS, mTempID, mParams.getAssetType(), LLVFile::APPEND);
	if (mNeedsCreate)
	{
		vf.setMaxSize(mSize);
		mNeedsCreate = FALSE;
	}

	if (!in_size)
	{
		return LLTS_OK;
	}

	if (!vf.write(in_datap, in_size))
	{
		llwarns << "Failure in LLTransferTargetVFile::dataCallback!" << llendl;
		return LLTS_ERROR;
	}
	return LLTS_OK;
}


void LLTransferTargetVFile::completionCallback(const LLTSCode status)
{
	//llinfos << "LLTransferTargetVFile::completionCallback" << llendl;

	if (!gAssetStorage)
	{
		llwarns << "Aborting vfile transfer after asset storage shut down!" << llendl;
		return;
	}
	
	// Still need to gracefully handle error conditions.
	S32 err_code = 0;
	switch (status)
	{
	  case LLTS_DONE:
		if (!mNeedsCreate)
		{
			LLVFile file(gAssetStorage->mVFS, mTempID, mParams.getAssetType(), LLVFile::WRITE);
			if (!file.rename(mParams.getAssetID(), mParams.getAssetType()))
			{
				llerrs << "LLTransferTargetVFile: rename failed" << llendl;
			}
		}
		err_code = LL_ERR_NOERR;
		lldebugs << "LLTransferTargetVFile::completionCallback for "
			 << mParams.getAssetID() << ","
			 << LLAssetType::lookup(mParams.getAssetType())
			 << " with temp id " << mTempID << llendl;
		break;
	  case LLTS_ERROR:
	  case LLTS_ABORT:
	  case LLTS_UNKNOWN_SOURCE:
	  default:
	  {
		  // We're aborting this transfer, we don't want to keep this file.
		  llwarns << "Aborting vfile transfer for " << mParams.getAssetID() << llendl;
		  LLVFile vf(gAssetStorage->mVFS, mTempID, mParams.getAssetType(), LLVFile::APPEND);
		  vf.remove();
	  }
	  break;
	}

	switch (status)
	{
	case LLTS_DONE:
		err_code = LL_ERR_NOERR;
		break;
	case LLTS_UNKNOWN_SOURCE:
		err_code = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
		break;
	case LLTS_INSUFFICIENT_PERMISSIONS:
		err_code = LL_ERR_INSUFFICIENT_PERMISSIONS;
		break;
	case LLTS_ERROR:
	case LLTS_ABORT:
	default:
		err_code = LL_ERR_ASSET_REQUEST_FAILED;
		break;
	}
	if (mParams.mCompleteCallback)
	{
		mParams.mCompleteCallback(err_code,
								  mParams.getAssetID(),
								  mParams.getAssetType(),
								  mParams.mUserDatap);
	}
}
