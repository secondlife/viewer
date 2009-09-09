/** 
 * @file llmousehandler.h
 * @brief LLMouseHandler class definition
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

#ifndef LL_MOUSEHANDLER_H
#define LL_MOUSEHANDLER_H

#include "linden_common.h"
#include "llrect.h"

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

	typedef enum {
		CLICK_LEFT,
		CLICK_MIDDLE,
		CLICK_RIGHT,
		CLICK_DOUBLELEFT
	} EClickType;

	virtual BOOL	handleAnyMouseClick(S32 x, S32 y, MASK mask, EClickType clicktype, BOOL down);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleMiddleMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask) = 0;

	virtual BOOL	handleHover(S32 x, S32 y, MASK mask) = 0;
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks) = 0;
	virtual BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect& sticky_rect_screen) = 0;
	virtual const std::string& getName() const = 0;

	virtual void	onMouseCaptureLost() = 0;

	// Hack to support LLFocusMgr
	virtual BOOL isView() const = 0;

	virtual void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const = 0;
	virtual void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const = 0;

	virtual BOOL hasMouseCapture() = 0;
};

#endif
