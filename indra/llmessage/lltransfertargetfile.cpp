/** 
 * @file lltransfertargetfile.cpp
 * @brief Transfer system for receiving a file.
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
        LL_ERRS() << "LLTransferTargetFile::~LLTransferTargetFile - Should have been cleaned up in completion callback" << LL_ENDL;
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
        LL_WARNS() << "Target parameter type doesn't match!" << LL_ENDL;
        return;
    }
    
    mParams = (LLTransferTargetParamsFile &)params;
}

LLTSCode LLTransferTargetFile::dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size)
{
    //LL_INFOS() << "LLTransferTargetFile::dataCallback" << LL_ENDL;
    //LL_INFOS() << "Packet: " << packet_id << LL_ENDL;

    if (!mFP)
    {
        mFP = LLFile::fopen(mParams.mFilename, "wb");       /* Flawfinder: ignore */        

        if (!mFP)
        {
            LL_WARNS() << "Failure opening " << mParams.mFilename << " for write by LLTransferTargetFile" << LL_ENDL;
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
        LL_WARNS() << "Failure in LLTransferTargetFile::dataCallback!" << LL_ENDL;
        return LLTS_ERROR;
    }
    return LLTS_OK;
}

void LLTransferTargetFile::completionCallback(const LLTSCode status)
{
    LL_INFOS() << "LLTransferTargetFile::completionCallback" << LL_ENDL;
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
        LL_WARNS() << "Aborting file transfer for " << mParams.mFilename << LL_ENDL;
        if (mFP)
        {
            // Only need to remove file if we successfully opened it.
            LLFile::remove(mParams.mFilename);
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
