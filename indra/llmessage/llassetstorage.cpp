/** 
 * @file llassetstorage.cpp
 * @brief Implementation of the base asset storage system.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

// system library includes
#include <sys/types.h>
#include <sys/stat.h>

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

#include "llmetrics.h"

LLAssetStorage *gAssetStorage = NULL;
LLMetrics *LLAssetStorage::metric_recipient = NULL;

const LLUUID CATEGORIZE_LOST_AND_FOUND_ID("00000000-0000-0000-0000-000000000010");

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
	mIsUserWaiting(FALSE),
	mTimeout(LL_ASSET_STORAGE_TIMEOUT),
	mIsPriority(FALSE),
	mDataSentInFirstPacket(FALSE),
	mDataIsInVFS( FALSE )
{
	// Need to guarantee that this time is up to date, we may be creating a circuit even though we haven't been
	//  running a message system loop.
	mTime = LLMessageSystem::getMessageTimeSeconds(TRUE);
}

// virtual
LLAssetRequest::~LLAssetRequest()
{
}

// virtual
LLSD LLAssetRequest::getTerseDetails() const
{
	LLSD sd;
	sd["asset_id"] = getUUID();
	sd["type_long"] = LLAssetType::lookupHumanReadable(getType());
	sd["type"] = LLAssetType::lookup(getType());
	sd["time"] = mTime;
	time_t timestamp = (time_t) mTime;
	std::ostringstream time_string;
	time_string << ctime(&timestamp);
	sd["time_string"] = time_string.str();
	return sd;
}

// virtual
LLSD LLAssetRequest::getFullDetails() const
{
	LLSD sd = getTerseDetails();
	sd["host"] = mHost.getIPandPort();
	sd["requesting_agent"] = mRequestingAgentID;
	sd["is_temp"] = mIsTemp;
	sd["is_local"] = mIsLocal;
	sd["is_priority"] = mIsPriority;
	sd["data_send_in_first_packet"] = mDataSentInFirstPacket;
	sd["data_is_in_vfs"] = mDataIsInVFS;

	return sd;
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
	F64 mt_secs = LLMessageSystem::getMessageTimeSeconds();

	request_list_t timed_out;
	S32 rt;
	for (rt = 0; rt < RT_COUNT; rt++)
	{
		request_list_t* requests = getRequestList((ERequestType)rt);
		for (request_list_t::iterator iter = requests->begin();
			 iter != requests->end(); )
		{
			request_list_t::iterator curiter = iter++;
			LLAssetRequest* tmp = *curiter;
			// if all is true, we want to clean up everything
			// otherwise just check for timed out requests
			// EXCEPT for upload timeouts
			if (all 
				|| ((RT_DOWNLOAD == rt)
					&& LL_ASSET_STORAGE_TIMEOUT < (mt_secs - tmp->mTime)))
			{
				llwarns << "Asset " << getRequestName((ERequestType)rt) << " request "
						<< (all ? "aborted" : "timed out") << " for "
						<< tmp->getUUID() << "."
						<< LLAssetType::lookup(tmp->getType()) << llendl;

				timed_out.push_front(tmp);
				iter = requests->erase(curiter);
			}
		}
	}

	LLAssetInfo	info;
	for (request_list_t::iterator iter = timed_out.begin();
		 iter != timed_out.end();  )
	{
		request_list_t::iterator curiter = iter++;
		LLAssetRequest* tmp = *curiter;
		if (tmp->mUpCallback)
		{
			tmp->mUpCallback(tmp->getUUID(), tmp->mUserData, error, LL_EXSTAT_NONE);
		}
		if (tmp->mDownCallback)
		{
			tmp->mDownCallback(mVFS, tmp->getUUID(), tmp->getType(), tmp->mUserData, error, LL_EXSTAT_NONE);
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
void LLAssetStorage::getAssetData(const LLUUID uuid, LLAssetType::EType type, void (*callback)(LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32, LLExtStat), void *user_data, BOOL is_priority)
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
			callback(mVFS, uuid, type, user_data, LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE, LL_EXSTAT_NULL_UUID);
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
		for (request_list_t::iterator iter = mPendingDownloads.begin();
			 iter != mPendingDownloads.end(); ++iter )
		{
			LLAssetRequest  *tmp = *iter;
			if ((type == tmp->getType()) && (uuid == tmp->getUUID()))
			{
				if (callback == tmp->mDownCallback && user_data == tmp->mUserData)
				{
					// this is a duplicate from the same subsystem - throw it away
					llwarns << "Discarding duplicate request for asset " << uuid
							<< "." << LLAssetType::lookup(type) << llendl;
					return;
				}
				
				// this is a duplicate request
				// queue the request, but don't actually ask for it again
				duplicate = TRUE;
			}
		}
		if (duplicate)
		{
			llinfos << "Adding additional non-duplicate request for asset " << uuid 
					<< "." << LLAssetType::lookup(type) << llendl;
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
			callback(mVFS, uuid, type, user_data, LL_ERR_NOERR, LL_EXSTAT_VFS_CACHED);
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
			callback(mVFS, uuid, atype, user_data, LL_ERR_CIRCUIT_GONE, LL_EXSTAT_NO_UPSTREAM);
		}
	}
}


void LLAssetStorage::downloadCompleteCallback(
	S32 result,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	void* user_data, LLExtStat ext_status)
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
	request_list_t requests;
	for (request_list_t::iterator iter = gAssetStorage->mPendingDownloads.begin();
		 iter != gAssetStorage->mPendingDownloads.end();  )
	{
		request_list_t::iterator curiter = iter++;
		LLAssetRequest* tmp = *curiter;
		if ((tmp->getUUID() == req->getUUID()) && (tmp->getType()== req->getType()))
		{
			requests.push_front(tmp);
			iter = gAssetStorage->mPendingDownloads.erase(curiter);
		}
	}
	for (request_list_t::iterator iter = requests.begin();
		 iter != requests.end();  )
	{
		request_list_t::iterator curiter = iter++;
		LLAssetRequest* tmp = *curiter;
		if (tmp->mDownCallback)
		{
			tmp->mDownCallback(gAssetStorage->mVFS, req->getUUID(), req->getType(), tmp->mUserData, result, ext_status);
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
			callback(mVFS, asset_id, atype, user_data, LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE, LL_EXSTAT_NULL_UUID);
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
				callback(mVFS, asset_id, atype, user_data, LL_ERR_CIRCUIT_GONE, LL_EXSTAT_NO_UPSTREAM);
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
			callback(mVFS, asset_id, atype, user_data, LL_ERR_NOERR, LL_EXSTAT_VFS_CACHED);
		}
	}
}

void LLAssetStorage::downloadEstateAssetCompleteCallback(
	S32 result,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	void* user_data,
	LLExtStat ext_status)
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

	req->mDownCallback(gAssetStorage->mVFS, req->getUUID(), req->getAType(), req->mUserData, result, ext_status);
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
				callback(mVFS, asset_id, atype, user_data, LL_ERR_CIRCUIT_GONE, LL_EXSTAT_NO_UPSTREAM);
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
			callback(mVFS, asset_id, atype, user_data, LL_ERR_NOERR, LL_EXSTAT_VFS_CACHED);
		}
	}
}


void LLAssetStorage::downloadInvItemCompleteCallback(
	S32 result,
	const LLUUID& file_id,
	LLAssetType::EType file_type,
	void* user_data,
	LLExtStat ext_status)
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

	req->mDownCallback(gAssetStorage->mVFS, req->getUUID(), req->getType(), req->mUserData, result, ext_status);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Store routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// static
void LLAssetStorage::uploadCompleteCallback(const LLUUID& uuid, void *user_data, S32 result, LLExtStat ext_status) // StoreAssetData callback (fixed)
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
	this_ptr->_callUploadCallbacks(uuid, asset_type, success, LL_EXSTAT_NONE);
}

void LLAssetStorage::_callUploadCallbacks(const LLUUID &uuid, LLAssetType::EType asset_type, BOOL success, LLExtStat ext_status )
{
	// SJB: We process the callbacks in reverse order, I do not know if this is important,
	//      but I didn't want to mess with it.
	request_list_t requests;
	for (request_list_t::iterator iter = mPendingUploads.begin();
		 iter != mPendingUploads.end();  )
	{
		request_list_t::iterator curiter = iter++;
		LLAssetRequest* req = *curiter;
		if ((req->getUUID() == uuid) && (req->getType() == asset_type))
		{
			requests.push_front(req);
			iter = mPendingUploads.erase(curiter);
		}
	}
	for (request_list_t::iterator iter = mPendingLocalUploads.begin();
		 iter != mPendingLocalUploads.end();  )
	{
		request_list_t::iterator curiter = iter++;
		LLAssetRequest* req = *curiter;
		if ((req->getUUID() == uuid) && (req->getType() == asset_type))
		{
			requests.push_front(req);
			iter = mPendingLocalUploads.erase(curiter);
		}
	}
	for (request_list_t::iterator iter = requests.begin();
		 iter != requests.end();  )
	{
		request_list_t::iterator curiter = iter++;
		LLAssetRequest* req = *curiter;
		if (req->mUpCallback)
		{
			req->mUpCallback(uuid, req->mUserData, (success ?  LL_ERR_NOERR :  LL_ERR_ASSET_REQUEST_FAILED ), ext_status );
		}
		delete req;
	}
}

LLAssetStorage::request_list_t* LLAssetStorage::getRequestList(LLAssetStorage::ERequestType rt)
{
	switch (rt)
	{
	case RT_DOWNLOAD:
		return &mPendingDownloads;
	case RT_UPLOAD:
		return &mPendingUploads;
	case RT_LOCALUPLOAD:
		return &mPendingLocalUploads;
	default:
		llwarns << "Unable to find request list for request type '" << rt << "'" << llendl;
		return NULL;
	}
}

const LLAssetStorage::request_list_t* LLAssetStorage::getRequestList(LLAssetStorage::ERequestType rt) const
{
	switch (rt)
	{
	case RT_DOWNLOAD:
		return &mPendingDownloads;
	case RT_UPLOAD:
		return &mPendingUploads;
	case RT_LOCALUPLOAD:
		return &mPendingLocalUploads;
	default:
		llwarns << "Unable to find request list for request type '" << rt << "'" << llendl;
		return NULL;
	}
}

// static
std::string LLAssetStorage::getRequestName(LLAssetStorage::ERequestType rt)
{
	switch (rt)
	{
	case RT_DOWNLOAD:
		return "download";
	case RT_UPLOAD:
		return "upload";
	case RT_LOCALUPLOAD:
		return "localupload";
	default:
		llwarns << "Unable to find request name for request type '" << rt << "'" << llendl;
		return "";
	}
}

S32 LLAssetStorage::getNumPending(LLAssetStorage::ERequestType rt) const
{
	const request_list_t* requests = getRequestList(rt);
	S32 num_pending = -1;
	if (requests)
	{
		num_pending = requests->size();
	}
	return num_pending;
}

S32 LLAssetStorage::getNumPendingDownloads() const
{
	return getNumPending(RT_DOWNLOAD);
}

S32 LLAssetStorage::getNumPendingUploads() const
{
	return getNumPending(RT_UPLOAD);
}

S32 LLAssetStorage::getNumPendingLocalUploads()
{
	return getNumPending(RT_LOCALUPLOAD);
}

// virtual
LLSD LLAssetStorage::getPendingDetails(LLAssetStorage::ERequestType rt,
										LLAssetType::EType asset_type,
										const std::string& detail_prefix) const
{
	const request_list_t* requests = getRequestList(rt);
	LLSD sd;
	sd["requests"] = getPendingDetails(requests, asset_type, detail_prefix);
	return sd;
}

// virtual
LLSD LLAssetStorage::getPendingDetails(const LLAssetStorage::request_list_t* requests,
										LLAssetType::EType asset_type,
										const std::string& detail_prefix) const
{
	LLSD details;
	if (requests)
	{
		request_list_t::const_iterator it = requests->begin();
		request_list_t::const_iterator end = requests->end();
		for ( ; it != end; ++it)
		{
			LLAssetRequest* req = *it;
			if (   (LLAssetType::AT_NONE == asset_type)
				|| (req->getType() == asset_type) )
			{
				LLSD row = req->getTerseDetails();

				std::ostringstream detail;
				detail	<< detail_prefix << "/" << LLAssetType::lookup(req->getType())
						<< "/" << req->getUUID();
				row["detail"] = LLURI(detail.str());

				details.append(row);
			}
		}
	}
	return details;
}


// static
const LLAssetRequest* LLAssetStorage::findRequest(const LLAssetStorage::request_list_t* requests,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id)
{
	if (requests) 
	{
		// Search the requests list for the asset.
		request_list_t::const_iterator iter = requests->begin();
		request_list_t::const_iterator end  = requests->end();
		for (; iter != end; ++iter)
		{
			const LLAssetRequest* req = *iter;
			if (asset_type == req->getType() &&
				asset_id == req->getUUID() )
			{
				return req;
			}
		}
	}
	return NULL;
}

// static
LLAssetRequest* LLAssetStorage::findRequest(LLAssetStorage::request_list_t* requests,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id)
{
	if (requests) 
	{
		// Search the requests list for the asset.
		request_list_t::iterator iter = requests->begin();
		request_list_t::iterator end  = requests->end();
		for (; iter != end; ++iter)
		{
			LLAssetRequest* req = *iter;
			if (asset_type == req->getType() &&
				asset_id == req->getUUID() )
			{
				return req;
			}
		}
	}
	return NULL;
}


// virtual
LLSD LLAssetStorage::getPendingRequest(LLAssetStorage::ERequestType rt,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id) const
{
	const request_list_t* requests = getRequestList(rt);
	return getPendingRequest(requests, asset_type, asset_id);
}

// virtual
LLSD LLAssetStorage::getPendingRequest(const LLAssetStorage::request_list_t* requests,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id) const
{
	LLSD sd;
	const LLAssetRequest* req = findRequest(requests, asset_type, asset_id);
	if (req)
	{
		sd = req->getFullDetails();
	}
	return sd;
}

// virtual
bool LLAssetStorage::deletePendingRequest(LLAssetStorage::ERequestType rt,
											LLAssetType::EType asset_type,
											const LLUUID& asset_id)
{
	request_list_t* requests = getRequestList(rt);
	if (deletePendingRequest(requests, asset_type, asset_id))
	{
		llinfos << "Asset " << getRequestName(rt) << " request for "
				<< asset_id << "." << LLAssetType::lookup(asset_type)
				<< " removed from pending queue." << llendl;
		return true;
	}
	return false;
}

// virtual
bool LLAssetStorage::deletePendingRequest(LLAssetStorage::request_list_t* requests,
											LLAssetType::EType asset_type,
											const LLUUID& asset_id)
{
	LLAssetRequest* req = findRequest(requests, asset_type, asset_id);
	if (req)
	{
		// Remove the request from this list.
		requests->remove(req);
		S32 error = LL_ERR_TCP_TIMEOUT;
		// Run callbacks.
		if (req->mUpCallback)
		{
			req->mUpCallback(req->getUUID(), req->mUserData, error, LL_EXSTAT_REQUEST_DROPPED);
		}
		if (req->mDownCallback)
		{
			req->mDownCallback(mVFS, req->getUUID(), req->getType(), req->mUserData, error, LL_EXSTAT_REQUEST_DROPPED);
		}
		if (req->mInfoCallback)
		{
			LLAssetInfo info;
			req->mInfoCallback(&info, req->mUserData, error);
		}
		delete req;
		return true;
	}
	
	return false;
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



void LLAssetStorage::getAssetData(const LLUUID uuid, LLAssetType::EType type, void (*callback)(const char*, const LLUUID&, void *, S32, LLExtStat), void *user_data, BOOL is_priority)
{
	// check for duplicates here, since we're about to fool the normal duplicate checker
	for (request_list_t::iterator iter = mPendingDownloads.begin();
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
void LLAssetStorage::legacyGetDataCallback(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, void *user_data, S32 status, LLExtStat ext_status)
{
	LLLegacyAssetRequest *legacy = (LLLegacyAssetRequest *)user_data;
	char filename[LL_MAX_PATH] = "";	/* Flawfinder: ignore */ 

	if (! status)
	{
		LLVFile file(vfs, uuid, type);

		char uuid_str[UUID_STR_LENGTH];	/* Flawfinder: ignore */ 

		uuid.toString(uuid_str);
		snprintf(filename,sizeof(filename),"%s.%s",gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str).c_str(),LLAssetType::lookup(type));	/* Flawfinder: ignore */

		FILE* fp = LLFile::fopen(filename, "wb");	/* Flawfinder: ignore */ 
		if (fp)
		{
			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while (file.read(copy_buf, buf_size))	/* Flawfinder: ignore */
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

	legacy->mDownCallback(filename, uuid, legacy->mUserData, status, ext_status);
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
	bool store_local,
	bool user_waiting,
	F64 timeout)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
	// LLAssetStorage metric: Virtual base call
	reportMetric( LLUUID::null, asset_type, NULL, LLUUID::null, 0, MR_BAD_FUNCTION, __FILE__, __LINE__, "Illegal call to base: LLAssetStorage::storeAssetData 1" );
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
	const LLUUID& requesting_agent_id,
	bool user_waiting,
	F64 timeout)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
	// LLAssetStorage metric: Virtual base call
	reportMetric( asset_id, asset_type, NULL, requesting_agent_id, 0, MR_BAD_FUNCTION, __FILE__, __LINE__, "Illegal call to base: LLAssetStorage::storeAssetData 2" );
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
	bool is_priority,
	bool user_waiting,
	F64 timeout)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
	// LLAssetStorage metric: Virtual base call
	reportMetric( asset_id, asset_type, NULL, LLUUID::null, 0, MR_BAD_FUNCTION, __FILE__, __LINE__, "Illegal call to base: LLAssetStorage::storeAssetData 3" );
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
	bool is_priority,
	bool user_waiting,
	F64 timeout)
{
	llwarns << "storeAssetData: wrong version called" << llendl;
	// LLAssetStorage metric: Virtual base call
	reportMetric( LLUUID::null, asset_type, NULL, LLUUID::null, 0, MR_BAD_FUNCTION, __FILE__, __LINE__, "Illegal call to base: LLAssetStorage::storeAssetData 4" );
}

