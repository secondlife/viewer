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

#include <boost/function.hpp>

#include "llviewerprecompiledheaders.h"
#include "llpathfindingmanager.h"
#include "llsingleton.h"
#include "llhttpclient.h"
#include "llagent.h"
#include "llviewerregion.h"

#define CAP_SERVICE_RETRIEVE_NAVMESH  "RetrieveNavMeshSrc"

#define CAP_SERVICE_AGENT_STATE       "AgentPreferences"
#define ALTER_PERMANENT_OBJECTS_FIELD "alter_permanent_objects"

//---------------------------------------------------------------------------
// AgentStateResponder
//---------------------------------------------------------------------------

class AgentStateResponder : public LLHTTPClient::Responder
{
public:
	AgentStateResponder(LLPathfindingManager::agent_state_callback_t pAgentStateCB, const std::string &pCapabilityURL);
	virtual ~AgentStateResponder();

	virtual void result(const LLSD &pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

protected:

private:
	LLPathfindingManager::agent_state_callback_t mAgentStateCB;
	std::string                                  mCapabilityURL;
};

//---------------------------------------------------------------------------
// LLPathfindingManager
//---------------------------------------------------------------------------

LLPathfindingManager::LLPathfindingManager()
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

void LLPathfindingManager::requestGetAgentState(agent_state_callback_t pAgentStateCB) const
{
	std::string agentStateURL = getAgentStateURLForCurrentRegion();

	if (agentStateURL.empty())
	{
		pAgentStateCB(kAgentStateError);
	}
	else
	{
		LLHTTPClient::ResponderPtr responder = new AgentStateResponder(pAgentStateCB, agentStateURL);
		LLHTTPClient::get(agentStateURL, responder);
	}
}

void LLPathfindingManager::requestSetAgentState(EAgentState pAgentState, agent_state_callback_t pAgentStateCB) const
{
	std::string agentStateURL = getAgentStateURLForCurrentRegion();

	if (agentStateURL.empty())
	{
		pAgentStateCB(kAgentStateError);
	}
	else
	{
		LLSD request;
		request[ALTER_PERMANENT_OBJECTS_FIELD] = static_cast<LLSD::Boolean>(pAgentState == kAgentStateUnfrozen);

		LLHTTPClient::ResponderPtr responder = new AgentStateResponder(pAgentStateCB, agentStateURL);
		LLHTTPClient::post(agentStateURL, request, responder);
	}
}

void LLPathfindingManager::handleAgentStateResult(const LLSD &pContent, agent_state_callback_t pAgentStateCB) const
{
	llassert(pContent.has(ALTER_PERMANENT_OBJECTS_FIELD));
	llassert(pContent.get(ALTER_PERMANENT_OBJECTS_FIELD).isBoolean());
	EAgentState agentState = (pContent.get(ALTER_PERMANENT_OBJECTS_FIELD).asBoolean() ? kAgentStateUnfrozen : kAgentStateFrozen);

	pAgentStateCB(agentState);
}

void LLPathfindingManager::handleAgentStateError(U32 pStatus, const std::string &pReason, const std::string &pURL, agent_state_callback_t pAgentStateCB) const
{
	llwarns << "error with request to URL '" << pURL << "' because " << pReason << " (statusCode:" << pStatus << ")" << llendl;
	pAgentStateCB(kAgentStateError);
}

std::string LLPathfindingManager::getRetrieveNavMeshURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_RETRIEVE_NAVMESH);
}

std::string LLPathfindingManager::getAgentStateURLForCurrentRegion() const
{
	return getCapabilityURLForCurrentRegion(CAP_SERVICE_AGENT_STATE);
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

AgentStateResponder::AgentStateResponder(LLPathfindingManager::agent_state_callback_t pAgentStateCB, const std::string &pCapabilityURL)
	: mAgentStateCB(pAgentStateCB),
	mCapabilityURL(pCapabilityURL)
{
}

AgentStateResponder::~AgentStateResponder()
{
}

void AgentStateResponder::result(const LLSD &pContent)
{
	LLPathfindingManager::getInstance()->handleAgentStateResult(pContent, mAgentStateCB);
}

void AgentStateResponder::error(U32 pStatus, const std::string &pReason)
{
	LLPathfindingManager::getInstance()->handleAgentStateError(pStatus, pReason, mCapabilityURL, mAgentStateCB);
}
