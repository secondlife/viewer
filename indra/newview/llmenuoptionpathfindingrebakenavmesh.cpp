/** 
* @file llmenuoptionpathfindingrebakenavmesh.cpp
* @brief Implementation of llmenuoptionpathfindingrebakenavmesh
* @author Prep@lindenlab.com
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

#include "llmenuoptionpathfindingrebakenavmesh.h"

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llenvmanager.h"
#include "llnotificationsutil.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "llviewerregion.h"

LLMenuOptionPathfindingRebakeNavmesh::LLMenuOptionPathfindingRebakeNavmesh() 
	: LLSingleton<LLMenuOptionPathfindingRebakeNavmesh>(),
	mIsInitialized(false),
	mCanRebakeRegion(false),
	mRebakeNavMeshMode(kRebakeNavMesh_Default),
	mNavMeshSlot(),
	mRegionCrossingSlot(),
	mAgentStateSlot()
{
}

LLMenuOptionPathfindingRebakeNavmesh::~LLMenuOptionPathfindingRebakeNavmesh() 
{
	if (mRebakeNavMeshMode == kRebakeNavMesh_RequestSent)
	{
		LL_WARNS("navmeshRebaking") << "During destruction of the LLMenuOptionPathfindingRebakeNavmesh "
			<< "singleton, the mode indicates that a request has been sent for which a response has yet "
			<< "to be received.  This could contribute to a crash on exit." << LL_ENDL;
	}

	llassert(!mIsInitialized);
	if (mIsInitialized)
	{
		quit();
	}
}

void LLMenuOptionPathfindingRebakeNavmesh::initialize()
{
	llassert(!mIsInitialized);
	if (!mIsInitialized)
	{
		mIsInitialized = true;

		setMode(kRebakeNavMesh_Default);

		createNavMeshStatusListenerForCurrentRegion();

		if ( !mRegionCrossingSlot.connected() )
		{
			mRegionCrossingSlot = LLEnvManagerNew::getInstance()->setRegionChangeCallback(boost::bind(&LLMenuOptionPathfindingRebakeNavmesh::handleRegionBoundaryCrossed, this));
		}

		if (!mAgentStateSlot.connected())
		{
			mAgentStateSlot = LLPathfindingManager::getInstance()->registerAgentStateListener(boost::bind(&LLMenuOptionPathfindingRebakeNavmesh::handleAgentState, this, _1));
		}
		LLPathfindingManager::getInstance()->requestGetAgentState();
	}
}

void LLMenuOptionPathfindingRebakeNavmesh::quit()
{
	llassert(mIsInitialized);
	if (mIsInitialized)
	{
		if (mNavMeshSlot.connected())
		{
			mNavMeshSlot.disconnect();
		}

		if (mRegionCrossingSlot.connected())
		{
			mRegionCrossingSlot.disconnect();
		}

		if (mAgentStateSlot.connected())
		{
			mAgentStateSlot.disconnect();
		}

		mIsInitialized = false;
	}
}

bool LLMenuOptionPathfindingRebakeNavmesh::canRebakeRegion() const
{
	if (!mIsInitialized)
	{
		LL_ERRS("navmeshRebaking") << "LLMenuOptionPathfindingRebakeNavmesh class has not been initialized "
			<< "when the ability to rebake navmesh is being requested." << LL_ENDL;
	}
	return mCanRebakeRegion;
}

LLMenuOptionPathfindingRebakeNavmesh::ERebakeNavMeshMode LLMenuOptionPathfindingRebakeNavmesh::getMode() const
{
	if (!mIsInitialized)
	{
		LL_ERRS("navmeshRebaking") << "LLMenuOptionPathfindingRebakeNavmesh class has not been initialized "
			<< "when the mode is being requested." << LL_ENDL;
	}
	return mRebakeNavMeshMode;
}

void LLMenuOptionPathfindingRebakeNavmesh::sendRequestRebakeNavmesh()
{
	if (!mIsInitialized)
	{
		LL_ERRS("navmeshRebaking") << "LLMenuOptionPathfindingRebakeNavmesh class has not been initialized "
			<< "when the request is being made to rebake the navmesh." << LL_ENDL;
	}
	else
	{
		if (!canRebakeRegion())
		{
			LL_WARNS("navmeshRebaking") << "attempting to rebake navmesh when user does not have permissions "
				<< "on this region" << LL_ENDL;
		}
		if (getMode() != kRebakeNavMesh_Available)
		{
			LL_WARNS("navmeshRebaking") << "attempting to rebake navmesh when mode is not available"
				<< LL_ENDL;
		}

		setMode(kRebakeNavMesh_RequestSent);
		LLPathfindingManager::getInstance()->requestRebakeNavMesh(boost::bind(&LLMenuOptionPathfindingRebakeNavmesh::handleRebakeNavMeshResponse, this, _1));
	}
}

void LLMenuOptionPathfindingRebakeNavmesh::setMode(ERebakeNavMeshMode pRebakeNavMeshMode)
{
	mRebakeNavMeshMode = pRebakeNavMeshMode;
}

void LLMenuOptionPathfindingRebakeNavmesh::handleAgentState(BOOL pCanRebakeRegion)
{
	llassert(mIsInitialized);
	mCanRebakeRegion = pCanRebakeRegion;
}

void LLMenuOptionPathfindingRebakeNavmesh::handleRebakeNavMeshResponse(bool pResponseStatus)
{
	llassert(mIsInitialized);
	if (getMode() == kRebakeNavMesh_RequestSent)
	{
		setMode(pResponseStatus ? kRebakeNavMesh_InProgress : kRebakeNavMesh_Default);
	}

	if (!pResponseStatus)
	{
		LLNotificationsUtil::add("PathfindingCannotRebakeNavmesh");
	}
}

void LLMenuOptionPathfindingRebakeNavmesh::handleNavMeshStatus(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	llassert(mIsInitialized);
	ERebakeNavMeshMode rebakeNavMeshMode = kRebakeNavMesh_Default;
	if (pNavMeshStatus.isValid())
	{
		switch (pNavMeshStatus.getStatus())
		{
		case LLPathfindingNavMeshStatus::kPending :
		case LLPathfindingNavMeshStatus::kRepending :
			rebakeNavMeshMode = kRebakeNavMesh_Available;
			break;
		case LLPathfindingNavMeshStatus::kBuilding :
			rebakeNavMeshMode = kRebakeNavMesh_InProgress;
			break;
		case LLPathfindingNavMeshStatus::kComplete :
			rebakeNavMeshMode = kRebakeNavMesh_NotAvailable;
			break;
		default : 
			rebakeNavMeshMode = kRebakeNavMesh_Default;
			llassert(0);
			break;
		}
	}

	setMode(rebakeNavMeshMode);
}

void LLMenuOptionPathfindingRebakeNavmesh::handleRegionBoundaryCrossed()
{
	llassert(mIsInitialized);
	createNavMeshStatusListenerForCurrentRegion();
	mCanRebakeRegion = FALSE;
	LLPathfindingManager::getInstance()->requestGetAgentState();
}

void LLMenuOptionPathfindingRebakeNavmesh::createNavMeshStatusListenerForCurrentRegion()
{
	if (mNavMeshSlot.connected())
	{
		mNavMeshSlot.disconnect();
	}

	LLViewerRegion *currentRegion = gAgent.getRegion();
	if (currentRegion != NULL)
	{
		mNavMeshSlot = LLPathfindingManager::getInstance()->registerNavMeshListenerForRegion(currentRegion, boost::bind(&LLMenuOptionPathfindingRebakeNavmesh::handleNavMeshStatus, this, _2));
		LLPathfindingManager::getInstance()->requestGetNavMeshForRegion(currentRegion, true);
	}
}
