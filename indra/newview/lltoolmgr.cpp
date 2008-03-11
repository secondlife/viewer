/** 
 * @file lltoolmgr.cpp
 * @brief LLToolMgr class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "lltoolmgr.h"

#include "lltool.h"
// tools and manipulators
#include "llmanipscale.h"
#include "lltoolbrush.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolindividual.h"
#include "lltoolmorph.h"
#include "lltoolpie.h"
#include "lltoolplacer.h"
#include "lltoolselect.h"
#include "lltoolselectland.h"
#include "lltoolobjpicker.h"
#include "lltoolpipette.h"

// Globals (created and destroyed by LLAgent)
LLToolMgr*		gToolMgr	= NULL;

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
	gToolNull = new LLTool(NULL);  // Does nothing
	setCurrentTool(gToolNull);

	gBasicToolset		= new LLToolset();
	gCameraToolset		= new LLToolset();
//	gLandToolset		= new LLToolset();
	gMouselookToolset	= new LLToolset();
	gFaceEditToolset	= new LLToolset();
}

void LLToolMgr::initTools()
{
	// Initialize all the tools
	// Variables that are reused for each tool
	LLTool*		tool	= NULL;

	//
	// Pie tool (not visible in UI, implicit)
	//
	gToolPie = new LLToolPie();

	gBasicToolset->addTool( gToolPie );
//	gCameraToolset->addTool( gToolPie );
//	gLandToolset->addTool( gToolPie );

	// Camera tool
	gToolCamera = new LLToolCamera();
	gBasicToolset	->addTool( gToolCamera );
	gCameraToolset->addTool( gToolCamera );

	//
	// Grab tool
	//
	gToolGrab = new LLToolGrab();
	tool = gToolGrab;
		
	gBasicToolset->addTool( tool );

	//
	// Translation tool
	//
	gToolTranslate = new LLToolCompTranslate();
	tool = gToolTranslate;

	gBasicToolset->addTool( tool );

	//
	// Scale ("Stretch") tool
	//
	gToolStretch = new LLToolCompScale();
	tool = gToolStretch;


	//
	// Rotation tool
	//
	gToolRotate = new LLToolCompRotate();
	tool = gToolRotate;


	//
	// Face tool
	//
	gToolFace = new LLToolFace();
	tool = gToolFace;

	//
	// Pipette tool
	//
	gToolPipette = new LLToolPipette();

	//
	// Individual object selector
	//
	gToolIndividual = new LLToolIndividual();

	
	//
	// Create object tool
	//
	gToolCreate = new LLToolCompCreate();
	tool = gToolCreate;

	gBasicToolset->addTool( tool );

	//
	// Land brush tool
	//
	gToolLand = new LLToolBrushLand();
	tool = gToolLand;

	gBasicToolset->addTool( tool );


	//
	// Land select tool
	//
	gToolParcel = new LLToolSelectLand();
	tool = gToolParcel;

	//
	// Gun tool
	//
	gToolGun = new LLToolCompGun();
	gMouselookToolset->addTool( gToolGun );

	//
	// Inspect tool
	//
	gToolInspect = new LLToolCompInspect();
	gBasicToolset->addTool( gToolInspect );

	//
	// Face edit tool
	//
//	gToolMorph = new LLToolMorph();
//	gFaceEditToolset->addTool( gToolMorph );
	gFaceEditToolset->addTool( gToolCamera );

	// Drag and drop tool
	gToolDragAndDrop = new LLToolDragAndDrop();

	gToolObjPicker = new LLToolObjPicker();

	// On startup, use "select" tool
	setCurrentToolset(gBasicToolset);
	gBasicToolset->selectTool( gToolPie );
}

LLToolMgr::~LLToolMgr()
{
	delete gToolPie;
	gToolPie = NULL;

	delete gToolInspect;
	gToolInspect = NULL;

	delete gToolGun;
	gToolGun = NULL;

	delete gToolCamera;
	gToolCamera = NULL;

//	delete gToolMorph;
//	gToolMorph = NULL;

	delete gToolDragAndDrop;
	gToolDragAndDrop = NULL;

	delete gBasicToolset;
	gBasicToolset = NULL;

	delete gToolGrab;
	gToolGrab = NULL;

	delete gToolRotate;
	gToolRotate = NULL;

	delete gToolTranslate;
	gToolTranslate = NULL;

	delete gToolStretch;
	gToolStretch = NULL;

	delete gToolIndividual;
	gToolIndividual = NULL;

	delete gToolPipette;
	gToolPipette = NULL;

	delete gToolCreate;
	gToolCreate = NULL;

	delete gToolFace;
	gToolFace = NULL;

	delete gToolLand;
	gToolLand = NULL;

	delete gToolParcel;
	gToolParcel = NULL;

	delete gToolObjPicker;
	gToolObjPicker = NULL;

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
}

LLTool* LLToolMgr::getCurrentTool()
{
	MASK override_mask = gKeyboard->currentMask(TRUE);

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

BOOL LLToolMgr::inEdit()
{
	return mBaseTool != gToolPie && mBaseTool != gToolNull;
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


// The "gun tool", used for handling mouselook, captures the mouse and
// locks it within the window.  When the app loses focus we need to
// release this locking.
void LLToolMgr::onAppFocusLost()
{
	mSavedTool = mBaseTool;
	mBaseTool = gToolNull;
	updateToolStatus();
}

void LLToolMgr::onAppFocusGained()
{
	if (mSavedTool)
	{
		mBaseTool = mSavedTool;
		mSavedTool = NULL;
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
	gToolMgr->setCurrentTool( mSelectedTool );
}


void LLToolset::selectToolByIndex( S32 index )
{
	LLTool *tool = (index >= 0 && index < (S32)mToolList.size()) ? mToolList[index] : NULL;
	if (tool)
	{
		mSelectedTool = tool;
		gToolMgr->setCurrentTool( tool );
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
	if (gToolMgr) 
	{
		gToolMgr->setCurrentTool( mSelectedTool );
	}
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
		gToolMgr->setCurrentTool( mSelectedTool );
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
		gToolMgr->setCurrentTool( mSelectedTool );
	}
	else if (mToolList.size() > 0)
	{
		selectToolByIndex((S32)mToolList.size()-1);
	}
	
}

void select_tool( void *tool_pointer )
{
	LLTool *tool = (LLTool *)tool_pointer;
	gToolMgr->getCurrentToolset()->selectTool( tool );
}
