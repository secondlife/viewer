/** 
 * @file lltransfermanager.cpp
 * @brief Improved transfer mechanism for moving data through the
 * message system.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "lltransfermanager.h"

#include "llerror.h"
#include "message.h"
#include "lldatapacker.h"

#include "lltransfersourcefile.h"
#include "lltransfersourceasset.h"
#include "lltransfertargetfile.h"
#include "lltransfertargetvfile.h"

const S32 MAX_PACKET_DATA_SIZE = 2048;
const S32 MAX_PARAMS_SIZE = 1024;

LLTransferManager gTransferManager;
LLTransferSource::stype_scfunc_map LLTransferSource::sSourceCreateMap;

//
// LLTransferManager implementation
//

LLTransferManager::LLTransferManager() :
	mValid(false)
{
	S32 i;
	for (i = 0; i < LLTTT_NUM_TYPES; i++)
	{
		mTransferBitsIn[i] = 0;
		mTransferBitsOut[i] = 0;
	}
}


LLTransferManager::~LLTransferManager()
{
	// LLTransferManager should have been cleaned up by message system shutdown process
	llassert(!mValid);
	if (mValid)
	{
		// Usually happens if OS tries to kill viewer
		cleanup();
	}
}


void LLTransferManager::init()
{
	if (mValid)
	{
		LL_ERRS() << "Double initializing LLTransferManager!" << LL_ENDL;
	}
	mValid = true;

	// Register message system handlers
	gMessageSystem->setHandlerFunc("TransferRequest", processTransferRequest, NULL);
	gMessageSystem->setHandlerFunc("TransferInfo", processTransferInfo, NULL);
	gMessageSystem->setHandlerFunc("TransferPacket", processTransferPacket, NULL);
	gMessageSystem->setHandlerFunc("TransferAbort", processTransferAbort, NULL);
}


void LLTransferManager::cleanup()
{
	mValid = false;

	host_tc_map::iterator iter;
	for (iter = mTransferConnections.begin(); iter != mTransferConnections.end(); iter++)
	{
		delete iter->second;
	}
	mTransferConnections.clear();
}


void LLTransferManager::updateTransfers()
{
	host_tc_map::iterator iter,cur;

	iter = mTransferConnections.begin();

	while (iter !=mTransferConnections.end())
	{
		cur = iter;
		iter++;
		cur->second->updateTransfers();
	}
}


void LLTransferManager::cleanupConnection(const LLHost &host)
{
	host_tc_map::iterator iter;
	iter = mTransferConnections.find(host);
	if (iter == mTransferConnections.end())
	{
		// This can happen legitimately if we've never done a transfer, and we're
		// cleaning up a circuit.
		//LL_WARNS() << "Cleaning up nonexistent transfer connection to " << host << LL_ENDL;
		return;
	}
	LLTransferConnection *connp = iter->second;
	delete connp;
	mTransferConnections.erase(iter);
}


LLTransferConnection *LLTransferManager::getTransferConnection(const LLHost &host)
{
	host_tc_map::iterator iter;
	iter = mTransferConnections.find(host);
	if (iter == mTransferConnections.end())
	{
		mTransferConnections[host] = new LLTransferConnection(host);
		return mTransferConnections[host];
	}

	return iter->second;
}


LLTransferSourceChannel *LLTransferManager::getSourceChannel(const LLHost &host, const LLTransferChannelType type)
{
	LLTransferConnection *tcp = getTransferConnection(host);
	if (!tcp)
	{
		return NULL;
	}
	return tcp->getSourceChannel(type);
}



LLTransferTargetChannel *LLTransferManager::getTargetChannel(const LLHost &host, const LLTransferChannelType type)
{
	LLTransferConnection *tcp = getTransferConnection(host);
	if (!tcp)
	{
		return NULL;
	}
	return tcp->getTargetChannel(type);
}

// virtual
LLTransferSourceParams::~LLTransferSourceParams()
{ }


LLTransferSource *LLTransferManager::findTransferSource(const LLUUID &transfer_id)
{
	// This linear traversal could screw us later if we do lots of
	// searches for sources.  However, this ONLY happens right now
	// in asset transfer callbacks, so this should be relatively quick.
	host_tc_map::iterator iter;
	for (iter = mTransferConnections.begin(); iter != mTransferConnections.end(); iter++)
	{
		LLTransferConnection *tcp = iter->second;
		LLTransferConnection::tsc_iter sc_iter;
		for (sc_iter = tcp->mTransferSourceChannels.begin(); sc_iter != tcp->mTransferSourceChannels.end(); sc_iter++)
		{
			LLTransferSourceChannel *scp = *sc_iter;
			LLTransferSource *sourcep = scp->findTransferSource(transfer_id);
			if (sourcep)
			{
				return sourcep;
			}
		}
	}

	return NULL;
}

//
// Message handlers
//

//static
void LLTransferManager::processTransferRequest(LLMessageSystem *msgp, void **)
{
	//LL_INFOS() << "LLTransferManager::processTransferRequest" << LL_ENDL;

	LLUUID transfer_id;
	LLTransferSourceType source_type;
	LLTransferChannelType channel_type;
	F32 priority;

	msgp->getUUID("TransferInfo", "TransferID", transfer_id);
	msgp->getS32("TransferInfo", "SourceType", (S32 &)source_type);
	msgp->getS32("TransferInfo", "ChannelType", (S32 &)channel_type);
	msgp->getF32("TransferInfo", "Priority", priority);

	LLTransferSourceChannel *tscp = gTransferManager.getSourceChannel(msgp->getSender(), channel_type);

	if (!tscp)
	{
		LL_WARNS() << "Source channel not found" << LL_ENDL;
		return;
	}

	if (tscp->findTransferSource(transfer_id))
	{
		LL_WARNS() << "Duplicate request for transfer " << transfer_id << ", aborting!" << LL_ENDL;
		return;
	}

	S32 size = msgp->getSize("TransferInfo", "Params");
	if(size > MAX_PARAMS_SIZE)
	{
		LL_WARNS() << "LLTransferManager::processTransferRequest params too big."
			<< LL_ENDL;
		return;
	}

	//LL_INFOS() << transfer_id << ":" << source_type << ":" << channel_type << ":" << priority << LL_ENDL;
	LLTransferSource* tsp = LLTransferSource::createSource(
		source_type,
		transfer_id,
		priority);
	if(!tsp)
	{
		LL_WARNS() << "LLTransferManager::processTransferRequest couldn't create"
			<< " transfer source!" << LL_ENDL;
		return;
	}
	U8 tmp[MAX_PARAMS_SIZE];
	msgp->getBinaryData("TransferInfo", "Params", tmp, size);

	LLDataPackerBinaryBuffer dpb(tmp, MAX_PARAMS_SIZE);
	bool unpack_ok = tsp->unpackParams(dpb);
	if (!unpack_ok)
	{
		// This should only happen if the data is corrupt or
		// incorrectly packed.
		// *NOTE: We may want to call abortTransfer().
		LL_WARNS() << "LLTransferManager::processTransferRequest: bad parameters."
			<< LL_ENDL;
		delete tsp;
		return;
	}

	tscp->addTransferSource(tsp);
	tsp->initTransfer();
}


//static
void LLTransferManager::processTransferInfo(LLMessageSystem *msgp, void **)
{
	//LL_INFOS() << "LLTransferManager::processTransferInfo" << LL_ENDL;

	LLUUID transfer_id;
	LLTransferTargetType target_type;
	LLTransferChannelType channel_type;
	LLTSCode status;
	S32 size;

	msgp->getUUID("TransferInfo", "TransferID", transfer_id);
	msgp->getS32("TransferInfo", "TargetType", (S32 &)target_type);
	msgp->getS32("TransferInfo", "ChannelType", (S32 &)channel_type);
	msgp->getS32("TransferInfo", "Status", (S32 &)status);
	msgp->getS32("TransferInfo", "Size", size);

	//LL_INFOS() << transfer_id << ":" << target_type<< ":" << channel_type << LL_ENDL;
	LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(msgp->getSender(), channel_type);
	if (!ttcp)
	{
		LL_WARNS() << "Target channel not found" << LL_ENDL;
		// Should send a message to abort the transfer.
		return;
	}

	LLTransferTarget *ttp = ttcp->findTransferTarget(transfer_id);
	if (!ttp)
	{
		LL_WARNS() << "TransferInfo for unknown transfer!  Not able to handle this yet!" << LL_ENDL;
		// This could happen if we're doing a push transfer, although to avoid confusion,
		// maybe it should be a different message.
		return;
	}

	if (status != LLTS_OK)
	{
		LL_WARNS() << transfer_id << ": Non-ok status, cleaning up" << LL_ENDL;
		ttp->completionCallback(status);
		// Clean up the transfer.
		ttcp->deleteTransfer(ttp);
		return;
	}

	// unpack the params
	S32 params_size = msgp->getSize("TransferInfo", "Params");
	if(params_size > MAX_PARAMS_SIZE)
	{
		LL_WARNS() << "LLTransferManager::processTransferInfo params too big."
			<< LL_ENDL;
		return;
	}
	else if(params_size > 0)
	{
		U8 tmp[MAX_PARAMS_SIZE];
		msgp->getBinaryData("TransferInfo", "Params", tmp, params_size);
		LLDataPackerBinaryBuffer dpb(tmp, MAX_PARAMS_SIZE);
		if (!ttp->unpackParams(dpb))
		{
			// This should only happen if the data is corrupt or
			// incorrectly packed.
			LL_WARNS() << "LLTransferManager::processTransferRequest: bad params."
				<< LL_ENDL;
			ttp->abortTransfer();
			ttcp->deleteTransfer(ttp);
			return;
		}
	}

	//LL_INFOS() << "Receiving " << transfer_id << ", size " << size << " bytes" << LL_ENDL;
	ttp->setSize(size);
	ttp->setGotInfo(true);

	// OK, at this point we to handle any delayed transfer packets (which could happen
	// if this packet was lost)

	// This is a lame cut and paste of code down below.  If we change the logic down there,
	// we HAVE to change the logic up here.

	while (1)
	{
		S32 packet_id = 0;
		U8 tmp_data[MAX_PACKET_DATA_SIZE];
		// See if we've got any delayed packets
		packet_id = ttp->getNextPacketID();
		if (ttp->mDelayedPacketMap.find(packet_id) != ttp->mDelayedPacketMap.end())
		{
			// Perhaps this stuff should be inside a method in LLTransferPacket?
			// I'm too lazy to do it now, though.
// 			LL_INFOS() << "Playing back delayed packet " << packet_id << LL_ENDL;
			LLTransferPacket *packetp = ttp->mDelayedPacketMap[packet_id];

			// This is somewhat inefficient, but avoids us having to duplicate
			// code between the off-the-wire and delayed paths.
			packet_id = packetp->mPacketID;
			size = packetp->mSize;
			if (size)
			{
				if ((packetp->mDatap != NULL) && (size<(S32)sizeof(tmp_data)))
				{
					memcpy(tmp_data, packetp->mDatap, size);	/*Flawfinder: ignore*/
				}
			}
			status = packetp->mStatus;
			ttp->mDelayedPacketMap.erase(packet_id);
			delete packetp;
		}
		else
		{
			// No matching delayed packet, we're done.
			break;
		}

		LLTSCode ret_code = ttp->dataCallback(packet_id, tmp_data, size);
		if (ret_code == LLTS_OK)
		{
			ttp->setLastPacketID(packet_id);
		}

		if (status != LLTS_OK)
		{
			if (status != LLTS_DONE)
			{
				LL_WARNS() << "LLTransferManager::processTransferInfo Error in playback!" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "LLTransferManager::processTransferInfo replay FINISHED for " << transfer_id << LL_ENDL;
			}
			// This transfer is done, either via error or not.
			ttp->completionCallback(status);
			ttcp->deleteTransfer(ttp);
			return;
		}
	}
}


