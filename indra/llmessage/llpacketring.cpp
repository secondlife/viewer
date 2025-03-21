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

#if LL_WINDOWS
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif

// linden library includes
#include "llerror.h"
#include "lltimer.h"
#include "llproxy.h"
#include "llrand.h"
#include "message.h"
#include "u64.h"

constexpr S16 MAX_BUFFER_RING_SIZE = 1024;
constexpr S16 DEFAULT_BUFFER_RING_SIZE = 256;

LLPacketRing::LLPacketRing ()
    : mPacketRing(DEFAULT_BUFFER_RING_SIZE, nullptr)
{
    LLHost invalid_host;
    for (size_t i = 0; i < mPacketRing.size(); ++i)
    {
        mPacketRing[i] = new LLPacketBuffer(invalid_host, nullptr, 0);
    }
}

LLPacketRing::~LLPacketRing ()
{
    for (auto packet : mPacketRing)
    {
        delete packet;
    }
    mPacketRing.clear();
    mNumBufferedPackets = 0;
    mNumBufferedBytes = 0;
    mHeadIndex = 0;
}

S32 LLPacketRing::receivePacket (S32 socket, char *datap)
{
    bool drop = computeDrop();
    return (mNumBufferedPackets > 0) ?
        receiveOrDropBufferedPacket(datap, drop) :
        receiveOrDropPacket(socket, datap, drop);
}

bool send_packet_helper(int socket, const char * datap, S32 data_size, LLHost host)
{
    if (!LLProxy::isSOCKSProxyEnabled())
    {
        return send_packet(socket, datap, data_size, host.getAddress(), host.getPort());
    }

    char headered_send_buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];

    proxywrap_t *socks_header = static_cast<proxywrap_t*>(static_cast<void*>(&headered_send_buffer));
    socks_header->rsv   = 0;
    socks_header->addr  = host.getAddress();
    socks_header->port  = htons(host.getPort());
    socks_header->atype = ADDRESS_IPV4;
    socks_header->frag  = 0;

    memcpy(headered_send_buffer + SOCKS_HEADER_SIZE, datap, data_size);

    return send_packet( socket,
                        headered_send_buffer,
                        data_size + SOCKS_HEADER_SIZE,
                        LLProxy::getInstance()->getUDPProxy().getAddress(),
                        LLProxy::getInstance()->getUDPProxy().getPort());
}

bool LLPacketRing::sendPacket(int socket, const char * datap, S32 data_size, LLHost host)
{
    mActualBytesOut += data_size;
    return send_packet_helper(socket, datap, data_size, host);
}

void LLPacketRing::dropPackets (U32 num_to_drop)
{
    mPacketsToDrop += num_to_drop;
}

void LLPacketRing::setDropPercentage (F32 percent_to_drop)
{
    mDropPercentage = percent_to_drop;
}

bool LLPacketRing::computeDrop()
{
    bool drop= (mDropPercentage > 0.0f && (ll_frand(100.f) < mDropPercentage));
    if (drop)
    {
        ++mPacketsToDrop;
    }
    if (mPacketsToDrop > 0)
    {
        --mPacketsToDrop;
        drop = true;
    }
    return drop;
}

S32 LLPacketRing::receiveOrDropPacket(S32 socket, char *datap, bool drop)
{
    S32 packet_size = 0;

    // pull straight from socket
    if (LLProxy::isSOCKSProxyEnabled())
    {
        char buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];   /* Flawfinder ignore */
        packet_size = receive_packet(socket, buffer);
        if (packet_size > 0)
        {
            mActualBytesIn += packet_size;
        }

        if (packet_size > SOCKS_HEADER_SIZE)
        {
            if (drop)
            {
                packet_size = 0;
            }
            else
            {
                // *FIX We are assuming ATYP is 0x01 (IPv4), not 0x03 (hostname) or 0x04 (IPv6)
                packet_size -= SOCKS_HEADER_SIZE; // The unwrapped packet size
                memcpy(datap, buffer + SOCKS_HEADER_SIZE, packet_size);
                proxywrap_t * header = static_cast<proxywrap_t*>(static_cast<void*>(buffer));
                mLastSender.setAddress(header->addr);
                mLastSender.setPort(ntohs(header->port));
                mLastReceivingIF = ::get_receiving_interface();
            }
        }
        else
        {
            packet_size = 0;
        }
    }
    else
    {
        packet_size = receive_packet(socket, datap);
        if (packet_size > 0)
        {
            mActualBytesIn += packet_size;
            if (drop)
            {
                packet_size = 0;
            }
            else
            {
                mLastSender = ::get_sender();
                mLastReceivingIF = ::get_receiving_interface();
            }
        }
    }
    return packet_size;
}

S32 LLPacketRing::receiveOrDropBufferedPacket(char *datap, bool drop)
{
    assert(mNumBufferedPackets > 0);
    S32 packet_size = 0;

    S16 ring_size = (S16)(mPacketRing.size());
    S16 packet_index = (mHeadIndex + ring_size - mNumBufferedPackets) % ring_size;
    LLPacketBuffer* packet = mPacketRing[packet_index];
    packet_size = packet->getSize();
    mLastSender = packet->getHost();
    mLastReceivingIF = packet->getReceivingInterface();

    --mNumBufferedPackets;
    mNumBufferedBytes -= packet_size;
    if (mNumBufferedPackets == 0)
    {
        assert(mNumBufferedBytes == 0);
    }

    if (!drop)
    {
        assert(packet_size > 0);
        memcpy(datap, packet->getData(), packet_size);
    }
    else
    {
        packet_size = 0;
    }
    return packet_size;
}

