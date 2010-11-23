/** 
 * @file llremoteparcelrequest.cpp
 * @author Sam Kolb
 * @brief Get information about a parcel you aren't standing in to display
 * landmark/teleport information.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "message.h"

#include "llpanel.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include "llviewerregion.h"
#include "llview.h"

#include "llagent.h"
#include "llremoteparcelrequest.h"


LLRemoteParcelRequestResponder::LLRemoteParcelRequestResponder(LLHandle<LLRemoteParcelInfoObserver> observer_handle)
	 : mObserverHandle(observer_handle)
{}

//If we get back a normal response, handle it here
//virtual
void LLRemoteParcelRequestResponder::result(const LLSD& content)
{
	LLUUID parcel_id = content["parcel_id"];

	// Panel inspecting the information may be closed and destroyed
	// before this response is received.
	LLRemoteParcelInfoObserver* observer = mObserverHandle.get();
	if (observer)
	{
		observer->setParcelID(parcel_id);
	}
}

//If we get back an error (not found, etc...), handle it here
//virtual
void LLRemoteParcelRequestResponder::error(U32 status, const std::string& reason)
{
	llinfos << "LLRemoteParcelRequest::error("
		<< status << ": " << reason << ")" << llendl;

	// Panel inspecting the information may be closed and destroyed
	// before this response is received.
	LLRemoteParcelInfoObserver* observer = mObserverHandle.get();
	if (observer)
	{
		observer->setErrorStatus(status, reason);
	}
}

void LLRemoteParcelInfoProcessor::addObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer)
{
	observer_multimap_t::iterator it;
	observer_multimap_t::iterator end = mObservers.upper_bound(parcel_id);

	// Check if the observer is already in observers list for this UUID
	for(it = mObservers.find(parcel_id); it != end; ++it)
	{
		if (it->second.get() == observer)
		{
			return;
		}
	}

	mObservers.insert(std::make_pair(parcel_id, observer->getObserverHandle()));
}

void LLRemoteParcelInfoProcessor::removeObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer)
{
	if (!observer)
	{
		return;
	}

	observer_multimap_t::iterator it;
	observer_multimap_t::iterator end = mObservers.upper_bound(parcel_id);

	for(it = mObservers.find(parcel_id); it != end; ++it)
	{
		if (it->second.get() == observer)
		{
			mObservers.erase(it);
			break;
		}
	}
}

//static
void LLRemoteParcelInfoProcessor::processParcelInfoReply(LLMessageSystem* msg, void**)
{
	LLParcelData parcel_data;

	msg->getUUID	("Data", "ParcelID", parcel_data.parcel_id);
	msg->getUUID	("Data", "OwnerID", parcel_data.owner_id);
	msg->getString	("Data", "Name", parcel_data.name);
	msg->getString	("Data", "Desc", parcel_data.desc);
	msg->getS32		("Data", "ActualArea", parcel_data.actual_area);
	msg->getS32		("Data", "BillableArea", parcel_data.billable_area);
	msg->getU8		("Data", "Flags", parcel_data.flags);
	msg->getF32		("Data", "GlobalX", parcel_data.global_x);
	msg->getF32		("Data", "GlobalY", parcel_data.global_y);
	msg->getF32		("Data", "GlobalZ", parcel_data.global_z);
	msg->getString	("Data", "SimName", parcel_data.sim_name);
	msg->getUUID	("Data", "SnapshotID", parcel_data.snapshot_id);
	msg->getF32		("Data", "Dwell", parcel_data.dwell);
	msg->getS32		("Data", "SalePrice", parcel_data.sale_price);
	msg->getS32		("Data", "AuctionID", parcel_data.auction_id);

	LLRemoteParcelInfoProcessor::observer_multimap_t & observers = LLRemoteParcelInfoProcessor::getInstance()->mObservers;

	typedef std::vector<observer_multimap_t::iterator> deadlist_t;
	deadlist_t dead_iters;

	observer_multimap_t::iterator oi;
	observer_multimap_t::iterator end = observers.upper_bound(parcel_data.parcel_id);

	for (oi = observers.find(parcel_data.parcel_id); oi != end; ++oi)
	{
		LLRemoteParcelInfoObserver * observer = oi->second.get();
		if(observer)
		{
			observer->processParcelInfo(parcel_data);
		}
		else
		{
			// the handle points to an expired observer, so don't keep it
			// around anymore
			dead_iters.push_back(oi);
		}
	}

	deadlist_t::iterator i;
	deadlist_t::iterator end_dead = dead_iters.end();
	for(i = dead_iters.begin(); i != end_dead; ++i)
	{
		observers.erase(*i);
	}
}

void LLRemoteParcelInfoProcessor::sendParcelInfoRequest(const LLUUID& parcel_id)
{
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessage("ParcelInfoRequest");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->nextBlock("Data");
	msg->addUUID("ParcelID", parcel_id);
	gAgent.sendReliableMessage();
}
