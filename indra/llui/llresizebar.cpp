/** 
 * @file llresizebar.cpp
 * @brief LLResizeBar base class
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

#include "linden_common.h"

#include "llresizebar.h"

#include "llmath.h"
#include "llui.h"
#include "llmenugl.h"
#include "llfocusmgr.h"
#include "llwindow.h"

LLResizeBar::LLResizeBar(const LLResizeBar::Params& p)
:	LLView(p),
	mDragLastScreenX( 0 ),
	mDragLastScreenY( 0 ),
	mLastMouseScreenX( 0 ),
	mLastMouseScreenY( 0 ),
	mMinSize( p.min_size ),
	mMaxSize( p.max_size ),
	mSide( p.side ),
	mSnappingEnabled(p.snapping_enabled),
	mAllowDoubleClickSnapping(p.allow_double_click_snapping),
	mResizingView(p.resizing_view)
{
	setFollowsNone();
	// set up some generically good follow code.
	switch( mSide )
	{
	case LEFT:
		setFollowsLeft();
		setFollowsTop();
		setFollowsBottom();
		break;
	case TOP:
		setFollowsTop();
		setFollowsLeft();
		setFollowsRight();
		break;
	case RIGHT:
		setFollowsRight();
		setFollowsTop();
		setFollowsBottom();
		break;
	case BOTTOM:
		setFollowsBottom();
		setFollowsLeft();
		setFollowsRight();
		break;
	default:
		break;
	}
}


BOOL LLResizeBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (!canResize()) return FALSE;

	// Route future Mouse messages here preemptively.  (Release on mouse up.)
	// No handler needed for focus lost since this clas has no state that depends on it.
	gFocusMgr.setMouseCapture( this );

	localPointToScreen(x, y, &mDragLastScreenX, &mDragLastScreenY);
	mLastMouseScreenX = mDragLastScreenX;
	mLastMouseScreenY = mDragLastScreenY;

	return TRUE;
}


BOOL LLResizeBar::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	if( hasMouseCapture() )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );
		handled = TRUE;
	}
	else
	{
		handled = TRUE;
	}
	return handled;
}


BOOL LLResizeBar::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);
		
		S32 delta_x = screen_x - mDragLastScreenX;
		S32 delta_y = screen_y - mDragLastScreenY;
				
		LLCoordGL mouse_dir;
		// use hysteresis on mouse motion to preserve user intent when mouse stops moving
		mouse_dir.mX = (screen_x == mLastMouseScreenX) ? mLastMouseDir.mX : screen_x - mLastMouseScreenX;
		mouse_dir.mY = (screen_y == mLastMouseScreenY) ? mLastMouseDir.mY : screen_y - mLastMouseScreenY;
		mLastMouseDir = mouse_dir;
		mLastMouseScreenX = screen_x;
		mLastMouseScreenY = screen_y;

		// Make sure the mouse in still over the application.  We don't want to make the parent
		// so big that we can't see the resize handle any more.
		LLRect valid_rect = getRootView()->getRect();
		
		if( valid_rect.localPointInRect( screen_x, screen_y ) && mResizingView )
		{
			// Resize the parent
			LLRect orig_rect = mResizingView->getRect();
			LLRect scaled_rect = orig_rect;
				
			S32 new_width = orig_rect.getWidth();
			S32 new_height = orig_rect.getHeight();

			switch( mSide )
			{
			case LEFT:
				new_width = llclamp(orig_rect.getWidth() - delta_x, mMinSize, mMaxSize);
				delta_x = orig_rect.getWidth() - new_width;
				scaled_rect.translate(delta_x, 0);
				break;

			case TOP:
				new_height = llclamp(orig_rect.getHeight() + delta_y, mMinSize, mMaxSize);
				delta_y = new_height - orig_rect.getHeight();
				break;
			
			case RIGHT:
				new_width = llclamp(orig_rect.getWidth() + delta_x, mMinSize, mMaxSize);
				delta_x = new_width - orig_rect.getWidth();
				break;
		
			case BOTTOM:
				new_height = llclamp(orig_rect.getHeight() - delta_y, mMinSize, mMaxSize);
				delta_y = orig_rect.getHeight() - new_height;
				scaled_rect.translate(0, delta_y);
				break;
			}

			notifyParent(LLSD().with("action", "resize")
				.with("view_name", mResizingView->getName())
				.with("new_height", new_height)
				.with("new_width", new_width));

			scaled_rect.mTop = scaled_rect.mBottom + new_height;
			scaled_rect.mRight = scaled_rect.mLeft + new_width;
			mResizingView->setRect(scaled_rect);

			LLView* snap_view = NULL;

			if (mSnappingEnabled)
			{
				static LLUICachedControl<S32> snap_margin ("SnapMargin", 0);
				switch( mSide )
				{
				case LEFT:
					snap_view = mResizingView->findSnapEdge(scaled_rect.mLeft, mouse_dir, SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, snap_margin);
					break;
				case TOP:
					snap_view = mResizingView->findSnapEdge(scaled_rect.mTop, mouse_dir, SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, snap_margin);
					break;
				case RIGHT:
					snap_view = mResizingView->findSnapEdge(scaled_rect.mRight, mouse_dir, SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, snap_margin);
					break;
				case BOTTOM:
					snap_view = mResizingView->findSnapEdge(scaled_rect.mBottom, mouse_dir, SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, snap_margin);
					break;
				}
			}

			// register "snap" behavior with snapped view
			mResizingView->setSnappedTo(snap_view);

			// restore original rectangle so the appropriate changes are detected
			mResizingView->setRect(orig_rect);
			// change view shape as user operation
			mResizingView->setShape(scaled_rect, true);

			// update last valid mouse cursor position based on resized view's actual size
			LLRect new_rect = mResizingView->getRect();

			switch(mSide)
			{
			case LEFT:
			{
				S32 actual_delta_x = new_rect.mLeft - orig_rect.mLeft;
				if (actual_delta_x != delta_x)
				{
					// restore everything by left
					new_rect.mBottom = orig_rect.mBottom;
					new_rect.mTop = orig_rect.mTop;
					new_rect.mRight = orig_rect.mRight;
					mResizingView->setShape(new_rect, true);
				}
				mDragLastScreenX += actual_delta_x;

				break;
			}
			case RIGHT:
			{
				S32 actual_delta_x = new_rect.mRight - orig_rect.mRight;
				if (actual_delta_x != delta_x)
				{
					// restore everything by left
					new_rect.mBottom = orig_rect.mBottom;
					new_rect.mTop = orig_rect.mTop;
					new_rect.mLeft = orig_rect.mLeft;
					mResizingView->setShape(new_rect, true);
				}
				mDragLastScreenX += new_rect.mRight - orig_rect.mRight;
				break;
			}
			case TOP:
			{
				S32 actual_delta_y = new_rect.mTop - orig_rect.mTop;
				if (actual_delta_y != delta_y)
				{
					// restore everything by left
					new_rect.mBottom = orig_rect.mBottom;
					new_rect.mLeft = orig_rect.mLeft;
					new_rect.mRight = orig_rect.mRight;
					mResizingView->setShape(new_rect, true);
				}
				mDragLastScreenY += new_rect.mTop - orig_rect.mTop;
				break;
			}
			case BOTTOM:
			{
				S32 actual_delta_y = new_rect.mBottom - orig_rect.mBottom;
				if (actual_delta_y != delta_y)
				{
					// restore everything by left
					new_rect.mTop = orig_rect.mTop;
					new_rect.mLeft = orig_rect.mLeft;
					new_rect.mRight = orig_rect.mRight;
					mResizingView->setShape(new_rect, true);
				}
				mDragLastScreenY += new_rect.mBottom- orig_rect.mBottom;
				break;
			}
			default:
				break;
			}
		}

		handled = TRUE;
	}
	else
	{
		handled = TRUE;
	}

	if( handled && canResize() )
	{
		switch( mSide )
		{
		case LEFT:
		case RIGHT:
			getWindow()->setCursor(UI_CURSOR_SIZEWE);
			break;

		case TOP:
		case BOTTOM:
			getWindow()->setCursor(UI_CURSOR_SIZENS);
			break;
		}
	}

	return handled;
} // end LLResizeBar::handleHover

BOOL LLResizeBar::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	LLRect orig_rect = mResizingView->getRect();
	LLRect scaled_rect = orig_rect;

	if (mSnappingEnabled && mAllowDoubleClickSnapping)
	{
		switch( mSide )
		{
		case LEFT:
			mResizingView->findSnapEdge(scaled_rect.mLeft, LLCoordGL(0, 0), SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, S32_MAX);
			scaled_rect.mLeft = scaled_rect.mRight - llclamp(scaled_rect.getWidth(), mMinSize, mMaxSize);
			break;
		case TOP:
			mResizingView->findSnapEdge(scaled_rect.mTop, LLCoordGL(0, 0), SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, S32_MAX);
			scaled_rect.mTop = scaled_rect.mBottom + llclamp(scaled_rect.getHeight(), mMinSize, mMaxSize);
			break;
		case RIGHT:
			mResizingView->findSnapEdge(scaled_rect.mRight, LLCoordGL(0, 0), SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, S32_MAX);
			scaled_rect.mRight = scaled_rect.mLeft + llclamp(scaled_rect.getWidth(), mMinSize, mMaxSize);
			break;
		case BOTTOM:
			mResizingView->findSnapEdge(scaled_rect.mBottom, LLCoordGL(0, 0), SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, S32_MAX);
			scaled_rect.mBottom = scaled_rect.mTop - llclamp(scaled_rect.getHeight(), mMinSize, mMaxSize);
			break;
		}

		mResizingView->setShape(scaled_rect, true);
	}

	return TRUE;
}

