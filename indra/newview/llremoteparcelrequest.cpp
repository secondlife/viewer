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
#include "llsdserialize.h"
#include "llurlentry.h"
#include "llviewerregion.h"
#include "llview.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
#include "llregionhandle.h"
#include "llagent.h"
#include "llremoteparcelrequest.h"
#include "llcorehttputil.h"

void LLRemoteParcelInfoProcessor::addObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer)
{
    observer_multimap_t::iterator it;
    observer_multimap_t::iterator start = mObservers.lower_bound(parcel_id);
    observer_multimap_t::iterator end = mObservers.upper_bound(parcel_id);

    // Check if the observer is already in observers list for this UUID
    for(it = start; it != end; ++it)
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
    observer_multimap_t::iterator start = mObservers.lower_bound(parcel_id);
    observer_multimap_t::iterator end = mObservers.upper_bound(parcel_id);

    for(it = start; it != end; ++it)
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

    msg->getUUID    ("Data", "ParcelID", parcel_data.parcel_id);
    msg->getUUID    ("Data", "OwnerID", parcel_data.owner_id);
    msg->getString  ("Data", "Name", parcel_data.name);
    msg->getString  ("Data", "Desc", parcel_data.desc);
    msg->getS32     ("Data", "ActualArea", parcel_data.actual_area);
    msg->getS32     ("Data", "BillableArea", parcel_data.billable_area);
    msg->getU8      ("Data", "Flags", parcel_data.flags);
    msg->getF32     ("Data", "GlobalX", parcel_data.global_x);
    msg->getF32     ("Data", "GlobalY", parcel_data.global_y);
    msg->getF32     ("Data", "GlobalZ", parcel_data.global_z);
    msg->getString  ("Data", "SimName", parcel_data.sim_name);
    msg->getUUID    ("Data", "SnapshotID", parcel_data.snapshot_id);
    msg->getF32     ("Data", "Dwell", parcel_data.dwell);
    msg->getS32     ("Data", "SalePrice", parcel_data.sale_price);
    msg->getS32     ("Data", "AuctionID", parcel_data.auction_id);

    LLRemoteParcelInfoProcessor::observer_multimap_t & observers = LLRemoteParcelInfoProcessor::getInstance()->mObservers;

    typedef std::vector<observer_multimap_t::iterator> deadlist_t;
    deadlist_t dead_iters;

    observer_multimap_t::iterator oi = observers.lower_bound(parcel_data.parcel_id);
    observer_multimap_t::iterator end = observers.upper_bound(parcel_data.parcel_id);

    while (oi != end)
    {
        // increment the loop iterator now since it may become invalid below
        observer_multimap_t::iterator cur_oi = oi++;

        LLRemoteParcelInfoObserver * observer = cur_oi->second.get();
        if(observer)
        {
            // may invalidate cur_oi if the observer removes itself
            observer->processParcelInfo(parcel_data);
        }
        else
        {
            // the handle points to an expired observer, so don't keep it
            // around anymore
            dead_iters.push_back(cur_oi);
        }
    }

    deadlist_t::iterator i;
    deadlist_t::iterator end_dead = dead_iters.end();
    for(i = dead_iters.begin(); i != end_dead; ++i)
    {
        observers.erase(*i);
    }

    LLUrlEntryParcel::LLParcelData url_data;
    url_data.parcel_id = parcel_data.parcel_id;
    url_data.name = parcel_data.name;
    url_data.sim_name = parcel_data.sim_name;
    url_data.global_x = parcel_data.global_x;
    url_data.global_y = parcel_data.global_y;
    url_data.global_z = parcel_data.global_z;

    // Pass the parcel data to LLUrlEntryParcel to render
    // human readable parcel name.
    LLUrlEntryParcel::processParcelInfo(url_data);
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

bool LLRemoteParcelInfoProcessor::requestRegionParcelInfo(const std::string &url,
    const LLUUID &regionId, const LLVector3 &regionPos, const LLVector3d&globalPos,
    LLHandle<LLRemoteParcelInfoObserver> observerHandle)
{

    if (!url.empty())
    {
        LLCoros::instance().launch("LLRemoteParcelInfoProcessor::regionParcelInfoCoro",
            boost::bind(&LLRemoteParcelInfoProcessor::regionParcelInfoCoro, this, url,
            regionId, regionPos, globalPos, observerHandle));
        return true;
    }

    return false;
}

void LLRemoteParcelInfoProcessor::regionParcelInfoCoro(std::string url,
    LLUUID regionId, LLVector3 posRegion, LLVector3d posGlobal,
    LLHandle<LLRemoteParcelInfoObserver> observerHandle)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("RemoteParcelRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD bodyData;

    bodyData["location"] = ll_sd_from_vector3(posRegion);
    if (!regionId.isNull())
    {
        bodyData["region_id"] = regionId;
    }
    if (!posGlobal.isExactlyZero())
    {
        U64 regionHandle = to_region_handle(posGlobal);
        bodyData["region_handle"] = ll_sd_from_U64(regionHandle);
    }

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, bodyData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLRemoteParcelInfoObserver* observer = observerHandle.get();
    // Panel inspecting the information may be closed and destroyed
    // before this response is received.
    if (!observer)
        return;

    if (!status)
    {
        std::string message = status.getMessage();
        if (message.empty())
        {
            message = status.toString();
        }
        observer->setErrorStatus(status.getStatus(), message);
    }
    else
    {
        LLUUID parcel_id = result["parcel_id"];
        observer->setParcelID(parcel_id);
    }

}
