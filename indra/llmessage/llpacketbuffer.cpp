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

///////////////////////////////////////////////////////////

LLPacketBuffer::LLPacketBuffer(const LLHost &host, const char *datap, const S32 size) : mHost(host)
{
    mSize = 0;
    mData[0] = '!';

    if (size > NET_BUFFER_SIZE)
    {
        LL_ERRS() << "Sending packet > " << NET_BUFFER_SIZE << " of size " << size << LL_ENDL;
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

///////////////////////////////////////////////////////////

LLPacketBuffer::~LLPacketBuffer ()
{
}

///////////////////////////////////////////////////////////

void LLPacketBuffer::init (S32 hSocket)
{
    mSize = receive_packet(hSocket, mData);
    mHost = ::get_sender();
    mReceivingIF = ::get_receiving_interface();
}

