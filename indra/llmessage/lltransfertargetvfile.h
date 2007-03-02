/** 
 * @file lltransfertargetvfile.h
 * @brief Transfer system for receiving a vfile.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTRANSFERTARGETVFILE_H
#define LL_LLTRANSFERTARGETVFILE_H

#include "lltransfermanager.h"
#include "llassetstorage.h"
#include "llvfile.h"

class LLVFile;

// Lame, an S32 for now until I figure out the deal with how we want to do
// error codes.
typedef void (*LLTTVFCompleteCallback)(
	S32 status,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	void* user_data);

class LLTransferTargetParamsVFile : public LLTransferTargetParams
{
public:
	LLTransferTargetParamsVFile();

	void setAsset(const LLUUID& asset_id, LLAssetType::EType asset_type);
	void setCallback(LLTTVFCompleteCallback cb, void* user_data);

	LLUUID getAssetID() const						{ return mAssetID; }
	LLAssetType::EType getAssetType() const			{ return mAssetType; }

	friend class LLTransferTargetVFile;
protected:
	bool unpackParams(LLDataPacker& dp);

	LLUUID				mAssetID;
	LLAssetType::EType	mAssetType;

	LLTTVFCompleteCallback	mCompleteCallback;
	void*					mUserDatap;
	S32						mErrCode;
	LLVFSThread::handle_t	mHandle;
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

	BOOL mNeedsCreate;
	LLUUID mTempID;
};

#endif // LL_LLTRANSFERTARGETFILE_H
