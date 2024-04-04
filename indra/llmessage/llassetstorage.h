/** 
 * @file llassetstorage.h
 * @brief definition of LLAssetStorage class which allows simple
 * up/downloads of uuid,type asets
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLASSETSTORAGE_H
#define LL_LLASSETSTORAGE_H
#include <string>
#include <functional>

#include "lluuid.h"
#include "lltimer.h"
#include "llnamevalue.h"
#include "llhost.h"
#include "lltransfermanager.h" // For LLTSCode enum
#include "llassettype.h"
#include "llstring.h"
#include "llextendedstatus.h"
#include "llxfer.h"

// Forward declarations
class LLMessageSystem;
class LLXferManager;
class LLAssetStorage;
class LLSD;

// anything that takes longer than this to download will abort.
// HTTP Uploads also timeout if they take longer than this.
const F32Minutes LL_ASSET_STORAGE_TIMEOUT(5);


// Specific error codes
const int LL_ERR_ASSET_REQUEST_FAILED = -1;
//const int LL_ERR_ASSET_REQUEST_INVALID = -2;
const int LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE = -3;
const int LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE = -4;
const int LL_ERR_INSUFFICIENT_PERMISSIONS = -5;
const int LL_ERR_PRICE_MISMATCH = -23018;

// *TODO: these typedefs are passed into the cache via a legacy C function pointer
// future project would be to convert these to C++ callables (std::function<>) so that 
// we can use bind and remove the userData parameter.
// 
typedef std::function<void(const LLUUID &asset_id, LLAssetType::EType asset_type, void *user_data, S32 status, LLExtStat ext_status)> LLGetAssetCallback;
typedef std::function<void(const LLUUID &asset_id, void *user_data, S32 status, LLExtStat ext_status)> LLStoreAssetCallback;


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


class LLBaseDownloadRequest
{
public:
    LLBaseDownloadRequest(const LLUUID &uuid, const LLAssetType::EType at);
    virtual ~LLBaseDownloadRequest();

    LLUUID getUUID() const					{ return mUUID; }
    LLAssetType::EType getType() const		{ return mType; }

    void setUUID(const LLUUID& id) { mUUID = id; }
    void setType(LLAssetType::EType type) { mType = type; }

    virtual LLBaseDownloadRequest* getCopy();

protected:
    LLUUID	mUUID;
    LLAssetType::EType mType;

public:
    LLGetAssetCallback mDownCallback;

    void	*mUserData;
    LLHost  mHost;
    bool	mIsTemp;
    F64Seconds		mTime;				// Message system time
    bool    mIsPriority;
    bool	mDataSentInFirstPacket;
    bool	mDataIsInCache;
};

class LLAssetRequest : public LLBaseDownloadRequest
{
public:
    LLAssetRequest(const LLUUID &uuid, const LLAssetType::EType at);
    virtual ~LLAssetRequest();

    void setTimeout(F64Seconds timeout) { mTimeout = timeout; }

    virtual LLBaseDownloadRequest* getCopy();

    LLStoreAssetCallback mUpCallback;
//	void	(*mUpCallback)(const LLUUID&, void *, S32, LLExtStat);
	void	(*mInfoCallback)(LLAssetInfo *, void *, S32);

	bool	mIsLocal;
	bool	mIsUserWaiting;		// We don't want to try forever if a user is waiting for a result.
	F64Seconds		mTimeout;			// Amount of time before timing out.
	LLUUID	mRequestingAgentID;	// Only valid for uploads from an agent
    F64	mBytesFetched;

	virtual LLSD getTerseDetails() const;
	virtual LLSD getFullDetails() const;
};

template <class T>
struct ll_asset_request_equal : public std::equal_to<T>
{
	bool operator()(const T& x, const T& y) const 
	{ 
		return (	x->getType() == y->getType()
				&&	x->getUUID() == y->getUUID() );
	}
};


class LLInvItemRequest : public LLBaseDownloadRequest
{
public:
    LLInvItemRequest(const LLUUID &uuid, const LLAssetType::EType at);
    virtual ~LLInvItemRequest();

    virtual LLBaseDownloadRequest* getCopy();
};

class LLEstateAssetRequest : public LLBaseDownloadRequest
{
public:
    LLEstateAssetRequest(const LLUUID &uuid, const LLAssetType::EType at, EstateAssetType et);
    virtual ~LLEstateAssetRequest();

    LLAssetType::EType getAType() const		{ return mType; }

    virtual LLBaseDownloadRequest* getCopy();

protected:
	EstateAssetType mEstateAssetType;
};


// Map of known bad assets
typedef std::map<LLUUID,U64,lluuid_less> toxic_asset_map_t;



class LLAssetStorage
{
public:
    typedef ::LLStoreAssetCallback LLStoreAssetCallback;
    typedef ::LLGetAssetCallback LLGetAssetCallback;

	enum ERequestType
	{
		RT_INVALID = -1,
		RT_DOWNLOAD = 0,
		RT_UPLOAD = 1,
		RT_LOCALUPLOAD = 2,
		RT_COUNT = 3
	};

protected:
	bool mShutDown;
	LLHost mUpstreamHost;
	
	LLMessageSystem *mMessageSys;
	LLXferManager	*mXferManager;


	typedef std::list<LLAssetRequest*> request_list_t;
	request_list_t mPendingDownloads;
	request_list_t mPendingUploads;
	request_list_t mPendingLocalUploads;
	
	// Map of toxic assets - these caused problems when recently rezzed, so avoid them
	toxic_asset_map_t	mToxicAssetMap;		// Objects in this list are known to cause problems and are not loaded

public:
	LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer, const LLHost &upstream_host);

	LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer);
	virtual ~LLAssetStorage();

	void setUpstream(const LLHost &upstream_host);

	bool hasLocalAsset(const LLUUID &uuid, LLAssetType::EType type);

	// public interface methods
	// note that your callback may get called BEFORE the function returns
	void getAssetData(const LLUUID uuid, LLAssetType::EType atype, LLGetAssetCallback cb, void *user_data, bool is_priority = false);

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
		bool store_local = false,
		bool user_waiting= false,
		F64Seconds timeout=LL_ASSET_STORAGE_TIMEOUT) = 0;

    virtual void logAssetStorageInfo() = 0;
    
	virtual void checkForTimeouts();

	void getEstateAsset(const LLHost &object_sim, const LLUUID &agent_id, const LLUUID &session_id,
									const LLUUID &asset_id, LLAssetType::EType atype, EstateAssetType etype,
									 LLGetAssetCallback callback, void *user_data, bool is_priority);

	void getInvItemAsset(const LLHost &object_sim,
						 const LLUUID &agent_id, const LLUUID &session_id,
						 const LLUUID &owner_id, const LLUUID &task_id, const LLUUID &item_id,
						 const LLUUID &asset_id, LLAssetType::EType atype,
						 LLGetAssetCallback cb, void *user_data, bool is_priority = false); // Get a particular inventory item.

	// Check if an asset is in the toxic map.  If it is, the entry is updated
	bool		isAssetToxic( const LLUUID& uuid );

	// Clean the toxic asset list, remove old entries
	void		flushOldToxicAssets( bool force_it );

	// Add an item to the toxic asset map
	void		markAssetToxic( const LLUUID& uuid );

protected:
	bool findInCacheAndInvokeCallback(const LLUUID& uuid, LLAssetType::EType type,
										  LLGetAssetCallback callback, void *user_data);

	LLSD getPendingDetailsImpl(const request_list_t* requests,
                               LLAssetType::EType asset_type,
                               const std::string& detail_prefix) const;

	LLSD getPendingRequestImpl(const request_list_t* requests,
                               LLAssetType::EType asset_type,
                               const LLUUID& asset_id) const;

	bool deletePendingRequestImpl(request_list_t* requests,
                                  LLAssetType::EType asset_type,
                                  const LLUUID& asset_id);

public:
	static const LLAssetRequest* findRequest(const request_list_t* requests,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id);
	static LLAssetRequest* findRequest(request_list_t* requests,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id);

	request_list_t* getRequestList(ERequestType rt);
	const request_list_t* getRequestList(ERequestType rt) const;
	static std::string getRequestName(ERequestType rt);

	S32 getNumPendingDownloads() const;
	S32 getNumPendingUploads() const;
	S32 getNumPendingLocalUploads();
	S32 getNumPending(ERequestType rt) const;

	LLSD getPendingDetails(ERequestType rt,
                           LLAssetType::EType asset_type,
                           const std::string& detail_prefix) const;

	LLSD getPendingRequest(ERequestType rt,
                           LLAssetType::EType asset_type,
                           const LLUUID& asset_id) const;

	bool deletePendingRequest(ERequestType rt,
                              LLAssetType::EType asset_type,
                              const LLUUID& asset_id);


    static void removeAndCallbackPendingDownloads(const LLUUID& file_id, LLAssetType::EType file_type,
                                                  const LLUUID& callback_id, LLAssetType::EType callback_type,
                                                  S32 result_code, LLExtStat ext_status);
    
	// download process callbacks
	static void downloadCompleteCallback(
		S32 result,
		const LLUUID& file_id,
		LLAssetType::EType file_type,
		LLBaseDownloadRequest* user_data, LLExtStat ext_status);
	static void downloadEstateAssetCompleteCallback(
		S32 result,
		const LLUUID& file_id,
		LLAssetType::EType file_type,
		LLBaseDownloadRequest* user_data, LLExtStat ext_status);
	static void downloadInvItemCompleteCallback(
		S32 result,
		const LLUUID& file_id,
		LLAssetType::EType file_type,
		LLBaseDownloadRequest* user_data, LLExtStat ext_status);

	// upload process callbacks
	static void uploadCompleteCallback(const LLUUID&, void *user_data, S32 result, LLExtStat ext_status);
	static void processUploadComplete(LLMessageSystem *msg, void **this_handle);

	// debugging
	static const char* getErrorString( S32 status );

	// deprecated file-based methods
    // Not overriden
	void getAssetData(const LLUUID uuid, LLAssetType::EType type, void (*callback)(const char*, const LLUUID&, void *, S32, LLExtStat), void *user_data, bool is_priority = false);

	/*
	 * TransactionID version
	 */
	virtual void storeAssetData(
		const std::string& filename,
		const LLTransactionID &transaction_id,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void *user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool user_waiting = false,
		F64Seconds timeout  = LL_ASSET_STORAGE_TIMEOUT) = 0;

	static void legacyGetDataCallback(const LLUUID &uuid, LLAssetType::EType, void *user_data, S32 status, LLExtStat ext_status);
	static void legacyStoreDataCallback(const LLUUID &uuid, void *user_data, S32 status, LLExtStat ext_status);

	// add extra methods to handle metadata

