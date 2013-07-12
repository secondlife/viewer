/** 
* @file llpathfindingmanager.cpp
* @brief Implementation of llpathfindingmanager
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

#include "llpathfindingmanager.h"

#include <string>
#include <map>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llhttpclient.h"
#include "llhttpnode.h"
#include "llnotificationsutil.h"
#include "llpathfindingcharacterlist.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "llpathfindingobject.h"
#include "llpathinglib.h"
#include "llsingleton.h"
#include "llsd.h"
#include "lltrans.h"
#include "lluuid.h"
#include "llviewerregion.h"
#include "llweb.h"

#define CAP_SERVICE_RETRIEVE_NAVMESH        "RetrieveNavMeshSrc"

#define CAP_SERVICE_NAVMESH_STATUS          "NavMeshGenerationStatus"

#define CAP_SERVICE_OBJECT_LINKSETS         "ObjectNavMeshProperties"
#define CAP_SERVICE_TERRAIN_LINKSETS        "TerrainNavMeshProperties"

#define CAP_SERVICE_CHARACTERS              "CharacterProperties"

#define SIM_MESSAGE_NAVMESH_STATUS_UPDATE   "/message/NavMeshStatusUpdate"
#define SIM_MESSAGE_AGENT_STATE_UPDATE      "/message/AgentStateUpdate"
#define SIM_MESSAGE_BODY_FIELD              "body"

#define CAP_SERVICE_AGENT_STATE             "AgentState"

#define AGENT_STATE_CAN_REBAKE_REGION_FIELD "can_modify_navmesh"

//---------------------------------------------------------------------------
// LLNavMeshSimStateChangeNode
//---------------------------------------------------------------------------

class LLNavMeshSimStateChangeNode : public LLHTTPNode
{
public:
	virtual void post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const;
};

LLHTTPRegistration<LLNavMeshSimStateChangeNode> gHTTPRegistrationNavMeshSimStateChangeNode(SIM_MESSAGE_NAVMESH_STATUS_UPDATE);


//---------------------------------------------------------------------------
// LLAgentStateChangeNode
//---------------------------------------------------------------------------
class LLAgentStateChangeNode : public LLHTTPNode
{
public:
	virtual void post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const;
};

LLHTTPRegistration<LLAgentStateChangeNode> gHTTPRegistrationAgentStateChangeNode(SIM_MESSAGE_AGENT_STATE_UPDATE);

//---------------------------------------------------------------------------
// NavMeshStatusResponder
//---------------------------------------------------------------------------

class NavMeshStatusResponder : public LLHTTPClient::Responder
{
public:
	NavMeshStatusResponder(const std::string &pCapabilityURL, LLViewerRegion *pRegion, bool pIsGetStatusOnly);
	virtual ~NavMeshStatusResponder();

	virtual void result(const LLSD &pContent);
	virtual void errorWithContent(U32 pStatus, const std::string& pReason, const LLSD& pContent);

protected:

private:
	std::string    mCapabilityURL;
	LLViewerRegion *mRegion;
	LLUUID         mRegionUUID;
	bool           mIsGetStatusOnly;
};

//---------------------------------------------------------------------------
// NavMeshResponder
//---------------------------------------------------------------------------

class NavMeshResponder : public LLHTTPClient::Responder
{
public:
	NavMeshResponder(const std::string &pCapabilityURL, U32 pNavMeshVersion, LLPathfindingNavMeshPtr pNavMeshPtr);
	virtual ~NavMeshResponder();

	virtual void result(const LLSD &pContent);
	virtual void errorWithContent(U32 pStatus, const std::string& pReason, const LLSD& pContent);

protected:

private:
	std::string             mCapabilityURL;
	U32                     mNavMeshVersion;
	LLPathfindingNavMeshPtr mNavMeshPtr;
};

//---------------------------------------------------------------------------
// AgentStateResponder
//---------------------------------------------------------------------------

class AgentStateResponder : public LLHTTPClient::Responder
{
public:
	AgentStateResponder(const std::string &pCapabilityURL);
	virtual ~AgentStateResponder();

	virtual void result(const LLSD &pContent);
	virtual void errorWithContent(U32 pStatus, const std::string& pReason, const LLSD& pContent);

protected:

private:
	std::string mCapabilityURL;
};


//---------------------------------------------------------------------------
// NavMeshRebakeResponder
//---------------------------------------------------------------------------
class NavMeshRebakeResponder : public LLHTTPClient::Responder
{
public:
	NavMeshRebakeResponder(const std::string &pCapabilityURL, LLPathfindingManager::rebake_navmesh_callback_t pRebakeNavMeshCallback);
	virtual ~NavMeshRebakeResponder();

	virtual void result(const LLSD &pContent);
	virtual void errorWithContent(U32 pStatus, const std::string& pReason, const LLSD& pContent);

protected:

private:
	std::string                                     mCapabilityURL;
	LLPathfindingManager::rebake_navmesh_callback_t mRebakeNavMeshCallback;
};

//---------------------------------------------------------------------------
// LinksetsResponder
//---------------------------------------------------------------------------

class LinksetsResponder
{
public:
	LinksetsResponder(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::object_request_callback_t pLinksetsCallback, bool pIsObjectRequested, bool pIsTerrainRequested);
	virtual ~LinksetsResponder();

	void handleObjectLinksetsResult(const LLSD &pContent);
	void handleObjectLinksetsError(U32 pStatus, const std::string &pReason, 
								   const LLSD& pContent, const std::string &pURL);
	void handleTerrainLinksetsResult(const LLSD &pContent);
	void handleTerrainLinksetsError(U32 pStatus, const std::string &pReason,
									const LLSD& pContent, const std::string &pURL);

protected:

private:
	void sendCallback();

	typedef enum
	{
		kNotRequested,
		kWaiting,
		kReceivedGood,
		kReceivedError
	} EMessagingState;

	LLPathfindingManager::request_id_t              mRequestId;
	LLPathfindingManager::object_request_callback_t mLinksetsCallback;

	EMessagingState                                 mObjectMessagingState;
	EMessagingState                                 mTerrainMessagingState;

	LLPathfindingObjectListPtr                      mObjectLinksetListPtr;
	LLPathfindingObjectPtr                          mTerrainLinksetPtr;
};

typedef boost::shared_ptr<LinksetsResponder> LinksetsResponderPtr;

//---------------------------------------------------------------------------
// ObjectLinksetsResponder
//---------------------------------------------------------------------------

class ObjectLinksetsResponder : public LLHTTPClient::Responder
{
public:
	ObjectLinksetsResponder(const std::string &pCapabilityURL, LinksetsResponderPtr pLinksetsResponsderPtr);
	virtual ~ObjectLinksetsResponder();

	virtual void result(const LLSD &pContent);
	virtual void errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent);

protected:

private:
	std::string          mCapabilityURL;
	LinksetsResponderPtr mLinksetsResponsderPtr;
};

//---------------------------------------------------------------------------
// TerrainLinksetsResponder
//---------------------------------------------------------------------------

class TerrainLinksetsResponder : public LLHTTPClient::Responder
{
public:
	TerrainLinksetsResponder(const std::string &pCapabilityURL, LinksetsResponderPtr pLinksetsResponsderPtr);
	virtual ~TerrainLinksetsResponder();

	virtual void result(const LLSD &pContent);
	virtual void errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent);

protected:

private:
	std::string          mCapabilityURL;
	LinksetsResponderPtr mLinksetsResponsderPtr;
};

//---------------------------------------------------------------------------
// CharactersResponder
//---------------------------------------------------------------------------

class CharactersResponder : public LLHTTPClient::Responder
{
public:
	CharactersResponder(const std::string &pCapabilityURL, LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::object_request_callback_t pCharactersCallback);
	virtual ~CharactersResponder();

	virtual void result(const LLSD &pContent);
	virtual void errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent);

protected:

private:
	std::string                                     mCapabilityURL;
	LLPathfindingManager::request_id_t              mRequestId;
	LLPathfindingManager::object_request_callback_t mCharactersCallback;
};

//---------------------------------------------------------------------------
// LLPathfindingManager
//---------------------------------------------------------------------------

LLPathfindingManager::LLPathfindingManager()
	: LLSingleton<LLPathfindingManager>(),
	mNavMeshMap(),
	mAgentStateSignal()
{
}

LLPathfindingManager::~LLPathfindingManager()
{	
	quitSystem();
}

void LLPathfindingManager::initSystem()
{
	if (LLPathingLib::getInstance() == NULL)
	{
		LLPathingLib::initSystem();
	}
}

void LLPathfindingManager::quitSystem()
{
	if (LLPathingLib::getInstance() != NULL)
	{
		LLPathingLib::quitSystem();
	}
}

bool LLPathfindingManager::isPathfindingViewEnabled() const
{
	return (LLPathingLib::getInstance() != NULL);
}

bool LLPathfindingManager::isPathfindingEnabledForCurrentRegion() const
{
	return isPathfindingEnabledForRegion(getCurrentRegion());
}

bool LLPathfindingManager::isPathfindingEnabledForRegion(LLViewerRegion *pRegion) const
{
	std::string retrieveNavMeshURL = getRetrieveNavMeshURLForRegion(pRegion);
	return !retrieveNavMeshURL.empty();
}

bool LLPathfindingManager::isAllowViewTerrainProperties() const
{
	LLViewerRegion* region = getCurrentRegion();
	return (gAgent.isGodlike() || ((region != NULL) && region->canManageEstate()));
}

LLPathfindingNavMesh::navmesh_slot_t LLPathfindingManager::registerNavMeshListenerForRegion(LLViewerRegion *pRegion, LLPathfindingNavMesh::navmesh_callback_t pNavMeshCallback)
{
	LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(pRegion);
	return navMeshPtr->registerNavMeshListener(pNavMeshCallback);
}

void LLPathfindingManager::requestGetNavMeshForRegion(LLViewerRegion *pRegion, bool pIsGetStatusOnly)
{
	LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(pRegion);

	if (pRegion == NULL)
	{
		navMeshPtr->handleNavMeshNotEnabled();
	}
	else if (!pRegion->capabilitiesReceived())
	{
		navMeshPtr->handleNavMeshWaitForRegionLoad();
		pRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetNavMeshForRegion, this, _1, pIsGetStatusOnly));
	}
	else if (!isPathfindingEnabledForRegion(pRegion))
	{
		navMeshPtr->handleNavMeshNotEnabled();
	}
	else
	{
		std::string navMeshStatusURL = getNavMeshStatusURLForRegion(pRegion);
		llassert(!navMeshStatusURL.empty());
		navMeshPtr->handleNavMeshCheckVersion();
		LLHTTPClient::ResponderPtr navMeshStatusResponder = new NavMeshStatusResponder(navMeshStatusURL, pRegion, pIsGetStatusOnly);
		LLHTTPClient::get(navMeshStatusURL, navMeshStatusResponder);
	}
}

void LLPathfindingManager::requestGetLinksets(request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const
{
	LLPathfindingObjectListPtr emptyLinksetListPtr;
	LLViewerRegion *currentRegion = getCurrentRegion();

	if (currentRegion == NULL)
	{
		pLinksetsCallback(pRequestId, kRequestNotEnabled, emptyLinksetListPtr);
	}
	else if (!currentRegion->capabilitiesReceived())
	{
		pLinksetsCallback(pRequestId, kRequestStarted, emptyLinksetListPtr);
		currentRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetLinksetsForRegion, this, _1, pRequestId, pLinksetsCallback));
	}
	else
	{
		std::string objectLinksetsURL = getObjectLinksetsURLForCurrentRegion();
		std::string terrainLinksetsURL = getTerrainLinksetsURLForCurrentRegion();
		if (objectLinksetsURL.empty() || terrainLinksetsURL.empty())
		{
			pLinksetsCallback(pRequestId, kRequestNotEnabled, emptyLinksetListPtr);
		}
		else
		{
			pLinksetsCallback(pRequestId, kRequestStarted, emptyLinksetListPtr);

			bool doRequestTerrain = isAllowViewTerrainProperties();
			LinksetsResponderPtr linksetsResponderPtr(new LinksetsResponder(pRequestId, pLinksetsCallback, true, doRequestTerrain));

			LLHTTPClient::ResponderPtr objectLinksetsResponder = new ObjectLinksetsResponder(objectLinksetsURL, linksetsResponderPtr);
			LLHTTPClient::get(objectLinksetsURL, objectLinksetsResponder);

			if (doRequestTerrain)
			{
				LLHTTPClient::ResponderPtr terrainLinksetsResponder = new TerrainLinksetsResponder(terrainLinksetsURL, linksetsResponderPtr);
				LLHTTPClient::get(terrainLinksetsURL, terrainLinksetsResponder);
			}
		}
	}
}

void LLPathfindingManager::requestSetLinksets(request_id_t pRequestId, const LLPathfindingObjectListPtr &pLinksetListPtr, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD, object_request_callback_t pLinksetsCallback) const
{
	LLPathfindingObjectListPtr emptyLinksetListPtr;

	std::string objectLinksetsURL = getObjectLinksetsURLForCurrentRegion();
	std::string terrainLinksetsURL = getTerrainLinksetsURLForCurrentRegion();
	if (objectLinksetsURL.empty() || terrainLinksetsURL.empty())
	{
		pLinksetsCallback(pRequestId, kRequestNotEnabled, emptyLinksetListPtr);
	}
	else if ((pLinksetListPtr == NULL) || pLinksetListPtr->isEmpty())
	{
		pLinksetsCallback(pRequestId, kRequestCompleted, emptyLinksetListPtr);
	}
	else 
	{
		const LLPathfindingLinksetList *linksetList = dynamic_cast<const LLPathfindingLinksetList *>(pLinksetListPtr.get());

		LLSD objectPostData = linksetList->encodeObjectFields(pLinksetUse, pA, pB, pC, pD);
		LLSD terrainPostData;
		if (isAllowViewTerrainProperties())
		{
			terrainPostData = linksetList->encodeTerrainFields(pLinksetUse, pA, pB, pC, pD);
		}

		if (objectPostData.isUndefined() && terrainPostData.isUndefined())
		{
			pLinksetsCallback(pRequestId, kRequestCompleted, emptyLinksetListPtr);
		}
		else
		{
			pLinksetsCallback(pRequestId, kRequestStarted, emptyLinksetListPtr);

			LinksetsResponderPtr linksetsResponderPtr(new LinksetsResponder(pRequestId, pLinksetsCallback, !objectPostData.isUndefined(), !terrainPostData.isUndefined()));

			if (!objectPostData.isUndefined())
			{
				LLHTTPClient::ResponderPtr objectLinksetsResponder = new ObjectLinksetsResponder(objectLinksetsURL, linksetsResponderPtr);
				LLHTTPClient::put(objectLinksetsURL, objectPostData, objectLinksetsResponder);
			}

			if (!terrainPostData.isUndefined())
			{
				LLHTTPClient::ResponderPtr terrainLinksetsResponder = new TerrainLinksetsResponder(terrainLinksetsURL, linksetsResponderPtr);
				LLHTTPClient::put(terrainLinksetsURL, terrainPostData, terrainLinksetsResponder);
			}
		}
	}
}

void LLPathfindingManager::requestGetCharacters(request_id_t pRequestId, object_request_callback_t pCharactersCallback) const
{
	LLPathfindingObjectListPtr emptyCharacterListPtr;

	LLViewerRegion *currentRegion = getCurrentRegion();

	if (currentRegion == NULL)
	{
		pCharactersCallback(pRequestId, kRequestNotEnabled, emptyCharacterListPtr);
	}
	else if (!currentRegion->capabilitiesReceived())
	{
		pCharactersCallback(pRequestId, kRequestStarted, emptyCharacterListPtr);
		currentRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetCharactersForRegion, this, _1, pRequestId, pCharactersCallback));
	}
	else
	{
		std::string charactersURL = getCharactersURLForCurrentRegion();
		if (charactersURL.empty())
		{
			pCharactersCallback(pRequestId, kRequestNotEnabled, emptyCharacterListPtr);
		}
		else
		{
			pCharactersCallback(pRequestId, kRequestStarted, emptyCharacterListPtr);

			LLHTTPClient::ResponderPtr charactersResponder = new CharactersResponder(charactersURL, pRequestId, pCharactersCallback);
			LLHTTPClient::get(charactersURL, charactersResponder);
		}
	}
}

LLPathfindingManager::agent_state_slot_t LLPathfindingManager::registerAgentStateListener(agent_state_callback_t pAgentStateCallback)
{
	return mAgentStateSignal.connect(pAgentStateCallback);
}

void LLPathfindingManager::requestGetAgentState()
{
	LLViewerRegion *currentRegion = getCurrentRegion();

	if (currentRegion == NULL)
	{
		mAgentStateSignal(FALSE);
	}
	else
	{
		if (!currentRegion->capabilitiesReceived())
		{
			currentRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetAgentStateForRegion, this, _1));
		}
		else if (!isPathfindingEnabledForRegion(currentRegion))
		{
			mAgentStateSignal(FALSE);
		}
		else
		{
			std::string agentStateURL = getAgentStateURLForRegion(currentRegion);
			llassert(!agentStateURL.empty());
			LLHTTPClient::ResponderPtr responder = new AgentStateResponder(agentStateURL);
			LLHTTPClient::get(agentStateURL, responder);
		}
	}
}

void LLPathfindingManager::requestRebakeNavMesh(rebake_navmesh_callback_t pRebakeNavMeshCallback)
{
	LLViewerRegion *currentRegion = getCurrentRegion();

	if (currentRegion == NULL)
	{
		pRebakeNavMeshCallback(false);
	}
	else if (!isPathfindingEnabledForRegion(currentRegion))
	{
		pRebakeNavMeshCallback(false);
	}
	else
	{
		std::string navMeshStatusURL = getNavMeshStatusURLForCurrentRegion();
		llassert(!navMeshStatusURL.empty());
		LLSD postData;			
		postData["command"] = "rebuild";
		LLHTTPClient::ResponderPtr responder = new NavMeshRebakeResponder(navMeshStatusURL, pRebakeNavMeshCallback);
		LLHTTPClient::post(navMeshStatusURL, postData, responder);
	}
}

void LLPathfindingManager::sendRequestGetNavMeshForRegion(LLPathfindingNavMeshPtr navMeshPtr, LLViewerRegion *pRegion, const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	if ((pRegion == NULL) || !pRegion->isAlive())
	{
		navMeshPtr->handleNavMeshNotEnabled();
	}
	else
	{
		std::string navMeshURL = getRetrieveNavMeshURLForRegion(pRegion);

		if (navMeshURL.empty())
		{
			navMeshPtr->handleNavMeshNotEnabled();
		}
		else
		{
			navMeshPtr->handleNavMeshStart(pNavMeshStatus);
			LLHTTPClient::ResponderPtr responder = new NavMeshResponder(navMeshURL, pNavMeshStatus.getVersion(), navMeshPtr);

			LLSD postData;
			LLHTTPClient::post(navMeshURL, postData, responder);
		}
	}
}

void LLPathfindingManager::handleDeferredGetAgentStateForRegion(const LLUUID &pRegionUUID)
{
	LLViewerRegion *currentRegion = getCurrentRegion();

	if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
	{
		requestGetAgentState();
	}
}

void LLPathfindingManager::handleDeferredGetNavMeshForRegion(const LLUUID &pRegionUUID, bool pIsGetStatusOnly)
{
	LLViewerRegion *currentRegion = getCurrentRegion();

	if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
	{
		requestGetNavMeshForRegion(currentRegion, pIsGetStatusOnly);
	}
}

void LLPathfindingManager::handleDeferredGetLinksetsForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const
{
	LLViewerRegion *currentRegion = getCurrentRegion();

	if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
	{
		requestGetLinksets(pRequestId, pLinksetsCallback);
	}
}

void LLPathfindingManager::handleDeferredGetCharactersForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pCharactersCallback) const
{
	LLViewerRegion *currentRegion = getCurrentRegion();

	if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
	{
		requestGetCharacters(pRequestId, pCharactersCallback);
	}
}

void LLPathfindingManager::handleNavMeshStatusRequest(const LLPathfindingNavMeshStatus &pNavMeshStatus, LLViewerRegion *pRegion, bool pIsGetStatusOnly)
{
	LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(pNavMeshStatus.getRegionUUID());

	if (!pNavMeshStatus.isValid())
	{
		navMeshPtr->handleNavMeshError();
	}
	else
	{
		if (navMeshPtr->hasNavMeshVersion(pNavMeshStatus))
		{
			navMeshPtr->handleRefresh(pNavMeshStatus);
		}
		else if (pIsGetStatusOnly)
		{
			navMeshPtr->handleNavMeshNewVersion(pNavMeshStatus);
		}
		else
		{
			sendRequestGetNavMeshForRegion(navMeshPtr, pRegion, pNavMeshStatus);
		}
	}
}

void LLPathfindingManager::handleNavMeshStatusUpdate(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(pNavMeshStatus.getRegionUUID());

	if (!pNavMeshStatus.isValid())
	{
		navMeshPtr->handleNavMeshError();
	}
	else
	{
		navMeshPtr->handleNavMeshNewVersion(pNavMeshStatus);
	}
}

void LLPathfindingManager::handleAgentState(BOOL pCanRebakeRegion) 
{
	mAgentStateSignal(pCanRebakeRegion);
}

LLPathfindingNavMeshPtr LLPathfindingManager::getNavMeshForRegion(const LLUUID &pRegionUUID)
{
	LLPathfindingNavMeshPtr navMeshPtr;
	NavMeshMap::iterator navMeshIter = mNavMeshMap.find(pRegionUUID);
	if (navMeshIter == mNavMeshMap.end())
	{
		navMeshPtr = LLPathfindingNavMeshPtr(new LLPathfindingNavMesh(pRegionUUID));
		mNavMeshMap.insert(std::pair<LLUUID, LLPathfindingNavMeshPtr>(pRegionUUID, navMeshPtr));
	}
	else
	{
		navMeshPtr = navMeshIter->second;
	}

	return navMeshPtr;
}

LLPathfindingNavMeshPtr LLPathfindingManager::getNavMeshForRegion(LLViewerRegion *pRegion)
{
	LLUUID regionUUID;
	if (pRegion != NULL)
	{
		regionUUID = pRegion->getRegionID();
	}

	return getNavMeshForRegion(regionUUID);
}

std::string LLPathfindingManager::getNavMeshStatusURLForCurrentRegion() const
{
	return getNavMeshStatusURLForRegion(getCurrentRegion());
}

std::string LLPathfindingManager::getNavMeshStatusURLForRegion(LLViewerRegion *pRegion) const
{
	return getCapabilityURLForRegion(pRegion, CAP_SERVICE_NAVMESH_STATUS);
}

std::string LLPathfindingManager::getRetrieveNavMeshURLForRegion(LLViewerRegion *pRegion) const
{
	return getCapabilityURLForRegion(pRegion, CAP_SERVICE_RETRIEVE_NAVMESH);
}

std::string LLPathfindingManager::getObjectLinksetsURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_OBJECT_LINKSETS);
}

std::string LLPathfindingManager::getTerrainLinksetsURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_TERRAIN_LINKSETS);
}

std::string LLPathfindingManager::getCharactersURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_CHARACTERS);
}

std::string LLPathfindingManager::getAgentStateURLForRegion(LLViewerRegion *pRegion) const
{
	return getCapabilityURLForRegion(pRegion, CAP_SERVICE_AGENT_STATE);
}

std::string LLPathfindingManager::getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const
{
	return getCapabilityURLForRegion(getCurrentRegion(), pCapabilityName);
}

std::string LLPathfindingManager::getCapabilityURLForRegion(LLViewerRegion *pRegion, const std::string &pCapabilityName) const
{
	std::string capabilityURL("");

	if (pRegion != NULL)
	{
		capabilityURL = pRegion->getCapability(pCapabilityName);
	}

	if (capabilityURL.empty())
	{
		llwarns << "cannot find capability '" << pCapabilityName << "' for current region '"
			<< ((pRegion != NULL) ? pRegion->getName() : "<null>") << "'" << llendl;
	}

	return capabilityURL;
}

LLViewerRegion *LLPathfindingManager::getCurrentRegion() const
{
	return gAgent.getRegion();
}

//---------------------------------------------------------------------------
// LLNavMeshSimStateChangeNode
//---------------------------------------------------------------------------

void LLNavMeshSimStateChangeNode::post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const
{
	llassert(pInput.has(SIM_MESSAGE_BODY_FIELD));
	llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).isMap());
	LLPathfindingNavMeshStatus navMeshStatus(pInput.get(SIM_MESSAGE_BODY_FIELD));
	LLPathfindingManager::getInstance()->handleNavMeshStatusUpdate(navMeshStatus);
}

//---------------------------------------------------------------------------
// LLAgentStateChangeNode
//---------------------------------------------------------------------------

void LLAgentStateChangeNode::post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const
{
	llassert(pInput.has(SIM_MESSAGE_BODY_FIELD));
	llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).isMap());
	llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).has(AGENT_STATE_CAN_REBAKE_REGION_FIELD));
	llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).isBoolean());
	BOOL canRebakeRegion = pInput.get(SIM_MESSAGE_BODY_FIELD).get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).asBoolean();
	
	LLPathfindingManager::getInstance()->handleAgentState(canRebakeRegion);
}

//---------------------------------------------------------------------------
// NavMeshStatusResponder
//---------------------------------------------------------------------------

NavMeshStatusResponder::NavMeshStatusResponder(const std::string &pCapabilityURL, LLViewerRegion *pRegion, bool pIsGetStatusOnly)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mRegion(pRegion),
	mRegionUUID(),
	mIsGetStatusOnly(pIsGetStatusOnly)
{
	if (mRegion != NULL)
	{
		mRegionUUID = mRegion->getRegionID();
	}
}

NavMeshStatusResponder::~NavMeshStatusResponder()
{
}

void NavMeshStatusResponder::result(const LLSD &pContent)
{
	LLPathfindingNavMeshStatus navMeshStatus(mRegionUUID, pContent);
	LLPathfindingManager::getInstance()->handleNavMeshStatusRequest(navMeshStatus, mRegion, mIsGetStatusOnly);
}

void NavMeshStatusResponder::errorWithContent(U32 pStatus, const std::string& pReason, const LLSD& pContent)
{
	llwarns << "NavMeshStatusResponder error [status:" << pStatus << "]: " << pContent << llendl;
	LLPathfindingNavMeshStatus navMeshStatus(mRegionUUID);
	LLPathfindingManager::getInstance()->handleNavMeshStatusRequest(navMeshStatus, mRegion, mIsGetStatusOnly);
}

//---------------------------------------------------------------------------
// NavMeshResponder
//---------------------------------------------------------------------------

NavMeshResponder::NavMeshResponder(const std::string &pCapabilityURL, U32 pNavMeshVersion, LLPathfindingNavMeshPtr pNavMeshPtr)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mNavMeshVersion(pNavMeshVersion),
	mNavMeshPtr(pNavMeshPtr)
{
}

NavMeshResponder::~NavMeshResponder()
{
}

void NavMeshResponder::result(const LLSD &pContent)
{
	mNavMeshPtr->handleNavMeshResult(pContent, mNavMeshVersion);
}

void NavMeshResponder::errorWithContent(U32 pStatus, const std::string& pReason, const LLSD& pContent)
{
	mNavMeshPtr->handleNavMeshError(pStatus, pReason, pContent, mCapabilityURL, mNavMeshVersion);
}

//---------------------------------------------------------------------------
// AgentStateResponder
//---------------------------------------------------------------------------

AgentStateResponder::AgentStateResponder(const std::string &pCapabilityURL)
: LLHTTPClient::Responder()
, mCapabilityURL(pCapabilityURL)
{
}

AgentStateResponder::~AgentStateResponder()
{
}

void AgentStateResponder::result(const LLSD &pContent)
{
	llassert(pContent.has(AGENT_STATE_CAN_REBAKE_REGION_FIELD));
	llassert(pContent.get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).isBoolean());
	BOOL canRebakeRegion = pContent.get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).asBoolean();
	LLPathfindingManager::getInstance()->handleAgentState(canRebakeRegion);
}

void AgentStateResponder::errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent)
{
	llwarns << "AgentStateResponder error [status:" << pStatus << "]: " << pContent << llendl;
	LLPathfindingManager::getInstance()->handleAgentState(FALSE);
}


//---------------------------------------------------------------------------
// navmesh rebake responder
//---------------------------------------------------------------------------
NavMeshRebakeResponder::NavMeshRebakeResponder(const std::string &pCapabilityURL, LLPathfindingManager::rebake_navmesh_callback_t pRebakeNavMeshCallback)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mRebakeNavMeshCallback(pRebakeNavMeshCallback)
{
}

NavMeshRebakeResponder::~NavMeshRebakeResponder()
{
}

void NavMeshRebakeResponder::result(const LLSD &pContent)
{
	mRebakeNavMeshCallback(true);
}

void NavMeshRebakeResponder::errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent)
{
	llwarns << "NavMeshRebakeResponder error [status:" << pStatus << "]: " << pContent << llendl;
	mRebakeNavMeshCallback(false);
}

//---------------------------------------------------------------------------
// LinksetsResponder
//---------------------------------------------------------------------------

LinksetsResponder::LinksetsResponder(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::object_request_callback_t pLinksetsCallback, bool pIsObjectRequested, bool pIsTerrainRequested)
	: mRequestId(pRequestId),
	mLinksetsCallback(pLinksetsCallback),
	mObjectMessagingState(pIsObjectRequested ? kWaiting : kNotRequested),
	mTerrainMessagingState(pIsTerrainRequested ? kWaiting : kNotRequested),
	mObjectLinksetListPtr(),
	mTerrainLinksetPtr()
{
}

LinksetsResponder::~LinksetsResponder()
{
}

void LinksetsResponder::handleObjectLinksetsResult(const LLSD &pContent)
{
	mObjectLinksetListPtr = LLPathfindingObjectListPtr(new LLPathfindingLinksetList(pContent));

	mObjectMessagingState = kReceivedGood;
	if (mTerrainMessagingState != kWaiting)
	{
		sendCallback();
	}
}

void LinksetsResponder::handleObjectLinksetsError(U32 pStatus, const std::string &pReason,
												 const LLSD& pContent, const std::string &pURL)
{
	llwarns << "LinksetsResponder object linksets error with request to URL '" << pURL << "' [status:"
			<< pStatus << "]: " << pContent << llendl;
	mObjectMessagingState = kReceivedError;
	if (mTerrainMessagingState != kWaiting)
	{
		sendCallback();
	}
}

void LinksetsResponder::handleTerrainLinksetsResult(const LLSD &pContent)
{
	mTerrainLinksetPtr = LLPathfindingObjectPtr(new LLPathfindingLinkset(pContent));

	mTerrainMessagingState = kReceivedGood;
	if (mObjectMessagingState != kWaiting)
	{
		sendCallback();
	}
}

void LinksetsResponder::handleTerrainLinksetsError(U32 pStatus, const std::string &pReason,
												   const LLSD& pContent, const std::string &pURL)
{
	llwarns << "LinksetsResponder terrain linksets error with request to URL '" << pURL << "' [status:"
			<< pStatus << "]: " << pContent << llendl;
	mTerrainMessagingState = kReceivedError;
	if (mObjectMessagingState != kWaiting)
	{
		sendCallback();
	}
}

void LinksetsResponder::sendCallback()
{
	llassert(mObjectMessagingState != kWaiting);
	llassert(mTerrainMessagingState != kWaiting);
	LLPathfindingManager::ERequestStatus requestStatus =
		((((mObjectMessagingState == kReceivedGood) || (mObjectMessagingState == kNotRequested)) &&
		  ((mTerrainMessagingState == kReceivedGood) || (mTerrainMessagingState == kNotRequested))) ?
		 LLPathfindingManager::kRequestCompleted : LLPathfindingManager::kRequestError);

	if (mObjectMessagingState != kReceivedGood)
	{
		mObjectLinksetListPtr = LLPathfindingObjectListPtr(new LLPathfindingLinksetList());
	}

	if (mTerrainMessagingState == kReceivedGood)
	{
		mObjectLinksetListPtr->update(mTerrainLinksetPtr);
	}

	mLinksetsCallback(mRequestId, requestStatus, mObjectLinksetListPtr);
}

//---------------------------------------------------------------------------
// ObjectLinksetsResponder
//---------------------------------------------------------------------------

ObjectLinksetsResponder::ObjectLinksetsResponder(const std::string &pCapabilityURL, LinksetsResponderPtr pLinksetsResponsderPtr)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mLinksetsResponsderPtr(pLinksetsResponsderPtr)
{
}

ObjectLinksetsResponder::~ObjectLinksetsResponder()
{
}

void ObjectLinksetsResponder::result(const LLSD &pContent)
{
	mLinksetsResponsderPtr->handleObjectLinksetsResult(pContent);
}

void ObjectLinksetsResponder::errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent)
{
	mLinksetsResponsderPtr->handleObjectLinksetsError(pStatus, pReason, pContent, mCapabilityURL);
}

//---------------------------------------------------------------------------
// TerrainLinksetsResponder
//---------------------------------------------------------------------------

TerrainLinksetsResponder::TerrainLinksetsResponder(const std::string &pCapabilityURL, LinksetsResponderPtr pLinksetsResponsderPtr)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mLinksetsResponsderPtr(pLinksetsResponsderPtr)
{
}

TerrainLinksetsResponder::~TerrainLinksetsResponder()
{
}

void TerrainLinksetsResponder::result(const LLSD &pContent)
{
	mLinksetsResponsderPtr->handleTerrainLinksetsResult(pContent);
}

void TerrainLinksetsResponder::errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent)
{
	mLinksetsResponsderPtr->handleTerrainLinksetsError(pStatus, pReason, pContent, mCapabilityURL);
}

//---------------------------------------------------------------------------
// CharactersResponder
//---------------------------------------------------------------------------

CharactersResponder::CharactersResponder(const std::string &pCapabilityURL, LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::object_request_callback_t pCharactersCallback)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mRequestId(pRequestId),
	mCharactersCallback(pCharactersCallback)
{
}

CharactersResponder::~CharactersResponder()
{
}

void CharactersResponder::result(const LLSD &pContent)
{
	LLPathfindingObjectListPtr characterListPtr = LLPathfindingObjectListPtr(new LLPathfindingCharacterList(pContent));
	mCharactersCallback(mRequestId, LLPathfindingManager::kRequestCompleted, characterListPtr);
}

void CharactersResponder::errorWithContent(U32 pStatus, const std::string &pReason, const LLSD& pContent)
{
	llwarns << "CharactersResponder error [status:" << pStatus << "]: " << pContent << llendl;

	LLPathfindingObjectListPtr characterListPtr =  LLPathfindingObjectListPtr(new LLPathfindingCharacterList());
	mCharactersCallback(mRequestId, LLPathfindingManager::kRequestError, characterListPtr);
}
