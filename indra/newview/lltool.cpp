/** 
 * @file lltool.cpp
 * @brief LLTool class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lltool.h"

#include "indra_constants.h"
#include "llerror.h"
#include "llview.h"

#include "llviewerwindow.h"
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
	gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROW);
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

BOOL LLTool::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
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
	if (mask & MASK_ALT)
	{
		return LLToolCamera::getInstance();
	}
	return NULL;
}
