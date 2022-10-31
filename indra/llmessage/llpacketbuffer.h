/** 
 * @file llpacketbuffer.h
 * @brief definition of LLPacketBuffer class for implementing a
 * resend, drop, or delay in packet transmissions.
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

#ifndef LL_LLPACKETBUFFER_H
#define LL_LLPACKETBUFFER_H

#include "net.h"        // for NET_BUFFER_SIZE
#include "llhost.h"

class LLPacketBuffer
{
public:
    LLPacketBuffer(const LLHost &host, const char *datap, const S32 size);
    LLPacketBuffer(S32 hSocket);           // receive a packet
    ~LLPacketBuffer();

    S32         getSize() const                 { return mSize; }
    const char  *getData() const                { return mData; }
    LLHost      getHost() const                 { return mHost; }
    LLHost      getReceivingInterface() const   { return mReceivingIF; }
    void init(S32 hSocket);

protected:
    char    mData[NET_BUFFER_SIZE];        // packet data       /* Flawfinder : ignore */
    S32     mSize;          // size of buffer in bytes
    LLHost  mHost;         // source/dest IP and port
    LLHost  mReceivingIF;         // source/dest IP and port
};

#endif


