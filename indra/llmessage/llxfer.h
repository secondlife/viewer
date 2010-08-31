/** 
 * @file llxfer.h
 * @brief definition of LLXfer class for a single xfer
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLXFER_H
#define LL_LLXFER_H

#include "message.h"
#include "lltimer.h"

const S32 LL_XFER_LARGE_PAYLOAD = 7680;

typedef enum ELLXferStatus {
	e_LL_XFER_UNINITIALIZED,
	e_LL_XFER_REGISTERED,         // a buffer which has been registered as available for a request
	e_LL_XFER_PENDING,            // a transfer which has been requested but is waiting for a free slot
	e_LL_XFER_IN_PROGRESS,
	e_LL_XFER_COMPLETE,
	e_LL_XFER_ABORTED,
	e_LL_XFER_NONE
} ELLXferStatus;

class LLXfer
{
 private:
 protected:
        S32 mChunkSize;
  
 public:
	LLXfer *mNext;
	U64 mID;
	S32 mPacketNum;

	LLHost mRemoteHost;
	S32 mXferSize;

	char *mBuffer;
	U32 mBufferLength;
	U32 mBufferStartOffset;
	BOOL mBufferContainsEOF;

	ELLXferStatus mStatus;

	BOOL mWaitingForACK;

	void (*mCallback)(void **,S32,LLExtStat);	
	void **mCallbackDataHandle;
	S32 mCallbackResult;

	LLTimer ACKTimer;
	S32 mRetries;

	static const U32 XFER_FILE;
	static const U32 XFER_VFILE;
	static const U32 XFER_MEM;

 private:
 protected:
 public:
	LLXfer (S32 chunk_size);
	virtual ~LLXfer();

	void init(S32 chunk_size);
	virtual void cleanup();

	virtual S32 startSend (U64 xfer_id, const LLHost &remote_host);
	virtual void sendPacket(S32 packet_num);
	virtual void sendNextPacket();
	virtual void resendLastPacket();
	virtual S32 processEOF();
	virtual S32 startDownload();
	virtual S32 receiveData (char *datap, S32 data_size);
	virtual void abort(S32);

	virtual S32 suck(S32 start_position);
	virtual S32 flush();
	
	virtual S32 encodePacketNum(S32 packet_num, BOOL is_eof);	
	virtual void setXferSize (S32 data_size);
	virtual S32  getMaxBufferSize();

	virtual std::string getFileName();

	virtual U32 getXferTypeTag();

	friend std::ostream& operator<< (std::ostream& os, LLXfer &hh);

};

#endif





