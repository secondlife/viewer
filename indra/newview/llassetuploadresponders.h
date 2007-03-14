// llassetuploadresponders.h
// Copyright 2006, Linden Research, Inc.
// Processes responses received for asset upload requests.

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
	LLAssetUploadResponder(const LLSD& post_data, const std::string& file_name);
	~LLAssetUploadResponder();
    virtual void error(U32 statusNum, const std::string& reason);
	virtual void result(const LLSD& content);
	virtual void uploadUpload(const LLSD& content);
	virtual void uploadComplete(const LLSD& content);
	virtual void uploadFailure(const LLSD& content);

protected:
	LLSD mPostData;
	LLUUID mVFileID;
	LLAssetType::EType mAssetType;
	std::string mFileName;
};

class LLNewAgentInventoryResponder : public LLAssetUploadResponder
{
public:
	LLNewAgentInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type);
	LLNewAgentInventoryResponder(const LLSD& post_data, const std::string& file_name);
	virtual void uploadComplete(const LLSD& content);
};

class LLUpdateAgentInventoryResponder : public LLAssetUploadResponder
{
public:
	LLUpdateAgentInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type);
	LLUpdateAgentInventoryResponder(const LLSD& post_data,
								const std::string& file_name);
	virtual void uploadComplete(const LLSD& content);
};

class LLUpdateTaskInventoryResponder : public LLAssetUploadResponder
{
public:
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const LLUUID& vfile_id,
								LLAssetType::EType asset_type);
	LLUpdateTaskInventoryResponder(const LLSD& post_data,
								const std::string& file_name);
	virtual void uploadComplete(const LLSD& content);
};

#endif // LL_LLASSETUPLOADRESPONDER_H
