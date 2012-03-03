/** 
 * @file llpathfindingnavmesh.h
 * @author William Todd Stinson
 * @brief A class for representing the navmesh of a pathfinding region.
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

#ifndef LL_LLPATHFINDINGNAVMESH_H
#define LL_LLPATHFINDINGNAVMESH_H

#include "llsd.h"
#include "lluuid.h"

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

class LLSD;
class LLPathfindingNavMesh;

typedef boost::shared_ptr<LLPathfindingNavMesh> LLPathfindingNavMeshPtr;

class LLPathfindingNavMesh
{
public:
	typedef enum {
		kNavMeshRequestUnknown,
		kNavMeshRequestStarted,
		kNavMeshRequestCompleted,
		kNavMeshRequestNotEnabled,
		kNavMeshRequestMessageError,
		kNavMeshRequestFormatError
	} ENavMeshRequestStatus;

	typedef boost::function<void (ENavMeshRequestStatus, const LLUUID &, U32, const LLSD::Binary &)>         navmesh_callback_t;
	typedef boost::signals2::signal<void (ENavMeshRequestStatus, const LLUUID &, U32, const LLSD::Binary &)> navmesh_signal_t;
	typedef boost::signals2::connection                                                                      navmesh_slot_t;

	LLPathfindingNavMesh(const LLUUID &pRegionUUID);
	virtual ~LLPathfindingNavMesh();

	navmesh_slot_t registerNavMeshListener(navmesh_callback_t pNavMeshCallback);

	bool hasNavMeshVersion(U32 pNavMeshVersion) const;

	void handleRefresh();
	void handleNavMeshStart(U32 pNavMeshVersion);
	void handleNavMeshResult(const LLSD &pContent, U32 pNavMeshVersion);
	void handleNavMeshNotEnabled();
	void handleNavMeshError(U32 pStatus, const std::string &pReason, const std::string &pURL, U32 pNavMeshVersion);

protected:

private:
	void setRequestStatus(ENavMeshRequestStatus pNavMeshRequestStatus);

	LLUUID                mRegionUUID;
	ENavMeshRequestStatus mNavMeshRequestStatus;
	navmesh_signal_t      mNavMeshSignal;
	LLSD::Binary          mNavMeshData;
	U32                   mNavMeshVersion;
};

#endif // LL_LLPATHFINDINGNAVMESH_H
