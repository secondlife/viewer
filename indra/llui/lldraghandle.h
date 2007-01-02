/** 
 * @file lldraghandle.h
 * @brief LLDragHandle base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// A widget for dragging a view around the screen using the mouse.

#ifndef LL_DRAGHANDLE_H
#define LL_DRAGHANDLE_H

#include "llview.h"
#include "v4color.h"
#include "llrect.h"
#include "llcoord.h"

class LLTextBox;

class LLDragHandle : public LLView
{
public:
	LLDragHandle(const LLString& name, const LLRect& rect, const LLString& title );

	virtual void setValue(const LLSD& value);

	void			setForeground(BOOL b)		{ mForeground = b; }
	void			setMaxTitleWidth(S32 max_width) {mMaxTitleWidth = llmin(max_width, mMaxTitleWidth); }
	void			setTitleVisible(BOOL visible);

	virtual void	setTitle( const LLString& title ) = 0;
	virtual const LLString&	getTitle() const = 0;
	virtual void	draw() = 0;
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE) = 0;

	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

protected:
	S32				mDragLastScreenX;
	S32				mDragLastScreenY;
	S32				mLastMouseScreenX;
	S32				mLastMouseScreenY;
	LLCoordGL		mLastMouseDir;
	LLColor4		mDragHighlightColor;
	LLColor4		mDragShadowColor;
	LLTextBox*		mTitleBox;
	S32				mMaxTitleWidth;
	BOOL			mForeground;

	// Pixels near the edge to snap floaters.
	static S32		sSnapMargin;
};


// Use this one for traditional top-of-window draggers
class LLDragHandleTop
: public LLDragHandle
{
public:
	LLDragHandleTop(const LLString& name, const LLRect& rect, const LLString& title );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual void	setTitle( const LLString& title );
	virtual const LLString& getTitle() const;
	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

private:
	void	reshapeTitleBox();
};


// Use this for left-side, vertical text draggers
class LLDragHandleLeft
: public LLDragHandle
{
public:
	LLDragHandleLeft(const LLString& name, const LLRect& rect, const LLString& title );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual void	setTitle( const LLString& title );
	virtual const LLString& getTitle() const;
	virtual void	draw();
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

};

const S32 DRAG_HANDLE_HEIGHT = 16;
const S32 DRAG_HANDLE_WIDTH = 16;

#endif  // LL_DRAGHANDLE_H
