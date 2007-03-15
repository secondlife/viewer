/** 
 * @file llhttpassetstorage.h
 * @brief Class for loading asset data to/from an external source over http.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLHTTPASSETSTORAGE_H
#define LLHTTPASSETSTORAGE_H

#include "llassetstorage.h"
#include "curl/curl.h"

class LLVFile;
class LLHTTPAssetRequest;
typedef void (*progress_callback)(void* userdata);

struct LLTempAssetData;

typedef std::map<LLUUID,LLTempAssetData> uuid_tempdata_map;

class LLHTTPAssetStorage : public LLAssetStorage
{
public:
	LLHTTPAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
					   LLVFS *vfs, const LLHost &upstream_host,
					   const char *web_host,
					   const char *local_web_host,
					   const char *host_name);

	LLHTTPAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
					   LLVFS *vfs,
					   const char *web_host,
					   const char *local_web_host,
					   const char *host_name);


	virtual ~LLHTTPAssetStorage();

	virtual void storeAssetData(
		const LLUUID& uuid,
		LLAssetType::EType atype,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool store_local = false,
		const LLUUID& requesting_agent_id = LLUUID::null);

	virtual void storeAssetData(
		const char* filename,
		const LLUUID& asset_id,
		LLAssetType::EType atype,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file,
		bool is_priority);

	virtual LLSD getPendingDetails(ERequestType rt,
	 				LLAssetType::EType asset_type,
	 				const std::string& detail_prefix) const;

	virtual LLSD getPendingRequest(ERequestType rt,
							LLAssetType::EType asset_type,
							const LLUUID& asset_id) const;

	virtual bool deletePendingRequest(ERequestType rt,
							LLAssetType::EType asset_type,
							const LLUUID& asset_id);

	// Hack.  One off curl download an URL to a file.  Probably should be elsewhere.
	// Only used by lldynamicstate.  The API is broken, and should be replaced with
	// a generic HTTP file fetch - Doug 9/25/06
	S32 getURLToFile(const LLUUID& uuid, LLAssetType::EType asset_type, const LLString &url, const char *filename, progress_callback callback, void *userdata);
	
	LLAssetRequest* findNextRequest(request_list_t& pending, request_list_t& running);

	void checkForTimeouts();
	
	static size_t curlDownCallback(void *data, size_t size, size_t nmemb, void *user_data);
	static size_t curlFileDownCallback(void *data, size_t size, size_t nmemb, void *user_data);
	static size_t curlUpCallback(void *data, size_t size, size_t nmemb, void *user_data);
	static size_t nullOutputCallback(void *data, size_t size, size_t nmemb, void *user_data);

	// Should only be used by the LLHTTPAssetRequest
	void addRunningRequest(ERequestType rt, LLHTTPAssetRequest* request);
	void removeRunningRequest(ERequestType rt, LLHTTPAssetRequest* request);

	request_list_t* getRunningList(ERequestType rt);
	const request_list_t* getRunningList(ERequestType rt) const;

	// Temp assets are stored on sim nodes, they have agent ID and location data associated with them.
	virtual void addTempAssetData(const LLUUID& asset_id, const LLUUID& agent_id, const std::string& host_name);
	virtual BOOL hasTempAssetData(const LLUUID& texture_id) const;
	virtual std::string getTempAssetHostName(const LLUUID& texture_id) const;
	virtual LLUUID getTempAssetAgentID(const LLUUID& texture_id) const;
	virtual void removeTempAssetData(const LLUUID& asset_id);
	virtual void removeTempAssetDataByAgentID(const LLUUID& agent_id);

	// Pass LLUUID::null for all
	virtual void dumpTempAssetData(const LLUUID& avatar_id) const;
	virtual void clearTempAssetData();

protected:
	void _queueDataRequest(const LLUUID& uuid, LLAssetType::EType type,
						   void (*callback)(LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32),
						   void *user_data, BOOL duplicate, BOOL is_priority);

private:
	void _init(const char *web_host, const char *local_web_host, const char* host_name);

	// This will return the correct base URI for any http asset request
	std::string getBaseURL(const LLUUID& asset_id, LLAssetType::EType asset_type);

protected:
	std::string		mBaseURL;
	std::string		mLocalBaseURL;
	std::string		mHostName;

	CURLM  *mCurlMultiHandle;

	request_list_t mRunningDownloads;
	request_list_t mRunningUploads;
	request_list_t mRunningLocalUploads;

	uuid_tempdata_map mTempAssets;
};

#endif