S32 LLPacketRing::bufferInboundPacket(S32 socket)
{
    if (mNumBufferedPackets == mPacketRing.size() && mNumBufferedPackets < MAX_BUFFER_RING_SIZE)
    {
        expandRing();
    }

    LLPacketBuffer* packet = mPacketRing[mHeadIndex];
    S32 old_packet_size = packet->getSize();
    S32 packet_size = 0;
    if (LLProxy::isSOCKSProxyEnabled())
    {
        char buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];   /* Flawfinder ignore */
        packet_size = receive_packet(socket, buffer);
        if (packet_size > 0)
        {
            mActualBytesIn += packet_size;
            if (packet_size > SOCKS_HEADER_SIZE)
            {
                // *FIX We are assuming ATYP is 0x01 (IPv4), not 0x03 (hostname) or 0x04 (IPv6)

                proxywrap_t * header = static_cast<proxywrap_t*>(static_cast<void*>(buffer));
                LLHost sender;
                sender.setAddress(header->addr);
                sender.setPort(ntohs(header->port));

                packet_size -= SOCKS_HEADER_SIZE; // The unwrapped packet size
                packet->init(buffer + SOCKS_HEADER_SIZE, packet_size, sender);

                mHeadIndex = (mHeadIndex + 1) % (S16)(mPacketRing.size());
                if (mNumBufferedPackets < MAX_BUFFER_RING_SIZE)
                {
                    ++mNumBufferedPackets;
                    mNumBufferedBytes += packet_size;
                }
                else
                {
                    // we overwrote an older packet
                    mNumBufferedBytes += packet_size - old_packet_size;
                }
            }
            else
            {
                packet_size = 0;
            }
        }
    }
    else
    {
        packet->init(socket);
        packet_size = packet->getSize();
        if (packet_size > 0)
        {
            mActualBytesIn += packet_size;

            mHeadIndex = (mHeadIndex + 1) % (S16)(mPacketRing.size());
            if (mNumBufferedPackets < MAX_BUFFER_RING_SIZE)
            {
                ++mNumBufferedPackets;
                mNumBufferedBytes += packet_size;
            }
            else
            {
                // we overwrote an older packet
                mNumBufferedBytes += packet_size - old_packet_size;
            }
        }
    }
    return packet_size;
}

S32 LLPacketRing::drainSocket(S32 socket)
{
    // drain into buffer
    S32 packet_size = 1;
    S32 num_loops = 0;
    S32 old_num_packets = mNumBufferedPackets;
    while (packet_size > 0)
    {
        packet_size = bufferInboundPacket(socket);
        ++num_loops;
    }
    S32 num_dropped_packets = (num_loops - 1 + old_num_packets) - mNumBufferedPackets;
    if (num_dropped_packets > 0)
    {
        // It will eventually be accounted by mDroppedPackets
        // and mPacketsLost, but track it here for logging purposes.
        mNumDroppedPackets += num_dropped_packets;
    }
    return (S32)(mNumBufferedPackets);
}

bool LLPacketRing::expandRing()
{
    // compute larger size
    constexpr S16 BUFFER_RING_EXPANSION = 256;
    S16 old_size = (S16)(mPacketRing.size());
    S16 new_size = llmin(old_size + BUFFER_RING_EXPANSION, MAX_BUFFER_RING_SIZE);
    if (new_size == old_size)
    {
        // mPacketRing is already maxed out
        return false;
    }

    // make a larger ring and copy packet pointers
    std::vector<LLPacketBuffer*> new_ring(new_size, nullptr);
    for (S16 i = 0; i < old_size; ++i)
    {
        S16 j = (mHeadIndex + i) % old_size;
        new_ring[i] = mPacketRing[j];
    }

    // allocate new packets for the remainder of new_ring
    LLHost invalid_host;
    for (S16 i = old_size; i < new_size; ++i)
    {
        new_ring[i] = new LLPacketBuffer(invalid_host, nullptr, 0);
    }

    // swap the rings and reset mHeadIndex
    mPacketRing.swap(new_ring);
    mHeadIndex = mNumBufferedPackets;
    return true;
}

F32 LLPacketRing::getBufferLoadRate() const
{
    // goes up to MAX_BUFFER_RING_SIZE
    return (F32)mNumBufferedPackets / (F32)DEFAULT_BUFFER_RING_SIZE;
}

void LLPacketRing::dumpPacketRingStats()
{
    mNumDroppedPacketsTotal += mNumDroppedPackets;
    LL_INFOS("Messaging") << "Packet ring stats: " << std::endl
                          << "Buffered packets: " << mNumBufferedPackets << std::endl
                          << "Buffered bytes: " << mNumBufferedBytes << std::endl
                          << "Dropped packets current: " << mNumDroppedPackets << std::endl
                          << "Dropped packets total: " << mNumDroppedPacketsTotal << std::endl
                          << "Dropped packets percentage: " << mDropPercentage << "%" << std::endl
                          << "Actual in bytes: " << mActualBytesIn << std::endl
                          << "Actual out bytes: " << mActualBytesOut << LL_ENDL;
    mNumDroppedPackets = 0;
}
