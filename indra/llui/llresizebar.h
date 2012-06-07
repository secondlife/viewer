/** 
 * @file llresizebar.h
 * @brief LLResizeBar base class
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

#ifndef LL_RESIZEBAR_H
#define LL_RESIZEBAR_H

#include "llview.h"
#include "llcoord.h"

class LLResizeBar : public LLView
{
public:
	enum Side { LEFT, TOP, RIGHT, BOTTOM };

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<LLView*> resizing_view;
		Mandatory<Side>	side;

		Optional<S32>	min_size;
		Optional<S32>	max_size;
		Optional<bool>	snapping_enabled;
		Optional<bool>	allow_double_click_snapping;

		Params()
		:	max_size("max_size", S32_MAX),
			snapping_enabled("snapping_enabled", true),
			resizing_view("resizing_view"),
			side("side"),
			allow_double_click_snapping("allow_double_click_snapping", true)
		{
			name = "resize_bar";
		}
	};

protected:
	LLResizeBar(const LLResizeBar::Params& p);
	friend class LLUICtrlFactory;
public:

//	virtual void	draw();  No appearance
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);

	void			setResizeLimits( S32 min_size, S32 max_size ) { mMinSize = min_size; mMaxSize = max_size; }
	void			setEnableSnapping(BOOL enable) { mSnappingEnabled = enable; }
	void			setAllowDoubleClickSnapping(BOOL allow) { mAllowDoubleClickSnapping = allow; }
	bool			canResize() { return getEnabled() && mMaxSize > mMinSize; }

private:
	S32				mDragLastScreenX;
	S32				mDragLastScreenY;
	S32				mLastMouseScreenX;
	S32				mLastMouseScreenY;
	LLCoordGL		mLastMouseDir;
	S32				mMinSize;
	S32				mMaxSize;
	const Side		mSide;
	BOOL			mSnappingEnabled;
	BOOL			mAllowDoubleClickSnapping;
	LLView*			mResizingView;
};

#endif  // LL_RESIZEBAR_H


