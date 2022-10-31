/** 
 * @file lltransfersourcefile.cpp
 * @brief Transfer system for sending a file.
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
        LL_ERRS() << "Destructor called without the completion callback being called!" << LL_ENDL;
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
        LL_WARNS() << "Attempting to transfer file " << filename << " with path delimiter, aborting!" << LL_ENDL;

        sendTransferStatus(LLTS_ERROR);
        return;
    }
    // Look for the file.
    mFP = LLFile::fopen(mParams.getFilename(), "rb");       /* Flawfinder: ignore */
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
    //LL_INFOS() << "LLTransferSourceFile::dataCallback" << LL_ENDL;

    if (!mFP)
    {
        LL_ERRS() << "Data callback without file set!" << LL_ENDL;
        return LLTS_ERROR;
    }

    if (packet_id != mLastPacketID + 1)
    {
        LL_ERRS() << "Can't handle out of order file transfer yet!" << LL_ENDL;
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
    if (mParams.getDeleteOnCompletion() && mParams.getFilename().substr(0, 4) == "TEMP")
    {
        LLFile::remove(mParams.getFilename());
    }
}

void LLTransferSourceFile::packParams(LLDataPacker& dp) const
{
    //LL_INFOS() << "LLTransferSourceFile::packParams" << LL_ENDL;
    mParams.packParams(dp);
}

BOOL LLTransferSourceFile::unpackParams(LLDataPacker &dp)
{
    //LL_INFOS() << "LLTransferSourceFile::unpackParams" << LL_ENDL;
    return mParams.unpackParams(dp);
}


LLTransferSourceParamsFile::LLTransferSourceParamsFile() :
    LLTransferSourceParams(LLTST_FILE),
    mDeleteOnCompletion(FALSE)
{
}


void LLTransferSourceParamsFile::packParams(LLDataPacker &dp) const
{
    dp.packString(mFilename, "Filename");
    dp.packU8((U8)mDeleteOnCompletion, "Delete");
}


BOOL LLTransferSourceParamsFile::unpackParams(LLDataPacker &dp)
{
    dp.unpackString(mFilename, "Filename");
    U8 delete_flag;
    dp.unpackU8(delete_flag, "Delete");
    mDeleteOnCompletion = delete_flag;

    LL_INFOS() << "Unpacked filename: " << mFilename << LL_ENDL;
    return TRUE;
}
