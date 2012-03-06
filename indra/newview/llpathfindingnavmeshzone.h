/** 
 * @file llpathfindingnavmeshzone.h
 * @author William Todd Stinson
 * @brief A class for representing the zone of navmeshes containing and possible surrounding the current region.
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

#ifndef LL_LLPATHFINDINGNAVMESHZONE_H
#define LL_LLPATHFINDINGNAVMESHZONE_H

#include "llsd.h"
#include "lluuid.h"
#include "llpathfindingnavmesh.h"

#include <map>

#include <boost/function.hpp>
#include <boost/signals2.hpp>

class LLPathfindingNavMeshZone
{
public:
	typedef enum {
		kNavMeshZoneRequestUnknown,
		kNavMeshZoneRequestStarted,
		kNavMeshZoneRequestCompleted,
		kNavMeshZoneRequestNotEnabled,
		kNavMeshZoneRequestError
	} ENavMeshZoneRequestStatus;

	typedef boost::function<void (ENavMeshZoneRequestStatus)>         navmesh_zone_callback_t;
	typedef boost::signals2::signal<void (ENavMeshZoneRequestStatus)> navmesh_zone_signal_t;
	typedef boost::signals2::connection                               navmesh_zone_slot_t;

	LLPathfindingNavMeshZone();
	virtual ~LLPathfindingNavMeshZone();

	navmesh_zone_slot_t registerNavMeshZoneListener(navmesh_zone_callback_t pNavMeshZoneCallback);
	void setCurrentRegionAsCenter();
	void refresh();
	void disable();

	void handleNavMesh(LLPathfindingNavMesh::ENavMeshRequestStatus pNavMeshRequestStatus, const LLUUID &pRegionUUID, U32 pNavMeshVersion, const LLSD::Binary &pNavMeshData);

protected:

private:
	class NavMeshLocation
	{
	public:
		NavMeshLocation(const LLUUID &pRegionUUID, S32 pDirection);
		NavMeshLocation(const NavMeshLocation &other);
		virtual ~NavMeshLocation();

		void handleNavMesh(LLPathfindingNavMesh::ENavMeshRequestStatus pNavMeshRequestStatus, const LLUUID &pRegionUUID, U32 pNavMeshVersion, const LLSD::Binary &pNavMeshData);
		LLPathfindingNavMesh::ENavMeshRequestStatus getRequestStatus() const;

		NavMeshLocation &operator =(const NavMeshLocation &other);

	protected:

	private:
		LLUUID                                      mRegionUUID;
		S32                                         mDirection;
		bool                                        mHasNavMesh;
		U32                                         mNavMeshVersion;
		LLPathfindingNavMesh::ENavMeshRequestStatus mRequestStatus;
	};

	typedef std::map<LLUUID, NavMeshLocation> NavMeshLocations;

	void updateStatus();

	NavMeshLocations                     mNavMeshLocations;
	navmesh_zone_signal_t                mNavMeshZoneSignal;
	LLPathfindingNavMesh::navmesh_slot_t mNavMeshSlot;
};

#endif // LL_LLPATHFINDINGNAVMESHZONE_H
