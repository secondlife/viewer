/** 
 * @file llscrollcontainer.cpp
 * @brief LLScrollContainer base class
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

#include "llscrollcontainer.h"

#include "llrender.h"
#include "llcontainerview.h"
#include "lllocalcliprect.h"
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
static const F32 AUTO_SCROLL_RATE_ACCEL = 120.f;

///----------------------------------------------------------------------------
/// Class LLScrollContainer
///----------------------------------------------------------------------------

static LLDefaultChildRegistry::Register<LLScrollContainer> r("scroll_container");

#include "llscrollingpanellist.h"
#include "llcontainerview.h"
#include "llpanel.h"

static ScrollContainerRegistry::Register<LLScrollingPanelList> r1("scrolling_panel_list");
static ScrollContainerRegistry::Register<LLContainerView> r2("container_view");
static ScrollContainerRegistry::Register<LLPanel> r3("panel", &LLPanel::fromXML);

LLScrollContainer::Params::Params()
:	is_opaque("opaque"),
	bg_color("color"),
	border_visible("border_visible"),
	hide_scrollbar("hide_scrollbar"),
	min_auto_scroll_rate("min_auto_scroll_rate", 100),
	max_auto_scroll_rate("max_auto_scroll_rate", 1000),
	reserve_scroll_corner("reserve_scroll_corner", false)
{}


// Default constructor
LLScrollContainer::LLScrollContainer(const LLScrollContainer::Params& p)
:	LLUICtrl(p),
	mAutoScrolling( FALSE ),
	mAutoScrollRate( 0.f ),
	mBackgroundColor(p.bg_color()),
	mIsOpaque(p.is_opaque),
	mHideScrollbar(p.hide_scrollbar),
	mReserveScrollCorner(p.reserve_scroll_corner),
	mMinAutoScrollRate(p.min_auto_scroll_rate),
	mMaxAutoScrollRate(p.max_auto_scroll_rate),
	mScrolledView(NULL)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	LLRect border_rect( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	LLViewBorder::Params params;
	params.name("scroll border");
	params.rect(border_rect);
	params.visible(p.border_visible);
	params.bevel_style(LLViewBorder::BEVEL_IN);
	mBorder = LLUICtrlFactory::create<LLViewBorder> (params);
	LLView::addChild( mBorder );

	mInnerRect.set( 0, getRect().getHeight(), getRect().getWidth(), 0 );
	mInnerRect.stretch( -getBorderWidth()  );

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
	sbparams.follows.flags(FOLLOWS_RIGHT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
	sbparams.visible(false);
	sbparams.change_callback(p.scroll_callback);
	mScrollbar[VERTICAL] = LLUICtrlFactory::create<LLScrollbar> (sbparams);
	LLView::addChild( mScrollbar[VERTICAL] );
	
	LLRect horizontal_scroll_rect = mInnerRect;
	horizontal_scroll_rect.mTop = horizontal_scroll_rect.mBottom + scrollbar_size;
	sbparams.name("scrollable horizontal");
	sbparams.rect(horizontal_scroll_rect);
	sbparams.orientation(LLScrollbar::HORIZONTAL);
	sbparams.doc_size(mInnerRect.getWidth());
	sbparams.doc_pos(0);
	sbparams.page_size(mInnerRect.getWidth());
	sbparams.step_size(VERTICAL_MULTIPLE);
	sbparams.visible(false);
	sbparams.follows.flags(FOLLOWS_LEFT | FOLLOWS_RIGHT);
	sbparams.change_callback(p.scroll_callback);
	mScrollbar[HORIZONTAL] = LLUICtrlFactory::create<LLScrollbar> (sbparams);
	LLView::addChild( mScrollbar[HORIZONTAL] );
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

	mInnerRect = getLocalRect();
	mInnerRect.stretch( -getBorderWidth() );

	if (mScrolledView)
	{
		const LLRect& scrolled_rect = mScrolledView->getRect();

		S32 visible_width = 0;
		S32 visible_height = 0;
		BOOL show_v_scrollbar = FALSE;
		BOOL show_h_scrollbar = FALSE;
		calcVisibleSize( &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );

		mScrollbar[VERTICAL]->setDocSize( scrolled_rect.getHeight() );
		mScrollbar[VERTICAL]->setPageSize( visible_height );

		mScrollbar[HORIZONTAL]->setDocSize( scrolled_rect.getWidth() );
		mScrollbar[HORIZONTAL]->setPageSize( visible_width );
		updateScroll();
	}
}

BOOL LLScrollContainer::handleKeyHere(KEY key, MASK mask)
{
	// allow scrolled view to handle keystrokes in case it delegated keyboard focus
	// to the scroll container.  
	// NOTE: this should not recurse indefinitely as handleKeyHere
	// should not propagate to parent controls, so mScrolledView should *not*
	// call LLScrollContainer::handleKeyHere in turn
	if (mScrolledView && mScrolledView->handleKeyHere(key, mask))
	{
		return TRUE;
	}
	for( S32 i = 0; i < SCROLLBAR_COUNT; i++ )
	{
		if( mScrollbar[i]->handleKeyHere(key, mask) )
		{
			updateScroll();
			return TRUE;
		}
	}	

	return FALSE;
}

BOOL LLScrollContainer::handleUnicodeCharHere(llwchar uni_char)
{
	if (mScrolledView && mScrolledView->handleUnicodeCharHere(uni_char))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL LLScrollContainer::handleScrollWheel( S32 x, S32 y, S32 clicks )
{
	// Give event to my child views - they may have scroll bars
	// (Bad UI design, but technically possible.)
	if (LLUICtrl::handleScrollWheel(x,y,clicks))
		return TRUE;

	// When the vertical scrollbar is visible, scroll wheel
	// only affects vertical scrolling.  It's confusing to have
	// scroll wheel perform both vertical and horizontal in a
	// single container.
	LLScrollbar* vertical = mScrollbar[VERTICAL];
	if (vertical->getVisible()
		&& vertical->getEnabled())
	{
		// Pretend the mouse is over the scrollbar
		if (vertical->handleScrollWheel( 0, 0, clicks ) )
		{
			updateScroll();
		}
		// Always eat the event
		return TRUE;
	}

	LLScrollbar* horizontal = mScrollbar[HORIZONTAL];
	// Test enablement and visibility for consistency with
	// LLView::childrenHandleScrollWheel().
	if (horizontal->getVisible()
		&& horizontal->getEnabled()
		&& horizontal->handleScrollWheel( 0, 0, clicks ) )
	{
		updateScroll();
		return TRUE;
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
	BOOL handled = autoScroll(x, y);

	if( !handled )
	{
		handled = childrenHandleDragAndDrop(x, y, mask, drop, cargo_type,
											cargo_data, accept, tooltip_msg) != NULL;
	}

	return TRUE;
}

bool LLScrollContainer::autoScroll(S32 x, S32 y)
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	bool scrolling = false;
	if( mScrollbar[HORIZONTAL]->getVisible() || mScrollbar[VERTICAL]->getVisible() )
	{
		LLRect screen_local_extents;
		screenRectToLocal(getRootView()->getLocalRect(), &screen_local_extents);

		LLRect inner_rect_local( 0, mInnerRect.getHeight(), mInnerRect.getWidth(), 0 );
		if(	mScrollbar[HORIZONTAL]->getVisible() )
		{
			inner_rect_local.mBottom += scrollbar_size;
		}
		if(	mScrollbar[VERTICAL]->getVisible() )
		{
			inner_rect_local.mRight -= scrollbar_size;
		}

		// clip rect against root view
		inner_rect_local.intersectWith(screen_local_extents);

		S32 auto_scroll_speed = llround(mAutoScrollRate * LLFrameTimer::getFrameDeltaTimeF32());
		// autoscroll region should take up no more than one third of visible scroller area
		S32 auto_scroll_region_width = llmin(inner_rect_local.getWidth() / 3, 10); 
		S32 auto_scroll_region_height = llmin(inner_rect_local.getHeight() / 3, 10); 

		if(	mScrollbar[HORIZONTAL]->getVisible() )
		{
			LLRect left_scroll_rect = screen_local_extents;
			left_scroll_rect.mRight = inner_rect_local.mLeft + auto_scroll_region_width;
			if( left_scroll_rect.pointInRect( x, y ) && (mScrollbar[HORIZONTAL]->getDocPos() > 0) )
			{
				mScrollbar[HORIZONTAL]->setDocPos( mScrollbar[HORIZONTAL]->getDocPos() - auto_scroll_speed );
				mAutoScrolling = TRUE;
				scrolling = true;
			}

			LLRect right_scroll_rect = screen_local_extents;
			right_scroll_rect.mLeft = inner_rect_local.mRight - auto_scroll_region_width;
			if( right_scroll_rect.pointInRect( x, y ) && (mScrollbar[HORIZONTAL]->getDocPos() < mScrollbar[HORIZONTAL]->getDocPosMax()) )
			{
				mScrollbar[HORIZONTAL]->setDocPos( mScrollbar[HORIZONTAL]->getDocPos() + auto_scroll_speed );
				mAutoScrolling = TRUE;
				scrolling = true;
			}
		}
		if(	mScrollbar[VERTICAL]->getVisible() )
		{
			LLRect bottom_scroll_rect = screen_local_extents;
			bottom_scroll_rect.mTop = inner_rect_local.mBottom + auto_scroll_region_height;
			if( bottom_scroll_rect.pointInRect( x, y ) && (mScrollbar[VERTICAL]->getDocPos() < mScrollbar[VERTICAL]->getDocPosMax()) )
			{
				mScrollbar[VERTICAL]->setDocPos( mScrollbar[VERTICAL]->getDocPos() + auto_scroll_speed );
				mAutoScrolling = TRUE;
				scrolling = true;
			}

			LLRect top_scroll_rect = screen_local_extents;
			top_scroll_rect.mBottom = inner_rect_local.mTop - auto_scroll_region_height;
			if( top_scroll_rect.pointInRect( x, y ) && (mScrollbar[VERTICAL]->getDocPos() > 0) )
			{
				mScrollbar[VERTICAL]->setDocPos( mScrollbar[VERTICAL]->getDocPos() - auto_scroll_speed );
				mAutoScrolling = TRUE;
				scrolling = true;
			}
		}
	}
	return scrolling;
}

void LLScrollContainer::calcVisibleSize( S32 *visible_width, S32 *visible_height, BOOL* show_h_scrollbar, BOOL* show_v_scrollbar ) const
{
	const LLRect& doc_rect = getScrolledViewRect();
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	S32 doc_width = doc_rect.getWidth();
	S32 doc_height = doc_rect.getHeight();

	S32 border_width = getBorderWidth();
	*visible_width = getRect().getWidth() - 2 * border_width;
	*visible_height = getRect().getHeight() - 2 * border_width;
	
	*show_v_scrollbar = FALSE;
	*show_h_scrollbar = FALSE;

	if (!mHideScrollbar)
	{
		// Note: 1 pixel change can happen on final animation and should not trigger 
		// the display of sliders.
		if ((doc_height - *visible_height) > 1)
		{
			*show_v_scrollbar = TRUE;
			*visible_width -= scrollbar_size;
		}
		if ((doc_width - *visible_width) > 1)
		{
			*show_h_scrollbar = TRUE;
			*visible_height -= scrollbar_size;
			// Note: Do *not* recompute *show_v_scrollbar here because with
			// The view inside the scroll container should not be extended
			// to container's full height to ensure the correct computation
			// of *show_v_scrollbar after subtracting horizontal scrollbar_size.

			if( !*show_v_scrollbar && ((doc_height - *visible_height) > 1) )
			{
				*show_v_scrollbar = TRUE;
				*visible_width -= scrollbar_size;
			}
		}
	}
}
	

void LLScrollContainer::draw()
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	if (mAutoScrolling)
	{
		// add acceleration to autoscroll
		mAutoScrollRate = llmin(mAutoScrollRate + (LLFrameTimer::getFrameDeltaTimeF32() * AUTO_SCROLL_RATE_ACCEL), mMaxAutoScrollRate);
	}
	else
	{
		// reset to minimum for next time
		mAutoScrollRate = mMinAutoScrollRate;
	}
	// clear this flag to be set on next call to autoScroll
	mAutoScrolling = FALSE;

	// auto-focus when scrollbar active
	// this allows us to capture user intent (i.e. stop automatically scrolling the view/etc)
	if (!hasFocus() 
		&& (mScrollbar[VERTICAL]->hasMouseCapture() || mScrollbar[HORIZONTAL]->hasMouseCapture()))
	{
		focusFirstItem();
	}

	if (getRect().isValid()) 
	{
		// Draw background
		if( mIsOpaque )
		{
			F32 alpha = getCurrentTransparency();

			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
			gl_rect_2d(mInnerRect, mBackgroundColor.get() % alpha);
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
				calcVisibleSize( &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );

				LLLocalClipRect clip(LLRect(mInnerRect.mLeft, 
						mInnerRect.mBottom + (show_h_scrollbar ? scrollbar_size : 0) + visible_height,
						mInnerRect.mRight - (show_v_scrollbar ? scrollbar_size: 0),
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
	}
} // end draw

bool LLScrollContainer::addChild(LLView* view, S32 tab_group)
{
	if (!mScrolledView)
	{
		// Use the first panel or container as the scrollable view (bit of a hack)
		mScrolledView = view;
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
	calcVisibleSize( &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );

	S32 border_width = getBorderWidth();
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
	// Recompute inner rect, as border visibility changes it
	mInnerRect = getLocalRect();
	mInnerRect.stretch( -getBorderWidth() );
}

LLRect LLScrollContainer::getVisibleContentRect()
{
	updateScroll();
	LLRect visible_rect = getContentWindowRect();
	LLRect contents_rect = mScrolledView->getRect();
	visible_rect.translate(-contents_rect.mLeft, -contents_rect.mBottom);
	return visible_rect;
}

LLRect LLScrollContainer::getContentWindowRect()
{
	updateScroll();
	LLRect scroller_view_rect;
	S32 visible_width = 0;
	S32 visible_height = 0;
	BOOL show_h_scrollbar = FALSE;
	BOOL show_v_scrollbar = FALSE;
	calcVisibleSize( &visible_width, &visible_height, &show_h_scrollbar, &show_v_scrollbar );
	S32 border_width = getBorderWidth();
	scroller_view_rect.setOriginAndSize(border_width, 
										show_h_scrollbar ? mScrollbar[HORIZONTAL]->getRect().mTop : border_width, 
										visible_width, 
										visible_height);
	return scroller_view_rect;
}

// rect is in document coordinates, constraint is in display coordinates relative to content window rect
void LLScrollContainer::scrollToShowRect(const LLRect& rect, const LLRect& constraint)
{
	if (!mScrolledView)
	{
		llwarns << "LLScrollContainer::scrollToShowRect with no view!" << llendl;
		return;
	}

	LLRect content_window_rect = getContentWindowRect();
	// get document rect
	LLRect scrolled_rect = mScrolledView->getRect();

	// shrink target rect to fit within constraint region, biasing towards top left
	LLRect rect_to_constrain = rect;
	rect_to_constrain.mBottom = llmax(rect_to_constrain.mBottom, rect_to_constrain.mTop - constraint.getHeight());
	rect_to_constrain.mRight = llmin(rect_to_constrain.mRight, rect_to_constrain.mLeft + constraint.getWidth());

	// calculate allowable positions for scroller window in document coordinates
	LLRect allowable_scroll_rect(rect_to_constrain.mRight - constraint.mRight,
								rect_to_constrain.mBottom - constraint.mBottom,
								rect_to_constrain.mLeft - constraint.mLeft,
								rect_to_constrain.mTop - constraint.mTop);

	// translate from allowable region for lower left corner to upper left corner
	allowable_scroll_rect.translate(0, content_window_rect.getHeight());

	S32 vert_pos = llclamp(mScrollbar[VERTICAL]->getDocPos(), 
					mScrollbar[VERTICAL]->getDocSize() - allowable_scroll_rect.mTop, // min vertical scroll
					mScrollbar[VERTICAL]->getDocSize() - allowable_scroll_rect.mBottom); // max vertical scroll	

	mScrollbar[VERTICAL]->setDocSize( scrolled_rect.getHeight() );
	mScrollbar[VERTICAL]->setPageSize( content_window_rect.getHeight() );
	mScrollbar[VERTICAL]->setDocPos( vert_pos );

	S32 horizontal_pos = llclamp(mScrollbar[HORIZONTAL]->getDocPos(), 
								allowable_scroll_rect.mLeft,
								allowable_scroll_rect.mRight);

	mScrollbar[HORIZONTAL]->setDocSize( scrolled_rect.getWidth() );
	mScrollbar[HORIZONTAL]->setPageSize( content_window_rect.getWidth() );
	mScrollbar[HORIZONTAL]->setDocPos( horizontal_pos );

	// propagate scroll to document
	updateScroll();

	// In case we are in accordion tab notify parent to show selected rectangle
	LLRect screen_rc;
	localRectToScreen(rect_to_constrain, &screen_rc);
	notifyParent(LLSD().with("scrollToShowRect",screen_rc.getValue()));
}

void LLScrollContainer::pageUp(S32 overlap)
{
	mScrollbar[VERTICAL]->pageUp(overlap);
	updateScroll();
}

void LLScrollContainer::pageDown(S32 overlap)
{
	mScrollbar[VERTICAL]->pageDown(overlap);
	updateScroll();
}

void LLScrollContainer::goToTop()
{
	mScrollbar[VERTICAL]->setDocPos(0);
	updateScroll();
}

void LLScrollContainer::goToBottom()
{
	mScrollbar[VERTICAL]->setDocPos(mScrollbar[VERTICAL]->getDocSize());
	updateScroll();
}

S32 LLScrollContainer::getBorderWidth() const
{
	if (mBorder->getVisible())
	{
		return mBorder->getBorderWidth();
	}

	return 0;
}

