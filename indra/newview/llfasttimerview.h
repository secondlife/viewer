/** 
 * @file llfasttimerview.h
 * @brief LLFastTimerView class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFASTTIMERVIEW_H
#define LL_LLFASTTIMERVIEW_H

#include "llfloater.h"
#include "llfasttimer.h"

class LLFastTimerView : public LLFloater
{
public:
	LLFastTimerView(const LLSD&);
	BOOL postBuild();

	static BOOL sAnalyzePerformance;

	static void outputAllMetrics();
	static void doAnalysis(std::string baseline, std::string target, std::string output);

private:
	static void doAnalysisDefault(std::string baseline, std::string target, std::string output) ;
	static LLSD analyzePerformanceLogDefault(std::istream& is) ;
	static void exportCharts(const std::string& base, const std::string& target);
	void onPause();

public:

	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL handleToolTip(S32 x, S32 y, MASK mask);
	virtual BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual void draw();

	LLFastTimer::NamedTimer* getLegendID(S32 y);
	F64 getTime(const std::string& name);

protected:
	virtual	void	onClickCloseBtn();
private:	
	typedef std::vector<std::vector<S32> > bar_positions_t;
	bar_positions_t mBarStart;
	bar_positions_t mBarEnd;
	S32 mDisplayMode;

	typedef enum child_alignment
	{
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT,
		ALIGN_COUNT
	} ChildAlignment;

	ChildAlignment mDisplayCenter;
	S32 mDisplayCalls;
	S32 mDisplayHz;
	U64 mAvgCountTotal;
	U64 mMaxCountTotal;
	LLRect mBarRect;
	S32	mScrollIndex;
	LLFastTimer::NamedTimer* mHoverID;
	LLFastTimer::NamedTimer* mHoverTimer;
	LLRect					mToolTipRect;
	S32 mHoverBarIndex;
	LLFrameTimer mHighlightTimer;
	S32 mPrintStats;
	S32 mAverageCyclesPerTimer;
	LLRect mGraphRect;
};

#endif
