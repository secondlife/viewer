/** 
 * @file llpacketring.h
 * @brief definition of LLPacketRing class for implementing a resend,
 * drop, or delay in packet transmissions
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPACKETRING_H
#define LL_LLPACKETRING_H

#include <queue>

#include "llpacketbuffer.h"
#include "linked_lists.h"
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
