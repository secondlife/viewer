/** 
 * @file llpacketbuffer.h
 * @brief definition of LLPacketBuffer class for implementing a
 * resend, drop, or delay in packet transmissions.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPACKETBUFFER_H
#define LL_LLPACKETBUFFER_H

#include "net.h"		// for NET_BUFFER_SIZE
#include "llhost.h"

class LLPacketBuffer
{
public:
	LLPacketBuffer(const LLHost &host, const char *datap, const S32 size);
	LLPacketBuffer(S32 hSocket);           // receive a packet
	~LLPacketBuffer();

	S32			getSize() const		{ return mSize; }
	const char	*getData() const	{ return mData; }
	LLHost		getHost() const		{ return mHost; }
	void init(S32 hSocket);
	void free();

protected:
	char	mData[NET_BUFFER_SIZE];        // packet data		/* Flawfinder : ignore */
	S32		mSize;          // size of buffer in bytes
	LLHost	mHost;         // source/dest IP and port
};

#endif


