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
#include <boost/signals2.hpp>

#include "llsingleton.h"

class LLFloater;

class LLPathfindingManager : public LLSingleton<LLPathfindingManager>
{
	friend class AgentStateResponder;
public:
	typedef enum {
		kAgentStateUnknown,
		kAgentStateFrozen,
		kAgentStateUnfrozen,
		kAgentStateNotEnabled,
		kAgentStateError
	} EAgentState;

	typedef boost::function<void (EAgentState pAgentState)>         agent_state_callback_t;
	typedef boost::signals2::signal<void (EAgentState pAgentState)> agent_state_signal_t;
	typedef boost::signals2::connection                             agent_state_slot_t;

	LLPathfindingManager();
	virtual ~LLPathfindingManager();

	bool isPathfindingEnabledForCurrentRegion() const;

	agent_state_slot_t registerAgentStateSignal(agent_state_callback_t pAgentStateCallback);
	EAgentState        getAgentState();
	EAgentState        getLastKnownNonErrorAgentState() const;
	void               requestSetAgentState(EAgentState pAgentState);

protected:

private:
	static bool isValidAgentState(EAgentState pAgentState);

	void requestGetAgentState();
	void setAgentState(EAgentState pAgentState);

	void handleAgentStateResult(const LLSD &pContent, EAgentState pRequestedAgentState);
	void handleAgentStateError(U32 pStatus, const std::string &pReason, const std::string &pURL);

	std::string getRetrieveNavMeshURLForCurrentRegion() const;
	std::string getAgentStateURLForCurrentRegion() const;

	std::string getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const;

	agent_state_signal_t mAgentStateSignal;
	EAgentState          mAgentState;
	EAgentState          mLastKnownNonErrorAgentState;
};

#endif // LL_LLPATHFINDINGMANAGER_H
