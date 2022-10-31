/** 
 * @file lltransfertargetvfile.cpp
 * @brief Transfer system for receiving a vfile.
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

#include "lltransfertargetvfile.h"

#include "lldatapacker.h"
#include "llerror.h"
#include "llfilesystem.h"

//static
void LLTransferTargetVFile::updateQueue(bool shutdown)
{
}


LLTransferTargetParamsVFile::LLTransferTargetParamsVFile() :
    LLTransferTargetParams(LLTTT_VFILE),
    mAssetType(LLAssetType::AT_NONE),
    mCompleteCallback(NULL),
    mRequestDatap(NULL),
    mErrCode(0)
{
}

void LLTransferTargetParamsVFile::setAsset(
    const LLUUID& asset_id,
    LLAssetType::EType asset_type)
{
    mAssetID = asset_id;
    mAssetType = asset_type;
}

void LLTransferTargetParamsVFile::setCallback(LLTTVFCompleteCallback cb, LLBaseDownloadRequest& request)
{
    mCompleteCallback = cb;
    if (mRequestDatap)
    {
        delete mRequestDatap;
    }
    mRequestDatap = request.getCopy();
}

bool LLTransferTargetParamsVFile::unpackParams(LLDataPacker& dp)
{
    // if the source provided a new key, assign that to the asset id.
    if(dp.hasNext())
    {
        LLUUID dummy_id;
        dp.unpackUUID(dummy_id, "AgentID");
        dp.unpackUUID(dummy_id, "SessionID");
        dp.unpackUUID(dummy_id, "OwnerID");
        dp.unpackUUID(dummy_id, "TaskID");
        dp.unpackUUID(dummy_id, "ItemID");
        dp.unpackUUID(mAssetID, "AssetID");
        S32 dummy_type;
        dp.unpackS32(dummy_type, "AssetType");
    }

    // if we never got an asset id, this will always fail.
    if(mAssetID.isNull())
    {
        return false;
    }
    return true;
}


LLTransferTargetVFile::LLTransferTargetVFile(
    const LLUUID& uuid,
    LLTransferSourceType src_type) :
    LLTransferTarget(LLTTT_VFILE, uuid, src_type),
    mNeedsCreate(TRUE)
{
    mTempID.generate();
}


LLTransferTargetVFile::~LLTransferTargetVFile()
{
    if (mParams.mRequestDatap)
    {
        // TODO: Consider doing it in LLTransferTargetParamsVFile's destructor
        delete mParams.mRequestDatap;
        mParams.mRequestDatap = NULL;
    }
}


// virtual
bool LLTransferTargetVFile::unpackParams(LLDataPacker& dp)
{
    if(LLTST_SIM_INV_ITEM == mSourceType)
    {
        return mParams.unpackParams(dp);
    }
    return true;
}

void LLTransferTargetVFile::applyParams(const LLTransferTargetParams &params)
{
    if (params.getType() != mType)
    {
        LL_WARNS() << "Target parameter type doesn't match!" << LL_ENDL;
        return;
    }
    
    mParams = (LLTransferTargetParamsVFile &)params;
}


LLTSCode LLTransferTargetVFile::dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size)
{
    //LL_INFOS() << "LLTransferTargetFile::dataCallback" << LL_ENDL;
    //LL_INFOS() << "Packet: " << packet_id << LL_ENDL;

    LLFileSystem vf(mTempID, mParams.getAssetType(), LLFileSystem::APPEND);
    if (mNeedsCreate)
    {
        mNeedsCreate = FALSE;
    }

    if (!in_size)
    {
        return LLTS_OK;
    }

    if (!vf.write(in_datap, in_size))
    {
        LL_WARNS() << "Failure in LLTransferTargetVFile::dataCallback!" << LL_ENDL;
        return LLTS_ERROR;
    }
    return LLTS_OK;
}


void LLTransferTargetVFile::completionCallback(const LLTSCode status)
{
    //LL_INFOS() << "LLTransferTargetVFile::completionCallback" << LL_ENDL;

    if (!gAssetStorage)
    {
        LL_WARNS() << "Aborting vfile transfer after asset storage shut down!" << LL_ENDL;
        return;
    }
    
    // Still need to gracefully handle error conditions.
    S32 err_code = 0;
    switch (status)
    {
      case LLTS_DONE:
        if (!mNeedsCreate)
        {
            LLFileSystem file(mTempID, mParams.getAssetType(), LLFileSystem::WRITE);
            if (!file.rename(mParams.getAssetID(), mParams.getAssetType()))
            {
                LL_ERRS() << "LLTransferTargetVFile: rename failed" << LL_ENDL;
            }
        }
        err_code = LL_ERR_NOERR;
        LL_DEBUGS() << "LLTransferTargetVFile::completionCallback for "
             << mParams.getAssetID() << ","
             << LLAssetType::lookup(mParams.getAssetType())
             << " with temp id " << mTempID << LL_ENDL;
        break;
      case LLTS_ERROR:
      case LLTS_ABORT:
      case LLTS_UNKNOWN_SOURCE:
      default:
      {
          // We're aborting this transfer, we don't want to keep this file.
          LL_WARNS() << "Aborting vfile transfer for " << mParams.getAssetID() << LL_ENDL;
          LLFileSystem vf(mTempID, mParams.getAssetType(), LLFileSystem::APPEND);
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

    if (mParams.mRequestDatap)
    {
        if (mParams.mCompleteCallback)
        {
            mParams.mCompleteCallback(err_code,
                mParams.getAssetID(),
                mParams.getAssetType(),
                mParams.mRequestDatap,
                LLExtStat::NONE);
        }
        delete mParams.mRequestDatap;
        mParams.mRequestDatap = NULL;
    }
}
