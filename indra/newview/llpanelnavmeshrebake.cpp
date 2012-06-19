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
#include "llpathfindingmanager.h"
#include <string>
#include <map>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include "llpanelnavmeshrebake.h"
#include "llagent.h"
#include "llhints.h"
#include "lltooltip.h"
#include "llbutton.h"
#include "llpanel.h"

LLPanelNavMeshRebake::LLPanelNavMeshRebake() 
: mNavMeshRebakeButton( NULL )
, mNavMeshBakingButton( NULL )
{
	// make sure we have the only instance of this class
	static bool b = true;
	llassert_always(b);
	b=false;
}

LLPanelNavMeshRebake* LLPanelNavMeshRebake::getInstance()
{
	static LLPanelNavMeshRebake* panel = getPanel();
	return panel;
}

BOOL LLPanelNavMeshRebake::postBuild()
{
	//Rebake initiated
	mNavMeshRebakeButton = getChild<LLButton>("navmesh_btn");
	mNavMeshRebakeButton->setCommitCallback(boost::bind(&LLPanelNavMeshRebake::onNavMeshRebakeClick, this));
	mNavMeshRebakeButton->setVisible( TRUE );
	LLHints::registerHintTarget("navmesh_btn", mNavMeshRebakeButton->getHandle());
	
	//Baking...
	mNavMeshBakingButton = getChild<LLButton>("navmesh_btn_baking");
	mNavMeshBakingButton->setVisible( FALSE );
	LLHints::registerHintTarget("navmesh_btn_baking", mNavMeshBakingButton->getHandle());
	return TRUE;
}

void LLPanelNavMeshRebake::setVisible( BOOL visible )
{
	LLPanel::setVisible(visible);
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

void LLPanelNavMeshRebake::reparent( LLView* rootp )
{
	LLPanel* parent = dynamic_cast<LLPanel*>( getParent() );
	if (!parent)
	{
		return;
	}
	rootp->addChild(this);
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
	mNavMeshRebakeButton->setVisible( FALSE ); 
	mNavMeshBakingButton->setVisible( TRUE ); 
	mNavMeshBakingButton->setForcePressedState( TRUE );
	LLPathfindingManager::getInstance()->triggerNavMeshRebuild();
}

void LLPanelNavMeshRebake::resetButtonStates()
{
	mNavMeshRebakeButton->setVisible( TRUE ); 
	mNavMeshBakingButton->setVisible( FALSE ); 
}

