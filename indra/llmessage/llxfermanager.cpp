/** 
 * @file llxfermanager.cpp
 * @brief implementation of LLXferManager class for a collection of xfers
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

#include "llxfermanager.h"

#include "llxfer.h"
#include "llxfer_file.h"
#include "llxfer_mem.h"
#include "llxfer_vfile.h"

#include "llerror.h"
#include "lluuid.h"
#include "u64.h"

const F32 LL_XFER_REGISTRATION_TIMEOUT = 60.0f;  // timeout if a registered transfer hasn't been requested in 60 seconds
const F32 LL_PACKET_TIMEOUT = 3.0f;             // packet timeout at 3 s
const S32 LL_PACKET_RETRY_LIMIT = 10;            // packet retransmission limit

const S32 LL_DEFAULT_MAX_SIMULTANEOUS_XFERS = 10;
const S32 LL_DEFAULT_MAX_REQUEST_FIFO_XFERS = 1000;

// Kills the connection if a viewer download queue hits this many requests backed up
// Also set in simulator.xml at "hard_limit_outgoing_xfers_per_circuit"
const S32 LL_DEFAULT_MAX_HARD_LIMIT_SIMULTANEOUS_XFERS = 500;   

// Use this to show sending some ConfirmXferPacket messages
//#define LL_XFER_PROGRESS_MESSAGES 1

// Use this for lots of diagnostic spam
//#define LL_XFER_DIAGNOISTIC_LOGGING 1

///////////////////////////////////////////////////////////

LLXferManager::LLXferManager ()
{
    init();
}

///////////////////////////////////////////////////////////

LLXferManager::~LLXferManager ()
{
    cleanup();
}

///////////////////////////////////////////////////////////

void LLXferManager::init()
{
    cleanup();

    setMaxOutgoingXfersPerCircuit(LL_DEFAULT_MAX_SIMULTANEOUS_XFERS);
    setHardLimitOutgoingXfersPerCircuit(LL_DEFAULT_MAX_HARD_LIMIT_SIMULTANEOUS_XFERS);
    setMaxIncomingXfers(LL_DEFAULT_MAX_REQUEST_FIFO_XFERS);

    // Turn on or off ack throttling
    mUseAckThrottling = FALSE;
    setAckThrottleBPS(100000);
}
    
///////////////////////////////////////////////////////////

void LLXferManager::cleanup ()
{
    for_each(mOutgoingHosts.begin(), mOutgoingHosts.end(), DeletePointer());
    mOutgoingHosts.clear();

    for_each(mSendList.begin(), mSendList.end(), DeletePointer());
    mSendList.clear();

    for_each(mReceiveList.begin(), mReceiveList.end(), DeletePointer());
    mReceiveList.clear();
}

///////////////////////////////////////////////////////////

void LLXferManager::setMaxIncomingXfers(S32 max_num)
{
    mMaxIncomingXfers = max_num;
}

///////////////////////////////////////////////////////////

void LLXferManager::setMaxOutgoingXfersPerCircuit(S32 max_num)
{
    mMaxOutgoingXfersPerCircuit = max_num;
}

void LLXferManager::setHardLimitOutgoingXfersPerCircuit(S32 max_num)
{
    mHardLimitOutgoingXfersPerCircuit = max_num;
}

void LLXferManager::setUseAckThrottling(const BOOL use)
{
    mUseAckThrottling = use;
}

void LLXferManager::setAckThrottleBPS(const F32 bps)
{
    // Let's figure out the min we can set based on the ack retry rate
    // and number of simultaneous.

    // Assuming we're running as slow as possible, this is the lowest ack
    // rate we can use.
    F32 min_bps = (1000.f * 8.f* mMaxIncomingXfers) / LL_PACKET_TIMEOUT;

    // Set
    F32 actual_rate = llmax(min_bps*1.1f, bps);
    LL_DEBUGS("AppInit") << "LLXferManager ack throttle min rate: " << min_bps << LL_ENDL;
    LL_DEBUGS("AppInit") << "LLXferManager ack throttle actual rate: " << actual_rate << LL_ENDL;
    #ifdef LL_XFER_DIAGNOISTIC_LOGGING
    LL_INFOS("Xfer") << "LLXferManager ack throttle min rate: " << min_bps << LL_ENDL;
    LL_INFOS("Xfer") << "LLXferManager ack throttle actual rate: " << actual_rate << LL_ENDL;
    #endif // LL_XFER_DIAGNOISTIC_LOGGING

    mAckThrottle.setRate(actual_rate);
}


///////////////////////////////////////////////////////////

void LLXferManager::updateHostStatus()
{
    // Clear the outgoing host list
    for_each(mOutgoingHosts.begin(), mOutgoingHosts.end(), DeletePointer());
    mOutgoingHosts.clear();
    
    // Loop through all outgoing xfers and re-build mOutgoingHosts
    for (xfer_list_t::iterator send_iter = mSendList.begin();
            send_iter != mSendList.end(); ++send_iter)
    {
        LLHostStatus *host_statusp = NULL;
        for (status_list_t::iterator iter = mOutgoingHosts.begin();
             iter != mOutgoingHosts.end(); ++iter)
        {
            if ((*iter)->mHost == (*send_iter)->mRemoteHost)
            {   // Already have this host
                host_statusp = *iter;
                break;
            }
        }
        if (!host_statusp)
        {   // Don't have this host, so add it
            host_statusp = new LLHostStatus();
            if (host_statusp)
            {
                host_statusp->mHost = (*send_iter)->mRemoteHost;
                mOutgoingHosts.push_front(host_statusp);
            }
        }
        if (host_statusp)
        {   // Do the accounting
            if ((*send_iter)->mStatus == e_LL_XFER_PENDING)
            {
                host_statusp->mNumPending++;
            }
            else if ((*send_iter)->mStatus == e_LL_XFER_IN_PROGRESS)
            {
                host_statusp->mNumActive++;
            }
        }
    }   

#ifdef LL_XFER_DIAGNOISTIC_LOGGING
    for (xfer_list_t::iterator send_iter = mSendList.begin();
            send_iter != mSendList.end(); ++send_iter)
    {
        LLXfer * xferp = *send_iter;
        LL_INFOS("Xfer") << "xfer to host " << xferp->mRemoteHost
            << " is " << xferp->mXferSize << " bytes"
            << ", status " << (S32)(xferp->mStatus)
            << ", waiting for ACK: " << (S32)(xferp->mWaitingForACK)
            << " in frame " << (S32) LLFrameTimer::getFrameCount()
            << LL_ENDL;
    }

    for (status_list_t::iterator iter = mOutgoingHosts.begin();
         iter != mOutgoingHosts.end(); ++iter)
    {
        LL_INFOS("Xfer") << "LLXfer host " << (*iter)->mHost.getIPandPort() 
            << " has " << (*iter)->mNumActive
            << " active, " << (*iter)->mNumPending
            << " pending" 
            << " in frame " << (S32) LLFrameTimer::getFrameCount()
            << LL_ENDL;
    }
#endif // LL_XFER_DIAGNOISTIC_LOGGING

}

///////////////////////////////////////////////////////////

void LLXferManager::printHostStatus()
{
    LLHostStatus *host_statusp = NULL;
    if (!mOutgoingHosts.empty())
    {
        LL_INFOS("Xfer") << "Outgoing Xfers:" << LL_ENDL;

        for (status_list_t::iterator iter = mOutgoingHosts.begin();
             iter != mOutgoingHosts.end(); ++iter)
        {
            host_statusp = *iter;
            LL_INFOS("Xfer") << "    " << host_statusp->mHost << "  active: " << host_statusp->mNumActive << "  pending: " << host_statusp->mNumPending << LL_ENDL;
        }
    }   
}

///////////////////////////////////////////////////////////

LLXfer * LLXferManager::findXferByID(U64 id, xfer_list_t & xfer_list)
{
    for (xfer_list_t::iterator iter = xfer_list.begin();
         iter != xfer_list.end();
         ++iter)
    {
        if ((*iter)->mID == id)
        {
            return(*iter);
        }
    }
    return(NULL);
}


///////////////////////////////////////////////////////////

// WARNING:  this invalidates iterators from xfer_list
void LLXferManager::removeXfer(LLXfer *delp, xfer_list_t & xfer_list)
{
    if (delp)
    {   
        std::string direction = "send";
        if (&xfer_list == &mReceiveList)
        {
            direction = "receive";
        }

        // This assumes that delp will occur in the list once at most
        // Find the pointer in the list
        for (xfer_list_t::iterator iter = xfer_list.begin();
             iter != xfer_list.end();
             ++iter)
        {
            if ((*iter) == delp)
            {
                LL_DEBUGS("Xfer") << "Deleting xfer to host " << (*iter)->mRemoteHost
                    << " of " << (*iter)->mXferSize << " bytes"
                    << ", status " << (S32)((*iter)->mStatus)
                    << " from the " << direction << " list"
                    << LL_ENDL;

                xfer_list.erase(iter);
                delete (delp);
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////

LLHostStatus * LLXferManager::findHostStatus(const LLHost &host)
{
    LLHostStatus *host_statusp = NULL;

    for (status_list_t::iterator iter = mOutgoingHosts.begin();
         iter != mOutgoingHosts.end(); ++iter)
    {
        host_statusp = *iter;
        if (host_statusp->mHost == host)
        {
            return (host_statusp);
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////
 
S32 LLXferManager::numPendingXfers(const LLHost &host)
{
    LLHostStatus *host_statusp = findHostStatus(host);
    if (host_statusp)
    {
        return host_statusp->mNumPending;
    }
    return 0;
}

///////////////////////////////////////////////////////////

S32 LLXferManager::numActiveXfers(const LLHost &host)
{
    LLHostStatus *host_statusp = findHostStatus(host);
    if (host_statusp)
    {
        return host_statusp->mNumActive;
    }
    return 0;
}

///////////////////////////////////////////////////////////

void LLXferManager::changeNumActiveXfers(const LLHost &host, S32 delta)
{
    LLHostStatus *host_statusp = NULL;

    for (status_list_t::iterator iter = mOutgoingHosts.begin();
         iter != mOutgoingHosts.end(); ++iter)
    {
        host_statusp = *iter;
        if (host_statusp->mHost == host)
        {
            host_statusp->mNumActive += delta;
        }
    }
}

///////////////////////////////////////////////////////////

void LLXferManager::registerCallbacks(LLMessageSystem *msgsystem)
{
    msgsystem->setHandlerFuncFast(_PREHASH_ConfirmXferPacket,  process_confirm_packet, NULL);
    msgsystem->setHandlerFuncFast(_PREHASH_RequestXfer,        process_request_xfer,        NULL);
    msgsystem->setHandlerFuncFast(_PREHASH_SendXferPacket,      continue_file_receive,       NULL);
    msgsystem->setHandlerFuncFast(_PREHASH_AbortXfer,       process_abort_xfer,          NULL);
}

///////////////////////////////////////////////////////////

U64 LLXferManager::getNextID ()
{
    LLUUID a_guid;

    a_guid.generate();

    
    return(*((U64*)(a_guid.mData)));
}

///////////////////////////////////////////////////////////

S32 LLXferManager::encodePacketNum(S32 packet_num, BOOL is_EOF)
{
    if (is_EOF)
    {
        packet_num |= 0x80000000;
    }
    return packet_num;
}

///////////////////////////////////////////////////////////

S32 LLXferManager::decodePacketNum(S32 packet_num)
{
    return(packet_num & 0x0FFFFFFF);
}

///////////////////////////////////////////////////////////

BOOL LLXferManager::isLastPacket(S32 packet_num)
{
    return(packet_num & 0x80000000);
}

///////////////////////////////////////////////////////////

U64 LLXferManager::requestFile(const std::string& local_filename,
                                const std::string& remote_filename,
                                ELLPath remote_path,
                                const LLHost& remote_host,
                                BOOL delete_remote_on_completion,
                                void (*callback)(void**,S32,LLExtStat),
                                void** user_data,
                                BOOL is_priority,
                                BOOL use_big_packets)
{
    LLXfer_File* file_xfer_p = NULL;

    // First check to see if it's already requested
    for (xfer_list_t::iterator iter = mReceiveList.begin();
            iter != mReceiveList.end(); ++iter)
    {
        if ((*iter)->getXferTypeTag() == LLXfer::XFER_FILE)
        {
            file_xfer_p = (LLXfer_File*)(*iter);
            if (file_xfer_p->matchesLocalFilename(local_filename)
                && file_xfer_p->matchesRemoteFilename(remote_filename, remote_path)
                && (remote_host == file_xfer_p->mRemoteHost)
                && (callback == file_xfer_p->mCallback)
                && (user_data == file_xfer_p->mCallbackDataHandle))
            {
                // Already have the request (already in progress)
                return (*iter)->mID;
            }
        }
    }

    U64 xfer_id = 0;

    S32 chunk_size = use_big_packets ? LL_XFER_LARGE_PAYLOAD : -1;
    file_xfer_p = new LLXfer_File(chunk_size);
    if (file_xfer_p)
    {
        addToList(file_xfer_p, mReceiveList, is_priority);

        // Remove any file by the same name that happens to be lying
        // around.
        // Note: according to AaronB, this is here to deal with locks on files that were
        // in transit during a crash,
        if(delete_remote_on_completion &&
           (remote_filename.substr(remote_filename.length()-4) == ".tmp"))
        {
            LLFile::remove(local_filename, ENOENT);
        }
        xfer_id = getNextID();
        file_xfer_p->initializeRequest(
            xfer_id,
            local_filename,
            remote_filename,
            remote_path,
            remote_host,
            delete_remote_on_completion,
            callback,user_data);
        startPendingDownloads();
    }
    else
    {
        LL_ERRS("Xfer") << "Xfer allocation error" << LL_ENDL;
    }
    return xfer_id;
}

void LLXferManager::requestVFile(const LLUUID& local_id,
                                 const LLUUID& remote_id,
                                 LLAssetType::EType type,
                                 const LLHost& remote_host,
                                 void (*callback)(void**,S32,LLExtStat),
                                 void** user_data,
                                 BOOL is_priority)
{
    LLXfer_VFile * xfer_p = NULL;

    for (xfer_list_t::iterator iter = mReceiveList.begin();
            iter != mReceiveList.end(); ++iter)
    {   // Find any matching existing requests
        if ((*iter)->getXferTypeTag() == LLXfer::XFER_VFILE)
        {
            xfer_p = (LLXfer_VFile*) (*iter);
            if (xfer_p->matchesLocalFile(local_id, type)
                && xfer_p->matchesRemoteFile(remote_id, type)
                && (remote_host == xfer_p->mRemoteHost)
                && (callback == xfer_p->mCallback)
                && (user_data == xfer_p->mCallbackDataHandle))

            {   // Have match, don't add a duplicate
                #ifdef LL_XFER_DIAGNOISTIC_LOGGING
                LL_INFOS("Xfer") << "Dropping duplicate xfer request for " << remote_id
                    << " on " << remote_host.getIPandPort()
                    << " local id " << local_id
                    << LL_ENDL;
                #endif  // LL_XFER_DIAGNOISTIC_LOGGING

                return;
            }
        }
    }

    xfer_p = new LLXfer_VFile();
    if (xfer_p)
    {
        #ifdef LL_XFER_DIAGNOISTIC_LOGGING
        LL_INFOS("Xfer") << "Starting file xfer for " << remote_id
            << " type " << LLAssetType::lookupHumanReadable(type)
            << " from " << xfer_p->mRemoteHost.getIPandPort()
            << ", local id " << local_id
            << LL_ENDL;
        #endif  // LL_XFER_DIAGNOISTIC_LOGGING

        addToList(xfer_p, mReceiveList, is_priority);
        ((LLXfer_VFile *)xfer_p)->initializeRequest(getNextID(),
            local_id,
            remote_id,
            type,
            remote_host,
            callback,
            user_data);
        startPendingDownloads();
    }
    else
    {
        LL_ERRS("Xfer") << "Xfer allocation error" << LL_ENDL;
    }

}

///////////////////////////////////////////////////////////

void LLXferManager::processReceiveData (LLMessageSystem *mesgsys, void ** /*user_data*/)
{
    // there's sometimes an extra 4 bytes added to an xfer payload
    const S32 BUF_SIZE = LL_XFER_LARGE_PAYLOAD + 4;
    char fdata_buf[BUF_SIZE];       /* Flawfinder : ignore */
    S32 fdata_size;
    U64 id;
    S32 packetnum;
    LLXfer * xferp;
    
    mesgsys->getU64Fast(_PREHASH_XferID, _PREHASH_ID, id);
    mesgsys->getS32Fast(_PREHASH_XferID, _PREHASH_Packet, packetnum);

    fdata_size = mesgsys->getSizeFast(_PREHASH_DataPacket,_PREHASH_Data);
    if (fdata_size < 0 ||
        fdata_size > BUF_SIZE)
    {
        char U64_BUF[MAX_STRING];       /* Flawfinder : ignore */
        LL_WARNS("Xfer") << "Received invalid xfer data size of " << fdata_size
            << " in packet number " << packetnum 
            << " from " << mesgsys->getSender()
            << " for xfer id: " << U64_to_str(id, U64_BUF, sizeof(U64_BUF)) 
            << LL_ENDL;
        return;
    }
    mesgsys->getBinaryDataFast(_PREHASH_DataPacket, _PREHASH_Data, fdata_buf, fdata_size, 0, BUF_SIZE);

    xferp = findXferByID(id, mReceiveList);
    if (!xferp)
    {
        char U64_BUF[MAX_STRING];       /* Flawfinder : ignore */
        LL_WARNS("Xfer") << "received xfer data from " << mesgsys->getSender()
            << " for non-existent xfer id: "
            << U64_to_str(id, U64_BUF, sizeof(U64_BUF)) << LL_ENDL;
        return;
    }

    S32 xfer_size;

    if (decodePacketNum(packetnum) != xferp->mPacketNum) // is the packet different from what we were expecting?
    {
        // confirm it if it was a resend of the last one, since the confirmation might have gotten dropped
        if (decodePacketNum(packetnum) == (xferp->mPacketNum - 1))
        {
            LL_INFOS("Xfer") << "Reconfirming xfer " << xferp->mRemoteHost << ":" << xferp->getFileName() << " packet " << packetnum << LL_ENDL;            sendConfirmPacket(mesgsys, id, decodePacketNum(packetnum), mesgsys->getSender());
        }
        else
        {
            LL_INFOS("Xfer") << "Ignoring xfer " << xferp->mRemoteHost << ":" << xferp->getFileName() << " recv'd packet " << packetnum << "; expecting " << xferp->mPacketNum << LL_ENDL;
        }
        return;     
    }

    S32 result = 0;

    if (xferp->mPacketNum == 0) // first packet has size encoded as additional S32 at beginning of data
    {
        ntohmemcpy(&xfer_size,fdata_buf,MVT_S32,sizeof(S32));
        
// do any necessary things on first packet ie. allocate memory
        xferp->setXferSize(xfer_size);

        // adjust buffer start and size
        result = xferp->receiveData(&(fdata_buf[sizeof(S32)]),fdata_size-(sizeof(S32)));
    }
    else
    {
        result = xferp->receiveData(fdata_buf,fdata_size);
    }
    
    if (result == LL_ERR_CANNOT_OPEN_FILE)
    {
            xferp->abort(LL_ERR_CANNOT_OPEN_FILE);
            removeXfer(xferp,mReceiveList);
            startPendingDownloads();
            return;     
    }

    xferp->mPacketNum++;  // expect next packet

    if (!mUseAckThrottling)
    {
        // No throttling, confirm right away
        sendConfirmPacket(mesgsys, id, decodePacketNum(packetnum), mesgsys->getSender());
    }
    else
    {
        // Throttling, put on queue to be confirmed later.
        LLXferAckInfo ack_info;
        ack_info.mID = id;
        ack_info.mPacketNum = decodePacketNum(packetnum);
        ack_info.mRemoteHost = mesgsys->getSender();
        mXferAckQueue.push_back(ack_info);
    }

    if (isLastPacket(packetnum))
    {
        xferp->processEOF();
        removeXfer(xferp,mReceiveList);
        startPendingDownloads();
    }
}

