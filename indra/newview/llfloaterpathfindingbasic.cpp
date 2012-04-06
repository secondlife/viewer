/** 
* @file llfloaterpathfindingbasic.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterpathfindingbasic.h"
#include "llsd.h"
#include "lltextbase.h"
#include "llbutton.h"
#include "llpathfindingmanager.h"

#include <boost/bind.hpp>

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

void LLFloaterPathfindingBasic::onOpen(const LLSD& pKey)
{
	LLFloater::onOpen(pKey);

	if (!mAgentStateSlot.connected())
	{
		mAgentStateSlot = LLPathfindingManager::getInstance()->registerAgentStateListener(boost::bind(&LLFloaterPathfindingBasic::onAgentStateCB, this, _1));
	}
	setAgentState(LLPathfindingManager::getInstance()->getAgentState());
}

void LLFloaterPathfindingBasic::onClose(bool pIsAppQuitting)
{
	if (mAgentStateSlot.connected())
	{
		mAgentStateSlot.disconnect();
	}

	LLFloater::onClose(pIsAppQuitting);
}

LLFloaterPathfindingBasic::LLFloaterPathfindingBasic(const LLSD& pSeed)
	: LLFloater(pSeed),
	mStatusText(NULL),
	mUnfreezeLabel(NULL),
	mUnfreezeButton(NULL),
	mFreezeLabel(NULL),
	mFreezeButton(NULL),
	mAgentStateSlot()
{
}

LLFloaterPathfindingBasic::~LLFloaterPathfindingBasic()
{
}

void LLFloaterPathfindingBasic::onUnfreezeClicked()
{
	mUnfreezeButton->setEnabled(FALSE);
	LLPathfindingManager::getInstance()->requestSetAgentState(LLPathfindingManager::kAgentStateUnfrozen);
}

void LLFloaterPathfindingBasic::onFreezeClicked()
{
	mUnfreezeButton->setEnabled(FALSE);
	LLPathfindingManager::getInstance()->requestSetAgentState(LLPathfindingManager::kAgentStateFrozen);
}

void LLFloaterPathfindingBasic::onAgentStateCB(LLPathfindingManager::EAgentState pAgentState)
{
	setAgentState(pAgentState);
}

void LLFloaterPathfindingBasic::setAgentState(LLPathfindingManager::EAgentState pAgentState)
{
	static const LLColor4 errorColor = LLUIColorTable::instance().getColor("PathfindingErrorColor");
	LLStyle::Params styleParams;

	switch (pAgentState)
	{
	case LLPathfindingManager::kAgentStateUnknown : 
		mStatusText->setVisible(TRUE);
		mStatusText->setText((LLStringExplicit)getString("status_querying_state"), styleParams);
		break;
	case LLPathfindingManager::kAgentStateNotEnabled : 
		mStatusText->setVisible(TRUE);
		styleParams.color = errorColor;
		mStatusText->setText((LLStringExplicit)getString("status_pathfinding_not_enabled"), styleParams);
		break;
	case LLPathfindingManager::kAgentStateError : 
		mStatusText->setVisible(TRUE);
		styleParams.color = errorColor;
		mStatusText->setText((LLStringExplicit)getString("status_unable_to_change_state"), styleParams);
		break;
	default :
		mStatusText->setVisible(FALSE);
		break;
	}

	switch (LLPathfindingManager::getInstance()->getLastKnownNonErrorAgentState())
	{
	case LLPathfindingManager::kAgentStateUnknown : 
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
	default :
		llassert(0);
		break;
	}
}
