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
#include "llunits.h"
#include "lltracerecording.h"
#include <deque>

class LLScrollbar;

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
	virtual void onOpen(const LLSD& key);
	virtual void onClose(bool app_quitting);
	LLTrace::BlockTimerStatHandle* getLegendID(S32 y);

private:	
	void drawTicks();
	void drawLineGraph();
	void drawLegend();
	void drawHelp(S32 y);
	void drawBorders( S32 y, const S32 x_start, S32 barh, S32 dy);
	void drawBars();

	void printLineStats();
	void generateUniqueColors();
	void updateTotalTime();

	struct TimerBar
	{
		TimerBar()
		:	mTotalTime(0),
			mSelfTime(0),
			mStartFraction(0.f),
			mEndFraction(1.f),
			mFirstChild(false),
			mLastChild(false)
		{}
		F32Seconds			mTotalTime,
							mSelfTime,
							mChildrenStart,
							mChildrenEnd,
							mSelfStart,
							mSelfEnd;
		LLTrace::BlockTimerStatHandle* mTimeBlock;
		bool				mVisible,
							mFirstChild,
							mLastChild;
		F32					mStartFraction,
							mEndFraction;
	};

	struct TimerBarRow
	{
		TimerBarRow() 
		:	mBottom(0),
			mTop(0),
			mBars(NULL)
		{}
		S32			mBottom,
					mTop;
		TimerBar*	mBars;
	};

	F32Seconds updateTimerBarWidths(LLTrace::BlockTimerStatHandle* time_block, TimerBarRow& row, S32 history_index, U32& bar_index);
	S32 updateTimerBarOffsets(LLTrace::BlockTimerStatHandle* time_block, TimerBarRow& row, S32 timer_bar_index = 0);
	S32 drawBar(LLRect bar_rect, TimerBarRow& row, S32 image_width, S32 image_height, bool hovered = false, bool visible = true, S32 bar_index = 0);
	void setPauseState(bool pause_state);

	std::deque<TimerBarRow> mTimerBarRows;
	TimerBarRow				mAverageTimerRow;

	enum EDisplayType
	{
		DISPLAY_TIME,
		DISPLAY_CALLS,
		DISPLAY_HZ
	}								mDisplayType;
	bool							mPauseHistory;
	F64Seconds						mAllTimeMax,
									mTotalTimeDisplay;
	S32								mScrollIndex,
									mHoverBarIndex,
									mStatsIndex;
	S32								mDisplayMode;
	LLTrace::BlockTimerStatHandle*				mHoverID;
	LLTrace::BlockTimerStatHandle*				mHoverTimer;
	LLRect							mToolTipRect,
									mGraphRect,
									mBarRect,
									mLegendRect;
	LLFrameTimer					mHighlightTimer;
	LLTrace::PeriodicRecording		mRecording;

	LLScrollbar* 					mScrollBar;
};

#endif
