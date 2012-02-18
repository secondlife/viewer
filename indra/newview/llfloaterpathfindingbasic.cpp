/** 
* @file llfloaterpathfindingbasic.cpp
* @author William Todd Stinson
* @brief "Pathfinding basic" floater, allowing for basic freezing and unfreezing of the pathfinding avator mode.
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

#include <boost/bind.hpp>

#include "llviewerprecompiledheaders.h"
#include "llfloaterpathfindingbasic.h"
#include "llsd.h"
#include "lltextbase.h"
#include "llbutton.h"
#include "llpathfindingmanager.h"

//---------------------------------------------------------------------------
// LLFloaterPathfindingBasic
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingBasic::postBuild()
{
	mStatusText = findChild<LLTextBase>("status_label");
	llassert(mStatusText != NULL);

	mUnfreezeLabel = findChild<LLTextBase>("unfreeze_label");
	llassert(mUnfreezeLabel != NULL);

	mUnfreezeButton = findChild<LLButton>("enter_unfrozen_mode");
	llassert(mUnfreezeButton != NULL);
	mUnfreezeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingBasic::onUnfreezeClicked, this));

	mFreezeLabel = findChild<LLTextBase>("freeze_label");
	llassert(mFreezeLabel != NULL);

	mFreezeButton = findChild<LLButton>("enter_frozen_mode");
	llassert(mFreezeButton != NULL);
	mFreezeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingBasic::onFreezeClicked, this));

	return LLFloater::postBuild();
}

void LLFloaterPathfindingBasic::onOpen(const LLSD& key)
{
	LLPathfindingManager *pathfindingManager = LLPathfindingManager::getInstance();
	if (pathfindingManager->isPathfindingEnabledForCurrentRegion())
	{
		LLPathfindingManager::getInstance()->requestGetAgentState(boost::bind(&LLFloaterPathfindingBasic::onAgentStateCB, this, _1));
	}
	else
	{
		setAgentState(LLPathfindingManager::kAgentStateNotEnabled);
	}

	LLFloater::onOpen(key);
}

LLFloaterPathfindingBasic::LLFloaterPathfindingBasic(const LLSD& pSeed)
	: LLFloater(pSeed),
	mStatusText(NULL),
	mUnfreezeLabel(NULL),
	mUnfreezeButton(NULL),
	mFreezeLabel(NULL),
	mFreezeButton(NULL),
	mAgentState(LLPathfindingManager::kAgentStateInitialDefault)
{
}

LLFloaterPathfindingBasic::~LLFloaterPathfindingBasic()
{
}

void LLFloaterPathfindingBasic::onUnfreezeClicked()
{
	mUnfreezeButton->setEnabled(FALSE);
	LLPathfindingManager::getInstance()->requestSetAgentState(LLPathfindingManager::kAgentStateUnfrozen, boost::bind(&LLFloaterPathfindingBasic::onAgentStateCB, this, _1));
}

void LLFloaterPathfindingBasic::onFreezeClicked()
{
	mUnfreezeButton->setEnabled(FALSE);
	LLPathfindingManager::getInstance()->requestSetAgentState(LLPathfindingManager::kAgentStateFrozen, boost::bind(&LLFloaterPathfindingBasic::onAgentStateCB, this, _1));
}

void LLFloaterPathfindingBasic::onAgentStateCB(LLPathfindingManager::EAgentState pAgentState)
{
	setAgentState(pAgentState);
}

void LLFloaterPathfindingBasic::setAgentState(LLPathfindingManager::EAgentState pAgentState)
{
	switch (pAgentState)
	{
	case LLPathfindingManager::kAgentStateNotEnabled : 
		mStatusText->setVisible(TRUE);
		mStatusText->setText((LLStringExplicit)getString("pathfinding_not_enabled"));
		mAgentState = pAgentState;
		break;
	case LLPathfindingManager::kAgentStateError : 
		mStatusText->setVisible(TRUE);
		mStatusText->setText((LLStringExplicit)getString("unable_to_change_state"));
		// Do not actually change the current state in the error case allowing user to retry previous command
		break;
	default :
		mStatusText->setVisible(FALSE);
		mAgentState = pAgentState;
		break;
	}

	switch (mAgentState)
	{
	case LLPathfindingManager::kAgentStateNotEnabled : 
		mUnfreezeLabel->setEnabled(FALSE);
		mUnfreezeButton->setEnabled(FALSE);
		mFreezeLabel->setEnabled(FALSE);
		mFreezeButton->setEnabled(FALSE);
		break;
	case LLPathfindingManager::kAgentStateFrozen : 
		mUnfreezeLabel->setEnabled(TRUE);
		mUnfreezeButton->setEnabled(TRUE);
		mFreezeLabel->setEnabled(FALSE);
		mFreezeButton->setEnabled(FALSE);
		break;
	case LLPathfindingManager::kAgentStateUnfrozen : 
		mUnfreezeLabel->setEnabled(FALSE);
		mUnfreezeButton->setEnabled(FALSE);
		mFreezeLabel->setEnabled(TRUE);
		mFreezeButton->setEnabled(TRUE);
		break;
	case LLPathfindingManager::kAgentStateError : 
		llassert(0);
		break;
	default :
		llassert(0);
		break;
	}
}
