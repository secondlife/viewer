/** 
 * @file llremoteparcelrequest.h
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

#ifndef LL_LLREMOTEPARCELREQUEST_H
#define LL_LLREMOTEPARCELREQUEST_H

#include "llhandle.h"
#include "llsingleton.h"
#include "llcoros.h"
#include "lleventcoro.h"

class LLMessageSystem;
class LLRemoteParcelInfoObserver;

struct LLParcelData
{
    LLUUID      parcel_id;
    LLUUID      owner_id;
    std::string name;
    std::string desc;
    S32         actual_area;
    S32         billable_area;
    U8          flags; // group owned, maturity
    F32         global_x;
    F32         global_y;
    F32         global_z;
    std::string sim_name;
    LLUUID      snapshot_id;
    F32         dwell;
    S32         sale_price;
    S32         auction_id;
};

// An interface class for panels which display parcel information
// like name, description, area, snapshot etc.
class LLRemoteParcelInfoObserver
{
public:
    LLRemoteParcelInfoObserver() { mObserverHandle.bind(this); }
    virtual ~LLRemoteParcelInfoObserver() {}
    virtual void processParcelInfo(const LLParcelData& parcel_data) = 0;
    virtual void setParcelID(const LLUUID& parcel_id) = 0;
    virtual void setErrorStatus(S32 status, const std::string& reason) = 0;
    LLHandle<LLRemoteParcelInfoObserver>    getObserverHandle() const { return mObserverHandle; }

protected:
    LLRootHandle<LLRemoteParcelInfoObserver> mObserverHandle;
};

class LLRemoteParcelInfoProcessor : public LLSingleton<LLRemoteParcelInfoProcessor>
{
    LLSINGLETON_EMPTY_CTOR(LLRemoteParcelInfoProcessor);
    virtual ~LLRemoteParcelInfoProcessor() {}

public:
    void addObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer);
    void removeObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer);

    void sendParcelInfoRequest(const LLUUID& parcel_id);

    static void processParcelInfoReply(LLMessageSystem* msg, void**);

    bool requestRegionParcelInfo(const std::string &url, const LLUUID &regionId, 
        const LLVector3 &regionPos, const LLVector3d& globalPos, LLHandle<LLRemoteParcelInfoObserver> observerHandle);

private:
    typedef std::multimap<LLUUID, LLHandle<LLRemoteParcelInfoObserver> > observer_multimap_t;
    observer_multimap_t mObservers;

    void regionParcelInfoCoro(std::string url, LLUUID regionId, LLVector3 posRegion, LLVector3d posGlobal, LLHandle<LLRemoteParcelInfoObserver> observerHandle);
};

#endif // LL_LLREMOTEPARCELREQUEST_H
