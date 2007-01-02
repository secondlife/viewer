/** 
 * @file llresizehandle.h
 * @brief LLResizeHandle base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_RESIZEHANDLE_H
#define LL_RESIZEHANDLE_H

#include "stdtypes.h"
#include "llview.h"
#include "llimagegl.h"
#include "llcoord.h"


class LLResizeHandle : public LLView
{
public:
	enum ECorner { LEFT_TOP, LEFT_BOTTOM, RIGHT_TOP, RIGHT_BOTTOM };

	
	LLResizeHandle(const LLString& name, const LLRect& rect, S32 min_width, S32 min_height, ECorner corner = RIGHT_BOTTOM );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual void	draw();
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);

	void			setResizeLimits( S32 min_width, S32 min_height ) { mMinWidth = min_width; mMinHeight = min_height; }

protected:
	BOOL			pointInHandle( S32 x, S32 y );

protected:
	S32				mDragStartScreenX;
	S32				mDragStartScreenY;
	S32				mLastMouseScreenX;
	S32				mLastMouseScreenY;
	LLCoordGL		mLastMouseDir;
	LLPointer<LLImageGL>	mImage;
	S32				mMinWidth;
	S32				mMinHeight;
	ECorner			mCorner;
};

const S32 RESIZE_HANDLE_HEIGHT = 16;
const S32 RESIZE_HANDLE_WIDTH = 16;

#endif  // LL_RESIZEHANDLE_H


