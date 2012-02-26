/** 
 * @file llpathfindingmanager.cpp
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

#include <string>

#include "llviewerprecompiledheaders.h"
#include "llpathfindingmanager.h"
#include "llsingleton.h"
#include "llhttpclient.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#define CAP_SERVICE_RETRIEVE_NAVMESH  "RetrieveNavMeshSrc"

#define CAP_SERVICE_AGENT_STATE       "AgentPreferences"
#define ALTER_PERMANENT_OBJECTS_FIELD "alter_permanent_objects"

#define CAP_SERVICE_OBJECT_LINKSETS   "ObjectNavMeshProperties"
#define CAP_SERVICE_TERRAIN_LINKSETS  "TerrainNavMeshProperties"

//---------------------------------------------------------------------------
// AgentStateResponder
//---------------------------------------------------------------------------

class AgentStateResponder : public LLHTTPClient::Responder
{
public:
	AgentStateResponder(const std::string &pCapabilityURL, LLPathfindingManager::EAgentState pRequestedAgentState = LLPathfindingManager::kAgentStateUnknown);
	virtual ~AgentStateResponder();

	virtual void result(const LLSD &pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

protected:

private:
	std::string                       mCapabilityURL;
	LLPathfindingManager::EAgentState mRequestedAgentState;
};

//---------------------------------------------------------------------------
// LinksetsResponder
//---------------------------------------------------------------------------

class LinksetsResponder
{
public:
	LinksetsResponder(LLPathfindingManager::linksets_callback_t pLinksetsCallback, bool pIsObjectRequested, bool pIsTerrainRequested);
	virtual ~LinksetsResponder();
	
	void handleObjectLinksetsResult(const LLSD &pContent);
	void handleObjectLinksetsError(U32 pStatus, const std::string &pReason, const std::string &pURL);
	void handleTerrainLinksetsResult(const LLSD &pContent);
	void handleTerrainLinksetsError(U32 pStatus, const std::string &pReason, const std::string &pURL);
	
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
	
	LLPathfindingManager::linksets_callback_t mLinksetsCallback;
	
	EMessagingState                           mObjectMessagingState;
	EMessagingState                           mTerrainMessagingState;
	
	LLPathfindingLinksetListPtr               mObjectLinksetListPtr;
	LLPathfindingLinksetPtr                   mTerrainLinksetPtr;
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
	virtual void error(U32 pStatus, const std::string &pReason);
	
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
	virtual void error(U32 pStatus, const std::string &pReason);
	
protected:
	
private:
	std::string          mCapabilityURL;
	LinksetsResponderPtr mLinksetsResponsderPtr;
};

//---------------------------------------------------------------------------
// LLPathfindingManager
//---------------------------------------------------------------------------

LLPathfindingManager::LLPathfindingManager()
	: LLSingleton<LLPathfindingManager>(),
	mAgentStateSignal(),
	mAgentState(kAgentStateUnknown),
	mLastKnownNonErrorAgentState(kAgentStateUnknown)
{
}

LLPathfindingManager::~LLPathfindingManager()
{
}

bool LLPathfindingManager::isPathfindingEnabledForCurrentRegion() const
{
	std::string retrieveNavMeshURL = getRetrieveNavMeshURLForCurrentRegion();
	return !retrieveNavMeshURL.empty();
}

bool LLPathfindingManager::isAllowAlterPermanent()
{
	return (!isPathfindingEnabledForCurrentRegion() || (getAgentState() == kAgentStateUnfrozen));
}

LLPathfindingManager::agent_state_slot_t LLPathfindingManager::registerAgentStateSignal(agent_state_callback_t pAgentStateCallback)
{
	return mAgentStateSignal.connect(pAgentStateCallback);
}

LLPathfindingManager::EAgentState LLPathfindingManager::getAgentState()
{
	if (!isPathfindingEnabledForCurrentRegion())
	{
		setAgentState(kAgentStateNotEnabled);
	}
	else
	{
		if (!isValidAgentState(mAgentState))
		{
			requestGetAgentState();
		}
	}

	return mAgentState;
}

LLPathfindingManager::EAgentState LLPathfindingManager::getLastKnownNonErrorAgentState() const
{
	return mLastKnownNonErrorAgentState;
}

void LLPathfindingManager::requestSetAgentState(EAgentState pRequestedAgentState)
{
	llassert(isValidAgentState(pRequestedAgentState));
	std::string agentStateURL = getAgentStateURLForCurrentRegion();

	if (agentStateURL.empty())
	{
		setAgentState(kAgentStateNotEnabled);
	}
	else
	{
		LLSD request;
		request[ALTER_PERMANENT_OBJECTS_FIELD] = static_cast<LLSD::Boolean>(pRequestedAgentState == kAgentStateUnfrozen);

		LLHTTPClient::ResponderPtr responder = new AgentStateResponder(agentStateURL, pRequestedAgentState);
		LLHTTPClient::post(agentStateURL, request, responder);
	}
}

LLPathfindingManager::ELinksetsRequestStatus LLPathfindingManager::requestGetLinksets(linksets_callback_t pLinksetsCallback) const
{
	ELinksetsRequestStatus status;

	std::string objectLinksetsURL = getObjectLinksetsURLForCurrentRegion();
	std::string terrainLinksetsURL = getTerrainLinksetsURLForCurrentRegion();
	if (objectLinksetsURL.empty() || terrainLinksetsURL.empty())
	{
		status = kLinksetsRequestNotEnabled;
	}
	else
	{
		LinksetsResponderPtr linksetsResponderPtr(new LinksetsResponder(pLinksetsCallback, true, true));
		
		LLHTTPClient::ResponderPtr objectLinksetsResponder = new ObjectLinksetsResponder(objectLinksetsURL, linksetsResponderPtr);
		LLHTTPClient::ResponderPtr terrainLinksetsResponder = new TerrainLinksetsResponder(terrainLinksetsURL, linksetsResponderPtr);
		
		LLHTTPClient::get(objectLinksetsURL, objectLinksetsResponder);
		LLHTTPClient::get(terrainLinksetsURL, terrainLinksetsResponder);
		status = kLinksetsRequestStarted;
	}

	return status;
}

LLPathfindingManager::ELinksetsRequestStatus LLPathfindingManager::requestSetLinksets(LLPathfindingLinksetListPtr pLinksetList, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD, linksets_callback_t pLinksetsCallback) const
{
	ELinksetsRequestStatus status = kLinksetsRequestNotEnabled;

	std::string objectLinksetsURL = getObjectLinksetsURLForCurrentRegion();
	std::string terrainLinksetsURL = getTerrainLinksetsURLForCurrentRegion();
	if (objectLinksetsURL.empty() || terrainLinksetsURL.empty())
	{
		status = kLinksetsRequestNotEnabled;
	}
	else
	{
		LLSD objectPostData = pLinksetList->encodeObjectFields(pLinksetUse, pA, pB, pC, pD);
		LLSD terrainPostData = pLinksetList->encodeTerrainFields(pLinksetUse, pA, pB, pC, pD);
		if (objectPostData.isUndefined() && terrainPostData.isUndefined())
		{
			status = kLinksetsRequestCompleted;
		}
		else
		{
			LinksetsResponderPtr linksetsResponderPtr(new LinksetsResponder(pLinksetsCallback, !objectPostData.isUndefined(), !terrainPostData.isUndefined()));

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

			status = kLinksetsRequestStarted;
		}
	}

	return status;
}

bool LLPathfindingManager::isValidAgentState(EAgentState pAgentState)
{
	return ((pAgentState == kAgentStateFrozen) || (pAgentState == kAgentStateUnfrozen));
}

void LLPathfindingManager::requestGetAgentState()
{
	std::string agentStateURL = getAgentStateURLForCurrentRegion();

	if (agentStateURL.empty())
	{
		setAgentState(kAgentStateNotEnabled);
	}
	else
	{
		LLHTTPClient::ResponderPtr responder = new AgentStateResponder(agentStateURL);
		LLHTTPClient::get(agentStateURL, responder);
	}
}

void LLPathfindingManager::setAgentState(EAgentState pAgentState)
{
	mAgentState = pAgentState;

	if (mAgentState != kAgentStateError)
	{
		mLastKnownNonErrorAgentState = mAgentState;
	}

	mAgentStateSignal(mAgentState);
}

void LLPathfindingManager::handleAgentStateResult(const LLSD &pContent, EAgentState pRequestedAgentState)
{
	llassert(pContent.has(ALTER_PERMANENT_OBJECTS_FIELD));
	llassert(pContent.get(ALTER_PERMANENT_OBJECTS_FIELD).isBoolean());
	EAgentState agentState = (pContent.get(ALTER_PERMANENT_OBJECTS_FIELD).asBoolean() ? kAgentStateUnfrozen : kAgentStateFrozen);

	if (isValidAgentState(pRequestedAgentState) && (agentState != pRequestedAgentState))
	{
		agentState = kAgentStateError;
		llassert(0);
	}

	setAgentState(agentState);
}

void LLPathfindingManager::handleAgentStateError(U32 pStatus, const std::string &pReason, const std::string &pURL)
{
	llwarns << "error with request to URL '" << pURL << "' because " << pReason << " (statusCode:" << pStatus << ")" << llendl;
	setAgentState(kAgentStateError);
}

std::string LLPathfindingManager::getRetrieveNavMeshURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_RETRIEVE_NAVMESH);
}

std::string LLPathfindingManager::getAgentStateURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_AGENT_STATE);
}

std::string LLPathfindingManager::getObjectLinksetsURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_OBJECT_LINKSETS);
}

std::string LLPathfindingManager::getTerrainLinksetsURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_TERRAIN_LINKSETS);
}

std::string LLPathfindingManager::getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const
{
	std::string capabilityURL("");

	LLViewerRegion* region = gAgent.getRegion();
	if (region != NULL)
	{
		capabilityURL = region->getCapability(pCapabilityName);
	}

	if (capabilityURL.empty())
	{
		llwarns << "cannot find capability '" << pCapabilityName << "' for current region '"
			<< ((region != NULL) ? region->getName() : "<null>") << "'" << llendl;
	}

	return capabilityURL;
}

//---------------------------------------------------------------------------
// AgentStateResponder
//---------------------------------------------------------------------------

AgentStateResponder::AgentStateResponder(const std::string &pCapabilityURL, LLPathfindingManager::EAgentState pRequestedAgentState)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mRequestedAgentState(pRequestedAgentState)
{
}

AgentStateResponder::~AgentStateResponder()
{
}

void AgentStateResponder::result(const LLSD &pContent)
{
	LLPathfindingManager::getInstance()->handleAgentStateResult(pContent, mRequestedAgentState);
}

void AgentStateResponder::error(U32 pStatus, const std::string &pReason)
{
	LLPathfindingManager::getInstance()->handleAgentStateError(pStatus, pReason, mCapabilityURL);
}

//---------------------------------------------------------------------------
// LinksetsResponder
//---------------------------------------------------------------------------

LinksetsResponder::LinksetsResponder(LLPathfindingManager::linksets_callback_t pLinksetsCallback, bool pIsObjectRequested, bool pIsTerrainRequested)
	: mLinksetsCallback(pLinksetsCallback),
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
	mObjectLinksetListPtr = LLPathfindingLinksetListPtr(new LLPathfindingLinksetList(pContent));

	mObjectMessagingState = kReceivedGood;
	if (mTerrainMessagingState != kWaiting)
	{
		sendCallback();
	}
}

void LinksetsResponder::handleObjectLinksetsError(U32 pStatus, const std::string &pReason, const std::string &pURL)
{
	llwarns << "error with request to URL '" << pURL << "' because " << pReason << " (statusCode:" << pStatus << ")" << llendl;
	mObjectMessagingState = kReceivedError;
	if (mTerrainMessagingState != kWaiting)
	{
		sendCallback();
	}
}

void LinksetsResponder::handleTerrainLinksetsResult(const LLSD &pContent)
{
	mTerrainLinksetPtr = LLPathfindingLinksetPtr(new LLPathfindingLinkset(pContent));

	mTerrainMessagingState = kReceivedGood;
	if (mObjectMessagingState != kWaiting)
	{
		sendCallback();
	}
}

void LinksetsResponder::handleTerrainLinksetsError(U32 pStatus, const std::string &pReason, const std::string &pURL)
{
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
	LLPathfindingManager::ELinksetsRequestStatus requestStatus =
		((((mObjectMessagingState == kReceivedGood) || (mObjectMessagingState == kNotRequested)) &&
		  ((mTerrainMessagingState == kReceivedGood) || (mTerrainMessagingState == kNotRequested))) ?
		 LLPathfindingManager::kLinksetsRequestCompleted : LLPathfindingManager::kLinksetsRequestError);

	if (mObjectMessagingState != kReceivedGood)
	{
		mObjectLinksetListPtr = LLPathfindingLinksetListPtr(new LLPathfindingLinksetList());
	}

	if (mTerrainMessagingState == kReceivedGood)
	{
		mObjectLinksetListPtr->insert(std::pair<std::string, LLPathfindingLinksetPtr>(mTerrainLinksetPtr->getUUID().asString(), mTerrainLinksetPtr));
	}

	mLinksetsCallback(requestStatus, mObjectLinksetListPtr);
}

//---------------------------------------------------------------------------
// ObjectLinksetsResponder
//---------------------------------------------------------------------------

ObjectLinksetsResponder::ObjectLinksetsResponder(const std::string &pCapabilityURL, LinksetsResponderPtr pLinksetsResponsderPtr)
	: mCapabilityURL(pCapabilityURL),
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

void ObjectLinksetsResponder::error(U32 pStatus, const std::string &pReason)
{
	mLinksetsResponsderPtr->handleObjectLinksetsError(pStatus, pReason, mCapabilityURL);
}
	
//---------------------------------------------------------------------------
// TerrainLinksetsResponder
//---------------------------------------------------------------------------

TerrainLinksetsResponder::TerrainLinksetsResponder(const std::string &pCapabilityURL, LinksetsResponderPtr pLinksetsResponsderPtr)
: mCapabilityURL(pCapabilityURL),
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

void TerrainLinksetsResponder::error(U32 pStatus, const std::string &pReason)
{
	mLinksetsResponsderPtr->handleTerrainLinksetsError(pStatus, pReason, mCapabilityURL);
}
