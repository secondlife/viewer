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

#include <boost/function.hpp>

#include "llsingleton.h"

class LLFloater;

class LLPathfindingManager : public LLSingleton<LLPathfindingManager>
{
public:
	typedef enum {
		kAgentStateNotEnabled     = 0,
		kAgentStateFrozen         = 1,
		kAgentStateUnfrozen       = 2,
		kAgentStateError          = 3,
		kAgentStateInitialDefault = kAgentStateUnfrozen
	} EAgentState;

	typedef boost::function<void (EAgentState pAgentState)> agent_state_callback_t;

	LLPathfindingManager();
	virtual ~LLPathfindingManager();

	bool isPathfindingEnabledForCurrentRegion() const;

	void requestGetAgentState(agent_state_callback_t pAgentStateCB) const;
	void requestSetAgentState(EAgentState, agent_state_callback_t pAgentStateCB) const;

	void handleAgentStateResult(const LLSD &pContent, agent_state_callback_t pAgentStateCB) const;
	void handleAgentStateError(U32 pStatus, const std::string &pReason, const std::string &pURL, agent_state_callback_t pAgentStateCB) const;

protected:

private:
	std::string getRetrieveNavMeshURLForCurrentRegion() const;
	std::string getAgentStateURLForCurrentRegion() const;

	std::string getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const;
};

#endif // LL_LLPATHFINDINGMANAGER_H
