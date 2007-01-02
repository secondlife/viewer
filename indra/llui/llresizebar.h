/** 
 * @file llresizebar.h
 * @brief LLResizeBar base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_RESIZEBAR_H
#define LL_RESIZEBAR_H

#include "llview.h"
#include "llcoord.h"

class LLResizeBar : public LLView
{
public:
	enum Side { LEFT, TOP, RIGHT, BOTTOM };

	LLResizeBar(const LLString& name, const LLRect& rect, S32 min_width, S32 min_height, Side side );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

//	virtual void	draw();  No appearance
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

	void			setResizeLimits( S32 min_width, S32 min_height ) { mMinWidth = min_width; mMinHeight = min_height; }

protected:
	S32				mDragStartScreenX;
	S32				mDragStartScreenY;
	S32				mLastMouseScreenX;
	S32				mLastMouseScreenY;
	LLCoordGL		mLastMouseDir;
	S32				mMinWidth;
	S32				mMinHeight;
	Side			mSide;
};

const S32 RESIZE_BAR_HEIGHT = 3;

#endif  // LL_RESIZEBAR_H


