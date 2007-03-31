/** 
 * @file lltool.cpp
 * @brief LLTool class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "llviewborder.h"

extern BOOL gDebugClicks;

//static
const LLString LLTool::sNameNull("null");

LLTool::LLTool( const LLString& name, LLToolComposite* composite ) :
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

BOOL LLTool::handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen)
{
	// by default, didn't handle it
	// llinfos << "LLTool::handleToolTip" << llendl;
	return FALSE;
}

void LLTool::setMouseCapture( BOOL b )
{
	if( b )
	{
		gViewerWindow->setMouseCapture(mComposite ? mComposite : this );
	}
	else
	if( hasMouseCapture() )
	{
		gViewerWindow->setMouseCapture( NULL );
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
	if (mask & MASK_ALT)
	{
		return gToolCamera;
	}
	return NULL;
}
