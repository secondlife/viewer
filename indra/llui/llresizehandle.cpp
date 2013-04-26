/** 
 * @file llresizehandle.cpp
 * @brief LLResizeHandle base class
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

#include "llresizehandle.h"

#include "llfocusmgr.h"
#include "llmath.h"
#include "llui.h"
#include "llmenugl.h"
#include "llcontrol.h"
#include "llfloater.h"
#include "llwindow.h"

const S32 RESIZE_BORDER_WIDTH = 3;

LLResizeHandle::Params::Params()
:	corner("corner"),
	min_width("min_width"),
	min_height("min_height")
{
	name = "resize_handle";
}

LLResizeHandle::LLResizeHandle(const LLResizeHandle::Params& p)
:	LLView(p),
	mDragLastScreenX( 0 ),
	mDragLastScreenY( 0 ),
	mLastMouseScreenX( 0 ),
	mLastMouseScreenY( 0 ),
	mImage( NULL ),
	mMinWidth( p.min_width ),
	mMinHeight( p.min_height ),
	mCorner( p.corner )
{
	if( RIGHT_BOTTOM == mCorner)
	{
		mImage = LLUI::getUIImage("Resize_Corner");
	}
	switch( p.corner )
	{
		case LEFT_TOP:		setFollows( FOLLOWS_LEFT | FOLLOWS_TOP );		break;
		case LEFT_BOTTOM:	setFollows( FOLLOWS_LEFT | FOLLOWS_BOTTOM );	break;
		case RIGHT_TOP:		setFollows( FOLLOWS_RIGHT | FOLLOWS_TOP );		break;
		case RIGHT_BOTTOM:	setFollows( FOLLOWS_RIGHT | FOLLOWS_BOTTOM );	break;
	}
}


BOOL LLResizeHandle::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if( pointInHandle(x, y) )
	{
		handled = TRUE;
		// Route future Mouse messages here preemptively.  (Release on mouse up.)
		// No handler needed for focus lost since this clas has no state that depends on it.
		gFocusMgr.setMouseCapture( this );

		localPointToScreen(x, y, &mDragLastScreenX, &mDragLastScreenY);
		mLastMouseScreenX = mDragLastScreenX;
		mLastMouseScreenY = mDragLastScreenY;
	}

	return handled;
}


BOOL LLResizeHandle::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	if( hasMouseCapture() )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );
		handled = TRUE;
	}
	else if( pointInHandle(x, y) )
	{
		handled = TRUE;
	}

	return handled;
}


BOOL LLResizeHandle::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// We only handle the click if the click both started and ended within us
	if( hasMouseCapture() )
	{
		// Make sure the mouse in still over the application.  We don't want to make the parent
		// so big that we can't see the resize handle any more.
	
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);
		const LLRect valid_rect = getRootView()->getRect();
		screen_x = llclamp( screen_x, valid_rect.mLeft, valid_rect.mRight );
		screen_y = llclamp( screen_y, valid_rect.mBottom, valid_rect.mTop );

		LLView* resizing_view = getParent();
		if( resizing_view )
		{
			// undock floater when user resize it
			LLFloater* floater_parent = dynamic_cast<LLFloater*>(getParent());
			if (floater_parent && floater_parent->isDocked()) 
			{
				floater_parent->setDocked(false, false);
			}

			// Resize the parent
			LLRect orig_rect = resizing_view->getRect();
			LLRect scaled_rect = orig_rect;
			S32 delta_x = screen_x - mDragLastScreenX;
			S32 delta_y = screen_y - mDragLastScreenY;
			LLCoordGL mouse_dir;
			// use hysteresis on mouse motion to preserve user intent when mouse stops moving
			mouse_dir.mX = (screen_x == mLastMouseScreenX) ? mLastMouseDir.mX : screen_x - mLastMouseScreenX;
			mouse_dir.mY = (screen_y == mLastMouseScreenY) ? mLastMouseDir.mY : screen_y - mLastMouseScreenY;
			mLastMouseScreenX = screen_x;
			mLastMouseScreenY = screen_y;
			mLastMouseDir = mouse_dir;

			S32 x_multiple = 1;
			S32 y_multiple = 1;
			switch( mCorner )
			{
			case LEFT_TOP:
				x_multiple = -1; 
				y_multiple =  1;	
				break;
			case LEFT_BOTTOM:	
				x_multiple = -1; 
				y_multiple = -1;	
				break;
			case RIGHT_TOP:		
				x_multiple =  1; 
				y_multiple =  1;	
				break;
			case RIGHT_BOTTOM:	
				x_multiple =  1; 
				y_multiple = -1;	
				break;
			}

			S32 new_width = orig_rect.getWidth() + x_multiple * delta_x;
			if( new_width < mMinWidth )
			{
				new_width = mMinWidth;
				delta_x = x_multiple * (mMinWidth - orig_rect.getWidth());
			}

			S32 new_height = orig_rect.getHeight() + y_multiple * delta_y;
			if( new_height < mMinHeight )
			{
				new_height = mMinHeight;
				delta_y = y_multiple * (mMinHeight - orig_rect.getHeight());
			}

			switch( mCorner )
			{
			case LEFT_TOP:		
				scaled_rect.translate(delta_x, 0);			
				break;
			case LEFT_BOTTOM:	
				scaled_rect.translate(delta_x, delta_y);	
				break;
			case RIGHT_TOP:		
				break;
			case RIGHT_BOTTOM:	
				scaled_rect.translate(0, delta_y);			
				break;
			}

			// temporarily set new parent rect
			scaled_rect.mRight = scaled_rect.mLeft + new_width;
			scaled_rect.mTop = scaled_rect.mBottom + new_height;
			resizing_view->setRect(scaled_rect);

			LLView* snap_view = NULL;
			LLView* test_view = NULL;

			static LLUICachedControl<S32> snap_margin ("SnapMargin", 0);
			// now do snapping
			switch(mCorner)
			{
			case LEFT_TOP:		
				snap_view = resizing_view->findSnapEdge(scaled_rect.mLeft, mouse_dir, SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				test_view = resizing_view->findSnapEdge(scaled_rect.mTop, mouse_dir, SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				if (!snap_view)
				{
					snap_view = test_view;
				}
				break;
			case LEFT_BOTTOM:	
				snap_view = resizing_view->findSnapEdge(scaled_rect.mLeft, mouse_dir, SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				test_view = resizing_view->findSnapEdge(scaled_rect.mBottom, mouse_dir, SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				if (!snap_view)
				{
					snap_view = test_view;
				}
				break;
			case RIGHT_TOP:		
				snap_view = resizing_view->findSnapEdge(scaled_rect.mRight, mouse_dir, SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				test_view = resizing_view->findSnapEdge(scaled_rect.mTop, mouse_dir, SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				if (!snap_view)
				{
					snap_view = test_view;
				}
				break;
			case RIGHT_BOTTOM:	
				snap_view = resizing_view->findSnapEdge(scaled_rect.mRight, mouse_dir, SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				test_view = resizing_view->findSnapEdge(scaled_rect.mBottom, mouse_dir, SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, snap_margin);
				if (!snap_view)
				{
					snap_view = test_view;
				}
				break;
			}

			// register "snap" behavior with snapped view
			resizing_view->setSnappedTo(snap_view);

			// reset parent rect
			resizing_view->setRect(orig_rect);

			// translate and scale to new shape
			resizing_view->setShape(scaled_rect, true);
			
			// update last valid mouse cursor position based on resized view's actual size
			LLRect new_rect = resizing_view->getRect();
			S32 actual_delta_x = 0;
			S32 actual_delta_y = 0;
			switch(mCorner)
			{
			case LEFT_TOP:
				actual_delta_x = new_rect.mLeft - orig_rect.mLeft;
				actual_delta_y = new_rect.mTop - orig_rect.mTop;
				if (actual_delta_x != delta_x
					|| actual_delta_y != delta_y)
				{
					new_rect.mRight = orig_rect.mRight;
					new_rect.mBottom = orig_rect.mBottom;
					resizing_view->setShape(new_rect, true);
				}

				mDragLastScreenX += actual_delta_x;
				mDragLastScreenY += actual_delta_y;
				break;
			case LEFT_BOTTOM:
				actual_delta_x = new_rect.mLeft - orig_rect.mLeft;
				actual_delta_y = new_rect.mBottom - orig_rect.mBottom;
				if (actual_delta_x != delta_x
					|| actual_delta_y != delta_y)
				{
					new_rect.mRight = orig_rect.mRight;
					new_rect.mTop = orig_rect.mTop;
					resizing_view->setShape(new_rect, true);
				}

				mDragLastScreenX += actual_delta_x;
				mDragLastScreenY += actual_delta_y;
				break;
			case RIGHT_TOP:
				actual_delta_x = new_rect.mRight - orig_rect.mRight;
				actual_delta_y = new_rect.mTop - orig_rect.mTop;
				if (actual_delta_x != delta_x
					|| actual_delta_y != delta_y)
				{
					new_rect.mLeft = orig_rect.mLeft;
					new_rect.mBottom = orig_rect.mBottom;
					resizing_view->setShape(new_rect, true);
				}

				mDragLastScreenX += actual_delta_x;
				mDragLastScreenY += actual_delta_y;
				break;
			case RIGHT_BOTTOM:
				actual_delta_x = new_rect.mRight - orig_rect.mRight;
				actual_delta_y = new_rect.mBottom - orig_rect.mBottom;
				if (actual_delta_x != delta_x
					|| actual_delta_y != delta_y)
				{
					new_rect.mLeft = orig_rect.mLeft;
					new_rect.mTop = orig_rect.mTop;
					resizing_view->setShape(new_rect, true);
				}

				mDragLastScreenX += actual_delta_x;
				mDragLastScreenY += actual_delta_y;
				break;
			default:
				break;
			}
		}

		handled = TRUE;
	}
	else // don't have mouse capture
	{
		if( pointInHandle( x, y ) )
		{
			handled = TRUE;
		}
	}

	if( handled )
	{
		switch( mCorner )
		{
		case RIGHT_BOTTOM:
		case LEFT_TOP:	
			getWindow()->setCursor(UI_CURSOR_SIZENWSE); 
			break;
		case LEFT_BOTTOM:
		case RIGHT_TOP:	
			getWindow()->setCursor(UI_CURSOR_SIZENESW); 
			break;
		}
	}

	return handled;
} // end handleHover


// assumes GL state is set for 2D
void LLResizeHandle::draw()
{
	if( mImage.notNull() && getVisible() && (RIGHT_BOTTOM == mCorner) ) 
	{
		mImage->draw(0, 0);
	}
}


BOOL LLResizeHandle::pointInHandle( S32 x, S32 y )
{
	if( pointInView(x, y) )
	{
		const S32 TOP_BORDER = (getRect().getHeight() - RESIZE_BORDER_WIDTH);
		const S32 RIGHT_BORDER = (getRect().getWidth() - RESIZE_BORDER_WIDTH);

		switch( mCorner )
		{
		case LEFT_TOP:		return (x <= RESIZE_BORDER_WIDTH) || (y >= TOP_BORDER);
		case LEFT_BOTTOM:	return (x <= RESIZE_BORDER_WIDTH) || (y <= RESIZE_BORDER_WIDTH);
		case RIGHT_TOP:		return (x >= RIGHT_BORDER) || (y >= TOP_BORDER);
		case RIGHT_BOTTOM:	return TRUE;
		}
	}
	return FALSE;
}
