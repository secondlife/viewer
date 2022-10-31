/** 
 * @file llmousehandler.cpp
 * @brief LLMouseHandler class implementation
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

#include "llmousehandler.h"

//virtual
BOOL LLMouseHandler::handleAnyMouseClick(S32 x, S32 y, MASK mask, EMouseClickType clicktype, BOOL down)
{
    BOOL handled = FALSE;
    if (down)
    {
        switch (clicktype)
        {
        case CLICK_LEFT: handled = handleMouseDown(x, y, mask); break;
        case CLICK_RIGHT: handled = handleRightMouseDown(x, y, mask); break;
        case CLICK_MIDDLE: handled = handleMiddleMouseDown(x, y, mask); break;
        case CLICK_DOUBLELEFT: handled = handleDoubleClick(x, y, mask); break;
        case CLICK_BUTTON4:
        case CLICK_BUTTON5:
            LL_INFOS() << "Handle mouse button " << clicktype + 1 << " down." << LL_ENDL;
            break;
        default:
            LL_WARNS() << "Unhandled enum." << LL_ENDL;
        }
    }
    else
    {
        switch (clicktype)
        {
        case CLICK_LEFT: handled = handleMouseUp(x, y, mask); break;
        case CLICK_RIGHT: handled = handleRightMouseUp(x, y, mask); break;
        case CLICK_MIDDLE: handled = handleMiddleMouseUp(x, y, mask); break;
        case CLICK_DOUBLELEFT: handled = handleDoubleClick(x, y, mask); break;
        case CLICK_BUTTON4:
        case CLICK_BUTTON5:
            LL_INFOS() << "Handle mouse button " << clicktype + 1 << " up." << LL_ENDL;
            break;
        default:
            LL_WARNS() << "Unhandled enum." << LL_ENDL;
        }
    }
    return handled;
}
