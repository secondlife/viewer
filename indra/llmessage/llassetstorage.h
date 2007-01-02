/** 
 * @file llassetstorage.h
 * @brief definition of LLAssetStorage class which allows simple
 * up/downloads of uuid,type asets
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLASSETSTORAGE_H
#define LL_LLASSETSTORAGE_H

#include <string>

#include "lluuid.h"
#include "lltimer.h"
#include "llnamevalue.h"
#include "llhost.h"
#include "stdenums.h" 	// for EDragAndDropType
#include "lltransfermanager.h" // For LLTSCode enum
#include "llassettype.h"
#include "llstring.h"

// Forward declarations
class LLMessageSystem;
class LLXferManager;
class LLAssetStorage;
class LLVFS;
class LLSD;

class LLAssetInfo
{
protected:
	std::string		mDescription;
	std::string		mName;

public:
	LLUUID			mUuid;
	LLTransactionID	mTransactionID;
	LLUUID			mCreatorID;
	LLAssetType::EType	mType;

	LLAssetInfo( void );
	LLAssetInfo( const LLUUID& object_id, const LLUUID& creator_id,
				 LLAssetType::EType type, const char* name, const char* desc );
	LLAssetInfo( const LLNameValue& nv );
	
	const std::string& getName( void ) const { return mName; }
	const std::string& getDescription( void ) const { return mDescription; } 
	void setName( const std::string& name );
	void setDescription( const std::string& desc );

	// Assets (aka potential inventory items) can be applied to an
	// object in the world. We'll store that as a string name value
	// pair where the name encodes part of asset info, and the value
	// the rest.  LLAssetInfo objects will be responsible for parsing
	// the meaning out froman LLNameValue object. See the inventory
	// design docs for details.
	void setFromNameValue( const LLNameValue& nv );
};


class LLAssetRequest
{
public:
	LLAssetRequest(const LLUUID &uuid, const LLAssetType::EType at);
	virtual ~LLAssetRequest();
	
	LLUUID getUUID() const					{ return mUUID; }
	LLAssetType::EType getType() const		{ return mType; }
protected:
	LLUUID	mUUID;
	LLAssetType::EType mType;

public:
	void	(*mDownCallback)(LLVFS*, const LLUUID&, LLAssetType::EType, void *, S32);
	void	(*mUpCallback)(const LLUUID&, void *, S32);
	void	(*mInfoCallback)(LLAssetInfo *, void *, S32);

	void	*mUserData;
	LLHost  mHost;
	BOOL	mIsTemp;
	BOOL	mIsLocal;
	F64		mTime;				// Message system time
	BOOL    mIsPriority;
	BOOL	mDataSentInFirstPacket;
	BOOL	mDataIsInVFS;
	LLUUID	mRequestingAgentID;	// Only valid for uploads from an agent
};


class LLInvItemRequest
{
public:
	LLInvItemRequest(const LLUUID &uuid, const LLAssetType::EType at);
	virtual ~LLInvItemRequest();

	LLUUID getUUID() const					{ return mUUID; }
	LLAssetType::EType getType() const		{ return mType; }
protected:
	LLUUID	mUUID;
	LLAssetType::EType mType;

public:
	void	(*mDownCallback)(LLVFS*, const LLUUID&, LLAssetType::EType, void *, S32);

	void	*mUserData;
	LLHost  mHost;
	BOOL	mIsTemp;
	F64		mTime;				// Message system time
	BOOL    mIsPriority;
	BOOL	mDataSentInFirstPacket;
	BOOL	mDataIsInVFS;

};

class LLEstateAssetRequest
{
public:
	LLEstateAssetRequest(const LLUUID &uuid, const LLAssetType::EType at, EstateAssetType et);
	virtual ~LLEstateAssetRequest();

	LLUUID getUUID() const					{ return mUUID; }
	LLAssetType::EType getAType() const		{ return mAType; }
protected:
	LLUUID	mUUID;
	LLAssetType::EType mAType;
	EstateAssetType mEstateAssetType;

public:
	void	(*mDownCallback)(LLVFS*, const LLUUID&, LLAssetType::EType, void *, S32);

	void	*mUserData;
	LLHost  mHost;
	BOOL	mIsTemp;
	F64		mTime;				// Message system time
	BOOL    mIsPriority;
	BOOL	mDataSentInFirstPacket;
	BOOL	mDataIsInVFS;

};




typedef void (*LLGetAssetCallback)(LLVFS *vfs, const LLUUID &asset_id,
										 LLAssetType::EType asset_type, void *user_data, S32 status);

class LLAssetStorage
{
public:
	// VFS member is public because static child methods need it :(
	LLVFS *mVFS;
	typedef void (*LLStoreAssetCallback)(const LLUUID &asset_id, void *user_data, S32 status);

protected:
	BOOL mShutDown;
	LLHost mUpstreamHost;
	
	LLMessageSystem *mMessageSys;
	LLXferManager	*mXferManager;

	std::list<LLAssetRequest*> mPendingDownloads;
	std::list<LLAssetRequest*> mPendingUploads;
	std::list<LLAssetRequest*> mPendingLocalUploads;
	
public:
	LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, const LLHost &upstream_host);

	LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs);
	virtual ~LLAssetStorage();

	void setUpstream(const LLHost &upstream_host);

	virtual BOOL hasLocalAsset(const LLUUID &uuid, LLAssetType::EType type);

	// public interface methods
	// note that your callback may get called BEFORE the function returns

	virtual void getAssetData(const LLUUID uuid, LLAssetType::EType atype, LLGetAssetCallback cb, void *user_data, BOOL is_priority = FALSE);

	/*
	 * TransactionID version
	 * Viewer needs the store_local
	 */
	virtual void storeAssetData(
		const LLTransactionID& tid,
		LLAssetType::EType atype,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool store_local = false);

	/*
	 * AssetID version
	 * Sim needs both store_local and requesting_agent_id.
	 */
	virtual	void storeAssetData(
		const LLUUID& asset_id,
		LLAssetType::EType asset_type,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool store_local = false,
		const LLUUID& requesting_agent_id = LLUUID::null);

	// This call will attempt to clear a store asset. This will only
	// attempt to cancel an upload that has not yet begun. The
	// callback will be called with an error code.
	virtual void cancelStoreAsset(
		const LLUUID& uuid,
		LLAssetType::EType oatype); 

	virtual void checkForTimeouts();

	void getEstateAsset(const LLHost &object_sim, const LLUUID &agent_id, const LLUUID &session_id,
									const LLUUID &asset_id, LLAssetType::EType atype, EstateAssetType etype,
									 LLGetAssetCallback callback, void *user_data, BOOL is_priority);

	void getInvItemAsset(const LLHost &object_sim,
						 const LLUUID &agent_id, const LLUUID &session_id,
						 const LLUUID &owner_id, const LLUUID &task_id, const LLUUID &item_id,
						 const LLUUID &asset_id, LLAssetType::EType atype,
						 LLGetAssetCallback cb, void *user_data, BOOL is_priority = FALSE); // Get a particular inventory item.


	S32 getNumPendingDownloads() const;
	S32 getNumPendingUploads() const;
	S32 getNumPendingLocalUploads();

	// Returns a map from type to num pending, eg 'texture' => 5, 'object' => 10
	LLSD getPendingDownloadTypes() const;
	LLSD getPendingUploadTypes() const;

	// download process callbacks
	static void downloadCompleteCallback(const S32 result, void *user_data);
	static void downloadEstateAssetCompleteCallback(const S32 result, void *user_data);
	static void downloadInvItemCompleteCallback(const S32 result, void *user_data);

	// upload process callbacks
	static void uploadCompleteCallback(const LLUUID&, void *user_data, S32 result);
	static void processUploadComplete(LLMessageSystem *msg, void **this_handle);

	// debugging
	static const char* getErrorString( S32 status );

	// deprecated file-based methods
	void getAssetData(const LLUUID uuid, LLAssetType::EType type, void (*callback)(const char*, const LLUUID&, void *, S32), void *user_data, BOOL is_priority = FALSE);

	/*
	 * AssetID version.
	 */
	virtual void storeAssetData(
		const char* filename,
		const LLUUID& asset_id,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false);

	/*
	 * TransactionID version
	 */
	virtual void storeAssetData(
		const char * filename,
		const LLTransactionID &transaction_id,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void *user_data,
		bool temp_file = false,
		bool is_priority = false);

	static void legacyGetDataCallback(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType, void *user_data, S32 status);
	static void legacyStoreDataCallback(const LLUUID &uuid, void *user_data, S32 status);

	// Temp assets are stored on sim nodes, they have agent ID and location data associated with them.
	// This is a no-op for non-http asset systems
	virtual void addTempAssetData(const LLUUID& asset_id, const LLUUID& agent_id, const std::string& host_name);
	virtual BOOL hasTempAssetData(const LLUUID& texture_id) const;
	virtual std::string getTempAssetHostName(const LLUUID& texture_id) const;
	virtual LLUUID getTempAssetAgentID(const LLUUID& texture_id) const;
	virtual void removeTempAssetData(const LLUUID& asset_id);
	virtual void removeTempAssetDataByAgentID(const LLUUID& agent_id);
	// Pass LLUUID::null for all
	virtual void dumpTempAssetData(const LLUUID& avatar_id) const;
	virtual void clearTempAssetData();

	// add extra methods to handle metadata

protected:
	void _cleanupRequests(BOOL all, S32 error);
	void _callUploadCallbacks(const LLUUID &uuid, const LLAssetType::EType asset_type, BOOL success);

	virtual void _queueDataRequest(const LLUUID& uuid, LLAssetType::EType type,
								   void (*callback)(LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32),
								   void *user_data, BOOL duplicate,
								   BOOL is_priority);

private:
	void _init(LLMessageSystem *msg,
			   LLXferManager *xfer,
			   LLVFS *vfs,
			   const LLHost &upstream_host);
	LLSD getPendingTypes(const std::list<LLAssetRequest*>& requests) const;

};

////////////////////////////////////////////////////////////////////////
// Wrappers to replicate deprecated API
////////////////////////////////////////////////////////////////////////

class LLLegacyAssetRequest
{
public:
	void	(*mDownCallback)(const char *, const LLUUID&, void *, S32);
	LLAssetStorage::LLStoreAssetCallback mUpCallback;

	void	*mUserData;
};

extern LLAssetStorage *gAssetStorage;
extern const LLUUID CATEGORIZE_LOST_AND_FOUND_ID;
#endif	