// static
void LLAssetStorage::legacyStoreDataCallback(const LLUUID &uuid, void *user_data, S32 status, LLExtStat ext_status)
{
	LLLegacyAssetRequest *legacy = (LLLegacyAssetRequest *)user_data;
	if (legacy && legacy->mUpCallback)
	{
		legacy->mUpCallback(uuid, legacy->mUserData, status, ext_status);
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

// static
void LLAssetStorage::reportMetric( const LLUUID& asset_id, const LLAssetType::EType asset_type, const char *filename,
								   const LLUUID& agent_id, S32 asset_size, EMetricResult result,
								   const char *file, const S32 line, const char *message )
{
	if( !metric_recipient )
	{
		llinfos << "Couldn't store LLAssetStoreage::reportMetric - no metrics_recipient" << llendl;
		return;
	}

	filename = filename ? filename : "";
	file = file ? file : "";

	// Create revised message - message = "message :: file:line"
	std::string new_message; //( message );
	new_message = message; // << " " << file << " " << line;
	new_message += " :: ";
	new_message += filename;
	char line_string[16];
	sprintf( line_string, ":%d", line );
	new_message += line_string;
	message = new_message.c_str();

	// Change always_report to true if debugging... do not check it in this way
	static bool always_report = false;
	const char *metric_name = "LLAssetStorage::Metrics";

	bool success = result == MR_OKAY;

	if( (!success) || always_report )
	{
		LLSD stats;
		stats["asset_id"] = asset_id;
		stats["asset_type"] = asset_type;
		stats["filename"] = filename? filename : "";
		stats["agent_id"] = agent_id;
		stats["asset_size"] = (S32)asset_size;
		stats["result"] = (S32)result;

		metric_recipient->recordEventDetails( metric_name, message, success, stats);
	}
	else
	{
		metric_recipient->recordEvent(metric_name, message, success);
	}
}
