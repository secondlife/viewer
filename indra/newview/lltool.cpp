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
		llwarns << "Tool deleted holding mouse capture.  Mouse capture removed." << llendl;
		gFocusMgr.removeMouseCaptureWithoutCallback( this );
	}
}

BOOL LLTool::handleAnyMouseClick(S32 x, S32 y, MASK mask, LLMouseHandler::EClickType clicktype, BOOL down)
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
		llinfos << "LLTool left mouse down" << llendl;
	}
	// by default, didn't handle it
	// llinfos << "LLTool::handleMouseDown" << llendl;
	gAgent.setControlFlags(AGENT_CONTROL_LBUTTON_DOWN);
	return TRUE;
}

BOOL LLTool::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (gDebugClicks) 
	{
		llinfos << "LLTool left mouse up" << llendl;
	}
	// by default, didn't handle it
	// llinfos << "LLTool::handleMouseUp" << llendl;
	gAgent.setControlFlags(AGENT_CONTROL_LBUTTON_UP);
	return TRUE;
}

BOOL LLTool::handleHover(S32 x, S32 y, MASK mask)
{
	gViewerWindow->setCursor(UI_CURSOR_ARROW);
	lldebugst(LLERR_USER_INPUT) << "hover handled by a tool" << llendl;		
	// by default, do nothing, say we handled it
	return TRUE;
}

BOOL LLTool::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	// by default, didn't handle it
	// llinfos << "LLTool::handleScrollWheel" << llendl;
	return FALSE;
}

BOOL LLTool::handleDoubleClick(S32 x,S32 y,MASK mask)
{
	// llinfos << "LLTool::handleDoubleClick" << llendl;
	// by default, pretend it's a left click
	return FALSE;
}

BOOL LLTool::handleRightMouseDown(S32 x,S32 y,MASK mask)
{
	// by default, didn't handle it
	// llinfos << "LLTool::handleRightMouseDown" << llendl;
	return FALSE;
}

BOOL LLTool::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	// by default, didn't handle it
	// llinfos << "LLTool::handleRightMouseDown" << llendl;
	return FALSE;
}
 
BOOL LLTool::handleMiddleMouseDown(S32 x,S32 y,MASK mask)
{
	// by default, didn't handle it
	// llinfos << "LLTool::handleMiddleMouseDown" << llendl;
	return FALSE;
}

BOOL LLTool::handleMiddleMouseUp(S32 x, S32 y, MASK mask)
{
	// by default, didn't handle it
	// llinfos << "LLTool::handleMiddleMouseUp" << llendl;
	return FALSE;
}

BOOL LLTool::handleToolTip(S32 x, S32 y, MASK mask)
{
	// by default, didn't handle it
	// llinfos << "LLTool::handleToolTip" << llendl;
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
	if (gSavedSettings.getBOOL("EnableAltZoom"))
	{
		if (mask & MASK_ALT)
		{
			return LLToolCamera::getInstance();
		}
	}
	return NULL;
}
