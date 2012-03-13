/** 
 * @file llpathfindingmanager.h
 * @author William Todd Stinson
 * @brief A state manager for the various pathfinding states.
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

#ifndef LL_LLPATHFINDINGMANAGER_H
#define LL_LLPATHFINDINGMANAGER_H

#include "llsingleton.h"
#include "lluuid.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"

#include <string>
#include <map>

#include <boost/function.hpp>
#include <boost/signals2.hpp>

class LLFloater;
class LLViewerRegion;
class LLPathfindingNavMeshStatus;

class LLPathfindingManager : public LLSingleton<LLPathfindingManager>
{
	friend class AgentStateResponder;
	friend class LLAgentStateChangeNode;
public:
	typedef std::map<LLUUID, LLPathfindingNavMeshPtr> NavMeshMap;

	typedef enum {
		kAgentStateUnknown,
		kAgentStateFrozen,
		kAgentStateUnfrozen,
		kAgentStateNotEnabled,
		kAgentStateError
	} EAgentState;

	typedef boost::function<void (EAgentState)>         agent_state_callback_t;
	typedef boost::signals2::signal<void (EAgentState)> agent_state_signal_t;
	typedef boost::signals2::connection                 agent_state_slot_t;

	typedef enum {
		kLinksetsRequestStarted,
		kLinksetsRequestCompleted,
		kLinksetsRequestNotEnabled,
		kLinksetsRequestError
	} ELinksetsRequestStatus;

	typedef boost::function<void (ELinksetsRequestStatus, LLPathfindingLinksetListPtr)> linksets_callback_t;

	LLPathfindingManager();
	virtual ~LLPathfindingManager();

	bool isPathfindingEnabledForCurrentRegion() const;
	bool isPathfindingEnabledForRegion(LLViewerRegion *pRegion) const;

	bool isAllowAlterPermanent();
	bool isAllowViewTerrainProperties() const;

	LLPathfindingNavMesh::navmesh_slot_t registerNavMeshListenerForRegion(LLViewerRegion *pRegion, LLPathfindingNavMesh::navmesh_callback_t pNavMeshCallback);
	void requestGetNavMeshForRegion(LLViewerRegion *pRegion);

	void handleNavMeshStatusRequest(const LLPathfindingNavMeshStatus &pNavMeshStatus, LLViewerRegion *pRegion);
	void handleNavMeshStatusUpdate(const LLPathfindingNavMeshStatus &pNavMeshStatus);

	agent_state_slot_t registerAgentStateListener(agent_state_callback_t pAgentStateCallback);
	EAgentState        getAgentState();
	EAgentState        getLastKnownNonErrorAgentState() const;
	void               requestSetAgentState(EAgentState pAgentState);

	ELinksetsRequestStatus requestGetLinksets(linksets_callback_t pLinksetsCallback) const;
	ELinksetsRequestStatus requestSetLinksets(LLPathfindingLinksetListPtr pLinksetList, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD, linksets_callback_t pLinksetsCallback) const;

protected:

private:
	void sendRequestGetNavMeshForRegion(LLPathfindingNavMeshPtr navMeshPtr, LLViewerRegion *pRegion, U32 pNavMeshVersion);

	LLPathfindingNavMeshPtr getNavMeshForRegion(const LLUUID &pRegionUUID);
	LLPathfindingNavMeshPtr getNavMeshForRegion(LLViewerRegion *pRegion);

	static bool isValidAgentState(EAgentState pAgentState);

	void requestGetAgentState();
	void setAgentState(EAgentState pAgentState);
	void handleAgentStateResult(const LLSD &pContent, EAgentState pRequestedAgentState);
	void handleAgentStateError(U32 pStatus, const std::string &pReason, const std::string &pURL);
	void handleAgentStateUpdate(const LLSD &pContent);

	std::string getNavMeshStatusURLForRegion(LLViewerRegion *pRegion) const;
	std::string getRetrieveNavMeshURLForRegion(LLViewerRegion *pRegion) const;
	std::string getAgentStateURLForCurrentRegion() const;
	std::string getObjectLinksetsURLForCurrentRegion() const;
	std::string getTerrainLinksetsURLForCurrentRegion() const;

	std::string    getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const;
	std::string    getCapabilityURLForRegion(LLViewerRegion *pRegion, const std::string &pCapabilityName) const;
	LLViewerRegion *getCurrentRegion() const;

	NavMeshMap           mNavMeshMap;

	agent_state_signal_t mAgentStateSignal;
	EAgentState          mAgentState;
	EAgentState          mLastKnownNonErrorAgentState;
};

#endif // LL_LLPATHFINDINGMANAGER_H