//static
void LLTransferManager::processTransferPacket(LLMessageSystem *msgp, void **)
{
	//LL_INFOS() << "LLTransferManager::processTransferPacket" << LL_ENDL;

	LLUUID transfer_id;
	LLTransferChannelType channel_type;
	S32 packet_id;
	LLTSCode status;
	S32 size;
	msgp->getUUID("TransferData", "TransferID", transfer_id);
	msgp->getS32("TransferData", "ChannelType", (S32 &)channel_type);
	msgp->getS32("TransferData", "Packet", packet_id);
	msgp->getS32("TransferData", "Status", (S32 &)status);

	// Find the transfer associated with this packet.
	//LL_INFOS() << transfer_id << ":" << channel_type << LL_ENDL;
	LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(msgp->getSender(), channel_type);
	if (!ttcp)
	{
		LL_WARNS() << "Target channel not found" << LL_ENDL;
		return;
	}

	LLTransferTarget *ttp = ttcp->findTransferTarget(transfer_id);
	if (!ttp)
	{
		LL_WARNS() << "Didn't find matching transfer for " << transfer_id
			<< " processing packet " << packet_id
			<< " from " << msgp->getSender() << LL_ENDL;
		return;
	}

	size = msgp->getSize("TransferData", "Data");

	S32 msg_bytes = 0;
	if (msgp->getReceiveCompressedSize())
	{
		msg_bytes = msgp->getReceiveCompressedSize();
	}
	else
	{
		msg_bytes = msgp->getReceiveSize();
	}
	gTransferManager.addTransferBitsIn(ttcp->mChannelType, msg_bytes*8);

	if ((size < 0) || (size > MAX_PACKET_DATA_SIZE))
	{
		LL_WARNS() << "Invalid transfer packet size " << size << LL_ENDL;
		return;
	}

	U8 tmp_data[MAX_PACKET_DATA_SIZE];
	if (size > 0)
	{
		// Only pull the data out if the size is > 0
		msgp->getBinaryData("TransferData", "Data", tmp_data, size);
	}

	if ((!ttp->gotInfo()) || (ttp->getNextPacketID() != packet_id))
	{
		// Put this on a list of packets to be delivered later.
		if(!ttp->addDelayedPacket(packet_id, status, tmp_data, size))
		{
			// Whoops - failed to add a delayed packet for some reason.
			LL_WARNS() << "Too many delayed packets processing transfer "
				<< transfer_id << " from " << msgp->getSender() << LL_ENDL;
			ttp->abortTransfer();
			ttcp->deleteTransfer(ttp);
			return;
		}
#if 0
		// Spammy!
		const S32 LL_TRANSFER_WARN_GAP = 10;
		if(!ttp->gotInfo())
		{
			LL_WARNS() << "Got data packet before information in transfer "
				<< transfer_id << " from " << msgp->getSender()
				<< ", got " << packet_id << LL_ENDL;
		}
		else if((packet_id - ttp->getNextPacketID()) > LL_TRANSFER_WARN_GAP)
		{
			LL_WARNS() << "Out of order packet in transfer " << transfer_id
				<< " from " << msgp->getSender() << ", got " << packet_id
				<< " expecting " << ttp->getNextPacketID() << LL_ENDL;
		}
#endif
		return;
	}

	// Loop through this until we're done with all delayed packets
	
	//
	// NOTE: THERE IS A CUT AND PASTE OF THIS CODE IN THE TRANSFERINFO HANDLER
	// SO WE CAN PLAY BACK DELAYED PACKETS THERE!!!!!!!!!!!!!!!!!!!!!!!!!
	//
	bool done = false;
	while (!done)
	{
		LLTSCode ret_code = ttp->dataCallback(packet_id, tmp_data, size);
		if (ret_code == LLTS_OK)
		{
			ttp->setLastPacketID(packet_id);
		}

		if (status != LLTS_OK)
		{
			if (status != LLTS_DONE)
			{
				LL_WARNS() << "LLTransferManager::processTransferPacket Error in transfer!" << LL_ENDL;
			}
			else
			{
// 				LL_INFOS() << "LLTransferManager::processTransferPacket done for " << transfer_id << LL_ENDL;
			}
			// This transfer is done, either via error or not.
			ttp->completionCallback(status);
			ttcp->deleteTransfer(ttp);
			return;
		}

		// See if we've got any delayed packets
		packet_id = ttp->getNextPacketID();
		if (ttp->mDelayedPacketMap.find(packet_id) != ttp->mDelayedPacketMap.end())
		{
			// Perhaps this stuff should be inside a method in LLTransferPacket?
			// I'm too lazy to do it now, though.
// 			LL_INFOS() << "Playing back delayed packet " << packet_id << LL_ENDL;
			LLTransferPacket *packetp = ttp->mDelayedPacketMap[packet_id];

			// This is somewhat inefficient, but avoids us having to duplicate
			// code between the off-the-wire and delayed paths.
			packet_id = packetp->mPacketID;
			size = packetp->mSize;
			if (size)
			{
				if ((packetp->mDatap != NULL) && (size<(S32)sizeof(tmp_data)))
				{
					memcpy(tmp_data, packetp->mDatap, size);	/*Flawfinder: ignore*/
				}
			}
			status = packetp->mStatus;
			ttp->mDelayedPacketMap.erase(packet_id);
			delete packetp;
		}
		else
		{
			// No matching delayed packet, abort it.
			done = true;
		}
	}
}


