/** 
 * @file lltoolpipette.cpp
 * @brief LLToolPipette class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

/**
 * A tool to pick texture entry infro from objects in world (color/texture)
 */

#include "llviewerprecompiledheaders.h"

// File includes
#include "lltoolpipette.h" 

// Library includes
#include "lltooltip.h"

// Viewer includes
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"

//
// Member functions
//

LLToolPipette::LLToolPipette()
:	LLTool(std::string("Pipette")),
	mSuccess(TRUE)
{ 
}


LLToolPipette::~LLToolPipette()
{ }


BOOL LLToolPipette::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mSuccess = TRUE;
	mTooltipMsg.clear();
	setMouseCapture(TRUE);
	gViewerWindow->pickAsync(x, y, mask, pickCallback);
	return TRUE;
}

BOOL LLToolPipette::handleMouseUp(S32 x, S32 y, MASK mask)
{
	mSuccess = TRUE;
	LLSelectMgr::getInstance()->unhighlightAll();
	// *NOTE: This assumes the pipette tool is a transient tool.
	LLToolMgr::getInstance()->clearTransientTool();
	setMouseCapture(FALSE);
	return TRUE;
}

BOOL LLToolPipette::handleHover(S32 x, S32 y, MASK mask)
{
	gViewerWindow->setCursor(mSuccess ? UI_CURSOR_PIPETTE : UI_CURSOR_NO);
	if (hasMouseCapture()) // mouse button is down
	{
		gViewerWindow->pickAsync(x, y, mask, pickCallback);
		return TRUE;
	}
	return FALSE;
}

BOOL LLToolPipette::handleToolTip(S32 x, S32 y, std::string& msg, LLRect &sticky_rect_screen)
{
	if (mTooltipMsg.empty())
	{
		return FALSE;
	}

	LLRect sticky_rect;
	sticky_rect.setCenterAndSize(x, y, 20, 20);
	LLToolTipMgr::instance().show(LLToolTipParams()
		.message(mTooltipMsg)
		.sticky_rect(sticky_rect));

	return TRUE;
}

void LLToolPipette::setTextureEntry(const LLTextureEntry* entry)
{
	if (entry)
	{
		mTextureEntry = *entry;
		mSignal(mTextureEntry);
	}
}

void LLToolPipette::pickCallback(const LLPickInfo& pick_info)
{
	LLViewerObject* hit_obj	= pick_info.getObject();
	LLSelectMgr::getInstance()->unhighlightAll();

	// if we clicked on a face of a valid prim, save off texture entry data
	if (hit_obj && 
		hit_obj->getPCode() == LL_PCODE_VOLUME &&
		pick_info.mObjectFace != -1)
	{
		//TODO: this should highlight the selected face only
		LLSelectMgr::getInstance()->highlightObjectOnly(hit_obj);
		const LLTextureEntry* entry = hit_obj->getTE(pick_info.mObjectFace);
		LLToolPipette::getInstance()->setTextureEntry(entry);
	}
}

void LLToolPipette::setResult(BOOL success, const std::string& msg)
{
	mTooltipMsg = msg;
	mSuccess = success;
}
