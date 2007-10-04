/** 
 * @file lltransfertargetfile.cpp
 * @brief Transfer system for receiving a file.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

#include "lltransfertargetfile.h"
#include "llerror.h"




LLTransferTargetFile::LLTransferTargetFile(
	const LLUUID& uuid,
	LLTransferSourceType src_type) :
	LLTransferTarget(LLTTT_FILE, uuid, src_type),
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

// virtual
bool LLTransferTargetFile::unpackParams(LLDataPacker& dp)
{
	// we can safely ignore this call
	return true;
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
