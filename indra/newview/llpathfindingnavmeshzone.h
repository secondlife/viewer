/**
* @file   llpathfindingnavmeshzone.h
* @brief  Header file for llpathfindingnavmeshzone
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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
#ifndef LL_LLPATHFINDINGNAVMESHZONE_H
#define LL_LLPATHFINDINGNAVMESHZONE_H

#include <functional>
#include <vector>

#include <boost/signals2.hpp>

#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "llsd.h"
#include "lluuid.h"

class LLViewerRegion;

class LLPathfindingNavMeshZone
{
public:
    enum ENavMeshZoneRequestStatus {
        kNavMeshZoneRequestUnknown,
        kNavMeshZoneRequestWaiting,
        kNavMeshZoneRequestChecking,
        kNavMeshZoneRequestNeedsUpdate,
        kNavMeshZoneRequestStarted,
        kNavMeshZoneRequestCompleted,
        kNavMeshZoneRequestNotEnabled,
        kNavMeshZoneRequestError
    };

    enum ENavMeshZoneStatus {
        kNavMeshZonePending,
        kNavMeshZoneBuilding,
        kNavMeshZoneSomePending,
        kNavMeshZoneSomeBuilding,
        kNavMeshZonePendingAndBuilding,
        kNavMeshZoneComplete
    };

    using navmesh_zone_callback_t = std::function<void(ENavMeshZoneRequestStatus)>;
    using navmesh_zone_signal_t = boost::signals2::signal<void (ENavMeshZoneRequestStatus)>;
    using navmesh_zone_slot_t = boost::signals2::connection;

    LLPathfindingNavMeshZone();
    virtual ~LLPathfindingNavMeshZone();

    navmesh_zone_slot_t registerNavMeshZoneListener(navmesh_zone_callback_t pNavMeshZoneCallback);
    void initialize();

    void enable();
    void disable();
    void refresh();

    ENavMeshZoneStatus getNavMeshZoneStatus() const;

protected:

private:
    using navmesh_location_callback_t = std::function<void(void)>;
    class NavMeshLocation
    {
    public:
        NavMeshLocation(S32 pDirection, navmesh_location_callback_t pLocationCallback);
        virtual ~NavMeshLocation();

        void enable();
        void refresh();
        void disable();

        LLPathfindingNavMesh::ENavMeshRequestStatus getRequestStatus() const;
        LLPathfindingNavMeshStatus::ENavMeshStatus  getNavMeshStatus() const;

    protected:

    private:
        void           handleNavMesh(LLPathfindingNavMesh::ENavMeshRequestStatus pNavMeshRequestStatus, const LLPathfindingNavMeshStatus &pNavMeshStatus, const LLSD::Binary &pNavMeshData);

        void           clear();
        LLViewerRegion *getRegion() const;

        S32                                         mDirection;
        LLUUID                                      mRegionUUID;
        bool                                        mHasNavMesh;
        U32                                         mNavMeshVersion;
        LLPathfindingNavMeshStatus::ENavMeshStatus  mNavMeshStatus;
        navmesh_location_callback_t                 mLocationCallback;
        LLPathfindingNavMesh::ENavMeshRequestStatus mRequestStatus;
        LLPathfindingNavMesh::navmesh_slot_t        mNavMeshSlot;
    };

    using NavMeshLocationPtr = std::shared_ptr<NavMeshLocation>;
    using NavMeshLocationPtrs = std::vector<NavMeshLocationPtr>;

    void handleNavMeshLocation();
    void updateStatus();

    NavMeshLocationPtrs       mNavMeshLocationPtrs;
    ENavMeshZoneRequestStatus mNavMeshZoneRequestStatus;
    navmesh_zone_signal_t     mNavMeshZoneSignal;
};

#endif // LL_LLPATHFINDINGNAVMESHZONE_H
