/** 
 * @file lltransfersourcefile.h
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

#ifndef LL_LLTRANSFERSOURCEFILE_H
#define LL_LLTRANSFERSOURCEFILE_H

#include "lltransfermanager.h"

class LLTransferSourceParamsFile : public LLTransferSourceParams
{
public:
    LLTransferSourceParamsFile();
    virtual ~LLTransferSourceParamsFile() {}
    /*virtual*/ void packParams(LLDataPacker &dp) const;
    /*virtual*/ BOOL unpackParams(LLDataPacker &dp);

    void setFilename(const std::string &filename)       { mFilename = filename; }
    std::string getFilename() const                 { return mFilename; }

    void setDeleteOnCompletion(BOOL enabled)        { mDeleteOnCompletion = enabled; }
    BOOL getDeleteOnCompletion()                    { return mDeleteOnCompletion; }
protected:
    std::string mFilename;
    // ONLY DELETE THINGS OFF THE SIM IF THE FILENAME BEGINS IN 'TEMP'
    BOOL mDeleteOnCompletion;
};

class LLTransferSourceFile : public LLTransferSource
{
public:
    LLTransferSourceFile(const LLUUID &transfer_id, const F32 priority);
    virtual ~LLTransferSourceFile();

protected:
    /*virtual*/ void initTransfer();
    /*virtual*/ F32 updatePriority();
    /*virtual*/ LLTSCode dataCallback(const S32 packet_id,
                                      const S32 max_bytes,
                                      U8 **datap,
                                      S32 &returned_bytes,
                                      BOOL &delete_returned);
    /*virtual*/ void completionCallback(const LLTSCode status);

    virtual void packParams(LLDataPacker& dp) const;
    /*virtual*/ BOOL unpackParams(LLDataPacker &dp);

protected:
    LLTransferSourceParamsFile mParams;
    LLFILE *mFP;
};

#endif // LL_LLTRANSFERSOURCEFILE_H
