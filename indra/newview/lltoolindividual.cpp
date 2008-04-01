/** 
 * @file lltoolindividual.cpp
 * @brief LLToolIndividual class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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
	LLSelectMgr::getInstance()->deselectAll();
	if(obj)
	{
		LLSelectMgr::getInstance()->selectObjectOnly(obj);
	}
}

BOOL LLToolIndividual::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if(!LLSelectMgr::getInstance()->getSelection()->isEmpty())
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
	const BOOL children_ok = TRUE;
	LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(children_ok);
	LLSelectMgr::getInstance()->deselectAll();
	if(obj)
	{
		LLSelectMgr::getInstance()->selectObjectOnly(obj);
	}
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
