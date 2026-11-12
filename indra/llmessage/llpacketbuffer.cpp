/**
 * @file llpacketbuffer.cpp
 * @brief implementation of LLPacketBuffer class for a packet.
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

#include "llpacketbuffer.h"

#include "net.h"
#include "lltimer.h"
#include "llhost.h"

LLPacketBuffer::LLPacketBuffer(const LLHost &host, const char *datap, const S32 size) : mHost(host)
{
    mSize = 0;
    mData[0] = '!';

    if (size > NET_BUFFER_SIZE)
    {
        LL_ERRS() << "Constructing packet with size=" << size << " > " << NET_BUFFER_SIZE << LL_ENDL;
    }
    else
    {
        if (datap != NULL)
        {
            memcpy(mData, datap, size);
            mSize = size;
        }
    }
}

LLPacketBuffer::LLPacketBuffer (S32 hSocket)
{
    init(hSocket);
}

LLPacketBuffer::~LLPacketBuffer ()
{
}

// Only the live mSize bytes of mData are copied, not the full
// NET_BUFFER_SIZE array. See the comment on these declarations in
// llpacketbuffer.h and the class comment on LLPacketRing.
LLPacketBuffer::LLPacketBuffer(const LLPacketBuffer& other)
    : mSize(other.mSize),
    mHost(other.mHost),
    mReceivingIF(other.mReceivingIF),
    mPacketIDChecked(other.mPacketIDChecked)
{
    if (mSize > 0)
    {
        memcpy(mData, other.mData, mSize);
    }
}

LLPacketBuffer& LLPacketBuffer::operator=(const LLPacketBuffer& other)
{
    if (this != &other)
    {
        mSize = other.mSize;
        mHost = other.mHost;
        mReceivingIF = other.mReceivingIF;
        mPacketIDChecked = other.mPacketIDChecked;
        if (mSize > 0)
        {
            memcpy(mData, other.mData, mSize);
        }
    }
    return *this;
}

LLPacketBuffer::LLPacketBuffer(LLPacketBuffer&& other) noexcept
    : mSize(other.mSize),
    mHost(other.mHost),
    mReceivingIF(other.mReceivingIF),
    mPacketIDChecked(other.mPacketIDChecked)
{
    if (mSize > 0)
    {
        memcpy(mData, other.mData, mSize);
    }
    // mData is a fixed inline array: there's nothing cheaper to "move" than
    // a memcpy of the live bytes, so move and copy are identical in cost.
    // Still worth having a distinct move overload so callers that write
    // std::move(pkt) aren't silently falling back to a (more expensive, if
    // mData were ever made larger) copy path.
    other.mSize = 0;
}

LLPacketBuffer& LLPacketBuffer::operator=(LLPacketBuffer&& other) noexcept
{
    if (this != &other)
    {
        mSize = other.mSize;
        mHost = other.mHost;
        mReceivingIF = other.mReceivingIF;
        mPacketIDChecked = other.mPacketIDChecked;
        if (mSize > 0)
        {
            memcpy(mData, other.mData, mSize);
        }
        other.mSize = 0;
    }
    return *this;
}

void LLPacketBuffer::init(S32 hSocket)
{
    mSize = receive_packet(hSocket, mData);
    mHost = ::get_sender();
    mReceivingIF = ::get_receiving_interface();
}

void LLPacketBuffer::init(const char* buffer, S32 data_size, const LLHost& host)
{
    if (data_size > NET_BUFFER_SIZE)
    {
        LL_ERRS() << "Initializing packet with size=" << data_size << " > " << NET_BUFFER_SIZE << LL_ENDL;
    }
    else
    {
        memcpy(mData, buffer, data_size);
        mSize = data_size;
        mHost = host;
        mReceivingIF = ::get_receiving_interface();
    }
}

