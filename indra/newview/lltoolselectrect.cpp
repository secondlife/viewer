/** 
 * @file lltoolselectrect.cpp
 * @brief A tool to select multiple objects with a screen-space rectangle.
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

// File includes
#include "lltoolselectrect.h"

// Library includes
#include "llgl.h"
#include "llrender.h"

// Viewer includes
#include "llviewercontrol.h"
#include "llui.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"

#include "llglheaders.h"

// Globals
const S32 SLOP_RADIUS = 5;


//
// Member functions
//

LLToolSelectRect::LLToolSelectRect( LLToolComposite* composite )
	:
	LLToolSelect( composite ),
	mDragStartX(0),
	mDragStartY(0),
	mDragEndX(0),
	mDragEndY(0),
	mDragLastWidth(0),
	mDragLastHeight(0),
	mMouseOutsideSlop(FALSE)

{ }


void dialog_refresh_all(void);

BOOL LLToolSelectRect::handleMouseDown(S32 x, S32 y, MASK mask)
{
    BOOL pick_rigged = false; //gSavedSettings.getBOOL("AnimatedObjectsAllowLeftClick");
	handlePick(gViewerWindow->pickImmediate(x, y, TRUE /* pick_transparent */, pick_rigged));

	LLTool::handleMouseDown(x, y, mask);

	return mPick.getObject().notNull();
}

void LLToolSelectRect::handlePick(const LLPickInfo& pick)
{
	mPick = pick;

	// start dragging rectangle
	setMouseCapture( TRUE );

	mDragStartX = pick.mMousePt.mX;
	mDragStartY = pick.mMousePt.mY;
	mDragEndX = pick.mMousePt.mX;
	mDragEndY = pick.mMousePt.mY;

	mMouseOutsideSlop = FALSE;
}


BOOL LLToolSelectRect::handleMouseUp(S32 x, S32 y, MASK mask)
{
	setMouseCapture( FALSE );

	if(	mMouseOutsideSlop )
	{
		mDragLastWidth = 0;
		mDragLastHeight = 0;

		mMouseOutsideSlop = FALSE;
		
		if (mask == MASK_CONTROL)
		{
			LLSelectMgr::getInstance()->deselectHighlightedObjects();
		}
		else
		{
			LLSelectMgr::getInstance()->selectHighlightedObjects();
		}
		return TRUE;
	}
	else
	{
		return LLToolSelect::handleMouseUp(x, y, mask);
	}
}


BOOL LLToolSelectRect::handleHover(S32 x, S32 y, MASK mask)
{
	if(	hasMouseCapture() )
	{
		if (mMouseOutsideSlop || outsideSlop(x, y, mDragStartX, mDragStartY))
		{
			if (!mMouseOutsideSlop && !(mask & MASK_SHIFT) && !(mask & MASK_CONTROL))
			{
				// just started rect select, and not adding to current selection
				LLSelectMgr::getInstance()->deselectAll();
			}
			mMouseOutsideSlop = TRUE;
			mDragEndX = x;
			mDragEndY = y;

			handleRectangleSelection(x, y, mask);
		}
		else
		{
			return LLToolSelect::handleHover(x, y, mask);
		}

		LL_DEBUGS("UserInput") << "hover handled by LLToolSelectRect (active)" << LL_ENDL;		
	}
	else
	{
		LL_DEBUGS("UserInput") << "hover handled by LLToolSelectRect (inactive)" << LL_ENDL;		
	}

	gViewerWindow->setCursor(UI_CURSOR_ARROW);
	return TRUE;
}


void LLToolSelectRect::draw()
{
	if(	hasMouseCapture() && mMouseOutsideSlop)
	{
		if (gKeyboard->currentMask(TRUE) == MASK_CONTROL)
		{
			gGL.color4f(1.f, 0.f, 0.f, 1.f);
		}
		else
		{
			gGL.color4f(1.f, 1.f, 0.f, 1.f);
		}
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gl_rect_2d(
			llmin(mDragStartX, mDragEndX),
			llmax(mDragStartY, mDragEndY),
			llmax(mDragStartX, mDragEndX),
			llmin(mDragStartY, mDragEndY),
			FALSE);
		if (gKeyboard->currentMask(TRUE) == MASK_CONTROL)
		{
			gGL.color4f(1.f, 0.f, 0.f, 0.1f);
		}
		else
		{
			gGL.color4f(1.f, 1.f, 0.f, 0.1f);
		}
		gl_rect_2d(
			llmin(mDragStartX, mDragEndX),
			llmax(mDragStartY, mDragEndY),
			llmax(mDragStartX, mDragEndX),
			llmin(mDragStartY, mDragEndY));
	}
}

// true if x,y outside small box around start_x,start_y
BOOL LLToolSelectRect::outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y)
{
	S32 dx = x - start_x;
	S32 dy = y - start_y;

	return (dx <= -SLOP_RADIUS || SLOP_RADIUS <= dx || dy <= -SLOP_RADIUS || SLOP_RADIUS <= dy);
}


//
// Static functions
//
