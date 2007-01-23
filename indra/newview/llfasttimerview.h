/** 
 * @file llfasttimerview.h
 * @brief LLFastTimerView class definition
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFASTTIMERVIEW_H
#define LL_LLFASTTIMERVIEW_H

#include "llview.h"
#include "llframetimer.h"

class LLFastTimerView : public LLView
{
public:
	LLFastTimerView(const std::string& name, const LLRect& rect);
	virtual ~LLFastTimerView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void draw();

	S32 getLegendIndex(S32 y);
	F64 getTime(LLFastTimer::EFastTimerType tidx);
	
private:	
	S32* mBarStart;
	S32* mBarEnd;
	S32 mDisplayMode;
	S32 mDisplayCenter;
	S32 mDisplayCalls;
	U64 mAvgCountTotal;
	U64 mMaxCountTotal;
	LLRect mBarRect;
	S32	mScrollIndex;
	S32 mHoverIndex;
	S32 mHoverBarIndex;
	LLFrameTimer mHighlightTimer;
	S32 mSubtractHidden;
	S32 mPrintStats;
};

#endif
