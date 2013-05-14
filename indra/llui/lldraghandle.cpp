/** 
 * @file lldraghandle.cpp
 * @brief LLDragHandle base class
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

// A widget for dragging a view around the screen using the mouse.

#include "linden_common.h"

#include "lldraghandle.h"

#include "llmath.h"

//#include "llviewerwindow.h"
#include "llui.h"
#include "llmenugl.h"
#include "lltextbox.h"
#include "llcontrol.h"
#include "llfontgl.h"
#include "llwindow.h"
#include "llfocusmgr.h"
#include "lluictrlfactory.h"

const S32 LEADING_PAD = 5;
const S32 TITLE_HPAD = 8;
const S32 BORDER_PAD = 1;
const S32 LEFT_PAD = BORDER_PAD + TITLE_HPAD + LEADING_PAD;
const S32 RIGHT_PAD = BORDER_PAD + 32; // HACK: space for close btn and minimize btn

S32 LLDragHandle::sSnapMargin = 5;

LLDragHandle::LLDragHandle(const LLDragHandle::Params& p)
:	LLView(p),
	mDragLastScreenX( 0 ),
	mDragLastScreenY( 0 ),
	mLastMouseScreenX( 0 ),
	mLastMouseScreenY( 0 ),
	mTitleBox( NULL ),
	mMaxTitleWidth( 0 ),
	mForeground( TRUE ),
	mDragHighlightColor(p.drag_highlight_color()),
	mDragShadowColor(p.drag_shadow_color())

{
	static LLUICachedControl<S32> snap_margin ("SnapMargin", 0);
	sSnapMargin = snap_margin;
}

LLDragHandle::~LLDragHandle()
{
	removeChild(mTitleBox);
	delete mTitleBox;
}

void LLDragHandle::initFromParams(const LLDragHandle::Params& p)
{
	LLView::initFromParams(p);
	setTitle( p.label );
}

void LLDragHandle::setTitleVisible(BOOL visible) 
{ 
	if(mTitleBox)
	{
		mTitleBox->setVisible(visible); 
	}
}

void LLDragHandleTop::setTitle(const std::string& title)
{
	std::string trimmed_title = title;
	LLStringUtil::trim(trimmed_title);

	if( mTitleBox )
	{
		mTitleBox->setText(trimmed_title);
	}
	else
	{
		const LLFontGL* font = LLFontGL::getFontSansSerif();
		LLTextBox::Params params;
		params.name("Drag Handle Title");
		params.rect(getRect());
		params.initial_value(trimmed_title);
		params.font(font);
		params.follows.flags(FOLLOWS_TOP | FOLLOWS_LEFT | FOLLOWS_RIGHT);
		params.font_shadow(LLFontGL::DROP_SHADOW_SOFT);
		params.use_ellipses = true;
		params.parse_urls = false; //cancel URL replacement in floater title
		mTitleBox = LLUICtrlFactory::create<LLTextBox> (params);
		addChild( mTitleBox );
	}
	
	reshapeTitleBox();
}


std::string LLDragHandleTop::getTitle() const
{
	return mTitleBox == NULL ? LLStringUtil::null : mTitleBox->getText();
}


void LLDragHandleLeft::setTitle(const std::string& )
{
	if( mTitleBox )
	{
		removeChild(mTitleBox);
		delete mTitleBox;
		mTitleBox = NULL;
	}
	/* no title on left edge */
}


std::string LLDragHandleLeft::getTitle() const
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
	if (mTitleBox)
	{
		mTitleBox->setEnabled(getForeground());
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
	if (mTitleBox)
	{
		mTitleBox->setEnabled(getForeground());
	}

	LLView::draw();
}

void LLDragHandleTop::reshapeTitleBox()
{
	static LLUICachedControl<S32> title_vpad("UIFloaterTitleVPad", 0);
	if( ! mTitleBox)
	{
		return;
	}
	const LLFontGL* font = LLFontGL::getFontSansSerif();
	S32 title_width = getRect().getWidth();
	title_width -= LEFT_PAD + 2 * BORDER_PAD + getButtonsRect().getWidth();
	S32 title_height = font->getLineHeight();
	LLRect title_rect;
	title_rect.setLeftTopAndSize( 
		LEFT_PAD, 
		getRect().getHeight() - title_vpad,
		title_width,
		title_height);

	// calls reshape on mTitleBox
	mTitleBox->setShape( title_rect );
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

		// if dragging a docked floater we want to undock
		if (((LLFloater*)getParent())->isDocked())
		{
			const S32 SLOP = 12;

			if (delta_y <= -SLOP || 
				delta_y >= SLOP)
			{
				((LLFloater*)getParent())->setDocked(false, false);
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

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

		getParent()->setSnappedTo(snap_view);
		delta_x = new_rect.mLeft - pre_snap_x;
		delta_y = new_rect.mBottom - pre_snap_y;
		translated_rect.translate(delta_x, delta_y);

		// restore original rect so delta are detected, then call user reshape method to handle snapped floaters, etc
		getParent()->setRect(original_rect);
		getParent()->setShape(translated_rect, true);

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
