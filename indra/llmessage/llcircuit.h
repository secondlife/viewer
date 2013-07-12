/** 
 * @file llcircuit.h
 * @brief Provides a method for tracking network circuit information
 * for the UDP message system
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

#ifndef LL_LLCIRCUIT_H
#define LL_LLCIRCUIT_H

#include <map>
#include <vector>

#include "llerror.h"

#include "lltimer.h"
#include "timing.h"
#include "net.h"
#include "llhost.h"
#include "llpacketack.h"
#include "lluuid.h"
#include "llthrottle.h"

//
// Constants
//
const F32 LL_AVERAGED_PING_ALPHA = 0.2f;  // relaxation constant on ping running average
const F32 LL_AVERAGED_PING_MAX = 2000;    // msec
const F32 LL_AVERAGED_PING_MIN =  100;    // msec  // IW: increased to avoid retransmits when a process is slow

const U32 INITIAL_PING_VALUE_MSEC = 1000; // initial value for the ping delay, or for ping delay for an unknown circuit

const TPACKETID LL_MAX_OUT_PACKET_ID = 0x01000000;

// 0 - flags
// [1,4] - packetid
// 5 - data offset (after message name)
const U8 LL_PACKET_ID_SIZE = 6;

const S32 LL_MAX_RESENT_PACKETS_PER_FRAME = 100;
const S32 LL_MAX_ACKED_PACKETS_PER_FRAME = 200;

//
// Prototypes and Predefines
//
class LLMessageSystem;
class LLEncodedDatagramService;
class LLSD;

//
// Classes
//


class LLCircuitData
{
public:
	LLCircuitData(const LLHost &host, TPACKETID in_id, 
				  const F32 circuit_heartbeat_interval, const F32 circuit_timeout);
	~LLCircuitData();

	S32		resendUnackedPackets(const F64 now);
	void	clearDuplicateList(TPACKETID oldest_id);


	void	dumpResendCountAndReset(); // Used for tracking how many resends are being done on a circuit.



	// Public because stupid message system callbacks uses it.
	void		pingTimerStart();
	void		pingTimerStop(const U8 ping_id);
	void			ackReliablePacket(TPACKETID packet_num);

	// remote computer information
	const LLUUID& getRemoteID() const { return mRemoteID; }
	const LLUUID& getRemoteSessionID() const { return mRemoteSessionID; }
	void setRemoteID(const LLUUID& id) { mRemoteID = id; }
	void setRemoteSessionID(const LLUUID& id) { mRemoteSessionID = id; }

	void		setTrusted(BOOL t);

	// The local end point ID is used when establishing a trusted circuit.
	// no matching set function for getLocalEndPointID()
	// mLocalEndPointID should only ever be setup in the LLCircuitData constructor
	const		LLUUID& getLocalEndPointID() const { return mLocalEndPointID; }

	U32		getPingDelay() const;
	S32				getPingsInTransit() const			{ return mPingsInTransit; }

	// ACCESSORS
	BOOL		isAlive() const;
	BOOL		isBlocked() const;
	BOOL		getAllowTimeout() const;
	F32			getPingDelayAveraged();
	F32			getPingInTransitTime();
	U32			getPacketsIn() const;
	S32			getBytesIn() const;
	S32			getBytesOut() const;
	U32			getPacketsOut() const;
	U32			getPacketsLost() const;
	TPACKETID	getPacketOutID() const;
	BOOL		getTrusted() const;
	F32			getAgeInSeconds() const;
	S32			getUnackedPacketCount() const	{ return mUnackedPacketCount; }
	S32			getUnackedPacketBytes() const	{ return mUnackedPacketBytes; }
	F64         getNextPingSendTime() const { return mNextPingSendTime; }
    U32         getLastPacketGap() const { return mLastPacketGap; }
    LLHost      getHost() const { return mHost; }
	F64			getLastPacketInTime() const		{ return mLastPacketInTime;	}

	LLThrottleGroup &getThrottleGroup()		{	return mThrottles; }

	class less
	{
	public:
		bool operator()(const LLCircuitData* lhs, const LLCircuitData* rhs) const
		{
			if (lhs->getNextPingSendTime() < rhs->getNextPingSendTime())
			{
				return true;
			}
			else if (lhs->getNextPingSendTime() > rhs->getNextPingSendTime())
			{
				return false;
			}
			else return lhs > rhs;
		}
	};

	//
	// Debugging stuff (not necessary for operation)
	//
	void					checkPeriodTime();		// Reset per-period counters if necessary.
	friend std::ostream&	operator<<(std::ostream& s, LLCircuitData &circuit);
	void getInfo(LLSD& info) const;

	friend class LLCircuit;
	friend class LLMessageSystem;
	friend class LLEncodedDatagramService;
	friend void crash_on_spaceserver_timeout (const LLHost &host, void *); // HACK, so it has access to setAlive() so it can send a final shutdown message.
protected:
	TPACKETID		nextPacketOutID();
	void				setPacketInID(TPACKETID id);
	void					checkPacketInID(TPACKETID id, BOOL receive_resent);
	void			setPingDelay(U32 ping);
	BOOL			checkCircuitTimeout();	// Return FALSE if the circuit is dead and should be cleaned up

	void			addBytesIn(S32 bytes);
	void			addBytesOut(S32 bytes);

	U8				nextPingID()			{ mLastPingID++; return mLastPingID; }

	BOOL			updateWatchDogTimers(LLMessageSystem *msgsys);	// Return FALSE if the circuit is dead and should be cleaned up

	void			addReliablePacket(S32 mSocket, U8 *buf_ptr, S32 buf_len, LLReliablePacketParams *params);
	BOOL			isDuplicateResend(TPACKETID packetnum);
	// Call this method when a reliable message comes in - this will
	// correctly place the packet in the correct list to be acked
	// later. RAack = requested ack
	BOOL collectRAck(TPACKETID packet_num);


	void			setTimeoutCallback(void (*callback_func)(const LLHost &host, void *user_data), void *user_data);



	void			setAlive(BOOL b_alive);
	void			setAllowTimeout(BOOL allow);

protected:
	// Identification for this circuit.
	LLHost mHost;
	LLUUID mRemoteID;
	LLUUID mRemoteSessionID;

	LLThrottleGroup	mThrottles;

	TPACKETID		mWrapID;

	// Current packet IDs of incoming/outgoing packets
	// Used for packet sequencing/packet loss detection.
	TPACKETID		mPacketsOutID;
	TPACKETID		mPacketsInID;
	TPACKETID		mHighestPacketID;


	// Callback and data to run in the case of a circuit timeout.
	// Used primarily to try and reconnect to servers if they crash/die.
	void	(*mTimeoutCallback)(const LLHost &host, void *user_data);
	void	*mTimeoutUserData;

	BOOL	mTrusted;					// Is this circuit trusted?
	BOOL	mbAllowTimeout;				// Machines can "pause" circuits, forcing them not to be dropped

	BOOL	mbAlive;					// Indicates whether a circuit is "alive", i.e. responded to pings

	BOOL	mBlocked;					// Blocked is true if the circuit is hosed, i.e. far behind on pings

	// Not sure what the difference between this and mLastPingSendTime is
	F64		mPingTime;					// Time at which a ping was sent.

	F64		mLastPingSendTime;			// Time we last sent a ping
	F64		mLastPingReceivedTime;		// Time we last received a ping
	F64     mNextPingSendTime;          // Time to try and send the next ping
	S32		mPingsInTransit;			// Number of pings in transit
	U8		mLastPingID;				// ID of the last ping that we sent out


	// Used for determining the resend time for reliable resends.
	U32		mPingDelay;             // raw ping delay
	F32		mPingDelayAveraged;     // averaged ping delay (fast attack/slow decay)

	typedef std::map<TPACKETID, U64> packet_time_map;

	packet_time_map							mPotentialLostPackets;
	packet_time_map							mRecentlyReceivedReliablePackets;
	std::vector<TPACKETID> mAcks;

	typedef std::map<TPACKETID, LLReliablePacket *> reliable_map;
	typedef reliable_map::iterator					reliable_iter;

	reliable_map							mUnackedPackets;
	reliable_map							mFinalRetryPackets;

	S32										mUnackedPacketCount;
	S32										mUnackedPacketBytes;

	F64										mLastPacketInTime;		// Time of last packet arrival

	LLUUID									mLocalEndPointID;

	//
	// These variables are being used for statistical and debugging purpose ONLY,
	// as far as I can tell.
	//

	U32		mPacketsOut;
	U32		mPacketsIn;
	S32		mPacketsLost;
	S32		mBytesIn;
	S32		mBytesOut;

	F32		mLastPeriodLength;		// seconds
	S32		mBytesInLastPeriod;
	S32		mBytesOutLastPeriod;
	S32		mBytesInThisPeriod;
	S32		mBytesOutThisPeriod;
	F32		mPeakBPSIn;				// bits per second, max of all period bps
	F32		mPeakBPSOut;			// bits per second, max of all period bps
	F64		mPeriodTime;
	LLTimer	mExistenceTimer;	    // initialized when circuit created, used to track bandwidth numbers

	S32		mCurrentResendCount;	// Number of resent packets since last spam
    U32     mLastPacketGap;         // Gap in sequence number of last packet.

	const F32 mHeartbeatInterval;
	const F32 mHeartbeatTimeout;
};


// Actually a singleton class -- the global messagesystem
// has a single LLCircuit member.
class LLCircuit
{
public:
	// CREATORS
	LLCircuit(const F32 circuit_heartbeat_interval, const F32 circuit_timeout);
	~LLCircuit();

	// ACCESSORS
	LLCircuitData* findCircuit(const LLHost& host) const;
	BOOL isCircuitAlive(const LLHost& host) const;
	
	// MANIPULATORS
	LLCircuitData	*addCircuitData(const LLHost &host, TPACKETID in_id);
	void			removeCircuitData(const LLHost &host);

	void		    updateWatchDogTimers(LLMessageSystem *msgsys);
	void			resendUnackedPackets(S32& unacked_list_length, S32& unacked_list_size);

	// this method is called during the message system processAcks()
	// to send out any acks that did not get sent already. 
	void sendAcks();

	friend std::ostream& operator<<(std::ostream& s, LLCircuit &circuit);
	void getInfo(LLSD& info) const;

	void			dumpResends();

	typedef std::map<LLHost, LLCircuitData*> circuit_data_map;

	/**
	 * @brief This method gets an iterator range starting after key in
	 * the circuit data map.
	 *
	 * @param key The the host before first.
	 * @param first[out] The first matching value after key. This
	 * value will equal end if there are no entries.
	 * @param end[out] The end of the iteration sequence.
	 */
	void getCircuitRange(
		const LLHost& key,
		circuit_data_map::iterator& first,
		circuit_data_map::iterator& end);

	// Lists that optimize how many circuits we need to traverse a frame
	// HACK - this should become protected eventually, but stupid !@$@# message system/circuit classes are jumbling things up.
	circuit_data_map mUnackedCircuitMap; // Map of circuits with unacked data
	circuit_data_map mSendAckMap; // Map of circuits which need to send acks
protected:
	circuit_data_map mCircuitData;

	typedef std::set<LLCircuitData *, LLCircuitData::less> ping_set_t; // Circuits sorted by next ping time
	ping_set_t mPingSet;

	// This variable points to the last circuit data we found to
	// optimize the many, many times we call findCircuit. This may be
	// set in otherwise const methods, so it is declared mutable.
	mutable LLCircuitData* mLastCircuit;

private:
	const F32 mHeartbeatInterval;
	const F32 mHeartbeatTimeout;
};
#endif
