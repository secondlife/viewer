/** 
 * @file llresizehandle.cpp
 * @brief LLResizeHandle base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

LLResizeHandle::LLResizeHandle( const LLString& name, const LLRect& rect, S32 min_width, S32 min_height, ECorner corner )
	:
	LLView( name, rect, TRUE ),
	mDragStartScreenX( 0 ),
	mDragStartScreenY( 0 ),
	mLastMouseScreenX( 0 ),
	mLastMouseScreenY( 0 ),
	mImage( NULL ),
	mMinWidth( min_width ),
	mMinHeight( min_height ),
	mCorner( corner )
{
	setSaveToXML(false);

	if( RIGHT_BOTTOM == mCorner)
	{
		LLUUID image_id(LLUI::sConfigGroup->getString("UIImgResizeBottomRightUUID"));
		mImage = LLUI::sImageProvider->getUIImageByID(image_id);
	}

	switch( mCorner )
	{
	case LEFT_TOP:		setFollows( FOLLOWS_LEFT | FOLLOWS_TOP );		break;
	case LEFT_BOTTOM:	setFollows( FOLLOWS_LEFT | FOLLOWS_BOTTOM );	break;
	case RIGHT_TOP:		setFollows( FOLLOWS_RIGHT | FOLLOWS_TOP );		break;
	case RIGHT_BOTTOM:	setFollows( FOLLOWS_RIGHT | FOLLOWS_BOTTOM );	break;
	}
}

EWidgetType LLResizeHandle::getWidgetType() const
{
	return WIDGET_TYPE_RESIZE_HANDLE;
}

LLString LLResizeHandle::getWidgetTag() const
{
	return LL_RESIZE_HANDLE_TAG;
}

BOOL LLResizeHandle::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
	if( getVisible() && pointInHandle(x, y) )
	{
		handled = TRUE;
		if( mEnabled )
		{
			// Route future Mouse messages here preemptively.  (Release on mouse up.)
			// No handler needed for focus lost since this clas has no state that depends on it.
			gFocusMgr.setMouseCapture( this, NULL );

			localPointToScreen(x, y, &mDragStartScreenX, &mDragStartScreenY);
			mLastMouseScreenX = mDragStartScreenX;
			mLastMouseScreenY = mDragStartScreenY;
		}
	}

	return handled;
}


BOOL LLResizeHandle::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	if( gFocusMgr.getMouseCapture() == this )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL, NULL );
		handled = TRUE;
	}
	else
	if( getVisible() && pointInHandle(x, y) )
	{
		handled = TRUE;
	}

	return handled;
}


BOOL LLResizeHandle::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// We only handle the click if the click both started and ended within us
	if( gFocusMgr.getMouseCapture() == this )
	{
		// Make sure the mouse in still over the application.  We don't want to make the parent
		// so big that we can't see the resize handle any more.
	
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y);
		const LLRect& valid_rect = gFloaterView->getRect(); // Assumes that the parent is a floater.
		screen_x = llclamp( screen_x, valid_rect.mLeft, valid_rect.mRight );
		screen_y = llclamp( screen_y, valid_rect.mBottom, valid_rect.mTop );

		LLView* parentView = getParent();
		if( parentView )
		{
			// Resize the parent
			LLRect parent_rect = parentView->getRect();
			LLRect scaled_rect = parent_rect;
			S32 delta_x = screen_x - mDragStartScreenX;
			S32 delta_y = screen_y - mDragStartScreenY;
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

			S32 new_width = parent_rect.getWidth() + x_multiple * delta_x;
			if( new_width < mMinWidth )
			{
				new_width = mMinWidth;
				delta_x = x_multiple * (mMinWidth - parent_rect.getWidth());
			}

			S32 new_height = parent_rect.getHeight() + y_multiple * delta_y;
			if( new_height < mMinHeight )
			{
				new_height = mMinHeight;
				delta_y = y_multiple * (mMinHeight - parent_rect.getHeight());
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
			parentView->setRect(scaled_rect);

			S32 snap_delta_x = 0;
			S32 snap_delta_y = 0;

			LLView* snap_view = NULL;
			LLView* test_view = NULL;

			// now do snapping
			switch(mCorner)
			{
			case LEFT_TOP:		
				snap_view = parentView->findSnapEdge(snap_delta_x, mouse_dir, SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_x -= scaled_rect.mLeft;
				test_view = parentView->findSnapEdge(snap_delta_y, mouse_dir, SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_y -= scaled_rect.mTop;
				if (!snap_view)
				{
					snap_view = test_view;
				}
				scaled_rect.mLeft += snap_delta_x;
				scaled_rect.mTop += snap_delta_y;
				break;
			case LEFT_BOTTOM:	
				snap_view = parentView->findSnapEdge(snap_delta_x, mouse_dir, SNAP_LEFT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_x -= scaled_rect.mLeft;
				test_view = parentView->findSnapEdge(snap_delta_y, mouse_dir, SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_y -= scaled_rect.mBottom;
				if (!snap_view)
				{
					snap_view = test_view;
				}
				scaled_rect.mLeft += snap_delta_x;
				scaled_rect.mBottom += snap_delta_y;
				break;
			case RIGHT_TOP:		
				snap_view = parentView->findSnapEdge(snap_delta_x, mouse_dir, SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_x -= scaled_rect.mRight;
				test_view = parentView->findSnapEdge(snap_delta_y, mouse_dir, SNAP_TOP, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_y -= scaled_rect.mTop;
				if (!snap_view)
				{
					snap_view = test_view;
				}
				scaled_rect.mRight += snap_delta_x;
				scaled_rect.mTop += snap_delta_y;
				break;
			case RIGHT_BOTTOM:	
				snap_view = parentView->findSnapEdge(snap_delta_x, mouse_dir, SNAP_RIGHT, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_x -= scaled_rect.mRight;
				test_view = parentView->findSnapEdge(snap_delta_y, mouse_dir, SNAP_BOTTOM, SNAP_PARENT_AND_SIBLINGS, LLUI::sConfigGroup->getS32("SnapMargin"));
				snap_delta_y -= scaled_rect.mBottom;
				if (!snap_view)
				{
					snap_view = test_view;
				}
				scaled_rect.mRight += snap_delta_x;
				scaled_rect.mBottom += snap_delta_y;
				break;
			}

			parentView->snappedTo(snap_view);

			// reset parent rect
			parentView->setRect(parent_rect);

			// translate and scale to new shape
			parentView->reshape(scaled_rect.getWidth(), scaled_rect.getHeight(), FALSE);
			parentView->translate(scaled_rect.mLeft - parentView->getRect().mLeft, scaled_rect.mBottom - parentView->getRect().mBottom);
			
			screen_x = mDragStartScreenX + delta_x + snap_delta_x;
			screen_y = mDragStartScreenY + delta_y + snap_delta_y;
			mDragStartScreenX = screen_x;
			mDragStartScreenY = screen_y;
		}

		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active) " << llendl;		
		handled = TRUE;
	}
	else
	if( getVisible() && pointInHandle( x, y ) )
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive) " << llendl;		
		handled = TRUE;
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
}

// assumes GL state is set for 2D
void LLResizeHandle::draw()
{
	if( mImage.notNull() && getVisible() && (RIGHT_BOTTOM == mCorner) ) 
	{
		gl_draw_image( 0, 0, mImage );
	}
}


BOOL LLResizeHandle::pointInHandle( S32 x, S32 y )
{
	if( pointInView(x, y) )
	{
		const S32 TOP_BORDER = (mRect.getHeight() - RESIZE_BORDER_WIDTH);
		const S32 RIGHT_BORDER = (mRect.getWidth() - RESIZE_BORDER_WIDTH);

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
