/** 
 * @file lltoolobjpicker.cpp
 * @brief LLToolObjPicker class implementation
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

// LLToolObjPicker is a transient tool, useful for a single object pick.

#include "llviewerprecompiledheaders.h"

#include "lltoolobjpicker.h"

#include "llagent.h"
#include "llselectmgr.h"
#include "llworld.h"
#include "viewer.h"				// for gFPSClamped, pie menus
#include "llviewercontrol.h"
#include "llmenugl.h"
#include "lltoolmgr.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewermenu.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "lldrawable.h"

LLToolObjPicker* gToolObjPicker = NULL;

LLToolObjPicker::LLToolObjPicker()
:	LLTool( "ObjPicker", NULL ),
	mPicked( FALSE ),
	mHitObjectID( LLUUID::null ),
	mExitCallback( NULL ),
	mExitCallbackData( NULL )
{ }


// returns TRUE if an object was selected 
BOOL LLToolObjPicker::handleMouseDown(S32 x, S32 y, MASK mask)
{
	LLView* viewp = gViewerWindow->getRootView();
	BOOL handled = viewp->handleMouseDown(x, y, mask);

	mHitObjectID.setNull();

	if (! handled)
	{
		// didn't click in any UI object, so must have clicked in the world
		gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
		handled = TRUE;
	}
	else
	{
		if (hasMouseCapture())
		{
			setMouseCapture(FALSE);
		}
		else
		{
			llwarns << "PickerTool doesn't have mouse capture on mouseDown" << llendl;	
		}
	}

	// Pass mousedown to base class
	LLTool::handleMouseDown(x, y, mask);

	return handled;
}

void LLToolObjPicker::pickCallback(S32 x, S32 y, MASK mask)
{
	// You must hit the body for this tool to think you hit the object.
	LLViewerObject*	objectp = NULL;
	objectp = gObjectList.findObject( gLastHitObjectID );
	if (objectp)
	{
		gToolObjPicker->mHitObjectID = objectp->mID;
		gToolObjPicker->mPicked = TRUE;
	}
}


BOOL LLToolObjPicker::handleMouseUp(S32 x, S32 y, MASK mask)
{
	LLView* viewp = gViewerWindow->getRootView();
	BOOL handled = viewp->handleHover(x, y, mask);
	if (handled)
	{
		// let UI handle this
	}

	LLTool::handleMouseUp(x, y, mask);
	if (hasMouseCapture())
	{
		setMouseCapture(FALSE);
	}
	else
	{
		llwarns << "PickerTool doesn't have mouse capture on mouseUp" << llendl;	
	}
	return handled;
}


BOOL LLToolObjPicker::handleHover(S32 x, S32 y, MASK mask)
{
	LLView *viewp = gViewerWindow->getRootView();
	BOOL handled = viewp->handleHover(x, y, mask);
	if (!handled) 
	{
		// Used to do pick on hover.  Now we just always display the cursor.
		ECursorType cursor = UI_CURSOR_ARROWLOCKED;

		cursor = UI_CURSOR_TOOLPICKOBJECT3;

		gViewerWindow->getWindow()->setCursor(cursor);
	}
	return handled;
}


void LLToolObjPicker::onMouseCaptureLost()
{
	if (mExitCallback)
	{
		mExitCallback(mExitCallbackData);

		mExitCallback = NULL;
		mExitCallbackData = NULL;
	}

	mPicked = FALSE;
	mHitObjectID.setNull();
}

// virtual
void LLToolObjPicker::setExitCallback(void (*callback)(void *), void *callback_data)
{
	mExitCallback = callback;
	mExitCallbackData = callback_data;
}

// virtual
void LLToolObjPicker::handleSelect()
{
	LLTool::handleSelect();
	setMouseCapture(TRUE);
}

// virtual
void LLToolObjPicker::handleDeselect()
{
	if (hasMouseCapture())
	{
		LLTool::handleDeselect();
		setMouseCapture(FALSE);
	}
}


