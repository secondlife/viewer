/** 
* @file llpanelpathfindingrebakenavmesh.cpp
* @brief Implementation of llpanelpathfindingrebakenavmesh
* @author Prep@lindenlab.com
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

#include "llpanelpathfindingrebakenavmesh.h"

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llbutton.h"
#include "llenvmanager.h"
#include "llhints.h"
#include "llnotificationsutil.h"
#include "llpanel.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "lltoolbar.h"
#include "lltoolbarview.h"
#include "lltooltip.h"
#include "llviewerregion.h"

LLPanelPathfindingRebakeNavmesh* LLPanelPathfindingRebakeNavmesh::getInstance()
{
	static LLPanelPathfindingRebakeNavmesh* panel = getPanel();
	return panel;
}

BOOL LLPanelPathfindingRebakeNavmesh::postBuild()
{
	//Rebake button
	mNavMeshRebakeButton = findChild<LLButton>("navmesh_btn");
	llassert(mNavMeshRebakeButton != NULL);
	mNavMeshRebakeButton->setCommitCallback(boost::bind(&LLPanelPathfindingRebakeNavmesh::onNavMeshRebakeClick, this));
	LLHints::registerHintTarget("navmesh_btn", mNavMeshRebakeButton->getHandle());
	
	//Sending rebake request
	mNavMeshSendingButton = findChild<LLButton>("navmesh_btn_sending");
	llassert(mNavMeshSendingButton != NULL);
	LLHints::registerHintTarget("navmesh_btn_sending", mNavMeshSendingButton->getHandle());

	//rebaking...
	mNavMeshBakingButton = findChild<LLButton>("navmesh_btn_baking");
	llassert(mNavMeshBakingButton != NULL);
	LLHints::registerHintTarget("navmesh_btn_baking", mNavMeshBakingButton->getHandle());

	setMode(kRebakeNavMesh_Default);

	createNavMeshStatusListenerForCurrentRegion();

	if ( !mRegionCrossingSlot.connected() )
	{
		mRegionCrossingSlot = LLEnvManagerNew::getInstance()->setRegionChangeCallback(boost::bind(&LLPanelPathfindingRebakeNavmesh::handleRegionBoundaryCrossed, this));
	}

	if (!mAgentStateSlot.connected())
	{
		mAgentStateSlot = LLPathfindingManager::getInstance()->registerAgentStateListener(boost::bind(&LLPanelPathfindingRebakeNavmesh::handleAgentState, this, _1));
	}
	LLPathfindingManager::getInstance()->requestGetAgentState();

	return LLPanel::postBuild();
}

void LLPanelPathfindingRebakeNavmesh::draw()
{
	if (doDraw())
	{
		updatePosition();
		LLPanel::draw();
	}
}

BOOL LLPanelPathfindingRebakeNavmesh::handleToolTip( S32 x, S32 y, MASK mask )
{
	LLToolTipMgr::instance().unblockToolTips();

	if (mNavMeshRebakeButton->getVisible())
	{
		LLToolTipMgr::instance().show(mNavMeshRebakeButton->getToolTip());
	}
	else if (mNavMeshSendingButton->getVisible())
	{
		LLToolTipMgr::instance().show(mNavMeshSendingButton->getToolTip());
	}
	else if (mNavMeshBakingButton->getVisible())
	{
		LLToolTipMgr::instance().show(mNavMeshBakingButton->getToolTip());
	}

	return LLPanel::handleToolTip(x, y, mask);
}

LLPanelPathfindingRebakeNavmesh::LLPanelPathfindingRebakeNavmesh() 
	: LLPanel(),
	mCanRebakeRegion(FALSE),
	mRebakeNavMeshMode(kRebakeNavMesh_Default),
	mNavMeshRebakeButton(NULL),
	mNavMeshSendingButton(NULL),
	mNavMeshBakingButton(NULL),
	mNavMeshSlot(),
	mRegionCrossingSlot(),
	mAgentStateSlot()
{
	// make sure we have the only instance of this class
	static bool b = true;
	llassert_always(b);
	b=false;
}

LLPanelPathfindingRebakeNavmesh::~LLPanelPathfindingRebakeNavmesh() 
{
}

LLPanelPathfindingRebakeNavmesh* LLPanelPathfindingRebakeNavmesh::getPanel()
{
	LLPanelPathfindingRebakeNavmesh* panel = new LLPanelPathfindingRebakeNavmesh();
	panel->buildFromFile("panel_navmesh_rebake.xml");
	return panel;
}

void LLPanelPathfindingRebakeNavmesh::setMode(ERebakeNavMeshMode pRebakeNavMeshMode)
{
	if (pRebakeNavMeshMode == kRebakeNavMesh_Available)
	{
		LLNotificationsUtil::add("PathfindingRebakeNavmesh");
	}
	mNavMeshRebakeButton->setVisible(pRebakeNavMeshMode == kRebakeNavMesh_Available);
	mNavMeshSendingButton->setVisible(pRebakeNavMeshMode == kRebakeNavMesh_RequestSent);
	mNavMeshBakingButton->setVisible(pRebakeNavMeshMode == kRebakeNavMesh_InProgress);
	mRebakeNavMeshMode = pRebakeNavMeshMode;
}

LLPanelPathfindingRebakeNavmesh::ERebakeNavMeshMode LLPanelPathfindingRebakeNavmesh::getMode() const
{
	return mRebakeNavMeshMode;
}

void LLPanelPathfindingRebakeNavmesh::onNavMeshRebakeClick()
{
	setMode(kRebakeNavMesh_RequestSent);
	LLPathfindingManager::getInstance()->requestRebakeNavMesh(boost::bind(&LLPanelPathfindingRebakeNavmesh::handleRebakeNavMeshResponse, this, _1));
}

void LLPanelPathfindingRebakeNavmesh::handleAgentState(BOOL pCanRebakeRegion)
{
	mCanRebakeRegion = pCanRebakeRegion;
}

void LLPanelPathfindingRebakeNavmesh::handleRebakeNavMeshResponse(bool pResponseStatus)
{
	if (getMode() == kRebakeNavMesh_RequestSent)
	{
		setMode(pResponseStatus ? kRebakeNavMesh_InProgress : kRebakeNavMesh_Default);
	}

	if (!pResponseStatus)
	{
		LLNotificationsUtil::add("PathfindingCannotRebakeNavmesh");
	}
}

void LLPanelPathfindingRebakeNavmesh::handleNavMeshStatus(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	ERebakeNavMeshMode rebakeNavMeshMode = kRebakeNavMesh_Default;
	if (pNavMeshStatus.isValid())
	{
		switch (pNavMeshStatus.getStatus())
		{
		case LLPathfindingNavMeshStatus::kPending :
		case LLPathfindingNavMeshStatus::kRepending :
			rebakeNavMeshMode = kRebakeNavMesh_Available;
			break;
		case LLPathfindingNavMeshStatus::kBuilding :
			rebakeNavMeshMode = kRebakeNavMesh_InProgress;
			break;
		case LLPathfindingNavMeshStatus::kComplete :
			rebakeNavMeshMode = kRebakeNavMesh_NotAvailable;
			break;
		default : 
			rebakeNavMeshMode = kRebakeNavMesh_Default;
			llassert(0);
			break;
		}
	}

	setMode(rebakeNavMeshMode);
}

void LLPanelPathfindingRebakeNavmesh::handleRegionBoundaryCrossed()
{
	createNavMeshStatusListenerForCurrentRegion();
	mCanRebakeRegion = FALSE;
	LLPathfindingManager::getInstance()->requestGetAgentState();
}

void LLPanelPathfindingRebakeNavmesh::createNavMeshStatusListenerForCurrentRegion()
{
	if (mNavMeshSlot.connected())
	{
		mNavMeshSlot.disconnect();
	}

	LLViewerRegion *currentRegion = gAgent.getRegion();
	if (currentRegion != NULL)
	{
		mNavMeshSlot = LLPathfindingManager::getInstance()->registerNavMeshListenerForRegion(currentRegion, boost::bind(&LLPanelPathfindingRebakeNavmesh::handleNavMeshStatus, this, _2));
		LLPathfindingManager::getInstance()->requestGetNavMeshForRegion(currentRegion, true);
	}
}

bool LLPanelPathfindingRebakeNavmesh::doDraw() const
{
	return (mCanRebakeRegion && (mRebakeNavMeshMode != kRebakeNavMesh_NotAvailable));
}

void LLPanelPathfindingRebakeNavmesh::updatePosition()
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
