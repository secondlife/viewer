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

#define CAP_SERVICE_LINKSETS          "ObjectNavMeshProperties"

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

class LinksetsResponder : public LLHTTPClient::Responder
{
public:
	LinksetsResponder(const std::string &pCapabilityURL, LLPathfindingManager::linksets_callback_t pLinksetsCallback);
	virtual ~LinksetsResponder();

	virtual void result(const LLSD &pContent);
	virtual void error(U32 pStatus, const std::string &pReason);

protected:

private:
	std::string                               mCapabilityURL;
	LLPathfindingManager::linksets_callback_t mLinksetsCallback;
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

	std::string linksetsURL = getLinksetsURLForCurrentRegion();
	if (linksetsURL.empty())
	{;
		status = kLinksetsRequestNotEnabled;
	}
	else
	{
		LLHTTPClient::ResponderPtr responder = new LinksetsResponder(linksetsURL, pLinksetsCallback);
		LLHTTPClient::get(linksetsURL, responder);
		status = kLinksetsRequestStarted;
	}

	return status;
}

LLPathfindingManager::ELinksetsRequestStatus LLPathfindingManager::requestSetLinksets(LLPathfindingLinksetListPtr pLinksetList, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD, linksets_callback_t pLinksetsCallback) const
{
	ELinksetsRequestStatus status;

	std::string linksetsURL = getLinksetsURLForCurrentRegion();
	if (linksetsURL.empty())
	{
		status = kLinksetsRequestNotEnabled;
	}
	else
	{
		LLHTTPClient::ResponderPtr responder = new LinksetsResponder(linksetsURL, pLinksetsCallback);
		LLSD postData = pLinksetList->encodeAlteredFields(pLinksetUse, pA, pB, pC, pD);
		LLHTTPClient::put(linksetsURL, postData, responder);
		status = kLinksetsRequestStarted;
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

void LLPathfindingManager::handleLinksetsResult(const LLSD &pContent, linksets_callback_t pLinksetsCallback) const
{
	LLPathfindingLinksetListPtr linksetListPtr(new LLPathfindingLinksetList(pContent));
	pLinksetsCallback(kLinksetsRequestCompleted, linksetListPtr);
}

void LLPathfindingManager::handleLinksetsError(U32 pStatus, const std::string &pReason, const std::string &pURL, linksets_callback_t pLinksetsCallback) const
{
	llwarns << "error with request to URL '" << pURL << "' because " << pReason << " (statusCode:" << pStatus << ")" << llendl;
	LLPathfindingLinksetListPtr linksetListPtr(new LLPathfindingLinksetList());
	pLinksetsCallback(kLinksetsRequestError, linksetListPtr);
}

std::string LLPathfindingManager::getRetrieveNavMeshURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_RETRIEVE_NAVMESH);
}

std::string LLPathfindingManager::getAgentStateURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_AGENT_STATE);
}

std::string LLPathfindingManager::getLinksetsURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_LINKSETS);
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

LinksetsResponder::LinksetsResponder(const std::string &pCapabilityURL, LLPathfindingManager::linksets_callback_t pLinksetsCallback)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mLinksetsCallback(pLinksetsCallback)
{
}

LinksetsResponder::~LinksetsResponder()
{
}

void LinksetsResponder::result(const LLSD &pContent)
{
	LLPathfindingManager::getInstance()->handleLinksetsResult(pContent, mLinksetsCallback);
}

void LinksetsResponder::error(U32 pStatus, const std::string &pReason)
{
	LLPathfindingManager::getInstance()->handleLinksetsError(pStatus, pReason, mCapabilityURL, mLinksetsCallback);
}