protected:
	void _cleanupRequests(bool all, S32 error);
	void _callUploadCallbacks(const LLUUID &uuid, const LLAssetType::EType asset_type, bool success, LLExtStat ext_status);

	virtual void _queueDataRequest(const LLUUID& uuid, LLAssetType::EType type, LLGetAssetCallback callback,
								   void *user_data, bool duplicate,
								   bool is_priority) = 0;

private:
	void _init(LLMessageSystem *msg,
			   LLXferManager *xfer,
			   const LLHost &upstream_host);

protected:
	enum EMetricResult
	{
		// Static valued enums for #dw readability - please copy this
		// declaration to them on updates -- source in llassetstorage.h
		MR_INVALID			= -1,	// Makes no sense
		MR_OKAY				= 0,	// Success - no metric normally
		MR_ZERO_SIZE		= 1,	// Zero size asset
		MR_BAD_FUNCTION		= 2,	// Tried to use a virtual base (PROGRAMMER ERROR)
		MR_FILE_NONEXIST	= 3,	// Old format store call - source file does not exist
		MR_NO_FILENAME		= 4,	// Old format store call - source filename is NULL/0-length
		MR_NO_UPSTREAM		= 5,	// Upstream provider is missing
		MR_CACHE_CORRUPTION	= 6		// cache is corrupt - too-large or mismatched stated/returned sizes
	};

	static class LLMetrics *metric_recipient;

	static void reportMetric( const LLUUID& asset_id, const LLAssetType::EType asset_type, const std::string& filename,
							  const LLUUID& agent_id, S32 asset_size, EMetricResult result,
							  const char* file, const S32 line, const std::string& message ); 
public:
	static void setMetricRecipient( LLMetrics *recip )
	{
		metric_recipient = recip;
	}
};

////////////////////////////////////////////////////////////////////////
// Wrappers to replicate deprecated API
////////////////////////////////////////////////////////////////////////

class LLLegacyAssetRequest
{
public:
	void	(*mDownCallback)(const char *, const LLUUID&, void *, S32, LLExtStat);
	LLStoreAssetCallback mUpCallback;

	void	*mUserData;
};

extern LLAssetStorage *gAssetStorage;
extern const LLUUID CATEGORIZE_LOST_AND_FOUND_ID;
#endif	
