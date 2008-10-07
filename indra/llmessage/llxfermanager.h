/** 
 * @file llxfermanager.h
 * @brief definition of LLXferManager class for a keeping track of
 * multiple xfers
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

#ifndef LL_LLXFERMANAGER_H
#define LL_LLXFERMANAGER_H

/**
 * this manager keeps both a send list and a receive list; anything with a 
 * LLXferManager can send and receive files via messages
 */

//Forward declaration to avoid circular dependencies
class LLXfer;
class LLVFS;

#include "llxfer.h"
#include "message.h"
#include "llassetstorage.h"
#include "lldir.h"
#include "lllinkedqueue.h"
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
 private:
	LLVFS *mVFS;

 protected:
	S32    mMaxOutgoingXfersPerCircuit;
	S32    mMaxIncomingXfers;

	BOOL	mUseAckThrottling; // Use ack throttling to cap file xfer bandwidth
	LLLinkedQueue<LLXferAckInfo> mXferAckQueue;
	LLThrottle mAckThrottle;
 public:

	// This enumeration is useful in the requestFile() to specify if
	// an xfer must happen asap.
	enum
	{
		LOW_PRIORITY = FALSE,
		HIGH_PRIORITY = TRUE,
	};

	LLXfer *mSendList;
	LLXfer *mReceiveList;

	typedef std::list<LLHostStatus*> status_list_t;
	status_list_t mOutgoingHosts;

 private:
 protected:
	// implementation methods
	virtual void startPendingDownloads();
	virtual void addToList(LLXfer* xferp, LLXfer*& head, BOOL is_priority);
	std::multiset<std::string> mExpectedTransfers; // files that are authorized to transfer out
	std::multiset<std::string> mExpectedRequests;  // files that are authorized to be downloaded on top of

 public:
	LLXferManager(LLVFS *vfs);
	virtual ~LLXferManager();

	virtual void init(LLVFS *vfs);
	virtual void cleanup();

	void setUseAckThrottling(const BOOL use);
	void setAckThrottleBPS(const F32 bps);

// list management routines
	virtual LLXfer *findXfer(U64 id, LLXfer *list_head);
	virtual void removeXfer (LLXfer *delp, LLXfer **list_head);
	virtual U32 numActiveListEntries(LLXfer *list_head);
	virtual S32 numActiveXfers(const LLHost &host);
	virtual S32 numPendingXfers(const LLHost &host);
	virtual void changeNumActiveXfers(const LLHost &host, S32 delta);

	virtual void setMaxOutgoingXfersPerCircuit (S32 max_num);
	virtual void setMaxIncomingXfers(S32 max_num);
	virtual void updateHostStatus();
	virtual void printHostStatus();

// general utility routines
	virtual void registerCallbacks(LLMessageSystem *mesgsys);
	virtual U64 getNextID ();
	virtual S32 encodePacketNum(S32 packet_num, BOOL is_eof);	
	virtual S32 decodePacketNum(S32 packet_num);	
	virtual BOOL isLastPacket(S32 packet_num);

	virtual U64 registerXfer(const void *datap, const S32 length);

// file requesting routines
// .. to file
	virtual void requestFile(const std::string& local_filename,
							 const std::string& remote_filename,
							 ELLPath remote_path,
							 const LLHost& remote_host,
 							 BOOL delete_remote_on_completion,
							 void (*callback)(void**,S32,LLExtStat), void** user_data,
							 BOOL is_priority = FALSE,
							 BOOL use_big_packets = FALSE);

// .. to memory
	virtual void requestFile(const std::string& remote_filename, 
							 ELLPath remote_path,
							 const LLHost &remote_host,
 							 BOOL delete_remote_on_completion,
							 void (*callback)(void*, S32, void**, S32, LLExtStat),
							 void** user_data,
							 BOOL is_priority = FALSE);

// vfile requesting
// .. to vfile
	virtual void requestVFile(const LLUUID &local_id, const LLUUID& remote_id,
							  LLAssetType::EType type, LLVFS* vfs,
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

/*
// xfer request (may be memory or file)
// .. to file
	virtual void requestXfer(const char *local_filename, U64 xfer_id, 
							 BOOL delete_remote_on_completion,
							 const LLHost &remote_host, void (*callback)(void **,S32),void **user_data);
// .. to memory
	virtual void requestXfer(U64 xfer_id, 
							 const LLHost &remote_host, 
							 BOOL delete_remote_on_completion,
							 void (*callback)(void *, S32, void **, S32),void **user_data);
*/

	virtual void processReceiveData (LLMessageSystem *mesgsys, void **user_data);
	virtual void sendConfirmPacket (LLMessageSystem *mesgsys, U64 id, S32 packetnum, const LLHost &remote_host);

// file sending routines
	virtual void processFileRequest (LLMessageSystem *mesgsys, void **user_data);
	virtual void processConfirmation (LLMessageSystem *mesgsys, void **user_data);
	virtual void retransmitUnackedPackets ();

// error handling
	virtual void processAbort (LLMessageSystem *mesgsys, void **user_data);
};

extern LLXferManager*	gXferManager;

// initialization and garbage collection
void start_xfer_manager(LLVFS *vfs);
void cleanup_xfer_manager();

// message system callbacks
void process_confirm_packet (LLMessageSystem *mesgsys, void **user_data);
void process_request_xfer (LLMessageSystem *mesgsys, void **user_data);
void continue_file_receive(LLMessageSystem *mesgsys, void **user_data);
void process_abort_xfer (LLMessageSystem *mesgsys, void **user_data);
#endif



