/** 
 * @file llscrollcontainer.cpp
 * @brief LLScrollContainer base class
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

#include "llscrollcontainer.h"

#include "llrender.h"
#include "llcontainerview.h"
// #include "llfolderview.h"
#include "llscrollingpanellist.h"
#include "llscrollbar.h"
#include "llui.h"
#include "llkeyboard.h"
#include "llviewborder.h"
#include "llfocusmgr.h"
#include "llframetimer.h"
#include "lluictrlfactory.h"
#include "llpanel.h"
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
/// Class LLScrollContainer
///----------------------------------------------------------------------------

static LLDefaultChildRegistry::Register<LLScrollContainer> r("scroll_container");

LLScrollContainer::Params::Params()
:	is_opaque("opaque"),
	bg_color("color"),
	reserve_scroll_corner("reserve_scroll_corner", false)
{
	name = "scroll_container";
	mouse_opaque(true);
	tab_stop(false);
}


// Default constructor
LLScrollContainer::LLScrollContainer(const LLScrollContainer::Params& p)
:	LLUICtrl(p),
	mAutoScrolling( FALSE ),
	mAutoScrollRate( 0.f ),
	mBackgroundColor(p.bg_color()),
	mIsOpaque(p.is_opaque),
	mReserveScrollCorner(p.reserve_scroll_corner),
	mScrolledView(NULL)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	LLRect border_rect( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	LLViewBorder::Params params;
	params.name("scroll border");
	params.rect(border_rect);
	params.bevel_style(LLViewBorder::BEVEL_IN);
	mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	LLView::addChild( mBorder );

	mInnerRect.set( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	mInnerRect.stretch( -mBorder->getBorderWidth()  );

	LLRect vertical_scroll_rect = mInnerRect;
	vertical_scroll_rect.mLeft = vertical_scroll_rect.mRight - scrollbar_size;
	LLScrollbar::Params sbparams;
	sbparams.name("scrollable vertical");
	sbparams.rect(vertical_scroll_rect);
	sbparams.orientation(LLScrollbar::VERTICAL);
	sbparams.doc_size(mInnerRect.getHeight());
	sbparams.doc_pos(0);
	sbparams.page_size(mInnerRect.getHeight());
	sbparams.step_size(VERTICAL_MULTIPLE);
	mScrollbar[VERTICAL] = LLUICtrlFactory::create<LLScrollbar> (sbparams);
	LLView::addChild( mScrollbar[VERTICAL] );
	mScrollbar[VERTICAL]->setVisible( FALSE );
	mScrollbar[VERTICAL]->setFollowsRight();
	mScrollbar[VERTICAL]->setFollowsTop();
	mScrollbar[VERTICAL]->setFollowsBottom();
	
	LLRect horizontal_scroll_rect = mInnerRect;
	horizontal_scroll_rect.mTop = horizontal_scroll_rect.mBottom + scrollbar_size;
	sbparams.name("scrollable horizontal");
	sbparams.rect(horizontal_scroll_rect);
	sbparams.orientation(LLScrollbar::HORIZONTAL);
	sbparams.doc_size(mInnerRect.getWidth());
	sbparams.doc_pos(0);
	sbparams.page_size(mInnerRect.getWidth());
	sbparams.step_size(VERTICAL_MULTIPLE);
	mScrollbar[HORIZONTAL] = LLUICtrlFactory::create<LLScrollbar> (sbparams);
	LLView::addChild( mScrollbar[HORIZONTAL] );
	mScrollbar[HORIZONTAL]->setVisible( FALSE );
	mScrollbar[HORIZONTAL]->setFollowsLeft();
	mScrollbar[HORIZONTAL]->setFollowsRight();
}

// Destroys the object
LLScrollContainer::~LLScrollContainer( void )
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
void LLScrollContainer::scrollHorizontal( S32 new_pos )
{
	//llinfos << "LLScrollContainer::scrollHorizontal()" << llendl;
	if( mScrolledView )
	{
		LLRect doc_rect = mScrolledView->getRect();
		S32 old_pos = -(doc_rect.mLeft - mInnerRect.mLeft);
		mScrolledView->translate( -(new_pos - old_pos), 0 );
	}
}

// virtual
void LLScrollContainer::scrollVertical( S32 new_pos )
{
	// llinfos << "LLScrollContainer::scrollVertical() " << new_pos << llendl;
	if( mScrolledView )
	{
		LLRect doc_rect = mScrolledView->getRect();
		S32 old_pos = doc_rect.mTop - mInnerRect.mTop;
		mScrolledView->translate( 0, new_pos - old_pos );
	}
}

// LLView functionality
void LLScrollContainer::reshape(S32 width, S32 height,
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

BOOL LLScrollContainer::handleKeyHere(KEY key, MASK mask)
{
	// allow scrolled view to handle keystrokes in case it delegated keyboard focus
	// to the scroll container.  
	// NOTE: this should not recurse indefinitely as handleKeyHere
	// should not propagate to parent controls, so mScrolledView should *not*
	// call LLScrollContainer::handleKeyHere in turn
	if (mScrolledView->handleKeyHere(key, mask))
	{
		return TRUE;
	}
	for( S32 i = 0; i < SCROLLBAR_COUNT; i++ )
	{
		if( mScrollbar[i]->handleKeyHere(key, mask) )
		{
			return TRUE;
		}
	}	

	return FALSE;
}

BOOL LLScrollContainer::handleScrollWheel( S32 x, S32 y, S32 clicks )
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

BOOL LLScrollContainer::needsToScroll(S32 x, S32 y, LLScrollContainer::SCROLL_ORIENTATION axis) const
{
	if(mScrollbar[axis]->getVisible())
	{
		LLRect inner_rect_local( 0, mInnerRect.getHeight(), mInnerRect.getWidth(), 0 );
		const S32 AUTOSCROLL_SIZE = 10;
		if(mScrollbar[axis]->getVisible())
		{
			static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
			inner_rect_local.mRight -= scrollbar_size;
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

BOOL LLScrollContainer::handleDragAndDrop(S32 x, S32 y, MASK mask,
												  BOOL drop,
												  EDragAndDropType cargo_type,
												  void* cargo_data,
												  EAcceptance* accept,
												  std::string& tooltip_msg)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
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
			inner_rect_local.mBottom += scrollbar_size;
		}
		if(	mScrollbar[VERTICAL]->getVisible() )
		{
			inner_rect_local.mRight -= scrollbar_size;
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


BOOL LLScrollContainer::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect)
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

void LLScrollContainer::calcVisibleSize( S32 *visible_width, S32 *visible_height, BOOL* show_h_scrollbar, BOOL* show_v_scrollbar ) const
{
	const LLRect& rect = mScrolledView->getRect();
	calcVisibleSize(rect, visible_width, visible_height, show_h_scrollbar, show_v_scrollbar);
}

void LLScrollContainer::calcVisibleSize( const LLRect& doc_rect, S32 *visible_width, S32 *visible_height, BOOL* show_h_scrollbar, BOOL* show_v_scrollbar ) const
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	S32 doc_width = doc_rect.getWidth();
	S32 doc_height = doc_rect.getHeight();

	*visible_width = getRect().getWidth() - 2 * mBorder->getBorderWidth();
	*visible_height = getRect().getHeight() - 2 * mBorder->getBorderWidth();
	
	*show_v_scrollbar = FALSE;
	if( *visible_height < doc_height )
	{
		*show_v_scrollbar = TRUE;
		*visible_width -= scrollbar_size;
	}

	*show_h_scrollbar = FALSE;
	if( *visible_width < doc_width )
	{
		*show_h_scrollbar = TRUE;
		*visible_height -= scrollbar_size;

		// Must retest now that visible_height has changed
		if( !*show_v_scrollbar && (*visible_height < doc_height) )
		{
			*show_v_scrollbar = TRUE;
			*visible_width -= scrollbar_size;
		}
	}
}
	

void LLScrollContainer::draw()
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
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
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4fv( mBackgroundColor.get().mV );
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
					mInnerRect.mBottom + (show_h_scrollbar ? scrollbar_size : 0) + visible_height,
					visible_width,
					mInnerRect.mBottom + (show_h_scrollbar ? scrollbar_size : 0)
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

	//// *HACK: also draw debug rectangles around currently-being-edited LLView, and any elements that are being highlighted by GUI preview code (see LLFloaterUIPreview)
	//std::set<LLView*>::iterator iter = std::find(sPreviewHighlightedElements.begin(), sPreviewHighlightedElements.end(), this);
	//if ((sEditingUI && this == sEditingUIView) || (iter != sPreviewHighlightedElements.end() && sDrawPreviewHighlights))
	//{
	//	drawDebugRect();
	//}

} // end draw

bool LLScrollContainer::addChild(LLView* view, S32 tab_group)
{
	if (!mScrolledView)
	{
		//*TODO: Move LLFolderView to llui and enable this check
// 		if (dynamic_cast<LLPanel*>(view) || dynamic_cast<LLContainerView*>(view) || dynamic_cast<LLScrollingPanelList*>(view) || dynamic_cast<LLFolderView*>(view))
		{
			// Use the first panel or container as the scrollable view (bit of a hack)
			mScrolledView = view;
		}
	}

	bool ret_val = LLView::addChild(view, tab_group);

	//bring the scrollbars to the front
	sendChildToFront( mScrollbar[HORIZONTAL] );
	sendChildToFront( mScrollbar[VERTICAL] );

	return ret_val;
}

void LLScrollContainer::updateScroll()
{
	if (!mScrolledView)
	{
		return;
	}
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
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
			v_scrollbar_height -= scrollbar_size;
		}
		mScrollbar[VERTICAL]->reshape( scrollbar_size, v_scrollbar_height, TRUE );

		// Make room for the horizontal scrollbar (or not)
		S32 v_scrollbar_offset = 0;
		if( show_h_scrollbar || mReserveScrollCorner )
		{
			v_scrollbar_offset = scrollbar_size;
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
			h_scrollbar_width -= scrollbar_size;
		}
		mScrollbar[HORIZONTAL]->reshape( h_scrollbar_width, scrollbar_size, TRUE );
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

void LLScrollContainer::setBorderVisible(BOOL b)
{
	mBorder->setVisible( b );
}

// Scroll so that as much of rect as possible is showing (where rect is defined in the space of scroller view, not scrolled)
void LLScrollContainer::scrollToShowRect(const LLRect& rect, const LLCoordGL& desired_offset)
{
	if (!mScrolledView)
	{
		llwarns << "LLScrollContainer::scrollToShowRect with no view!" << llendl;
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

void LLScrollContainer::pageUp(S32 overlap)
{
	mScrollbar[VERTICAL]->pageUp(overlap);
}

void LLScrollContainer::pageDown(S32 overlap)
{
	mScrollbar[VERTICAL]->pageDown(overlap);
}

void LLScrollContainer::goToTop()
{
	mScrollbar[VERTICAL]->setDocPos(0);
}

void LLScrollContainer::goToBottom()
{
	mScrollbar[VERTICAL]->setDocPos(mScrollbar[VERTICAL]->getDocSize());
}

S32 LLScrollContainer::getBorderWidth() const
{
	if (mBorder)
	{
		return mBorder->getBorderWidth();
	}

	return 0;
}

