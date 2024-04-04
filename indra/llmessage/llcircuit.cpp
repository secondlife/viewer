/** 
 * @file llcircuit.cpp
 * @brief Class to track UDP endpoints for the message system.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
 
#if LL_WINDOWS

#include <process.h>

#else

#if LL_LINUX
#include <dlfcn.h>		// RTLD_LAZY
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#endif


#if !defined(USE_CIRCUIT_LIST)
#include <algorithm>
#endif
#include <sstream>
#include <iterator>
#include <stack>

#include "llcircuit.h"

#include "message.h"
#include "llrand.h"
#include "llstl.h"
#include "lltransfermanager.h"
#include "llmodularmath.h"

const S32 PING_START_BLOCK = 3;		// How many pings behind we have to be to consider ourself blocked.
const S32 PING_RELEASE_BLOCK = 2;	// How many pings behind we have to be to consider ourself unblocked.

const F32Seconds TARGET_PERIOD_LENGTH(5.f);
const F32Seconds LL_DUPLICATE_SUPPRESSION_TIMEOUT(60.f); //this can be long, as time-based cleanup is
													// only done when wrapping packetids, now...

LLCircuitData::LLCircuitData(const LLHost &host, TPACKETID in_id, 
							 const F32Seconds circuit_heartbeat_interval, const F32Seconds circuit_timeout)
:	mHost (host),
	mWrapID(0),
	mPacketsOutID(0), 
	mPacketsInID(in_id),
	mHighestPacketID(in_id),
	mTimeoutCallback(NULL),
	mTimeoutUserData(NULL),
	mTrusted(false),
	mbAllowTimeout(true),
	mbAlive(true),
	mBlocked(false),
	mPingTime(0.0),
	mLastPingSendTime(0.0),
	mLastPingReceivedTime(0.0),
	mNextPingSendTime(0.0),
	mPingsInTransit(0),
	mLastPingID(0),
	mPingDelay(INITIAL_PING_VALUE_MSEC), 
	mPingDelayAveraged(INITIAL_PING_VALUE_MSEC), 
	mUnackedPacketCount(0),
	mUnackedPacketBytes(0),
	mLastPacketInTime(0.0),
	mLocalEndPointID(),
	mPacketsOut(0),
	mPacketsIn(0), 
	mPacketsLost(0),
	mBytesIn(0),
	mBytesOut(0),
	mLastPeriodLength(-1.f),
	mBytesInLastPeriod(0),
	mBytesOutLastPeriod(0),
	mBytesInThisPeriod(0),
	mBytesOutThisPeriod(0),
	mPeakBPSIn(0.f),
	mPeakBPSOut(0.f),
	mPeriodTime(0.0),
	mExistenceTimer(),
	mAckCreationTime(0.f),
	mCurrentResendCount(0),
	mLastPacketGap(0),
	mHeartbeatInterval(circuit_heartbeat_interval), 
	mHeartbeatTimeout(circuit_timeout)
{
	// Need to guarantee that this time is up to date, we may be creating a circuit even though we haven't been
	//  running a message system loop.
	F64Seconds mt_sec = LLMessageSystem::getMessageTimeSeconds(true);
	F32 distribution_offset = ll_frand();
	
	mPingTime = mt_sec;
	mLastPingSendTime = mt_sec + mHeartbeatInterval * distribution_offset;
	mLastPingReceivedTime = mt_sec;
	mNextPingSendTime = mLastPingSendTime + 0.95*mHeartbeatInterval + F32Seconds(ll_frand(0.1f*mHeartbeatInterval.value()));
	mPeriodTime = mt_sec;

	mLocalEndPointID.generate();
}


LLCircuitData::~LLCircuitData()
{
	LLReliablePacket *packetp = NULL;

	// Clean up all pending transfers.
	gTransferManager.cleanupConnection(mHost);

	// remove all pending reliable messages on this circuit
	std::vector<TPACKETID> doomed;
	reliable_iter iter;
	reliable_iter end = mUnackedPackets.end();
	for(iter = mUnackedPackets.begin(); iter != end; ++iter)
	{
		packetp = iter->second;
		gMessageSystem->mFailedResendPackets++;
		if(gMessageSystem->mVerboseLog)
		{
			doomed.push_back(packetp->mPacketID);
		}
		if (packetp->mCallback)
		{
			packetp->mCallback(packetp->mCallbackData,LL_ERR_CIRCUIT_GONE);
		}

		// Update stats
		mUnackedPacketCount--;
		mUnackedPacketBytes -= packetp->mBufferLength;

		delete packetp;
	}

	// remove all pending final retry reliable messages on this circuit
	end = mFinalRetryPackets.end();
	for(iter = mFinalRetryPackets.begin(); iter != end; ++iter)
	{
		packetp = iter->second;
		gMessageSystem->mFailedResendPackets++;
		if(gMessageSystem->mVerboseLog)
		{
			doomed.push_back(packetp->mPacketID);
		}
		if (packetp->mCallback)
		{
			packetp->mCallback(packetp->mCallbackData,LL_ERR_CIRCUIT_GONE);
		}

		// Update stats
		mUnackedPacketCount--;
		mUnackedPacketBytes -= packetp->mBufferLength;

		delete packetp;
	}

	// log aborted reliable packets for this circuit.
	if(gMessageSystem->mVerboseLog && !doomed.empty())
	{
		std::ostringstream str;
		std::ostream_iterator<TPACKETID> append(str, " ");
		str << "MSG: -> " << mHost << "\tABORTING RELIABLE:\t";
		std::copy(doomed.begin(), doomed.end(), append);
		LL_INFOS() << str.str() << LL_ENDL;
	}
}


void LLCircuitData::ackReliablePacket(TPACKETID packet_num)
{
	reliable_iter iter;
	LLReliablePacket *packetp;

	iter = mUnackedPackets.find(packet_num);
	if (iter != mUnackedPackets.end())
	{
		packetp = iter->second;

		if(gMessageSystem->mVerboseLog)
		{
			std::ostringstream str;
			str << "MSG: <- " << packetp->mHost << "\tRELIABLE ACKED:\t"
				<< packetp->mPacketID;
			LL_INFOS() << str.str() << LL_ENDL;
		}
		if (packetp->mCallback)
		{
			if (packetp->mTimeout < F32Seconds(0.f))   // negative timeout will always return timeout even for successful ack, for debugging
			{
				packetp->mCallback(packetp->mCallbackData,LL_ERR_TCP_TIMEOUT);					
			}
			else
			{
				packetp->mCallback(packetp->mCallbackData,LL_ERR_NOERR);
			}
		}

		// Update stats
		mUnackedPacketCount--;
		mUnackedPacketBytes -= packetp->mBufferLength;

		// Cleanup
		delete packetp;
		mUnackedPackets.erase(iter);
		return;
	}

	iter = mFinalRetryPackets.find(packet_num);
	if (iter != mFinalRetryPackets.end())
	{
		packetp = iter->second;
		// LL_INFOS() << "Packet " << packet_num << " removed from the pending list" << LL_ENDL;
		if(gMessageSystem->mVerboseLog)
		{
			std::ostringstream str;
			str << "MSG: <- " << packetp->mHost << "\tRELIABLE ACKED:\t"
				<< packetp->mPacketID;
			LL_INFOS() << str.str() << LL_ENDL;
		}
		if (packetp->mCallback)
		{
			if (packetp->mTimeout < F32Seconds(0.f))   // negative timeout will always return timeout even for successful ack, for debugging
			{
				packetp->mCallback(packetp->mCallbackData,LL_ERR_TCP_TIMEOUT);					
			}
			else
			{
				packetp->mCallback(packetp->mCallbackData,LL_ERR_NOERR);
			}
		}

		// Update stats
		mUnackedPacketCount--;
		mUnackedPacketBytes -= packetp->mBufferLength;

		// Cleanup
		delete packetp;
		mFinalRetryPackets.erase(iter);
	}
	else
	{
		// Couldn't find this packet on either of the unacked lists.
		// maybe it's a duplicate ack?
	}
}



S32 LLCircuitData::resendUnackedPackets(const F64Seconds now)
{
	LLReliablePacket *packetp;


	//
	// Theoretically we should search through the list for the packet with the oldest
	// packet ID, as otherwise when we WRAP we will resend reliable packets out of order.
	// Since resends are ALREADY out of order, and wrapping is highly rare (16+million packets),
	// I'm not going to worry about this for now - djs
	//

	reliable_iter iter;
	bool have_resend_overflow = false;
	for (iter = mUnackedPackets.begin(); iter != mUnackedPackets.end();)
	{
		packetp = iter->second;

		// Only check overflow if we haven't had one yet.
		if (!have_resend_overflow)
		{
			have_resend_overflow = mThrottles.checkOverflow(TC_RESEND, 0);
		}

		if (have_resend_overflow)
		{
			// We've exceeded our bandwidth for resends.
			// Time to stop trying to send them.

			// If we have too many unacked packets, we need to start dropping expired ones.
			if (mUnackedPacketBytes > 512000)
			{
				if (now > packetp->mExpirationTime)
				{
					// This circuit has overflowed.  Do not retry.  Do not pass go.
					packetp->mRetries = 0;
					// Remove it from this list and add it to the final list.
					mUnackedPackets.erase(iter++);
					mFinalRetryPackets[packetp->mPacketID] = packetp;
				}
				else
				{
					++iter;
				}
				// Move on to the next unacked packet.
				continue;
			}
			
			if (mUnackedPacketBytes > 256000 && !(getPacketsOut() % 1024))
			{
				// Warn if we've got a lot of resends waiting.
				LL_WARNS() << mHost << " has " << mUnackedPacketBytes 
						<< " bytes of reliable messages waiting" << LL_ENDL;
			}
			// Stop resending.  There are less than 512000 unacked packets.
			break;
		}

		if (now > packetp->mExpirationTime)
		{
			packetp->mRetries--;
			
			// retry		
			mCurrentResendCount++;

			gMessageSystem->mResentPackets++;

			if(gMessageSystem->mVerboseLog)
			{
				std::ostringstream str;
				str << "MSG: -> " << packetp->mHost
					<< "\tRESENDING RELIABLE:\t" << packetp->mPacketID;
				LL_INFOS() << str.str() << LL_ENDL;
			}

			packetp->mBuffer[0] |= LL_RESENT_FLAG;  // tag packet id as being a resend	

			gMessageSystem->mPacketRing.sendPacket(packetp->mSocket, 
											   (char *)packetp->mBuffer, packetp->mBufferLength, 
											   packetp->mHost);

			mThrottles.throttleOverflow(TC_RESEND, packetp->mBufferLength * 8.f);

			// The new method, retry time based on ping
			if (packetp->mPingBasedRetry)
			{
				packetp->mExpirationTime = now + llmax(LL_MINIMUM_RELIABLE_TIMEOUT_SECONDS, F32Seconds(LL_RELIABLE_TIMEOUT_FACTOR * getPingDelayAveraged()));
			}
			else
			{
				// custom, constant retry time
				packetp->mExpirationTime = now + packetp->mTimeout;
			}

			if (!packetp->mRetries)
			{
				// Last resend, remove it from this list and add it to the final list.
				mUnackedPackets.erase(iter++);
				mFinalRetryPackets[packetp->mPacketID] = packetp;
			}
			else
			{
				// Don't remove it yet, it still gets to try to resend at least once.
				++iter;
			}
		}
		else
		{
			// Don't need to do anything with this packet, keep iterating.
			++iter;
		}
	}


	for (iter = mFinalRetryPackets.begin(); iter != mFinalRetryPackets.end();)
	{
		packetp = iter->second;
		if (now > packetp->mExpirationTime)
		{
			// fail (too many retries)
			//LL_INFOS() << "Packet " << packetp->mPacketID << " removed from the pending list: exceeded retry limit" << LL_ENDL;
			//if (packetp->mMessageName)
			//{
			//	LL_INFOS() << "Packet name " << packetp->mMessageName << LL_ENDL;
			//}
			gMessageSystem->mFailedResendPackets++;

			if(gMessageSystem->mVerboseLog)
			{
				std::ostringstream str;
				str << "MSG: -> " << packetp->mHost << "\tABORTING RELIABLE:\t"
					<< packetp->mPacketID;
				LL_INFOS() << str.str() << LL_ENDL;
			}

			if (packetp->mCallback)
			{
				packetp->mCallback(packetp->mCallbackData,LL_ERR_TCP_TIMEOUT);
			}

			// Update stats
			mUnackedPacketCount--;
			mUnackedPacketBytes -= packetp->mBufferLength;

			mFinalRetryPackets.erase(iter++);
			delete packetp;
		}
		else
		{
			++iter;
		}
	}

	return mUnackedPacketCount;
}


LLCircuit::LLCircuit(const F32Seconds circuit_heartbeat_interval, const F32Seconds circuit_timeout) 
:	mLastCircuit(NULL),  
	mHeartbeatInterval(circuit_heartbeat_interval), 
	mHeartbeatTimeout(circuit_timeout)
{}

LLCircuit::~LLCircuit()
{
	// delete pointers in the map.
	std::for_each(mCircuitData.begin(),
				  mCircuitData.end(),
				  llcompose1(
					  DeletePointerFunctor<LLCircuitData>(),
					  llselect2nd<circuit_data_map::value_type>()));
}

LLCircuitData *LLCircuit::addCircuitData(const LLHost &host, TPACKETID in_id)
{
	// This should really validate if one already exists
	LL_INFOS() << "LLCircuit::addCircuitData for " << host << LL_ENDL;
	LLCircuitData *tempp = new LLCircuitData(host, in_id, mHeartbeatInterval, mHeartbeatTimeout);
	mCircuitData.insert(circuit_data_map::value_type(host, tempp));
	mPingSet.insert(tempp);

	mLastCircuit = tempp;
	return tempp;
}

void LLCircuit::removeCircuitData(const LLHost &host)
{
	LL_INFOS() << "LLCircuit::removeCircuitData for " << host << LL_ENDL;
	mLastCircuit = NULL;
	circuit_data_map::iterator it = mCircuitData.find(host);
	if(it != mCircuitData.end())
	{
		LLCircuitData *cdp = it->second;
		mCircuitData.erase(it);

		LLCircuit::ping_set_t::iterator psit = mPingSet.find(cdp);
		if (psit != mPingSet.end())
        {
		    mPingSet.erase(psit);
		}
		else
		{
			LL_WARNS() << "Couldn't find entry for next ping in ping set!" << LL_ENDL;
		}

		// Clean up from optimization maps
		mUnackedCircuitMap.erase(host);
		mSendAckMap.erase(host);
		delete cdp;
	}

	// This also has to happen AFTER we nuke the circuit, because various
	// callbacks for the circuit may result in messages being sent to
	// this circuit, and the setting of mLastCircuit.  We don't check
	// if the host matches, but we don't really care because mLastCircuit
	// is an optimization, and this happens VERY rarely.
	mLastCircuit = NULL;
}

void LLCircuitData::setAlive(bool b_alive)
{
	if (mbAlive != b_alive)
	{
		mPacketsOutID = 0;
		mPacketsInID = 0;
		mbAlive = b_alive;
	}
	if (b_alive)
	{
		mLastPingReceivedTime = LLMessageSystem::getMessageTimeSeconds();
		mPingsInTransit = 0;
		mBlocked = false;
	}
}


void LLCircuitData::setAllowTimeout(bool allow)
{
	mbAllowTimeout = allow;

	if (allow)
	{
		// resuming circuit
		// make sure it's alive
		setAlive(true);
	}
}


// Reset per-period counters if necessary.
void LLCircuitData::checkPeriodTime()
{
	F64Seconds mt_sec = LLMessageSystem::getMessageTimeSeconds();
	F64Seconds period_length = mt_sec - mPeriodTime;
	if ( period_length > TARGET_PERIOD_LENGTH)
	{
		F32 bps_in = F32Bits(mBytesInThisPeriod).value() / period_length.value();
		if (bps_in > mPeakBPSIn)
		{
			mPeakBPSIn = bps_in;
		}

		F32 bps_out = F32Bits(mBytesOutThisPeriod).value() / period_length.value();
		if (bps_out > mPeakBPSOut)
		{
			mPeakBPSOut = bps_out;
		}

		mBytesInLastPeriod	= mBytesInThisPeriod;
		mBytesOutLastPeriod	= mBytesOutThisPeriod;
		mBytesInThisPeriod	= S32Bytes(0);
		mBytesOutThisPeriod	= S32Bytes(0);
		mLastPeriodLength	= F32Seconds::convert(period_length);

		mPeriodTime = mt_sec;
	}
}


void LLCircuitData::addBytesIn(S32Bytes bytes)
{
	mBytesIn += bytes;
	mBytesInThisPeriod += bytes;
}


void LLCircuitData::addBytesOut(S32Bytes bytes)
{
	mBytesOut += bytes;
	mBytesOutThisPeriod += bytes;
}


void LLCircuitData::addReliablePacket(S32 mSocket, U8 *buf_ptr, S32 buf_len, LLReliablePacketParams *params)
{
	LLReliablePacket *packet_info;

	packet_info = new LLReliablePacket(mSocket, buf_ptr, buf_len, params);

	mUnackedPacketCount++;
	mUnackedPacketBytes += packet_info->mBufferLength;

	if (params && params->mRetries)
	{
		mUnackedPackets[packet_info->mPacketID] = packet_info;
	}
	else
	{
		mFinalRetryPackets[packet_info->mPacketID] = packet_info;
	}
}


void LLCircuit::resendUnackedPackets(S32& unacked_list_length, S32& unacked_list_size)
{
	F64Seconds now = LLMessageSystem::getMessageTimeSeconds();
	unacked_list_length = 0;
	unacked_list_size = 0;

	LLCircuitData* circ;
	circuit_data_map::iterator end = mUnackedCircuitMap.end();
	for(circuit_data_map::iterator it = mUnackedCircuitMap.begin(); it != end; ++it)
	{
		circ = (*it).second;
		unacked_list_length += circ->resendUnackedPackets(now);
		unacked_list_size += circ->getUnackedPacketBytes();
	}
}


bool LLCircuitData::isDuplicateResend(TPACKETID packetnum)
{
	return (mRecentlyReceivedReliablePackets.find(packetnum) != mRecentlyReceivedReliablePackets.end());
}


void LLCircuit::dumpResends()
{
	circuit_data_map::iterator end = mCircuitData.end();
	for(circuit_data_map::iterator it = mCircuitData.begin(); it != end; ++it)
	{
		(*it).second->dumpResendCountAndReset();
	}
}

LLCircuitData* LLCircuit::findCircuit(const LLHost& host) const
{
	// An optimization on finding the previously found circuit.
	if (mLastCircuit && (mLastCircuit->mHost == host))
	{
		return mLastCircuit;
	}

	circuit_data_map::const_iterator it = mCircuitData.find(host);
	if(it == mCircuitData.end())
	{
		return NULL;
	}
	mLastCircuit = it->second;
	return mLastCircuit;
}


bool LLCircuit::isCircuitAlive(const LLHost& host) const
{
	LLCircuitData *cdp = findCircuit(host);
	if(cdp)
	{
		return cdp->mbAlive;
	}

	return false;
}

void LLCircuitData::setTimeoutCallback(void (*callback_func)(const LLHost &host, void *user_data), void *user_data)
{
	mTimeoutCallback = callback_func;
	mTimeoutUserData = user_data; 
}

void LLCircuitData::checkPacketInID(TPACKETID id, bool receive_resent)
{
	// Done as floats so we don't have to worry about running out of room
	// with U32 getting poked into an S32.
	F32 delta = (F32)mHighestPacketID - (F32)id;
	if (delta > (0.5f*LL_MAX_OUT_PACKET_ID))
	{
		// We've almost definitely wrapped, reset the mLastPacketID to be low again.
		mHighestPacketID = id;
	}
	else if (delta < (-0.5f*LL_MAX_OUT_PACKET_ID))
	{
		// This is almost definitely an old packet coming in after a wrap, ignore it.
	}
	else
	{
		mHighestPacketID = llmax(mHighestPacketID, id);
	}

	// Save packet arrival time
	mLastPacketInTime = LLMessageSystem::getMessageTimeSeconds();

	// Have we received anything on this circuit yet?
	if (0 == mPacketsIn)
	{
		// Must be first packet from unclosed circuit.
		mPacketsIn++;
		setPacketInID((id + 1) % LL_MAX_OUT_PACKET_ID);

        mLastPacketGap = 0;
		return;
	}

	mPacketsIn++;


	// now, check to see if we've got a gap
    U32 gap = 0;
	if (mPacketsInID == id)
	{
		// nope! bump and wrap the counter, then return
		mPacketsInID++;
		mPacketsInID = (mPacketsInID) % LL_MAX_OUT_PACKET_ID;
	}
	else if (id < mWrapID)
	{
		// id < mWrapID will happen if the first few packets are out of order. . . 
		// at that point we haven't marked anything "potentially lost" and 
		// the out-of-order packet will cause a full wrap marking all the IDs "potentially lost"

		// do nothing
	}
	else
	{
		// we have a gap!  if that id is in the map, remove it from the map, leave mCurrentCircuit->mPacketsInID
		// alone
		// otherwise, walk from mCurrentCircuit->mPacketsInID to id with wrapping, adding the values to the map
		// and setting mPacketsInID to id + 1 % LL_MAX_OUT_PACKET_ID

        // babbage: all operands in expression are unsigned, so modular 
		// arithmetic will always find correct gap, regardless of wrap arounds.
		const U8 width = 24;
		gap = LLModularMath::subtract<width>(mPacketsInID, id);

		if (mPotentialLostPackets.find(id) != mPotentialLostPackets.end())
		{
			if(gMessageSystem->mVerboseLog)
			{
				std::ostringstream str;
				str << "MSG: <- " << mHost << "\tRECOVERING LOST:\t" << id;
				LL_INFOS() << str.str() << LL_ENDL;
			}
			//			LL_INFOS() << "removing potential lost: " << id << LL_ENDL;
			mPotentialLostPackets.erase(id);
		}
		else if (!receive_resent) // don't freak out over out-of-order reliable resends
		{
			U64Microseconds time = LLMessageSystem::getMessageTimeUsecs();
			TPACKETID index = mPacketsInID;
			S32 gap_count = 0;
			if ((index < id) && ((id - index) < 16))
			{
				while (index != id)
				{
					if(gMessageSystem->mVerboseLog)
					{
						std::ostringstream str;
						str << "MSG: <- " << mHost << "\tPACKET GAP:\t"
							<< index;
						LL_INFOS() << str.str() << LL_ENDL;
					}

//						LL_INFOS() << "adding potential lost: " << index << LL_ENDL;
					mPotentialLostPackets[index] = time;
					index++;
					index = index % LL_MAX_OUT_PACKET_ID;
					gap_count++;
				}
			}
			else
			{
				LL_INFOS() << "packet_out_of_order - got packet " << id << " expecting " << index << " from " << mHost << LL_ENDL;
				if(gMessageSystem->mVerboseLog)
				{
					std::ostringstream str;
					str << "MSG: <- " << mHost << "\tPACKET GAP:\t"
						<< id << " expected " << index;
					LL_INFOS() << str.str() << LL_ENDL;
				}
			}
				
			mPacketsInID = id + 1;
			mPacketsInID = (mPacketsInID) % LL_MAX_OUT_PACKET_ID;

			if (gap_count > 128)
			{
				LL_WARNS() << "Packet loss gap filler running amok!" << LL_ENDL;
			}
			else if (gap_count > 16)
			{
				LL_WARNS() << "Sustaining large amounts of packet loss!" << LL_ENDL;
			}

		}
	}
    mLastPacketGap = gap;
}


void LLCircuit::updateWatchDogTimers(LLMessageSystem *msgsys)
{
	F64Seconds cur_time = LLMessageSystem::getMessageTimeSeconds();
	S32 count = mPingSet.size();
	S32 cur = 0;

	// Only process each circuit once at most, stop processing if no circuits
	while((cur < count) && !mPingSet.empty())
	{
		cur++;

		LLCircuit::ping_set_t::iterator psit = mPingSet.begin();
		LLCircuitData *cdp = *psit;

		if (!cdp->mbAlive)
		{
			// We suspect that this case should never happen, given how
			// the alive status is set.
			// Skip over dead circuits, just add the ping interval and push it to the back
			// Always remember to remove it from the set before changing the sorting
			// key (mNextPingSendTime)
			mPingSet.erase(psit);
			cdp->mNextPingSendTime = cur_time + mHeartbeatInterval;
			mPingSet.insert(cdp);
			continue;
		}
		else
		{
			// Check to see if this needs a ping
			if (cur_time < cdp->mNextPingSendTime)
			{
				// This circuit doesn't need a ping, break out because
				// we have a sorted list, thus no more circuits need pings
				break;
			}

			// Update watchdog timers
			if (cdp->updateWatchDogTimers(msgsys))
            {
				// Randomize our pings a bit by doing some up to 5% early or late
				F64Seconds dt = 0.95f*mHeartbeatInterval + F32Seconds(ll_frand(0.1f*mHeartbeatInterval.value()));

				// Remove it, and reinsert it with the new next ping time.
				// Always remove before changing the sorting key.
				mPingSet.erase(psit);
				cdp->mNextPingSendTime = cur_time + dt;
				mPingSet.insert(cdp);
    
			    // Update our throttles
			    cdp->mThrottles.dynamicAdjust();
    
			    // Update some stats, this is not terribly important
			    cdp->checkPeriodTime();
			}
			else
			{
				// This mPingSet.erase isn't necessary, because removing the circuit will
				// remove the ping set.
				//mPingSet.erase(psit);
				removeCircuitData(cdp->mHost);
			}
		}
	}
}


bool LLCircuitData::updateWatchDogTimers(LLMessageSystem *msgsys)
{
	F64Seconds cur_time = LLMessageSystem::getMessageTimeSeconds();
	mLastPingSendTime = cur_time;

	if (!checkCircuitTimeout())
	{
		// Pass this back to the calling LLCircuit, this circuit needs to be cleaned up.
		return false;
	}

	// WARNING!
	// Duplicate suppression can FAIL if packets are delivered out of
	// order, although it's EXTREMELY unlikely.  It would require
	// that the ping get delivered out of order enough that the ACK
	// for the packet that it was out of order with was received BEFORE
	// the ping was sent.

	// Find the current oldest reliable packetID
	// This is to handle the case if we actually manage to wrap our
	// packet IDs - the oldest will actually have a higher packet ID
	// than the current.
	bool wrapped = false;
	reliable_iter iter;
	iter = mUnackedPackets.upper_bound(getPacketOutID());
	if (iter == mUnackedPackets.end())
	{
		// Nothing AFTER this one, so we want the lowest packet ID
		// then.
		iter = mUnackedPackets.begin();
		wrapped = true;
	}

	TPACKETID packet_id = 0;

	// Check against the "final" packets
	bool wrapped_final = false;
	reliable_iter iter_final;
	iter_final = mFinalRetryPackets.upper_bound(getPacketOutID());
	if (iter_final == mFinalRetryPackets.end())
	{
		iter_final = mFinalRetryPackets.begin();
		wrapped_final = true;
	}

	//LL_INFOS() << mHost << " - unacked count " << mUnackedPackets.size() << LL_ENDL;
	//LL_INFOS() << mHost << " - final count " << mFinalRetryPackets.size() << LL_ENDL;
	if (wrapped != wrapped_final)
	{
		// One of the "unacked" or "final" lists hasn't wrapped.  Whichever one
		// hasn't has the oldest packet.
		if (!wrapped)
		{
			// Hasn't wrapped, so the one on the
			// unacked packet list is older
			packet_id = iter->first;
			//LL_INFOS() << mHost << ": nowrapped unacked" << LL_ENDL;
		}
		else
		{
			packet_id = iter_final->first;
			//LL_INFOS() << mHost << ": nowrapped final" << LL_ENDL;
		}
	}
	else
	{
		// They both wrapped, we can just use the minimum of the two.
		if ((iter == mUnackedPackets.end()) && (iter_final == mFinalRetryPackets.end()))
		{
			// Wow!  No unacked packets at all!
			// Send the ID of the last packet we sent out.
			// This will flush all of the destination's
			// unacked packets, theoretically.
			//LL_INFOS() << mHost << ": No unacked!" << LL_ENDL;
			packet_id = getPacketOutID();
		}
		else
		{
			bool had_unacked = false;
			if (iter != mUnackedPackets.end())
			{
				// Unacked list has the lowest so far
				packet_id = iter->first;
				had_unacked = true;
				//LL_INFOS() << mHost << ": Unacked" << LL_ENDL;
			}

			if (iter_final != mFinalRetryPackets.end())
			{
				// Use the lowest of the unacked list and the final list
				if (had_unacked)
				{
					// Both had a packet, use the lowest.
					packet_id = llmin(packet_id, iter_final->first);
					//LL_INFOS() << mHost << ": Min of unacked/final" << LL_ENDL;
				}
				else
				{
					// Only the final had a packet, use it.
					packet_id = iter_final->first;
					//LL_INFOS() << mHost << ": Final!" << LL_ENDL;
				}
			}
		}
	}

	// Send off the another ping.
	pingTimerStart();
	msgsys->newMessageFast(_PREHASH_StartPingCheck);
	msgsys->nextBlock(_PREHASH_PingID);
	msgsys->addU8Fast(_PREHASH_PingID, nextPingID());
	msgsys->addU32Fast(_PREHASH_OldestUnacked, packet_id);
	msgsys->sendMessage(mHost);

	// Also do lost packet accounting.
	// Check to see if anything on our lost list is old enough to
	// be considered lost

	LLCircuitData::packet_time_map::iterator it;
	U64Microseconds timeout = llmin(LL_MAX_LOST_TIMEOUT, F32Seconds(getPingDelayAveraged()) * LL_LOST_TIMEOUT_FACTOR);

	U64Microseconds mt_usec = LLMessageSystem::getMessageTimeUsecs();
	for (it = mPotentialLostPackets.begin(); it != mPotentialLostPackets.end(); )
	{
		U64Microseconds delta_t_usec = mt_usec - (*it).second;
		if (delta_t_usec > timeout)
		{
			// let's call this one a loss!
			mPacketsLost++;
			gMessageSystem->mDroppedPackets++;
			if(gMessageSystem->mVerboseLog)
			{
				std::ostringstream str;
				str << "MSG: <- " << mHost << "\tLOST PACKET:\t"
					<< (*it).first;
				LL_INFOS() << str.str() << LL_ENDL;
			}
			mPotentialLostPackets.erase(it++);
		}
		else
		{
			++it;
		}
	}

	return true;
}


void LLCircuitData::clearDuplicateList(TPACKETID oldest_id)
{
	// purge old data from the duplicate suppression queue

	// we want to KEEP all x where oldest_id <= x <= last incoming packet, and delete everything else.

	//LL_INFOS() << mHost << ": clearing before oldest " << oldest_id << LL_ENDL;
	//LL_INFOS() << "Recent list before: " << mRecentlyReceivedReliablePackets.size() << LL_ENDL;
	if (oldest_id < mHighestPacketID)
	{
		// Clean up everything with a packet ID less than oldest_id.
		packet_time_map::iterator pit_start;
		packet_time_map::iterator pit_end;
		pit_start = mRecentlyReceivedReliablePackets.begin();
		pit_end = mRecentlyReceivedReliablePackets.lower_bound(oldest_id);
		mRecentlyReceivedReliablePackets.erase(pit_start, pit_end);
	}

	// Do timeout checks on everything with an ID > mHighestPacketID.
	// This should be empty except for wrapping IDs.  Thus, this should be
	// highly rare.
	U64Microseconds mt_usec = LLMessageSystem::getMessageTimeUsecs();

	packet_time_map::iterator pit;
	for(pit = mRecentlyReceivedReliablePackets.upper_bound(mHighestPacketID);
		pit != mRecentlyReceivedReliablePackets.end(); )
	{
		// Validate that the packet ID seems far enough away
		if ((pit->first - mHighestPacketID) < 100)
		{
			LL_WARNS() << "Probably incorrectly timing out non-wrapped packets!" << LL_ENDL;
		}
		U64Microseconds delta_t_usec = mt_usec - (*pit).second;
		F64Seconds delta_t_sec = delta_t_usec;
		if (delta_t_sec > LL_DUPLICATE_SUPPRESSION_TIMEOUT)
		{
			// enough time has elapsed we're not likely to get a duplicate on this one
			LL_INFOS() << "Clearing " << pit->first << " from recent list" << LL_ENDL;
			mRecentlyReceivedReliablePackets.erase(pit++);
		}
		else
		{
			++pit;
		}
	}
	//LL_INFOS() << "Recent list after: " << mRecentlyReceivedReliablePackets.size() << LL_ENDL;
}

bool LLCircuitData::checkCircuitTimeout()
{
	F64Seconds time_since_last_ping = LLMessageSystem::getMessageTimeSeconds() - mLastPingReceivedTime;

	// Nota Bene: This needs to be turned off if you are debugging multiple simulators
	if (time_since_last_ping > mHeartbeatTimeout)
	{
		LL_WARNS() << "LLCircuitData::checkCircuitTimeout for " << mHost << " last ping " << time_since_last_ping << " seconds ago." <<LL_ENDL;
		setAlive(false);
		if (mTimeoutCallback)
		{
			LL_WARNS() << "LLCircuitData::checkCircuitTimeout for " << mHost << " calling callback." << LL_ENDL;
			mTimeoutCallback(mHost, mTimeoutUserData);
		}
		if (!isAlive())
		{
			// The callback didn't try and resurrect the circuit.  We should kill it.
			LL_WARNS() << "LLCircuitData::checkCircuitTimeout for " << mHost << " still dead, dropping." << LL_ENDL;
			return false;
		}
	}

	return true;
}

// Call this method when a reliable message comes in - this will
// correctly place the packet in the correct list to be acked later.
bool LLCircuitData::collectRAck(TPACKETID packet_num)
{
	if (mAcks.empty())
	{
		// First extra ack, we need to add ourselves to the list of circuits that need to send acks
		gMessageSystem->mCircuitInfo.mSendAckMap[mHost] = this;
	}

	mAcks.push_back(packet_num);
	if (mAckCreationTime == 0)
	{
		mAckCreationTime = getAgeInSeconds();
	}
	return true;
}

// this method is called during the message system processAcks() to
// send out any acks that did not get sent already.
void LLCircuit::sendAcks(F32 collect_time)
{
	collect_time = llclamp(collect_time, 0.f, LL_COLLECT_ACK_TIME_MAX);
	LLCircuitData* cd;
	circuit_data_map::iterator it = mSendAckMap.begin();
	while (it != mSendAckMap.end())
	{
		circuit_data_map::iterator cur_it = it++;
		cd = (*cur_it).second;
		S32 count = (S32)cd->mAcks.size();
		F32 age = cd->getAgeInSeconds() - cd->mAckCreationTime;
		if (age > collect_time || count == 0)
		{
			if (count>0)
			{
				// send the packet acks
				S32 acks_this_packet = 0;
				for(S32 i = 0; i < count; ++i)
				{
					if(acks_this_packet == 0)
					{
						gMessageSystem->newMessageFast(_PREHASH_PacketAck);
					}
					gMessageSystem->nextBlockFast(_PREHASH_Packets);
					gMessageSystem->addU32Fast(_PREHASH_ID, cd->mAcks[i]);
					++acks_this_packet;
					if(acks_this_packet > 250)
					{
						gMessageSystem->sendMessage(cd->mHost);
						acks_this_packet = 0;
					}
				}
				if(acks_this_packet > 0)
				{
					gMessageSystem->sendMessage(cd->mHost);
				}

				if(gMessageSystem->mVerboseLog)
				{
					std::ostringstream str;
					str << "MSG: -> " << cd->mHost << "\tPACKET ACKS:\t";
					std::ostream_iterator<TPACKETID> append(str, " ");
					std::copy(cd->mAcks.begin(), cd->mAcks.end(), append);
					LL_INFOS() << str.str() << LL_ENDL;
				}

				// empty out the acks list
				cd->mAcks.clear();
				cd->mAckCreationTime = 0.f;
			}
			// remove data map
			mSendAckMap.erase(cur_it);
		}
	}
}


std::ostream& operator<<(std::ostream& s, LLCircuitData& circuit)
{
	F32 age = circuit.mExistenceTimer.getElapsedTimeF32();

	using namespace std;
	s << "Circuit " << circuit.mHost << " "
		<< circuit.mRemoteID << " "
		<< (circuit.mbAlive ? "Alive" : "Not Alive") << " "
		<< (circuit.mbAllowTimeout ? "Timeout Allowed" : "Timeout Not Allowed")
		<< endl;

	s << " Packets Lost: " << circuit.mPacketsLost
		<< " Measured Ping: " << circuit.mPingDelay
		<< " Averaged Ping: " << circuit.mPingDelayAveraged
		<< endl;

	s << "Global In/Out " << S32(age) << " sec"
		<< " KBytes: " << circuit.mBytesIn.valueInUnits<LLUnits::Kilobytes>() << "/" << circuit.mBytesOut.valueInUnits<LLUnits::Kilobytes>()
		<< " Kbps: "
		<< S32(circuit.mBytesIn.valueInUnits<LLUnits::Kilobits>() / circuit.mExistenceTimer.getElapsedTimeF32().value())
		<< "/"
		<< S32(circuit.mBytesOut.valueInUnits<LLUnits::Kilobits>() / circuit.mExistenceTimer.getElapsedTimeF32().value())
		<< " Packets: " << circuit.mPacketsIn << "/" << circuit.mPacketsOut
		<< endl;

	s << "Recent In/Out   " << circuit.mLastPeriodLength
		<< " KBytes: "
		<< circuit.mBytesInLastPeriod.valueInUnits<LLUnits::Kilobytes>()
		<< "/"
		<< circuit.mBytesOutLastPeriod.valueInUnits<LLUnits::Kilobytes>()
		<< " Kbps: "
		<< (S32)(circuit.mBytesInLastPeriod.valueInUnits<LLUnits::Kilobits>() / circuit.mLastPeriodLength.value())
		<< "/"
		<< (S32)(circuit.mBytesOutLastPeriod.valueInUnits<LLUnits::Kilobits>() / circuit.mLastPeriodLength.value())
		<< " Peak kbps: "
		<< S32(circuit.mPeakBPSIn / 1024.f)
		<< "/"
		<< S32(circuit.mPeakBPSOut / 1024.f)
		<< endl;

	return s;
}

void LLCircuitData::getInfo(LLSD& info) const
{
	info["Host"] = mHost.getIPandPort();
	info["Alive"] = mbAlive;
	info["Age"] = mExistenceTimer.getElapsedTimeF32();
}

void LLCircuitData::dumpResendCountAndReset()
{
	if (mCurrentResendCount)
	{
		LL_INFOS() << "Circuit: " << mHost << " resent " << mCurrentResendCount << " packets" << LL_ENDL;
		mCurrentResendCount = 0;
	}
}

std::ostream& operator<<(std::ostream& s, LLCircuit &circuit)
{
	s << "Circuit Info:" << std::endl;
	LLCircuit::circuit_data_map::iterator end = circuit.mCircuitData.end();
	LLCircuit::circuit_data_map::iterator it;
	for(it = circuit.mCircuitData.begin(); it != end; ++it)
	{
		s << *((*it).second) << std::endl;
	}
	return s;
}

void LLCircuit::getInfo(LLSD& info) const
{
	LLCircuit::circuit_data_map::const_iterator end = mCircuitData.end();
	LLCircuit::circuit_data_map::const_iterator it;
	LLSD circuit_info;
	for(it = mCircuitData.begin(); it != end; ++it)
	{
		(*it).second->getInfo(circuit_info);
		info["Circuits"].append(circuit_info);
	}
}

void LLCircuit::getCircuitRange(
	const LLHost& key,
	LLCircuit::circuit_data_map::iterator& first,
	LLCircuit::circuit_data_map::iterator& end)
{
	end = mCircuitData.end();
	first = mCircuitData.upper_bound(key);
}

TPACKETID LLCircuitData::nextPacketOutID()
{
	mPacketsOut++;
	
	TPACKETID id; 

	id = (mPacketsOutID + 1) % LL_MAX_OUT_PACKET_ID;
			
	if (id < mPacketsOutID)
	{
		// we just wrapped on a circuit, reset the wrap ID to zero
		mWrapID = 0;
	}
	mPacketsOutID = id;
	return id;
}


void LLCircuitData::setPacketInID(TPACKETID id)
{
	id = id % LL_MAX_OUT_PACKET_ID;
	mPacketsInID = id;
	mRecentlyReceivedReliablePackets.clear();

	mWrapID = id;
}


void LLCircuitData::pingTimerStop(const U8 ping_id)
{
	F64Seconds mt_secs = LLMessageSystem::getMessageTimeSeconds();

	// Nota Bene: no averaging of ping times until we get a feel for how this works
	F64Seconds time = mt_secs - mPingTime;
	if (time == F32Seconds(0.0))
	{
		// Ack, we got our ping response on the same frame! Sigh, let's get a real time otherwise
		// all of our ping calculations will be skewed.
		mt_secs = LLMessageSystem::getMessageTimeSeconds(true);
	}
	mLastPingReceivedTime = mt_secs;

	// If ping is longer than 1 second, we'll get sequence deltas in the ping.
	// Approximate by assuming each ping counts for 1 second (slightly low, probably)
	S32 delta_ping = (S32)mLastPingID - (S32) ping_id;
	if (delta_ping < 0)
	{
		delta_ping += 256;
	}

	U32Milliseconds msec = delta_ping*mHeartbeatInterval + time;
	setPingDelay(msec);

	mPingsInTransit = delta_ping;
	if (mBlocked && (mPingsInTransit <= PING_RELEASE_BLOCK))
	{
		mBlocked = false;
	}
}


void LLCircuitData::pingTimerStart()
{
	mPingTime = LLMessageSystem::getMessageTimeSeconds();
	mPingsInTransit++;

	if (!mBlocked && (mPingsInTransit > PING_START_BLOCK))
	{
		mBlocked = true;
	}
}


U32 LLCircuitData::getPacketsIn() const
{
	return mPacketsIn;
}


S32Bytes LLCircuitData::getBytesIn() const
{
	return mBytesIn;
}


S32Bytes LLCircuitData::getBytesOut() const
{
	return mBytesOut;
}


U32 LLCircuitData::getPacketsOut() const
{
	return mPacketsOut;
}


TPACKETID LLCircuitData::getPacketOutID() const
{
	return mPacketsOutID;
}


U32 LLCircuitData::getPacketsLost() const
{
	return mPacketsLost;
}


bool LLCircuitData::isAlive() const
{
	return mbAlive;
}


bool LLCircuitData::isBlocked() const
{
	return mBlocked;
}


bool LLCircuitData::getAllowTimeout() const
{
	return mbAllowTimeout;
}


U32Milliseconds LLCircuitData::getPingDelay() const
{
	return mPingDelay;
}


F32Milliseconds LLCircuitData::getPingInTransitTime()
{
	// This may be inaccurate in the case of a circuit that was "dead" and then revived,
	// but only until the first round trip ping is sent - djs
	F32Milliseconds time_since_ping_was_sent(0);

	if (mPingsInTransit)
	{
		time_since_ping_was_sent =  F32Milliseconds::convert(((mPingsInTransit*mHeartbeatInterval - F32Seconds(1)) 
			+ (LLMessageSystem::getMessageTimeSeconds() - mPingTime)));
	}

	return time_since_ping_was_sent;
}


void LLCircuitData::setPingDelay(U32Milliseconds ping)
{
	mPingDelay = ping;
	mPingDelayAveraged = llmax((F32Milliseconds)ping, getPingDelayAveraged());
	mPingDelayAveraged = ((1.f - LL_AVERAGED_PING_ALPHA) * mPingDelayAveraged) 
						  + (LL_AVERAGED_PING_ALPHA * (F32Milliseconds) ping);
	mPingDelayAveraged = llclamp(mPingDelayAveraged, 
								 LL_AVERAGED_PING_MIN,
								 LL_AVERAGED_PING_MAX);
}


F32Milliseconds LLCircuitData::getPingDelayAveraged()
{
	return llmin(llmax(getPingInTransitTime(), mPingDelayAveraged), LL_AVERAGED_PING_MAX);
}


bool LLCircuitData::getTrusted() const
{
	return mTrusted;
}


void LLCircuitData::setTrusted(bool t)
{
	mTrusted = t;
}

F32 LLCircuitData::getAgeInSeconds() const
{
	return mExistenceTimer.getElapsedTimeF32();
}
