/** 
 * @file lltoolpie.h
 * @brief LLToolPie class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TOOLPIE_H
#define LL_TOOLPIE_H

#include "lltool.h"
#include "lluuid.h"

class LLViewerObject;

class LLToolPie 
:	public LLTool
{
public:
	LLToolPie( );

	virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL		handleRightMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL		handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);

	virtual void		render();

	virtual void		stopEditing();

	virtual void		onMouseCaptureLost();
	virtual void		handleDeselect();

	static void			leftMouseCallback(S32 x, S32 y, MASK mask);
	static void			rightMouseCallback(S32 x, S32 y, MASK mask);

	static void			selectionPropertiesReceived();

protected:
	BOOL outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);
	BOOL pickAndShowMenu(S32 x, S32 y, MASK mask, BOOL edit_menu);
	BOOL useClickAction(BOOL always_show, MASK mask, LLViewerObject* object,
						LLViewerObject* parent);

protected:
	BOOL				mPieMouseButtonDown;
	BOOL				mGrabMouseButtonDown;
	BOOL				mHitLand;
	LLUUID				mHitObjectID;
	BOOL				mMouseOutsideSlop;				// for this drag, has mouse moved outside slop region
	static LLViewerObject* sClickActionObject;
};

extern LLToolPie *gToolPie;

#endif
