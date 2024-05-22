/**
 * @file llwindowcallbacks.cpp
 * @brief OS event callback class
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

#include "linden_common.h"

#include "llwindowcallbacks.h"

//
// LLWindowCallbacks
//

bool LLWindowCallbacks::handleTranslatedKeyDown(const KEY key, const MASK mask, bool repeated)
{
    return false;
}


bool LLWindowCallbacks::handleTranslatedKeyUp(const KEY key, const MASK mask)
{
    return false;
}

void LLWindowCallbacks::handleScanKey(KEY key, bool key_down, bool key_up, bool key_level)
{
}

bool LLWindowCallbacks::handleUnicodeChar(llwchar uni_char, MASK mask)
{
    return false;
}


bool LLWindowCallbacks::handleMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
    return false;
}

bool LLWindowCallbacks::handleMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
    return false;
}

void LLWindowCallbacks::handleMouseLeave(LLWindow *window)
{
    return;
}

bool LLWindowCallbacks::handleCloseRequest(LLWindow *window)
{
    //allow the window to close
    return true;
}

void LLWindowCallbacks::handleQuit(LLWindow *window)
{
}

bool LLWindowCallbacks::handleRightMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
    return false;
}

bool LLWindowCallbacks::handleRightMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
    return false;
}

bool LLWindowCallbacks::handleMiddleMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
    return false;
}

bool LLWindowCallbacks::handleMiddleMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
    return false;
}

bool LLWindowCallbacks::handleOtherMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask, S32 button)
{
    return false;
}

bool LLWindowCallbacks::handleOtherMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask, S32 button)
{
    return false;
}

bool LLWindowCallbacks::handleActivate(LLWindow *window, bool activated)
{
    return false;
}

bool LLWindowCallbacks::handleActivateApp(LLWindow *window, bool activating)
{
    return false;
}

void LLWindowCallbacks::handleMouseMove(LLWindow *window, const LLCoordGL pos, MASK mask)
{
}

void LLWindowCallbacks::handleMouseDragged(LLWindow *window, const LLCoordGL pos, MASK mask)
{
}

void LLWindowCallbacks::handleScrollWheel(LLWindow *window, S32 clicks)
{
}

void LLWindowCallbacks::handleScrollHWheel(LLWindow *window, S32 clicks)
{
}

void LLWindowCallbacks::handleResize(LLWindow *window, const S32 width, const S32 height)
{
}

void LLWindowCallbacks::handleFocus(LLWindow *window)
{
     LL_WARNS("COCOA") << "Called handleFocus proto" << LL_ENDL;
}

void LLWindowCallbacks::handleFocusLost(LLWindow *window)
{
}

void LLWindowCallbacks::handleMenuSelect(LLWindow *window, const S32 menu_item)
{
}

bool LLWindowCallbacks::handlePaint(LLWindow *window, const S32 x, const S32 y,
                                    const S32 width, const S32 height)
{
    return false;
}

bool LLWindowCallbacks::handleDoubleClick(LLWindow *window, const LLCoordGL pos, MASK mask)
{
    return false;
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

bool LLWindowCallbacks::handleTimerEvent(LLWindow *window)
{
    return false;
}

bool LLWindowCallbacks::handleDeviceChange(LLWindow *window)
{
    return false;
}

bool LLWindowCallbacks::handleDPIChanged(LLWindow *window, F32 ui_scale_factor, S32 window_width, S32 window_height)
{
    return false;
}

bool LLWindowCallbacks::handleWindowDidChangeScreen(LLWindow *window)
{
    return false;
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
