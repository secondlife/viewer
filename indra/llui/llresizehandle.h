/** 
 * @file llresizehandle.h
 * @brief LLResizeHandle base class
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

#ifndef LL_RESIZEHANDLE_H
#define LL_RESIZEHANDLE_H

#include "stdtypes.h"
#include "llview.h"
#include "llcoord.h"


class LLResizeHandle : public LLView
{
public:
	enum ECorner { LEFT_TOP, LEFT_BOTTOM, RIGHT_TOP, RIGHT_BOTTOM };

	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Mandatory<ECorner>	corner;
		Optional<S32>		min_width;
		Optional<S32>		min_height;
		Params();
	};

protected:
	LLResizeHandle(const LLResizeHandle::Params&);
	friend class LLUICtrlFactory;
public:
	virtual void	draw();
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

	void			setResizeLimits( S32 min_width, S32 min_height ) { mMinWidth = min_width; mMinHeight = min_height; }
	
private:
	BOOL			pointInHandle( S32 x, S32 y );

	S32				mDragLastScreenX;
	S32				mDragLastScreenY;
	S32				mLastMouseScreenX;
	S32				mLastMouseScreenY;
	LLCoordGL		mLastMouseDir;
	LLPointer<LLUIImage>	mImage;
	S32				mMinWidth;
	S32				mMinHeight;
	const ECorner	mCorner;
};

const S32 RESIZE_HANDLE_HEIGHT = 11;
const S32 RESIZE_HANDLE_WIDTH = 11;

#endif  // LL_RESIZEHANDLE_H