//static
void LLTransferManager::processTransferAbort(LLMessageSystem *msgp, void **)
{
	//LL_INFOS() << "LLTransferManager::processTransferPacket" << LL_ENDL;

	LLUUID transfer_id;
	LLTransferChannelType channel_type;
	msgp->getUUID("TransferInfo", "TransferID", transfer_id);
	msgp->getS32("TransferInfo", "ChannelType", (S32 &)channel_type);

	// See if it's a target that we're trying to abort
	// Find the transfer associated with this packet.
	LLTransferTargetChannel *ttcp = gTransferManager.getTargetChannel(msgp->getSender(), channel_type);
	if (ttcp)
	{
		LLTransferTarget *ttp = ttcp->findTransferTarget(transfer_id);
		if (ttp)
		{
			ttp->abortTransfer();
			ttcp->deleteTransfer(ttp);
			return;
		}
	}

	// Hmm, not a target.  Maybe it's a source.
	LLTransferSourceChannel *tscp = gTransferManager.getSourceChannel(msgp->getSender(), channel_type);
	if (tscp)
	{
		LLTransferSource *tsp = tscp->findTransferSource(transfer_id);
		if (tsp)
		{
			tsp->abortTransfer();
			tscp->deleteTransfer(tsp);
			return;
		}
	}

	LL_WARNS() << "Couldn't find transfer " << transfer_id << " to abort!" << LL_ENDL;
}


