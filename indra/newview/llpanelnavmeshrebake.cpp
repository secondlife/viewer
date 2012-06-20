/**
 * @file LLPanelNavMeshRebake.cpp
 * @author prep
 * @brief handles the buttons for navmesh rebaking
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

#include "llpanelnavmeshrebake.h"

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llbutton.h"
#include "llenvmanager.h"
#include "llhandle.h"
#include "llhints.h"
#include "llpanel.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "lltoolbar.h"
#include "lltoolbarview.h"
#include "lltoolmgr.h"
#include "lltooltip.h"
#include "llview.h"
#include "llviewerregion.h"

LLPanelNavMeshRebake* LLPanelNavMeshRebake::getInstance()
{
	static LLPanelNavMeshRebake* panel = getPanel();
	return panel;
}

BOOL LLPanelNavMeshRebake::postBuild()
{
	//Rebake initiated
	mNavMeshRebakeButton = findChild<LLButton>("navmesh_btn");
	llassert(mNavMeshRebakeButton != NULL);
	mNavMeshRebakeButton->setCommitCallback(boost::bind(&LLPanelNavMeshRebake::onNavMeshRebakeClick, this));
	LLHints::registerHintTarget("navmesh_btn", mNavMeshRebakeButton->getHandle());
	
	//Baking...
	mNavMeshBakingButton = findChild<LLButton>("navmesh_btn_baking");
	llassert(mNavMeshBakingButton != NULL);
	LLHints::registerHintTarget("navmesh_btn_baking", mNavMeshBakingButton->getHandle());

	setMode(kRebakeNavMesh_Default);

	createNavMeshStatusListenerForCurrentRegion();

	if ( !mRegionCrossingSlot.connected() )
	{
		mRegionCrossingSlot = LLEnvManagerNew::getInstance()->setRegionChangeCallback(boost::bind(&LLPanelNavMeshRebake::handleRegionBoundaryCrossed, this));
	}

	if (!mAgentStateSlot.connected())
	{
		mAgentStateSlot = LLPathfindingManager::getInstance()->registerAgentStateListener(boost::bind(&LLPanelNavMeshRebake::handleAgentState, this, _1));
	}
	LLPathfindingManager::getInstance()->requestGetAgentState();

	return LLPanel::postBuild();
}

void LLPanelNavMeshRebake::draw()
{
	updatePosition();
	LLPanel::draw();
}

BOOL LLPanelNavMeshRebake::handleToolTip( S32 x, S32 y, MASK mask )
{
	LLToolTipMgr::instance().unblockToolTips();

	if (mNavMeshRebakeButton->getVisible())
	{
		LLToolTipMgr::instance().show(mNavMeshRebakeButton->getToolTip());
	}

	return LLPanel::handleToolTip(x, y, mask);
}

LLPanelNavMeshRebake::LLPanelNavMeshRebake() 
	: mCanRebakeRegion(TRUE),
	mNavMeshRebakeButton( NULL ),
	mNavMeshBakingButton( NULL ),
	mNavMeshSlot(),
	mRegionCrossingSlot(),
	mAgentStateSlot()
{
	// make sure we have the only instance of this class
	static bool b = true;
	llassert_always(b);
	b=false;
}

LLPanelNavMeshRebake::~LLPanelNavMeshRebake() 
{
}

LLPanelNavMeshRebake* LLPanelNavMeshRebake::getPanel()
{
	LLPanelNavMeshRebake* panel = new LLPanelNavMeshRebake();
	panel->buildFromFile("panel_navmesh_rebake.xml");
	panel->setVisible(FALSE);
	return panel;
}

void LLPanelNavMeshRebake::setMode(ERebakeNavMeshMode pRebakeNavMeshMode)
{
	mNavMeshRebakeButton->setVisible(pRebakeNavMeshMode == kRebakeNavMesh_Available);
	mNavMeshBakingButton->setVisible(pRebakeNavMeshMode == kRebakeNavMesh_RequestSent);
	setVisible(pRebakeNavMeshMode != kRebakeNavMesh_NotAvailable);
}

void LLPanelNavMeshRebake::onNavMeshRebakeClick()
{
	setMode(kRebakeNavMesh_RequestSent);
	LLPathfindingManager::getInstance()->requestRebakeNavMesh(boost::bind(&LLPanelNavMeshRebake::handleRebakeNavMeshResponse, this, _1));
}

void LLPanelNavMeshRebake::handleAgentState(BOOL pCanRebakeRegion)
{
	mCanRebakeRegion = pCanRebakeRegion;
	llinfos << "STINSON DEBUG: canRebakeRegion => " << (pCanRebakeRegion ? "TRUE" : "FALSE") << llendl;
}

void LLPanelNavMeshRebake::handleRebakeNavMeshResponse(bool pResponseStatus)
{
	setMode(pResponseStatus ? kRebakeNavMesh_NotAvailable : kRebakeNavMesh_Available);
}

void LLPanelNavMeshRebake::handleNavMeshStatus(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	ERebakeNavMeshMode rebakeNavMeshMode = kRebakeNavMesh_Default;
	if (pNavMeshStatus.isValid())
	{
		switch (pNavMeshStatus.getStatus())
		{
		case LLPathfindingNavMeshStatus::kPending :
			rebakeNavMeshMode = kRebakeNavMesh_Available;
			break;
		case LLPathfindingNavMeshStatus::kBuilding :
			rebakeNavMeshMode = kRebakeNavMesh_NotAvailable;
			break;
		case LLPathfindingNavMeshStatus::kComplete :
			rebakeNavMeshMode = kRebakeNavMesh_NotAvailable;
			break;
		case LLPathfindingNavMeshStatus::kRepending :
			rebakeNavMeshMode = kRebakeNavMesh_Available;
			break;
		default : 
			rebakeNavMeshMode = kRebakeNavMesh_Default;
			llassert(0);
			break;
		}
	}

	setMode(rebakeNavMeshMode);
}

void LLPanelNavMeshRebake::handleRegionBoundaryCrossed()
{
	createNavMeshStatusListenerForCurrentRegion();
	LLPathfindingManager::getInstance()->requestGetAgentState();
}

void LLPanelNavMeshRebake::createNavMeshStatusListenerForCurrentRegion()
{
	if (mNavMeshSlot.connected())
	{
		mNavMeshSlot.disconnect();
	}

	LLViewerRegion *currentRegion = gAgent.getRegion();
	if (currentRegion != NULL)
	{
		mNavMeshSlot = LLPathfindingManager::getInstance()->registerNavMeshListenerForRegion(currentRegion, boost::bind(&LLPanelNavMeshRebake::handleNavMeshStatus, this, _2));
		LLPathfindingManager::getInstance()->requestGetNavMeshForRegion(currentRegion, true);
	}
}

void LLPanelNavMeshRebake::updatePosition()
{
	S32 y_pos = 0;
	S32 bottom_tb_center = 0;

	if (LLToolBar* toolbar_bottom = gToolBarView->getChild<LLToolBar>("toolbar_bottom"))
	{
		y_pos = toolbar_bottom->getRect().getHeight();
		bottom_tb_center = toolbar_bottom->getRect().getCenterX();
	}

	S32 left_tb_width = 0;
	if (LLToolBar* toolbar_left = gToolBarView->getChild<LLToolBar>("toolbar_left"))
	{
		left_tb_width = toolbar_left->getRect().getWidth();
	}

	if(LLPanel* panel_ssf_container = getRootView()->getChild<LLPanel>("state_management_buttons_container"))
	{
		panel_ssf_container->setOrigin(0, y_pos);
	}

	S32 x_pos = bottom_tb_center-getRect().getWidth()/2 - left_tb_width + 113 /* width of stand/fly button */ + 10 /* margin */;

	setOrigin( x_pos, 0);
}
