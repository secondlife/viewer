/** 
 * @file llxfermanager.h
 * @brief definition of LLXferManager class for a keeping track of
 * multiple xfers
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

#ifndef LL_LLXFERMANAGER_H
#define LL_LLXFERMANAGER_H

/**
 * this manager keeps both a send list and a receive list; anything with a 
 * LLXferManager can send and receive files via messages
 */

//Forward declaration to avoid circular dependencies
class LLXfer;

#include "llxfer.h"
#include "message.h"
#include "llassetstorage.h"
#include "lldir.h"
#include <deque>
#include "llthrottle.h"

class LLHostStatus
{
 public:
    LLHost mHost;
    S32    mNumActive;
    S32    mNumPending;

    LLHostStatus() {mNumActive = 0; mNumPending = 0;};
    virtual ~LLHostStatus(){};
};

// Class stores ack information, to be put on list so we can throttle xfer rate.
class LLXferAckInfo
{
public:
    LLXferAckInfo(U32 dummy = 0)
    {
        mID = 0;
        mPacketNum = -1;
    }

    U64 mID;
    S32 mPacketNum;
    LLHost mRemoteHost;
};

class LLXferManager
{
 protected:
    S32    mMaxOutgoingXfersPerCircuit;
    S32    mHardLimitOutgoingXfersPerCircuit;   // At this limit, kill off the connection
    S32    mMaxIncomingXfers;

    BOOL    mUseAckThrottling; // Use ack throttling to cap file xfer bandwidth
    std::deque<LLXferAckInfo> mXferAckQueue;
    LLThrottle mAckThrottle;
 public:

    // This enumeration is useful in the requestFile() to specify if
    // an xfer must happen asap.
    enum
    {
        LOW_PRIORITY = FALSE,
        HIGH_PRIORITY = TRUE,
    };

    // Linked FIFO list, add to the front and pull from back
    typedef std::deque<LLXfer *> xfer_list_t;
    xfer_list_t     mSendList;
    xfer_list_t     mReceiveList;

    typedef std::list<LLHostStatus*> status_list_t;
    status_list_t mOutgoingHosts;

 protected:
    // implementation methods
    virtual void startPendingDownloads();
    virtual void addToList(LLXfer* xferp, xfer_list_t & xfer_list, BOOL is_priority);
    std::multiset<std::string> mExpectedTransfers; // files that are authorized to transfer out
    std::multiset<std::string> mExpectedRequests;  // files that are authorized to be downloaded on top of
    std::multiset<std::string> mExpectedVFileTransfers; // files that are authorized to transfer out
    std::multiset<std::string> mExpectedVFileRequests;  // files that are authorized to be downloaded on top of

 public:
    LLXferManager();
    virtual ~LLXferManager();

    virtual void init();
    virtual void cleanup();

    void setUseAckThrottling(const BOOL use);
    void setAckThrottleBPS(const F32 bps);

// list management routines
    virtual LLXfer *findXferByID(U64 id, xfer_list_t & xfer_list);
    virtual void removeXfer (LLXfer *delp, xfer_list_t & xfer_list);

    LLHostStatus * findHostStatus(const LLHost &host);
    virtual S32 numActiveXfers(const LLHost &host);
    virtual S32 numPendingXfers(const LLHost &host);

    virtual void changeNumActiveXfers(const LLHost &host, S32 delta);

    virtual void setMaxOutgoingXfersPerCircuit (S32 max_num);
    virtual void setHardLimitOutgoingXfersPerCircuit(S32 max_num);
    virtual void setMaxIncomingXfers(S32 max_num);
    virtual void updateHostStatus();
    virtual void printHostStatus();

// general utility routines
    virtual void registerCallbacks(LLMessageSystem *mesgsys);
    virtual U64 getNextID ();
    virtual S32 encodePacketNum(S32 packet_num, BOOL is_eof);   
    virtual S32 decodePacketNum(S32 packet_num);    
    virtual BOOL isLastPacket(S32 packet_num);

// file requesting routines
// .. to file
    virtual U64 requestFile(const std::string& local_filename,
                             const std::string& remote_filename,
                             ELLPath remote_path,
                             const LLHost& remote_host,
                             BOOL delete_remote_on_completion,
                             void (*callback)(void**,S32,LLExtStat), void** user_data,
                             BOOL is_priority = FALSE,
                             BOOL use_big_packets = FALSE);
    /*
// .. to memory
    virtual void requestFile(const std::string& remote_filename, 
                             ELLPath remote_path,
                             const LLHost &remote_host,
                             BOOL delete_remote_on_completion,
                             void (*callback)(void*, S32, void**, S32, LLExtStat),
                             void** user_data,
                             BOOL is_priority = FALSE);
    */
// vfile requesting
// .. to vfile
    virtual void requestVFile(const LLUUID &local_id, const LLUUID& remote_id,
                              LLAssetType::EType type,
                              const LLHost& remote_host,
                              void (*callback)(void**,S32,LLExtStat), void** user_data,
                              BOOL is_priority = FALSE);
    /**
        When arbitrary files are requested to be transfered (by giving a dir of LL_PATH_NONE)
       they must be "expected", but having something pre-authorize them. This pair of functions
       maintains a pre-authorized list. The first function adds something to the list, the second
       checks if is authorized, removing it if so.  In this way, a file is only authorized for
       a single use.
    */
    virtual void expectFileForTransfer(const std::string& filename);
    virtual bool validateFileForTransfer(const std::string& filename);
    /**
        Same idea, but for the viewer about to call InitiateDownload to track what it requested.
    */
    virtual void expectFileForRequest(const std::string& filename);
    virtual bool validateFileForRequest(const std::string& filename);

    /**
        Same idea but for VFiles, kept separate to avoid namespace overlap
    */
    /* Present in fireengine, not used by viewer
    virtual void expectVFileForTransfer(const std::string& filename);
    virtual bool validateVFileForTransfer(const std::string& filename);
    virtual void expectVFileForRequest(const std::string& filename);
    virtual bool validateVFileForRequest(const std::string& filename);
    */

    virtual void processReceiveData (LLMessageSystem *mesgsys, void **user_data);
    virtual void sendConfirmPacket (LLMessageSystem *mesgsys, U64 id, S32 packetnum, const LLHost &remote_host);

// file sending routines
    virtual void processFileRequest (LLMessageSystem *mesgsys, void **user_data);
    virtual void processConfirmation (LLMessageSystem *mesgsys, void **user_data);
    virtual void retransmitUnackedPackets ();

// error handling
    void abortRequestById(U64 xfer_id, S32 result_code);
    virtual void processAbort (LLMessageSystem *mesgsys, void **user_data);

    virtual bool isHostFlooded(const LLHost & host);
};

extern LLXferManager*   gXferManager;

// initialization and garbage collection
void start_xfer_manager();
void cleanup_xfer_manager();

// message system callbacks
void process_confirm_packet (LLMessageSystem *mesgsys, void **user_data);
void process_request_xfer (LLMessageSystem *mesgsys, void **user_data);
void continue_file_receive(LLMessageSystem *mesgsys, void **user_data);
void process_abort_xfer (LLMessageSystem *mesgsys, void **user_data);
#endif



