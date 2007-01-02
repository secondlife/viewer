/** 
 * @file lltoolindividual.cpp
 * @brief LLToolIndividual class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//*****************************************************************************
//
// This is a tool for selecting individual object from the
// toolbox. Handy for when you want to deal with child object
// inventory...
//
//*****************************************************************************

#include "llviewerprecompiledheaders.h"
#include "lltoolindividual.h"

#include "llselectmgr.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llfloatertools.h"

///----------------------------------------------------------------------------
/// Globals
///----------------------------------------------------------------------------

LLToolIndividual* gToolIndividual = NULL;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------


///----------------------------------------------------------------------------
/// Class LLToolIndividual
///----------------------------------------------------------------------------

// Default constructor
LLToolIndividual::LLToolIndividual()
	: LLTool("Individual")
{
}

// Destroys the object
LLToolIndividual::~LLToolIndividual()
{
}

BOOL LLToolIndividual::handleMouseDown(S32 x, S32 y, MASK mask)
{
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

void LLToolIndividual::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* obj = gViewerWindow->lastObjectHit();
	gSelectMgr->deselectAll();
	if(obj)
	{
		gSelectMgr->selectObjectOnly(obj);
	}
}

BOOL LLToolIndividual::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if(!gSelectMgr->isEmpty())
	{
		// You should already have an object selected from the mousedown.
		// If so, show its inventory. 
		//gBuildView->showInventoryPanel();
		//gBuildView->showPanel(LLBuildView::PANEL_CONTENTS);
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
		return TRUE;
	}
	else
	{
		// Nothing selected means the first mouse click was probably
		// bad, so try again.
		return FALSE;
	}
}

void LLToolIndividual::handleSelect()
{
	LLViewerObject* obj = gSelectMgr->getFirstRootObject();
	if(!obj)
	{
		obj = gSelectMgr->getFirstObject();
	}
	gSelectMgr->deselectAll();
	if(obj)
	{
		gSelectMgr->selectObjectOnly(obj);
	}
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
