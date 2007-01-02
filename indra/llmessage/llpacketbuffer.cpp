/** 
 * @file llpacketbuffer.cpp
 * @brief implementation of LLPacketBuffer class for a packet.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llpacketbuffer.h"

#include "net.h"
#include "timing.h"
#include "llhost.h"

///////////////////////////////////////////////////////////

LLPacketBuffer::LLPacketBuffer(const LLHost &host, const char *datap, const S32 size) : mHost(host)
{
	if (size > NET_BUFFER_SIZE)
	{
		llerrs << "Sending packet > " << NET_BUFFER_SIZE << " of size " << size << llendl;
	}

	if (datap != NULL)
	{
		memcpy(mData, datap, size);
		mSize = size;
	}
	
}

LLPacketBuffer::LLPacketBuffer (S32 hSocket)
{
	init(hSocket);
}

///////////////////////////////////////////////////////////

LLPacketBuffer::~LLPacketBuffer ()
{
	free();
}

///////////////////////////////////////////////////////////

void LLPacketBuffer::init (S32 hSocket)
{
	mSize = receive_packet(hSocket, mData);
	mHost = ::get_sender();
}
	
///////////////////////////////////////////////////////////

void LLPacketBuffer::free ()
{
}

















