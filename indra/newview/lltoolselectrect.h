/** 
 * @file lltoolselectrect.h
 * @brief A tool to select multiple objects with a screen-space rectangle.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOLSELECTRECT_H
#define LL_LLTOOLSELECTRECT_H

#include "lltool.h"
#include "lltoolselect.h"

class LLToolSelectRect
:	public LLToolSelect
{
public:
	LLToolSelectRect( LLToolComposite* composite );

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual void	draw();							// draw the select rectangle

protected:
	void			handleRectangleSelection(S32 x, S32 y, MASK mask);	// true if you selected one
	BOOL			outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);

protected:
	S32				mDragStartX;					// screen coords, from left
	S32				mDragStartY;					// screen coords, from bottom

	S32				mDragEndX;
	S32				mDragEndY;

	S32				mDragLastWidth;
	S32				mDragLastHeight;

	BOOL			mMouseOutsideSlop;		// has mouse ever gone outside slop region?
};


#endif
