/**
 * @file llassetuploadresponders.h
 * @brief Processes responses received for asset upload requests.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLASSETUPLOADRESPONDER_H
#define LL_LLASSETUPLOADRESPONDER_H

#include "llhttpclient.h"

// Abstract class for supporting asset upload
// via capabilities
class LLAssetUploadResponder : public LLHTTPClient::Responder
{
public:
	LLAssetUploadResponder(const LLSD& post_data,
							const LLUUID& vfile_id,
							LLAssetType::EType asset_type);
	LLAssetUploadResponder(const LLSD& post_data, 
							const std::string& file_name,
							LLAssetType::EType asset_type);
	~LLAssetUploadResponder();

    virtual void error(U32 statusNum, const std::string& reason);
	virtual void result(const LLSD& content);
	virtual void uploadUpload(const LLSD& content);
	virtual void uploadComplete(const LLSD& content);
	virtual void uploadFailure(const LLSD& content);

protected:
	LLSD mPostData;
	LLAssetType::EType mAssetType;
	LLUUID mVFileID;
	std::string mFileName;
};

// TODO*: Remove this once deprecated
class LLNewAgentInventoryResponder : public LLAssetUploadResponder
{
public:
	LLNewAgentInventoryResponder(
		const LLSD& post_data,
		const LLUUID& vfile_id,
		LLAssetType::EType asset_type);
	LLNewAgentInventoryResponder(
		const LLSD& post_data,
		const std::string& file_name,
		LLAssetType::EType asset_type);
    virtual void error(U32 statusNum, const std::string& reason);
	virtual void uploadComplete(const LLSD& content);
	virtual void uploadFailure(const LLSD& content);
};

// A base class which goes through and performs some default
// actions for variable price uploads.  If more specific actions
// are needed (such as different confirmation messages, etc.)
// the functions onApplicationLevelError and showConfirmationDialog.
class LLNewAgentInventoryVariablePriceResponder :
	public LLHTTPClient::Responder
{
public:
	LLNewAgentInventoryVariablePriceResponder(
		const LLUUID& vfile_id,
		LLAssetType::EType asset_type,
		const LLSD& inventory_info);

	LLNewAgentInventoryVariablePriceResponder(
		const std::string& file_name,
		LLAssetType::EType asset_type,
		const LLSD& inventory_info);
	virtual ~LLNewAgentInventoryVariablePriceResponder();

	void errorWithContent(
		U32 statusNum,
		const std::string& reason,
		const LLSD& content);
	void result(const LLSD& content);

	virtual void onApplicationLevelError(
		const LLSD& error);
	virtual void showConfirmationDialog(
		S32 upload_price,
		S32 resource_cost,
		const std::string& confirmation_url);

private:
	class Impl;
	Impl* mImpl;
};

struct LLBakedUploadData;
class LLSendTexLayerResponder : public LLAssetUploadResponder
{
public:
	LLSendTexLayerResponder(const LLSD& post_data,
							const LLUUID& vfile_id,
							LLAssetType::EType asset_type,
							LLBakedUploadData * baked_upload_data);

	~LLSendTexLayerResponder();

	virtual void uploadComplete(const LLSD& content);
	virtual void error(U32 statusNum, const std::string& reason);

	LLBakedUploadData * mBakedUploadData;
};

class LLUpdateAgentInventoryResponder : public LLAssetUploadResponder
{
public:
	LLUpdateAgentInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type);
	LLUpdateAgentInventoryResponder(const LLSD& post_data,
								const std::string& file_name,
											   LLAssetType::EType asset_type);
	virtual void uploadComplete(const LLSD& content);
};

class LLUpdateTaskInventoryResponder : public LLAssetUploadResponder
{
public:
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type);
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const std::string& file_name,
								LLAssetType::EType asset_type);
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const std::string& file_name,
								const LLUUID& queue_id,
								LLAssetType::EType asset_type);

	virtual void uploadComplete(const LLSD& content);
	
private:
	LLUUID mQueueId;
};

#endif // LL_LLASSETUPLOADRESPONDER_H