//static
void LLTransferManager::reliablePacketCallback(void **user_data, S32 result)
{
	LLUUID *transfer_idp = (LLUUID *)user_data;
	if (result &&
		transfer_idp != NULL)
	{
		LLTransferSource *tsp = gTransferManager.findTransferSource(*transfer_idp);
		if (tsp)
		{
			LL_WARNS() << "Aborting reliable transfer " << *transfer_idp << " due to failed reliable resends!" << LL_ENDL;
			LLTransferSourceChannel *tscp = tsp->mChannelp;
			tsp->abortTransfer();
			tscp->deleteTransfer(tsp);
		}
		else
		{
			LL_WARNS() << "Aborting reliable transfer " << *transfer_idp << " but can't find the LLTransferSource object" << LL_ENDL;
		}
	}
	delete transfer_idp;
}

//
// LLTransferConnection implementation
//

LLTransferConnection::LLTransferConnection(const LLHost &host)
{
	mHost = host;
}

LLTransferConnection::~LLTransferConnection()
{
	tsc_iter itersc;
	for (itersc = mTransferSourceChannels.begin(); itersc != mTransferSourceChannels.end(); itersc++)
	{
		delete *itersc;
	}
	mTransferSourceChannels.clear();

	ttc_iter itertc;
	for (itertc = mTransferTargetChannels.begin(); itertc != mTransferTargetChannels.end(); itertc++)
	{
		delete *itertc;
	}
	mTransferTargetChannels.clear();
}


void LLTransferConnection::updateTransfers()
{
	// Do stuff for source transfers (basically, send data out).
	tsc_iter iter, cur;
	iter = mTransferSourceChannels.begin();

	while (iter !=mTransferSourceChannels.end())
	{
		cur = iter;
		iter++;
		(*cur)->updateTransfers();
	}

	// Do stuff for target transfers
	// Primarily, we should be aborting transfers that are irredeemably broken
	// (large packet gaps that don't appear to be getting filled in, most likely)
	// Probably should NOT be doing timeouts for other things, as new priority scheme
	// means that a high priority transfer COULD block a transfer for a long time.
}


LLTransferSourceChannel *LLTransferConnection::getSourceChannel(const LLTransferChannelType channel_type)
{
	tsc_iter iter;
	for (iter = mTransferSourceChannels.begin(); iter != mTransferSourceChannels.end(); iter++)
	{
		if ((*iter)->getChannelType() == channel_type)
		{
			return *iter;
		}
	}

	LLTransferSourceChannel *tscp = new LLTransferSourceChannel(channel_type, mHost);
	mTransferSourceChannels.push_back(tscp);
	return tscp;
}


