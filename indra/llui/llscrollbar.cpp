/** 
 * @file llscrollbar.cpp
 * @brief Scrollbar UI widget
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

#include "linden_common.h"

#include "llscrollbar.h"

#include "llmath.h"
#include "lltimer.h"
#include "v3color.h"

#include "llbutton.h"
#include "llcriticaldamp.h"
#include "llkeyboard.h"
#include "llui.h"
//#include "llviewerimagelist.h"
#include "llfocusmgr.h"
#include "llwindow.h"
#include "llglheaders.h"
#include "llcontrol.h"
#include "llglimmediate.h"

LLScrollbar::LLScrollbar(
		const LLString& name, LLRect rect,
		LLScrollbar::ORIENTATION orientation,
		S32 doc_size, S32 doc_pos, S32 page_size,
		void (*change_callback)( S32 new_pos, LLScrollbar* self, void* userdata ),
		void* callback_user_data,
		S32 step_size)
:		LLUICtrl( name, rect, TRUE, NULL, NULL ),

		mChangeCallback( change_callback ),
		mCallbackUserData( callback_user_data ),
		mOrientation( orientation ),
		mDocSize( doc_size ),
		mDocPos( doc_pos ),
		mPageSize( page_size ),
		mStepSize( step_size ),
		mDocChanged(FALSE),
		mDragStartX( 0 ),
		mDragStartY( 0 ),
		mHoverGlowStrength(0.15f),
		mCurGlowStrength(0.f),
		mTrackColor( LLUI::sColorsGroup->getColor("ScrollbarTrackColor") ),
		mThumbColor ( LLUI::sColorsGroup->getColor("ScrollbarThumbColor") ),
		mHighlightColor ( LLUI::sColorsGroup->getColor("DefaultHighlightLight") ),
		mShadowColor ( LLUI::sColorsGroup->getColor("DefaultShadowLight") ),
		mOnScrollEndCallback( NULL ),
		mOnScrollEndData( NULL )
{
	//llassert( 0 <= mDocSize );
	//llassert( 0 <= mDocPos && mDocPos <= mDocSize );
	
	setTabStop(FALSE);
	updateThumbRect();
	
	// Page up and page down buttons
	LLRect line_up_rect;
	LLString line_up_img;
	LLString line_up_selected_img;
	LLString line_down_img;
	LLString line_down_selected_img;

	LLRect line_down_rect;

	if( LLScrollbar::VERTICAL == mOrientation )
	{
		line_up_rect.setLeftTopAndSize( 0, getRect().getHeight(), SCROLLBAR_SIZE, SCROLLBAR_SIZE );
		line_up_img="UIImgBtnScrollUpOutUUID";
		line_up_selected_img="UIImgBtnScrollUpInUUID";

		line_down_rect.setOriginAndSize( 0, 0, SCROLLBAR_SIZE, SCROLLBAR_SIZE );
		line_down_img="UIImgBtnScrollDownOutUUID";
		line_down_selected_img="UIImgBtnScrollDownInUUID";
	}
	else
	{
		// Horizontal
		line_up_rect.setOriginAndSize( 0, 0, SCROLLBAR_SIZE, SCROLLBAR_SIZE );
		line_up_img="UIImgBtnScrollLeftOutUUID";
		line_up_selected_img="UIImgBtnScrollLeftInUUID";

		line_down_rect.setOriginAndSize( getRect().getWidth() - SCROLLBAR_SIZE, 0, SCROLLBAR_SIZE, SCROLLBAR_SIZE );
		line_down_img="UIImgBtnScrollRightOutUUID";
		line_down_selected_img="UIImgBtnScrollRightInUUID";
	}

	LLButton* line_up_btn = new LLButton(
		"Line Up", line_up_rect,
		line_up_img, line_up_selected_img, "",
		&LLScrollbar::onLineUpBtnPressed, this, LLFontGL::sSansSerif );
	if( LLScrollbar::VERTICAL == mOrientation )
	{
		line_up_btn->setFollowsRight();
		line_up_btn->setFollowsTop();
	}
	else
	{
		// horizontal
		line_up_btn->setFollowsLeft();
		line_up_btn->setFollowsBottom();
	}
	line_up_btn->setHeldDownCallback( &LLScrollbar::onLineUpBtnPressed );
	line_up_btn->setTabStop(FALSE);
	addChild(line_up_btn);

	LLButton* line_down_btn = new LLButton(
		"Line Down", line_down_rect,
		line_down_img, line_down_selected_img, "",
		&LLScrollbar::onLineDownBtnPressed, this, LLFontGL::sSansSerif );
	line_down_btn->setFollowsRight();
	line_down_btn->setFollowsBottom();
	line_down_btn->setHeldDownCallback( &LLScrollbar::onLineDownBtnPressed );
	line_down_btn->setTabStop(FALSE);
	addChild(line_down_btn);
}


LLScrollbar::~LLScrollbar()
{
	// Children buttons killed by parent class
}

void LLScrollbar::setDocParams( S32 size, S32 pos )
{
	mDocSize = size;
	mDocPos = llclamp( pos, 0, getDocPosMax() );
	mDocChanged = TRUE;

	updateThumbRect();
}

void LLScrollbar::setDocPos(S32 pos)
{
	if (pos != mDocPos)
	{
		mDocPos = llclamp( pos, 0, getDocPosMax() );
		mDocChanged = TRUE;

		updateThumbRect();
	}
}

void LLScrollbar::setDocSize(S32 size)
{
	if (size != mDocSize)
	{
		mDocSize = size;
		mDocPos = llclamp( mDocPos, 0, getDocPosMax() );
		mDocChanged = TRUE;

		updateThumbRect();
	}
}

void LLScrollbar::setPageSize( S32 page_size )
{
	if (page_size != mPageSize)
	{
		mPageSize = page_size;
		mDocPos = llclamp( mDocPos, 0, getDocPosMax() );
		mDocChanged = TRUE;

		updateThumbRect();
	}
}

BOOL LLScrollbar::isAtBeginning()
{
	return mDocPos == 0;
}

BOOL LLScrollbar::isAtEnd()
{
	return mDocPos == getDocPosMax();
}


void LLScrollbar::updateThumbRect()
{
//	llassert( 0 <= mDocSize );
//	llassert( 0 <= mDocPos && mDocPos <= getDocPosMax() );
	
	const S32 THUMB_MIN_LENGTH = 16;

	S32 window_length = (mOrientation == LLScrollbar::HORIZONTAL) ? getRect().getWidth() : getRect().getHeight();
	S32 thumb_bg_length = window_length - 2 * SCROLLBAR_SIZE;
	S32 visible_lines = llmin( mDocSize, mPageSize );
	S32 thumb_length = mDocSize ? llmax( visible_lines * thumb_bg_length / mDocSize, THUMB_MIN_LENGTH ) : thumb_bg_length;

	S32 variable_lines = mDocSize - visible_lines;

	if( mOrientation == LLScrollbar::VERTICAL )
	{ 
		S32 thumb_start_max = thumb_bg_length + SCROLLBAR_SIZE;
		S32 thumb_start_min = SCROLLBAR_SIZE + THUMB_MIN_LENGTH;
		S32 thumb_start = variable_lines ? llclamp( thumb_start_max - (mDocPos * (thumb_bg_length - thumb_length)) / variable_lines, thumb_start_min, thumb_start_max ) : thumb_start_max;

		mThumbRect.mLeft =  0;
		mThumbRect.mTop = thumb_start;
		mThumbRect.mRight = SCROLLBAR_SIZE;
		mThumbRect.mBottom = thumb_start - thumb_length;
	}
	else
	{
		// Horizontal
		S32 thumb_start_max = thumb_bg_length + SCROLLBAR_SIZE - thumb_length;
		S32 thumb_start_min = SCROLLBAR_SIZE;
		S32 thumb_start = variable_lines ? llclamp( thumb_start_min + (mDocPos * (thumb_bg_length - thumb_length)) / variable_lines, thumb_start_min, thumb_start_max ) : thumb_start_min;
	
		mThumbRect.mLeft = thumb_start;
		mThumbRect.mTop = SCROLLBAR_SIZE;
		mThumbRect.mRight = thumb_start + thumb_length;
		mThumbRect.mBottom = 0;
	}
	
	if (mOnScrollEndCallback && mOnScrollEndData && (mDocPos == getDocPosMax()))
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}
}

BOOL LLScrollbar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Check children first
	BOOL handled_by_child = LLView::childrenHandleMouseDown(x, y, mask) != NULL;
	if( !handled_by_child )
	{
		if( mThumbRect.pointInRect(x,y) )
		{
			// Start dragging the thumb
			// No handler needed for focus lost since this clas has no state that depends on it.
			gFocusMgr.setMouseCapture( this );  
			mDragStartX = x;
			mDragStartY = y;
			mOrigRect.mTop = mThumbRect.mTop;
			mOrigRect.mBottom = mThumbRect.mBottom;
			mOrigRect.mLeft = mThumbRect.mLeft;
			mOrigRect.mRight = mThumbRect.mRight;
			mLastDelta = 0;
		}
		else
		{
			if( 
				( (LLScrollbar::VERTICAL == mOrientation) && (mThumbRect.mTop < y) ) ||
				( (LLScrollbar::HORIZONTAL == mOrientation) && (x < mThumbRect.mLeft) )
			)
			{
				// Page up
				pageUp(0);
			}
			else
			if(
				( (LLScrollbar::VERTICAL == mOrientation) && (y < mThumbRect.mBottom) ) ||
				( (LLScrollbar::HORIZONTAL == mOrientation) && (mThumbRect.mRight < x) )
			)
			{
				// Page down
				pageDown(0);
			}
		}
	}

	return TRUE;
}


BOOL LLScrollbar::handleHover(S32 x, S32 y, MASK mask)
{
	// Note: we don't bother sending the event to the children (the arrow buttons)
	// because they'll capture the mouse whenever they need hover events.
	
	BOOL handled = FALSE;
	if( hasMouseCapture() )
	{
		S32 height = getRect().getHeight();
		S32 width = getRect().getWidth();

		if( VERTICAL == mOrientation )
		{
//			S32 old_pos = mThumbRect.mTop;

			S32 delta_pixels = y - mDragStartY;
			if( mOrigRect.mBottom + delta_pixels < SCROLLBAR_SIZE )
			{
				delta_pixels = SCROLLBAR_SIZE - mOrigRect.mBottom - 1;
			}
			else
			if( mOrigRect.mTop + delta_pixels > height - SCROLLBAR_SIZE )
			{
				delta_pixels = height - SCROLLBAR_SIZE - mOrigRect.mTop + 1;
			}

			mThumbRect.mTop = mOrigRect.mTop + delta_pixels;
			mThumbRect.mBottom = mOrigRect.mBottom + delta_pixels;

			S32 thumb_length = mThumbRect.getHeight();
			S32 thumb_track_length = height - 2 * SCROLLBAR_SIZE;


			if( delta_pixels != mLastDelta || mDocChanged)
			{	
				// Note: delta_pixels increases as you go up.  mDocPos increases down (line 0 is at the top of the page).
				S32 usable_track_length = thumb_track_length - thumb_length;
				if( 0 < usable_track_length )
				{
					S32 variable_lines = getDocPosMax();
					S32 pos = mThumbRect.mTop;
					F32 ratio = F32(pos - SCROLLBAR_SIZE - thumb_length) / usable_track_length;	
	
					S32 new_pos = llclamp( S32(variable_lines - ratio * variable_lines + 0.5f), 0, variable_lines );
					// Note: we do not call updateThumbRect() here.  Instead we let the thumb and the document go slightly
					// out of sync (less than a line's worth) to make the thumb feel responsive.
					changeLine( new_pos - mDocPos, FALSE );
				}
			}

			mLastDelta = delta_pixels;
		
		}
		else
		{
			// Horizontal
//			S32 old_pos = mThumbRect.mLeft;

			S32 delta_pixels = x - mDragStartX;

			if( mOrigRect.mLeft + delta_pixels < SCROLLBAR_SIZE )
			{
				delta_pixels = SCROLLBAR_SIZE - mOrigRect.mLeft - 1;
			}
			else
			if( mOrigRect.mRight + delta_pixels > width - SCROLLBAR_SIZE )
			{
				delta_pixels = width - SCROLLBAR_SIZE - mOrigRect.mRight + 1;
			}

			mThumbRect.mLeft = mOrigRect.mLeft + delta_pixels;
			mThumbRect.mRight = mOrigRect.mRight + delta_pixels;
			
			S32 thumb_length = mThumbRect.getWidth();
			S32 thumb_track_length = width - 2 * SCROLLBAR_SIZE;

			if( delta_pixels != mLastDelta || mDocChanged)
			{	
				// Note: delta_pixels increases as you go up.  mDocPos increases down (line 0 is at the top of the page).
				S32 usable_track_length = thumb_track_length - thumb_length;
				if( 0 < usable_track_length )
				{
					S32 variable_lines = getDocPosMax();
					S32 pos = mThumbRect.mLeft;
					F32 ratio = F32(pos - SCROLLBAR_SIZE) / usable_track_length;	
	
					S32 new_pos = llclamp( S32(ratio * variable_lines + 0.5f), 0, variable_lines);
	
					// Note: we do not call updateThumbRect() here.  Instead we let the thumb and the document go slightly
					// out of sync (less than a line's worth) to make the thumb feel responsive.
					changeLine( new_pos - mDocPos, FALSE );
				}
			}

			mLastDelta = delta_pixels;
		}

		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
		handled = TRUE;
	}
	else
	{
		handled = childrenHandleMouseUp( x, y, mask ) != NULL;
	}

	// Opaque
	if( !handled )
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)"  << llendl;		
		handled = TRUE;
	}

	mDocChanged = FALSE;
	return handled;
} // end handleHover


BOOL LLScrollbar::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	changeLine( clicks * mStepSize, TRUE );
	return TRUE;
}

BOOL LLScrollbar::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, LLString &tooltip_msg)
{
	// enable this to get drag and drop to control scrollbars
	//if (!drop)
	//{
	//	//TODO: refactor this
	//	S32 variable_lines = getDocPosMax();
	//	S32 pos = (VERTICAL == mOrientation) ? y : x;
	//	S32 thumb_length = (VERTICAL == mOrientation) ? mThumbRect.getHeight() : mThumbRect.getWidth();
	//	S32 thumb_track_length = (VERTICAL == mOrientation) ? (getRect().getHeight() - 2 * SCROLLBAR_SIZE) : (getRect().getWidth() - 2 * SCROLLBAR_SIZE);
	//	S32 usable_track_length = thumb_track_length - thumb_length;
	//	F32 ratio = (VERTICAL == mOrientation) ? F32(pos - SCROLLBAR_SIZE - thumb_length) / usable_track_length
	//		: F32(pos - SCROLLBAR_SIZE) / usable_track_length;	
	//	S32 new_pos = (VERTICAL == mOrientation) ? llclamp( S32(variable_lines - ratio * variable_lines + 0.5f), 0, variable_lines )
	//		: llclamp( S32(ratio * variable_lines + 0.5f), 0, variable_lines );
	//	changeLine( new_pos - mDocPos, TRUE );
	//}
	//return TRUE;
	return FALSE;
}

BOOL LLScrollbar::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if( hasMouseCapture() )
	{
		gFocusMgr.setMouseCapture( NULL );
		handled = TRUE;
	}
	else
	{
		// Opaque, so don't just check children
		handled = LLView::handleMouseUp( x, y, mask );
	}

	return handled;
}

void LLScrollbar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLView::reshape( width, height, called_from_parent );
	updateThumbRect();
}


void LLScrollbar::draw()
{
	S32 local_mouse_x;
	S32 local_mouse_y;
	LLCoordWindow cursor_pos_window;
	getWindow()->getCursorPosition(&cursor_pos_window);
	LLCoordGL cursor_pos_gl;
	getWindow()->convertCoords(cursor_pos_window, &cursor_pos_gl);

	screenPointToLocal(cursor_pos_gl.mX, cursor_pos_gl.mY, &local_mouse_x, &local_mouse_y);
	BOOL other_captor = gFocusMgr.getMouseCapture() && gFocusMgr.getMouseCapture() != this;
	BOOL hovered = getEnabled() && !other_captor && (hasMouseCapture() || mThumbRect.pointInRect(local_mouse_x, local_mouse_y));
	if (hovered)
	{
		mCurGlowStrength = lerp(mCurGlowStrength, mHoverGlowStrength, LLCriticalDamp::getInterpolant(0.05f));
	}
	else
	{
		mCurGlowStrength = lerp(mCurGlowStrength, 0.f, LLCriticalDamp::getInterpolant(0.05f));
	}


	// Draw background and thumb.
	LLUIImage* rounded_rect_imagep = LLUI::sImageProvider->getUIImage("rounded_square.tga");

	if (!rounded_rect_imagep)
	{
		gl_rect_2d(mOrientation == HORIZONTAL ? SCROLLBAR_SIZE : 0, 
		mOrientation == VERTICAL ? getRect().getHeight() - 2 * SCROLLBAR_SIZE : getRect().getHeight(),
		mOrientation == HORIZONTAL ? getRect().getWidth() - 2 * SCROLLBAR_SIZE : getRect().getWidth(), 
		mOrientation == VERTICAL ? SCROLLBAR_SIZE : 0, mTrackColor, TRUE);

		gl_rect_2d(mThumbRect, mThumbColor, TRUE);

	}
	else
	{
		// Background
		rounded_rect_imagep->drawSolid(mOrientation == HORIZONTAL ? SCROLLBAR_SIZE : 0, 
			mOrientation == VERTICAL ? SCROLLBAR_SIZE : 0,
			mOrientation == HORIZONTAL ? getRect().getWidth() - 2 * SCROLLBAR_SIZE : getRect().getWidth(), 
			mOrientation == VERTICAL ? getRect().getHeight() - 2 * SCROLLBAR_SIZE : getRect().getHeight(),
			mTrackColor);

		// Thumb
		LLRect outline_rect = mThumbRect;
		outline_rect.stretch(2);

		if (gFocusMgr.getKeyboardFocus() == this)
		{
			rounded_rect_imagep->draw(outline_rect, gFocusMgr.getFocusColor());
		}

		rounded_rect_imagep->draw(mThumbRect, mThumbColor);
		if (mCurGlowStrength > 0.01f)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			rounded_rect_imagep->drawSolid(mThumbRect, LLColor4(1.f, 1.f, 1.f, mCurGlowStrength));
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

	}

	BOOL was_scrolled_to_bottom = (getDocPos() == getDocPosMax());
	if (mOnScrollEndCallback && was_scrolled_to_bottom)
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}

	// Draw children
	LLView::draw();
} // end draw


void LLScrollbar::changeLine( S32 delta, BOOL update_thumb )
{
	S32 new_pos = llclamp( mDocPos + delta, 0, getDocPosMax() );
	if( new_pos != mDocPos )
	{
		mDocPos = new_pos;
	}

	if( mChangeCallback )
	{
		mChangeCallback( mDocPos, this, mCallbackUserData );
	}

	if( update_thumb )
	{
		updateThumbRect();
	}
}

void LLScrollbar::setValue(const LLSD& value) 
{ 
	setDocPos((S32) value.asInteger());
}


BOOL LLScrollbar::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;

	switch( key )
	{
	case KEY_HOME:
		changeLine( -mDocPos, TRUE );
		handled = TRUE;
		break;
	
	case KEY_END:
		changeLine( getDocPosMax() - mDocPos, TRUE );
		handled = TRUE;
		break;
	
	case KEY_DOWN:
		changeLine( mStepSize, TRUE );
		handled = TRUE;
		break;
	
	case KEY_UP:
		changeLine( - mStepSize, TRUE );
		handled = TRUE;
		break;

	case KEY_PAGE_DOWN:
		pageDown(1);
		break;

	case KEY_PAGE_UP:
		pageUp(1);
		break;
	}

	return handled;
}

void LLScrollbar::pageUp(S32 overlap)
{
	if (mDocSize > mPageSize)
	{
		changeLine( -(mPageSize - overlap), TRUE );
	}
}

void LLScrollbar::pageDown(S32 overlap)
{
	if (mDocSize > mPageSize)
	{
		changeLine( mPageSize - overlap, TRUE );
	}
}

// static
void LLScrollbar::onLineUpBtnPressed( void* userdata )
{
	LLScrollbar* self = (LLScrollbar*) userdata;

	self->changeLine( - self->mStepSize, TRUE );
}

// static
void LLScrollbar::onLineDownBtnPressed( void* userdata )
{
	LLScrollbar* self = (LLScrollbar*) userdata;
	self->changeLine( self->mStepSize, TRUE );
}


