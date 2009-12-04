/** 
 * @file llwindowcallbacks.cpp
 * @brief OS event callback class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llwindowcallbacks.h"

#include "llcoord.h"

//
// LLWindowCallbacks
//

BOOL LLWindowCallbacks::handleTranslatedKeyDown(const KEY key, const MASK mask, BOOL repeated)
{
	return FALSE;
}


BOOL LLWindowCallbacks::handleTranslatedKeyUp(const KEY key, const MASK mask)
{
	return FALSE;
}

void LLWindowCallbacks::handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level)
{
}

BOOL LLWindowCallbacks::handleUnicodeChar(llwchar uni_char, MASK mask)
{
	return FALSE;
}


BOOL LLWindowCallbacks::handleMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

void LLWindowCallbacks::handleMouseLeave(LLWindow *window)
{
	return;
}

BOOL LLWindowCallbacks::handleCloseRequest(LLWindow *window)
{
	//allow the window to close
	return TRUE;
}

void LLWindowCallbacks::handleQuit(LLWindow *window)
{
}

BOOL LLWindowCallbacks::handleRightMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleRightMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleMiddleMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleMiddleMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleActivate(LLWindow *window, BOOL activated)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleActivateApp(LLWindow *window, BOOL activating)
{
	return FALSE;
}

void LLWindowCallbacks::handleMouseMove(LLWindow *window, const LLCoordGL pos, MASK mask)
{
}

void LLWindowCallbacks::handleScrollWheel(LLWindow *window, S32 clicks)
{
}

void LLWindowCallbacks::handleResize(LLWindow *window, const S32 width, const S32 height)
{
}

void LLWindowCallbacks::handleFocus(LLWindow *window)
{
}

void LLWindowCallbacks::handleFocusLost(LLWindow *window)
{
}

void LLWindowCallbacks::handleMenuSelect(LLWindow *window, const S32 menu_item)
{
}

BOOL LLWindowCallbacks::handlePaint(LLWindow *window, const S32 x, const S32 y, 
									const S32 width, const S32 height)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleDoubleClick(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

void LLWindowCallbacks::handleWindowBlock(LLWindow *window)
{
}

void LLWindowCallbacks::handleWindowUnblock(LLWindow *window)
{
}

void LLWindowCallbacks::handleDataCopy(LLWindow *window, S32 data_type, void *data)
{
}

LLWindowCallbacks::DragNDropResult LLWindowCallbacks::handleDragNDrop(LLWindow *window, LLCoordGL pos, MASK mask, DragNDropAction action, std::string data )
{
	return LLWindowCallbacks::DND_NONE;
}

BOOL LLWindowCallbacks::handleTimerEvent(LLWindow *window)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleDeviceChange(LLWindow *window)
{
	return FALSE;
}

void LLWindowCallbacks::handlePingWatchdog(LLWindow *window, const char * msg)
{

}

void LLWindowCallbacks::handlePauseWatchdog(LLWindow *window)
{

}

void LLWindowCallbacks::handleResumeWatchdog(LLWindow *window)
{

}

std::string LLWindowCallbacks::translateString(const char* tag)
{
    return std::string();
}

//virtual
std::string LLWindowCallbacks::translateString(const char* tag,
		const std::map<std::string, std::string>& args)
{
	return std::string();
}
