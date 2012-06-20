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

#include "llbutton.h"
#include "llhandle.h"
#include "llhints.h"
#include "llpanel.h"
#include "llpathfindingmanager.h"
#include "lltoolbar.h"
#include "lltoolbarview.h"
#include "lltoolmgr.h"
#include "lltooltip.h"
#include "llview.h"

LLPanelNavMeshRebake* LLPanelNavMeshRebake::getInstance()
{
	static LLPanelNavMeshRebake* panel = getPanel();
	return panel;
}

void LLPanelNavMeshRebake::setMode(ERebakeNavMeshMode pRebakeNavMeshMode)
{
	mNavMeshRebakeButton->setVisible(pRebakeNavMeshMode == kRebakeNavMesh_Available);
	mNavMeshBakingButton->setVisible(pRebakeNavMeshMode == kRebakeNavMesh_RequestSent);
	setVisible(pRebakeNavMeshMode != kRebakeNavMesh_NotAvailable);
}

BOOL LLPanelNavMeshRebake::postBuild()
{
	//Rebake initiated
	mNavMeshRebakeButton = getChild<LLButton>("navmesh_btn");
	mNavMeshRebakeButton->setCommitCallback(boost::bind(&LLPanelNavMeshRebake::onNavMeshRebakeClick, this));
	LLHints::registerHintTarget("navmesh_btn", mNavMeshRebakeButton->getHandle());
	
	//Baking...
	mNavMeshBakingButton = getChild<LLButton>("navmesh_btn_baking");
	LLHints::registerHintTarget("navmesh_btn_baking", mNavMeshBakingButton->getHandle());

	setMode(kRebakeNavMesh_Default);

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
	: mNavMeshRebakeButton( NULL ),
	mNavMeshBakingButton( NULL )
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

void LLPanelNavMeshRebake::onNavMeshRebakeClick()
{
#if 0
	mNavMeshRebakeButton->setVisible( FALSE ); 
	mNavMeshBakingButton->setVisible( TRUE ); 
	mNavMeshBakingButton->setForcePressedState( TRUE );
#endif
	LLPathfindingManager::getInstance()->triggerNavMeshRebuild();
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

	S32 x_pos = bottom_tb_center-getRect().getWidth()/2 - left_tb_width + 113 /*width of stand/fly button */ + 10;

	setOrigin( x_pos, 0);
}
