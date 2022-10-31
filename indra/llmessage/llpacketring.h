/** 
 * @file llpacketring.h
 * @brief definition of LLPacketRing class for implementing a resend,
 * drop, or delay in packet transmissions
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

#ifndef LL_LLPACKETRING_H
#define LL_LLPACKETRING_H

#include <queue>

#include "llhost.h"
#include "llpacketbuffer.h"
#include "llproxy.h"
#include "llthrottle.h"
#include "net.h"

class LLPacketRing
{
public:
    LLPacketRing();         
    ~LLPacketRing();

    void cleanup();

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
    inline LLHost getLastReceivingInterface();

    S32 getAndResetActualInBits()               { S32 bits = mActualBitsIn; mActualBitsIn = 0; return bits;}
    S32 getAndResetActualOutBits()              { S32 bits = mActualBitsOut; mActualBitsOut = 0; return bits;}
protected:
    BOOL mUseInThrottle;
    BOOL mUseOutThrottle;
    
    // For simulating a lower-bandwidth connection - BPS
    LLThrottle mInThrottle;
    LLThrottle mOutThrottle;

    S32 mActualBitsIn;
    S32 mActualBitsOut;
    S32 mMaxBufferLength;           // How much data can we queue up before dropping data.
    S32 mInBufferLength;            // Current incoming buffer length
    S32 mOutBufferLength;           // Current outgoing buffer length

    F32 mDropPercentage;            // % of packets to drop
    U32 mPacketsToDrop;             // drop next n packets

    std::queue<LLPacketBuffer *> mReceiveQueue;
    std::queue<LLPacketBuffer *> mSendQueue;

    LLHost mLastSender;
    LLHost mLastReceivingIF;

private:
    BOOL sendPacketImpl(int h_socket, const char * send_buffer, S32 buf_size, LLHost host);
};


inline LLHost LLPacketRing::getLastSender()
{
    return mLastSender;
}

inline LLHost LLPacketRing::getLastReceivingInterface()
{
    return mLastReceivingIF;
}

#endif
