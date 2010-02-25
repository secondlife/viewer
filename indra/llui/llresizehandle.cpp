/** 
 * @file llresizehandle.cpp
 * @brief LLResizeHandle base class
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
			if (((LLFloater*)getParent())->isDocked())
			{
				((LLFloater*)getParent())->setDocked(false, false);
			}

			// Resize the parent
			LLRect orig_rect = resizing_view->getRect();
			LLRect scaled_rect = orig_rect;
			S32 delta_x = screen_x - mDragLastScreenX;
			S32 delta_y = screen_y - mDragLastScreenY;

			if(delta_x == 0 && delta_y == 0)
				return FALSE;

			LLCoordGL mouse_dir;
			// use hysteresis on mouse motion to preserve user intent when mouse stops moving
			mouse_dir.mX = (screen_x == mLastMouseScreenX) ? mLastMouseDir.mX : screen_x - mLastMouseScreenX;
			mouse_dir.mY = (screen_y == mLastMouseScreenY) ? mLastMouseDir.mY : screen_y - mLastMouseScreenY;
			
			mLastMouseScreenX = screen_x;
			mLastMouseScreenY = screen_y;
			mLastMouseDir = mouse_dir;

			S32 new_width = orig_rect.getWidth();
			S32 new_height = orig_rect.getHeight();

			S32 new_pos_x = orig_rect.mLeft;
			S32 new_pos_y = orig_rect.mTop;

			switch( mCorner )
			{
			case LEFT_TOP:
				new_width-=delta_x;
				new_height+=delta_y;
				new_pos_x+=delta_x;
				new_pos_y+=delta_y;
				break;
			case LEFT_BOTTOM:	
				new_width-=delta_x;
				new_height-=delta_y;
				new_pos_x+=delta_x;
				break;
			case RIGHT_TOP:		
				new_width+=delta_x;
				new_height+=delta_y;
				new_pos_y+=delta_y;
				break;
			case RIGHT_BOTTOM:	
				new_width+=delta_x;
				new_height-=delta_y;
				break;
			}

			new_width = llmax(new_width,mMinWidth);
			new_height = llmax(new_height,mMinHeight);
			
			LLRect::tCoordType screen_width = resizing_view->getParent()->getSnapRect().getWidth();
			LLRect::tCoordType screen_height = resizing_view->getParent()->getSnapRect().getHeight();
			
			new_width = llmin(new_width, screen_width);
			new_height = llmin(new_height, screen_height);
			
			// temporarily set new parent rect
			scaled_rect.setLeftTopAndSize(new_pos_x,new_pos_y,new_width,new_height);
				
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
			resizing_view->reshape(scaled_rect.getWidth(),scaled_rect.getHeight());
			resizing_view->setRect(scaled_rect);
			//set shape to handle dependent floaters...
			resizing_view->handleReshape(scaled_rect, false);
			
			
			// update last valid mouse cursor position based on resized view's actual size
			LLRect new_rect = resizing_view->getRect();
			switch(mCorner)
			{
			case LEFT_TOP:
				mDragLastScreenX += new_rect.mLeft - orig_rect.mLeft;
				mDragLastScreenY += new_rect.mTop - orig_rect.mTop;
				break;
			case LEFT_BOTTOM:
				mDragLastScreenX += new_rect.mLeft - orig_rect.mLeft;
				mDragLastScreenY += new_rect.mBottom- orig_rect.mBottom;
				break;
			case RIGHT_TOP:
				mDragLastScreenX += new_rect.mRight - orig_rect.mRight;
				mDragLastScreenY += new_rect.mTop - orig_rect.mTop;
				break;
			case RIGHT_BOTTOM:
				mDragLastScreenX += new_rect.mRight - orig_rect.mRight;
				mDragLastScreenY += new_rect.mBottom- orig_rect.mBottom;
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
