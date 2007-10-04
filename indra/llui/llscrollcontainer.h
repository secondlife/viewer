/** 
 * @file llscrollcontainer.h
 * @brief LLScrollableContainerView class header file.
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

#ifndef LL_LLSCROLLCONTAINER_H
#define LL_LLSCROLLCONTAINER_H

#include "lluictrl.h"
#ifndef LL_V4COLOR_H
#include "v4color.h"
#endif
#include "stdenums.h"
#include "llcoord.h"
#include "llscrollbar.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLScrollableContainerView
//
// A view meant to encapsulate a clipped region which is
// scrollable. It automatically takes care of pixel perfect scrolling
// and cliipping, as well as turning the scrollbars on or off based on
// the width and height of the view you're scrolling.
//
// This class is a decorator class.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLViewBorder;
class LLUICtrlFactory;


class LLScrollableContainerView : public LLUICtrl
{
public:
	// Note: vertical comes before horizontal because vertical
	// scrollbars have priority for mouse and keyboard events.
	enum SCROLL_ORIENTATION { VERTICAL, HORIZONTAL, SCROLLBAR_COUNT };

	LLScrollableContainerView( const LLString& name, const LLRect& rect,
							   LLView* scrolled_view, BOOL is_opaque = FALSE,
							   const LLColor4& bg_color = LLColor4(0,0,0,0) );
	LLScrollableContainerView( const LLString& name, const LLRect& rect,
							   LLUICtrl* scrolled_ctrl, BOOL is_opaque = FALSE,
							   const LLColor4& bg_color = LLColor4(0,0,0,0) );
	virtual ~LLScrollableContainerView( void );

	void init();

	void setScrolledView(LLView* view) { mScrolledView = view; }

	virtual void setValue(const LLSD& value) { mInnerRect.setValue(value); }
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_SCROLL_CONTAINER; }
	virtual LLString getWidgetTag() const { return LL_SCROLLABLE_CONTAINER_VIEW_TAG; }

	// scrollbar handlers
	static void		horizontalChange( S32 new_pos, LLScrollbar* sb, void* user_data );
	static void		verticalChange( S32 new_pos, LLScrollbar* sb, void* user_data );

	void			calcVisibleSize( S32 *visible_width, S32 *visible_height, BOOL* show_h_scrollbar, BOOL* show_v_scrollbar );
	void			calcVisibleSize( const LLRect& doc_rect, S32 *visible_width, S32 *visible_height, BOOL* show_h_scrollbar, BOOL* show_v_scrollbar );
	void			setBorderVisible( BOOL b );

	void			scrollToShowRect( const LLRect& rect, const LLCoordGL& desired_offset );
	void			setReserveScrollCorner( BOOL b ) { mReserveScrollCorner = b; }
	const LLRect&	getScrolledViewRect() { return mScrolledView->getRect(); }
	void			pageUp(S32 overlap = 0);
	void			pageDown(S32 overlap = 0);
	void			goToTop();
	void			goToBottom();
	S32				getBorderWidth();

	BOOL			needsToScroll(S32 x, S32 y, SCROLL_ORIENTATION axis);

	// LLView functionality
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent);
	virtual BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleScrollWheel( S32 x, S32 y, S32 clicks );
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   LLString& tooltip_msg);

	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect);
	virtual void	draw();

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

protected:
	// internal scrollbar handlers
	virtual void scrollHorizontal( S32 new_pos );
	virtual void scrollVertical( S32 new_pos );
	void updateScroll();

	LLScrollbar* mScrollbar[SCROLLBAR_COUNT];
	LLView*		mScrolledView;
	S32			mSize;
	BOOL		mIsOpaque;
	LLColor4	mBackgroundColor;
	LLRect		mInnerRect;
	LLViewBorder* mBorder;
	BOOL		mReserveScrollCorner;
	BOOL		mAutoScrolling;
	F32			mAutoScrollRate;
};


#endif // LL_LLSCROLLCONTAINER_H
