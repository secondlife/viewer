/** 
 * @file lltoolmgr.cpp
 * @brief LLToolMgr class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

LLToolset*		gCurrentToolset		= NULL;
LLToolset*		gBasicToolset		= NULL;
LLToolset*		gCameraToolset		= NULL;
//LLToolset*		gLandToolset		= NULL;
LLToolset*		gMouselookToolset	= NULL;
LLToolset*		gFaceEditToolset	= NULL;

/////////////////////////////////////////////////////
// LLToolMgr

LLToolMgr::LLToolMgr()
	:
	mCurrentTool(NULL), 
	mSavedTool(NULL),
	mTransientTool( NULL ),
	mOverrideTool( NULL )
{
	gToolNull = new LLTool(NULL);  // Does nothing
	setCurrentTool(gToolNull);

	gBasicToolset		= new LLToolset();
	gCameraToolset		= new LLToolset();
//	gLandToolset		= new LLToolset();
	gMouselookToolset	= new LLToolset();
	gFaceEditToolset	= new LLToolset();

	gCurrentToolset = gBasicToolset;
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
	gBasicToolset->selectTool( gToolPie );
	useSelectedTool( gBasicToolset );
}

LLToolMgr::~LLToolMgr()
{
	delete gToolPie;
	gToolPie = NULL;

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


void LLToolMgr::useSelectedTool( LLToolset* vp )
{
	setCurrentTool( vp->getSelectedTool() ); 
}

BOOL LLToolMgr::usingTransientTool()
{
	return mTransientTool ? TRUE : FALSE;
}

void LLToolMgr::setCurrentTool( LLTool* tool )
{
	if (tool == mCurrentTool)
	{
		// didn't change tool, so don't mess with
		// handleSelect or handleDeselect
		return;
	}

	if (mTransientTool)
	{
		mTransientTool->handleDeselect();
		mTransientTool = NULL;
	}
	else if( mCurrentTool )
	{
		mCurrentTool->handleDeselect();
	}

	mCurrentTool = tool;
	if (mCurrentTool)
	{
		mCurrentTool->handleSelect();
	}
}

LLTool* LLToolMgr::getCurrentTool(MASK override_mask)
{
	// In mid-drag, always keep the current tool
	if (gToolTranslate->hasMouseCapture()
		|| gToolRotate->hasMouseCapture()
		|| gToolStretch->hasMouseCapture())
	{
		// might have gotten here by overriding another tool
		if (mOverrideTool)
		{
			return mOverrideTool;
		}
		else
		{
			return mCurrentTool;
		}
	}

	if (mTransientTool)
	{
		mOverrideTool = NULL;
		return mTransientTool;
	}

	if (mCurrentTool == gToolGun)
	{
		mOverrideTool = NULL;
		return mCurrentTool;
	}

	// ALT always gets you the camera tool
	if (override_mask & MASK_ALT)
	{
		mOverrideTool = gToolCamera;
		return mOverrideTool;
	}

	if (mCurrentTool == gToolCamera)
	{
		// ...can't switch out of camera
		mOverrideTool = NULL;
		return mCurrentTool;
	}
	else if (mCurrentTool == gToolGrab)
	{
		// ...can't switch out of grab
		mOverrideTool = NULL;
		return mCurrentTool;
	}
	else if (mCurrentTool == gToolInspect)
	{
		// ...can't switch out of grab
		mOverrideTool = NULL;
		return mCurrentTool;
	}
	else
	{
		// ...can switch between editing tools
		if (override_mask == MASK_CONTROL)
		{
			// Control lifts when in the pie tool, otherwise switches to rotate
			if (mCurrentTool == gToolPie)
			{
				mOverrideTool = gToolGrab;
			}
			else
			{
				mOverrideTool = gToolRotate;
			}
			return mOverrideTool;
		}
		else if (override_mask == (MASK_CONTROL | MASK_SHIFT))
		{
			// Shift-Control spins when in the pie tool, otherwise switches to scale
			if (mCurrentTool == gToolPie)
			{
				mOverrideTool = gToolGrab;
			}
			else
			{
				mOverrideTool = gToolStretch;
			}
			return mOverrideTool;
		}
		else
		{
			mOverrideTool = NULL;
			return mCurrentTool;
		}
	}
}

BOOL LLToolMgr::inEdit()
{
	return mCurrentTool != gToolPie && mCurrentTool != gToolNull;
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
			mTransientTool->handleDeselect();
			mTransientTool = NULL;
		}
		else if (mCurrentTool)
		{
			mCurrentTool->handleDeselect();
		}

		mTransientTool = tool;
		mTransientTool->handleSelect();
	}
}

void LLToolMgr::clearTransientTool()
{
	if (mTransientTool)
	{
		mTransientTool->handleDeselect();
		mTransientTool = NULL;
		if (mCurrentTool)
		{
			mCurrentTool->handleSelect();
		}
		else
		{
			llwarns << "mCurrentTool is NULL" << llendl;
		}
	}
}


// The "gun tool", used for handling mouselook, captures the mouse and
// locks it within the window.  When the app loses focus we need to
// release this locking.
void LLToolMgr::onAppFocusLost()
{
	if (mCurrentTool 
		&& mCurrentTool == gToolGun)
	{
		mCurrentTool->handleDeselect();
	}
	mSavedTool = mCurrentTool;
	mCurrentTool = gToolNull;
}

void LLToolMgr::onAppFocusGained()
{
	if (mSavedTool)
	{
		if (mSavedTool == gToolGun)
		{
			mCurrentTool->handleSelect();
		}
		mCurrentTool = mSavedTool;
		mSavedTool = NULL;
	}
}

/////////////////////////////////////////////////////
// LLToolset

void LLToolset::addTool(LLTool* tool)
{
	llassert( !mToolList.checkData( tool ) ); // check for duplicates

	mToolList.addDataAtEnd( tool );
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
	LLTool *tool = mToolList.getNthData( index );
	if (tool)
	{
		mSelectedTool = tool;
		gToolMgr->setCurrentTool( tool );
	}
}

BOOL LLToolset::isToolSelected( S32 index )
{
	return (mToolList.getNthData( index ) == mSelectedTool);
}


void LLToolset::selectFirstTool()
{
	mSelectedTool = mToolList.getFirstData();
	gToolMgr->setCurrentTool( mSelectedTool );
}


void LLToolset::selectNextTool()
{
	LLTool* next = NULL;
	for( LLTool* cur = mToolList.getFirstData(); cur; cur = mToolList.getNextData() )
	{
		if( cur == mSelectedTool )
		{
			next = mToolList.getNextData();
			break;
		}
	}

	if( !next )
	{
		next = mToolList.getFirstData();
	}

	mSelectedTool = next;
	gToolMgr->setCurrentTool( mSelectedTool );
}

void LLToolset::selectPrevTool()
{
	LLTool* prev = NULL;
	for( LLTool* cur = mToolList.getLastData(); cur; cur = mToolList.getPreviousData() )
	{
		if( cur == mSelectedTool )
		{
			prev = mToolList.getPreviousData();
			break;
		}
	}

	if( !prev )
	{
		prev = mToolList.getLastData();
	}
	
	mSelectedTool = prev;
	gToolMgr->setCurrentTool( mSelectedTool );
}

void select_tool( void *tool_pointer )
{
	LLTool *tool = (LLTool *)tool_pointer;
	gCurrentToolset->selectTool( tool );
}
