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
#include <vector>

#include <boost/bind.hpp>

#define CENTER_REGION 99

//---------------------------------------------------------------------------
// LLPathfindingNavMeshZone
//---------------------------------------------------------------------------

LLPathfindingNavMeshZone::LLPathfindingNavMeshZone()
	: mNavMeshLocationPtrs(),
	mNavMeshZoneSignal()
{
}

LLPathfindingNavMeshZone::~LLPathfindingNavMeshZone()
{
}

LLPathfindingNavMeshZone::navmesh_zone_slot_t LLPathfindingNavMeshZone::registerNavMeshZoneListener(navmesh_zone_callback_t pNavMeshZoneCallback)
{
	return mNavMeshZoneSignal.connect(pNavMeshZoneCallback);
}

void LLPathfindingNavMeshZone::initialize()
{
	llassert(LLPathingLib::getInstance() != NULL);
	if (LLPathingLib::getInstance() != NULL)
	{
		LLPathingLib::getInstance()->cleanupResidual();
	}
	mNavMeshLocationPtrs.clear();

	NavMeshLocationPtr centerNavMeshPtr(new NavMeshLocation(CENTER_REGION, boost::bind(&LLPathfindingNavMeshZone::handleNavMeshLocation, this)));
	mNavMeshLocationPtrs.push_back(centerNavMeshPtr);

	U32 neighborRegionDir = gSavedSettings.getU32("RetrieveNeighboringRegion");
	if (neighborRegionDir != CENTER_REGION)
	{
		NavMeshLocationPtr neighborNavMeshPtr(new NavMeshLocation(neighborRegionDir, boost::bind(&LLPathfindingNavMeshZone::handleNavMeshLocation, this)));
		mNavMeshLocationPtrs.push_back(neighborNavMeshPtr);
	}
}

void LLPathfindingNavMeshZone::enable()
{
	for (NavMeshLocationPtrs::iterator navMeshLocationPtrIter = mNavMeshLocationPtrs.begin();
		navMeshLocationPtrIter != mNavMeshLocationPtrs.end(); ++navMeshLocationPtrIter)
	{
		NavMeshLocationPtr navMeshLocationPtr = *navMeshLocationPtrIter;
		navMeshLocationPtr->enable();
	}
}

void LLPathfindingNavMeshZone::disable()
{
	for (NavMeshLocationPtrs::iterator navMeshLocationPtrIter = mNavMeshLocationPtrs.begin();
		navMeshLocationPtrIter != mNavMeshLocationPtrs.end(); ++navMeshLocationPtrIter)
	{
		NavMeshLocationPtr navMeshLocationPtr = *navMeshLocationPtrIter;
		navMeshLocationPtr->disable();
	}
#if 0
	llassert(LLPathingLib::getInstance() != NULL);
	if (LLPathingLib::getInstance() != NULL)
	{
		LLPathingLib::getInstance()->cleanupResidual();
	}
#endif
}

void LLPathfindingNavMeshZone::refresh()
{
	for (NavMeshLocationPtrs::iterator navMeshLocationPtrIter = mNavMeshLocationPtrs.begin();
		navMeshLocationPtrIter != mNavMeshLocationPtrs.end(); ++navMeshLocationPtrIter)
	{
		NavMeshLocationPtr navMeshLocationPtr = *navMeshLocationPtrIter;
		navMeshLocationPtr->refresh();
	}
}

void LLPathfindingNavMeshZone::handleNavMeshLocation()
{
	updateStatus();
}

