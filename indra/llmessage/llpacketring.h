/** 
 * @file llpacketring.h
 * @brief definition of LLPacketRing class for implementing a resend,
 * drop, or delay in packet transmissions
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

#ifndef LL_LLPACKETRING_H
#define LL_LLPACKETRING_H

#include <queue>

#include "llpacketbuffer.h"
#include "llhost.h"
#include "net.h"
#include "llthrottle.h"


class LLPacketRing
{
public:
	LLPacketRing();         
    ~LLPacketRing();

	void free();

	void dropPackets(U32);	
	void setDropPercentage (F32 percent_to_drop);
	void setUseInThrottle(const BOOL use_throttle);
	void setUseOutThrottle(const BOOL use_throttle);
	void setInBandwidth(const F32 bps);
	void setOutBandwidth(const F32 bps);
	S32  receivePacket (S32 socket, char *datap);
	S32  receiveFromRing (S32 socket, char *datap);

	BOOL sendPacket(int h_socket, char * send_buffer, S32 buf_size, LLHost host);

	inline LLHost getLastSender();

	S32 getAndResetActualInBits()				{ S32 bits = mActualBitsIn; mActualBitsIn = 0; return bits;}
	S32 getAndResetActualOutBits()				{ S32 bits = mActualBitsOut; mActualBitsOut = 0; return bits;}
protected:
	BOOL mUseInThrottle;
	BOOL mUseOutThrottle;
	
	// For simulating a lower-bandwidth connection - BPS
	LLThrottle mInThrottle;
	LLThrottle mOutThrottle;

	S32 mActualBitsIn;
	S32 mActualBitsOut;
	S32 mMaxBufferLength;			// How much data can we queue up before dropping data.
	S32 mInBufferLength;			// Current incoming buffer length
	S32 mOutBufferLength;			// Current outgoing buffer length

	F32 mDropPercentage;			// % of packets to drop
	U32 mPacketsToDrop;				// drop next n packets

	std::queue<LLPacketBuffer *> mReceiveQueue;
	std::queue<LLPacketBuffer *> mSendQueue;

	LLHost mLastSender;
};


inline LLHost LLPacketRing::getLastSender()
{
	return mLastSender;
}

#endif
