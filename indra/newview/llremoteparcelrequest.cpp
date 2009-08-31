/** 
 * @file llremoteparcelrequest.cpp
 * @author Sam Kolb
 * @brief Get information about a parcel you aren't standing in to display
 * landmark/teleport information.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "message.h"

#include "llpanelplace.h"
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
	// Check if the observer is alredy in observsrs list for this UUID
	observer_multimap_t::iterator it;

	it = mObservers.find(parcel_id);
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			return;
		}
		else
		{
			++it;
		}
	}

	mObservers.insert(std::pair<LLUUID, LLRemoteParcelInfoObserver*>(parcel_id, observer));
}

void LLRemoteParcelInfoProcessor::removeObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer)
{
	if (!observer)
	{
		return;
	}

	observer_multimap_t::iterator it;

	it = mObservers.find(parcel_id);
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			mObservers.erase(it);
			break;
		}
		else
		{
			++it;
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

	LLRemoteParcelInfoProcessor::observer_multimap_t observers = LLRemoteParcelInfoProcessor::getInstance()->mObservers;

	observer_multimap_t::iterator oi = observers.find(parcel_data.parcel_id);
	observer_multimap_t::iterator end = observers.upper_bound(parcel_data.parcel_id);
	for (; oi != end; ++oi)
	{
		oi->second->processParcelInfo(parcel_data);
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(parcel_data.parcel_id, oi->second);
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
