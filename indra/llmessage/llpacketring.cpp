/** 
 * @file llpacketring.cpp
 * @brief implementation of LLPacketRing class for a packet.
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

///////////////////////////////////////////////////////////
LLPacketRing::LLPacketRing () :
	mUseInThrottle(FALSE),
	mUseOutThrottle(FALSE),
	mInThrottle(256000.f),
	mOutThrottle(64000.f),
	mActualBitsIn(0),
	mActualBitsOut(0),
	mMaxBufferLength(64000),
	mInBufferLength(0),
	mOutBufferLength(0),
	mDropPercentage(0.0f),
	mPacketsToDrop(0x0)
{
}

///////////////////////////////////////////////////////////
LLPacketRing::~LLPacketRing ()
{
	cleanup();
}
	
///////////////////////////////////////////////////////////
void LLPacketRing::cleanup ()
{
	LLPacketBuffer *packetp;

	while (!mReceiveQueue.empty())
	{
		packetp = mReceiveQueue.front();
		delete packetp;
		mReceiveQueue.pop();
	}

	while (!mSendQueue.empty())
	{
		packetp = mSendQueue.front();
		delete packetp;
		mSendQueue.pop();
	}
}

///////////////////////////////////////////////////////////
void LLPacketRing::dropPackets (U32 num_to_drop)
{
	mPacketsToDrop += num_to_drop;
}

///////////////////////////////////////////////////////////
void LLPacketRing::setDropPercentage (F32 percent_to_drop)
{
	mDropPercentage = percent_to_drop;
}

void LLPacketRing::setUseInThrottle(const bool use_throttle)
{
	mUseInThrottle = use_throttle;
}

void LLPacketRing::setUseOutThrottle(const bool use_throttle)
{
	mUseOutThrottle = use_throttle;
}

void LLPacketRing::setInBandwidth(const F32 bps)
{
	mInThrottle.setRate(bps);
}

void LLPacketRing::setOutBandwidth(const F32 bps)
{
	mOutThrottle.setRate(bps);
}
///////////////////////////////////////////////////////////
S32 LLPacketRing::receiveFromRing (S32 socket, char *datap)
{

	if (mInThrottle.checkOverflow(0))
	{
		// We don't have enough bandwidth, don't give them a packet.
		return 0;
	}

	LLPacketBuffer *packetp = NULL;
	if (mReceiveQueue.empty())
	{
		// No packets on the queue, don't give them any.
		return 0;
	}

	S32 packet_size = 0;
	packetp = mReceiveQueue.front();
	mReceiveQueue.pop();
	packet_size = packetp->getSize();
	if (packetp->getData() != NULL)
	{
		memcpy(datap, packetp->getData(), packet_size);	/*Flawfinder: ignore*/
	}
	// need to set sender IP/port!!
	mLastSender = packetp->getHost();
	mLastReceivingIF = packetp->getReceivingInterface();
	delete packetp;

	this->mInBufferLength -= packet_size;

	// Adjust the throttle
	mInThrottle.throttleOverflow(packet_size * 8.f);
	return packet_size;
}

///////////////////////////////////////////////////////////
S32 LLPacketRing::receivePacket (S32 socket, char *datap)
{
	S32 packet_size = 0;

	// If using the throttle, simulate a limited size input buffer.
	if (mUseInThrottle)
	{
		bool done = false;

		// push any current net packet (if any) onto delay ring
		while (!done)
		{
			LLPacketBuffer *packetp;
			packetp = new LLPacketBuffer(socket);

			if (packetp->getSize())
			{
				mActualBitsIn += packetp->getSize() * 8;

				// Fake packet loss
				if (mDropPercentage && (ll_frand(100.f) < mDropPercentage))
				{
					mPacketsToDrop++;
				}

				if (mPacketsToDrop)
				{
					delete packetp;
					packetp = NULL;
					packet_size = 0;
					mPacketsToDrop--;
				}
			}

			// If we faked packet loss, then we don't have a packet
			// to use for buffer overflow testing
			if (packetp)
			{
				if (mInBufferLength + packetp->getSize() > mMaxBufferLength)
				{
					// Toss it.
					LL_WARNS() << "Throwing away packet, overflowing buffer" << LL_ENDL;
					delete packetp;
					packetp = NULL;
				}
				else if (packetp->getSize())
				{
					mReceiveQueue.push(packetp);
					mInBufferLength += packetp->getSize();
				}
				else
				{
					delete packetp;
					packetp = NULL;
					done = true;
				}
			}
			else
			{
				// No packetp, keep going? - no packetp == faked packet loss
			}
		}

		// Now, grab data off of the receive queue according to our
		// throttled bandwidth settings.
		packet_size = receiveFromRing(socket, datap);
	}
	else
	{
		// no delay, pull straight from net
		if (LLProxy::isSOCKSProxyEnabled())
		{
			U8 buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];
			packet_size = receive_packet(socket, static_cast<char*>(static_cast<void*>(buffer)));
			
			if (packet_size > SOCKS_HEADER_SIZE)
			{
				// *FIX We are assuming ATYP is 0x01 (IPv4), not 0x03 (hostname) or 0x04 (IPv6)
				memcpy(datap, buffer + SOCKS_HEADER_SIZE, packet_size - SOCKS_HEADER_SIZE);
				proxywrap_t * header = static_cast<proxywrap_t*>(static_cast<void*>(buffer));
				mLastSender.setAddress(header->addr);
				mLastSender.setPort(ntohs(header->port));

				packet_size -= SOCKS_HEADER_SIZE; // The unwrapped packet size
			}
			else
			{
				packet_size = 0;
			}
		}
		else
		{
			packet_size = receive_packet(socket, datap);
			mLastSender = ::get_sender();
		}

		mLastReceivingIF = ::get_receiving_interface();

		if (packet_size)  // did we actually get a packet?
		{
			if (mDropPercentage && (ll_frand(100.f) < mDropPercentage))
			{
				mPacketsToDrop++;
			}

			if (mPacketsToDrop)
			{
				packet_size = 0;
				mPacketsToDrop--;
			}
		}
	}

	return packet_size;
}

