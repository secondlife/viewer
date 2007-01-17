/** 
 * @file llassetstorage.cpp
 * @brief Implementation of the base asset storage system.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

// system library includes
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

#include "llassetstorage.h"

// linden library includes
#include "llmath.h"
#include "llstring.h"
#include "lldir.h"
#include "llsd.h"

// this library includes
#include "message.h"
#include "llxfermanager.h"
#include "llvfile.h"
#include "llvfs.h"
#include "lldbstrings.h"

#include "lltransfersourceasset.h"
#include "lltransfertargetvfile.h" // For debugging

LLAssetStorage *gAssetStorage = NULL;

const LLUUID CATEGORIZE_LOST_AND_FOUND_ID("00000000-0000-0000-0000-000000000010");

const F32 LL_ASSET_STORAGE_TIMEOUT = 300.0f;  // anything that takes longer than this will abort



///----------------------------------------------------------------------------
/// LLAssetInfo
///----------------------------------------------------------------------------

LLAssetInfo::LLAssetInfo( void )
:	mDescription(),
	mName(),
	mUuid(),
	mCreatorID(),
	mType( LLAssetType::AT_NONE )
{ }

LLAssetInfo::LLAssetInfo( const LLUUID& object_id, const LLUUID& creator_id,
						  LLAssetType::EType type, const char* name,
						  const char* desc )
:	mUuid( object_id ), 
	mCreatorID( creator_id ), 
	mType( type )
{
	setName( name );
	setDescription( desc );
}

LLAssetInfo::LLAssetInfo( const LLNameValue& nv )
{
	setFromNameValue( nv );
}

// make sure the name is short enough, and strip all pipes since they
// are reserved characters in our inventory tracking system.
void LLAssetInfo::setName( const std::string& name )
{
	if( !name.empty() )
	{
		mName.assign( name, 0, llmin((U32)name.size(), (U32)DB_INV_ITEM_NAME_STR_LEN) );
		mName.erase( std::remove(mName.begin(), mName.end(), '|'),
					 mName.end() );
	}
}

// make sure the name is short enough, and strip all pipes since they
// are reserved characters in our inventory tracking system.
void LLAssetInfo::setDescription( const std::string& desc )
{
	if( !desc.empty() )
	{
		mDescription.assign( desc, 0, llmin((U32)desc.size(),
										 (U32)DB_INV_ITEM_DESC_STR_LEN) );
		mDescription.erase( std::remove(mDescription.begin(),
										mDescription.end(), '|'),
							mDescription.end() );
	}
}

// Assets (aka potential inventory items) can be applied to an
// object in the world. We'll store that as a string name value
// pair where the name encodes part of asset info, and the value
// the rest.  LLAssetInfo objects will be responsible for parsing
// the meaning out froman LLNameValue object. See the inventory
// design docs for details. Briefly:
//   name=<inv_type>|<uuid>
//   value=<creatorid>|<name>|<description>|
void LLAssetInfo::setFromNameValue( const LLNameValue& nv )
{
	std::string str;
	std::string buf;
	std::string::size_type pos1;
	std::string::size_type pos2;

	// convert the name to useful information
	str.assign( nv.mName );
	pos1 = str.find('|');
	buf.assign( str, 0, pos1++ );
	mType = LLAssetType::lookup( buf.c_str() );
	buf.assign( str, pos1, std::string::npos );
	mUuid.set( buf.c_str() );

	// convert the value to useful information
	str.assign( nv.getAsset() );
	pos1 = str.find('|');
	buf.assign( str, 0, pos1++ );
	mCreatorID.set( buf.c_str() );
	pos2 = str.find( '|', pos1 );
	buf.assign( str, pos1, (pos2++) - pos1 );
	setName( buf.c_str() );
	buf.assign( str, pos2, std::string::npos );
	setDescription( buf.c_str() );
	llinfos << "uuid: " << mUuid << llendl;
	llinfos << "creator: " << mCreatorID << llendl;
}

///----------------------------------------------------------------------------
/// LLAssetRequest
///----------------------------------------------------------------------------

LLAssetRequest::LLAssetRequest(const LLUUID &uuid, const LLAssetType::EType type)
:	mUUID(uuid),
	mType(type),
	mDownCallback( NULL ),
	mUpCallback( NULL ),
	mInfoCallback( NULL ),
	mUserData( NULL ),
	mHost(),
	mIsTemp( FALSE ),
	mIsLocal(FALSE),
	mIsPriority(FALSE),
	mDataSentInFirstPacket(FALSE),
	mDataIsInVFS( FALSE )
{
	// Need to guarantee that this time is up to date, we may be creating a circuit even though we haven't been
	//  running a message system loop.
	mTime = LLMessageSystem::getMessageTimeSeconds(TRUE);
}

LLAssetRequest::~LLAssetRequest()
{
}


///----------------------------------------------------------------------------
/// LLInvItemRequest
///----------------------------------------------------------------------------

LLInvItemRequest::LLInvItemRequest(const LLUUID &uuid, const LLAssetType::EType type)
:	mUUID(uuid),
	mType(type),
	mDownCallback( NULL ),
	mUserData( NULL ),
	mHost(),
	mIsTemp( FALSE ),
	mIsPriority(FALSE),
	mDataSentInFirstPacket(FALSE),
	mDataIsInVFS( FALSE )
{
	// Need to guarantee that this time is up to date, we may be creating a circuit even though we haven't been
	//  running a message system loop.
	mTime = LLMessageSystem::getMessageTimeSeconds(TRUE);
}

LLInvItemRequest::~LLInvItemRequest()
{
}

///----------------------------------------------------------------------------
/// LLEstateAssetRequest
///----------------------------------------------------------------------------

LLEstateAssetRequest::LLEstateAssetRequest(const LLUUID &uuid, const LLAssetType::EType atype,
										   EstateAssetType etype)
:	mUUID(uuid),
	mAType(atype),
	mEstateAssetType(etype),
	mDownCallback( NULL ),
	mUserData( NULL ),
	mHost(),
	mIsTemp( FALSE ),
	mIsPriority(FALSE),
	mDataSentInFirstPacket(FALSE),
	mDataIsInVFS( FALSE )
{
	// Need to guarantee that this time is up to date, we may be creating a circuit even though we haven't been
	//  running a message system loop.
	mTime = LLMessageSystem::getMessageTimeSeconds(TRUE);
}

LLEstateAssetRequest::~LLEstateAssetRequest()
{
}


///----------------------------------------------------------------------------
/// LLAssetStorage
///----------------------------------------------------------------------------

// since many of these functions are called by the messaging and xfer systems,
// they are declared as static and are passed a "this" handle
// it's a C/C++ mish-mash!

// TODO: permissions on modifications - maybe don't allow at all?
// TODO: verify that failures get propogated down
// TODO: rework tempfile handling?


LLAssetStorage::LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer, LLVFS *vfs, const LLHost &upstream_host)
{
	_init(msg, xfer, vfs, upstream_host);
}


LLAssetStorage::LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
							   LLVFS *vfs)
{
	_init(msg, xfer, vfs, LLHost::invalid);
}


void LLAssetStorage::_init(LLMessageSystem *msg,
						   LLXferManager *xfer,
						   LLVFS *vfs,
						   const LLHost &upstream_host)
{
	mShutDown = FALSE;
	mMessageSys = msg;
	mXferManager = xfer;
	mVFS = vfs;

	setUpstream(upstream_host);
	msg->setHandlerFuncFast(_PREHASH_AssetUploadComplete, processUploadComplete, (void **)this);
}

LLAssetStorage::~LLAssetStorage()
{
	mShutDown = TRUE;
	
	_cleanupRequests(TRUE, LL_ERR_CIRCUIT_GONE);

	if (gMessageSystem)
	{
		// Warning!  This won't work if there's more than one asset storage.
		// unregister our callbacks with the message system
		gMessageSystem->setHandlerFuncFast(_PREHASH_AssetUploadComplete, NULL, NULL);
	}
}

void LLAssetStorage::setUpstream(const LLHost &upstream_host)
{
	llinfos << "AssetStorage: Setting upstream provider to " << upstream_host << llendl;
	
	mUpstreamHost = upstream_host;
}

void LLAssetStorage::checkForTimeouts()
{
	_cleanupRequests(FALSE, LL_ERR_TCP_TIMEOUT);
}

void LLAssetStorage::_cleanupRequests(BOOL all, S32 error)
{
	const S32 NUM_QUEUES = 3;
	F64 mt_secs = LLMessageSystem::getMessageTimeSeconds();

	std::list<LLAssetRequest*>* requests[NUM_QUEUES];
	requests[0] = &mPendingDownloads;
	requests[1] = &mPendingUploads;
	requests[2] = &mPendingLocalUploads;
	static const char* REQUEST_TYPE[NUM_QUEUES] = { "download", "upload", "localuploads"};
	
	std::list<LLAssetRequest*> timed_out;
	
	for (S32 ii = 0; ii < NUM_QUEUES; ++ii)
	{
		for (std::list<LLAssetRequest*>::iterator iter = requests[ii]->begin();
			 iter != requests[ii]->end(); )
		{
			std::list<LLAssetRequest*>::iterator curiter = iter++;
			LLAssetRequest* tmp = *curiter;
			// if all is true, we want to clean up everything
			// otherwise just check for timed out requests
			// EXCEPT for upload timeouts
			if (all 
				|| ((0 == ii)
					&& LL_ASSET_STORAGE_TIMEOUT < (mt_secs - tmp->mTime)))
			{
				llwarns << "Asset " << REQUEST_TYPE[ii] << " request "
						<< (all ? "aborted" : "timed out") << " for "
						<< tmp->getUUID() << "."
						<< LLAssetType::lookup(tmp->getType()) << llendl;

				timed_out.push_front(tmp);
				iter = requests[ii]->erase(curiter);
			}
		}
	}

	LLAssetInfo	info;
	for (std::list<LLAssetRequest*>::iterator iter = timed_out.begin();
		 iter != timed_out.end();  )
	{
		std::list<LLAssetRequest*>::iterator curiter = iter++;
		LLAssetRequest* tmp = *curiter;
		if (tmp->mUpCallback)
		{
			tmp->mUpCallback(tmp->getUUID(), tmp->mUserData, error);
		}
		if (tmp->mDownCallback)
		{
			tmp->mDownCallback(mVFS, tmp->getUUID(), tmp->getType(), tmp->mUserData, error);
		}
		if (tmp->mInfoCallback)
		{
			tmp->mInfoCallback(&info, tmp->mUserData, error);
		}
		delete tmp;
	}

}

BOOL LLAssetStorage::hasLocalAsset(const LLUUID &uuid, const LLAssetType::EType type)
{
	return mVFS->getExists(uuid, type);
}

///////////////////////////////////////////////////////////////////////////
// GET routines
///////////////////////////////////////////////////////////////////////////

// IW - uuid is passed by value to avoid side effects, please don't re-add &    
void LLAssetStorage::getAssetData(const LLUUID uuid, LLAssetType::EType type, void (*callback)(LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *,S32), void *user_data, BOOL is_priority)
{
	lldebugs << "LLAssetStorage::getAssetData() - " << uuid << "," << LLAssetType::lookup(type) << llendl;

	if (mShutDown)
	{
		return; // don't get the asset or do any callbacks, we are shutting down
	}
		
	if (uuid.isNull())
	{
		// Special case early out for NULL uuid
		if (callback)
		{
			callback(mVFS, uuid, type, user_data, LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE);
		}
		return;
	}

	BOOL exists = mVFS->getExists(uuid, type);
	LLVFile file(mVFS, uuid, type);
	U32 size = exists ? file.getSize() : 0;
	
	if (size < 1)
	{
		if (exists)
		{
			llwarns << "Asset vfile " << uuid << ":" << type << " found with bad size " << file.getSize() << ", removing" << llendl;
			file.remove();
		}
		
		BOOL duplicate = FALSE;
		
		// check to see if there's a pending download of this uuid already
		for (std::list<LLAssetRequest*>::iterator iter = mPendingDownloads.begin();
			 iter != mPendingDownloads.end(); ++iter )
		{
			LLAssetRequest  *tmp = *iter;
			if ((type == tmp->getType()) && (uuid == tmp->getUUID()))
			{
				if (callback == tmp->mDownCallback && user_data == tmp->mUserData)
				{
					// this is a duplicate from the same subsystem - throw it away
					llinfos << "Discarding duplicate request for UUID " << uuid << llendl;
					return;
				}
				else
				{
					llinfos << "Adding additional non-duplicate request for UUID " << uuid << llendl;
				}
				
				// this is a duplicate request
				// queue the request, but don't actually ask for it again
				duplicate = TRUE;
				break;
			}
		}
		
		// This can be overridden by subclasses
		_queueDataRequest(uuid, type, callback, user_data, duplicate, is_priority);	
	}
	else
	{
		// we've already got the file
		// theoretically, partial files w/o a pending request shouldn't happen
		// unless there's a weird error
		if (callback)
		{
			callback(mVFS, uuid, type, user_data, LL_ERR_NOERR);
		}
	}
}

void LLAssetStorage::_queueDataRequest(const LLUUID& uuid, LLAssetType::EType atype,
									   LLGetAssetCallback callback,
									   void *user_data, BOOL duplicate,
									   BOOL is_priority)
{
	if (mUpstreamHost.isOk())
	{
		// stash the callback info so we can find it after we get the response message
		LLAssetRequest *req = new LLAssetRequest(uuid, atype);
		req->mDownCallback = callback;
		req->mUserData = user_data;
		req->mIsPriority = is_priority;
	
		mPendingDownloads.push_back(req);
	
		if (!duplicate)
		{
			// send request message to our upstream data provider
			// Create a new asset transfer.
			LLTransferSourceParamsAsset spa;
			spa.setAsset(uuid, atype);

			// Set our destination file, and the completion callback.
			LLTransferTargetParamsVFile tpvf;
			tpvf.setAsset(uuid, atype);
			tpvf.setCallback(downloadCompleteCallback, req);

			llinfos << "Starting transfer for " << uuid << llendl;
			LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(mUpstreamHost, LLTCT_ASSET);
			ttcp->requestTransfer(spa, tpvf, 100.f + (is_priority ? 1.f : 0.f));
		}
	}
	else
	{
		// uh-oh, we shouldn't have gotten here
		llwarns << "Attempt to move asset data request upstream w/o valid upstream provider" << llendl;
		if (callback)
		{
			callback(mVFS, uuid, atype, user_data, LL_ERR_CIRCUIT_GONE);
		}
	}
}


void LLAssetStorage::downloadCompleteCallback(
	S32 result,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	void* user_data)
{
	lldebugs << "LLAssetStorage::downloadCompleteCallback() for " << file_id
		 << "," << LLAssetType::lookup(file_type) << llendl;
	LLAssetRequest* req = (LLAssetRequest*)user_data;
	if(!req)
	{
		llwarns << "LLAssetStorage::downloadCompleteCallback called without"
			"a valid request." << llendl;
		return;
	}
	if (!gAssetStorage)
	{
		llwarns << "LLAssetStorage::downloadCompleteCallback called without any asset system, aborting!" << llendl;
		return;
	}

	req->setUUID(file_id);
	req->setType(file_type);
	if (LL_ERR_NOERR == result)
	{
		// we might have gotten a zero-size file
		LLVFile vfile(gAssetStorage->mVFS, req->getUUID(), req->getType());
		if (vfile.getSize() <= 0)
		{
			llwarns << "downloadCompleteCallback has non-existent or zero-size asset " << req->getUUID() << llendl;
			
			result = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
			vfile.remove();
		}
	}
	
	// find and callback ALL pending requests for this UUID
	// SJB: We process the callbacks in reverse order, I do not know if this is important,
	//      but I didn't want to mess with it.
	std::list<LLAssetRequest*> requests;
	for (std::list<LLAssetRequest*>::iterator iter = gAssetStorage->mPendingDownloads.begin();
		 iter != gAssetStorage->mPendingDownloads.end();  )
	{
		std::list<LLAssetRequest*>::iterator curiter = iter++;
		LLAssetRequest* tmp = *curiter;
		if ((tmp->getUUID() == req->getUUID()) && (tmp->getType()== req->getType()))
		{
			requests.push_front(tmp);
			iter = gAssetStorage->mPendingDownloads.erase(curiter);
		}
	}
	for (std::list<LLAssetRequest*>::iterator iter = requests.begin();
		 iter != requests.end();  )
	{
		std::list<LLAssetRequest*>::iterator curiter = iter++;
		LLAssetRequest* tmp = *curiter;
		if (tmp->mDownCallback)
		{
			tmp->mDownCallback(gAssetStorage->mVFS, req->getUUID(), req->getType(), tmp->mUserData, result);
		}
		delete tmp;
	}
}

void LLAssetStorage::getEstateAsset(const LLHost &object_sim, const LLUUID &agent_id, const LLUUID &session_id,
									const LLUUID &asset_id, LLAssetType::EType atype, EstateAssetType etype,
									 LLGetAssetCallback callback, void *user_data, BOOL is_priority)
{
	lldebugs << "LLAssetStorage::getEstateAsset() - " << asset_id << "," << LLAssetType::lookup(atype) << ", estatetype " << etype << llendl;

	//
	// Probably will get rid of this early out?
	//
	if (asset_id.isNull())
	{
		// Special case early out for NULL uuid
		if (callback)
		{
			callback(mVFS, asset_id, atype, user_data, LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE);
		}
		return;
	}

	BOOL exists = mVFS->getExists(asset_id, atype);
	LLVFile file(mVFS, asset_id, atype);
	U32 size = exists ? file.getSize() : 0;

	if (size < 1)
	{
		if (exists)
		{
			llwarns << "Asset vfile " << asset_id << ":" << atype << " found with bad size " << file.getSize() << ", removing" << llendl;
			file.remove();
		}

		// See whether we should talk to the object's originating sim, or the upstream provider.
		LLHost source_host;
		if (object_sim.isOk())
		{
			source_host = object_sim;
		}
		else
		{
			source_host = mUpstreamHost;
		}
		if (source_host.isOk())
		{
			// stash the callback info so we can find it after we get the response message
			LLEstateAssetRequest *req = new LLEstateAssetRequest(asset_id, atype, etype);
			req->mDownCallback = callback;
			req->mUserData = user_data;
			req->mIsPriority = is_priority;

			// send request message to our upstream data provider
			// Create a new asset transfer.
			LLTransferSourceParamsEstate spe;
			spe.setAgentSession(agent_id, session_id);
			spe.setEstateAssetType(etype);

			// Set our destination file, and the completion callback.
			LLTransferTargetParamsVFile tpvf;
			tpvf.setAsset(asset_id, atype);
			tpvf.setCallback(downloadEstateAssetCompleteCallback, req);

			llinfos << "Starting transfer for " << asset_id << llendl;
			LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(source_host, LLTCT_ASSET);
			ttcp->requestTransfer(spe, tpvf, 100.f + (is_priority ? 1.f : 0.f));
		}
		else
		{
			// uh-oh, we shouldn't have gotten here
			llwarns << "Attempt to move asset data request upstream w/o valid upstream provider" << llendl;
			if (callback)
			{
				callback(mVFS, asset_id, atype, user_data, LL_ERR_CIRCUIT_GONE);
			}
		}
	}
	else
	{
		// we've already got the file
		// theoretically, partial files w/o a pending request shouldn't happen
		// unless there's a weird error
		if (callback)
		{
			callback(mVFS, asset_id, atype, user_data, LL_ERR_NOERR);
		}
	}
}

void LLAssetStorage::downloadEstateAssetCompleteCallback(
	S32 result,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	void* user_data)
{
	LLEstateAssetRequest *req = (LLEstateAssetRequest*)user_data;
	if(!req)
	{
		llwarns << "LLAssetStorage::downloadEstateAssetCompleteCallback called"
			" without a valid request." << llendl;
		return;
	}
	if (!gAssetStorage)
	{
		llwarns << "LLAssetStorage::downloadEstateAssetCompleteCallback called"
			" without any asset system, aborting!" << llendl;
		return;
	}

	req->setUUID(file_id);
	req->setType(file_type);
	if (LL_ERR_NOERR == result)
	{
		// we might have gotten a zero-size file
		LLVFile vfile(gAssetStorage->mVFS, req->getUUID(), req->getAType());
		if (vfile.getSize() <= 0)
		{
			llwarns << "downloadCompleteCallback has non-existent or zero-size asset!" << llendl;

			result = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
			vfile.remove();
		}
	}

	req->mDownCallback(gAssetStorage->mVFS, req->getUUID(), req->getAType(), req->mUserData, result);
}

void LLAssetStorage::getInvItemAsset(const LLHost &object_sim, const LLUUID &agent_id, const LLUUID &session_id,
									 const LLUUID &owner_id, const LLUUID &task_id, const LLUUID &item_id,
									 const LLUUID &asset_id, LLAssetType::EType atype,
									 LLGetAssetCallback callback, void *user_data, BOOL is_priority)
{
	lldebugs << "LLAssetStorage::getInvItemAsset() - " << asset_id << "," << LLAssetType::lookup(atype) << llendl;

	//
	// Probably will get rid of this early out?
	//
	//if (asset_id.isNull())
	//{
	//	// Special case early out for NULL uuid
	//	if (callback)
	//	{
	//		callback(mVFS, asset_id, atype, user_data, LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE);
	//	}
	//	return;
	//}

	bool exists = false; 
	U32 size = 0;

	if(asset_id.notNull())
	{
		exists = mVFS->getExists(asset_id, atype);
		LLVFile file(mVFS, asset_id, atype);
		size = exists ? file.getSize() : 0;
		if(exists && size < 1)
		{
			llwarns << "Asset vfile " << asset_id << ":" << atype << " found with bad size " << file.getSize() << ", removing" << llendl;
			file.remove();
		}

	}

	if (size < 1)
	{
		// See whether we should talk to the object's originating sim,
		// or the upstream provider.
		LLHost source_host;
		if (object_sim.isOk())
		{
			source_host = object_sim;
		}
		else
		{
			source_host = mUpstreamHost;
		}
		if (source_host.isOk())
		{
			// stash the callback info so we can find it after we get the response message
			LLInvItemRequest *req = new LLInvItemRequest(asset_id, atype);
			req->mDownCallback = callback;
			req->mUserData = user_data;
			req->mIsPriority = is_priority;

			// send request message to our upstream data provider
			// Create a new asset transfer.
			LLTransferSourceParamsInvItem spi;
			spi.setAgentSession(agent_id, session_id);
			spi.setInvItem(owner_id, task_id, item_id);
			spi.setAsset(asset_id, atype);

			// Set our destination file, and the completion callback.
			LLTransferTargetParamsVFile tpvf;
			tpvf.setAsset(asset_id, atype);
			tpvf.setCallback(downloadInvItemCompleteCallback, req);

			llinfos << "Starting transfer for inventory asset "
				<< item_id << " owned by " << owner_id << "," << task_id
				<< llendl;
			LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(source_host, LLTCT_ASSET);
			ttcp->requestTransfer(spi, tpvf, 100.f + (is_priority ? 1.f : 0.f));
		}
		else
		{
			// uh-oh, we shouldn't have gotten here
			llwarns << "Attempt to move asset data request upstream w/o valid upstream provider" << llendl;
			if (callback)
			{
				callback(mVFS, asset_id, atype, user_data, LL_ERR_CIRCUIT_GONE);
			}
		}
	}
	else
	{
		// we've already got the file
		// theoretically, partial files w/o a pending request shouldn't happen
		// unless there's a weird error
		if (callback)
		{
			callback(mVFS, asset_id, atype, user_data, LL_ERR_NOERR);
		}
	}
}


void LLAssetStorage::downloadInvItemCompleteCallback(
	S32 result,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	void* user_data)
{
	LLInvItemRequest *req = (LLInvItemRequest*)user_data;
	if(!req)
	{
		llwarns << "LLAssetStorage::downloadEstateAssetCompleteCallback called"
			" without a valid request." << llendl;
		return;
	}
	if (!gAssetStorage)
	{
		llwarns << "LLAssetStorage::downloadCompleteCallback called without any asset system, aborting!" << llendl;
		return;
	}

	req->setUUID(file_id);
	req->setType(file_type);
	if (LL_ERR_NOERR == result)
	{
		// we might have gotten a zero-size file
		LLVFile vfile(gAssetStorage->mVFS, req->getUUID(), req->getType());
		if (vfile.getSize() <= 0)
		{
			llwarns << "downloadCompleteCallback has non-existent or zero-size asset!" << llendl;

			result = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
			vfile.remove();
		}
	}

	req->mDownCallback(gAssetStorage->mVFS, req->getUUID(), req->getType(), req->mUserData, result);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Store routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// virtual
void LLAssetStorage::cancelStoreAsset(
	const LLUUID& uuid,
	LLAssetType::EType atype)
{
	bool do_callback = true;
	LLAssetRequest* req = NULL;

	if(mPendingUploads.size() > 0)
	{
		req = mPendingUploads.front();
		if((req->getUUID() == uuid) && (req->getType() == atype))
		{
			// This is probably because the request is in progress - do
			// not attempt to cancel.
			do_callback = false;
		}
	}

	if (mPendingLocalUploads.size() > 0)
		{
		req = mPendingLocalUploads.front();
		if((req->getUUID() == uuid) && (req->getType() == atype))
		{
			// This is probably because the request is in progress - do
			// not attempt to cancel.
			do_callback = false;
		}		
	}

	if (do_callback)
	{
			// clear it out of the upload queue if it is there.
			_callUploadCallbacks(uuid, atype, FALSE);
	}
}

// static
void LLAssetStorage::uploadCompleteCallback(const LLUUID& uuid, void *user_data, S32 result) // StoreAssetData callback (fixed)
{
	if (!gAssetStorage)
	{
		llwarns << "LLAssetStorage::uploadCompleteCallback has no gAssetStorage!" << llendl;
		return;
	}
	LLAssetRequest	*req	 = (LLAssetRequest *)user_data;
	BOOL			success  = TRUE;

	if (result)
	{
		llwarns << "LLAssetStorage::uploadCompleteCallback " << result << ":" << getErrorString(result) << " trying to upload file to upstream provider" << llendl;
		success = FALSE;
	}

	// we're done grabbing the file, tell the client
	gAssetStorage->mMessageSys->newMessageFast(_PREHASH_AssetUploadComplete);
	gAssetStorage->mMessageSys->nextBlockFast(_PREHASH_AssetBlock);
	gAssetStorage->mMessageSys->addUUIDFast(_PREHASH_UUID, uuid);
	gAssetStorage->mMessageSys->addS8Fast(_PREHASH_Type, req->getType());
	gAssetStorage->mMessageSys->addBOOLFast(_PREHASH_Success, success);
	gAssetStorage->mMessageSys->sendReliable(req->mHost);

	delete req;
}

void LLAssetStorage::processUploadComplete(LLMessageSystem *msg, void **user_data)
{
	LLAssetStorage	*this_ptr = (LLAssetStorage *)user_data;
	LLUUID			uuid;
	S8				asset_type_s8;
	LLAssetType::EType asset_type;
	BOOL			success = FALSE;

	msg->getUUIDFast(_PREHASH_AssetBlock, _PREHASH_UUID, uuid);
	msg->getS8Fast(_PREHASH_AssetBlock, _PREHASH_Type, asset_type_s8);
	msg->getBOOLFast(_PREHASH_AssetBlock, _PREHASH_Success, success);

	asset_type = (LLAssetType::EType)asset_type_s8;
	this_ptr->_callUploadCallbacks(uuid, asset_type, success);
}

void LLAssetStorage::_callUploadCallbacks(const LLUUID &uuid, LLAssetType::EType asset_type, BOOL success)
{
	// SJB: We process the callbacks in reverse order, I do not know if this is important,
	//      but I didn't want to mess with it.
	std::list<LLAssetRequest*> requests;
	for (std::list<LLAssetRequest*>::iterator iter = mPendingUploads.begin();
		 iter != mPendingUploads.end();  )
	{
		std::list<LLAssetRequest*>::iterator curiter = iter++;
		LLAssetRequest* req = *curiter;
		if ((req->getUUID() == uuid) && (req->getType() == asset_type))
		{
			requests.push_front(req);
			iter = mPendingUploads.erase(curiter);
		}
	}
	for (std::list<LLAssetRequest*>::iterator iter = mPendingLocalUploads.begin();
		 iter != mPendingLocalUploads.end();  )
	{
		std::list<LLAssetRequest*>::iterator curiter = iter++;
		LLAssetRequest* req = *curiter;
		if ((req->getUUID() == uuid) && (req->getType() == asset_type))
		{
			requests.push_front(req);
			iter = mPendingLocalUploads.erase(curiter);
		}
	}
	for (std::list<LLAssetRequest*>::iterator iter = requests.begin();
		 iter != requests.end();  )
	{
		std::list<LLAssetRequest*>::iterator curiter = iter++;
		LLAssetRequest* req = *curiter;
		if (req->mUpCallback)
		{
			req->mUpCallback(uuid, req->mUserData, (success ?  LL_ERR_NOERR :  LL_ERR_ASSET_REQUEST_FAILED ));
		}
		delete req;
	}
}


S32 LLAssetStorage::getNumPendingDownloads() const
{
	return mPendingDownloads.size();
}

S32 LLAssetStorage::getNumPendingUploads() const
{
	return mPendingUploads.size();
}

S32 LLAssetStorage::getNumPendingLocalUploads()
{
	return mPendingLocalUploads.size();
}

LLSD LLAssetStorage::getPendingTypes(const std::list<LLAssetRequest*>& requests) const
{
	LLSD type_counts;
	std::list<LLAssetRequest*>::const_iterator it = requests.begin();
	std::list<LLAssetRequest*>::const_iterator end = requests.end();
	for ( ; it != end; ++it)
	{
		LLAssetRequest* req = *it;

		const char* type_name = LLAssetType::lookupHumanReadable(req->getType());
		type_counts[type_name] = type_counts[type_name].asInteger() + 1;
	}
	return type_counts;
}

LLSD LLAssetStorage::getPendingDownloadTypes() const
{
	return getPendingTypes(mPendingDownloads);
}

LLSD LLAssetStorage::getPendingUploadTypes() const
{
	return getPendingTypes(mPendingUploads);
}

// static
const char* LLAssetStorage::getErrorString(S32 status)
{
	switch( status )
	{
	case LL_ERR_NOERR:
		return "No error";

	case LL_ERR_ASSET_REQUEST_FAILED:
		return "Asset request: failed";

	case LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE:
		return "Asset request: non-existent file";

	case LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE:
		return "Asset request: asset not found in database";

	case LL_ERR_EOF:
		return "End of file";

	case LL_ERR_CANNOT_OPEN_FILE:
		return "Cannot open file";

	case LL_ERR_FILE_NOT_FOUND:
		return "File not found";

	case LL_ERR_TCP_TIMEOUT:
		return "File transfer timeout";

	case LL_ERR_CIRCUIT_GONE:
		return "Circuit gone";

	default:
		return "Unknown status";
	}
}



void LLAssetStorage::getAssetData(const LLUUID uuid, LLAssetType::EType type, void (*callback)(const char*, const LLUUID&, void *, S32), void *user_data, BOOL is_priority)
{
	// check for duplicates here, since we're about to fool the normal duplicate checker
	for (std::list<LLAssetRequest*>::iterator iter = mPendingDownloads.begin();
		 iter != mPendingDownloads.end();  )
	{
		LLAssetRequest* tmp = *iter++;
		if (type == tmp->getType() && 
			uuid == tmp->getUUID() &&
			legacyGetDataCallback == tmp->mDownCallback &&
			callback == ((LLLegacyAssetRequest *)tmp->mUserData)->mDownCallback &&
			user_data == ((LLLegacyAssetRequest *)tmp->mUserData)->mUserData)
		{
			// this is a duplicate from the same subsystem - throw it away
			llinfos << "Discarding duplicate request for UUID " << uuid << llendl;
			return;
		}
	}
	
	
	LLLegacyAssetRequest *legacy = new LLLegacyAssetRequest;

	legacy->mDownCallback = callback;
	legacy->mUserData = user_data;

	getAssetData(uuid, type, legacyGetDataCallback, (void **)legacy,
				 is_priority);
}

// static
void LLAssetStorage::legacyGetDataCallback(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, void *user_data, S32 status)
{
	LLLegacyAssetRequest *legacy = (LLLegacyAssetRequest *)user_data;
	char filename[LL_MAX_PATH] = "";	/* Flawfinder: ignore */ 

	if (! status)
	{
		LLVFile file(vfs, uuid, type);

		char uuid_str[UUID_STR_LENGTH];	/* Flawfinder: ignore */ 

		uuid.toString(uuid_str);
		snprintf(filename,sizeof(filename),"%s.%s",gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str).c_str(),LLAssetType::lookup(type));	/* Flawfinder: ignore */ 

		FILE *fp = LLFile::fopen(filename, "wb");	/* Flawfinder: ignore */ 
		if (fp)
		{
			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while (file.read(copy_buf, buf_size))
			{
				if (fwrite(copy_buf, file.getLastBytesRead(), 1, fp) < 1)
				{
					// return a bad file error if we can't write the whole thing
					status = LL_ERR_CANNOT_OPEN_FILE;
				}
			}

			fclose(fp);
		}
		else
		{
			status = LL_ERR_CANNOT_OPEN_FILE;
		}
	}

	legacy->mDownCallback(filename, uuid, legacy->mUserData, status);
	delete legacy;
}

