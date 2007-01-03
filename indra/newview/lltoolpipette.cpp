/** 
 * @file lltoolpipette.cpp
 * @brief LLToolPipette class implementation
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * A tool to pick texture entry infro from objects in world (color/texture)
 */

#include "llviewerprecompiledheaders.h"

// File includes
#include "lltoolpipette.h" 

// Library includes

// Viewer includes
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"

// Globals
LLToolPipette *gToolPipette = NULL;

//
// Member functions
//

LLToolPipette::LLToolPipette()
:	LLTool("Pipette"),
mSuccess(TRUE)
{ 
	mSelectCallback = NULL;
	mUserData = NULL;
}


LLToolPipette::~LLToolPipette()
{ }


BOOL LLToolPipette::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mSuccess = TRUE;
	mTooltipMsg.clear();
	gPickFaces = TRUE;
	setMouseCapture(TRUE);
	gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
	return TRUE;
}

BOOL LLToolPipette::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mSuccess = TRUE;
	gSelectMgr->unhighlightAll();
	// *NOTE: This assumes the pipette tool is a transient tool.
	gToolMgr->clearTransientTool();
	setMouseCapture(FALSE);
	return TRUE;
}

BOOL LLToolPipette::handleHover(S32 x, S32 y, MASK mask)
{
	gViewerWindow->setCursor(mSuccess ? UI_CURSOR_PIPETTE : UI_CURSOR_NO);
	if (hasMouseCapture()) // mouse button is down
	{
		gPickFaces = TRUE;
		gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback);
		return TRUE;
	}
	return FALSE;
}

BOOL LLToolPipette::handleToolTip(S32 x, S32 y, LLString& msg, LLRect *sticky_rect_screen)
{
	if (mTooltipMsg.empty())
	{
		return FALSE;
	}
	// keep tooltip message up when mouse in this part of screen
	sticky_rect_screen->setCenterAndSize(x, y, 20, 20);
	msg = mTooltipMsg;
	return TRUE;
}

void LLToolPipette::pickCallback(S32 x, S32 y, MASK mask)
{
	LLViewerObject* hit_obj	= gViewerWindow->lastObjectHit();
	gSelectMgr->unhighlightAll();

	// if we clicked on a face of a valid prim, save off texture entry data
	if (hit_obj && 
		hit_obj->getPCode() == LL_PCODE_VOLUME &&
		gLastHitObjectFace != -1)
	{
		//TODO: this should highlight the selected face only
		gSelectMgr->highlightObjectOnly(hit_obj);
		gToolPipette->mTextureEntry = *hit_obj->getTE(gLastHitObjectFace);
		if (gToolPipette->mSelectCallback)
		{
			gToolPipette->mSelectCallback(gToolPipette->mTextureEntry, gToolPipette->mUserData);
		}
	}
}

void LLToolPipette::setSelectCallback(select_callback callback, void* user_data)
{
	mSelectCallback = callback;
	mUserData = user_data;
}

void LLToolPipette::setResult(BOOL success, const LLString& msg)
{
	mTooltipMsg = msg;
	mSuccess = success;
}
