/** 
 * @file lltransfertargetvfile.h
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

#ifndef LL_LLTRANSFERTARGETVFILE_H
#define LL_LLTRANSFERTARGETVFILE_H

#include "lltransfermanager.h"
#include "llassetstorage.h"
#include "llfilesystem.h"

class LLFileSystem;

// Lame, an S32 for now until I figure out the deal with how we want to do
// error codes.
typedef void (*LLTTVFCompleteCallback)(
	S32 status,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	LLBaseDownloadRequest* user_data, LLExtStat ext_status );

class LLTransferTargetParamsVFile : public LLTransferTargetParams
{
public:
	LLTransferTargetParamsVFile();

	void setAsset(const LLUUID& asset_id, LLAssetType::EType asset_type);
	void setCallback(LLTTVFCompleteCallback cb, LLBaseDownloadRequest& request);

	LLUUID getAssetID() const						{ return mAssetID; }
	LLAssetType::EType getAssetType() const			{ return mAssetType; }

	friend class LLTransferTargetVFile;
protected:
	bool unpackParams(LLDataPacker& dp);

	LLUUID				mAssetID;
	LLAssetType::EType	mAssetType;

	LLTTVFCompleteCallback	mCompleteCallback;
	LLBaseDownloadRequest*	mRequestDatap;
	S32						mErrCode;
};


class LLTransferTargetVFile : public LLTransferTarget
{
public:
	LLTransferTargetVFile(const LLUUID& uuid, LLTransferSourceType src_type);
	virtual ~LLTransferTargetVFile();

	//static void requestTransfer(LLTransferTargetChannel* channelp,
	//							const char* local_filename,
	//							const LLTransferSourceParams& source_params,
	//							LLTTVFCompleteCallback callback);

	static void updateQueue(bool shutdown = false);
	
protected:
	virtual bool unpackParams(LLDataPacker& dp);
	/*virtual*/ void applyParams(const LLTransferTargetParams& params);
	/*virtual*/ LLTSCode dataCallback(const S32 packet_id, U8* in_datap, const S32 in_size);
	/*virtual*/ void completionCallback(const LLTSCode status);

	LLTransferTargetParamsVFile mParams;

	bool mNeedsCreate;
	LLUUID mTempID;
};

#endif // LL_LLTRANSFERTARGETFILE_H
