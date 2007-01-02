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
	mDragStartScreenX( 0 ),
	mDragStartScreenY( 0 ),
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
}


BOOL LLResizeBar::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if( mEnabled )
	{
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		// No handler needed for focus lost since this clas has no state that depends on it.
		gFocusMgr.setMouseCapture( this, NULL );

		//localPointToScreen(x, y, &mDragStartScreenX, &mDragStartScreenX);
		localPointToOtherView(x, y, &mDragStartScreenX, &mDragStartScreenY, getParent()->getParent());
		mLastMouseScreenX = mDragStartScreenX;
		mLastMouseScreenY = mDragStartScreenY;
	}

	return TRUE;
}


BOOL LLResizeBar::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	if( gFocusMgr.getMouseCapture() == this )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL, NULL );
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
	if( gFocusMgr.getMouseCapture() == this )
	{
		//FIXME: this, of course, is fragile
		LLView* floater_view = getParent()->getParent();
		S32 floater_view_x;
		S32 floater_view_y;
		localPointToOtherView(x, y, &floater_view_x, &floater_view_y, floater_view);
		
		S32 delta_x = floater_view_x - mDragStartScreenX;
		S32 delta_y = floater_view_y - mDragStartScreenY;
				
		LLCoordGL mouse_dir;
		// use hysteresis on mouse motion to preserve user intent when mouse stops moving
		mouse_dir.mX = (floater_view_x == mLastMouseScreenX) ? mLastMouseDir.mX : floater_view_x - mLastMouseScreenX;
		mouse_dir.mY = (floater_view_y == mLastMouseScreenY) ? mLastMouseDir.mY : floater_view_y - mLastMouseScreenY;
		mLastMouseDir = mouse_dir;
		mLastMouseScreenX = floater_view_x;
		mLastMouseScreenY = floater_view_y;

		// Make sure the mouse in still over the application.  We don't want to make the parent
		// so big that we can't see the resize handle any more.
		LLRect valid_rect = floater_view->getRect();
		LLView* parentView = getParent();
		if( valid_rect.localPointInRect( floater_view_x, floater_view_y ) && parentView )
		{
			// Resize the parent
			LLRect parent_rect = parentView->getRect();
			LLRect scaled_rect = parent_rect;
				
			S32 new_width = parent_rect.getWidth();
			S32 new_height = parent_rect.getHeight();

			switch( mSide )
			{
			case LEFT:
				new_width = parent_rect.getWidth() - delta_x;
				if( new_width < mMinWidth )
				{
					new_width = mMinWidth;
					delta_x = parent_rect.getWidth() - mMinWidth;
				}
				scaled_rect.translate(delta_x, 0);
				break;

			case TOP:
				new_height = parent_rect.getHeight() + delta_y;
				if( new_height < mMinHeight )
				{
					new_height = mMinHeight;
					delta_y = mMinHeight - parent_rect.getHeight();
				}
				break;
			
			case RIGHT:
				new_width = parent_rect.getWidth() + delta_x;
				if( new_width < mMinWidth )
				{
					new_width = mMinWidth;
					delta_x = mMinWidth - parent_rect.getWidth();
				}
				break;
		
			case BOTTOM:
				new_height = parent_rect.getHeight() - delta_y;
				if( new_height < mMinHeight )
				{
					new_height = mMinHeight;
					delta_y = parent_rect.getHeight() - mMinHeight;
				}
				scaled_rect.translate(0, delta_y);
				break;
			}

			scaled_rect.mTop = scaled_rect.mBottom + new_height;
			scaled_rect.mRight = scaled_rect.mLeft + new_width;
			parentView->setRect(scaled_rect);

			S32 snap_delta_x = 0;
			S32 snap_delta_y = 0;

			LLView* snap_view = NULL;

			switch( mSide )
			{
			case LEFT:
				snap_view = parentView->findSnapEdge(snap_delta_x, mouse_dir, SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_x -= scaled_rect.mLeft;
				scaled_rect.mLeft += snap_delta_x;
				break;
			case TOP:
				snap_view = parentView->findSnapEdge(snap_delta_y, mouse_dir, SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_y -= scaled_rect.mTop;
				scaled_rect.mTop += snap_delta_y;
				break;
			case RIGHT:
				snap_view = parentView->findSnapEdge(snap_delta_x, mouse_dir, SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_x -= scaled_rect.mRight;
				scaled_rect.mRight += snap_delta_x;
				break;
			case BOTTOM:
				snap_view = parentView->findSnapEdge(snap_delta_y, mouse_dir, SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_y -= scaled_rect.mBottom;
				scaled_rect.mBottom += snap_delta_y;
				break;
			}

			parentView->snappedTo(snap_view);

			parentView->setRect(parent_rect);

			parentView->reshape(scaled_rect.getWidth(), scaled_rect.getHeight(), FALSE);
			parentView->translate(scaled_rect.mLeft - parentView->getRect().mLeft, scaled_rect.mBottom - parentView->getRect().mBottom);

			floater_view_x = mDragStartScreenX + delta_x;
			floater_view_y = mDragStartScreenY + delta_y;
			mDragStartScreenX = floater_view_x + snap_delta_x;
			mDragStartScreenY = floater_view_y + snap_delta_y;
		}

		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
		handled = TRUE;
	}
	else
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
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

