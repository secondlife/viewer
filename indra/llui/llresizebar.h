/** 
 * @file llresizebar.h
 * @brief LLResizeBar base class
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
		:	max_size("", S32_MAX),
			snapping_enabled("", true),
			resizing_view("resizing_view"),
			side("side"),
			allow_double_click_snapping("", true)
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


