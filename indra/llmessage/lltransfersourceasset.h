/** 
 * @file lltransfersourceasset.h
 * @brief Transfer system for sending an asset.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTRANSFERSOURCEASSET_H
#define LL_LLTRANSFERSOURCEASSET_H

#include "lltransfermanager.h"
#include "llassetstorage.h"

class LLVFile;

class LLTransferSourceParamsAsset : public LLTransferSourceParams
{
public:
	LLTransferSourceParamsAsset();
	virtual ~LLTransferSourceParamsAsset() {}
	/*virtual*/ void packParams(LLDataPacker &dp) const;
	/*virtual*/ BOOL unpackParams(LLDataPacker &dp);

	void setAsset(const LLUUID &asset_id, const LLAssetType::EType asset_type);

	LLUUID getAssetID() const						{ return mAssetID; }
	LLAssetType::EType getAssetType() const			{ return mAssetType; }

protected:
	LLUUID				mAssetID;
	LLAssetType::EType	mAssetType;
};

class LLTransferSourceAsset : public LLTransferSource
{
public:
	LLTransferSourceAsset(const LLUUID &request_id, const F32 priority);
	virtual ~LLTransferSourceAsset();

	static void responderCallback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type,
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

/**
 * @brief Quick check to see if the asset allows direct download.
 *
 * This might not be the right place for this function call, but it
 * originally started life inside the LLTransferSourceAsset code.
 * @param type The type of asset.
 * @return Returns true if the asset can be fetched by id.
 */
bool is_asset_fetch_by_id_allowed(LLAssetType::EType type);

/**
 * @brief Quick check to see if all asset data can be known by the viewer.
 *
 * This might not be the right place for this function call, but it
 * originally started life inside the LLTransferSourceAsset code.
 * @param type The type of asset.
 * @return Returns true if the asset id can be transmitted to the viewer.
 */
bool is_asset_id_knowable(LLAssetType::EType type);

#endif // LL_LLTRANSFERSOURCEASSET_H