LLTransferTargetChannel *LLTransferConnection::getTargetChannel(const LLTransferChannelType channel_type)
{
	ttc_iter iter;
	for (iter = mTransferTargetChannels.begin(); iter != mTransferTargetChannels.end(); iter++)
	{
		if ((*iter)->getChannelType() == channel_type)
		{
			return *iter;
		}
	}

	LLTransferTargetChannel *ttcp = new LLTransferTargetChannel(channel_type, mHost);
	mTransferTargetChannels.push_back(ttcp);
	return ttcp;
}


//
// LLTransferSourceChannel implementation
//

const S32 DEFAULT_PACKET_SIZE = 1000;


LLTransferSourceChannel::LLTransferSourceChannel(const LLTransferChannelType channel_type, const LLHost &host) :
	mChannelType(channel_type),
	mHost(host),
	mTransferSources(LLTransferSource::sSetPriority, LLTransferSource::sGetPriority),
	mThrottleID(TC_ASSET)
{
}


LLTransferSourceChannel::~LLTransferSourceChannel()
{
	LLPriQueueMap<LLTransferSource*>::pqm_iter iter =
		mTransferSources.mMap.begin();
	LLPriQueueMap<LLTransferSource*>::pqm_iter end =
		mTransferSources.mMap.end();
	for (; iter != end; ++iter)
	{
		// Just kill off all of the transfers
		(*iter).second->abortTransfer();
		delete iter->second;
	}
	mTransferSources.mMap.clear();
}

void LLTransferSourceChannel::updatePriority(LLTransferSource *tsp, const F32 priority)
{
	mTransferSources.reprioritize(priority, tsp);
}

void LLTransferSourceChannel::updateTransfers()
{
	// Actually, this should do the following:
	// Decide if we can actually send data.
	// If so, update priorities so we know who gets to send it.
	// Send data from the sources, while updating until we've sent our throttle allocation.

	LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(getHost());
	if (!cdp)
	{
		return;
	}

	if (cdp->isBlocked())
	{
		// *NOTE: We need to make sure that the throttle bits
		// available gets reset.

		// We DON'T want to send any packets if they're blocked, they'll just end up
		// piling up on the other end.
		//LL_WARNS() << "Blocking transfers due to blocked circuit for " << getHost() << LL_ENDL;
		return;
	}

	const S32 throttle_id = mThrottleID;

	LLThrottleGroup &tg = cdp->getThrottleGroup();

	if (tg.checkOverflow(throttle_id, 0.f))
	{
		return;
	}

	LLPriQueueMap<LLTransferSource *>::pqm_iter iter, next;

	bool done = false;
	for (iter = mTransferSources.mMap.begin(); (iter != mTransferSources.mMap.end()) && !done;)
	{
		//LL_INFOS() << "LLTransferSourceChannel::updateTransfers()" << LL_ENDL;
		// Do stuff. 
		next = iter;
		next++;

		LLTransferSource *tsp = iter->second;
		U8 *datap = NULL;
		S32 data_size = 0;
		bool delete_data = false;
		S32 packet_id = 0;
		S32 sent_bytes = 0;
		LLTSCode status = LLTS_OK;

		// Get the packetID for the next packet that we're transferring.
		packet_id = tsp->getNextPacketID();
		status = tsp->dataCallback(packet_id, DEFAULT_PACKET_SIZE, &datap, data_size, delete_data);

		if (status == LLTS_SKIP)
		{
			// We don't have any data, but we're not done, just go on.
			// This will presumably be used for streaming or async transfers that
			// are stalled waiting for data from another source.
			iter=next;
			continue;
		}

		LLUUID *cb_uuid = new LLUUID(tsp->getID());
		LLUUID transaction_id = tsp->getID();

		// Send the data now, even if it's an error.
		// The status code will tell the other end what to do.
		gMessageSystem->newMessage("TransferPacket");
		gMessageSystem->nextBlock("TransferData");
		gMessageSystem->addUUID("TransferID", tsp->getID());
		gMessageSystem->addS32("ChannelType", getChannelType());
		gMessageSystem->addS32("Packet", packet_id);	// HACK!  Need to put in a REAL packet id
		gMessageSystem->addS32("Status", status);
		gMessageSystem->addBinaryData("Data", datap, data_size);
		sent_bytes = gMessageSystem->getCurrentSendTotal();
		gMessageSystem->sendReliable(getHost(), LL_DEFAULT_RELIABLE_RETRIES, true, F32Seconds(0.f),
									 LLTransferManager::reliablePacketCallback, (void**)cb_uuid);

		// Do bookkeeping for the throttle
		done = tg.throttleOverflow(throttle_id, sent_bytes*8.f);
		gTransferManager.addTransferBitsOut(mChannelType, sent_bytes*8);

		// Clean up our temporary data.
		if (delete_data)
		{
			delete[] datap;
			datap = NULL;
		}

		if (findTransferSource(transaction_id) == NULL)
		{
			//Warning!  In the case of an aborted transfer, the sendReliable call above calls 
			//AbortTransfer which in turn calls deleteTransfer which means that somewhere way 
			//down the chain our current iter can get invalidated resulting in an infrequent
			//sim crash.  This check gets us to a valid transfer source in this event.
			iter=next;
			continue;
		}

		// Update the packet counter
		tsp->setLastPacketID(packet_id);

		switch (status)
		{
		case LLTS_OK:
			// We're OK, don't need to do anything.  Keep sending data.
			break;
		case LLTS_ERROR:
			LL_WARNS() << "Error in transfer dataCallback!" << LL_ENDL;
			// fall through
		case LLTS_DONE:
			// We need to clean up this transfer source.
			//LL_INFOS() << "LLTransferSourceChannel::updateTransfers() " << tsp->getID() << " done" << LL_ENDL;
			tsp->completionCallback(status);
			delete tsp;
			
			mTransferSources.mMap.erase(iter);
			iter = next;
			break;
		default:
			LL_ERRS() << "Unknown transfer error code!" << LL_ENDL;
		}

		// At this point, we should do priority adjustment (since some transfers like
		// streaming transfers will adjust priority based on how much they've sent and time,
		// but I'm not going to bother yet. - djs.
	}
}


