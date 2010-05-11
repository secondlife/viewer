/**
 * @file llassetuploadresponders.h
 * @brief Processes responses received for asset upload requests.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_LLASSETUPLOADRESPONDER_H
#define LL_LLASSETUPLOADRESPONDER_H

#include "llassetstorage.h"
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

class LLNewAgentInventoryResponder : public LLAssetUploadResponder
{
public:
	LLNewAgentInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type,
								boost::function<void(const LLUUID& uuid)> callback = NULL);
	LLNewAgentInventoryResponder(const LLSD& post_data, 
								const std::string& file_name,
								 LLAssetType::EType asset_type,
								boost::function<void(const LLUUID& uuid)> callback = NULL);
	virtual void uploadComplete(const LLSD& content);

	boost::function<void(const LLUUID& uuid)> mCallback;
};

class LLBakedUploadData;
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
