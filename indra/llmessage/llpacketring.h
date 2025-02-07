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

#pragma once

#include <queue>

#include "llhost.h"
#include "llthrottle.h"
#include "net.h"

class LLPacketBuffer;

class LLPacketRing
{
public:
    LLPacketRing();
    ~LLPacketRing();

    S32  receivePacket (S32 socket, char *datap);
    bool sendPacket(int h_socket, char * send_buffer, S32 buf_size, LLHost host);

    void dropPackets(U32);
    void setDropPercentage (F32 percent_to_drop);

    inline LLHost getLastSender() const;
    inline LLHost getLastReceivingInterface() const;

    S32 getAndResetActualInBits()   { S32 bits = mActualBitsIn; mActualBitsIn = 0; return bits;}
    S32 getAndResetActualOutBits()  { S32 bits = mActualBitsOut; mActualBitsOut = 0; return bits;}

    void setUseInThrottle(const bool use_throttle);
    void setUseOutThrottle(const bool use_throttle);
    void setInBandwidth(const F32 bps);
    void setOutBandwidth(const F32 bps);

protected:
    void cleanup();
    S32  receiveFromRing (S32 socket, char *datap);

protected:
    bool mUseInThrottle;
    bool mUseOutThrottle;

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
};


inline LLHost LLPacketRing::getLastSender() const
{
    return mLastSender;
}

inline LLHost LLPacketRing::getLastReceivingInterface() const
{
    return mLastReceivingIF;
}
