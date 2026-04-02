/**
* @file   llpathfindingnavmesh.h
* @brief  Header file for llpathfindingnavmesh
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
#ifndef LL_LLPATHFINDINGNAVMESH_H
#define LL_LLPATHFINDINGNAVMESH_H

#include <functional>
#include <string>

#include <boost/signals2.hpp>

#include "llpathfindingnavmeshstatus.h"
#include "llsd.h"

class LLPathfindingNavMesh;
class LLUUID;

using LLPathfindingNavMeshPtr = std::shared_ptr<LLPathfindingNavMesh>;

class LLPathfindingNavMesh
{
public:
    enum ENavMeshRequestStatus {
        kNavMeshRequestUnknown,
        kNavMeshRequestWaiting,
        kNavMeshRequestChecking,
        kNavMeshRequestNeedsUpdate,
        kNavMeshRequestStarted,
        kNavMeshRequestCompleted,
        kNavMeshRequestNotEnabled,
        kNavMeshRequestError
    };

    using navmesh_callback_t = std::function<void(ENavMeshRequestStatus, const LLPathfindingNavMeshStatus&, const LLSD::Binary&)>;
    using navmesh_signal_t = boost::signals2::signal<void (ENavMeshRequestStatus, const LLPathfindingNavMeshStatus &, const LLSD::Binary &)>;
    using navmesh_slot_t = boost::signals2::connection;

    LLPathfindingNavMesh(const LLUUID &pRegionUUID);
    virtual ~LLPathfindingNavMesh();

    navmesh_slot_t registerNavMeshListener(navmesh_callback_t pNavMeshCallback);

    bool hasNavMeshVersion(const LLPathfindingNavMeshStatus &pNavMeshStatus) const;

    void handleNavMeshWaitForRegionLoad();
    void handleNavMeshCheckVersion();
    void handleRefresh(const LLPathfindingNavMeshStatus &pNavMeshStatus);
    void handleNavMeshNewVersion(const LLPathfindingNavMeshStatus &pNavMeshStatus);
    void handleNavMeshStart(const LLPathfindingNavMeshStatus &pNavMeshStatus);
    void handleNavMeshResult(const LLSD &pContent, U32 pNavMeshVersion);
    void handleNavMeshNotEnabled();
    void handleNavMeshError();
    void handleNavMeshError(U32 pNavMeshVersion);

protected:

private:
    void setRequestStatus(ENavMeshRequestStatus pNavMeshRequestStatus);
    void sendStatus();

    LLPathfindingNavMeshStatus mNavMeshStatus;
    ENavMeshRequestStatus      mNavMeshRequestStatus;
    navmesh_signal_t           mNavMeshSignal;
    LLSD::Binary               mNavMeshData;
};

#endif // LL_LLPATHFINDINGNAVMESH_H
