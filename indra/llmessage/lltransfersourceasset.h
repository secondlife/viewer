/** 
 * @file lltransfersourceasset.h
 * @brief Transfer system for sending an asset.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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

#endif // LL_LLTRANSFERSOURCEASSET_H
