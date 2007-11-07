/** 
 * @file lltoolselectrect.cpp
 * @brief A tool to select multiple objects with a screen-space rectangle.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

// File includes
#include "lltoolselectrect.h"

// Library includes
#include "llgl.h"
#include "lldarray.h"

// Viewer includes
#include "llviewercontrol.h"
#include "llui.h"
#include "llselectmgr.h"
#include "lltoolview.h"
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
	// start dragging rectangle
	setMouseCapture( TRUE );

	mDragStartX = x;
	mDragStartY = y;
	mDragEndX = x;
	mDragEndY = y;

	mMouseOutsideSlop = FALSE;

	LLToolSelect::handleMouseDown(x, y, mask);
	return TRUE;
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
			gSelectMgr->deselectHighlightedObjects();
		}
		else
		{
			gSelectMgr->selectHighlightedObjects();
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
				gSelectMgr->deselectAll();
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

		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolSelectRect (active)" << llendl;		
	}
	else
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolSelectRect (inactive)" << llendl;		
	}

	gViewerWindow->getWindow()->setCursor(UI_CURSOR_ARROW);
	return TRUE;
}


void LLToolSelectRect::draw()
{
	if(	hasMouseCapture() && mMouseOutsideSlop)
	{
		if (gKeyboard->currentMask(TRUE) == MASK_CONTROL)
		{
			glColor4f(1.f, 0.f, 0.f, 1.f);
		}
		else
		{
			glColor4f(1.f, 1.f, 0.f, 1.f);
		}
		LLGLSNoTexture gls_no_texture;
		gl_rect_2d(
			llmin(mDragStartX, mDragEndX),
			llmax(mDragStartY, mDragEndY),
			llmax(mDragStartX, mDragEndX),
			llmin(mDragStartY, mDragEndY),
			FALSE);
		if (gKeyboard->currentMask(TRUE) == MASK_CONTROL)
		{
			glColor4f(1.f, 0.f, 0.f, 0.1f);
		}
		else
		{
			glColor4f(1.f, 1.f, 0.f, 0.1f);
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
