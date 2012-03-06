/** 
 * @file llpathfindingnavmeshzone.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llsd.h"
#include "lluuid.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshzone.h"
#include "llpathfindingmanager.h"
#include "llviewercontrol.h"

#include "LLPathingLib.h"

#include <string>
#include <map>

#define CENTER_REGION 99

//---------------------------------------------------------------------------
// LLPathfindingNavMeshZone
//---------------------------------------------------------------------------

LLPathfindingNavMeshZone::LLPathfindingNavMeshZone()
	: mNavMeshLocations(),
	mNavMeshZoneSignal(),
	mNavMeshSlot()
{
}

LLPathfindingNavMeshZone::~LLPathfindingNavMeshZone()
{
}

LLPathfindingNavMeshZone::navmesh_zone_slot_t LLPathfindingNavMeshZone::registerNavMeshZoneListener(navmesh_zone_callback_t pNavMeshZoneCallback)
{
	return mNavMeshZoneSignal.connect(pNavMeshZoneCallback);
}

void LLPathfindingNavMeshZone::setCurrentRegionAsCenter()
{
	llassert(LLPathingLib::getInstance() != NULL);
	LLPathingLib::getInstance()->cleanupResidual();
	mNavMeshLocations.clear();
	LLViewerRegion *currentRegion = gAgent.getRegion();
	const LLUUID &currentRegionUUID = currentRegion->getRegionID();
	NavMeshLocation centerNavMesh(currentRegionUUID, CENTER_REGION);
	mNavMeshLocations.insert(std::pair<LLUUID, NavMeshLocation>(currentRegionUUID, centerNavMesh));
}

void LLPathfindingNavMeshZone::refresh()
{
	LLPathfindingManager *pathfindingManagerInstance = LLPathfindingManager::getInstance();
	if (!mNavMeshSlot.connected())
	{
		pathfindingManagerInstance->registerNavMeshListenerForCurrentRegion(boost::bind(&LLPathfindingNavMeshZone::handleNavMesh, this, _1, _2, _3, _4));
	}

	pathfindingManagerInstance->requestGetNavMeshForCurrentRegion();
}

void LLPathfindingNavMeshZone::disable()
{
	if (mNavMeshSlot.connected())
	{
		mNavMeshSlot.disconnect();
	}

	llassert(LLPathingLib::getInstance() != NULL);
	LLPathingLib::getInstance()->cleanupResidual();

	mNavMeshLocations.clear();
}

void LLPathfindingNavMeshZone::handleNavMesh(LLPathfindingNavMesh::ENavMeshRequestStatus pNavMeshRequestStatus, const LLUUID &pRegionUUID, U32 pNavMeshVersion, const LLSD::Binary &pNavMeshData)
{
	NavMeshLocations::iterator navMeshIter = mNavMeshLocations.find(pRegionUUID);
	if (navMeshIter != mNavMeshLocations.end())
	{
		navMeshIter->second.handleNavMesh(pNavMeshRequestStatus, pRegionUUID, pNavMeshVersion, pNavMeshData);
		updateStatus();
	}
}

void LLPathfindingNavMeshZone::updateStatus()
{
	bool hasRequestUnknown = false;
	bool hasRequestStarted = false;
	bool hasRequestCompleted = false;
	bool hasRequestNotEnabled = false;
	bool hasRequestError = false;

	for (NavMeshLocations::iterator navMeshIter = mNavMeshLocations.begin();
		navMeshIter != mNavMeshLocations.end(); ++navMeshIter)
	{
		switch (navMeshIter->second.getRequestStatus())
		{
		case LLPathfindingNavMesh::kNavMeshRequestUnknown :
			hasRequestUnknown = true;
			break;
		case LLPathfindingNavMesh::kNavMeshRequestStarted :
			hasRequestStarted = true;
			break;
		case LLPathfindingNavMesh::kNavMeshRequestCompleted :
			hasRequestCompleted = true;
			break;
		case LLPathfindingNavMesh::kNavMeshRequestNotEnabled :
			hasRequestNotEnabled = true;
			break;
		case LLPathfindingNavMesh::kNavMeshRequestError :
			hasRequestError = true;
			break;
		default :
			hasRequestError = true;
			llassert(0);
			break;
		}
	}

	ENavMeshZoneRequestStatus zoneRequestStatus = kNavMeshZoneRequestUnknown;
	if (hasRequestNotEnabled)
	{
		zoneRequestStatus = kNavMeshZoneRequestNotEnabled;
	}
	else if (hasRequestError)
	{
		zoneRequestStatus = kNavMeshZoneRequestError;
	}
	else if (hasRequestStarted)
	{
		zoneRequestStatus = kNavMeshZoneRequestStarted;
	}
	else if (hasRequestUnknown)
	{
		zoneRequestStatus = kNavMeshZoneRequestUnknown;
		llassert(0);
	}
	else if (hasRequestCompleted)
	{
		zoneRequestStatus = kNavMeshZoneRequestCompleted;
		LLPathingLib::getInstance()->stitchNavMeshes( gSavedSettings.getBOOL("EnableVBOForNavMeshVisualization") );
	}
	else
	{
		zoneRequestStatus = kNavMeshZoneRequestError;
		llassert(0);
	}

	mNavMeshZoneSignal(zoneRequestStatus);
}

//---------------------------------------------------------------------------
// LLPathfindingNavMeshZone::NavMeshLocation
//---------------------------------------------------------------------------

LLPathfindingNavMeshZone::NavMeshLocation::NavMeshLocation(const LLUUID &pRegionUUID, S32 pDirection)
	: mRegionUUID(pRegionUUID),
	mDirection(pDirection),
	mHasNavMesh(false),
	mNavMeshVersion(0U),
	mRequestStatus(LLPathfindingNavMesh::kNavMeshRequestUnknown)
{
}

LLPathfindingNavMeshZone::NavMeshLocation::NavMeshLocation(const NavMeshLocation &other)
	: mRegionUUID(other.mRegionUUID),
	mDirection(other.mDirection),
	mHasNavMesh(other.mHasNavMesh),
	mNavMeshVersion(other.mNavMeshVersion),
	mRequestStatus(other.mRequestStatus)
{
}

LLPathfindingNavMeshZone::NavMeshLocation::~NavMeshLocation()
{
}

void LLPathfindingNavMeshZone::NavMeshLocation::handleNavMesh(LLPathfindingNavMesh::ENavMeshRequestStatus pNavMeshRequestStatus, const LLUUID &pRegionUUID, U32 pNavMeshVersion, const LLSD::Binary &pNavMeshData)
{
	llassert(mRegionUUID == pRegionUUID);
	mRequestStatus = pNavMeshRequestStatus;
	if ((pNavMeshRequestStatus == LLPathfindingNavMesh::kNavMeshRequestCompleted) && (!mHasNavMesh || (mNavMeshVersion != pNavMeshVersion)))
	{
		llassert(!pNavMeshData.empty());
		mHasNavMesh = true;
		mNavMeshVersion = pNavMeshVersion;
		LLPathingLib::getInstance()->extractNavMeshSrcFromLLSD(pNavMeshData, mDirection);
	}
}

LLPathfindingNavMesh::ENavMeshRequestStatus LLPathfindingNavMeshZone::NavMeshLocation::getRequestStatus() const
{
	return mRequestStatus;
}

LLPathfindingNavMeshZone::NavMeshLocation &LLPathfindingNavMeshZone::NavMeshLocation::operator =(const NavMeshLocation &other)
{
	mRegionUUID = other.mRegionUUID;
	mDirection = other.mDirection;
	mHasNavMesh = other.mHasNavMesh;
	mNavMeshVersion = other.mNavMeshVersion;
	mRequestStatus = other.mRequestStatus;

	return (*this);
}