void LLPathfindingNavMeshZone::updateStatus()
{
	bool hasRequestUnknown = false;
	bool hasRequestStarted = false;
	bool hasRequestCompleted = false;
	bool hasRequestNotEnabled = false;
	bool hasRequestError = false;

	for (NavMeshLocationPtrs::iterator navMeshLocationPtrIter = mNavMeshLocationPtrs.begin();
		navMeshLocationPtrIter != mNavMeshLocationPtrs.end(); ++navMeshLocationPtrIter)
	{
		NavMeshLocationPtr navMeshLocationPtr = *navMeshLocationPtrIter;
		switch (navMeshLocationPtr->getRequestStatus())
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
	if (hasRequestStarted)
	{
		zoneRequestStatus = kNavMeshZoneRequestStarted;
	}
	else if (hasRequestError)
	{
		zoneRequestStatus = kNavMeshZoneRequestError;
	}
	else if (hasRequestCompleted)
	{
		zoneRequestStatus = kNavMeshZoneRequestCompleted;
		llassert(LLPathingLib::getInstance() != NULL);
		if (LLPathingLib::getInstance() != NULL)
		{
			LLPathingLib::getInstance()->stitchNavMeshes( gSavedSettings.getBOOL("EnableVBOForNavMeshVisualization") );
		}
	}
	else if (hasRequestNotEnabled)
	{
		zoneRequestStatus = kNavMeshZoneRequestNotEnabled;
	}
	else if (hasRequestUnknown)
	{
		zoneRequestStatus = kNavMeshZoneRequestUnknown;
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

LLPathfindingNavMeshZone::NavMeshLocation::NavMeshLocation(S32 pDirection, navmesh_location_callback_t pLocationCallback)
	: mDirection(pDirection),
	mRegionUUID(),
	mHasNavMesh(false),
	mNavMeshVersion(0U),
	mLocationCallback(pLocationCallback),
	mRequestStatus(LLPathfindingNavMesh::kNavMeshRequestUnknown),
	mNavMeshSlot()
{
}

LLPathfindingNavMeshZone::NavMeshLocation::~NavMeshLocation()
{
}

void LLPathfindingNavMeshZone::NavMeshLocation::enable()
{
	clear();

	LLViewerRegion *region = getRegion();
	if (region == NULL)
	{
		mRegionUUID.setNull();
	}
	else
	{
		mRegionUUID = region->getRegionID();
		mNavMeshSlot = LLPathfindingManager::getInstance()->registerNavMeshListenerForRegion(region, boost::bind(&LLPathfindingNavMeshZone::NavMeshLocation::handleNavMesh, this, _1, _2, _3, _4));
	}
}

void LLPathfindingNavMeshZone::NavMeshLocation::refresh()
{
	LLViewerRegion *region = getRegion();

	if (region == NULL)
	{
		llassert(mRegionUUID.isNull());
		LLSD::Binary nullData;
		handleNavMesh(LLPathfindingNavMesh::kNavMeshRequestUnknown, mRegionUUID, 0U, nullData);
	}
	else
	{
		llassert(mRegionUUID == region->getRegionID());
		LLPathfindingManager::getInstance()->requestGetNavMeshForRegion(region);
	}
}

void LLPathfindingNavMeshZone::NavMeshLocation::disable()
{
	clear();
}

LLPathfindingNavMesh::ENavMeshRequestStatus LLPathfindingNavMeshZone::NavMeshLocation::getRequestStatus() const
{
	return mRequestStatus;
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
		llassert(LLPathingLib::getInstance() != NULL);
		if (LLPathingLib::getInstance() != NULL)
		{
			LLPathingLib::getInstance()->extractNavMeshSrcFromLLSD(pNavMeshData, mDirection);
		}
	}
	mLocationCallback();
}

void LLPathfindingNavMeshZone::NavMeshLocation::clear()
{
	mHasNavMesh = false;
	mRequestStatus = LLPathfindingNavMesh::kNavMeshRequestUnknown;
	if (mNavMeshSlot.connected())
	{
		mNavMeshSlot.disconnect();
	}
}

LLViewerRegion *LLPathfindingNavMeshZone::NavMeshLocation::getRegion() const
{
	LLViewerRegion *region = NULL;

	LLViewerRegion *currentRegion = gAgent.getRegion();
	if (currentRegion != NULL)
	{
		if (mDirection == CENTER_REGION)
		{
			region = currentRegion;
		}
		else
		{
			//User wants to pull in a neighboring region
			std::vector<S32> availableRegions;
			currentRegion->getNeighboringRegionsStatus( availableRegions );
			//Is the desired region in the available list
			std::vector<S32>::iterator foundElem = std::find(availableRegions.begin(),availableRegions.end(),mDirection); 
			if ( foundElem != availableRegions.end() )
			{
				std::vector<LLViewerRegion*> neighborRegionsPtrs;
				currentRegion->getNeighboringRegions( neighborRegionsPtrs );
				region = neighborRegionsPtrs[foundElem - availableRegions.begin()];
			}
		}
	}

	return region;
}
