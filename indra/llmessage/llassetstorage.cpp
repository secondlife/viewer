/** 
 * @file llassetstorage.cpp
 * @brief Implementation of the base asset storage system.
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
#include "llframetimer.h"

// this library includes
#include "message.h"
#include "llxfermanager.h"
#include "llfilesystem.h"
#include "lldbstrings.h"

#include "lltransfersourceasset.h"
#include "lltransfertargetvfile.h" // For debugging

#include "llmetrics.h"
#include "lltrace.h"

LLAssetStorage *gAssetStorage = NULL;
LLMetrics *LLAssetStorage::metric_recipient = NULL;

static LLTrace::CountStatHandle<> sFailedDownloadCount("faileddownloads", "Number of times LLAssetStorage::getAssetData() has failed");


const LLUUID CATEGORIZE_LOST_AND_FOUND_ID(std::string("00000000-0000-0000-0000-000000000010"));

const U64 TOXIC_ASSET_LIFETIME = (120 * 1000000);       // microseconds

namespace
{
    bool operator == (const LLAssetStorage::LLGetAssetCallback &lhs, const LLAssetStorage::LLGetAssetCallback &rhs)
    {
        auto fnPtrLhs = lhs.target<LLAssetStorage::LLGetAssetCallback>();
        auto fnPtrRhs = rhs.target<LLAssetStorage::LLGetAssetCallback>();
        if (fnPtrLhs && fnPtrRhs)
            return (*fnPtrLhs == *fnPtrRhs);
        else if (!fnPtrLhs && !fnPtrRhs)
            return true;
        return false;
    }

// Rider: This is the general case of the operator declared above. The code compares the callback 
// passed into the LLAssetStorage functions to determine if there are duplicated requests for an 
// asset.  Unfortunately std::function does not provide a direct way to compare two variables so 
// we define the operator here. 
// XCode is not very happy with the variadic temples in use below so we will just define the specific 
// case of comparing two LLGetAssetCallback objects since that is all we really use.
// 
//     template<typename T, typename... U>
//     bool operator == (const std::function<T(U...)> &a, const std::function <T(U...)> &b)
//     {
//         typedef T(fnType)(U...);
// 
//         auto fnPtrA = a.target<T(*)(U...)>();
//         auto fnPtrB = b.target<T(*)(U...)>();
//         if (fnPtrA && fnPtrB)
//             return (*fnPtrA == *fnPtrB);
//         else if (!fnPtrA && !fnPtrB)
//             return true;
//         return false;
//     }

}

///----------------------------------------------------------------------------
/// LLAssetInfo
///----------------------------------------------------------------------------

LLAssetInfo::LLAssetInfo( void )
    :   mDescription(),
        mName(),
        mUuid(),
        mCreatorID(),
        mType( LLAssetType::AT_NONE )
{ }

LLAssetInfo::LLAssetInfo( const LLUUID& object_id, const LLUUID& creator_id,
                          LLAssetType::EType type, const char* name,
                          const char* desc )
    :   mUuid( object_id ), 
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
    mType = LLAssetType::lookup( buf );
    buf.assign( str, pos1, std::string::npos );
    mUuid.set( buf );

    // convert the value to useful information
    str.assign( nv.getAsset() );
    pos1 = str.find('|');
    buf.assign( str, 0, pos1++ );
    mCreatorID.set( buf );
    pos2 = str.find( '|', pos1 );
    buf.assign( str, pos1, (pos2++) - pos1 );
    setName( buf );
    buf.assign( str, pos2, std::string::npos );
    setDescription( buf );
    LL_DEBUGS("AssetStorage") << "uuid: " << mUuid << LL_ENDL;
    LL_DEBUGS("AssetStorage") << "creator: " << mCreatorID << LL_ENDL;
}

///----------------------------------------------------------------------------
/// LLBaseDownloadRequest
///----------------------------------------------------------------------------

LLBaseDownloadRequest::LLBaseDownloadRequest(const LLUUID &uuid, const LLAssetType::EType type)
    : mUUID(uuid),
      mType(type),
      mDownCallback(),
      mUserData(NULL),
      mHost(),
      mIsTemp(false),
      mIsPriority(false),
      mDataSentInFirstPacket(false),
      mDataIsInCache(false)
{
    // Need to guarantee that this time is up to date, we may be creating a circuit even though we haven't been
    //  running a message system loop.
    mTime = LLMessageSystem::getMessageTimeSeconds(true);
}

// virtual
LLBaseDownloadRequest::~LLBaseDownloadRequest()
{
}

// virtual
LLBaseDownloadRequest* LLBaseDownloadRequest::getCopy()
{
    return new LLBaseDownloadRequest(*this);
}


///----------------------------------------------------------------------------
/// LLAssetRequest
///----------------------------------------------------------------------------

LLAssetRequest::LLAssetRequest(const LLUUID &uuid, const LLAssetType::EType type)
    :   LLBaseDownloadRequest(uuid, type),
        mUpCallback(),
        mInfoCallback( NULL ),
        mIsLocal(false),
        mIsUserWaiting(false),
        mTimeout(LL_ASSET_STORAGE_TIMEOUT),
        mBytesFetched(0)
{
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
    sd["time"] = mTime.value();
    time_t timestamp = (time_t) mTime.value();
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
    // Note: cannot change this (easily) since it is consumed by server
    sd["data_is_in_vfs"] = mDataIsInCache;

    return sd;
}

LLBaseDownloadRequest* LLAssetRequest::getCopy()
{
    return new LLAssetRequest(*this);
}

///----------------------------------------------------------------------------
/// LLInvItemRequest
///----------------------------------------------------------------------------

LLInvItemRequest::LLInvItemRequest(const LLUUID &uuid, const LLAssetType::EType type)
    :   LLBaseDownloadRequest(uuid, type)
{
}

// virtual
LLInvItemRequest::~LLInvItemRequest()
{
}

LLBaseDownloadRequest* LLInvItemRequest::getCopy()
{
    return new LLInvItemRequest(*this);
}

///----------------------------------------------------------------------------
/// LLEstateAssetRequest
///----------------------------------------------------------------------------

LLEstateAssetRequest::LLEstateAssetRequest(const LLUUID &uuid, const LLAssetType::EType atype,
                                           EstateAssetType etype)
    :   LLBaseDownloadRequest(uuid, atype),
        mEstateAssetType(etype)
{
}

// Virtual
LLEstateAssetRequest::~LLEstateAssetRequest()
{
}

LLBaseDownloadRequest* LLEstateAssetRequest::getCopy()
{
    return new LLEstateAssetRequest(*this);
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


LLAssetStorage::LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer, const LLHost &upstream_host)
{
    _init(msg, xfer, upstream_host);
}

LLAssetStorage::LLAssetStorage(LLMessageSystem *msg, LLXferManager *xfer)
{
    _init(msg, xfer, LLHost());
}

void LLAssetStorage::_init(LLMessageSystem *msg,
                           LLXferManager *xfer,
                           const LLHost &upstream_host)
{
    mShutDown = false;
    mMessageSys = msg;
    mXferManager = xfer;

    setUpstream(upstream_host);
    msg->setHandlerFuncFast(_PREHASH_AssetUploadComplete, processUploadComplete, (void **)this);
}

LLAssetStorage::~LLAssetStorage()
{
    mShutDown = true;
    
    _cleanupRequests(true, LL_ERR_CIRCUIT_GONE);

    if (gMessageSystem)
    {
        // Warning!  This won't work if there's more than one asset storage.
        // unregister our callbacks with the message system
        gMessageSystem->setHandlerFuncFast(_PREHASH_AssetUploadComplete, NULL, NULL);
    }

    // Clear the toxic asset map
    mToxicAssetMap.clear();
}

void LLAssetStorage::setUpstream(const LLHost &upstream_host)
{
    LL_DEBUGS("AppInit") << "AssetStorage: Setting upstream provider to " << upstream_host << LL_ENDL;
    
    mUpstreamHost = upstream_host;
}

void LLAssetStorage::checkForTimeouts()
{
    _cleanupRequests(false, LL_ERR_TCP_TIMEOUT);
}

void LLAssetStorage::_cleanupRequests(bool all, S32 error)
{
    F64Seconds mt_secs = LLMessageSystem::getMessageTimeSeconds();

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
                LL_WARNS("AssetStorage") << "Asset " << getRequestName((ERequestType)rt) << " request "
                                         << (all ? "aborted" : "timed out") << " for "
                                         << tmp->getUUID() << "."
                                         << LLAssetType::lookup(tmp->getType()) << LL_ENDL;
                
                timed_out.push_front(tmp);
                iter = requests->erase(curiter);
            }
        }
    }

    LLAssetInfo     info;
    for (request_list_t::iterator iter = timed_out.begin();
         iter != timed_out.end();  )
    {
        request_list_t::iterator curiter = iter++;
        LLAssetRequest* tmp = *curiter;
        if (tmp->mUpCallback)
        {
            tmp->mUpCallback(tmp->getUUID(), tmp->mUserData, error, LLExtStat::NONE);
        }
        if (tmp->mDownCallback)
        {
            tmp->mDownCallback(tmp->getUUID(), tmp->getType(), tmp->mUserData, error, LLExtStat::NONE);
        }
        if (tmp->mInfoCallback)
        {
            tmp->mInfoCallback(&info, tmp->mUserData, error);
        }
        delete tmp;
    }

}

bool LLAssetStorage::hasLocalAsset(const LLUUID &uuid, const LLAssetType::EType type)
{
    return LLFileSystem::getExists(uuid, type);
}

bool LLAssetStorage::findInCacheAndInvokeCallback(const LLUUID& uuid, LLAssetType::EType type,
                                                      LLGetAssetCallback callback, void *user_data)
{
    if (user_data)
    {
        // The *user_data should not be passed without a callback to clean it up.
        llassert(callback != NULL);
    }

    bool exists = LLFileSystem::getExists(uuid, type);
    if (exists)
    {
        LLFileSystem file(uuid, type);
        U32 size = file.getSize();
        if (size > 0)
        {
            // we've already got the file
            if (callback)
            {
                callback(uuid, type, user_data, LL_ERR_NOERR, LLExtStat::CACHE_CACHED);
            }
            return true;
        }
        else
        {
            LL_WARNS("AssetStorage") << "Asset vfile " << uuid << ":" << type
                                     << " found in static cache with bad size " << file.getSize() << ", ignoring" << LL_ENDL;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////
// GET routines
///////////////////////////////////////////////////////////////////////////

// IW - uuid is passed by value to avoid side effects, please don't re-add &    
void LLAssetStorage::getAssetData(const LLUUID uuid,
                                  LLAssetType::EType type, 
                                  LLAssetStorage::LLGetAssetCallback callback,
                                  void *user_data, 
                                  bool is_priority)
{
    LL_DEBUGS("AssetStorage") << "LLAssetStorage::getAssetData() - " << uuid << "," << LLAssetType::lookup(type) << LL_ENDL;

    LL_DEBUGS("AssetStorage") << "ASSET_TRACE requesting " << uuid << " type " << LLAssetType::lookup(type) << LL_ENDL;

    if (user_data)
    {
        // The *user_data should not be passed without a callback to clean it up.
        llassert(callback != NULL);
    }

    if (mShutDown)
    {
        LL_DEBUGS("AssetStorage") << "ASSET_TRACE cancelled " << uuid << " type " << LLAssetType::lookup(type) << " shutting down" << LL_ENDL;

        if (callback)
        {
            add(sFailedDownloadCount, 1);
            callback(uuid, type, user_data, LL_ERR_ASSET_REQUEST_FAILED, LLExtStat::NONE);
        }
        return;
    }

    if (uuid.isNull())
    {
        // Special case early out for NULL uuid and for shutting down
        if (callback)
        {
            add(sFailedDownloadCount, 1);
            callback(uuid, type, user_data, LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE, LLExtStat::NULL_UUID);
        }
        return;
    }

    if (findInCacheAndInvokeCallback(uuid,type,callback,user_data))
    {
        LL_DEBUGS("AssetStorage") << "ASSET_TRACE asset " << uuid << " found in cache" << LL_ENDL;
        return;
    }

    bool exists = LLFileSystem::getExists(uuid, type);
    LLFileSystem file(uuid, type);
    U32 size = exists ? file.getSize() : 0;

    if (size > 0)
    {
        // we've already got the file
        // theoretically, partial files w/o a pending request shouldn't happen
        // unless there's a weird error
        if (callback)
        {
            callback(uuid, type, user_data, LL_ERR_NOERR, LLExtStat::CACHE_CACHED);
        }

        LL_DEBUGS("AssetStorage") << "ASSET_TRACE asset " << uuid << " found in cache" << LL_ENDL;
    }
    else
    {
        if (exists)
        {
            LL_WARNS("AssetStorage") << "Asset vfile " << uuid << ":" << type << " found with bad size " << file.getSize() << ", removing" << LL_ENDL;
            file.remove();
        }
        
        bool duplicate = false;
        
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
                    LL_WARNS("AssetStorage") << "Discarding duplicate request for asset " << uuid
                                             << "." << LLAssetType::lookup(type) << LL_ENDL;
                    return;
                }
                
                // this is a duplicate request
                // queue the request, but don't actually ask for it again
                duplicate = true;
            }
        }
        if (duplicate)
        {
            LL_DEBUGS("AssetStorage") << "Adding additional non-duplicate request for asset " << uuid 
                                      << "." << LLAssetType::lookup(type) << LL_ENDL;
        }
        
        _queueDataRequest(uuid, type, callback, user_data, duplicate, is_priority);     
    }
}

// static
void LLAssetStorage::removeAndCallbackPendingDownloads(const LLUUID& file_id, LLAssetType::EType file_type,
                                                       const LLUUID& callback_id, LLAssetType::EType callback_type,
                                                       S32 result_code, LLExtStat ext_status)
{
    // find and callback ALL pending requests for this UUID
    // SJB: We process the callbacks in reverse order, I do not know if this is important,
    //      but I didn't want to mess with it.
    request_list_t requests;
    for (request_list_t::iterator iter = gAssetStorage->mPendingDownloads.begin();
         iter != gAssetStorage->mPendingDownloads.end();  )
    {
        request_list_t::iterator curiter = iter++;
        LLAssetRequest* tmp = *curiter;
        if ((tmp->getUUID() == file_id) && (tmp->getType()== file_type))
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
            if (result_code!= LL_ERR_NOERR)
            {
                add(sFailedDownloadCount, 1);
            }
            tmp->mDownCallback(callback_id, callback_type, tmp->mUserData, result_code, ext_status);
        }
        delete tmp;
    }
}

void LLAssetStorage::downloadCompleteCallback(
    S32 result,
    const LLUUID& file_id,
    LLAssetType::EType file_type,
    LLBaseDownloadRequest* user_data,
    LLExtStat ext_status)
{
    LL_DEBUGS("AssetStorage") << "ASSET_TRACE asset " << file_id << " downloadCompleteCallback" << LL_ENDL;

    LL_DEBUGS("AssetStorage") << "LLAssetStorage::downloadCompleteCallback() for " << file_id
                              << "," << LLAssetType::lookup(file_type) << LL_ENDL;
    LLAssetRequest* req = (LLAssetRequest*)user_data;
    if(!req)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::downloadCompleteCallback called without"
            "a valid request." << LL_ENDL;
        return;
    }
    if (!gAssetStorage)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::downloadCompleteCallback called without any asset system, aborting!" << LL_ENDL;
        return;
    }

    LLUUID callback_id;
    LLAssetType::EType callback_type;

    // Inefficient since we're doing a find through a list that may have thousands of elements.
    // This is due for refactoring; we will probably change mPendingDownloads into a set.
    request_list_t::iterator download_iter = std::find(gAssetStorage->mPendingDownloads.begin(), 
                                                       gAssetStorage->mPendingDownloads.end(),
                                                       req);

    if (download_iter != gAssetStorage->mPendingDownloads.end())
    {
        callback_id = file_id;
        callback_type = file_type;
    }
    else
    {
        // either has already been deleted by _cleanupRequests or it's a transfer.
        callback_id = req->getUUID();
        callback_type = req->getType();
    }

    if (LL_ERR_NOERR == result)
    {
        // we might have gotten a zero-size file
        LLFileSystem vfile(callback_id, callback_type);
        if (vfile.getSize() <= 0)
        {
            LL_WARNS("AssetStorage") << "downloadCompleteCallback has non-existent or zero-size asset " << callback_id << LL_ENDL;
            
            result = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
            vfile.remove();
        }
        else
        {
#if 1
            for (request_list_t::iterator iter = gAssetStorage->mPendingDownloads.begin();
                 iter != gAssetStorage->mPendingDownloads.end(); ++iter  )
            {
                LLAssetRequest* dlreq = *iter;
                if ((dlreq->getUUID() == file_id) && (dlreq->getType()== file_type))
                {
                    dlreq->mBytesFetched = vfile.getSize();
                }
            }
#endif
        }
    }

    removeAndCallbackPendingDownloads(file_id, file_type, callback_id, callback_type, result, ext_status);
}

void LLAssetStorage::getEstateAsset(
    const LLHost &object_sim,
    const LLUUID &agent_id, 
    const LLUUID &session_id,
    const LLUUID &asset_id, 
    LLAssetType::EType atype, 
    EstateAssetType etype,
    LLGetAssetCallback callback, 
    void *user_data, 
    bool is_priority)
{
    LL_DEBUGS() << "LLAssetStorage::getEstateAsset() - " << asset_id << "," << LLAssetType::lookup(atype) << ", estatetype " << etype << LL_ENDL;

    //
    // Probably will get rid of this early out?
    //
    if (asset_id.isNull())
    {
        // Special case early out for NULL uuid
        if (callback)
        {
            add(sFailedDownloadCount, 1);
            callback(asset_id, atype, user_data, LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE, LLExtStat::NULL_UUID);
        }
        return;
    }

    // Try static first.
    if (findInCacheAndInvokeCallback(asset_id,atype,callback,user_data))
    {
        return;
    }
    
    bool exists = LLFileSystem::getExists(asset_id, atype);
    LLFileSystem file(asset_id, atype);
    U32 size = exists ? file.getSize() : 0;

    if (size > 0)
    {
        // we've already got the file
        // theoretically, partial files w/o a pending request shouldn't happen
        // unless there's a weird error
        if (callback)
        {
            callback(asset_id, atype, user_data, LL_ERR_NOERR, LLExtStat::CACHE_CACHED);
        }
    }
    else
    {
        if (exists)
        {
            LL_WARNS("AssetStorage") << "Asset vfile " << asset_id << ":" << atype << " found with bad size " << file.getSize() << ", removing" << LL_ENDL;
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
            LLEstateAssetRequest req(asset_id, atype, etype);
            req.mDownCallback = callback;
            req.mUserData = user_data;
            req.mIsPriority = is_priority;

            // send request message to our upstream data provider
            // Create a new asset transfer.
            LLTransferSourceParamsEstate spe;
            spe.setAgentSession(agent_id, session_id);
            spe.setEstateAssetType(etype);

            // Set our destination file, and the completion callback.
            LLTransferTargetParamsVFile tpvf;
            tpvf.setAsset(asset_id, atype);
            tpvf.setCallback(downloadEstateAssetCompleteCallback, req);

            LL_DEBUGS("AssetStorage") << "Starting transfer for " << asset_id << LL_ENDL;
            LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(source_host, LLTCT_ASSET);
            ttcp->requestTransfer(spe, tpvf, 100.f + (is_priority ? 1.f : 0.f));
        }
        else
        {
            // uh-oh, we shouldn't have gotten here
            LL_WARNS("AssetStorage") << "Attempt to move asset data request upstream w/o valid upstream provider" << LL_ENDL;
            if (callback)
            {
                add(sFailedDownloadCount, 1);
                callback(asset_id, atype, user_data, LL_ERR_CIRCUIT_GONE, LLExtStat::NO_UPSTREAM);
            }
        }
    }
}

void LLAssetStorage::downloadEstateAssetCompleteCallback(
    S32 result,
    const LLUUID& file_id,
    LLAssetType::EType file_type,
    LLBaseDownloadRequest* user_data,
    LLExtStat ext_status)
{
    LLEstateAssetRequest *req = (LLEstateAssetRequest*)user_data;
    if(!req)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::downloadEstateAssetCompleteCallback called"
            " without a valid request." << LL_ENDL;
        return;
    }
    if (!gAssetStorage)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::downloadEstateAssetCompleteCallback called"
            " without any asset system, aborting!" << LL_ENDL;
        return;
    }

    req->setUUID(file_id);
    req->setType(file_type);
    if (LL_ERR_NOERR == result)
    {
        // we might have gotten a zero-size file
        LLFileSystem vfile(req->getUUID(), req->getAType());
        if (vfile.getSize() <= 0)
        {
            LL_WARNS("AssetStorage") << "downloadCompleteCallback has non-existent or zero-size asset!" << LL_ENDL;

            result = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
            vfile.remove();
        }
    }

    if (result != LL_ERR_NOERR)
    {
        add(sFailedDownloadCount, 1);
    }
    req->mDownCallback(req->getUUID(), req->getAType(), req->mUserData, result, ext_status);
}

void LLAssetStorage::getInvItemAsset(
    const LLHost &object_sim, 
    const LLUUID &agent_id, 
    const LLUUID &session_id,
    const LLUUID &owner_id,
    const LLUUID &task_id, 
    const LLUUID &item_id,
    const LLUUID &asset_id, 
    LLAssetType::EType atype,
    LLGetAssetCallback callback, 
    void *user_data, 
    bool is_priority)
{
    LL_DEBUGS() << "LLAssetStorage::getInvItemAsset() - " << asset_id << "," << LLAssetType::lookup(atype) << LL_ENDL;

    bool exists = false; 
    U32 size = 0;

    if(asset_id.notNull())
    {
        if (findInCacheAndInvokeCallback( asset_id, atype, callback, user_data))
        {
            return;
        }

        exists = LLFileSystem::getExists(asset_id, atype);
        LLFileSystem file(asset_id, atype);
        size = exists ? file.getSize() : 0;
        if(exists && size < 1)
        {
            LL_WARNS("AssetStorage") << "Asset vfile " << asset_id << ":" << atype << " found with bad size " << file.getSize() << ", removing" << LL_ENDL;
            file.remove();
        }

    }

    if (size > 0)
    {
        // we've already got the file
        // theoretically, partial files w/o a pending request shouldn't happen
        // unless there's a weird error
        if (callback)
        {
            callback(asset_id, atype, user_data, LL_ERR_NOERR, LLExtStat::CACHE_CACHED);
        }
    }
    else
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
            LLInvItemRequest req(asset_id, atype);
            req.mDownCallback = callback;
            req.mUserData = user_data;
            req.mIsPriority = is_priority;

            // send request message to our upstream data provider
            // Create a new asset transfer.
            LLTransferSourceParamsInvItem spi;
            spi.setAgentSession(agent_id, session_id);
            spi.setInvItem(owner_id, task_id, item_id);
            spi.setAsset(asset_id, atype);

            LL_DEBUGS("ViewerAsset") << "requesting inv item id " << item_id << " asset_id " << asset_id << " type " << LLAssetType::lookup(atype) << LL_ENDL;
            
            // Set our destination file, and the completion callback.
            LLTransferTargetParamsVFile tpvf;
            tpvf.setAsset(asset_id, atype);
            tpvf.setCallback(downloadInvItemCompleteCallback, req);

            LL_DEBUGS("AssetStorage") << "Starting transfer for inventory asset "
                                      << item_id << " owned by " << owner_id << "," << task_id
                                      << LL_ENDL;
            LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(source_host, LLTCT_ASSET);
            ttcp->requestTransfer(spi, tpvf, 100.f + (is_priority ? 1.f : 0.f));
        }
        else
        {
            // uh-oh, we shouldn't have gotten here
            LL_WARNS("AssetStorage") << "Attempt to move asset data request upstream w/o valid upstream provider" << LL_ENDL;
            if (callback)
            {
                add(sFailedDownloadCount, 1);
                callback(asset_id, atype, user_data, LL_ERR_CIRCUIT_GONE, LLExtStat::NO_UPSTREAM);
            }
        }
    }
}


void LLAssetStorage::downloadInvItemCompleteCallback(
    S32 result,
    const LLUUID& file_id,
    LLAssetType::EType file_type,
    LLBaseDownloadRequest* user_data,
    LLExtStat ext_status)
{
    LLInvItemRequest *req = (LLInvItemRequest*)user_data;
    if(!req)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::downloadEstateAssetCompleteCallback called"
            " without a valid request." << LL_ENDL;
        return;
    }
    if (!gAssetStorage)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::downloadCompleteCallback called without any asset system, aborting!" << LL_ENDL;
        return;
    }

    req->setUUID(file_id);
    req->setType(file_type);
    if (LL_ERR_NOERR == result)
    {
        // we might have gotten a zero-size file
        LLFileSystem vfile(req->getUUID(), req->getType());
        if (vfile.getSize() <= 0)
        {
            LL_WARNS("AssetStorage") << "downloadCompleteCallback has non-existent or zero-size asset!" << LL_ENDL;

            result = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
            vfile.remove();
        }
    }

    if (result != LL_ERR_NOERR)
    {
        add(sFailedDownloadCount, 1);
    }
    req->mDownCallback(req->getUUID(), req->getType(), req->mUserData, result, ext_status);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Store routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// static
void LLAssetStorage::uploadCompleteCallback(
    const LLUUID& uuid, 
    void *user_data, 
    S32 result, 
    LLExtStat ext_status) // StoreAssetData callback (fixed)
{
    if (!gAssetStorage)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::uploadCompleteCallback has no gAssetStorage!" << LL_ENDL;
        return;
    }
    LLAssetRequest  *req     = (LLAssetRequest *)user_data;
    bool            success  = true;

    if (result)
    {
        LL_WARNS("AssetStorage") << "LLAssetStorage::uploadCompleteCallback " << result << ":" << getErrorString(result) << " trying to upload file to upstream provider" << LL_ENDL;
        success = false;
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
    LLAssetStorage  *this_ptr = (LLAssetStorage *)user_data;
    LLUUID          uuid;
    S8              asset_type_s8;
    LLAssetType::EType asset_type;
    bool            success = false;

    msg->getUUIDFast(_PREHASH_AssetBlock, _PREHASH_UUID, uuid);
    msg->getS8Fast(_PREHASH_AssetBlock, _PREHASH_Type, asset_type_s8);
    msg->getBOOLFast(_PREHASH_AssetBlock, _PREHASH_Success, success);

    asset_type = (LLAssetType::EType)asset_type_s8;
    this_ptr->_callUploadCallbacks(uuid, asset_type, success, LLExtStat::NONE);
}

void LLAssetStorage::_callUploadCallbacks(const LLUUID &uuid, LLAssetType::EType asset_type, bool success, LLExtStat ext_status )
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
            LL_WARNS("AssetStorage") << "Unable to find request list for request type '" << rt << "'" << LL_ENDL;
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
            LL_WARNS("AssetStorage") << "Unable to find request list for request type '" << rt << "'" << LL_ENDL;
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
            LL_WARNS("AssetStorage") << "Unable to find request name for request type '" << rt << "'" << LL_ENDL;
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

LLSD LLAssetStorage::getPendingDetails(LLAssetStorage::ERequestType rt,
                                       LLAssetType::EType asset_type,
                                       const std::string& detail_prefix) const
{
    const request_list_t* requests = getRequestList(rt);
    LLSD sd;
    sd["requests"] = getPendingDetailsImpl(requests, asset_type, detail_prefix);
    return sd;
}

LLSD LLAssetStorage::getPendingDetailsImpl(const LLAssetStorage::request_list_t* requests,
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
                detail  << detail_prefix << "/" << LLAssetType::lookup(req->getType())
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


LLSD LLAssetStorage::getPendingRequest(LLAssetStorage::ERequestType rt,
                                       LLAssetType::EType asset_type,
                                       const LLUUID& asset_id) const
{
    const request_list_t* requests = getRequestList(rt);
    return getPendingRequestImpl(requests, asset_type, asset_id);
}

LLSD LLAssetStorage::getPendingRequestImpl(const LLAssetStorage::request_list_t* requests,
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

bool LLAssetStorage::deletePendingRequest(LLAssetStorage::ERequestType rt,
                                          LLAssetType::EType asset_type,
                                          const LLUUID& asset_id)
{
    request_list_t* requests = getRequestList(rt);
    if (deletePendingRequestImpl(requests, asset_type, asset_id))
    {
        LL_DEBUGS("AssetStorage") << "Asset " << getRequestName(rt) << " request for "
                                  << asset_id << "." << LLAssetType::lookup(asset_type)
                                  << " removed from pending queue." << LL_ENDL;
        return true;
    }
    return false;
}

bool LLAssetStorage::deletePendingRequestImpl(LLAssetStorage::request_list_t* requests,
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
            req->mUpCallback(req->getUUID(), req->mUserData, error, LLExtStat::REQUEST_DROPPED);
        }
        if (req->mDownCallback)
        {
            add(sFailedDownloadCount, 1);
            req->mDownCallback(req->getUUID(), req->getType(), req->mUserData, error, LLExtStat::REQUEST_DROPPED);
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

        case LL_ERR_PRICE_MISMATCH:
            return "Viewer and server do not agree on price";

        default:
            return "Unknown status";
    }
}

void LLAssetStorage::getAssetData(const LLUUID uuid, 
                                  LLAssetType::EType type, 
                                  void (*callback)(const char*, 
                                                   const LLUUID&, 
                                                   void *, 
                                                   S32, 
                                                   LLExtStat), 
                                  void *user_data, 
                                  bool is_priority)
{
    // check for duplicates here, since we're about to fool the normal duplicate checker
    for (request_list_t::iterator iter = mPendingDownloads.begin();
         iter != mPendingDownloads.end();  )
    {
        LLAssetRequest* tmp = *iter++;

        auto cbptr = tmp->mDownCallback.target<void(*)(const LLUUID &, LLAssetType::EType, void *, S32, LLExtStat)>();

        if (type == tmp->getType() && 
            uuid == tmp->getUUID() &&
            (cbptr && (*cbptr == legacyGetDataCallback)) &&
            callback == ((LLLegacyAssetRequest *)tmp->mUserData)->mDownCallback &&
            user_data == ((LLLegacyAssetRequest *)tmp->mUserData)->mUserData)
        {
            // this is a duplicate from the same subsystem - throw it away
            LL_DEBUGS("AssetStorage") << "Discarding duplicate request for UUID " << uuid << LL_ENDL;
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
void LLAssetStorage::legacyGetDataCallback(const LLUUID &uuid, 
                                           LLAssetType::EType type,
                                           void *user_data, 
                                           S32 status, 
                                           LLExtStat ext_status)
{
    LLLegacyAssetRequest *legacy = (LLLegacyAssetRequest *)user_data;
    std::string filename;

    // Check if the asset is marked toxic, and don't load bad stuff
    bool toxic = gAssetStorage->isAssetToxic( uuid );

    if ( !status
         && !toxic )
    {
        LLFileSystem file(uuid, type);

        std::string uuid_str;

        uuid.toString(uuid_str);
        filename = llformat("%s.%s",gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str).c_str(),LLAssetType::lookup(type));

        LLFILE* fp = LLFile::fopen(filename, "wb");     /* Flawfinder: ignore */ 
        if (fp)
        {
            const S32 buf_size = 65536;
            U8 copy_buf[buf_size];
            while (file.read(copy_buf, buf_size))   /* Flawfinder: ignore */
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

    if (status != LL_ERR_NOERR)
    {
        add(sFailedDownloadCount, 1);
    }
    legacy->mDownCallback(filename.c_str(), uuid, legacy->mUserData, status, ext_status);
    delete legacy;
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

// static
void LLAssetStorage::reportMetric( const LLUUID& asset_id, const LLAssetType::EType asset_type, const std::string& in_filename,
                                   const LLUUID& agent_id, S32 asset_size, EMetricResult result,
                                   const char *file, const S32 line, const std::string& in_message )
{
    if( !metric_recipient )
    {
        LL_DEBUGS("AssetStorage") << "Couldn't store LLAssetStoreage::reportMetric - no metrics_recipient" << LL_ENDL;
        return;
    }

    std::string filename(in_filename);
    if (filename.empty())
        filename = ll_safe_string(file);
    
    // Create revised message - new_message = "in_message :: file:line"
    std::stringstream new_message;
    new_message << in_message << " :: " << filename << ":" << line;

    // Change always_report to true if debugging... do not check it in this way
    static bool always_report = false;
    const char *metric_name = "LLAssetStorage::Metrics";

    bool success = result == MR_OKAY;

    if( (!success) || always_report )
    {
        LLSD stats;
        stats["asset_id"] = asset_id;
        stats["asset_type"] = asset_type;
        stats["filename"] = filename;
        stats["agent_id"] = agent_id;
        stats["asset_size"] = (S32)asset_size;
        stats["result"] = (S32)result;

        metric_recipient->recordEventDetails( metric_name, new_message.str(), success, stats);
    }
    else
    {
        metric_recipient->recordEvent(metric_name, new_message.str(), success);
    }
}


// Check if an asset is in the toxic map.  If it is, the entry is updated
bool LLAssetStorage::isAssetToxic( const LLUUID& uuid )
{
    bool is_toxic = false;

    if ( !uuid.isNull() )
    {
        toxic_asset_map_t::iterator iter = mToxicAssetMap.find( uuid );
        if ( iter != mToxicAssetMap.end() )
        {   // Found toxic asset
            (*iter).second = LLFrameTimer::getTotalTime() + TOXIC_ASSET_LIFETIME;
            is_toxic = true;
        }
    }
    return is_toxic;
}




// Clean the toxic asset list, remove old entries
void LLAssetStorage::flushOldToxicAssets( bool force_it )
{
    // Scan and look for old entries
    U64 now = LLFrameTimer::getTotalTime();
    toxic_asset_map_t::iterator iter = mToxicAssetMap.begin();
    while ( iter != mToxicAssetMap.end() )
    {
        if ( force_it
             || (*iter).second < now )
        {   // Too old - remove it
            mToxicAssetMap.erase( iter++ );
        }
        else
        {
            iter++;
        }
    }
}


// Add an item to the toxic asset map
void LLAssetStorage::markAssetToxic( const LLUUID& uuid )
{   
    if ( !uuid.isNull() )
    {
        // Set the value to the current time.  Creates a new entry if needed
        mToxicAssetMap[ uuid ] = LLFrameTimer::getTotalTime() + TOXIC_ASSET_LIFETIME;
    }
}


