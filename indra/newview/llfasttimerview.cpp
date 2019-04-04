/** 
 * @file llfasttimerview.cpp
 * @brief LLFastTimerView class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llfasttimerview.h"

#include "llviewerwindow.h"
#include "llrect.h"
#include "llcombobox.h"
#include "llerror.h"
#include "llgl.h"
#include "llimagepng.h"
#include "llrender.h"
#include "llrendertarget.h"
#include "lllocalcliprect.h"
#include "lllayoutstack.h"
#include "llmath.h"
#include "llfontgl.h"
#include "llsdserialize.h"
#include "lltooltip.h"
#include "llbutton.h"
#include "llscrollbar.h"

#include "llappviewer.h"
#include "llviewertexturelist.h"
#include "llui.h"
#include "llviewercontrol.h"

#include "llfasttimer.h"
#include "lltreeiterators.h"
#include "llmetricperformancetester.h"
#include "llviewerstats.h"

//////////////////////////////////////////////////////////////////////////////

using namespace LLTrace;

static const S32 MAX_VISIBLE_HISTORY = 12;
static const S32 LINE_GRAPH_HEIGHT = 240;
static const S32 MIN_BAR_HEIGHT = 3;
static const S32 RUNNING_AVERAGE_WIDTH = 100;
static const S32 NUM_FRAMES_HISTORY = 200;

std::vector<BlockTimerStatHandle*> ft_display_idx; // line of table entry for display purposes (for collapse)

BOOL LLFastTimerView::sAnalyzePerformance = FALSE;

S32 get_depth(const BlockTimerStatHandle* blockp)
{
	S32 depth = 0;
	BlockTimerStatHandle* timerp = blockp->getParent();
	while(timerp)
	{
		depth++;
		if (timerp->getParent() == timerp) break;
		timerp = timerp->getParent();
	}
	return depth;
}

LLFastTimerView::LLFastTimerView(const LLSD& key)
:	LLFloater(key),
	mHoverTimer(NULL),
	mDisplayMode(0),
	mDisplayType(DISPLAY_TIME),
	mScrollIndex(0),
	mHoverID(NULL),
	mHoverBarIndex(-1),
	mStatsIndex(-1),
	mPauseHistory(false),
	mRecording(NUM_FRAMES_HISTORY)
{
	mTimerBarRows.resize(NUM_FRAMES_HISTORY);
}

LLFastTimerView::~LLFastTimerView()
{
}

void LLFastTimerView::onPause()
{
	setPauseState(!mPauseHistory);
}

void LLFastTimerView::setPauseState(bool pause_state)
{
	if (pause_state == mPauseHistory) return;

	// reset scroll to bottom when unpausing
	if (!pause_state)
	{
		
		getChild<LLButton>("pause_btn")->setLabel(getString("pause"));
	}
	else
	{
		mScrollIndex = 0;

		getChild<LLButton>("pause_btn")->setLabel(getString("run"));
	}

	mPauseHistory = pause_state;
}

BOOL LLFastTimerView::postBuild()
{
	LLButton& pause_btn = getChildRef<LLButton>("pause_btn");
	mScrollBar = getChild<LLScrollbar>("scroll_vert");

	pause_btn.setCommitCallback(boost::bind(&LLFastTimerView::onPause, this));
	return TRUE;
}

BOOL LLFastTimerView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (mHoverTimer )
	{
		// right click collapses timers
		if (!mHoverTimer->getTreeNode().mCollapsed)
		{
			mHoverTimer->getTreeNode().mCollapsed = true;
		}
		else if (mHoverTimer->getParent())
		{
			mHoverTimer->getParent()->getTreeNode().mCollapsed = true;
		}
		return TRUE;
	}
	else if (mBarRect.pointInRect(x, y))
	{
		S32 bar_idx = MAX_VISIBLE_HISTORY - ((y - mBarRect.mBottom) * (MAX_VISIBLE_HISTORY + 2) / mBarRect.getHeight());
		bar_idx = llclamp(bar_idx, 0, MAX_VISIBLE_HISTORY);
		mStatsIndex = mScrollIndex + bar_idx;
		return TRUE;
	}
	return LLFloater::handleRightMouseDown(x, y, mask);
}

BlockTimerStatHandle* LLFastTimerView::getLegendID(S32 y)
{
	S32 idx = (mLegendRect.mTop - y) / (LLFontGL::getFontMonospace()->getLineHeight() + 2);

	if (idx >= 0 && idx < (S32)ft_display_idx.size())
	{
		return ft_display_idx[idx];
	}
	
	return NULL;
}

BOOL LLFastTimerView::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	for(LLTrace::block_timer_tree_df_iterator_t it = LLTrace::begin_block_timer_tree_df(FTM_FRAME);
		it != LLTrace::end_block_timer_tree_df();
		++it)
	{
		(*it)->getTreeNode().mCollapsed = false;
	}
	return TRUE;
}

BOOL LLFastTimerView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (x < mScrollBar->getRect().mLeft)
	{
		BlockTimerStatHandle* idp = getLegendID(y);
		if (idp)
		{
			idp->getTreeNode().mCollapsed = !idp->getTreeNode().mCollapsed;
		}
	}
	else if (mHoverTimer)
	{
		//left click drills down by expanding timers
		mHoverTimer->getTreeNode().mCollapsed = false;
	}
	else if (mGraphRect.pointInRect(x, y))
	{
		gFocusMgr.setMouseCapture(this);
		return TRUE;
	}

	return LLFloater::handleMouseDown(x, y, mask);
}

BOOL LLFastTimerView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
	}
	return LLFloater::handleMouseUp(x, y, mask);;
}

BOOL LLFastTimerView::handleHover(S32 x, S32 y, MASK mask)
{
	if (hasMouseCapture())
	{
		F32 lerp = llclamp(1.f - (F32) (x - mGraphRect.mLeft) / (F32) mGraphRect.getWidth(), 0.f, 1.f);
		mScrollIndex = ll_round( lerp * (F32)(mRecording.getNumRecordedPeriods() - MAX_VISIBLE_HISTORY));
		mScrollIndex = llclamp(	mScrollIndex, 0, (S32)mRecording.getNumRecordedPeriods());
		return TRUE;
	}
	mHoverTimer = NULL;
	mHoverID = NULL;

	if(mPauseHistory && mBarRect.pointInRect(x, y))
	{
		//const S32 bars_top = mBarRect.mTop;
		const S32 bars_top = mBarRect.mTop - ((S32)LLFontGL::getFontMonospace()->getLineHeight() + 4);

		mHoverBarIndex = llmin((bars_top - y) / (mBarRect.getHeight() / (MAX_VISIBLE_HISTORY + 2)) - 1,
								(S32)mRecording.getNumRecordedPeriods() - 1,
								MAX_VISIBLE_HISTORY);
		if (mHoverBarIndex == 0)
		{
			return TRUE;
		}
		else if (mHoverBarIndex < 0)
		{
			mHoverBarIndex = 0;
		}

		TimerBarRow& row = mHoverBarIndex == 0 ? mAverageTimerRow : mTimerBarRows[mScrollIndex + mHoverBarIndex - 1];

		TimerBar* hover_bar = NULL;
		F32Seconds mouse_time_offset = ((F32)(x - mBarRect.mLeft) / (F32)mBarRect.getWidth()) * mTotalTimeDisplay;
		for (int bar_index = 0, end_index = LLTrace::BlockTimerStatHandle::instance_tracker_t::instanceCount(); 
			bar_index < end_index; 
			++bar_index)
		{
			TimerBar& bar = row.mBars[bar_index];
			if (bar.mSelfStart > mouse_time_offset)
			{
				break;
			}
			if (bar.mSelfEnd > mouse_time_offset)
			{
				hover_bar = &bar;
				if (bar.mTimeBlock->getTreeNode().mCollapsed)
				{
					// stop on first collapsed BlockTimerStatHandle, since we can't select any children
					break;
				}
			}
		}

		if (hover_bar)
		{
			mHoverID = hover_bar->mTimeBlock;
			if (mHoverTimer != mHoverID)
			{
				// could be that existing tooltip is for a parent and is thus
				// covering region for this new timer, go ahead and unblock 
				// so we can create a new tooltip
				LLToolTipMgr::instance().unblockToolTips();
				mHoverTimer = mHoverID;
				mToolTipRect.set(mBarRect.mLeft + (hover_bar->mSelfStart / mTotalTimeDisplay) * mBarRect.getWidth(),
								row.mTop,
								mBarRect.mLeft + (hover_bar->mSelfEnd / mTotalTimeDisplay) * mBarRect.getWidth(),
								row.mBottom);
			}
		}
	}
	else if (x < mScrollBar->getRect().mLeft) 
	{
		BlockTimerStatHandle* timer_id = getLegendID(y);
		if (timer_id)
		{
			mHoverID = timer_id;
		}
	}
	
	return LLFloater::handleHover(x, y, mask);
}


static std::string get_tooltip(BlockTimerStatHandle& timer, S32 history_index, PeriodicRecording& frame_recording)
{
	std::string tooltip;
	if (history_index == 0)
	{
		// by default, show average number of call
		tooltip = llformat("%s (%d ms, %d calls)", timer.getName().c_str(), (S32)F64Milliseconds(frame_recording.getPeriodMean (timer, RUNNING_AVERAGE_WIDTH)).value(), (S32)frame_recording.getPeriodMean(timer.callCount(), RUNNING_AVERAGE_WIDTH));
	}
	else
	{
		tooltip = llformat("%s (%d ms, %d calls)", timer.getName().c_str(), (S32)F64Milliseconds(frame_recording.getPrevRecording(history_index).getSum(timer)).value(), (S32)frame_recording.getPrevRecording(history_index).getSum(timer.callCount()));
	}
	return tooltip;
}

BOOL LLFastTimerView::handleToolTip(S32 x, S32 y, MASK mask)
{
	if(mPauseHistory && mBarRect.pointInRect(x, y))
	{
		// tooltips for timer bars
		if (mHoverTimer)
		{
			LLRect screen_rect;
			localRectToScreen(mToolTipRect, &screen_rect);

			std::string tooltip = get_tooltip(*mHoverTimer, mHoverBarIndex > 0 ? mScrollIndex + mHoverBarIndex : 0, mRecording);

			LLToolTipMgr::instance().show(LLToolTip::Params()
				.message(tooltip)
				.sticky_rect(screen_rect)
				.delay_time(0.f));

			return TRUE;
		}
	}
	else
	{
		// tooltips for timer legend
		if (x < mScrollBar->getRect().mLeft) 
		{
			BlockTimerStatHandle* idp = getLegendID(y);
			if (idp)
			{
				LLToolTipMgr::instance().show(get_tooltip(*idp, 0, mRecording));

				return TRUE;
			}
		}
	}

	return LLFloater::handleToolTip(x, y, mask);
}

BOOL LLFastTimerView::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	setPauseState(true);
	mScrollIndex = llclamp(	mScrollIndex + clicks,
							0,
							llmin((S32)mRecording.getNumRecordedPeriods(), (S32)mRecording.getNumRecordedPeriods() - MAX_VISIBLE_HISTORY));
	return TRUE;
}

static BlockTimerStatHandle FTM_RENDER_TIMER("Timers");
static const S32 MARGIN = 10;

static std::vector<LLColor4> sTimerColors;

void LLFastTimerView::draw()
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_TIMER);
	
	if (!mPauseHistory)
	{
		mRecording.appendRecording(LLTrace::get_frame_recording().getLastRecording());
		mTimerBarRows.pop_back();
		mTimerBarRows.push_front(TimerBarRow());
	}

	mDisplayMode = llclamp(getChild<LLComboBox>("time_scale_combo")->getCurrentIndex(), 0, 3);
	mDisplayType = (EDisplayType)llclamp(getChild<LLComboBox>("metric_combo")->getCurrentIndex(), 0, 2);
		
	generateUniqueColors();

	LLView::drawChildren();
	//getChild<LLLayoutStack>("timer_bars_stack")->updateLayout();
	//getChild<LLLayoutStack>("legend_stack")->updateLayout();
	LLView* bars_panel = getChildView("bars_panel");
	bars_panel->localRectToOtherView(bars_panel->getLocalRect(), &mBarRect, this);

	LLView* lines_panel = getChildView("lines_panel");
	lines_panel->localRectToOtherView(lines_panel->getLocalRect(), &mGraphRect, this);

	LLView* legend_panel = getChildView("legend");
	legend_panel->localRectToOtherView(legend_panel->getLocalRect(), &mLegendRect, this);

	// Draw the window background
			gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gl_rect_2d(getLocalRect(), LLColor4(0.f, 0.f, 0.f, 0.25f));

	drawHelp(getRect().getHeight() - MARGIN);
	drawLegend();
			
	//mBarRect.mLeft = MARGIN + LEGEND_WIDTH + 8;
	//mBarRect.mTop = y;
	//mBarRect.mRight = getRect().getWidth() - MARGIN;
	//mBarRect.mBottom = MARGIN + LINE_GRAPH_HEIGHT;

	drawBars();
	drawLineGraph();
	printLineStats();
	LLView::draw();

	mAllTimeMax = llmax(mAllTimeMax, mRecording.getLastRecording().getSum(FTM_FRAME));
	mHoverID = NULL;
	mHoverBarIndex = -1;
}

void LLFastTimerView::onOpen(const LLSD& key)
{
	setPauseState(false);
	mRecording.reset();
	mRecording.appendPeriodicRecording(LLTrace::get_frame_recording());
	for(std::deque<TimerBarRow>::iterator it = mTimerBarRows.begin(), end_it = mTimerBarRows.end();
		it != end_it; 
		++it)
	{
		delete []it->mBars;
		it->mBars = NULL;
	}
}
										
void LLFastTimerView::onClose(bool app_quitting)
{
	setVisible(FALSE);
}

void saveChart(const std::string& label, const char* suffix, LLImageRaw* scratch)
{
	// disable use of glReadPixels which messes up nVidia nSight graphics debugging
	if (!LLRender::sNsightDebugSupport)
	{
		//read result back into raw image
		glReadPixels(0, 0, 1024, 512, GL_RGB, GL_UNSIGNED_BYTE, scratch->getData());

		//write results to disk
		LLPointer<LLImagePNG> result = new LLImagePNG();
		result->encode(scratch, 0.f);

		std::string ext = result->getExtension();
		std::string filename = llformat("%s_%s.%s", label.c_str(), suffix, ext.c_str());
	
		std::string out_file = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, filename);
		result->save(out_file);
	}
}

//static
void LLFastTimerView::exportCharts(const std::string& base, const std::string& target)
{
	//allocate render target for drawing charts 
	LLRenderTarget buffer;
	buffer.allocate(1024,512, GL_RGB, FALSE, FALSE);
	

	LLSD cur;

	LLSD base_data;

	{ //read base log into memory
		S32 i = 0;
		llifstream is(base.c_str());
		while (!is.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(cur, is))
		{
			base_data[i++] = cur;
		}
		is.close();
	}

	LLSD cur_data;
	std::set<std::string> chart_names;

	{ //read current log into memory
		S32 i = 0;
		llifstream is(target.c_str());
		while (!is.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(cur, is))
		{
			cur_data[i++] = cur;

			for (LLSD::map_iterator iter = cur.beginMap(); iter != cur.endMap(); ++iter)
			{
				std::string label = iter->first;
				chart_names.insert(label);
			}
		}
		is.close();
	}

	//get time domain
	LLSD::Real cur_total_time = 0.0;

	for (U32 i = 0; i < cur_data.size(); ++i)
	{
		cur_total_time += cur_data[i]["Total"]["Time"].asReal();
	}

	LLSD::Real base_total_time = 0.0;
	for (U32 i = 0; i < base_data.size(); ++i)
	{
		base_total_time += base_data[i]["Total"]["Time"].asReal();
	}

	//allocate raw scratch space
	LLPointer<LLImageRaw> scratch = new LLImageRaw(1024, 512, 3);

	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.loadIdentity();
	gGL.ortho(-0.05f, 1.05f, -0.05f, 1.05f, -1.0f, 1.0f);

	//render charts
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	
	buffer.bindTarget();

	for (std::set<std::string>::iterator iter = chart_names.begin(); iter != chart_names.end(); ++iter)
	{
		std::string label = *iter;
	
		LLSD::Real max_time = 0.0;
		LLSD::Integer max_calls = 0;
		LLSD::Real max_execution = 0.0;

		std::vector<LLSD::Real> cur_execution;
		std::vector<LLSD::Real> cur_times;
		std::vector<LLSD::Integer> cur_calls;

		std::vector<LLSD::Real> base_execution;
		std::vector<LLSD::Real> base_times;
		std::vector<LLSD::Integer> base_calls;

		for (U32 i = 0; i < cur_data.size(); ++i)
		{
			LLSD::Real time = cur_data[i][label]["Time"].asReal();
			LLSD::Integer calls = cur_data[i][label]["Calls"].asInteger();

			LLSD::Real execution = 0.0;
			if (calls > 0)
			{
				execution = time/calls;
				cur_execution.push_back(execution);
				cur_times.push_back(time);
			}

			cur_calls.push_back(calls);
		}

		for (U32 i = 0; i < base_data.size(); ++i)
		{
			LLSD::Real time = base_data[i][label]["Time"].asReal();
			LLSD::Integer calls = base_data[i][label]["Calls"].asInteger();

			LLSD::Real execution = 0.0;
			if (calls > 0)
			{
				execution = time/calls;
				base_execution.push_back(execution);
				base_times.push_back(time);
			}

			base_calls.push_back(calls);
		}

		std::sort(base_calls.begin(), base_calls.end());
		std::sort(base_times.begin(), base_times.end());
		std::sort(base_execution.begin(), base_execution.end());

		std::sort(cur_calls.begin(), cur_calls.end());
		std::sort(cur_times.begin(), cur_times.end());
		std::sort(cur_execution.begin(), cur_execution.end());

		//remove outliers
		const U32 OUTLIER_CUTOFF = 512;
		if (base_times.size() > OUTLIER_CUTOFF)
		{ 
			ll_remove_outliers(base_times, 1.f);
		}

		if (base_execution.size() > OUTLIER_CUTOFF)
		{ 
			ll_remove_outliers(base_execution, 1.f);
		}

		if (cur_times.size() > OUTLIER_CUTOFF)
		{ 
			ll_remove_outliers(cur_times, 1.f);
		}

		if (cur_execution.size() > OUTLIER_CUTOFF)
		{ 
			ll_remove_outliers(cur_execution, 1.f);
		}


		max_time = llmax(base_times.empty() ? 0.0 : *base_times.rbegin(), cur_times.empty() ? 0.0 : *cur_times.rbegin());
		max_calls = llmax(base_calls.empty() ? 0 : *base_calls.rbegin(), cur_calls.empty() ? 0 : *cur_calls.rbegin());
		max_execution = llmax(base_execution.empty() ? 0.0 : *base_execution.rbegin(), cur_execution.empty() ? 0.0 : *cur_execution.rbegin());


		LLVector3 last_p;

		//====================================
		// basic
		//====================================
		buffer.clear();

		last_p.clear();

		LLGLDisable cull(GL_CULL_FACE);

		LLVector3 base_col(0, 0.7f, 0.f);
		LLVector3 cur_col(1.f, 0.f, 0.f);

		gGL.setSceneBlendType(LLRender::BT_ADD);

		gGL.color3fv(base_col.mV);
		for (U32 i = 0; i < base_times.size(); ++i)
		{
			gGL.begin(LLRender::TRIANGLE_STRIP);
			gGL.vertex3fv(last_p.mV);
			gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
			last_p.set((F32)i/(F32) base_times.size(), base_times[i]/max_time, 0.f);
			gGL.vertex3fv(last_p.mV);
			gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
			gGL.end();
		}
		
		gGL.flush();

		
		last_p.clear();
		{
			LLGLEnable blend(GL_BLEND);
						
			gGL.color3fv(cur_col.mV);
			for (U32 i = 0; i < cur_times.size(); ++i)
			{
				gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
				gGL.vertex3fv(last_p.mV);
				last_p.set((F32) i / (F32) cur_times.size(), cur_times[i]/max_time, 0.f);
				gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
				gGL.vertex3fv(last_p.mV);
				gGL.end();
			}
			
			gGL.flush();
		}

		saveChart(label, "time", scratch);
		
		//======================================
		// calls
		//======================================
		buffer.clear();

		last_p.clear();

		gGL.color3fv(base_col.mV);
		for (U32 i = 0; i < base_calls.size(); ++i)
		{
			gGL.begin(LLRender::TRIANGLE_STRIP);
			gGL.vertex3fv(last_p.mV);
			gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
			last_p.set((F32) i / (F32) base_calls.size(), (F32)base_calls[i]/max_calls, 0.f);
			gGL.vertex3fv(last_p.mV);
			gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
			gGL.end();
		}
		
		gGL.flush();

		{
			LLGLEnable blend(GL_BLEND);
			gGL.color3fv(cur_col.mV);
			last_p.clear();

			for (U32 i = 0; i < cur_calls.size(); ++i)
			{
				gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
				gGL.vertex3fv(last_p.mV);
				last_p.set((F32) i / (F32) cur_calls.size(), (F32) cur_calls[i]/max_calls, 0.f);
				gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
				gGL.vertex3fv(last_p.mV);
				gGL.end();
				
			}
			
			gGL.flush();
		}

		saveChart(label, "calls", scratch);

		//======================================
		// execution
		//======================================
		buffer.clear();


		gGL.color3fv(base_col.mV);
		U32 count = 0;
		U32 total_count = base_execution.size();

		last_p.clear();

		for (std::vector<LLSD::Real>::iterator iter = base_execution.begin(); iter != base_execution.end(); ++iter)
		{
			gGL.begin(LLRender::TRIANGLE_STRIP);
			gGL.vertex3fv(last_p.mV);
			gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
			last_p.set((F32)count/(F32)total_count, *iter/max_execution, 0.f);
			gGL.vertex3fv(last_p.mV);
			gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
			gGL.end();
			count++;
		}

		last_p.clear();
				
		{
			LLGLEnable blend(GL_BLEND);
			gGL.color3fv(cur_col.mV);
			count = 0;
			total_count = cur_execution.size();

			for (std::vector<LLSD::Real>::iterator iter = cur_execution.begin(); iter != cur_execution.end(); ++iter)
			{
				gGL.begin(LLRender::TRIANGLE_STRIP);
				gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
				gGL.vertex3fv(last_p.mV);
				last_p.set((F32)count/(F32)total_count, *iter/max_execution, 0.f);			
				gGL.vertex3f(last_p.mV[0], 0.f, 0.f);
				gGL.vertex3fv(last_p.mV);
				gGL.end();
				count++;
			}

			gGL.flush();
		}

		saveChart(label, "execution", scratch);
	}

	buffer.flush();

	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();
}

//static
LLSD LLFastTimerView::analyzePerformanceLogDefault(std::istream& is)
{
	LLSD ret;

	LLSD cur;

	LLSD::Real total_time = 0.0;
	LLSD::Integer total_frames = 0;

	typedef std::map<std::string,LLViewerStats::StatsAccumulator> stats_map_t;
	stats_map_t time_stats;
	stats_map_t sample_stats;

	while (!is.eof() && LLSDParser::PARSE_FAILURE != LLSDSerialize::fromXML(cur, is))
	{
		for (LLSD::map_iterator iter = cur.beginMap(); iter != cur.endMap(); ++iter)
		{
			std::string label = iter->first;

			F64 time = iter->second["Time"].asReal();

			// Skip the total figure
			if(label.compare("Total") != 0)
			{
				total_time += time;
			}			

			if (time > 0.0)
			{
				LLSD::Integer samples = iter->second["Calls"].asInteger();

				time_stats[label].push(time);
				sample_stats[label].push(samples);
			}
		}
		total_frames++;
	}

	for(stats_map_t::iterator it = time_stats.begin(); it != time_stats.end(); ++it)
	{
		std::string label = it->first;
		ret[label]["TotalTime"] = time_stats[label].getSum();
		ret[label]["MeanTime"] = time_stats[label].getMean();
		ret[label]["MaxTime"] = time_stats[label].getMaxValue();
		ret[label]["MinTime"] = time_stats[label].getMinValue();
		ret[label]["StdDevTime"] = time_stats[label].getStdDev();
		
        ret[label]["Samples"] = sample_stats[label].getSum();
		ret[label]["MaxSamples"] = sample_stats[label].getMaxValue();
		ret[label]["MinSamples"] = sample_stats[label].getMinValue();
		ret[label]["StdDevSamples"] = sample_stats[label].getStdDev();

		ret[label]["Frames"] = (LLSD::Integer)time_stats[label].getCount();
	}
		
	ret["SessionTime"] = total_time;
	ret["FrameCount"] = total_frames;

	return ret;

}

//static
void LLFastTimerView::doAnalysisDefault(std::string baseline, std::string target, std::string output)
{
	// Open baseline and current target, exit if one is inexistent
	llifstream base_is(baseline.c_str());
	llifstream target_is(target.c_str());
	if (!base_is.is_open() || !target_is.is_open())
	{
		LL_WARNS() << "'-analyzeperformance' error : baseline or current target file inexistent" << LL_ENDL;
		base_is.close();
		target_is.close();
		return;
	}

	//analyze baseline
	LLSD base = analyzePerformanceLogDefault(base_is);
	base_is.close();

	//analyze current
	LLSD current = analyzePerformanceLogDefault(target_is);
	target_is.close();

	//output comparison
	llofstream os(output.c_str());

	LLSD::Real session_time = current["SessionTime"].asReal();
	os <<
		"Label, "
		"% Change, "
		"% of Session, "
		"Cur Min, "
		"Cur Max, "
		"Cur Mean/sample, "
		"Cur Mean/frame, "
		"Cur StdDev/frame, "
		"Cur Total, "
		"Cur Frames, "
		"Cur Samples, "
		"Base Min, "
		"Base Max, "
		"Base Mean/sample, "
		"Base Mean/frame, "
		"Base StdDev/frame, "
		"Base Total, "
		"Base Frames, "
		"Base Samples\n"; 

	for (LLSD::map_iterator iter = base.beginMap();  iter != base.endMap(); ++iter)
	{
		LLSD::String label = iter->first;

		if (current[label]["Samples"].asInteger() == 0 ||
			base[label]["Samples"].asInteger() == 0)
		{
			//cannot compare
			continue;
		}	
		LLSD::Real a = base[label]["TotalTime"].asReal() / base[label]["Samples"].asReal();
		LLSD::Real b = current[label]["TotalTime"].asReal() / current[label]["Samples"].asReal();
			
		LLSD::Real diff = b-a;

		LLSD::Real perc = diff/a * 100;

		os << llformat("%s, %.2f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %d, %d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %d, %d\n",
			label.c_str(), 
			(F32) perc, 
			(F32) (current[label]["TotalTime"].asReal()/session_time * 100.0), 

			(F32) current[label]["MinTime"].asReal(), 
			(F32) current[label]["MaxTime"].asReal(), 
			(F32) b, 
			(F32) current[label]["MeanTime"].asReal(), 
			(F32) current[label]["StdDevTime"].asReal(),
			(F32) current[label]["TotalTime"].asReal(), 
			current[label]["Frames"].asInteger(),
			current[label]["Samples"].asInteger(),
			(F32) base[label]["MinTime"].asReal(), 
			(F32) base[label]["MaxTime"].asReal(), 
			(F32) a, 
			(F32) base[label]["MeanTime"].asReal(), 
			(F32) base[label]["StdDevTime"].asReal(),
			(F32) base[label]["TotalTime"].asReal(), 
			base[label]["Frames"].asInteger(),
			base[label]["Samples"].asInteger());			
	}

	exportCharts(baseline, target);

	os.flush();
	os.close();
}

//static
void LLFastTimerView::outputAllMetrics()
{
	if (LLMetricPerformanceTesterBasic::hasMetricPerformanceTesters())
	{
		for (LLMetricPerformanceTesterBasic::name_tester_map_t::iterator iter = LLMetricPerformanceTesterBasic::sTesterMap.begin(); 
			iter != LLMetricPerformanceTesterBasic::sTesterMap.end(); ++iter)
		{
			LLMetricPerformanceTesterBasic* tester = ((LLMetricPerformanceTesterBasic*)iter->second);	
			tester->outputTestResults();
		}
	}
}

//static
void LLFastTimerView::doAnalysis(std::string baseline, std::string target, std::string output)
{
	if(BlockTimer::sLog)
	{
		doAnalysisDefault(baseline, target, output) ;
		return ;
	}

	if(BlockTimer::sMetricLog)
	{
		LLMetricPerformanceTesterBasic::doAnalysisMetrics(baseline, target, output) ;
		return ;
	}
}


void LLFastTimerView::printLineStats()
{
	// Output stats for clicked bar to log
	if (mStatsIndex >= 0)
	{
		std::string legend_stat;
		bool first = true;
		for(block_timer_tree_df_iterator_t it = LLTrace::begin_block_timer_tree_df(FTM_FRAME);
			it != LLTrace::end_block_timer_tree_df();
			++it)
		{
			BlockTimerStatHandle* idp = (*it);

			if (!first)
			{
				legend_stat += ", ";
			}
			first = false;
			legend_stat += idp->getName();

			//if (idp->getTreeNode().mCollapsed)
			//{
			//	it.skipDescendants();
			//}
		}
		LL_INFOS() << legend_stat << LL_ENDL;

		std::string timer_stat;
		first = true;
		for(block_timer_tree_df_iterator_t it = LLTrace::begin_block_timer_tree_df(FTM_FRAME);
			it != LLTrace::end_block_timer_tree_df();
			++it)
		{
			BlockTimerStatHandle* idp = (*it);

			if (!first)
			{
				timer_stat += ", ";
			}
			first = false;

			F32Seconds ticks;
			if (mStatsIndex == 0)
			{
				ticks = mRecording.getPeriodMean(*idp, RUNNING_AVERAGE_WIDTH);
			}
			else
			{
				ticks = mRecording.getPrevRecording(mStatsIndex).getSum(*idp);
			}
			F32Milliseconds ms = ticks;

			timer_stat += llformat("%.1f",ms.value());

			//if (idp->getTreeNode().mCollapsed)
			//{
			//	it.skipDescendants();
			//}
		}
		LL_INFOS() << timer_stat << LL_ENDL;
		mStatsIndex = -1;
	}
}

static LLTrace::BlockTimerStatHandle FTM_DRAW_LINE_GRAPH("Draw line graph");

void LLFastTimerView::drawLineGraph()
{
	LL_RECORD_BLOCK_TIME(FTM_DRAW_LINE_GRAPH);
	//draw line graph history
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLLocalClipRect clip(mGraphRect);

	//normalize based on last frame's maximum
	static F32Seconds max_time(0.000001);
	static U32 max_calls = 0;
	static F32 alpha_interp = 0.f;

	//highlight visible range
	{
		S32 first_frame = mRecording.getNumRecordedPeriods() - mScrollIndex;
		S32 last_frame = first_frame - MAX_VISIBLE_HISTORY;

		F32 frame_delta = ((F32) (mGraphRect.getWidth()))/(mRecording.getNumRecordedPeriods()-1);

		F32 right = (F32) mGraphRect.mLeft + frame_delta*first_frame;
		F32 left = (F32) mGraphRect.mLeft + frame_delta*last_frame;

		gGL.color4f(0.5f,0.5f,0.5f,0.3f);
		gl_rect_2d((S32) left, mGraphRect.mTop, (S32) right, mGraphRect.mBottom);

		if (mHoverBarIndex > 0)
		{
			S32 bar_frame = first_frame - (mScrollIndex + mHoverBarIndex) - 1;
			F32 bar = (F32) mGraphRect.mLeft + frame_delta*bar_frame;

			gGL.color4f(0.5f,0.5f,0.5f,1);

			gGL.begin(LLRender::LINES);
			gGL.vertex2i((S32)bar, mGraphRect.mBottom);
			gGL.vertex2i((S32)bar, mGraphRect.mTop);
			gGL.end();
		}
	}

	F32Seconds cur_max(0);
	U32 cur_max_calls = 0;
	for(block_timer_tree_df_iterator_t it = LLTrace::begin_block_timer_tree_df(FTM_FRAME);
		it != LLTrace::end_block_timer_tree_df();
		++it)
	{
		BlockTimerStatHandle* idp = (*it);

		//fatten highlighted timer
		if (mHoverID == idp)
		{
			gGL.flush();
			glLineWidth(3);
		}

		llassert(idp->getIndex() < sTimerColors.size());
		const F32 * col = sTimerColors[idp->getIndex()].mV;// ft_display_table[idx].color->mV;

		F32 alpha = 1.f;
		bool is_hover_timer = true;

		if (mHoverID != NULL &&
			mHoverID != idp)
		{	//fade out non-highlighted timers
			if (idp->getParent() != mHoverID)
			{
				alpha = alpha_interp;
				is_hover_timer = false;
			}
		}

		gGL.color4f(col[0], col[1], col[2], alpha);				
		gGL.begin(LLRender::TRIANGLE_STRIP);
		F32 call_scale_factor = (F32)mGraphRect.getHeight() / (F32)max_calls;
		F32 time_scale_factor = (F32)mGraphRect.getHeight() / max_time.value();
		F32 hz_scale_factor = (F32) mGraphRect.getHeight() / (1.f / max_time.value());
		for (U32 j = mRecording.getNumRecordedPeriods();
			j > 0;
			j--)
		{
			LLTrace::Recording& recording = mRecording.getPrevRecording(j);
			F32Seconds time = llmax(recording.getSum(*idp), F64Seconds(0.000001));
			U32 calls = recording.getSum(idp->callCount());

			if (is_hover_timer)
			{ 
				//normalize to highlighted timer
				cur_max = llmax(cur_max, time);
				cur_max_calls = llmax(cur_max_calls, calls);
			}
			F32 x = mGraphRect.mRight - j * (F32)(mGraphRect.getWidth())/(mRecording.getNumRecordedPeriods()-1);
			F32 y;
			switch(mDisplayType)
{
			case DISPLAY_TIME:
				y = mGraphRect.mBottom + time.value() * time_scale_factor;
				break;
			case DISPLAY_CALLS:
				y = mGraphRect.mBottom + (F32)calls * call_scale_factor;
				break;
			case DISPLAY_HZ:
				y = mGraphRect.mBottom + (1.f / time.value()) * hz_scale_factor;
				break;
			}
			gGL.vertex2f(x,y);
			gGL.vertex2f(x,mGraphRect.mBottom);
		}
		gGL.end();

		if (mHoverID == idp)
		{
			gGL.flush();
			glLineWidth(1);
		}

		if (idp->getTreeNode().mCollapsed)
		{	
			//skip hidden timers
			it.skipDescendants();
		}
	}
	
	//interpolate towards new maximum
	max_time = (F32Seconds)lerp(max_time.value(), cur_max.value(), LLSmoothInterpolation::getInterpolant(0.1f));
	if (llabs((max_time - cur_max).value()) <= 1)
	{
		max_time = llmax(F32Microseconds(1.f), F32Microseconds(cur_max));
	}

	max_calls = ll_round(lerp((F32)max_calls, (F32) cur_max_calls, LLSmoothInterpolation::getInterpolant(0.1f)));
	if (llabs((S32)(max_calls - cur_max_calls)) <= 1)
	{
		max_calls = cur_max_calls;
	}

	// TODO: make sure alpha is correct in DisplayHz mode
	F32 alpha_target = (max_time > cur_max)
		? llmin(max_time / cur_max - 1.f,1.f) 
		: llmin(cur_max/ max_time - 1.f,1.f);
	alpha_interp = lerp(alpha_interp, alpha_target, LLSmoothInterpolation::getInterpolant(0.1f));

	if (mHoverID != NULL)
	{
		S32 x = (mGraphRect.mRight + mGraphRect.mLeft)/2;
		S32 y = mGraphRect.mBottom + 8;

		LLFontGL::getFontMonospace()->renderUTF8(
			mHoverID->getName(), 
			0, 
			x, y, 
			LLColor4::white,
			LLFontGL::LEFT, LLFontGL::BOTTOM);
	}

	//display y-axis range
	std::string axis_label;
	switch(mDisplayType)
	{
	case DISPLAY_TIME:
		axis_label = llformat("%4.2f ms", F32Milliseconds(max_time).value());
		break;
	case DISPLAY_CALLS:
		axis_label = llformat("%d calls", (int)max_calls);
		break;
	case DISPLAY_HZ:
		axis_label = llformat("%4.2f Hz", max_time.value() ? 1.f / max_time.value() : 0.f);
		break;
	}

	LLFontGL* font = LLFontGL::getFontMonospace();
	S32 x = mGraphRect.mRight - font->getWidth(axis_label)-5;
	S32 y = mGraphRect.mTop - font->getLineHeight();;

	font->renderUTF8(axis_label, 0, x, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
}

void LLFastTimerView::drawLegend()
{
	// draw legend
	S32 dx;
	S32 x = mLegendRect.mLeft;
	S32 y = mLegendRect.mTop;
	const S32 TEXT_HEIGHT = (S32)LLFontGL::getFontMonospace()->getLineHeight();

	{
		LLLocalClipRect clip(mLegendRect);
		S32 cur_line = 0;
		S32 scroll_offset = 0; // element's y offset from top of the inner scroll's rect
		ft_display_idx.clear();
		std::map<BlockTimerStatHandle*, S32> display_line;
		for (block_timer_tree_df_iterator_t it = LLTrace::begin_block_timer_tree_df(FTM_FRAME);
			it != block_timer_tree_df_iterator_t();
			++it)
		{
			BlockTimerStatHandle* idp = (*it);
			// Needed to figure out offsets and parenting
			display_line[idp] = cur_line;
			cur_line++;
			if (scroll_offset < mScrollBar->getDocPos())
			{
				// only offset for visible items
				scroll_offset += TEXT_HEIGHT + 2;
				if (idp->getTreeNode().mCollapsed)
				{
					it.skipDescendants();
				}
				continue;
			}

			// used for mouse clicks
			ft_display_idx.push_back(idp);

			// Actual draw, first bar (square), then text
			x = MARGIN;

			LLRect bar_rect(x, y, x + TEXT_HEIGHT, y - TEXT_HEIGHT);
			S32 scale_offset = 0;
			if (idp == mHoverID)
			{
				scale_offset = llfloor(sinf(mHighlightTimer.getElapsedTimeF32() * 6.f) * 2.f);
			}
			bar_rect.stretch(scale_offset);
			llassert(idp->getIndex() < sTimerColors.size());
			gl_rect_2d(bar_rect, sTimerColors[idp->getIndex()]);

			F32Milliseconds ms(0);
			S32 calls = 0;
			if (mHoverBarIndex > 0 && mHoverID)
			{
				S32 hidx = mScrollIndex + mHoverBarIndex;
				ms = mRecording.getPrevRecording(hidx).getSum(*idp);
				calls = mRecording.getPrevRecording(hidx).getSum(idp->callCount());
			}
			else
			{
				ms = F64Seconds(mRecording.getPeriodMean(*idp, RUNNING_AVERAGE_WIDTH));
				calls = (S32)mRecording.getPeriodMean(idp->callCount(), RUNNING_AVERAGE_WIDTH);
			}

			std::string timer_label;
			switch(mDisplayType)
			{
			case DISPLAY_TIME:
				timer_label = llformat("%s [%.1f]",idp->getName().c_str(),ms.value());
				break;
			case DISPLAY_CALLS:
				timer_label = llformat("%s (%d)",idp->getName().c_str(),calls);
				break;
			case DISPLAY_HZ:
				timer_label = llformat("%.1f", ms.value() ? (1.f / ms.value()) : 0.f);
				break;
			}
			dx = (TEXT_HEIGHT+4) + get_depth(idp)*8;

			LLColor4 color = LLColor4::white;
			if (get_depth(idp) > 0)
			{
				S32 line_start_y = bar_rect.getCenterY();
				S32 line_end_y = line_start_y + ((TEXT_HEIGHT + 2) * (cur_line - display_line[idp->getParent()])) - TEXT_HEIGHT;
				gl_line_2d(x + dx - 8, line_start_y, x + dx, line_start_y, color);
				S32 line_x = x + (TEXT_HEIGHT + 4) + ((get_depth(idp) - 1) * 8);
				gl_line_2d(line_x, line_start_y, line_x, line_end_y, color);
				if (idp->getTreeNode().mCollapsed && !idp->getChildren().empty())
				{
					gl_line_2d(line_x+4, line_start_y-3, line_x+4, line_start_y+4, color);
				}
			}

			x += dx;
			BOOL is_child_of_hover_item = (idp == mHoverID);
			BlockTimerStatHandle* next_parent = idp->getParent();
			while(!is_child_of_hover_item && next_parent)
			{
				is_child_of_hover_item = (mHoverID == next_parent);
				if (next_parent->getParent() == next_parent) break;
				next_parent = next_parent->getParent();
			}

			LLFontGL::getFontMonospace()->renderUTF8(timer_label, 0, 
				x, y, 
				color, 
				LLFontGL::LEFT, LLFontGL::TOP, 
				is_child_of_hover_item ? LLFontGL::BOLD : LLFontGL::NORMAL);

			y -= (TEXT_HEIGHT + 2);

			scroll_offset += TEXT_HEIGHT + 2;
			if (idp->getTreeNode().mCollapsed) 
			{
				it.skipDescendants();
			}
		}
		// Recalculate scroll size
		mScrollBar->setDocSize(scroll_offset - mLegendRect.getHeight());
	}
}

void LLFastTimerView::generateUniqueColors()
{
	// generate unique colors
	{
		sTimerColors.resize(LLTrace::BlockTimerStatHandle::getNumIndices());
		sTimerColors[FTM_FRAME.getIndex()] = LLColor4::grey;

		F32 hue = 0.f;

		for (block_timer_tree_df_iterator_t it = LLTrace::begin_block_timer_tree_df(FTM_FRAME);
			it != block_timer_tree_df_iterator_t();
			++it)
		{
			BlockTimerStatHandle* idp = (*it);

			const F32 HUE_INCREMENT = 0.23f;
			hue = fmodf(hue + HUE_INCREMENT, 1.f);
			// saturation increases with depth
			F32 saturation = clamp_rescale((F32)get_depth(idp), 0.f, 3.f, 0.f, 1.f);
			// lightness alternates with depth
			F32 lightness = get_depth(idp) % 2 ? 0.5f : 0.6f;

			LLColor4 child_color;
			child_color.setHSL(hue, saturation, lightness);

			llassert(idp->getIndex() < sTimerColors.size());
			sTimerColors[idp->getIndex()] = child_color;
		}
	}
}

void LLFastTimerView::drawHelp( S32 y )
{
	// Draw some help
	const S32 texth = (S32)LLFontGL::getFontMonospace()->getLineHeight();

	y -= (texth + 2);
	y -= (texth + 2);

	LLFontGL::getFontMonospace()->renderUTF8(std::string("[Right-Click log selected]"),
		0, MARGIN, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
}

void LLFastTimerView::drawTicks()
{
	// Draw MS ticks
	{
		U32Milliseconds ms = mTotalTimeDisplay;
		std::string tick_label;
		S32 x;
		S32 barw = mBarRect.getWidth();

		tick_label = llformat("%.1f ms |", (F32)ms.value()*.25f);
		x = mBarRect.mLeft + barw/4 - LLFontGL::getFontMonospace()->getWidth(tick_label);
		LLFontGL::getFontMonospace()->renderUTF8(tick_label, 0, x, mBarRect.mTop, LLColor4::white,
			LLFontGL::LEFT, LLFontGL::TOP);

		tick_label = llformat("%.1f ms |", (F32)ms.value()*.50f);
		x = mBarRect.mLeft + barw/2 - LLFontGL::getFontMonospace()->getWidth(tick_label);
		LLFontGL::getFontMonospace()->renderUTF8(tick_label, 0, x, mBarRect.mTop, LLColor4::white,
			LLFontGL::LEFT, LLFontGL::TOP);

		tick_label = llformat("%.1f ms |", (F32)ms.value()*.75f);
		x = mBarRect.mLeft + (barw*3)/4 - LLFontGL::getFontMonospace()->getWidth(tick_label);
		LLFontGL::getFontMonospace()->renderUTF8(tick_label, 0, x, mBarRect.mTop, LLColor4::white,
			LLFontGL::LEFT, LLFontGL::TOP);

		tick_label = llformat( "%d ms |", (U32)ms.value());
		x = mBarRect.mLeft + barw - LLFontGL::getFontMonospace()->getWidth(tick_label);
		LLFontGL::getFontMonospace()->renderUTF8(tick_label, 0, x, mBarRect.mTop, LLColor4::white,
			LLFontGL::LEFT, LLFontGL::TOP);
	}
}

void LLFastTimerView::drawBorders( S32 y, const S32 x_start, S32 bar_height, S32 dy )
{
	// Draw borders
	{
		S32 by = y + 6 + (S32)LLFontGL::getFontMonospace()->getLineHeight();	

		//heading
		gl_rect_2d(x_start-5, by, getRect().getWidth()-5, y+5, LLColor4::grey, FALSE);

		//tree view
		gl_rect_2d(5, by, x_start-10, 5, LLColor4::grey, FALSE);

		by = y + 5;
		//average bar
		gl_rect_2d(x_start-5, by, getRect().getWidth()-5, by-bar_height-dy-5, LLColor4::grey, FALSE);

		by -= bar_height*2+dy;

		//current frame bar
		gl_rect_2d(x_start-5, by, getRect().getWidth()-5, by-bar_height-dy-2, LLColor4::grey, FALSE);

		by -= bar_height+dy+1;

		//history bars
		gl_rect_2d(x_start-5, by, getRect().getWidth()-5, LINE_GRAPH_HEIGHT-bar_height-dy-2, LLColor4::grey, FALSE);			

		by = LINE_GRAPH_HEIGHT-dy;

		//line graph
		//mGraphRect = LLRect(x_start-5, by, getRect().getWidth()-5, 5);

		gl_rect_2d(mGraphRect, FALSE);
	}
}

void LLFastTimerView::updateTotalTime()
{
	switch(mDisplayMode)
	{
	case 0:
		mTotalTimeDisplay = mRecording.getPeriodMean(FTM_FRAME, RUNNING_AVERAGE_WIDTH)*2;
		break;
	case 1:
		mTotalTimeDisplay = mRecording.getPeriodMax(FTM_FRAME);
		break;
	case 2:
		// Calculate the max total ticks for the current history
		mTotalTimeDisplay = mRecording.getPeriodMax(FTM_FRAME, 20);
		break;
	default:
		mTotalTimeDisplay = F64Milliseconds(100);
		break;
	}

	mTotalTimeDisplay = LLUnits::Milliseconds::fromValue(llceil(mTotalTimeDisplay.valueInUnits<LLUnits::Milliseconds>() / 20.f) * 20.f);
}

void LLFastTimerView::drawBars()
{
	LLLocalClipRect clip(mBarRect);

	S32 bar_height = mBarRect.getHeight() / (MAX_VISIBLE_HISTORY + 2);
	const S32 vpad = llmax(1, bar_height / 4); // spacing between bars
	bar_height -= vpad;

	updateTotalTime();
	if (mTotalTimeDisplay <= (F32Seconds)0.0) return;

	drawTicks();
	const S32 bars_top = mBarRect.mTop - ((S32)LLFontGL::getFontMonospace()->getLineHeight() + 4);
	drawBorders(bars_top, mBarRect.mLeft, bar_height, vpad);

	// Draw bars for each history entry
	// Special: 0 = show running average
	LLPointer<LLUIImage> bar_image = LLUI::getUIImage("Rounded_Square");

	const S32 image_width = bar_image->getTextureWidth();
	const S32 image_height = bar_image->getTextureHeight();

	gGL.getTexUnit(0)->bind(bar_image->getImage());
	{	
		const S32 histmax = (S32)mRecording.getNumRecordedPeriods();

		// update widths
		if (!mPauseHistory)
		{
			U32 bar_index = 0;
			if (!mAverageTimerRow.mBars)
			{
				mAverageTimerRow.mBars = new TimerBar[LLTrace::BlockTimerStatHandle::instance_tracker_t::instanceCount()];
			}
			updateTimerBarWidths(&FTM_FRAME, mAverageTimerRow, -1, bar_index);
			updateTimerBarOffsets(&FTM_FRAME, mAverageTimerRow);

			for (S32 history_index = 1; history_index <= histmax; history_index++)
			{
				llassert(history_index <= mTimerBarRows.size());
				TimerBarRow& row = mTimerBarRows[history_index - 1];
				bar_index = 0;
				if (!row.mBars)
				{
					row.mBars = new TimerBar[LLTrace::BlockTimerStatHandle::instance_tracker_t::instanceCount()];
					updateTimerBarWidths(&FTM_FRAME, row, history_index, bar_index);
					updateTimerBarOffsets(&FTM_FRAME, row);
				}
			}
		}

		// draw bars
		LLRect frame_bar_rect;
		frame_bar_rect.setLeftTopAndSize(mBarRect.mLeft, 
										bars_top, 
										ll_round((mAverageTimerRow.mBars[0].mTotalTime / mTotalTimeDisplay) * mBarRect.getWidth()), 
										bar_height);
		mAverageTimerRow.mTop = frame_bar_rect.mTop;
		mAverageTimerRow.mBottom = frame_bar_rect.mBottom;
		drawBar(frame_bar_rect, mAverageTimerRow, image_width, image_height);
		frame_bar_rect.translate(0, -(bar_height + vpad + bar_height));

		for(S32 bar_index = mScrollIndex; bar_index < llmin(histmax, mScrollIndex + MAX_VISIBLE_HISTORY); ++bar_index)
		{
			llassert(bar_index < mTimerBarRows.size());
			TimerBarRow& row = mTimerBarRows[bar_index];
			row.mTop = frame_bar_rect.mTop;
			row.mBottom = frame_bar_rect.mBottom;
			frame_bar_rect.mRight = frame_bar_rect.mLeft 
									+ ll_round((row.mBars[0].mTotalTime / mTotalTimeDisplay) * mBarRect.getWidth());
 			drawBar(frame_bar_rect, row, image_width, image_height);

			frame_bar_rect.translate(0, -(bar_height + vpad));
		}

	}	
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

static LLTrace::BlockTimerStatHandle FTM_UPDATE_TIMER_BAR_WIDTHS("Update timer bar widths");

F32Seconds LLFastTimerView::updateTimerBarWidths(LLTrace::BlockTimerStatHandle* time_block, TimerBarRow& row, S32 history_index, U32& bar_index)
{
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_TIMER_BAR_WIDTHS);
	const F32Seconds self_time = history_index == -1
										? mRecording.getPeriodMean(time_block->selfTime(), RUNNING_AVERAGE_WIDTH) 
										: mRecording.getPrevRecording(history_index).getSum(time_block->selfTime());

	F32Seconds full_time = self_time;

	// reserve a spot for this bar to be rendered before its children
	// even though we don't know its size yet
	TimerBar& timer_bar = row.mBars[bar_index];
	bar_index++;

	for (BlockTimerStatHandle::child_iter it = time_block->beginChildren(), end_it = time_block->endChildren(); it != end_it; ++it)
	{
		full_time += updateTimerBarWidths(*it, row, history_index, bar_index);
	}

	timer_bar.mTotalTime  = full_time;
	timer_bar.mSelfTime   = self_time;
	timer_bar.mTimeBlock  = time_block;
	
	return full_time;
}

static LLTrace::BlockTimerStatHandle FTM_UPDATE_TIMER_BAR_FRACTIONS("Update timer bar fractions");

S32 LLFastTimerView::updateTimerBarOffsets(LLTrace::BlockTimerStatHandle* time_block, TimerBarRow& row, S32 timer_bar_index)
{
	LL_RECORD_BLOCK_TIME(FTM_UPDATE_TIMER_BAR_FRACTIONS);

	TimerBar& timer_bar = row.mBars[timer_bar_index];
	const F32Seconds bar_time = timer_bar.mTotalTime - timer_bar.mSelfTime;
	timer_bar.mChildrenStart = timer_bar.mSelfStart + timer_bar.mSelfTime / 2;
	timer_bar.mChildrenEnd = timer_bar.mChildrenStart + timer_bar.mTotalTime - timer_bar.mSelfTime;

	if (timer_bar_index == 0)
	{
		timer_bar.mSelfStart = F32Seconds(0.f);
		timer_bar.mSelfEnd = bar_time;
	}

	//now loop through children and figure out portion of bar image covered by each bar, now that we know the
	//sum of all children
	F32 bar_fraction_start = 0.f;
	TimerBar* last_child_timer_bar = NULL;

	bool first_child = true;
	for (BlockTimerStatHandle::child_iter it = time_block->beginChildren(), end_it = time_block->endChildren(); 
		it != end_it; 
		++it)
	{
		timer_bar_index++;
		
		TimerBar& child_timer_bar = row.mBars[timer_bar_index];
		BlockTimerStatHandle* child_time_block = *it;

		if (last_child_timer_bar)
		{
			last_child_timer_bar->mLastChild = false;
		}
		child_timer_bar.mLastChild = true;
		last_child_timer_bar = &child_timer_bar;

		child_timer_bar.mFirstChild = first_child;
		if (first_child)
		{
			first_child = false;
		}

		child_timer_bar.mStartFraction = bar_fraction_start;
		child_timer_bar.mEndFraction = bar_time > (S32Seconds)0
										? bar_fraction_start + child_timer_bar.mTotalTime / bar_time
										: 1.f;
		child_timer_bar.mSelfStart = timer_bar.mChildrenStart 
									+ child_timer_bar.mStartFraction 
										* (timer_bar.mChildrenEnd - timer_bar.mChildrenStart);
		child_timer_bar.mSelfEnd =	timer_bar.mChildrenStart 
									+ child_timer_bar.mEndFraction 
										* (timer_bar.mChildrenEnd - timer_bar.mChildrenStart);

		timer_bar_index = updateTimerBarOffsets(child_time_block, row, timer_bar_index);

		bar_fraction_start = child_timer_bar.mEndFraction;
	}
	return timer_bar_index;
}

S32 LLFastTimerView::drawBar(LLRect bar_rect, TimerBarRow& row, S32 image_width, S32 image_height, bool hovered, bool visible, S32 bar_index)
{
	TimerBar& timer_bar = row.mBars[bar_index];
	LLTrace::BlockTimerStatHandle* time_block = timer_bar.mTimeBlock;

	hovered |= mHoverID == time_block;

	// animate scale of bar when hovering over that particular timer
	if (visible && (F32)bar_rect.getWidth() * (timer_bar.mEndFraction - timer_bar.mStartFraction) > 2.f)
	{
		LLRect render_rect(bar_rect);
		S32 scale_offset = 0;
		if (mHoverID == time_block)
		{
			scale_offset = llfloor(sinf(mHighlightTimer.getElapsedTimeF32() * 6.f) * 3.f);
			render_rect.mTop += scale_offset;
			render_rect.mBottom -= scale_offset;
		}

		llassert(time_block->getIndex() < sTimerColors.size());
		LLColor4 color = sTimerColors[time_block->getIndex()];
		if (!hovered) color = lerp(color, LLColor4::grey, 0.2f);
		gGL.color4fv(color.mV);
		gl_segmented_rect_2d_fragment_tex(render_rect,
			image_width, image_height, 
			16, 
			timer_bar.mStartFraction, timer_bar.mEndFraction);
	}

	LLRect children_rect;
	children_rect.mLeft  = ll_round(timer_bar.mChildrenStart / mTotalTimeDisplay * (F32)mBarRect.getWidth()) + mBarRect.mLeft;
	children_rect.mRight = ll_round(timer_bar.mChildrenEnd   / mTotalTimeDisplay * (F32)mBarRect.getWidth()) + mBarRect.mLeft;

	if (bar_rect.getHeight() > MIN_BAR_HEIGHT)
	{
		// shrink as we go down a level
		children_rect.mTop = bar_rect.mTop - 1;
		children_rect.mBottom = bar_rect.mBottom + 1;
	}
	else
	{
		children_rect.mTop = bar_rect.mTop;
		children_rect.mBottom = bar_rect.mBottom;
	}

	bool children_visible = visible && !time_block->getTreeNode().mCollapsed;

	bar_index++;
	const U32 num_bars = LLTrace::BlockTimerStatHandle::instance_tracker_t::instanceCount();
	if (bar_index < num_bars && row.mBars[bar_index].mFirstChild)
	{
		bool is_last = false;
		do
		{
			is_last = row.mBars[bar_index].mLastChild;
			bar_index = drawBar(children_rect, row, image_width, image_height, hovered, children_visible, bar_index);
		}
		while(!is_last && bar_index < num_bars);
	}

	return bar_index;
}
