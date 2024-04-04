/** 
 * @file llscrollcontainer.h
 * @brief LLScrollContainer class header file.
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

#ifndef LL_LLSCROLLCONTAINER_H
#define LL_LLSCROLLCONTAINER_H

#include "lluictrl.h"
#ifndef LL_V4COLOR_H
#include "v4color.h"
#endif
#include "llcoord.h"
#include "llscrollbar.h"


class LLViewBorder;
class LLUICtrlFactory;

/*****************************************************************************
 * 
 * A decorator view class meant to encapsulate a clipped region which is
 * scrollable. It automatically takes care of pixel perfect scrolling
 * and cliipping, as well as turning the scrollbars on or off based on
 * the width and height of the view you're scrolling.
 *
 *****************************************************************************/

struct ScrollContainerRegistry : public LLChildRegistry<ScrollContainerRegistry>
{
	LLSINGLETON_EMPTY_CTOR(ScrollContainerRegistry);
};

class LLScrollContainer : public LLUICtrl
{
public:
	// Note: vertical comes before horizontal because vertical
	// scrollbars have priority for mouse and keyboard events.

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<bool>		is_opaque,
							reserve_scroll_corner,
							border_visible,
							hide_scrollbar,
							ignore_arrow_keys;
		Optional<F32>		min_auto_scroll_rate,
							max_auto_scroll_rate;
		Optional<U32>		max_auto_scroll_zone;
		Optional<LLUIColor>	bg_color;
		Optional<LLScrollbar::callback_t> scroll_callback;
		Optional<S32>		size;
		
		Params();
	};

	// my valid children are stored in this registry
 	typedef ScrollContainerRegistry child_registry_t;

protected:
	LLScrollContainer(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLScrollContainer( void );

	virtual void 	setValue(const LLSD& value) { mInnerRect.setValue(value); }

	void			setBorderVisible( bool b );

	void			scrollToShowRect( const LLRect& rect, const LLRect& constraint);
	void			scrollToShowRect( const LLRect& rect) { scrollToShowRect(rect, LLRect(0, mInnerRect.getHeight(), mInnerRect.getWidth(), 0)); }

	void			setReserveScrollCorner( bool b ) { mReserveScrollCorner = b; }
	LLRect			getVisibleContentRect();
	LLRect			getContentWindowRect();
	virtual const LLRect	getScrolledViewRect() const { return mScrolledView ? mScrolledView->getRect() : LLRect::null; }
	void			pageUp(S32 overlap = 0);
	void			pageDown(S32 overlap = 0);
	void			goToTop();
	void			goToBottom();
	bool			isAtTop() const { return mScrollbar[VERTICAL]->isAtBeginning(); }
	bool			isAtBottom() const { return mScrollbar[VERTICAL]->isAtEnd(); }
    S32             getDocPosVertical() const { return mScrollbar[VERTICAL]->getDocPos(); }
    S32             getDocPosHorizontal() const { return mScrollbar[HORIZONTAL]->getDocPos(); }
	S32				getBorderWidth() const;

	// LLView functionality
	virtual void	reshape(S32 width, S32 height, bool called_from_parent = true);
	virtual bool	handleKeyHere(KEY key, MASK mask);
	virtual bool	handleUnicodeCharHere(llwchar uni_char);
	virtual bool	handleScrollWheel( S32 x, S32 y, S32 clicks );
	virtual bool	handleScrollHWheel( S32 x, S32 y, S32 clicks );
	virtual bool	handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);

	virtual void	draw();
	virtual bool	addChild(LLView* view, S32 tab_group = 0);

	bool canAutoScroll(S32 x, S32 y);
	bool autoScroll(S32 x, S32 y);

	S32 getSize() const { return mSize; }
	void setSize(S32 thickness);

protected:
	LLView*		mScrolledView;

private:
	// internal scrollbar handlers
	virtual void scrollHorizontal( S32 new_pos );
	virtual void scrollVertical( S32 new_pos );
	void updateScroll();
	bool autoScroll(S32 x, S32 y, bool do_scroll);
	void calcVisibleSize( S32 *visible_width, S32 *visible_height, bool* show_h_scrollbar, bool* show_v_scrollbar ) const;

	LLScrollbar* mScrollbar[ORIENTATION_COUNT];
	S32			mSize;
	bool		mIsOpaque;
	LLUIColor	mBackgroundColor;
	LLRect		mInnerRect;
	LLViewBorder* mBorder;
	bool		mReserveScrollCorner;
	bool		mAutoScrolling;
	F32			mAutoScrollRate;
	F32			mMinAutoScrollRate;
	F32			mMaxAutoScrollRate;
	U32			mMaxAutoScrollZone;
	bool		mHideScrollbar;
	bool		mIgnoreArrowKeys;
};


#endif // LL_LLSCROLLCONTAINER_H
