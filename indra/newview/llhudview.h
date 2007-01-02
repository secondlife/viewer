/** 
 * @file llhudview.h
 * @brief 2D HUD overlay
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLHUDVIEW_H
#define LL_LLHUDVIEW_H

#include "llview.h"
#include "v4color.h"

class LLVector3d;

class LLHUDView
: public LLView
{
public:
	LLHUDView(const std::string& name, const LLRect& rect);
	virtual ~LLHUDView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual void draw();

	const LLColor4& colorFromType(S32 type);

protected:
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
};

extern LLHUDView *gHUDView;

#endif
