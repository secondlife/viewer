/** 
* @file llpathfindingnavmeshzone.cpp
* @brief Implementation of llpathfindingnavmeshzone
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


#include "llviewerprecompiledheaders.h"

#include "llpathfindingnavmeshzone.h"

#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "llpathinglib.h"
#include "llsd.h"
#include "lluuid.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"

#define CENTER_REGION 99

//---------------------------------------------------------------------------
// LLPathfindingNavMeshZone
//---------------------------------------------------------------------------

LLPathfindingNavMeshZone::LLPathfindingNavMeshZone()
	: mNavMeshLocationPtrs(),
	mNavMeshZoneRequestStatus(kNavMeshZoneRequestUnknown),
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
	mNavMeshLocationPtrs.clear();

	NavMeshLocationPtr centerNavMeshPtr(new NavMeshLocation(CENTER_REGION, boost::bind(&LLPathfindingNavMeshZone::handleNavMeshLocation, this)));
	mNavMeshLocationPtrs.push_back(centerNavMeshPtr);

	U32 neighborRegionDir = gSavedSettings.getU32("PathfindingRetrieveNeighboringRegion");
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
}

void LLPathfindingNavMeshZone::refresh()
{
	if (LLPathingLib::getInstance() != NULL)
	{
		LLPathingLib::getInstance()->cleanupResidual();
	}

	for (NavMeshLocationPtrs::iterator navMeshLocationPtrIter = mNavMeshLocationPtrs.begin();
		navMeshLocationPtrIter != mNavMeshLocationPtrs.end(); ++navMeshLocationPtrIter)
	{
		NavMeshLocationPtr navMeshLocationPtr = *navMeshLocationPtrIter;
		navMeshLocationPtr->refresh();
	}
}

LLPathfindingNavMeshZone::ENavMeshZoneStatus LLPathfindingNavMeshZone::getNavMeshZoneStatus() const
{
	bool hasPending = false;
	bool hasBuilding = false;
	bool hasComplete = false;
	bool hasRepending = false;

	for (NavMeshLocationPtrs::const_iterator navMeshLocationPtrIter = mNavMeshLocationPtrs.begin();
		navMeshLocationPtrIter != mNavMeshLocationPtrs.end(); ++navMeshLocationPtrIter)
	{
		const NavMeshLocationPtr navMeshLocationPtr = *navMeshLocationPtrIter;

		switch (navMeshLocationPtr->getNavMeshStatus())
		{
		case LLPathfindingNavMeshStatus::kPending :
			hasPending = true;
			break;
		case LLPathfindingNavMeshStatus::kBuilding :
			hasBuilding = true;
			break;
		case LLPathfindingNavMeshStatus::kComplete :
			hasComplete = true;
			break;
		case LLPathfindingNavMeshStatus::kRepending :
			hasRepending = true;
			break;
		default :
			hasPending = true;
			llassert(0);
			break;
		}
	}

	ENavMeshZoneStatus zoneStatus = kNavMeshZoneComplete;
	if (hasRepending || (hasPending && hasBuilding))
	{
		zoneStatus = kNavMeshZonePendingAndBuilding;
	}
	else if (hasComplete)
	{
		if (hasPending)
		{
			zoneStatus = kNavMeshZoneSomePending;
		}
		else if (hasBuilding)
		{
			zoneStatus = kNavMeshZoneSomeBuilding;
		}
		else
		{
			zoneStatus = kNavMeshZoneComplete;
		}
	}
	else if (hasPending)
	{
		zoneStatus = kNavMeshZonePending;
	}
	else if (hasBuilding)
	{
		zoneStatus = kNavMeshZoneBuilding;
	}

	return zoneStatus;
}

void LLPathfindingNavMeshZone::handleNavMeshLocation()
{
	updateStatus();
}

void LLPathfindingNavMeshZone::updateStatus()
{
	bool hasRequestUnknown = false;
	bool hasRequestWaiting = false;
	bool hasRequestChecking = false;
	bool hasRequestNeedsUpdate = false;
	bool hasRequestStarted = false;
	bool hasRequestCompleted = false;
	bool hasRequestNotEnabled = false;
	bool hasRequestError = false;

	for (NavMeshLocationPtrs::const_iterator navMeshLocationPtrIter = mNavMeshLocationPtrs.begin();
		navMeshLocationPtrIter != mNavMeshLocationPtrs.end(); ++navMeshLocationPtrIter)
	{
		const NavMeshLocationPtr navMeshLocationPtr = *navMeshLocationPtrIter;
		switch (navMeshLocationPtr->getRequestStatus())
		{
		case LLPathfindingNavMesh::kNavMeshRequestUnknown :
			hasRequestUnknown = true;
			break;
		case LLPathfindingNavMesh::kNavMeshRequestWaiting :
			hasRequestWaiting = true;
			break;
		case LLPathfindingNavMesh::kNavMeshRequestChecking :
			hasRequestChecking = true;
			break;
		case LLPathfindingNavMesh::kNavMeshRequestNeedsUpdate :
			hasRequestNeedsUpdate = true;
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
	if (hasRequestWaiting)
	{
		zoneRequestStatus = kNavMeshZoneRequestWaiting;
	}
	else if (hasRequestNeedsUpdate)
	{
		zoneRequestStatus = kNavMeshZoneRequestNeedsUpdate;
	}
	else if (hasRequestChecking)
	{
		zoneRequestStatus = kNavMeshZoneRequestChecking;
	}
	else if (hasRequestStarted)
	{
		zoneRequestStatus = kNavMeshZoneRequestStarted;
	}
	else if (hasRequestError)
	{
		zoneRequestStatus = kNavMeshZoneRequestError;
	}
	else if (hasRequestUnknown)
	{
		zoneRequestStatus = kNavMeshZoneRequestUnknown;
	}
	else if (hasRequestCompleted)
	{
		zoneRequestStatus = kNavMeshZoneRequestCompleted;
	}
	else if (hasRequestNotEnabled)
	{
		zoneRequestStatus = kNavMeshZoneRequestNotEnabled;
	}
	else
	{
		zoneRequestStatus = kNavMeshZoneRequestError;
		llassert(0);
	}

	if ((mNavMeshZoneRequestStatus != kNavMeshZoneRequestCompleted) &&
		(zoneRequestStatus == kNavMeshZoneRequestCompleted))
	{
		llassert(LLPathingLib::getInstance() != NULL);
		if (LLPathingLib::getInstance() != NULL)
		{
			LLPathingLib::getInstance()->processNavMeshData();
		}
	}

	mNavMeshZoneRequestStatus = zoneRequestStatus;
	mNavMeshZoneSignal(mNavMeshZoneRequestStatus);
}

//---------------------------------------------------------------------------
// LLPathfindingNavMeshZone::NavMeshLocation
//---------------------------------------------------------------------------

LLPathfindingNavMeshZone::NavMeshLocation::NavMeshLocation(S32 pDirection, navmesh_location_callback_t pLocationCallback)
	: mDirection(pDirection),
	mRegionUUID(),
	mHasNavMesh(false),
	mNavMeshVersion(0U),
	mNavMeshStatus(LLPathfindingNavMeshStatus::kComplete),
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
		mNavMeshSlot = LLPathfindingManager::getInstance()->registerNavMeshListenerForRegion(region, boost::bind(&LLPathfindingNavMeshZone::NavMeshLocation::handleNavMesh, this, _1, _2, _3));
	}
}

void LLPathfindingNavMeshZone::NavMeshLocation::refresh()
{
	LLViewerRegion *region = getRegion();

	if (region == NULL)
	{
		llassert(mRegionUUID.isNull());
		LLPathfindingNavMeshStatus newNavMeshStatus(mRegionUUID);
		LLSD::Binary nullData;
		handleNavMesh(LLPathfindingNavMesh::kNavMeshRequestNotEnabled, newNavMeshStatus, nullData);
	}
	else
	{
		llassert(mRegionUUID == region->getRegionID());
		LLPathfindingManager::getInstance()->requestGetNavMeshForRegion(region, false);
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

LLPathfindingNavMeshStatus::ENavMeshStatus LLPathfindingNavMeshZone::NavMeshLocation::getNavMeshStatus() const
{
	return mNavMeshStatus;
}

void LLPathfindingNavMeshZone::NavMeshLocation::handleNavMesh(LLPathfindingNavMesh::ENavMeshRequestStatus pNavMeshRequestStatus, const LLPathfindingNavMeshStatus &pNavMeshStatus, const LLSD::Binary &pNavMeshData)
{
	llassert(mRegionUUID == pNavMeshStatus.getRegionUUID());

	if ((pNavMeshRequestStatus == LLPathfindingNavMesh::kNavMeshRequestCompleted) &&
		(!mHasNavMesh || (mNavMeshVersion != pNavMeshStatus.getVersion())))
	{
		llassert(!pNavMeshData.empty());
		mHasNavMesh = true;
		mNavMeshVersion = pNavMeshStatus.getVersion();
		llassert(LLPathingLib::getInstance() != NULL);
		if (LLPathingLib::getInstance() != NULL)
		{
			LLPathingLib::getInstance()->extractNavMeshSrcFromLLSD(pNavMeshData, mDirection);
		}
	}

	mRequestStatus = pNavMeshRequestStatus;
	mNavMeshStatus = pNavMeshStatus.getStatus();
	mLocationCallback();
}

void LLPathfindingNavMeshZone::NavMeshLocation::clear()
{
	mHasNavMesh = false;
	mRequestStatus = LLPathfindingNavMesh::kNavMeshRequestUnknown;
	mNavMeshStatus = LLPathfindingNavMeshStatus::kComplete;
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
