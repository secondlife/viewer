/** 
 * @file lltransfertargetvfile.cpp
 * @brief Transfer system for receiving a vfile.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lltransfertargetvfile.h"
#include "llerror.h"

#include "llvfile.h"

//static
std::list<LLTransferTargetParamsVFile*> LLTransferTargetVFile::sCallbackQueue;

//static
void LLTransferTargetVFile::updateQueue(bool shutdown)
{
	for(std::list<LLTransferTargetParamsVFile*>::iterator iter = sCallbackQueue.begin();
		iter != sCallbackQueue.end(); )
	{
		std::list<LLTransferTargetParamsVFile*>::iterator curiter = iter++;
		LLTransferTargetParamsVFile* params = *curiter;
		LLVFSThread::status_t s = LLVFile::getVFSThread()->getRequestStatus(params->mHandle);
		if (s == LLVFSThread::STATUS_COMPLETE || s == LLVFSThread::STATUS_EXPIRED)
		{
			params->mCompleteCallback(params->mErrCode, params->mUserDatap);
			delete params;
			iter = sCallbackQueue.erase(curiter);
		}
		else if (shutdown)
		{
			delete params;
			iter = sCallbackQueue.erase(curiter);
		}
	}
}


LLTransferTargetParamsVFile::LLTransferTargetParamsVFile() :
	LLTransferTargetParams(LLTTT_VFILE),
	mAssetType(LLAssetType::AT_NONE),
	mCompleteCallback(NULL),
	mUserDatap(NULL),
	mErrCode(0),
	mHandle(LLVFSThread::nullHandle())
{
}

void LLTransferTargetParamsVFile::setAsset(const LLUUID &asset_id, const LLAssetType::EType asset_type)
{
	mAssetID = asset_id;
	mAssetType = asset_type;
}

void LLTransferTargetParamsVFile::setCallback(LLTTVFCompleteCallback cb, void *user_data)
{
	mCompleteCallback = cb;
	mUserDatap = user_data;
}


LLTransferTargetVFile::LLTransferTargetVFile(const LLUUID &uuid) :
	LLTransferTarget(LLTTT_VFILE, uuid),
	mNeedsCreate(TRUE)
{
	mTempID.generate();
}


LLTransferTargetVFile::~LLTransferTargetVFile()
{
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
	LLVFSThread::handle_t handle = LLVFSThread::nullHandle();
	
	// Still need to gracefully handle error conditions.
	S32 err_code = 0;
	switch (status)
	{
	  case LLTS_DONE:
		if (!mNeedsCreate)
		{
			handle = LLVFile::getVFSThread()->rename(gAssetStorage->mVFS,
													 mTempID, mParams.getAssetType(),
													 mParams.getAssetID(), mParams.getAssetType(),
													 LLVFSThread::AUTO_DELETE);
		}
		err_code = LL_ERR_NOERR;
		// 		llinfos << "Successful vfile transfer for " << mParams.getAssetID() << llendl;
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
		if (handle != LLVFSThread::nullHandle())
		{
			LLTransferTargetParamsVFile* params = new LLTransferTargetParamsVFile(mParams);
			params->mErrCode = err_code;
			params->mHandle = handle;
			sCallbackQueue.push_back(params);
		}
		else
		{
			mParams.mCompleteCallback(err_code, mParams.mUserDatap);
		}
	}
}
