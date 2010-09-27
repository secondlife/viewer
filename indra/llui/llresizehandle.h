/** 
 * @file llresizehandle.h
 * @brief LLResizeHandle base class
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


