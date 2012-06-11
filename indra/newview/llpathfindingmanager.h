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

#include <string>
#include <map>

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llpathfindinglinkset.h"
#include "llpathfindingobjectlist.h"
#include "llpathfindingnavmesh.h"
#include "llsingleton.h"
#include "lluuid.h"

class LLViewerRegion;
class LLPathfindingNavMeshStatus;

class LLPathfindingManager : public LLSingleton<LLPathfindingManager>
{
	friend class LLNavMeshSimStateChangeNode;
	friend class LLAgentStateChangeNode;
	friend class NavMeshStatusResponder;
	friend class AgentStateResponder;
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
		kRequestStarted,
		kRequestCompleted,
		kRequestNotEnabled,
		kRequestError
	} ERequestStatus;

	LLPathfindingManager();
	virtual ~LLPathfindingManager();

	bool isPathfindingEnabledForCurrentRegion() const;
	bool isPathfindingEnabledForRegion(LLViewerRegion *pRegion) const;
#ifdef DEPRECATED_UNVERSIONED_NAVMESH
	bool isPathfindingNavMeshVersioningEnabledForCurrentRegionXXX() const;
#endif // DEPRECATED_UNVERSIONED_NAVMESH

	bool isAllowViewTerrainProperties() const;

	LLPathfindingNavMesh::navmesh_slot_t registerNavMeshListenerForRegion(LLViewerRegion *pRegion, LLPathfindingNavMesh::navmesh_callback_t pNavMeshCallback);
	void requestGetNavMeshForRegion(LLViewerRegion *pRegion);

	agent_state_slot_t registerAgentStateListener(agent_state_callback_t pAgentStateCallback);
	EAgentState        getAgentState();
	EAgentState        getLastKnownNonErrorAgentState() const;
	void               requestSetAgentState(EAgentState pAgentState);

	typedef U32 request_id_t;
	typedef boost::function<void (request_id_t, ERequestStatus, LLPathfindingObjectListPtr)> object_request_callback_t;

	void requestGetLinksets(request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const;
	void requestSetLinksets(request_id_t pRequestId, const LLPathfindingObjectListPtr &pLinksetListPtr, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD, object_request_callback_t pLinksetsCallback) const;

	void requestGetCharacters(request_id_t pRequestId, object_request_callback_t pCharactersCallback) const;

protected:

private:
	void sendRequestGetNavMeshForRegion(LLPathfindingNavMeshPtr navMeshPtr, LLViewerRegion *pRegion, const LLPathfindingNavMeshStatus &pNavMeshStatus);

	void handleDeferredGetNavMeshForRegion(const LLUUID &pRegionUUID);
	void handleDeferredGetLinksetsForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const;
	void handleDeferredGetCharactersForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pCharactersCallback) const;

	void handleNavMeshStatusRequest(const LLPathfindingNavMeshStatus &pNavMeshStatus, LLViewerRegion *pRegion);
	void handleNavMeshStatusUpdate(const LLPathfindingNavMeshStatus &pNavMeshStatus);

	LLPathfindingNavMeshPtr getNavMeshForRegion(const LLUUID &pRegionUUID);
	LLPathfindingNavMeshPtr getNavMeshForRegion(LLViewerRegion *pRegion);

	static bool isValidAgentState(EAgentState pAgentState);

	void requestGetAgentState();
	void setAgentState(EAgentState pAgentState);
	void handleAgentStateResult(const LLSD &pContent, EAgentState pRequestedAgentState);
	void handleAgentStateError(U32 pStatus, const std::string &pReason, const std::string &pURL);
	void handleAgentStateUpdate(const LLSD &pContent);
	void handleAgentStateUserNotification(const LLSD &pNotification, const LLSD &pResponse);

	std::string getNavMeshStatusURLForRegion(LLViewerRegion *pRegion) const;
	std::string getRetrieveNavMeshURLForRegion(LLViewerRegion *pRegion) const;
	std::string getAgentStateURLForCurrentRegion() const;
	std::string getObjectLinksetsURLForCurrentRegion() const;
	std::string getTerrainLinksetsURLForCurrentRegion() const;
	std::string getCharactersURLForCurrentRegion() const;

	std::string    getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const;
	std::string    getCapabilityURLForRegion(LLViewerRegion *pRegion, const std::string &pCapabilityName) const;
	LLViewerRegion *getCurrentRegion() const;

	NavMeshMap           mNavMeshMap;

	agent_state_signal_t mAgentStateSignal;
	EAgentState          mAgentState;
	EAgentState          mLastKnownNonErrorAgentState;
};

#endif // LL_LLPATHFINDINGMANAGER_H
