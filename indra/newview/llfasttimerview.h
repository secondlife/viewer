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
#include "llunit.h"
#include "lltracerecording.h"

class LLFastTimerView : public LLFloater
{
public:
	LLFastTimerView(const LLSD&);
	~LLFastTimerView();
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

	LLTrace::TimeBlock* getLegendID(S32 y);

protected:
	virtual	void	onClickCloseBtn();

private:	
	void drawTicks();
	void drawLineGraph();
	void drawLegend(S32 y);
	S32 drawHelp(S32 y);
	void drawBorders( S32 y, const S32 x_start, S32 barh, S32 dy);
	void drawBars();

	void printLineStats();
	void generateUniqueColors();
	void updateTotalTime();

	struct TimerBar
	{
		TimerBar()
		:	mWidth(0),
			mSelfWidth(0),
			mVisible(true),
			mStartFraction(0.f),
			mEndFraction(1.f)
		{}
		S32			mWidth;
		S32			mSelfWidth;
		LLRect		mVisibleRect,
					mChildrenRect;
		LLColor4	mColor;
		bool		mVisible;
		F32			mStartFraction,
					mEndFraction;
	};
	S32 updateTimerBarWidths(LLTrace::TimeBlock* time_block, std::vector<TimerBar>& bars, S32 history_index, bool visible);
	S32 updateTimerBarFractions(LLTrace::TimeBlock* time_block, S32 timer_bar_index, std::vector<TimerBar>& bars);
	S32 drawBar(LLTrace::TimeBlock* time_block, LLRect bar_rect, std::vector<TimerBar>& bars, S32 bar_index, LLPointer<LLUIImage>& bar_image);
	void setPauseState(bool pause_state);

	std::vector<TimerBar>* mTimerBars;
	S32 mDisplayMode;

	typedef enum child_alignment
	{
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT,
		ALIGN_COUNT
	} ChildAlignment;

	ChildAlignment mDisplayCenter;
	bool                          mDisplayCalls,
								  mDisplayHz;
	LLUnit<LLUnits::Seconds, F64> mAllTimeMax,
								  mTotalTimeDisplay;
	LLRect mBarRect;
	S32	mScrollIndex;
	LLTrace::TimeBlock*           mHoverID;
	LLTrace::TimeBlock*           mHoverTimer;
	LLRect					mToolTipRect;
	S32 mHoverBarIndex;
	LLFrameTimer mHighlightTimer;
	S32 mPrintStats;
	LLRect mGraphRect;
	LLTrace::PeriodicRecording*	  mRecording;
	bool						  mPauseHistory;
};

#endif
