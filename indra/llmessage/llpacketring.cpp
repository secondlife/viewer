/**
 * @file llpacketring.cpp
 * @brief implementation of LLPacketRing class.
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

#include "llpacketring.h"

#include "llerror.h"

constexpr S16 MAX_BUFFER_RING_SIZE     = 8192;

// DANGER: don't adjust DEFAULT_BUFFER_RING_SIZE unless you know what
// you're doing.  Its value affects the "buffer load rate" which is used
// to supply backpressure to an overloaded nework queue.
constexpr S16 DEFAULT_BUFFER_RING_SIZE = 256;

LLPacketRing::LLPacketRing()
    : mRing(DEFAULT_BUFFER_RING_SIZE, nullptr)
{
    LLHost invalid_host;
    for (size_t i = 0; i < mRing.size(); ++i)
    {
        mRing[i] = new LLPacketBuffer(invalid_host, nullptr, 0);
    }
}

LLPacketRing::~LLPacketRing()
{
    for (auto* packet : mRing)
    {
        delete packet;
    }
    mRing.clear();
    mNumBufferedPackets = 0;
    mNumBufferedBytes = 0;
    mHeadIndex = 0;
}

void LLPacketRing::pushPacket(const LLPacketBuffer& packet)
{
    S16 ring_size = (S16)mRing.size();
    if (mNumBufferedPackets >= ring_size && ring_size < MAX_BUFFER_RING_SIZE)
    {
        expandRing();
        ring_size = (S16)mRing.size();
    }

    LLPacketBuffer* slot = mRing[mHeadIndex];
    S32 old_size = slot->getSize();

    *slot = packet;

    mHeadIndex = (mHeadIndex + 1) % ring_size;

    if (mNumBufferedPackets < ring_size)
    {
        ++mNumBufferedPackets;
        mNumBufferedBytes += packet.getSize();
    }
    else
    {
        // Ring is at maximum capacity; oldest packet was overwritten.
        // This is VERY BAD because we've already ACKed the packet we're loosing
        // (if it was "reliable").
        LL_WARNS("PacketRing") << "buffer overflow at " << mNumBufferedPackets << " packets" << LL_ENDL;
        mNumBufferedBytes += packet.getSize() - old_size;
    }
}

bool LLPacketRing::popPacket(LLPacketBuffer& packet)
{
    if (mNumBufferedPackets <= 0)
    {
        return false;
    }

    S16 ring_size  = (S16)mRing.size();
    S16 tail_index = (mHeadIndex + ring_size - mNumBufferedPackets) % ring_size;

    LLPacketBuffer* slot = mRing[tail_index];
    S32 packet_size = slot->getSize();

    packet = *slot;

    --mNumBufferedPackets;
    mNumBufferedBytes -= packet_size;

    llassert(mNumBufferedPackets > 0 || mNumBufferedBytes == 0);

    return true;
}

bool LLPacketRing::expandRing()
{
    constexpr S16 BUFFER_RING_EXPANSION = 512;
    S16 old_size = (S16)mRing.size();
    S16 new_size = llmin(old_size + BUFFER_RING_EXPANSION, MAX_BUFFER_RING_SIZE);
    if (new_size == old_size)
    {
        return false;
    }

    // Lay existing entries out linearly in FIFO order starting at index 0.
    std::vector<LLPacketBuffer*> new_ring(new_size, nullptr);
    for (S16 i = 0; i < old_size; ++i)
    {
        S16 j = (mHeadIndex + i) % old_size;
        new_ring[i] = mRing[j];
    }

    LLHost invalid_host;
    for (S16 i = old_size; i < new_size; ++i)
    {
        new_ring[i] = new LLPacketBuffer(invalid_host, nullptr, 0);
    }

    mRing.swap(new_ring);
    mHeadIndex = mNumBufferedPackets;
    return true;
}

F32 LLPacketRing::getBufferLoadRate() const
{
    return (F32)mNumBufferedPackets / (F32)DEFAULT_BUFFER_RING_SIZE;
}
