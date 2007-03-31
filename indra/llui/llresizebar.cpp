/** 
 * @file llresizebar.cpp
 * @brief LLResizeBar base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llresizebar.h"

//#include "llviewermenu.h"
//#include "llviewerimagelist.h"
#include "llmath.h"
#include "llui.h"
#include "llmenugl.h"
#include "llfocusmgr.h"
#include "llwindow.h"

LLResizeBar::LLResizeBar( const LLString& name, const LLRect& rect, S32 min_width, S32 min_height, Side side )
	:
	LLView( name, rect, TRUE ),
	mDragLastScreenX( 0 ),
	mDragLastScreenY( 0 ),
	mLastMouseScreenX( 0 ),
	mLastMouseScreenY( 0 ),
	mMinWidth( min_width ),
	mMinHeight( min_height ),
	mSide( side )
{
	// set up some generically good follow code.
	switch( side )
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
	// this is just a decorator
	setSaveToXML(FALSE);
}


BOOL LLResizeBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if( mEnabled )
	{
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		// No handler needed for focus lost since this clas has no state that depends on it.
		gFocusMgr.setMouseCapture( this );

		localPointToScreen(x, y, &mDragLastScreenX, &mDragLastScreenY);
		mLastMouseScreenX = mDragLastScreenX;
		mLastMouseScreenY = mDragLastScreenY;
	}

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

EWidgetType LLResizeBar::getWidgetType() const
{
	return WIDGET_TYPE_RESIZE_BAR;
}

LLString LLResizeBar::getWidgetTag() const
{
	return LL_RESIZE_BAR_TAG;
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
		LLView* resizing_view = getParent();
		
		if( valid_rect.localPointInRect( screen_x, screen_y ) && resizing_view )
		{
			// Resize the parent
			LLRect orig_rect = resizing_view->getRect();
			LLRect scaled_rect = orig_rect;
				
			S32 new_width = orig_rect.getWidth();
			S32 new_height = orig_rect.getHeight();

			switch( mSide )
			{
			case LEFT:
				new_width = orig_rect.getWidth() - delta_x;
				if( new_width < mMinWidth )
				{
					new_width = mMinWidth;
					delta_x = orig_rect.getWidth() - mMinWidth;
				}
				scaled_rect.translate(delta_x, 0);
				break;

			case TOP:
				new_height = orig_rect.getHeight() + delta_y;
				if( new_height < mMinHeight )
				{
					new_height = mMinHeight;
					delta_y = mMinHeight - orig_rect.getHeight();
				}
				break;
			
			case RIGHT:
				new_width = orig_rect.getWidth() + delta_x;
				if( new_width < mMinWidth )
				{
					new_width = mMinWidth;
					delta_x = mMinWidth - orig_rect.getWidth();
				}
				break;
		
			case BOTTOM:
				new_height = orig_rect.getHeight() - delta_y;
				if( new_height < mMinHeight )
				{
					new_height = mMinHeight;
					delta_y = orig_rect.getHeight() - mMinHeight;
				}
				scaled_rect.translate(0, delta_y);
				break;
			}

			scaled_rect.mTop = scaled_rect.mBottom + new_height;
			scaled_rect.mRight = scaled_rect.mLeft + new_width;
			resizing_view->setRect(scaled_rect);

			LLView* snap_view = NULL;

			switch( mSide )
			{
			case LEFT:
				snap_view = resizing_view->findSnapEdge(scaled_rect.mLeft, mouse_dir, SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				break;
			case TOP:
				snap_view = resizing_view->findSnapEdge(scaled_rect.mTop, mouse_dir, SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				break;
			case RIGHT:
				snap_view = resizing_view->findSnapEdge(scaled_rect.mRight, mouse_dir, SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				break;
			case BOTTOM:
				snap_view = resizing_view->findSnapEdge(scaled_rect.mBottom, mouse_dir, SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				break;
			}

			// register "snap" behavior with snapped view
			resizing_view->snappedTo(snap_view);

			// restore original rectangle so the appropriate changes are detected
			resizing_view->setRect(orig_rect);
			// change view shape as user operation
			resizing_view->userSetShape(scaled_rect);

			// update last valid mouse cursor position based on resized view's actual size
			LLRect new_rect = resizing_view->getRect();
			switch(mSide)
			{
			case LEFT:
				mDragLastScreenX += new_rect.mLeft - orig_rect.mLeft;
				break;
			case RIGHT:
				mDragLastScreenX += new_rect.mRight - orig_rect.mRight;
				break;
			case TOP:
				mDragLastScreenY += new_rect.mTop - orig_rect.mTop;
				break;
			case BOTTOM:
				mDragLastScreenY += new_rect.mBottom- orig_rect.mBottom;
				break;
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

	if( handled )
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
}

