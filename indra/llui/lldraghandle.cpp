/** 
 * @file lldraghandle.cpp
 * @brief LLDragHandle base class
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

// A widget for dragging a view around the screen using the mouse.

#include "linden_common.h"

#include "lldraghandle.h"

#include "llmath.h"

//#include "llviewerwindow.h"
#include "llui.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "llcontrol.h"
#include "llresmgr.h"
#include "llfontgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"

const S32 LEADING_PAD = 5;
const S32 TITLE_PAD = 8;
const S32 BORDER_PAD = 1;
const S32 LEFT_PAD = BORDER_PAD + TITLE_PAD + LEADING_PAD;
const S32 RIGHT_PAD = BORDER_PAD + 32; // HACK: space for close btn and minimize btn

S32 LLDragHandle::sSnapMargin = 5;

LLDragHandle::LLDragHandle( const std::string& name, const LLRect& rect, const std::string& title )
:	LLView( name, rect, TRUE ),
	mDragLastScreenX( 0 ),
	mDragLastScreenY( 0 ),
	mLastMouseScreenX( 0 ),
	mLastMouseScreenY( 0 ),
	mDragHighlightColor(	LLUI::sColorsGroup->getColor( "DefaultHighlightLight" ) ),
	mDragShadowColor(		LLUI::sColorsGroup->getColor( "DefaultShadowDark" ) ),
	mTitleBox( NULL ),
	mMaxTitleWidth( 0 ),
	mForeground( TRUE )
{
	sSnapMargin = LLUI::sConfigGroup->getS32("SnapMargin");

	setSaveToXML(false);
}

void LLDragHandle::setTitleVisible(BOOL visible) 
{ 
	if(mTitleBox)
	{
		mTitleBox->setVisible(visible); 
	}
}

void LLDragHandle::setTitleBox(LLTextBox* titlebox)
{	
	if( mTitleBox )
	{
		removeChild(mTitleBox);
		delete mTitleBox;
	}
	mTitleBox = titlebox;
	if(mTitleBox)
	{
		addChild( mTitleBox );
	}
}

LLDragHandleTop::LLDragHandleTop(const std::string& name, const LLRect &rect, const std::string& title)
:	LLDragHandle(name, rect, title)
{
	setFollowsAll();
	setTitle( title );
}

LLDragHandleLeft::LLDragHandleLeft(const std::string& name, const LLRect &rect, const std::string& title)
:	LLDragHandle(name, rect, title)
{
	setFollowsAll();
	setTitle( title );
}

void LLDragHandleTop::setTitle(const std::string& title)
{
	std::string trimmed_title = title;
	LLStringUtil::trim(trimmed_title);

	const LLFontGL* font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF );
	LLTextBox* titlebox = new LLTextBox( std::string("Drag Handle Title"), getRect(), trimmed_title, font );
	titlebox->setFollows(FOLLOWS_TOP | FOLLOWS_LEFT | FOLLOWS_RIGHT);
	titlebox->setFontStyle(LLFontGL::DROP_SHADOW_SOFT);
	
	setTitleBox(titlebox);
	reshapeTitleBox();
}


const std::string& LLDragHandleTop::getTitle() const
{
	return getTitleBox() == NULL ? LLStringUtil::null : getTitleBox()->getText();
}


void LLDragHandleLeft::setTitle(const std::string& )
{
	setTitleBox(NULL);
	/* no title on left edge */
}


const std::string& LLDragHandleLeft::getTitle() const
{
	return LLStringUtil::null;
}


void LLDragHandleTop::draw()
{
	/* Disable lines.  Can drag anywhere in most windows.  JC
	if( getVisible() && getEnabled() && mForeground) 
	{
		const S32 BORDER_PAD = 2;
		const S32 HPAD = 2;
		const S32 VPAD = 2;
		S32 left = BORDER_PAD + HPAD;
		S32 top = getRect().getHeight() - 2 * VPAD;
		S32 right = getRect().getWidth() - HPAD;
//		S32 bottom = VPAD;

		// draw lines for drag areas

		const S32 LINE_SPACING = (DRAG_HANDLE_HEIGHT - 2 * VPAD) / 4;
		S32 line = top - LINE_SPACING;

		LLRect title_rect = mTitleBox->getRect();
		S32 title_right = title_rect.mLeft + mTitleWidth;
		BOOL show_right_side = title_right < getRect().getWidth();

		for( S32 i=0; i<4; i++ )
		{
			gl_line_2d(left, line+1, title_rect.mLeft - LEADING_PAD, line+1, mDragHighlightColor);
			if( show_right_side )
			{
				gl_line_2d(title_right, line+1, right, line+1, mDragHighlightColor);
			}

			gl_line_2d(left, line, title_rect.mLeft - LEADING_PAD, line, mDragShadowColor);
			if( show_right_side )
			{
				gl_line_2d(title_right, line, right, line, mDragShadowColor);
			}
			line -= LINE_SPACING;
		}
	}
	*/

	// Colorize the text to match the frontmost state
	if (getTitleBox())
	{
		getTitleBox()->setEnabled(getForeground());
	}

	LLView::draw();
}


