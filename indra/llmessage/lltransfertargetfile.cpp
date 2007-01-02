/** 
 * @file lltransfertargetfile.cpp
 * @brief Transfer system for receiving a file.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lltransfertargetfile.h"
#include "llerror.h"




LLTransferTargetFile::LLTransferTargetFile(const LLUUID &uuid) :
	LLTransferTarget(LLTTT_FILE, uuid),
	mFP(NULL)
{
}

LLTransferTargetFile::~LLTransferTargetFile()
{
	if (mFP)
	{
		llerrs << "LLTransferTargetFile::~LLTransferTargetFile - Should have been cleaned up in completion callback" << llendl;
		fclose(mFP);
		mFP = NULL;
	}
}

void LLTransferTargetFile::applyParams(const LLTransferTargetParams &params)
{
	if (params.getType() != mType)
	{
		llwarns << "Target parameter type doesn't match!" << llendl;
		return;
	}
	
	mParams = (LLTransferTargetParamsFile &)params;
}

LLTSCode LLTransferTargetFile::dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size)
{
	//llinfos << "LLTransferTargetFile::dataCallback" << llendl;
	//llinfos << "Packet: " << packet_id << llendl;

	if (!mFP)
	{
		mFP = LLFile::fopen(mParams.mFilename.c_str(), "wb");		/* Flawfinder: ignore */		

		if (!mFP)
		{
			llwarns << "Failure opening " << mParams.mFilename << " for write by LLTransferTargetFile" << llendl;
			return LLTS_ERROR;
		}
	}
	if (!in_size)
	{
		return LLTS_OK;
	}

	S32 count = (S32)fwrite(in_datap, 1, in_size, mFP);
	if (count != in_size)
	{
		llwarns << "Failure in LLTransferTargetFile::dataCallback!" << llendl;
		return LLTS_ERROR;
	}
	return LLTS_OK;
}

void LLTransferTargetFile::completionCallback(const LLTSCode status)
{
	llinfos << "LLTransferTargetFile::completionCallback" << llendl;
	if (mFP)
	{
		fclose(mFP);
	}

	// Still need to gracefully handle error conditions.
	switch (status)
	{
	case LLTS_DONE:
		break;
	case LLTS_ABORT:
	case LLTS_ERROR:
		// We're aborting this transfer, we don't want to keep this file.
		llwarns << "Aborting file transfer for " << mParams.mFilename << llendl;
		if (mFP)
		{
			// Only need to remove file if we successfully opened it.
			LLFile::remove(mParams.mFilename.c_str());
		}
	default:
		break;
	}

	mFP = NULL;
	if (mParams.mCompleteCallback)
	{
		mParams.mCompleteCallback(status, mParams.mUserData);
	}
}
