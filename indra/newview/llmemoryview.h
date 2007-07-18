/** 
 * @file llmemoryview.h
 * @brief LLMemoryView class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMEMORYVIEW_H
#define LL_LLMEMORYVIEW_H

#include "llview.h"

class LLMemoryView : public LLView
{
public:
	LLMemoryView(const std::string& name, const LLRect& rect);
	virtual ~LLMemoryView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual void draw();

private:
	void setDataDumpInterval(float delay);
	void dumpData();

	float mDelay;
	LLFrameTimer mDumpTimer;

private:
};

#endif