void LLTransferSourceChannel::addTransferSource(LLTransferSource *sourcep)
{
	sourcep->mChannelp = this;
	mTransferSources.push(sourcep->getPriority(), sourcep);
}


LLTransferSource *LLTransferSourceChannel::findTransferSource(const LLUUID &transfer_id)
{
	LLPriQueueMap<LLTransferSource *>::pqm_iter iter;
	for (iter = mTransferSources.mMap.begin(); iter != mTransferSources.mMap.end(); iter++)
	{
		LLTransferSource *tsp = iter->second;
		if (tsp->getID() == transfer_id)
		{
			return tsp;
		}
	}
	return NULL;
}


void LLTransferSourceChannel::deleteTransfer(LLTransferSource *tsp)
{
	if (tsp)
	{
		LLPriQueueMap<LLTransferSource *>::pqm_iter iter;
		for (iter = mTransferSources.mMap.begin(); iter != mTransferSources.mMap.end(); iter++)
		{
			if (iter->second == tsp)
			{
				delete tsp;
				mTransferSources.mMap.erase(iter);
				return;
			}
		}

		LL_WARNS() << "Unable to find transfer source id " 
			<< tsp->getID()
			<< " to delete!" 
			<< LL_ENDL;
	}
}


//
// LLTransferTargetChannel implementation
//

LLTransferTargetChannel::LLTransferTargetChannel(const LLTransferChannelType channel_type, const LLHost &host) :
	mChannelType(channel_type),
	mHost(host)
{
}

LLTransferTargetChannel::~LLTransferTargetChannel()
{
	tt_iter iter;
	for (iter = mTransferTargets.begin(); iter != mTransferTargets.end(); iter++)
	{
		// Abort all of the current transfers
		(*iter)->abortTransfer();
		delete *iter;
	}
	mTransferTargets.clear();
}


void LLTransferTargetChannel::requestTransfer(
	const LLTransferSourceParams& source_params,
	const LLTransferTargetParams& target_params,
	const F32 priority)
{
	LLUUID id;
	id.generate();
	LLTransferTarget* ttp = LLTransferTarget::createTarget(
		target_params.getType(),
		id,
		source_params.getType());
	if (!ttp)
	{
		LL_WARNS() << "LLTransferManager::requestTransfer aborting due to target creation failure!" << LL_ENDL;
		return;
	}

	ttp->applyParams(target_params);
	addTransferTarget(ttp);

	sendTransferRequest(ttp, source_params, priority);
}


void LLTransferTargetChannel::sendTransferRequest(LLTransferTarget *targetp,
												  const LLTransferSourceParams &params,
												  const F32 priority)
{
	//
	// Pack the message with data which explains how to get the source, and
	// send it off to the source for this channel.
	//
	llassert(targetp);
	llassert(targetp->getChannel() == this);

	gMessageSystem->newMessage("TransferRequest");
	gMessageSystem->nextBlock("TransferInfo");
	gMessageSystem->addUUID("TransferID", targetp->getID());
	gMessageSystem->addS32("SourceType", params.getType());
	gMessageSystem->addS32("ChannelType", getChannelType());
	gMessageSystem->addF32("Priority", priority);

	U8 tmp[MAX_PARAMS_SIZE];
	LLDataPackerBinaryBuffer dp(tmp, MAX_PARAMS_SIZE);
	params.packParams(dp);
	S32 len = dp.getCurrentSize();
	gMessageSystem->addBinaryData("Params", tmp, len);

	gMessageSystem->sendReliable(mHost);
}


void LLTransferTargetChannel::addTransferTarget(LLTransferTarget *targetp)
{
	targetp->mChannelp = this;
	mTransferTargets.push_back(targetp);
}


LLTransferTarget *LLTransferTargetChannel::findTransferTarget(const LLUUID &transfer_id)
{
	tt_iter iter;
	for (iter = mTransferTargets.begin(); iter != mTransferTargets.end(); iter++)
	{
		LLTransferTarget *ttp = *iter;
		if (ttp->getID() == transfer_id)
		{
			return ttp;
		}
	}
	return NULL;
}


void LLTransferTargetChannel::deleteTransfer(LLTransferTarget *ttp)
{
	if (ttp)
	{
		tt_iter iter;
		for (iter = mTransferTargets.begin(); iter != mTransferTargets.end(); iter++)
		{
			if (*iter == ttp)
			{
				delete ttp;
				mTransferTargets.erase(iter);
				return;
			}
		}

		LL_WARNS() << "Unable to find transfer target id " 
			<< ttp->getID()
			<< " to delete!" 
			<< LL_ENDL;
	}
}


//
// LLTransferSource implementation
//

LLTransferSource::LLTransferSource(const LLTransferSourceType type,
								   const LLUUID &transfer_id,
								   const F32 priority) :
	mType(type),
	mID(transfer_id),
	mChannelp(NULL),
	mPriority(priority),
	mSize(0),
	mLastPacketID(-1)
{
	setPriority(priority);
}


