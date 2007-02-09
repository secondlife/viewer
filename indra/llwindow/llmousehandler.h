/** 
 * @file llmousehandler.h
 * @brief LLMouseHandler class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_MOUSEHANDLER_H
#define LL_MOUSEHANDLER_H

#include "llstring.h"

// Abstract interface.
// Intended for use via multiple inheritance. 
// A class may have as many interfaces as it likes, but never needs to inherit one more than once.

#include "llstring.h"

class LLMouseHandler
{
public:
	LLMouseHandler() {}
	virtual ~LLMouseHandler() {}
	
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks) = 0;
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen) = 0;
	virtual const LLString& getName() const = 0;

	// Hack to support LLFocusMgr
	virtual BOOL isView() = 0;

	virtual void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const = 0;
	virtual void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const = 0;
};

#endif
