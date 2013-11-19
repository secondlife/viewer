/** 
 * @file lltoolmgr.cpp
 * @brief LLToolMgr class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "lltoolmgr.h"

#include "lluictrl.h"
#include "llmenugl.h"
#include "llfloaterreg.h"

//#include "llfirstuse.h"
// tools and manipulators
#include "lltool.h"
#include "llmanipscale.h"
#include "llselectmgr.h"
#include "lltoolbrush.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolindividual.h"
#include "lltoolmorph.h"
#include "lltoolpie.h"
#include "lltoolselectland.h"
#include "lltoolobjpicker.h"
#include "lltoolpipette.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewercontrol.h"
#include "llviewerjoystick.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"


// Used when app not active to avoid processing hover.
LLTool*			gToolNull	= NULL;

LLToolset*		gBasicToolset		= NULL;
LLToolset*		gCameraToolset		= NULL;
//LLToolset*		gLandToolset		= NULL;
LLToolset*		gMouselookToolset	= NULL;
LLToolset*		gFaceEditToolset	= NULL;

/////////////////////////////////////////////////////
// LLToolMgr

LLToolMgr::LLToolMgr()
	:
	mBaseTool(NULL), 
	mSavedTool(NULL),
	mTransientTool( NULL ),
	mOverrideTool( NULL ),
	mSelectedTool( NULL ),
	mCurrentToolset( NULL )
{
	// Not a panel, register these callbacks globally.
	LLUICtrl::EnableCallbackRegistry::currentRegistrar().add("Build.Active", boost::bind(&LLToolMgr::inEdit, this));
	LLUICtrl::EnableCallbackRegistry::currentRegistrar().add("Build.Enabled", boost::bind(&LLToolMgr::canEdit, this));
	LLUICtrl::CommitCallbackRegistry::currentRegistrar().add("Build.Toggle", boost::bind(&LLToolMgr::toggleBuildMode, this, _2));
	
	gToolNull = new LLTool(LLStringUtil::null);  // Does nothing
	setCurrentTool(gToolNull);

	gBasicToolset		= new LLToolset();
	gCameraToolset		= new LLToolset();
//	gLandToolset		= new LLToolset();
	gMouselookToolset	= new LLToolset();
	gFaceEditToolset	= new LLToolset();
	gMouselookToolset->setShowFloaterTools(false);
	gFaceEditToolset->setShowFloaterTools(false);
}

void LLToolMgr::initTools()
{
	static BOOL initialized = FALSE;
	if(initialized)
	{
		return;
	}
	initialized = TRUE;
	gBasicToolset->addTool( LLToolPie::getInstance() );
	gBasicToolset->addTool( LLToolCamera::getInstance() );
	gCameraToolset->addTool( LLToolCamera::getInstance() );
	gBasicToolset->addTool( LLToolGrab::getInstance() );
	gBasicToolset->addTool( LLToolCompTranslate::getInstance() );
	gBasicToolset->addTool( LLToolCompCreate::getInstance() );
	gBasicToolset->addTool( LLToolBrushLand::getInstance() );
	gMouselookToolset->addTool( LLToolCompGun::getInstance() );
	gBasicToolset->addTool( LLToolCompInspect::getInstance() );
	gFaceEditToolset->addTool( LLToolCamera::getInstance() );

	// On startup, use "select" tool
	setCurrentToolset(gBasicToolset);

	gBasicToolset->selectTool( LLToolPie::getInstance() );
}

LLToolMgr::~LLToolMgr()
{
	delete gBasicToolset;
	gBasicToolset = NULL;

	delete gMouselookToolset;
	gMouselookToolset = NULL;

	delete gFaceEditToolset;
	gFaceEditToolset = NULL;

	delete gCameraToolset;
	gCameraToolset = NULL;
	
	delete gToolNull;
	gToolNull = NULL;
}

BOOL LLToolMgr::usingTransientTool()
{
	return mTransientTool ? TRUE : FALSE;
}

void LLToolMgr::setCurrentToolset(LLToolset* current)
{
	if (!current) return;

	// switching toolsets?
	if (current != mCurrentToolset)
	{
		// deselect current tool
		if (mSelectedTool)
		{
			mSelectedTool->handleDeselect();
		}
		mCurrentToolset = current;
		// select first tool of new toolset only if toolset changed
		mCurrentToolset->selectFirstTool();
	}
	// update current tool based on new toolset
	setCurrentTool( mCurrentToolset->getSelectedTool() );
}

LLToolset* LLToolMgr::getCurrentToolset()
{
	return mCurrentToolset;
}

void LLToolMgr::setCurrentTool( LLTool* tool )
{
	if (mTransientTool)
	{
		mTransientTool = NULL;
	}

	mBaseTool = tool;
	updateToolStatus();

	mSavedTool = NULL;
}

LLTool* LLToolMgr::getCurrentTool()
{
	MASK override_mask = gKeyboard ? gKeyboard->currentMask(TRUE) : 0;

	LLTool* cur_tool = NULL;
	// always use transient tools if available
	if (mTransientTool)
	{
		mOverrideTool = NULL;
		cur_tool = mTransientTool;
	}
	// tools currently grabbing mouse input will stay active
	else if (mSelectedTool && mSelectedTool->hasMouseCapture())
	{
		cur_tool = mSelectedTool;
	}
	else
	{
		mOverrideTool = mBaseTool ? mBaseTool->getOverrideTool(override_mask) : NULL;

		// use override tool if available otherwise drop back to base tool
		cur_tool = mOverrideTool ? mOverrideTool : mBaseTool;
	}

	LLTool* prev_tool = mSelectedTool;
	// Set the selected tool to avoid infinite recursion
	mSelectedTool = cur_tool;
	
	//update tool selection status
	if (prev_tool != cur_tool)
	{
		if (prev_tool)
		{
			prev_tool->handleDeselect();
		}
		if (cur_tool)
		{
			cur_tool->handleSelect();
		}
	}

	return mSelectedTool;
}

LLTool* LLToolMgr::getBaseTool()
{
	return mBaseTool;
}

void LLToolMgr::updateToolStatus()
{
	// call getcurrenttool() to calculate active tool and call handleSelect() and handleDeselect() immediately
	// when active tool changes
	getCurrentTool();
}

bool LLToolMgr::inEdit()
{
	return mBaseTool != LLToolPie::getInstance() && mBaseTool != gToolNull;
}

bool LLToolMgr::canEdit()
{
	return LLViewerParcelMgr::getInstance()->allowAgentBuild();
}

void LLToolMgr::toggleBuildMode(const LLSD& sdname)
{
	const std::string& param = sdname.asString();

	if (param == "build" && !canEdit())
	{
		return;
	}

	LLFloaterReg::toggleInstanceOrBringToFront("build");

	bool build_visible = LLFloaterReg::instanceVisible("build");
	if (build_visible)
	{
		ECameraMode camMode = gAgentCamera.getCameraMode();
		if (CAMERA_MODE_MOUSELOOK == camMode ||	CAMERA_MODE_CUSTOMIZE_AVATAR == camMode)
		{
			// pull the user out of mouselook or appearance mode when entering build mode
			handle_reset_view();
		}

		if (gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// camera should be set
			if (LLViewerJoystick::getInstance()->getOverrideCamera())
			{
				handle_toggle_flycam();
			}

			if (gAgentCamera.getFocusOnAvatar())
			{
				// zoom in if we're looking at the avatar
				gAgentCamera.setFocusOnAvatar(FALSE, ANIMATE);
				gAgentCamera.setFocusGlobal(gAgent.getPositionGlobal() + 2.0 * LLVector3d(gAgent.getAtAxis()));
				gAgentCamera.cameraZoomIn(0.666f);
				gAgentCamera.cameraOrbitOver( 30.f * DEG_TO_RAD );
			}
		}


		setCurrentToolset(gBasicToolset);
		getCurrentToolset()->selectTool( LLToolCompCreate::getInstance() );

		// Could be first use
		//LLFirstUse::useBuild();

		gAgentCamera.resetView(false);

		// avoid spurious avatar movements
		LLViewerJoystick::getInstance()->setNeedsReset();

	}
	else
	{
		if (gSavedSettings.getBOOL("EditCameraMovement"))
		{
			// just reset the view, will pull us out of edit mode
			handle_reset_view();
		}
		else
		{
			// manually disable edit mode, but do not affect the camera
			gAgentCamera.resetView(false);
			LLFloaterReg::hideInstance("build");
			gViewerWindow->showCursor();			
		}
		// avoid spurious avatar movements pulling out of edit mode
		LLViewerJoystick::getInstance()->setNeedsReset();
	}

}

bool LLToolMgr::inBuildMode()
{
	// when entering mouselook inEdit() immediately returns true before 
	// cameraMouselook() actually starts returning true.  Also, appearance edit
	// sets build mode to true, so let's exclude that.
	bool b=(inEdit() 
			&& !gAgentCamera.cameraMouselook()
			&& mCurrentToolset != gFaceEditToolset);
	
	return b;
}

void LLToolMgr::setTransientTool(LLTool* tool)
{
	if (!tool)
	{
		clearTransientTool();
	}
	else
	{
		if (mTransientTool)
		{
			mTransientTool = NULL;
		}

		mTransientTool = tool;
	}

	updateToolStatus();
}

void LLToolMgr::clearTransientTool()
{
	if (mTransientTool)
	{
		mTransientTool = NULL;
		if (!mBaseTool)
		{
			llwarns << "mBaseTool is NULL" << llendl;
		}
	}
	updateToolStatus();
}


void LLToolMgr::onAppFocusLost()
{
	if (mSelectedTool)
	{
		mSelectedTool->handleDeselect();
	}
	updateToolStatus();
}

void LLToolMgr::onAppFocusGained()
{
	if (mSelectedTool)
	{
		mSelectedTool->handleSelect();
	}
	updateToolStatus();
}

void LLToolMgr::clearSavedTool()
{
	mSavedTool = NULL;
}

/////////////////////////////////////////////////////
// LLToolset

void LLToolset::addTool(LLTool* tool)
{
	mToolList.push_back( tool );
	if( !mSelectedTool )
	{
		mSelectedTool = tool;
	}
}


void LLToolset::selectTool(LLTool* tool)
{
	mSelectedTool = tool;
	LLToolMgr::getInstance()->setCurrentTool( mSelectedTool );
}


void LLToolset::selectToolByIndex( S32 index )
{
	LLTool *tool = (index >= 0 && index < (S32)mToolList.size()) ? mToolList[index] : NULL;
	if (tool)
	{
		mSelectedTool = tool;
		LLToolMgr::getInstance()->setCurrentTool( tool );
	}
}

BOOL LLToolset::isToolSelected( S32 index )
{
	LLTool *tool = (index >= 0 && index < (S32)mToolList.size()) ? mToolList[index] : NULL;
	return (tool == mSelectedTool);
}


void LLToolset::selectFirstTool()
{
	mSelectedTool = (0 < mToolList.size()) ? mToolList[0] : NULL;
	LLToolMgr::getInstance()->setCurrentTool( mSelectedTool );
}


void LLToolset::selectNextTool()
{
	LLTool* next = NULL;
	for( tool_list_t::iterator iter = mToolList.begin();
		 iter != mToolList.end(); )
	{
		LLTool* cur = *iter++;
		if( cur == mSelectedTool && iter != mToolList.end() )
		{
			next = *iter;
			break;
		}
	}

	if( next )
	{
		mSelectedTool = next;
		LLToolMgr::getInstance()->setCurrentTool( mSelectedTool );
	}
	else
	{
		selectFirstTool();
	}
}

void LLToolset::selectPrevTool()
{
	LLTool* prev = NULL;
	for( tool_list_t::reverse_iterator iter = mToolList.rbegin();
		 iter != mToolList.rend(); )
	{
		LLTool* cur = *iter++;
		if( cur == mSelectedTool && iter != mToolList.rend() )
		{
			prev = *iter;
			break;
		}
	}

	if( prev )
	{
		mSelectedTool = prev;
		LLToolMgr::getInstance()->setCurrentTool( mSelectedTool );
	}
	else if (mToolList.size() > 0)
	{
		selectToolByIndex((S32)mToolList.size()-1);
	}
}

////////////////////////////////////////////////////////////////////////////


