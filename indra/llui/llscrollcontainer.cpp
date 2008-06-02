/** 
 * @file llscrollcontainer.cpp
 * @brief LLScrollableContainerView base class
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

#include "llglimmediate.h"
#include "llscrollcontainer.h"
#include "llscrollbar.h"
#include "llui.h"
#include "llkeyboard.h"
#include "llviewborder.h"
#include "llfocusmgr.h"
#include "llframetimer.h"
#include "lluictrlfactory.h"
#include "llfontgl.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

static const S32 HORIZONTAL_MULTIPLE = 8;
static const S32 VERTICAL_MULTIPLE = 16;
static const F32 MIN_AUTO_SCROLL_RATE = 120.f;
static const F32 MAX_AUTO_SCROLL_RATE = 500.f;
static const F32 AUTO_SCROLL_RATE_ACCEL = 120.f;

///----------------------------------------------------------------------------
/// Class LLScrollableContainerView
///----------------------------------------------------------------------------

static LLRegisterWidget<LLScrollableContainerView> r("scroll_container");

// Default constructor
LLScrollableContainerView::LLScrollableContainerView( const LLString& name,
													  const LLRect& rect,
													  LLView* scrolled_view,
													  BOOL is_opaque,
													  const LLColor4& bg_color ) :
	LLUICtrl( name, rect, FALSE, NULL, NULL ),
	mScrolledView( scrolled_view ),
	mIsOpaque( is_opaque ),
	mBackgroundColor( bg_color ),
	mReserveScrollCorner( FALSE ),
	mAutoScrolling( FALSE ),
	mAutoScrollRate( 0.f )
{
	if( mScrolledView )
	{
		addChild( mScrolledView );
	}

	init();
}

// LLUICtrl constructor
LLScrollableContainerView::LLScrollableContainerView( const LLString& name, const LLRect& rect,
							   LLUICtrl* scrolled_ctrl, BOOL is_opaque,
							   const LLColor4& bg_color) :
	LLUICtrl( name, rect, FALSE, NULL, NULL ),
	mScrolledView( scrolled_ctrl ),
	mIsOpaque( is_opaque ),
	mBackgroundColor( bg_color ),
	mReserveScrollCorner( FALSE ),
	mAutoScrolling( FALSE ),
	mAutoScrollRate( 0.f )
{
	if( scrolled_ctrl )
	{
		addChild( scrolled_ctrl );
	}

	init();
}

void LLScrollableContainerView::init()
{
	LLRect border_rect( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	mBorder = new LLViewBorder( "scroll border", border_rect, LLViewBorder::BEVEL_IN );
	addChild( mBorder );

	mInnerRect.set( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	mInnerRect.stretch( -mBorder->getBorderWidth()  );

	LLRect vertical_scroll_rect = mInnerRect;
	vertical_scroll_rect.mLeft = vertical_scroll_rect.mRight - SCROLLBAR_SIZE;
	mScrollbar[VERTICAL] = new LLScrollbar( "scrollable vertical",
											vertical_scroll_rect,
											LLScrollbar::VERTICAL,
											mInnerRect.getHeight(), 
											0,
											mInnerRect.getHeight(),
											NULL, this,
											VERTICAL_MULTIPLE);
	addChild( mScrollbar[VERTICAL] );
	mScrollbar[VERTICAL]->setVisible( FALSE );
	mScrollbar[VERTICAL]->setFollowsRight();
	mScrollbar[VERTICAL]->setFollowsTop();
	mScrollbar[VERTICAL]->setFollowsBottom();
	
	LLRect horizontal_scroll_rect = mInnerRect;
	horizontal_scroll_rect.mTop = horizontal_scroll_rect.mBottom + SCROLLBAR_SIZE;
	mScrollbar[HORIZONTAL] = new LLScrollbar( "scrollable horizontal",
											  horizontal_scroll_rect,
											  LLScrollbar::HORIZONTAL,
											  mInnerRect.getWidth(),
											  0,
											  mInnerRect.getWidth(),
											  NULL, this,
											  HORIZONTAL_MULTIPLE);
	addChild( mScrollbar[HORIZONTAL] );
	mScrollbar[HORIZONTAL]->setVisible( FALSE );
	mScrollbar[HORIZONTAL]->setFollowsLeft();
	mScrollbar[HORIZONTAL]->setFollowsRight();

	setTabStop(FALSE);
}

// Destroys the object
LLScrollableContainerView::~LLScrollableContainerView( void )
{
	// mScrolledView and mScrollbar are child views, so the LLView
	// destructor takes care of memory deallocation.
	for( S32 i = 0; i < SCROLLBAR_COUNT; i++ )
	{
		mScrollbar[i] = NULL;
	}
	mScrolledView = NULL;
}

// internal scrollbar handlers
// virtual
void LLScrollableContainerView::scrollHorizontal( S32 new_pos )
{
	//llinfos << "LLScrollableContainerView::scrollHorizontal()" << llendl;
	if( mScrolledView )
	{
		LLRect doc_rect = mScrolledView->getRect();
		S32 old_pos = -(doc_rect.mLeft - mInnerRect.mLeft);
		mScrolledView->translate( -(new_pos - old_pos), 0 );
	}
}

// virtual
void LLScrollableContainerView::scrollVertical( S32 new_pos )
{
	// llinfos << "LLScrollableContainerView::scrollVertical() " << new_pos << llendl;
	if( mScrolledView )
	{
		LLRect doc_rect = mScrolledView->getRect();
		S32 old_pos = doc_rect.mTop - mInnerRect.mTop;
		mScrolledView->translate( 0, new_pos - old_pos );
	}
}

// LLView functionality
void LLScrollableContainerView::reshape(S32 width, S32 height,
										BOOL called_from_parent)
{
	LLUICtrl::reshape( width, height, called_from_parent );

	mInnerRect.set( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	mInnerRect.stretch( -mBorder->getBorderWidth() );

	if (mScrolledView)
	{
		const LLRect& scrolled_rect = mScrolledView->getRect();

		S32 visible_width = 0;
		S32 visible_height = 0;
		BOOL show_v_scrollbar = FALSE;
		BOOL show_h_scrollbar = FALSE;
		calcVisibleSize( scrolled_rect, &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );

		mScrollbar[VERTICAL]->setDocSize( scrolled_rect.getHeight() );
		mScrollbar[VERTICAL]->setPageSize( visible_height );

		mScrollbar[HORIZONTAL]->setDocSize( scrolled_rect.getWidth() );
		mScrollbar[HORIZONTAL]->setPageSize( visible_width );
	}
}

BOOL LLScrollableContainerView::handleKeyHere(KEY key, MASK mask)
{
	for( S32 i = 0; i < SCROLLBAR_COUNT; i++ )
	{
		if( mScrollbar[i]->handleKeyHere(key, mask) )
		{
			return TRUE;
		}
	}	

	return FALSE;
}

BOOL LLScrollableContainerView::handleScrollWheel( S32 x, S32 y, S32 clicks )
{
	for( S32 i = 0; i < SCROLLBAR_COUNT; i++ )
	{
		// Note: tries vertical and then horizontal

		// Pretend the mouse is over the scrollbar
		if( mScrollbar[i]->handleScrollWheel( 0, 0, clicks ) )
		{
			return TRUE;
		}
	}

	// Eat scroll wheel event (to avoid scrolling nested containers?)
	return TRUE;
}

BOOL LLScrollableContainerView::needsToScroll(S32 x, S32 y, LLScrollableContainerView::SCROLL_ORIENTATION axis) const
{
	if(mScrollbar[axis]->getVisible())
	{
		LLRect inner_rect_local( 0, mInnerRect.getHeight(), mInnerRect.getWidth(), 0 );
		const S32 AUTOSCROLL_SIZE = 10;
		if(mScrollbar[axis]->getVisible())
		{
			inner_rect_local.mRight -= SCROLLBAR_SIZE;
			inner_rect_local.mTop += AUTOSCROLL_SIZE;
			inner_rect_local.mBottom = inner_rect_local.mTop - AUTOSCROLL_SIZE;
		}
		if( inner_rect_local.pointInRect( x, y ) && (mScrollbar[axis]->getDocPos() > 0) )
		{
			return TRUE;
		}

	}
	return FALSE;
}

BOOL LLScrollableContainerView::handleDragAndDrop(S32 x, S32 y, MASK mask,
												  BOOL drop,
												  EDragAndDropType cargo_type,
												  void* cargo_data,
												  EAcceptance* accept,
												  LLString& tooltip_msg)
{
	// Scroll folder view if needed.  Never accepts a drag or drop.
	*accept = ACCEPT_NO;
	BOOL handled = FALSE;
	if( mScrollbar[HORIZONTAL]->getVisible() || mScrollbar[VERTICAL]->getVisible() )
	{
		const S32 AUTOSCROLL_SIZE = 10;
		S32 auto_scroll_speed = llround(mAutoScrollRate * LLFrameTimer::getFrameDeltaTimeF32());
		
		LLRect inner_rect_local( 0, mInnerRect.getHeight(), mInnerRect.getWidth(), 0 );
		if(	mScrollbar[HORIZONTAL]->getVisible() )
		{
			inner_rect_local.mBottom += SCROLLBAR_SIZE;
		}
		if(	mScrollbar[VERTICAL]->getVisible() )
		{
			inner_rect_local.mRight -= SCROLLBAR_SIZE;
		}

		if(	mScrollbar[HORIZONTAL]->getVisible() )
		{
			LLRect left_scroll_rect = inner_rect_local;
			left_scroll_rect.mRight = AUTOSCROLL_SIZE;
			if( left_scroll_rect.pointInRect( x, y ) && (mScrollbar[HORIZONTAL]->getDocPos() > 0) )
			{
				mScrollbar[HORIZONTAL]->setDocPos( mScrollbar[HORIZONTAL]->getDocPos() - auto_scroll_speed );
				mAutoScrolling = TRUE;
				handled = TRUE;
			}

			LLRect right_scroll_rect = inner_rect_local;
			right_scroll_rect.mLeft = inner_rect_local.mRight - AUTOSCROLL_SIZE;
			if( right_scroll_rect.pointInRect( x, y ) && (mScrollbar[HORIZONTAL]->getDocPos() < mScrollbar[HORIZONTAL]->getDocPosMax()) )
			{
				mScrollbar[HORIZONTAL]->setDocPos( mScrollbar[HORIZONTAL]->getDocPos() + auto_scroll_speed );
				mAutoScrolling = TRUE;
				handled = TRUE;
			}
		}
		if(	mScrollbar[VERTICAL]->getVisible() )
		{
			LLRect bottom_scroll_rect = inner_rect_local;
			bottom_scroll_rect.mTop = AUTOSCROLL_SIZE + bottom_scroll_rect.mBottom;
			if( bottom_scroll_rect.pointInRect( x, y ) && (mScrollbar[VERTICAL]->getDocPos() < mScrollbar[VERTICAL]->getDocPosMax()) )
			{
				mScrollbar[VERTICAL]->setDocPos( mScrollbar[VERTICAL]->getDocPos() + auto_scroll_speed );
				mAutoScrolling = TRUE;
				handled = TRUE;
			}

			LLRect top_scroll_rect = inner_rect_local;
			top_scroll_rect.mBottom = inner_rect_local.mTop - AUTOSCROLL_SIZE;
			if( top_scroll_rect.pointInRect( x, y ) && (mScrollbar[VERTICAL]->getDocPos() > 0) )
			{
				mScrollbar[VERTICAL]->setDocPos( mScrollbar[VERTICAL]->getDocPos() - auto_scroll_speed );
				mAutoScrolling = TRUE;
				handled = TRUE;
			}
		}
	}

	if( !handled )
	{
		handled = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type,
											cargo_data, accept, tooltip_msg) != NULL;
	}

	return TRUE;
}


BOOL LLScrollableContainerView::handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect)
{
	S32 local_x, local_y;
	for( S32 i = 0; i < SCROLLBAR_COUNT; i++ )
	{
		local_x = x - mScrollbar[i]->getRect().mLeft;
		local_y = y - mScrollbar[i]->getRect().mBottom;
		if( mScrollbar[i]->handleToolTip(local_x, local_y, msg, sticky_rect) )
		{
			return TRUE;
		}
	}
	// Handle 'child' view.
	if( mScrolledView )
	{
		local_x = x - mScrolledView->getRect().mLeft;
		local_y = y - mScrolledView->getRect().mBottom;
		if( mScrolledView->handleToolTip(local_x, local_y, msg, sticky_rect) )
		{
			return TRUE;
		}
	}

	// Opaque
	return TRUE;
}

void LLScrollableContainerView::calcVisibleSize( S32 *visible_width, S32 *visible_height, BOOL* show_h_scrollbar, BOOL* show_v_scrollbar ) const
{
	const LLRect& rect = mScrolledView->getRect();
	calcVisibleSize(rect, visible_width, visible_height, show_h_scrollbar, show_v_scrollbar);
}

void LLScrollableContainerView::calcVisibleSize( const LLRect& doc_rect, S32 *visible_width, S32 *visible_height, BOOL* show_h_scrollbar, BOOL* show_v_scrollbar ) const
{
	S32 doc_width = doc_rect.getWidth();
	S32 doc_height = doc_rect.getHeight();

	*visible_width = getRect().getWidth() - 2 * mBorder->getBorderWidth();
	*visible_height = getRect().getHeight() - 2 * mBorder->getBorderWidth();
	
	*show_v_scrollbar = FALSE;
	if( *visible_height < doc_height )
	{
		*show_v_scrollbar = TRUE;
		*visible_width -= SCROLLBAR_SIZE;
	}

	*show_h_scrollbar = FALSE;
	if( *visible_width < doc_width )
	{
		*show_h_scrollbar = TRUE;
		*visible_height -= SCROLLBAR_SIZE;

		// Must retest now that visible_height has changed
		if( !*show_v_scrollbar && (*visible_height < doc_height) )
		{
			*show_v_scrollbar = TRUE;
			*visible_width -= SCROLLBAR_SIZE;
		}
	}
}

void LLScrollableContainerView::draw()
{
	if (mAutoScrolling)
	{
		// add acceleration to autoscroll
		mAutoScrollRate = llmin(mAutoScrollRate + (LLFrameTimer::getFrameDeltaTimeF32() * AUTO_SCROLL_RATE_ACCEL), MAX_AUTO_SCROLL_RATE);
	}
	else
	{
		// reset to minimum
		mAutoScrollRate = MIN_AUTO_SCROLL_RATE;
	}
	// clear this flag to be set on next call to handleDragAndDrop
	mAutoScrolling = FALSE;

	// auto-focus when scrollbar active
	// this allows us to capture user intent (i.e. stop automatically scrolling the view/etc)
	if (!gFocusMgr.childHasKeyboardFocus(this) && 
		(mScrollbar[VERTICAL]->hasMouseCapture() || mScrollbar[HORIZONTAL]->hasMouseCapture()))
	{
		focusFirstItem();
	}

	// Draw background
	if( mIsOpaque )
	{
		LLGLSNoTexture no_texture;
		gGL.color4fv( mBackgroundColor.mV );
		gl_rect_2d( mInnerRect );
	}
	
	// Draw mScrolledViews and update scroll bars.
	// get a scissor region ready, and draw the scrolling view. The
	// scissor region ensures that we don't draw outside of the bounds
	// of the rectangle.
	if( mScrolledView )
	{
		updateScroll();

		// Draw the scrolled area.
		{
			S32 visible_width = 0;
			S32 visible_height = 0;
			BOOL show_v_scrollbar = FALSE;
			BOOL show_h_scrollbar = FALSE;
			calcVisibleSize( mScrolledView->getRect(), &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );

			LLLocalClipRect clip(LLRect(mInnerRect.mLeft, 
					mInnerRect.mBottom + (show_h_scrollbar ? SCROLLBAR_SIZE : 0) + visible_height,
					visible_width,
					mInnerRect.mBottom + (show_h_scrollbar ? SCROLLBAR_SIZE : 0)
					));
			drawChild(mScrolledView);
		}
	}

	// Highlight border if a child of this container has keyboard focus
	if( mBorder->getVisible() )
	{
		mBorder->setKeyboardFocusHighlight( gFocusMgr.childHasKeyboardFocus(this) );
	}

	// Draw all children except mScrolledView
	// Note: scrollbars have been adjusted by above drawing code
	for (child_list_const_reverse_iter_t child_iter = getChildList()->rbegin();
		 child_iter != getChildList()->rend(); ++child_iter)
	{
		LLView *viewp = *child_iter;
		if( sDebugRects )
		{
			sDepth++;
		}
		if( (viewp != mScrolledView) && viewp->getVisible() )
		{
			drawChild(viewp);
		}
		if( sDebugRects )
		{
			sDepth--;
		}
	}

	if (sDebugRects)
	{
		drawDebugRect();
	}

} // end draw

void LLScrollableContainerView::updateScroll()
{
	LLRect doc_rect = mScrolledView->getRect();
	S32 doc_width = doc_rect.getWidth();
	S32 doc_height = doc_rect.getHeight();
	S32 visible_width = 0;
	S32 visible_height = 0;
	BOOL show_v_scrollbar = FALSE;
	BOOL show_h_scrollbar = FALSE;
	calcVisibleSize( doc_rect, &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );

	S32 border_width = mBorder->getBorderWidth();
	if( show_v_scrollbar )
	{
		if( doc_rect.mTop < getRect().getHeight() - border_width )
		{
			mScrolledView->translate( 0, getRect().getHeight() - border_width - doc_rect.mTop );
		}

		scrollVertical(	mScrollbar[VERTICAL]->getDocPos() );
		mScrollbar[VERTICAL]->setVisible( TRUE );

		S32 v_scrollbar_height = visible_height;
		if( !show_h_scrollbar && mReserveScrollCorner )
		{
			v_scrollbar_height -= SCROLLBAR_SIZE;
		}
		mScrollbar[VERTICAL]->reshape( SCROLLBAR_SIZE, v_scrollbar_height, TRUE );

		// Make room for the horizontal scrollbar (or not)
		S32 v_scrollbar_offset = 0;
		if( show_h_scrollbar || mReserveScrollCorner )
		{
			v_scrollbar_offset = SCROLLBAR_SIZE;
		}
		LLRect r = mScrollbar[VERTICAL]->getRect();
		r.translate( 0, mInnerRect.mBottom - r.mBottom + v_scrollbar_offset );
		mScrollbar[VERTICAL]->setRect( r );
	}
	else
	{
		mScrolledView->translate( 0, getRect().getHeight() - border_width - doc_rect.mTop );

		mScrollbar[VERTICAL]->setVisible( FALSE );
		mScrollbar[VERTICAL]->setDocPos( 0 );
	}
		
	if( show_h_scrollbar )
	{
		if( doc_rect.mLeft > border_width )
		{
			mScrolledView->translate( border_width - doc_rect.mLeft, 0 );
			mScrollbar[HORIZONTAL]->setDocPos( 0 );
		}
		else
		{
			scrollHorizontal( mScrollbar[HORIZONTAL]->getDocPos() );
		}
	
		mScrollbar[HORIZONTAL]->setVisible( TRUE );
		S32 h_scrollbar_width = visible_width;
		if( !show_v_scrollbar && mReserveScrollCorner )
		{
			h_scrollbar_width -= SCROLLBAR_SIZE;
		}
		mScrollbar[HORIZONTAL]->reshape( h_scrollbar_width, SCROLLBAR_SIZE, TRUE );
	}
	else
	{
		mScrolledView->translate( border_width - doc_rect.mLeft, 0 );
		
		mScrollbar[HORIZONTAL]->setVisible( FALSE );
		mScrollbar[HORIZONTAL]->setDocPos( 0 );
	}

	mScrollbar[HORIZONTAL]->setDocSize( doc_width );
	mScrollbar[HORIZONTAL]->setPageSize( visible_width );

	mScrollbar[VERTICAL]->setDocSize( doc_height );
	mScrollbar[VERTICAL]->setPageSize( visible_height );
} // end updateScroll

void LLScrollableContainerView::setBorderVisible(BOOL b)
{
	mBorder->setVisible( b );
}

// Scroll so that as much of rect as possible is showing (where rect is defined in the space of scroller view, not scrolled)
void LLScrollableContainerView::scrollToShowRect(const LLRect& rect, const LLCoordGL& desired_offset)
{
	if (!mScrolledView)
	{
		llwarns << "LLScrollableContainerView::scrollToShowRect with no view!" << llendl;
		return;
	}

	S32 visible_width = 0;
	S32 visible_height = 0;
	BOOL show_v_scrollbar = FALSE;
	BOOL show_h_scrollbar = FALSE;
	const LLRect& scrolled_rect = mScrolledView->getRect();
	calcVisibleSize( scrolled_rect, &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );

	// can't be so far left that right side of rect goes off screen, or so far right that left side does
	S32 horiz_offset = llclamp(desired_offset.mX, llmin(0, -visible_width + rect.getWidth()), 0);
	// can't be so high that bottom of rect goes off screen, or so low that top does
	S32 vert_offset = llclamp(desired_offset.mY, 0, llmax(0, visible_height - rect.getHeight()));

	// Vertical
	// 1. First make sure the top is visible
	// 2. Then, if possible without hiding the top, make the bottom visible.
	S32 vert_pos = mScrollbar[VERTICAL]->getDocPos();

	// find scrollbar position to get top of rect on screen (scrolling up)
	S32 top_offset = scrolled_rect.mTop - rect.mTop - vert_offset;
	// find scrollbar position to get bottom of rect on screen (scrolling down)
	S32 bottom_offset = vert_offset == 0 ? scrolled_rect.mTop - rect.mBottom - visible_height : top_offset;
	// scroll up far enough to see top or scroll down just enough if item is bigger than visual area
	if( vert_pos >= top_offset || visible_height < rect.getHeight())
	{
		vert_pos = top_offset;
	}
	// else scroll down far enough to see bottom
	else
	if( vert_pos <= bottom_offset )
	{
		vert_pos = bottom_offset;
	}

	mScrollbar[VERTICAL]->setDocSize( scrolled_rect.getHeight() );
	mScrollbar[VERTICAL]->setPageSize( visible_height );
	mScrollbar[VERTICAL]->setDocPos( vert_pos );

	// Horizontal
	// 1. First make sure left side is visible
	// 2. Then, if possible without hiding the left side, make the right side visible.
	S32 horiz_pos = mScrollbar[HORIZONTAL]->getDocPos();
	S32 left_offset = rect.mLeft - scrolled_rect.mLeft + horiz_offset;
	S32 right_offset = horiz_offset == 0 ? rect.mRight - scrolled_rect.mLeft - visible_width : left_offset;

	if( horiz_pos >= left_offset || visible_width < rect.getWidth() )
	{
		horiz_pos = left_offset;
	}
	else if( horiz_pos <= right_offset )
	{
		horiz_pos = right_offset;
	}
	
	mScrollbar[HORIZONTAL]->setDocSize( scrolled_rect.getWidth() );
	mScrollbar[HORIZONTAL]->setPageSize( visible_width );
	mScrollbar[HORIZONTAL]->setDocPos( horiz_pos );

	// propagate scroll to document
	updateScroll();
}

void LLScrollableContainerView::pageUp(S32 overlap)
{
	mScrollbar[VERTICAL]->pageUp(overlap);
}

void LLScrollableContainerView::pageDown(S32 overlap)
{
	mScrollbar[VERTICAL]->pageDown(overlap);
}

void LLScrollableContainerView::goToTop()
{
	mScrollbar[VERTICAL]->setDocPos(0);
}

void LLScrollableContainerView::goToBottom()
{
	mScrollbar[VERTICAL]->setDocPos(mScrollbar[VERTICAL]->getDocSize());
}

S32 LLScrollableContainerView::getBorderWidth() const
{
	if (mBorder)
	{
		return mBorder->getBorderWidth();
	}

	return 0;
}

// virtual
LLXMLNodePtr LLScrollableContainerView::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLView::getXML();

	// Attributes

	node->createChild("opaque", TRUE)->setBoolValue(mIsOpaque);

	if (mIsOpaque)
	{
		node->createChild("color", TRUE)->setFloatValue(4, mBackgroundColor.mV);
	}

	// Contents

	LLXMLNodePtr child_node = mScrolledView->getXML();

	node->addChild(child_node);

	return node;
}

LLView* LLScrollableContainerView::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("scroll_container");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());
	
	BOOL opaque = FALSE;
	node->getAttributeBOOL("opaque", opaque);

	LLColor4 color(0,0,0,0);
	LLUICtrlFactory::getAttributeColor(node,"color", color);

	// Create the scroll view
	LLScrollableContainerView *ret = new LLScrollableContainerView(name, rect, (LLPanel*)NULL, opaque, color);

	LLPanel* panelp = NULL;

	// Find a child panel to add
	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		LLView *control = factory->createCtrlWidget(panelp, child);
		if (control && control->isPanel())
		{
			if (panelp)
			{
				llinfos << "Warning! Attempting to put multiple panels into a scrollable container view!" << llendl;
				delete control;
			}
			else
			{
				panelp = (LLPanel*)control;
			}
		}
	}

	if (panelp == NULL)
	{
		panelp = new LLPanel("dummy", LLRect::null, FALSE);
	}

	ret->mScrolledView = panelp;

	return ret;
}