// assumes GL state is set for 2D
void LLDragHandleLeft::draw()
{
	/* Disable lines.  Can drag anywhere in most windows. JC
	if( getVisible() && getEnabled() && mForeground ) 
	{
		const S32 BORDER_PAD = 2;
//		const S32 HPAD = 2;
		const S32 VPAD = 2;
		const S32 LINE_SPACING = 3;

		S32 left = BORDER_PAD + LINE_SPACING;
		S32 top = getRect().getHeight() - 2 * VPAD;
//		S32 right = getRect().getWidth() - HPAD;
		S32 bottom = VPAD;
 
		// draw lines for drag areas

		// no titles yet
		//LLRect title_rect = mTitleBox->getRect();
		//S32 title_right = title_rect.mLeft + mTitleWidth;
		//BOOL show_right_side = title_right < getRect().getWidth();

		S32 line = left;
		for( S32 i=0; i<4; i++ )
		{
			gl_line_2d(line, top, line, bottom, mDragHighlightColor);

			gl_line_2d(line+1, top, line+1, bottom, mDragShadowColor);

			line += LINE_SPACING;
		}
	}
	*/

	// Colorize the text to match the frontmost state
	if (getTitleBox())
	{
		getTitleBox()->setEnabled(getForeground());
	}

	LLView::draw();
}

void LLDragHandleTop::reshapeTitleBox()
{
	if( ! getTitleBox())
	{
		return;
	}
	const LLFontGL* font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF );
	S32 title_width = font->getWidth( getTitleBox()->getText() ) + TITLE_PAD;
	if (getMaxTitleWidth() > 0)
		title_width = llmin(title_width, getMaxTitleWidth());
	S32 title_height = llround(font->getLineHeight());
	LLRect title_rect;
	title_rect.setLeftTopAndSize( 
		LEFT_PAD, 
		getRect().getHeight() - BORDER_PAD,
		getRect().getWidth() - LEFT_PAD - RIGHT_PAD,
		title_height);

	getTitleBox()->setRect( title_rect );
}

void LLDragHandleTop::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);
	reshapeTitleBox();
}

void LLDragHandleLeft::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape(width, height, called_from_parent);
}

//-------------------------------------------------------------
// UI event handling
//-------------------------------------------------------------

BOOL LLDragHandle::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	// No handler needed for focus lost since this clas has no state that depends on it.
	gFocusMgr.setMouseCapture(this);

	localPointToScreen(x, y, &mDragLastScreenX, &mDragLastScreenY);
	mLastMouseScreenX = mDragLastScreenX;
	mLastMouseScreenY = mDragLastScreenY;

	// Note: don't pass on to children
	return TRUE;
}


BOOL LLDragHandle::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );
	}

	// Note: don't pass on to children
	return TRUE;
}


BOOL LLDragHandle::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);

		// Resize the parent
		S32 delta_x = screen_x - mDragLastScreenX;
		S32 delta_y = screen_y - mDragLastScreenY;

		LLRect original_rect = getParent()->getRect();
		LLRect translated_rect = getParent()->getRect();
		translated_rect.translate(delta_x, delta_y);
		// temporarily slam dragged window to new position
		getParent()->setRect(translated_rect);
		S32 pre_snap_x = getParent()->getRect().mLeft;
		S32 pre_snap_y = getParent()->getRect().mBottom;
		mDragLastScreenX = screen_x;
		mDragLastScreenY = screen_y;

		LLRect new_rect;
		LLCoordGL mouse_dir;
		// use hysteresis on mouse motion to preserve user intent when mouse stops moving
		mouse_dir.mX = (screen_x == mLastMouseScreenX) ? mLastMouseDir.mX : screen_x - mLastMouseScreenX;
		mouse_dir.mY = (screen_y == mLastMouseScreenY) ? mLastMouseDir.mY : screen_y - mLastMouseScreenY;
		mLastMouseDir = mouse_dir;
		mLastMouseScreenX = screen_x;
		mLastMouseScreenY = screen_y;

		LLView* snap_view = getParent()->findSnapRect(new_rect, mouse_dir, SNAP_PARENT_AND_SIBLINGS, sSnapMargin);

		getParent()->snappedTo(snap_view);
		delta_x = new_rect.mLeft - pre_snap_x;
		delta_y = new_rect.mBottom - pre_snap_y;
		translated_rect.translate(delta_x, delta_y);

		// restore original rect so delta are detected, then call user reshape method to handle snapped floaters, etc
		getParent()->setRect(original_rect);
		getParent()->userSetShape(translated_rect);

		mDragLastScreenX += delta_x;
		mDragLastScreenY += delta_y;

		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" <<llendl;		
		handled = TRUE;
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
		handled = TRUE;
	}

	// Note: don't pass on to children

	return handled;
}

void LLDragHandle::setValue(const LLSD& value)
{
	setTitle(value.asString());
}
