/**
 * @file LLPanelNavMeshRebake.cpp
 * @author 
 * @brief
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

#include "llpathfindingmanager.h"

#include <string>
#include <map>

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llpanelnavmeshrebake.h"
#include "llagent.h"
#include "llfloaterreg.h"
#include "llhints.h"
#include "lltooltip.h"
#include "llbutton.h"
#include "llpanel.h"

LLPanelNavMeshRebake::LLPanelNavMeshRebake() 
: mNavMeshRebakeButton(NULL)
, mAttached(false)
{
	// make sure we have the only instance of this class
	static bool b = true;
	llassert_always(b);
	b=false;
}

// static
LLPanelNavMeshRebake* LLPanelNavMeshRebake::getInstance()
{
	static LLPanelNavMeshRebake* panel = getPanel();
	return panel;
}

//static
void LLPanelNavMeshRebake::setMode( ESNavMeshRebakeMode mode )
{
	LLPanelNavMeshRebake* panel = getInstance();

	panel->mNavMeshRebakeButton->setVisible( true );

	//visibility of it should be updated after updating visibility of the buttons
	panel->setVisible(TRUE);
}

void LLPanelNavMeshRebake::clearMode( ESNavMeshRebakeMode mode )
{
	LLPanelNavMeshRebake* panel = getInstance();
	switch(mode)
	{
	case NMRM_Visible:
		panel->mNavMeshRebakeButton->setVisible(FALSE);
		break;
	
	default:
		llerrs << "Unexpected mode is passed: " << mode << llendl;
	}
}

BOOL LLPanelNavMeshRebake::postBuild()
{
	mNavMeshRebakeButton = getChild<LLButton>("navmesh_btn");
	mNavMeshRebakeButton->setCommitCallback(boost::bind(&LLPanelNavMeshRebake::onNavMeshRebakeClick, this));
	mNavMeshRebakeButton->setVisible( TRUE );
	LLHints::registerHintTarget("navmesh_btn", mNavMeshRebakeButton->getHandle());

	return TRUE;
}

void LLPanelNavMeshRebake::setVisible( BOOL visible )
{

	LLPanel::setVisible(visible);
}

BOOL LLPanelNavMeshRebake::handleToolTip(S32 x, S32 y, MASK mask)
{
	LLToolTipMgr::instance().unblockToolTips();

	if (mNavMeshRebakeButton->getVisible())
	{
		LLToolTipMgr::instance().show(mNavMeshRebakeButton->getToolTip());
	}
	return LLPanel::handleToolTip(x, y, mask);
}

void LLPanelNavMeshRebake::reparent(LLView* rootp)
{
	LLPanel* parent = dynamic_cast<LLPanel*>(getParent());
	if (!parent)
	{
		return;
	}

	rootp->addChild(this);
	mAttached = true;
}

//static
LLPanelNavMeshRebake* LLPanelNavMeshRebake::getPanel()
{
	LLPanelNavMeshRebake* panel = new LLPanelNavMeshRebake();
	panel->buildFromFile("panel_navmesh_rebake.xml");

	panel->setVisible(FALSE);

	llinfos << "Build LLPanelNavMeshRebake panel" << llendl;

	//prep#panel->updatePosition();
	return panel;
}

void LLPanelNavMeshRebake::onNavMeshRebakeClick()
{
	setFocus(FALSE); 
	mNavMeshRebakeButton->setVisible(FALSE); 
}

/**
 * Updates position  to be center aligned with Move button.
 */
/*
void LLPanelNavMeshRebake::updatePosition()
{
	if (mAttached) return;

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

	if(LLPanel* panel_ssf_container = getRootView()->getChild<LLPanel>("navmesh_rebake_container"))
	{
		panel_ssf_container->setOrigin(0, y_pos);
	}

	S32 x_pos = bottom_tb_center-getRect().getWidth()/2 - left_tb_width;

	setOrigin( x_pos, 0);


	*/
