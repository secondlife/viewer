/** 
* @file   llpathfindingmanager.h
* @brief  Header file for llpathfindingmanager
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

class LLPathfindingNavMeshStatus;
class LLUUID;
class LLViewerRegion;

class LLPathfindingManager : public LLSingleton<LLPathfindingManager>
{
	friend class LLNavMeshSimStateChangeNode;
	friend class NavMeshStatusResponder;
	friend class LLAgentStateChangeNode;
	friend class AgentStateResponder;
public:
	typedef enum {
		kRequestStarted,
		kRequestCompleted,
		kRequestNotEnabled,
		kRequestError
	} ERequestStatus;

	LLPathfindingManager();
	virtual ~LLPathfindingManager();

	void initSystem();
	void quitSystem();

	bool isPathfindingViewEnabled() const;
	bool isPathfindingEnabledForCurrentRegion() const;
	bool isPathfindingEnabledForRegion(LLViewerRegion *pRegion) const;

	bool isAllowViewTerrainProperties() const;

	LLPathfindingNavMesh::navmesh_slot_t registerNavMeshListenerForRegion(LLViewerRegion *pRegion, LLPathfindingNavMesh::navmesh_callback_t pNavMeshCallback);
	void requestGetNavMeshForRegion(LLViewerRegion *pRegion, bool pIsGetStatusOnly);

	typedef U32 request_id_t;
	typedef boost::function<void (request_id_t, ERequestStatus, LLPathfindingObjectListPtr)> object_request_callback_t;

	void requestGetLinksets(request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const;
	void requestSetLinksets(request_id_t pRequestId, const LLPathfindingObjectListPtr &pLinksetListPtr, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD, object_request_callback_t pLinksetsCallback) const;

	void requestGetCharacters(request_id_t pRequestId, object_request_callback_t pCharactersCallback) const;

	typedef boost::function<void (BOOL)>         agent_state_callback_t;
	typedef boost::signals2::signal<void (BOOL)> agent_state_signal_t;
	typedef boost::signals2::connection          agent_state_slot_t;	

	agent_state_slot_t registerAgentStateListener(agent_state_callback_t pAgentStateCallback);
	void requestGetAgentState();	

	typedef boost::function<void (bool)> rebake_navmesh_callback_t;
	void requestRebakeNavMesh(rebake_navmesh_callback_t pRebakeNavMeshCallback);

protected:

private:
	typedef std::map<LLUUID, LLPathfindingNavMeshPtr> NavMeshMap;

	void sendRequestGetNavMeshForRegion(LLPathfindingNavMeshPtr navMeshPtr, LLViewerRegion *pRegion, const LLPathfindingNavMeshStatus &pNavMeshStatus);

	void handleDeferredGetAgentStateForRegion(const LLUUID &pRegionUUID);
	void handleDeferredGetNavMeshForRegion(const LLUUID &pRegionUUID, bool pIsGetStatusOnly);
	void handleDeferredGetLinksetsForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const;
	void handleDeferredGetCharactersForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pCharactersCallback) const;

	void handleNavMeshStatusRequest(const LLPathfindingNavMeshStatus &pNavMeshStatus, LLViewerRegion *pRegion, bool pIsGetStatusOnly);
	void handleNavMeshStatusUpdate(const LLPathfindingNavMeshStatus &pNavMeshStatus);

	void handleAgentState(BOOL pCanRebakeRegion);

	LLPathfindingNavMeshPtr getNavMeshForRegion(const LLUUID &pRegionUUID);
	LLPathfindingNavMeshPtr getNavMeshForRegion(LLViewerRegion *pRegion);

	std::string getNavMeshStatusURLForCurrentRegion() const;
	std::string getNavMeshStatusURLForRegion(LLViewerRegion *pRegion) const;
	std::string getRetrieveNavMeshURLForRegion(LLViewerRegion *pRegion) const;
	std::string getObjectLinksetsURLForCurrentRegion() const;
	std::string getTerrainLinksetsURLForCurrentRegion() const;
	std::string getCharactersURLForCurrentRegion() const;
	std::string	getAgentStateURLForRegion(LLViewerRegion *pRegion) const;
	std::string getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const;
	std::string getCapabilityURLForRegion(LLViewerRegion *pRegion, const std::string &pCapabilityName) const;
	LLViewerRegion *getCurrentRegion() const;

	NavMeshMap           mNavMeshMap;
	agent_state_signal_t mAgentStateSignal;
};

#endif // LL_LLPATHFINDINGMANAGER_H