///////////////////////////////////////////////////////////

void LLXferManager::sendConfirmPacket (LLMessageSystem *mesgsys, U64 id, S32 packetnum, const LLHost &remote_host)
{
#ifdef LL_XFER_PROGRESS_MESSAGES
    if (!(packetnum % 50))
    {
        cout << "confirming xfer packet #" << packetnum << endl;
    }
#endif
    mesgsys->newMessageFast(_PREHASH_ConfirmXferPacket);
    mesgsys->nextBlockFast(_PREHASH_XferID);
    mesgsys->addU64Fast(_PREHASH_ID, id);
    mesgsys->addU32Fast(_PREHASH_Packet, packetnum);

    // Ignore a circuit failure here, we'll catch it with another message
    mesgsys->sendMessage(remote_host);
}

///////////////////////////////////////////////////////////

static bool find_and_remove(std::multiset<std::string>& files,
        const std::string& filename)
{
    std::multiset<std::string>::iterator ptr;
    if ( (ptr = files.find(filename)) != files.end())
    {
        //erase(filename) erases *all* entries with that key
        files.erase(ptr);
        return true;
    }
    return false;
}

void LLXferManager::expectFileForRequest(const std::string& filename)
{
    mExpectedRequests.insert(filename);
}

bool LLXferManager::validateFileForRequest(const std::string& filename)
{
    return find_and_remove(mExpectedRequests, filename);
}

