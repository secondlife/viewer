/**
 * @file lludpreceiverthread.cpp
 * @brief Background thread that pulls raw UDP packets off the socket
 *        and hands them to the main thread via a thread-safe queue.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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
#include "lludpreceiverthread.h"
#include "llproxy.h"
#include "message.h"
#include "net.h"

#include <boost/fiber/algo/round_robin.hpp>

LLUDPReceiverThread::LLUDPReceiverThread(S32 hSocket, std::shared_ptr<PacketQueue> queue)
:   LLThread("UDP Receiver"),
    mSocket(hSocket),
    // queue was passed by lvalue so in the end there will be two live shared_ptr
    // instances pointing at the same PacketQueue (mIncomingQueue)
    mQueue(std::move(queue))
{
}

LLUDPReceiverThread::~LLUDPReceiverThread()
{
}

void LLUDPReceiverThread::run()
{
#if LL_WINDOWS
    // LLThreadSafeQueue is built on boost::fibers primitives, which require
    // a fiber scheduling algorithm to be installed on the thread.
    // Todo: most threads do not need coroutines, perhaps find a way around?
    boost::fibers::use_scheduling_algorithm<boost::fibers::algo::round_robin>();
#endif // LL_WINDOWS

    // Thread name is already registered with the profiler by
    // LLThread::threadRun() via LL_PROFILER_SET_THREAD_NAME("UDP Receiver").
    while (!isQuitting())
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_NETWORK;

        LLHost invalid_host;
        LLPacketBuffer pkt(invalid_host, nullptr, 0);
        S32 packet_size = 0;

        if (LLProxy::isSOCKSProxyEnabled())
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_NETWORK("udp recv (socks)");
            char buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];   /* Flawfinder ignore */
            packet_size = receive_packet(mSocket, buffer);
            if (packet_size > SOCKS_HEADER_SIZE)
            {
                proxywrap_t* header = static_cast<proxywrap_t*>(static_cast<void*>(buffer));
                LLHost sender;
                sender.setAddress(header->addr);
                sender.setPort(ntohs(header->port));
                packet_size -= SOCKS_HEADER_SIZE;
                pkt.init(buffer + SOCKS_HEADER_SIZE, packet_size, sender);
            }
            else
            {
                packet_size = 0;
            }
        }
        else
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_NETWORK("udp recv");
            pkt.init(mSocket);
            packet_size = pkt.getSize();
        }

        LL_PROFILE_ZONE_NUM(packet_size);

        if (packet_size > 0)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_NETWORK("udp queue push");
            // Blocks if the queue is momentarily full (backpressure),
            // raises LLThreadSafeQueueInterrupt if the queue is closed
            // during shutdown, which unwinds this loop naturally.
            try
            {
                mQueue->push(pkt);
            }
            catch (const LLThreadSafeQueueInterrupt&)
            {
                break;
            }
        }
        else
        {
            // Nothing available right now; avoid busy-spinning the CPU
            // when the socket has no data queued (recv_packet returns 0
            // for EWOULDBLOCK). ms_sleep() is already profiled internally.
            ms_sleep(1);
        }

        // Drain mQueue (== gMessageSystem->mIncomingQueue) into the
        // priority rings: this does per-packet ACK bookkeeping and
        // packet-sequence checks (same as drainUdpSocket()).
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_NETWORK("udp buffer inbound");
            while (gMessageSystem->bufferInboundPacket() > 0)
            {
                // keep draining
            }
        }

        // Fully decode whatever is sitting in the priority rings and
        // hand each result off to the main thread's dispatch queue.
        // decodeDataOwned() returns nullptr both when the rings are
        // empty and when a packet was invalid/banned/off-circuit and
        // should simply be dropped.
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_NETWORK("udp decode");
            while (gMessageSystem->getNumBufferedPackets() > 0)
            {
                std::unique_ptr<LLDecodedMessage> decoded = gMessageSystem->decodeDataOwned();
                if (decoded)
                {
                    if (gMessageSystem->isHandledOnUdpThread(*decoded))
                    {
                        LL_PROFILE_ZONE_NAMED_CATEGORY_NETWORK("udp thread dispatch");
                        gMessageSystem->dispatchDecodedOnThread(*decoded);
                    }
                    else
                    {
                        gMessageSystem->pushDecoded(std::move(decoded));
                    }
                }
            }
        }
    }
}