LLTransferSource::~LLTransferSource()
{
	// No actual cleanup of the transfer is done here, this is purely for
	// memory cleanup.  The completionCallback is guaranteed to get called
	// before this happens.
}


void LLTransferSource::sendTransferStatus(LLTSCode status)
{
	gMessageSystem->newMessage("TransferInfo");
	gMessageSystem->nextBlock("TransferInfo");
	gMessageSystem->addUUID("TransferID", getID());
	gMessageSystem->addS32("TargetType", LLTTT_UNKNOWN);
	gMessageSystem->addS32("ChannelType", mChannelp->getChannelType());
	gMessageSystem->addS32("Status", status);
	gMessageSystem->addS32("Size", mSize);
	U8 tmp[MAX_PARAMS_SIZE];
	LLDataPackerBinaryBuffer dp(tmp, MAX_PARAMS_SIZE);
	packParams(dp);
	S32 len = dp.getCurrentSize();
	gMessageSystem->addBinaryData("Params", tmp, len);
	gMessageSystem->sendReliable(mChannelp->getHost());

	// Abort if there was as asset system issue.
	if (status != LLTS_OK)
	{
		completionCallback(status);
		mChannelp->deleteTransfer(this);
	}
}


// This should never be called directly, the transfer manager is responsible for
// aborting the transfer from the channel.  I might want to rethink this in the
// future, though.
void LLTransferSource::abortTransfer()
{
	// Send a message down, call the completion callback
	LL_INFOS() << "LLTransferSource::Aborting transfer " << getID() << " to " << mChannelp->getHost() << LL_ENDL;
	gMessageSystem->newMessage("TransferAbort");
	gMessageSystem->nextBlock("TransferInfo");
	gMessageSystem->addUUID("TransferID", getID());
	gMessageSystem->addS32("ChannelType", mChannelp->getChannelType());
	gMessageSystem->sendReliable(mChannelp->getHost());

	completionCallback(LLTS_ABORT);
}


//static
void LLTransferSource::registerSourceType(const LLTransferSourceType stype, LLTransferSourceCreateFunc func)
{
	if (sSourceCreateMap.count(stype))
	{
		// Disallow changing what class handles a source type
		// Unclear when you would want to do this, and whether it would work.
		LL_ERRS() << "Reregistering source type " << stype << LL_ENDL;
	}
	else
	{
		sSourceCreateMap[stype] = func;
	}
}

//static
LLTransferSource *LLTransferSource::createSource(const LLTransferSourceType stype,
												 const LLUUID &id,
												 const F32 priority)
{
	switch (stype)
	{
	// *NOTE: The source file transfer mechanism is highly insecure and could
	// lead to easy exploitation of a server process.
	// I have removed all uses of it from the codebase. Phoenix.
	// 
	//case LLTST_FILE:
	//	return new LLTransferSourceFile(id, priority);
	case LLTST_ASSET:
		return new LLTransferSourceAsset(id, priority);
	default:
		{
			if (!sSourceCreateMap.count(stype))
			{
				// Use the callback to create the source type if it's not there.
				LL_WARNS() << "Unknown transfer source type: " << stype << LL_ENDL;
				return NULL;
			}
			return (sSourceCreateMap[stype])(id, priority);
		}
	}
}


// static
void LLTransferSource::sSetPriority(LLTransferSource *&tsp, const F32 priority)
{
	tsp->setPriority(priority);
}


// static
F32 LLTransferSource::sGetPriority(LLTransferSource *&tsp)
{
	return tsp->getPriority();
}


//
// LLTransferPacket implementation
//

LLTransferPacket::LLTransferPacket(const S32 packet_id, const LLTSCode status, const U8 *datap, const S32 size) :
	mPacketID(packet_id),
	mStatus(status),
	mDatap(NULL),
	mSize(size)
{
	if (size == 0)
	{
		return;
	}
	
	mDatap = new U8[size];
	if (mDatap != NULL)
	{
		memcpy(mDatap, datap, size);	/*Flawfinder: ignore*/
	}
}

LLTransferPacket::~LLTransferPacket()
{
	delete[] mDatap;
}

//
// LLTransferTarget implementation
//

LLTransferTarget::LLTransferTarget(
	LLTransferTargetType type,
	const LLUUID& transfer_id,
	LLTransferSourceType source_type) : 
	mType(type),
	mSourceType(source_type),
	mID(transfer_id),
	mChannelp(NULL),
	mGotInfo(false),
	mSize(0),
	mLastPacketID(-1)
{
}

LLTransferTarget::~LLTransferTarget()
{
	// No actual cleanup of the transfer is done here, this is purely for
	// memory cleanup.  The completionCallback is guaranteed to get called
	// before this happens.
	tpm_iter iter;
	for (iter = mDelayedPacketMap.begin(); iter != mDelayedPacketMap.end(); iter++)
	{
		delete iter->second;
	}
	mDelayedPacketMap.clear();
}

// This should never be called directly, the transfer manager is responsible for
// aborting the transfer from the channel.  I might want to rethink this in the
// future, though.
void LLTransferTarget::abortTransfer()
{
	// Send a message up, call the completion callback
	LL_INFOS() << "LLTransferTarget::Aborting transfer " << getID() << " from " << mChannelp->getHost() << LL_ENDL;
	gMessageSystem->newMessage("TransferAbort");
	gMessageSystem->nextBlock("TransferInfo");
	gMessageSystem->addUUID("TransferID", getID());
	gMessageSystem->addS32("ChannelType", mChannelp->getChannelType());
	gMessageSystem->sendReliable(mChannelp->getHost());

	completionCallback(LLTS_ABORT);
}