void LLXferManager::expectFileForTransfer(const std::string& filename)
{
    mExpectedTransfers.insert(filename);
}

bool LLXferManager::validateFileForTransfer(const std::string& filename)
{
    return find_and_remove(mExpectedTransfers, filename);
}

/* Present in fireengine, not used by viewer
void LLXferManager::expectVFileForRequest(const std::string& filename)
{
    mExpectedVFileRequests.insert(filename);
}

bool LLXferManager::validateVFileForRequest(const std::string& filename)
{
    return find_and_remove(mExpectedVFileRequests, filename);
}

void LLXferManager::expectVFileForTransfer(const std::string& filename)
{
    mExpectedVFileTransfers.insert(filename);
}

bool LLXferManager::validateVFileForTransfer(const std::string& filename)
{
    return find_and_remove(mExpectedVFileTransfers, filename);
}
*/

static bool remove_prefix(std::string& filename, const std::string& prefix)
{
    if (std::equal(prefix.begin(), prefix.end(), filename.begin()))
    {
        filename = filename.substr(prefix.length());
        return true;
    }
    return false;
}

static bool verify_cache_filename(const std::string& filename)
{
    //NOTE: This routine is only used to check file names that our own
    // code places in the cache directory.  As such, it can be limited
    // to this very restrictive file name pattern.  It does not need to
    // handle other characters. The only known uses of this are (with examples):
    //  sim to sim object pass:         fc0b72d8-9456-63d9-a802-a557ef847313.tmp
    //  sim to viewer mute list:        mute_b78eacd0-1244-448e-93ca-28ede242f647.tmp
    //  sim to viewer task inventory:   inventory_d8ab59d2-baf0-0e79-c4c2-a3f99b9fcf45.tmp
    
    //IMPORTANT: Do not broaden the filenames accepted by this routine
    // without careful analysis. Anything allowed by this function can
    // be downloaded by the viewer.
    
    size_t len = filename.size();
    //const boost::regex expr("[0-9a-zA-Z_-]<1,46>\.tmp");
    if (len < 5 || len > 50)
    {   
        return false;
    }
    for(size_t i=0; i<(len-4); ++i)
    {   
        char c = filename[i];
        bool ok = isalnum(c) || '_'==c || '-'==c;
        if (!ok)
        {
            return false;
        }
    }
    return filename[len-4] == '.'
        && filename[len-3] == 't'
        && filename[len-2] == 'm'
        && filename[len-1] == 'p';
}