bool LLPacketRing::sendPacket(int h_socket, char * send_buffer, S32 buf_size, LLHost host)
{
	bool status = true;
	if (!mUseOutThrottle)
	{
		return sendPacketImpl(h_socket, send_buffer, buf_size, host );
	}
	else
	{
		mActualBitsOut += buf_size * 8;
		LLPacketBuffer *packetp = NULL;
		// See if we've got enough throttle to send a packet.
		while (!mOutThrottle.checkOverflow(0.f))
		{
			// While we have enough bandwidth, send a packet from the queue or the current packet

			S32 packet_size = 0;
			if (!mSendQueue.empty())
			{
				// Send a packet off of the queue
				LLPacketBuffer *packetp = mSendQueue.front();
				mSendQueue.pop();

				mOutBufferLength -= packetp->getSize();
				packet_size = packetp->getSize();

				status = sendPacketImpl(h_socket, packetp->getData(), packet_size, packetp->getHost());
				
				delete packetp;
				// Update the throttle
				mOutThrottle.throttleOverflow(packet_size * 8.f);
			}
			else
			{
				// If the queue's empty, we can just send this packet right away.
				status =  sendPacketImpl(h_socket, send_buffer, buf_size, host );
				packet_size = buf_size;

				// Update the throttle
				mOutThrottle.throttleOverflow(packet_size * 8.f);

				// This was the packet we're sending now, there are no other packets
				// that we need to send
				return status;
			}

		}

		// We haven't sent the incoming packet, add it to the queue
		if (mOutBufferLength + buf_size > mMaxBufferLength)
		{
			// Nuke this packet, we overflowed the buffer.
			// Toss it.
			LL_WARNS() << "Throwing away outbound packet, overflowing buffer" << LL_ENDL;
		}
		else
		{
			static LLTimer queue_timer;
			if ((mOutBufferLength > 4192) && queue_timer.getElapsedTimeF32() > 1.f)
			{
				// Add it to the queue
				LL_INFOS() << "Outbound packet queue " << mOutBufferLength << " bytes" << LL_ENDL;
				queue_timer.reset();
			}
			packetp = new LLPacketBuffer(host, send_buffer, buf_size);

			mOutBufferLength += packetp->getSize();
			mSendQueue.push(packetp);
		}
	}

	return status;
}

bool LLPacketRing::sendPacketImpl(int h_socket, const char * send_buffer, S32 buf_size, LLHost host)
{
	
	if (!LLProxy::isSOCKSProxyEnabled())
	{
		return send_packet(h_socket, send_buffer, buf_size, host.getAddress(), host.getPort());
	}

	char headered_send_buffer[NET_BUFFER_SIZE + SOCKS_HEADER_SIZE];

	proxywrap_t *socks_header = static_cast<proxywrap_t*>(static_cast<void*>(&headered_send_buffer));
	socks_header->rsv   = 0;
	socks_header->addr  = host.getAddress();
	socks_header->port  = htons(host.getPort());
	socks_header->atype = ADDRESS_IPV4;
	socks_header->frag  = 0;

	memcpy(headered_send_buffer + SOCKS_HEADER_SIZE, send_buffer, buf_size);

	return send_packet(	h_socket,
						headered_send_buffer,
						buf_size + SOCKS_HEADER_SIZE,
						LLProxy::getInstance()->getUDPProxy().getAddress(),
						LLProxy::getInstance()->getUDPProxy().getPort());
}
