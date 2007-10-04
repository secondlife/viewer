/** 
 * @file lltransfersourcefile.cpp
 * @brief Transfer system for sending a file.
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

#include "lltransfersourcefile.h"

#include "llerror.h"
#include "message.h"
#include "lldatapacker.h"
#include "lldir.h"

LLTransferSourceFile::LLTransferSourceFile(const LLUUID &request_id, const F32 priority) :
	LLTransferSource(LLTST_FILE, request_id, priority),
	mFP(NULL)
{
}

LLTransferSourceFile::~LLTransferSourceFile()
{
	if (mFP)
	{
		llerrs << "Destructor called without the completion callback being called!" << llendl;
	}
}

void LLTransferSourceFile::initTransfer()
{
	std::string filename = mParams.getFilename();
	std::string delimiter = gDirUtilp->getDirDelimiter();

	if((filename == ".")
	   || (filename == "..")
	   || (filename.find(delimiter[0]) != std::string::npos))
	{
		llwarns << "Attempting to transfer file " << filename << " with path delimiter, aborting!" << llendl;

		sendTransferStatus(LLTS_ERROR);
		return;
	}
	// Look for the file.
	mFP = LLFile::fopen(mParams.getFilename().c_str(), "rb");		/* Flawfinder: ignore */
	if (!mFP)
	{
		sendTransferStatus(LLTS_ERROR);
		return;
	}

	// Get the size of the file using the hack from
	fseek(mFP,0,SEEK_END);
	mSize = ftell(mFP);
	fseek(mFP,0,SEEK_SET);

	sendTransferStatus(LLTS_OK);
}

F32 LLTransferSourceFile::updatePriority()
{
	return 0.f;
}

LLTSCode LLTransferSourceFile::dataCallback(const S32 packet_id,
											const S32 max_bytes,
											U8 **data_handle,
											S32 &returned_bytes,
											BOOL &delete_returned)
{
	//llinfos << "LLTransferSourceFile::dataCallback" << llendl;

	if (!mFP)
	{
		llerrs << "Data callback without file set!" << llendl;
		return LLTS_ERROR;
	}

	if (packet_id != mLastPacketID + 1)
	{
		llerrs << "Can't handle out of order file transfer yet!" << llendl;
	}

	// Grab up until the max number of bytes from the file.
	delete_returned = TRUE;
	U8 *tmpp = new U8[max_bytes];
	*data_handle = tmpp;
	returned_bytes = (S32)fread(tmpp, 1, max_bytes, mFP);
	if (!returned_bytes)
	{
		delete[] tmpp;
		*data_handle = NULL;
		returned_bytes = 0;
		delete_returned = FALSE;
		return LLTS_DONE;
	}

	return LLTS_OK;
}

void LLTransferSourceFile::completionCallback(const LLTSCode status)
{
	// No matter what happens, all we want to do is close the file pointer if
	// we've got it open.
	if (mFP)
	{
		fclose(mFP);
		mFP = NULL;

	}
	// Delete the file iff the filename begins with "TEMP"
	if (mParams.getDeleteOnCompletion() && memcmp(mParams.getFilename().c_str(), "TEMP", 4) == 0)
	{
		LLFile::remove(mParams.getFilename().c_str());
	}
}

void LLTransferSourceFile::packParams(LLDataPacker& dp) const
{
	//llinfos << "LLTransferSourceFile::packParams" << llendl;
	mParams.packParams(dp);
}

BOOL LLTransferSourceFile::unpackParams(LLDataPacker &dp)
{
	//llinfos << "LLTransferSourceFile::unpackParams" << llendl;
	return mParams.unpackParams(dp);
}


LLTransferSourceParamsFile::LLTransferSourceParamsFile() : LLTransferSourceParams(LLTST_FILE)
{
}


void LLTransferSourceParamsFile::packParams(LLDataPacker &dp) const
{
	dp.packString(mFilename.c_str(), "Filename");
	dp.packU8((U8)mDeleteOnCompletion, "Delete");
}


BOOL LLTransferSourceParamsFile::unpackParams(LLDataPacker &dp)
{
	dp.unpackString(mFilename, "Filename");
	U8 delete_flag;
	dp.unpackU8(delete_flag, "Delete");
	mDeleteOnCompletion = delete_flag;

	llinfos << "Unpacked filename: " << mFilename << llendl;
	return TRUE;
}