void LLXferManager::processFileRequest (LLMessageSystem *mesgsys, void ** /*user_data*/)
{
        
    U64 id;
    std::string local_filename;
    ELLPath local_path = LL_PATH_NONE;
    S32 result = LL_ERR_NOERR;
    LLUUID  uuid;
    LLAssetType::EType type;
    S16 type_s16;
    BOOL b_use_big_packets;

    mesgsys->getBOOL("XferID", "UseBigPackets", b_use_big_packets);
    
    mesgsys->getU64Fast(_PREHASH_XferID, _PREHASH_ID, id);
    char U64_BUF[MAX_STRING];       /* Flawfinder : ignore */
    LL_INFOS("Xfer") << "xfer request id: " << U64_to_str(id, U64_BUF, sizeof(U64_BUF))
           << " to " << mesgsys->getSender() << LL_ENDL;

    mesgsys->getStringFast(_PREHASH_XferID, _PREHASH_Filename, local_filename);
    
    {
        U8 local_path_u8;
        mesgsys->getU8("XferID", "FilePath", local_path_u8);
        local_path = (ELLPath)local_path_u8;
    }

    mesgsys->getUUIDFast(_PREHASH_XferID, _PREHASH_VFileID, uuid);
    mesgsys->getS16Fast(_PREHASH_XferID, _PREHASH_VFileType, type_s16);
    type = (LLAssetType::EType)type_s16;

    LLXfer *xferp;

    if (uuid != LLUUID::null)
    {   // Request for an asset - use a cache file
        if(NULL == LLAssetType::lookup(type))
        {
            LL_WARNS("Xfer") << "Invalid type for xfer request: " << uuid << ":"
                    << type_s16 << " to " << mesgsys->getSender() << LL_ENDL;
            return;
        }

        LL_INFOS("Xfer") << "starting vfile transfer: " << uuid << "," << LLAssetType::lookup(type) << " to " << mesgsys->getSender() << LL_ENDL;

        xferp = (LLXfer *)new LLXfer_VFile(uuid, type);
        if (xferp)
        {
            mSendList.push_front(xferp);
            result = xferp->startSend(id,mesgsys->getSender());
        }
        else
        {
            LL_ERRS("Xfer") << "Xfer allcoation error" << LL_ENDL;
        }
    }
    else if (!local_filename.empty())
    {   // Was given a file name to send
        // See DEV-21775 for detailed security issues

        if (local_path == LL_PATH_NONE)
        {
            // this handles legacy simulators that are passing objects
            // by giving a filename that explicitly names the cache directory
            static const std::string legacy_cache_prefix = "data/";
            if (remove_prefix(local_filename, legacy_cache_prefix))
            {
                local_path = LL_PATH_CACHE;
            }
        }

        switch (local_path)
        {
            case LL_PATH_NONE:
                if(!validateFileForTransfer(local_filename))
                {
                    LL_WARNS("Xfer") << "SECURITY: Unapproved filename '" << local_filename << LL_ENDL;
                    return;
                }
                break;

            case LL_PATH_CACHE:
                if(!verify_cache_filename(local_filename))
                {
                    LL_WARNS("Xfer") << "SECURITY: Illegal cache filename '" << local_filename << LL_ENDL;
                    return;
                }
                break;

            default:
                LL_WARNS("Xfer") << "SECURITY: Restricted file dir enum: " << (U32)local_path << LL_ENDL;
                return;
        }

        // If we want to use a special path (e.g. LL_PATH_CACHE), we want to make sure we create the
        // proper expanded filename.
        std::string expanded_filename;
        if (local_path != LL_PATH_NONE)
        {
            expanded_filename = gDirUtilp->getExpandedFilename( local_path, local_filename );
        }
        else
        {
            expanded_filename = local_filename;
        }
        LL_INFOS("Xfer") << "starting file transfer: " <<  expanded_filename << " to " << mesgsys->getSender() << LL_ENDL;

        BOOL delete_local_on_completion = FALSE;
        mesgsys->getBOOL("XferID", "DeleteOnCompletion", delete_local_on_completion);

        // -1 chunk_size causes it to use the default
        xferp = (LLXfer *)new LLXfer_File(expanded_filename, delete_local_on_completion, b_use_big_packets ? LL_XFER_LARGE_PAYLOAD : -1);
        
        if (xferp)
        {
            mSendList.push_front(xferp);
            result = xferp->startSend(id,mesgsys->getSender());
        }
        else
        {
            LL_ERRS("Xfer") << "Xfer allcoation error" << LL_ENDL;
        }
    }
    else
    {   // no uuid or filename - use the ID sent
        char U64_BUF[MAX_STRING];       /* Flawfinder : ignore */
        LL_INFOS("Xfer") << "starting memory transfer: "
            << U64_to_str(id, U64_BUF, sizeof(U64_BUF)) << " to "
            << mesgsys->getSender() << LL_ENDL;

        xferp = findXferByID(id, mSendList);
        
        if (xferp)
        {
            result = xferp->startSend(id,mesgsys->getSender());
        }
        else
        {
            LL_INFOS("Xfer") << "Warning: xfer ID " << U64_BUF << " not found." << LL_ENDL;
            result = LL_ERR_FILE_NOT_FOUND;
        }
    }

    if (result)
    {
        if (xferp)
        {
            xferp->abort(result);
            removeXfer(xferp, mSendList);
        }
        else // can happen with a memory transfer not found
        {
            LL_INFOS("Xfer") << "Aborting xfer to " << mesgsys->getSender() << " with error: " << result << LL_ENDL;

            mesgsys->newMessageFast(_PREHASH_AbortXfer);
            mesgsys->nextBlockFast(_PREHASH_XferID);
            mesgsys->addU64Fast(_PREHASH_ID, id);
            mesgsys->addS32Fast(_PREHASH_Result, result);
    
            mesgsys->sendMessage(mesgsys->getSender());     
        }
    }
    else if(xferp)
    {
        // Figure out how many transfers the host has requested
        LLHostStatus *host_statusp = findHostStatus(xferp->mRemoteHost);
        if (host_statusp)
        {
            if (host_statusp->mNumActive < mMaxOutgoingXfersPerCircuit)
            {   // Not many transfers in progress already, so start immediately
                xferp->sendNextPacket();
                changeNumActiveXfers(xferp->mRemoteHost,1);
                LL_DEBUGS("Xfer") << "Starting xfer ID " << U64_to_str(id) << " immediately" << LL_ENDL;
            }
            else if (mHardLimitOutgoingXfersPerCircuit == 0 ||
                     (host_statusp->mNumActive + host_statusp->mNumPending) < mHardLimitOutgoingXfersPerCircuit)
            {   // Must close the file handle and wait for earlier ones to complete
                LL_INFOS("Xfer") << "  queueing xfer request id " << U64_to_str(id) << ", " 
                                 << host_statusp->mNumActive << " active and "
                                 << host_statusp->mNumPending << " pending ahead of this one" 
                                 << LL_ENDL;
                xferp->closeFileHandle();   // Close the file handle until we're ready to send again
            }
            else if (mHardLimitOutgoingXfersPerCircuit > 0)
            {   // Way too many requested ... it's time to stop being nice and kill the circuit
                xferp->closeFileHandle();   // Close the file handle in any case
                LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(xferp->mRemoteHost);
                if (cdp)
                {
                    if (cdp->getTrusted())
                    {   // Trusted internal circuit - don't kill it
                        LL_WARNS("Xfer") << "Trusted circuit to " << xferp->mRemoteHost << " has too many xfer requests in the queue " 
                                        << host_statusp->mNumActive << " active and "
                                        << host_statusp->mNumPending << " pending ahead of this one" 
                                        << LL_ENDL;
                    }
                    else
                    {   // Untrusted circuit - time to stop messing around and kill it
                        LL_WARNS("Xfer") << "Killing circuit to " << xferp->mRemoteHost << " for having too many xfer requests in the queue " 
                                        << host_statusp->mNumActive << " active and "
                                        << host_statusp->mNumPending << " pending ahead of this one" 
                                        << LL_ENDL;
                        gMessageSystem->disableCircuit(xferp->mRemoteHost);
                    }
                }
                else
                {   // WTF?   Why can't we find a circuit?  Try to kill it off
                    LL_WARNS("Xfer") << "Backlog with circuit to " << xferp->mRemoteHost << " with too many xfer requests in the queue " 
                                    << host_statusp->mNumActive << " active and "
                                    << host_statusp->mNumPending << " pending ahead of this one" 
                                    << " but no LLCircuitData found???"
                                    << LL_ENDL;
                    gMessageSystem->disableCircuit(xferp->mRemoteHost);
                }
            }
        }
        else
        {
            LL_WARNS("Xfer") << "LLXferManager::processFileRequest() - no LLHostStatus found for id " << U64_to_str(id) 
                << " host " << xferp->mRemoteHost << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS("Xfer") << "LLXferManager::processFileRequest() - no xfer found for id " << U64_to_str(id) << LL_ENDL;
    }
}
 
///////////////////////////////////////////////////////////

// Return true if host is in a transfer-flood sitation.  Same check for both internal and external hosts
bool LLXferManager::isHostFlooded(const LLHost & host)
{
    bool flooded = false;
    LLHostStatus *host_statusp = findHostStatus(host);
    if (host_statusp)
    {
        flooded = (mHardLimitOutgoingXfersPerCircuit > 0 &&
                    (host_statusp->mNumActive + host_statusp->mNumPending) >= (S32)(mHardLimitOutgoingXfersPerCircuit * 0.8f));
    }

    return flooded;
}


///////////////////////////////////////////////////////////

void LLXferManager::processConfirmation (LLMessageSystem *mesgsys, void ** /*user_data*/)
{
    U64 id = 0;
    S32 packetNum = 0;

    mesgsys->getU64Fast(_PREHASH_XferID, _PREHASH_ID, id);
    mesgsys->getS32Fast(_PREHASH_XferID, _PREHASH_Packet, packetNum);

    LLXfer* xferp = findXferByID(id, mSendList);
    if (xferp)
    {
//      cout << "confirmed packet #" << packetNum << " ping: "<< xferp->ACKTimer.getElapsedTimeF32() <<  endl;
        xferp->mWaitingForACK = FALSE;
        if (xferp->mStatus == e_LL_XFER_IN_PROGRESS)
        {
            xferp->sendNextPacket();
        }
        else
        {
            removeXfer(xferp, mSendList);
        }
    }
}

///////////////////////////////////////////////////////////

// Called from LLMessageSystem::processAcks()
void LLXferManager::retransmitUnackedPackets()
{
    LLXfer *xferp;

    xfer_list_t::iterator iter = mReceiveList.begin();
    while (iter != mReceiveList.end())
    {
        xferp = (*iter);
        if (xferp->mStatus == e_LL_XFER_IN_PROGRESS)
        {
            // if the circuit dies, abort
            if (! gMessageSystem->mCircuitInfo.isCircuitAlive( xferp->mRemoteHost ))
            {
                LL_WARNS("Xfer") << "Xfer found in progress on dead circuit, aborting transfer to " 
                    << xferp->mRemoteHost.getIPandPort()
                    << LL_ENDL;
                xferp->mCallbackResult = LL_ERR_CIRCUIT_GONE;
                xferp->processEOF();

                iter = mReceiveList.erase(iter);    // iter is set to next one after the deletion point
                delete (xferp);
                continue;
            }
                
        }
        ++iter;
    }

    // Re-build mOutgoingHosts data
    updateHostStatus();

    F32 et;
    iter = mSendList.begin();
    while (iter != mSendList.end())
    {
        xferp = (*iter);
        if (xferp->mWaitingForACK && ( (et = xferp->ACKTimer.getElapsedTimeF32()) > LL_PACKET_TIMEOUT))
        {
            if (xferp->mRetries > LL_PACKET_RETRY_LIMIT)
            {
                LL_INFOS("Xfer") << "dropping xfer " << xferp->mRemoteHost << ":" << xferp->getFileName() << " packet retransmit limit exceeded, xfer dropped" << LL_ENDL;
                xferp->abort(LL_ERR_TCP_TIMEOUT);
                iter = mSendList.erase(iter);
                delete xferp;
                continue;
            }
            else
            {
                LL_INFOS("Xfer") << "resending xfer " << xferp->mRemoteHost << ":" << xferp->getFileName() << " packet unconfirmed after: "<< et << " sec, packet " << xferp->mPacketNum << LL_ENDL;
                xferp->resendLastPacket();
            }
        }
        else if ((xferp->mStatus == e_LL_XFER_REGISTERED) && ( (et = xferp->ACKTimer.getElapsedTimeF32()) > LL_XFER_REGISTRATION_TIMEOUT))
        {
            LL_INFOS("Xfer") << "registered xfer never requested, xfer dropped" << LL_ENDL;
            xferp->abort(LL_ERR_TCP_TIMEOUT);
            iter = mSendList.erase(iter);
            delete xferp;
            continue;
        }
        else if (xferp->mStatus == e_LL_XFER_ABORTED)
        {
            LL_WARNS("Xfer") << "Removing aborted xfer " << xferp->mRemoteHost << ":" << xferp->getFileName() << LL_ENDL;
            iter = mSendList.erase(iter);
            delete xferp;
            continue;
        }
        else if (xferp->mStatus == e_LL_XFER_PENDING)
        {
//          LL_INFOS("Xfer") << "*** numActiveXfers = " << numActiveXfers(xferp->mRemoteHost) << "        mMaxOutgoingXfersPerCircuit = " << mMaxOutgoingXfersPerCircuit << LL_ENDL;   
            if (numActiveXfers(xferp->mRemoteHost) < mMaxOutgoingXfersPerCircuit)
            {
                if (xferp->reopenFileHandle())
                {
                    LL_WARNS("Xfer") << "Error re-opening file handle for xfer ID " << U64_to_str(xferp->mID)
                        << " to host " << xferp->mRemoteHost << LL_ENDL;
                    xferp->abort(LL_ERR_CANNOT_OPEN_FILE);
                    iter = mSendList.erase(iter);
                    delete xferp;
                    continue;
                }
                else
                {   // No error re-opening the file, send the first packet
                    LL_DEBUGS("Xfer") << "Moving pending xfer ID " << U64_to_str(xferp->mID) << " to active" << LL_ENDL;
                    xferp->sendNextPacket();
                    changeNumActiveXfers(xferp->mRemoteHost,1);
                }
            }
        }
        ++iter;
    } // end while() loop

    //
    // HACK - if we're using xfer confirm throttling, throttle our xfer confirms here
    // so we don't blow through bandwidth.
    //

    while (mXferAckQueue.size())
    {
        if (mAckThrottle.checkOverflow(1000.0f*8.0f))
        {
            break;
        }
        //LL_INFOS("Xfer") << "Confirm packet queue length:" << mXferAckQueue.size() << LL_ENDL;
        LLXferAckInfo ack_info = mXferAckQueue.front();
        mXferAckQueue.pop_front();
        //LL_INFOS("Xfer") << "Sending confirm packet" << LL_ENDL;
        sendConfirmPacket(gMessageSystem, ack_info.mID, ack_info.mPacketNum, ack_info.mRemoteHost);
        mAckThrottle.throttleOverflow(1000.f*8.f); // Assume 1000 bytes/packet
    }
}

///////////////////////////////////////////////////////////

void LLXferManager::abortRequestById(U64 xfer_id, S32 result_code)
{
    LLXfer * xferp = findXferByID(xfer_id, mReceiveList);
    if (xferp)
    {
        if (xferp->mStatus == e_LL_XFER_IN_PROGRESS)
        {
            // causes processAbort();
            xferp->abort(result_code);
        }
        else
        {
            xferp->mCallbackResult = result_code;
            xferp->processEOF(); //should notify requester
            removeXfer(xferp, mReceiveList);
        }
        // Since already removed or marked as aborted no need
        // to wait for processAbort() to start new download
        startPendingDownloads();
    }
}

///////////////////////////////////////////////////////////

void LLXferManager::processAbort (LLMessageSystem *mesgsys, void ** /*user_data*/)
{
    U64 id = 0;
    S32 result_code = 0;
    LLXfer * xferp;

    mesgsys->getU64Fast(_PREHASH_XferID, _PREHASH_ID, id);
    mesgsys->getS32Fast(_PREHASH_XferID, _PREHASH_Result, result_code);

    xferp = findXferByID(id, mReceiveList);
    if (xferp)
    {
        xferp->mCallbackResult = result_code;
        xferp->processEOF();
        removeXfer(xferp, mReceiveList);
        startPendingDownloads();
    }
}

///////////////////////////////////////////////////////////

void LLXferManager::startPendingDownloads()
{
    // This method goes through the list, and starts pending
    // operations until active downloads == mMaxIncomingXfers. I copy
    // the pending xfers into a temporary data structure because the
    // xfers are stored as an intrusive linked list where older
    // requests get pushed toward the back. Thus, if we didn't do a
    // stateful iteration, it would be possible for old requests to
    // never start.
    LLXfer* xferp;
    std::list<LLXfer*> pending_downloads;
    S32 download_count = 0;
    S32 pending_count = 0;
    for (xfer_list_t::iterator iter = mReceiveList.begin();
         iter != mReceiveList.end();
         ++iter)
    {
        xferp = (*iter);
        if(xferp->mStatus == e_LL_XFER_PENDING)
        {   // Count and accumulate pending downloads
            ++pending_count;
            pending_downloads.push_front(xferp);
        }
        else if(xferp->mStatus == e_LL_XFER_IN_PROGRESS)
        {   // Count downloads in progress
            ++download_count;
        }
    }

    S32 start_count = mMaxIncomingXfers - download_count;

    LL_DEBUGS("Xfer") << "LLXferManager::startPendingDownloads() - XFER_IN_PROGRESS: "
             << download_count << " XFER_PENDING: " << pending_count
             << " startring " << llmin(start_count, pending_count) << LL_ENDL;

    if((start_count > 0) && (pending_count > 0))
    {
        S32 result;
        for (std::list<LLXfer*>::iterator iter = pending_downloads.begin();
             iter != pending_downloads.end(); ++iter)
        {
            xferp = *iter;
            if (start_count-- <= 0)
                break;
            result = xferp->startDownload();
            if(result)
            {
                xferp->abort(result);
                ++start_count;
            }
        }
    }
}

///////////////////////////////////////////////////////////

void LLXferManager::addToList(LLXfer* xferp, xfer_list_t & xfer_list, BOOL is_priority)
{
    if(is_priority)
    {
        xfer_list.push_back(xferp);
    }
    else
    {
        xfer_list.push_front(xferp);
    }
}

///////////////////////////////////////////////////////////
//  Globals and C routines
///////////////////////////////////////////////////////////

LLXferManager *gXferManager = NULL;


void start_xfer_manager()
{
    gXferManager = new LLXferManager();
}

void cleanup_xfer_manager()
{
    if (gXferManager)
    {
        delete(gXferManager);
        gXferManager = NULL;
    }
}

void process_confirm_packet (LLMessageSystem *mesgsys, void **user_data)
{
    gXferManager->processConfirmation(mesgsys,user_data);
}

void process_request_xfer(LLMessageSystem *mesgsys, void **user_data)
{
    gXferManager->processFileRequest(mesgsys,user_data);
}

void continue_file_receive(LLMessageSystem *mesgsys, void **user_data)
{
#if LL_TEST_XFER_REXMIT
    if (ll_frand() > 0.05f)
    {
#endif
        gXferManager->processReceiveData(mesgsys,user_data);
#if LL_TEST_XFER_REXMIT
    }
    else
    {
        cout << "oops! dropped a xfer packet" << endl;
    }
#endif
}

void process_abort_xfer(LLMessageSystem *mesgsys, void **user_data)
{
    gXferManager->processAbort(mesgsys,user_data);
}


























