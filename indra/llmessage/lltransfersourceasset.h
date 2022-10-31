/** 
 * @file lltransfersourceasset.h
 * @brief Transfer system for sending an asset.
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

#ifndef LL_LLTRANSFERSOURCEASSET_H
#define LL_LLTRANSFERSOURCEASSET_H

#include "lltransfermanager.h"
#include "llassetstorage.h"

class LLFileSystem;

class LLTransferSourceParamsAsset : public LLTransferSourceParams
{
public:
    LLTransferSourceParamsAsset();
    virtual ~LLTransferSourceParamsAsset() {}
    /*virtual*/ void packParams(LLDataPacker &dp) const;
    /*virtual*/ BOOL unpackParams(LLDataPacker &dp);

    void setAsset(const LLUUID &asset_id, const LLAssetType::EType asset_type);

    LLUUID getAssetID() const                       { return mAssetID; }
    LLAssetType::EType getAssetType() const         { return mAssetType; }

protected:
    LLUUID              mAssetID;
    LLAssetType::EType  mAssetType;
};

class LLTransferSourceAsset : public LLTransferSource
{
public:
    LLTransferSourceAsset(const LLUUID &request_id, const F32 priority);
    virtual ~LLTransferSourceAsset();

    static void responderCallback(const LLUUID& uuid, LLAssetType::EType type,
                                  void *user_data, S32 result, LLExtStat ext_status );
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
    LLTransferSourceParamsAsset mParams;
    BOOL mGotResponse;

    S32 mCurPos;
};

#endif // LL_LLTRANSFERSOURCEASSET_H
