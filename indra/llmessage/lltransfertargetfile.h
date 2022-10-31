/** 
 * @file lltransfertargetfile.h
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

#ifndef LL_LLTRANSFERTARGETFILE_H
#define LL_LLTRANSFERTARGETFILE_H

#include "lltransfermanager.h"

typedef void (*LLTTFCompleteCallback)(const LLTSCode status, void *user_data);

class LLTransferTargetParamsFile : public LLTransferTargetParams
{
public:
    LLTransferTargetParamsFile()
        : LLTransferTargetParams(LLTTT_FILE),

        mCompleteCallback(NULL),
        mUserData(NULL)
    {}
    void setFilename(const std::string& filename)   { mFilename = filename; }
    void setCallback(LLTTFCompleteCallback cb, void *user_data)     { mCompleteCallback = cb; mUserData = user_data; }

    friend class LLTransferTargetFile;
protected:
    std::string             mFilename;
    LLTTFCompleteCallback   mCompleteCallback;
    void *                  mUserData;
};


class LLTransferTargetFile : public LLTransferTarget
{
public:
    LLTransferTargetFile(const LLUUID& uuid, LLTransferSourceType src_type);
    virtual ~LLTransferTargetFile();

    static void requestTransfer(LLTransferTargetChannel *channelp,
                                const char *local_filename,
                                const LLTransferSourceParams &source_params,
                                LLTTFCompleteCallback callback);
protected:
    virtual bool unpackParams(LLDataPacker& dp);
    /*virtual*/ void applyParams(const LLTransferTargetParams &params);
    /*virtual*/ LLTSCode dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size);
    /*virtual*/ void completionCallback(const LLTSCode status);

    LLTransferTargetParamsFile mParams;

    LLFILE *mFP;
};

#endif // LL_LLTRANSFERTARGETFILE_H
