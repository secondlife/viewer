/** 
 * @file lltool.cpp
 * @brief LLTool class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lltool.h"

#include "indra_constants.h"
#include "llerror.h"
#include "llview.h"

#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "lltoolcomp.h"
#include "lltoolfocus.h"
#include "llfocusmgr.h"
#include "llagent.h"
#include "llviewerjoystick.h"

extern BOOL gDebugClicks;

//static
const std::string LLTool::sNameNull("null");

LLTool::LLTool( const std::string& name, LLToolComposite* composite ) :
    mComposite( composite ),
    mName(name)
{
}

LLTool::~LLTool()
{
    if( hasMouseCapture() )
    {
        LL_WARNS() << "Tool deleted holding mouse capture.  Mouse capture removed." << LL_ENDL;
        gFocusMgr.removeMouseCaptureWithoutCallback( this );
    }
}

BOOL LLTool::handleAnyMouseClick(S32 x, S32 y, MASK mask, EMouseClickType clicktype, BOOL down)
{
    BOOL result = LLMouseHandler::handleAnyMouseClick(x, y, mask, clicktype, down);
    
    // This behavior was moved here from LLViewerWindow::handleAnyMouseClick, so it can be selectively overridden by LLTool subclasses.
    if(down && result)
    {
        // This is necessary to force clicks in the world to cause edit
        // boxes that might have keyboard focus to relinquish it, and hence
        // cause a commit to update their value.  JC
        gFocusMgr.setKeyboardFocus(NULL);
    }
    
    return result;
}

BOOL LLTool::handleMouseDown(S32 x, S32 y, MASK mask)
{
    if (gDebugClicks)
    {
        LL_INFOS() << "LLTool left mouse down" << LL_ENDL;
    }
    // by default, didn't handle it
    // AGENT_CONTROL_LBUTTON_DOWN is handled by scanMouse() and scanKey()
    // LL_INFOS() << "LLTool::handleMouseDown" << LL_ENDL;
    return FALSE;
}

BOOL LLTool::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if (gDebugClicks) 
    {
        LL_INFOS() << "LLTool left mouse up" << LL_ENDL;
    }
    // by default, didn't handle it
    // AGENT_CONTROL_LBUTTON_UP is handled by scanMouse() and scanKey()
    // LL_INFOS() << "LLTool::handleMouseUp" << LL_ENDL;
    return TRUE;
}

BOOL LLTool::handleHover(S32 x, S32 y, MASK mask)
{
    gViewerWindow->setCursor(UI_CURSOR_ARROW);
    LL_DEBUGS("UserInput") << "hover handled by a tool" << LL_ENDL;     
    // by default, do nothing, say we handled it
    return TRUE;
}

BOOL LLTool::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
    // by default, didn't handle it
    // LL_INFOS() << "LLTool::handleScrollWheel" << LL_ENDL;
    return FALSE;
}

BOOL LLTool::handleScrollHWheel(S32 x, S32 y, S32 clicks)
{
    // by default, didn't handle it
    return FALSE;
}

BOOL LLTool::handleDoubleClick(S32 x,S32 y,MASK mask)
{
    // LL_INFOS() << "LLTool::handleDoubleClick" << LL_ENDL;
    // by default, pretend it's a left click
    return FALSE;
}

BOOL LLTool::handleRightMouseDown(S32 x,S32 y,MASK mask)
{
    // by default, didn't handle it
    // LL_INFOS() << "LLTool::handleRightMouseDown" << LL_ENDL;
    return FALSE;
}

BOOL LLTool::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
    // by default, didn't handle it
    // LL_INFOS() << "LLTool::handleRightMouseDown" << LL_ENDL;
    return FALSE;
}
 
BOOL LLTool::handleMiddleMouseDown(S32 x,S32 y,MASK mask)
{
    // by default, didn't handle it
    // LL_INFOS() << "LLTool::handleMiddleMouseDown" << LL_ENDL;
    return FALSE;
}

BOOL LLTool::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
    // by default, didn't handle it
    // LL_INFOS() << "LLTool::handleMiddleMouseUp" << LL_ENDL;
    return FALSE;
}

BOOL LLTool::handleToolTip(S32 x, S32 y, MASK mask)
{
    // by default, didn't handle it
    // LL_INFOS() << "LLTool::handleToolTip" << LL_ENDL;
    return FALSE;
}

void LLTool::setMouseCapture( BOOL b )
{
    if( b )
    {
        gFocusMgr.setMouseCapture(mComposite ? mComposite : this );
    }
    else
    if( hasMouseCapture() )
    {
        gFocusMgr.setMouseCapture( NULL );
    }
}

// virtual
void LLTool::draw()
{ }

BOOL LLTool::hasMouseCapture()
{
    return gFocusMgr.getMouseCapture() == (mComposite ? mComposite : this);
}

BOOL LLTool::handleKey(KEY key, MASK mask)
{
    return FALSE;
}

LLTool* LLTool::getOverrideTool(MASK mask)
{
    // NOTE: if in flycam mode, ALT-ZOOM camera should be disabled
    if (LLViewerJoystick::getInstance()->getOverrideCamera())
    {
        return NULL;
    }

    static LLCachedControl<bool> alt_zoom(gSavedSettings, "EnableAltZoom", true);
    if (alt_zoom)
    {
        if (mask & MASK_ALT)
        {
            return LLToolCamera::getInstance();
        }
    }
    return NULL;
}
