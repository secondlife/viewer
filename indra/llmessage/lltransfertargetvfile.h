/** 
 * @file lltransfertargetvfile.h
 * @brief Transfer system for receiving a vfile.
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
	void* user_data, LLExtStat ext_status );

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
