/** 
 * @file llxfer_vfile.h
 * @brief definition of LLXfer_VFile class for a single xfer_vfile.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLXFER_VFILE_H
#define LL_LLXFER_VFILE_H

#include "llxfer.h"
#include "llassetstorage.h"

class LLFileSystem;

class LLXfer_VFile : public LLXfer
{
 protected:
    LLUUID mLocalID;
    LLUUID mRemoteID;
    LLUUID mTempID;
    LLAssetType::EType mType;
    
    LLFileSystem *mVFile;

    std::string mName;

    BOOL    mDeleteTempFile;

 public:
    LLXfer_VFile ();
    LLXfer_VFile (const LLUUID &local_id, LLAssetType::EType type);
    virtual ~LLXfer_VFile();

    virtual void init(const LLUUID &local_id, LLAssetType::EType type);
    virtual void cleanup();

    virtual S32 initializeRequest(U64 xfer_id,
            const LLUUID &local_id,
            const LLUUID &remote_id,
            const LLAssetType::EType type,
            const LLHost &remote_host,
            void (*callback)(void **,S32,LLExtStat),
            void **user_data);
    virtual S32 startDownload();

    virtual S32 processEOF();

    virtual S32 startSend(U64 xfer_id, const LLHost &remote_host);
    virtual void closeFileHandle();
    virtual S32 reopenFileHandle();

    virtual S32 suck(S32 start_position);
    virtual S32 flush();

    virtual BOOL matchesLocalFile(const LLUUID &id, LLAssetType::EType type);
    virtual BOOL matchesRemoteFile(const LLUUID &id, LLAssetType::EType type);

    virtual void setXferSize(S32 xfer_size);
    virtual S32  getMaxBufferSize();

    virtual U32 getXferTypeTag();

    virtual std::string getFileName();
};

#endif





