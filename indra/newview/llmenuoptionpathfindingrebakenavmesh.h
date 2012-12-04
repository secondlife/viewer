/** 
* @file   llmenuoptionpathfindingrebakenavmesh.h
* @brief  Header file for llmenuoptionpathfindingrebakenavmesh
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
#ifndef LL_LLMENUOPTIONPATHFINDINGREBAKENAVMESH_H
#define LL_LLMENUOPTIONPATHFINDINGREBAKENAVMESH_H

#include <boost/signals2.hpp>

#include "llpathfindingmanager.h"
#include "llpathfindingnavmesh.h"
#include "llsingleton.h"

class LLPathfindingNavMeshStatus;

class LLMenuOptionPathfindingRebakeNavmesh : public LLSingleton<LLMenuOptionPathfindingRebakeNavmesh>
{
	LOG_CLASS(LLMenuOptionPathfindingRebakeNavmesh);

public:
	typedef enum
	{
		kRebakeNavMesh_Available,
		kRebakeNavMesh_RequestSent,
		kRebakeNavMesh_InProgress,
		kRebakeNavMesh_NotAvailable,
		kRebakeNavMesh_Default = kRebakeNavMesh_NotAvailable
	} ERebakeNavMeshMode;

	LLMenuOptionPathfindingRebakeNavmesh();
	virtual ~LLMenuOptionPathfindingRebakeNavmesh();

	void               initialize();
	void               quit();

	bool               canRebakeRegion() const;
	ERebakeNavMeshMode getMode() const;
	
	void               sendRequestRebakeNavmesh();

protected:

private:
	void setMode(ERebakeNavMeshMode pRebakeNavMeshMode);

	void handleAgentState(BOOL pCanRebakeRegion);
	void handleRebakeNavMeshResponse(bool pResponseStatus);
	void handleNavMeshStatus(const LLPathfindingNavMeshStatus &pNavMeshStatus);
	void handleRegionBoundaryCrossed();

	void createNavMeshStatusListenerForCurrentRegion();

	bool                                     mIsInitialized;

	bool                                     mCanRebakeRegion;
	ERebakeNavMeshMode                       mRebakeNavMeshMode;
	
	LLPathfindingNavMesh::navmesh_slot_t     mNavMeshSlot;
	boost::signals2::connection              mRegionCrossingSlot;
	LLPathfindingManager::agent_state_slot_t mAgentStateSlot;
};

#endif // LL_LLMENUOPTIONPATHFINDINGREBAKENAVMESH_H