// this is overridden on the viewer and the sim, so it doesn't really do anything
// virtual 
void LLAssetStorage::storeAssetData(
	const LLTransactionID& tid,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority,
	bool store_local)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
}

// virtual
// this does nothing, viewer and sim both override this.
void LLAssetStorage::storeAssetData(
	const LLUUID& asset_id,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file ,
	bool is_priority,
	bool store_local,
	const LLUUID& requesting_agent_id)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
}

// virtual
// this does nothing, viewer and sim both override this.
void LLAssetStorage::storeAssetData(
	const char* filename,
	const LLUUID& asset_id,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
}

// virtual
// this does nothing, viewer and sim both override this.
void LLAssetStorage::storeAssetData(
	const char* filename,
	const LLTransactionID &transactoin_id,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
}

// static
void LLAssetStorage::legacyStoreDataCallback(const LLUUID &uuid, void *user_data, S32 status)
{
	LLLegacyAssetRequest *legacy = (LLLegacyAssetRequest *)user_data;
	if (legacy && legacy->mUpCallback)
	{
		legacy->mUpCallback(uuid, legacy->mUserData, status);
	}
	delete legacy;
}

// virtual
void LLAssetStorage::addTempAssetData(const LLUUID& asset_id, const LLUUID& agent_id, const std::string& host_name) 
{ }

// virtual
BOOL LLAssetStorage::hasTempAssetData(const LLUUID& texture_id) const 
{ return FALSE; }

// virtual
std::string LLAssetStorage::getTempAssetHostName(const LLUUID& texture_id) const 
{ return std::string(); }

// virtual
LLUUID LLAssetStorage::getTempAssetAgentID(const LLUUID& texture_id) const 
{ return LLUUID::null; }

// virtual
void LLAssetStorage::removeTempAssetData(const LLUUID& asset_id) 
{ }

// virtual
void LLAssetStorage::removeTempAssetDataByAgentID(const LLUUID& agent_id) 
{ }

// virtual
void LLAssetStorage::dumpTempAssetData(const LLUUID& avatar_id) const 
{ }

// virtual
void LLAssetStorage::clearTempAssetData() 
{ }