bool LLTransferTarget::addDelayedPacket(
	const S32 packet_id,
	const LLTSCode status,
	U8* datap,
	const S32 size)
{
	const transfer_packet_map::size_type LL_MAX_DELAYED_PACKETS = 100;
	if(mDelayedPacketMap.size() > LL_MAX_DELAYED_PACKETS)
	{
		// too many delayed packets
		return false;
	}

	LLTransferPacket* tpp = new LLTransferPacket(
		packet_id,
		status,
		datap,
		size);

#ifdef _DEBUG
    transfer_packet_map::iterator iter = mDelayedPacketMap.find(packet_id);
	if (iter != mDelayedPacketMap.end())
	{
        if (!(iter->second->mSize == size) && !(iter->second->mDatap == datap))
        {
            LL_ERRS() << "Packet ALREADY in delayed packet map!" << LL_ENDL;
        }
	}
#endif

	mDelayedPacketMap[packet_id] = tpp;
	return true;
}


LLTransferTarget* LLTransferTarget::createTarget(
	LLTransferTargetType type,
	const LLUUID& id,
	LLTransferSourceType source_type)
{
	switch (type)
	{
	case LLTTT_FILE:
		return new LLTransferTargetFile(id, source_type);
	case LLTTT_VFILE:
		return new LLTransferTargetVFile(id, source_type);
	default:
		LL_WARNS() << "Unknown transfer target type: " << type << LL_ENDL;
		return NULL;
	}
}


LLTransferSourceParamsInvItem::LLTransferSourceParamsInvItem() : LLTransferSourceParams(LLTST_SIM_INV_ITEM), mAssetType(LLAssetType::AT_NONE)
{
}


void LLTransferSourceParamsInvItem::setAgentSession(const LLUUID &agent_id, const LLUUID &session_id)
{
	mAgentID = agent_id;
	mSessionID = session_id;
}


void LLTransferSourceParamsInvItem::setInvItem(const LLUUID &owner_id, const LLUUID &task_id, const LLUUID &item_id)
{
	mOwnerID = owner_id;
	mTaskID = task_id;
	mItemID = item_id;
}


void LLTransferSourceParamsInvItem::setAsset(const LLUUID &asset_id, const LLAssetType::EType asset_type)
{
	mAssetID = asset_id;
	mAssetType = asset_type;
}


void LLTransferSourceParamsInvItem::packParams(LLDataPacker &dp) const
{
	LL_DEBUGS() << "LLTransferSourceParamsInvItem::packParams()" << LL_ENDL;
	dp.packUUID(mAgentID, "AgentID");
	dp.packUUID(mSessionID, "SessionID");
	dp.packUUID(mOwnerID, "OwnerID");
	dp.packUUID(mTaskID, "TaskID");
	dp.packUUID(mItemID, "ItemID");
	dp.packUUID(mAssetID, "AssetID");
	dp.packS32(mAssetType, "AssetType");
}


bool LLTransferSourceParamsInvItem::unpackParams(LLDataPacker &dp)
{
	S32 tmp_at;

	dp.unpackUUID(mAgentID, "AgentID");
	dp.unpackUUID(mSessionID, "SessionID");
	dp.unpackUUID(mOwnerID, "OwnerID");
	dp.unpackUUID(mTaskID, "TaskID");
	dp.unpackUUID(mItemID, "ItemID");
	dp.unpackUUID(mAssetID, "AssetID");
	dp.unpackS32(tmp_at, "AssetType");

	mAssetType = (LLAssetType::EType)tmp_at;

	return true;
}

LLTransferSourceParamsEstate::LLTransferSourceParamsEstate() :
	LLTransferSourceParams(LLTST_SIM_ESTATE),
	mEstateAssetType(ET_NONE),
	mAssetType(LLAssetType::AT_NONE)
{
}

void LLTransferSourceParamsEstate::setAgentSession(const LLUUID &agent_id, const LLUUID &session_id)
{
	mAgentID = agent_id;
	mSessionID = session_id;
}

void LLTransferSourceParamsEstate::setEstateAssetType(const EstateAssetType etype)
{
	mEstateAssetType = etype;
}

void LLTransferSourceParamsEstate::setAsset(const LLUUID &asset_id, const LLAssetType::EType asset_type)
{
	mAssetID = asset_id;
	mAssetType = asset_type;
}

void LLTransferSourceParamsEstate::packParams(LLDataPacker &dp) const
{
	dp.packUUID(mAgentID, "AgentID");
	// *NOTE: We do not want to pass the session id from the server to
	// the client, but I am not sure if anyone expects this value to
	// be set on the client.
	dp.packUUID(mSessionID, "SessionID");
	dp.packS32(mEstateAssetType, "EstateAssetType");
}


bool LLTransferSourceParamsEstate::unpackParams(LLDataPacker &dp)
{
	S32 tmp_et;

	dp.unpackUUID(mAgentID, "AgentID");
	dp.unpackUUID(mSessionID, "SessionID");
	dp.unpackS32(tmp_et, "EstateAssetType");

	mEstateAssetType = (EstateAssetType)tmp_et;

	return true;
}
