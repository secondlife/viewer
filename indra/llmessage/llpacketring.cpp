/** 
 * @file llpacketring.cpp
 * @brief implementation of LLPacketRing class for a packet.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llpacketring.h"

// linden library includes
#include "llerror.h"
#include "lltimer.h"
#include "timing.h"
#include "llrand.h"
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
	free();
}
	
///////////////////////////////////////////////////////////
void LLPacketRing::free ()
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

void LLPacketRing::setUseInThrottle(const BOOL use_throttle)
{
	mUseInThrottle = use_throttle;
}

void LLPacketRing::setUseOutThrottle(const BOOL use_throttle)
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
		memcpy(datap, packetp->getData(), packet_size);
	}
	// need to set sender IP/port!!
	mLastSender = packetp->getHost();
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
		BOOL done = FALSE;

		// push any current net packet (if any) onto delay ring
		while (!done)
		{
			LLPacketBuffer *packetp;
			packetp = new LLPacketBuffer(socket);

			if (packetp->getSize())
			{
				mActualBitsIn += packetp->getSize() * 8;

				// Fake packet loss
				if (mDropPercentage && (frand(100.f) < mDropPercentage))
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
					llwarns << "Throwing away packet, overflowing buffer" << llendl;
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
		packet_size = receive_packet(socket, datap);		
		mLastSender = ::get_sender();

		if (packet_size)  // did we actually get a packet?
		{
			if (mDropPercentage && (frand(100.f) < mDropPercentage))
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

BOOL LLPacketRing::sendPacket(int h_socket, char * send_buffer, S32 buf_size, LLHost host)
{
	BOOL status = TRUE;
	if (!mUseOutThrottle)
	{
		return send_packet(h_socket, send_buffer, buf_size, host.getAddress(), host.getPort() );
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

				status = send_packet(h_socket, packetp->getData(), packet_size, packetp->getHost().getAddress(), packetp->getHost().getPort());
				
				delete packetp;
				// Update the throttle
				mOutThrottle.throttleOverflow(packet_size * 8.f);
			}
			else
			{
				// If the queue's empty, we can just send this packet right away.
				status = send_packet(h_socket, send_buffer, buf_size, host.getAddress(), host.getPort() );
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
			llwarns << "Throwing away outbound packet, overflowing buffer" << llendl;
		}
		else
		{
			static LLTimer queue_timer;
			if ((mOutBufferLength > 4192) && queue_timer.getElapsedTimeF32() > 1.f)
			{
				// Add it to the queue
				llinfos << "Outbound packet queue " << mOutBufferLength << " bytes" << llendl;
				queue_timer.reset();
			}
			packetp = new LLPacketBuffer(host, send_buffer, buf_size);

			mOutBufferLength += packetp->getSize();
			mSendQueue.push(packetp);
		}
	}

	return status;
}
