/** 
 * @file llmousehandler.h
 * @brief LLMouseHandler class definition
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

#ifndef LL_MOUSEHANDLER_H
#define LL_MOUSEHANDLER_H

#include "linden_common.h"
#include "llrect.h"
#include "indra_constants.h"

// Mostly-abstract interface.
// Intended for use via multiple inheritance. 
// A class may have as many interfaces as it likes, but never needs to inherit one more than once.

class LLMouseHandler
{
public:
	LLMouseHandler() {}
	virtual ~LLMouseHandler() {}

	typedef enum {
		SHOW_NEVER,
		SHOW_IF_NOT_BLOCKED,
		SHOW_ALWAYS,
	} EShowToolTip;

	virtual bool	handleAnyMouseClick(S32 x, S32 y, MASK mask, EMouseClickType clicktype, bool down);
	virtual bool	handleMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual bool	handleMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual bool	handleMiddleMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual bool	handleMiddleMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual bool	handleRightMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual bool	handleRightMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual bool	handleDoubleClick(S32 x, S32 y, MASK mask) = 0;

	virtual bool	handleHover(S32 x, S32 y, MASK mask) = 0;
	virtual bool	handleScrollWheel(S32 x, S32 y, S32 clicks) = 0;
	virtual bool	handleScrollHWheel(S32 x, S32 y, S32 clicks) = 0;
	virtual bool	handleToolTip(S32 x, S32 y, MASK mask) = 0;
	virtual const std::string& getName() const = 0;

	virtual void	onMouseCaptureLost() = 0;

	virtual void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const = 0;
	virtual void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const = 0;

	virtual bool hasMouseCapture() = 0;
};

#endif
