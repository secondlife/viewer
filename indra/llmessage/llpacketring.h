/**
 * @file llpacketring.h
 * @brief LLPacketRing: a simple ring buffer for LLPacketBuffers.
 *
 * LLPacketRing stores incoming UDP packets that have already been received
 * from the network socket. It has no socket or proxy awareness; callers
 * push packets in with pushPacket() and retrieve them in FIFO order with
 * popPacket().
 *
 * The ring starts at DEFAULT_BUFFER_RING_SIZE slots and grows in increments
 * of BUFFER_RING_EXPANSION up to MAX_BUFFER_RING_SIZE.  Once at the ceiling,
 * pushPacket() silently overwrites the oldest queued packet to make room for
 * the incoming one.
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

#include <vector>

#include "llpacketbuffer.h"


class LLPacketRing
{
public:
    LLPacketRing();
    ~LLPacketRing();

    // Copy 'packet' onto the tail of the ring, growing the ring or
    // overwriting the oldest entry when the ring is at capacity.
    void pushPacket(const LLPacketBuffer& packet);

    // Copy the head (oldest) packet into 'packet'.
    // Returns true if a packet was available, false if the ring was empty.
    bool popPacket(LLPacketBuffer& packet);

    S32 getNumBufferedPackets() const { return (S32)(mNumBufferedPackets); }
    S32 getNumBufferedBytes()   const { return mNumBufferedBytes; }

    // Ratio of buffered packets to DEFAULT_BUFFER_RING_SIZE (0 = empty, 1 = nominal full).
    F32 getBufferLoadRate() const;

private:
    // Returns true if the ring was expanded, false if already at the ceiling.
    bool expandRing();

    std::vector<LLPacketBuffer*> mRing;
    S16 mHeadIndex          { 0 };
    S16 mNumBufferedPackets { 0 };
    S32 mNumBufferedBytes   { 0 };
};
