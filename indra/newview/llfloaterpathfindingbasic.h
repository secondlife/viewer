/** 
 * @file llfloaterpathfindingbasic.h
 * @author William Todd Stinson
 * @brief "Pathfinding basic" floater, allowing for basic freezing and unfreezing of the pathfinding avatar mode.
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

#ifndef LL_LLFLOATERPATHFINDINGBASIC_H
#define LL_LLFLOATERPATHFINDINGBASIC_H

#include "llfloater.h"
#include "llpathfindingmanager.h"

class LLSD;
class LLTextBase;
class LLButton;

class LLFloaterPathfindingBasic
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool pIsAppQuitting);

protected:

private:
	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingBasic(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingBasic();

	void onUnfreezeClicked();
	void onFreezeClicked();

	void onAgentStateCB(LLPathfindingManager::EAgentState pAgentState);

	void setAgentState(LLPathfindingManager::EAgentState pAgentState);

	LLTextBase                               *mStatusText;
	LLTextBase                               *mUnfreezeLabel;
	LLButton                                 *mUnfreezeButton;
	LLTextBase                               *mFreezeLabel;
	LLButton                                 *mFreezeButton;
	LLPathfindingManager::agent_state_slot_t mAgentStateSlot;
};

#endif // LL_LLFLOATERPATHFINDINGBASIC_H
