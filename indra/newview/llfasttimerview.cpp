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
#include "llerror.h"
#include "llgl.h"
#include "llimagepng.h"
#include "llrender.h"
#include "llrendertarget.h"
#include "lllocalcliprect.h"
#include "llmath.h"
#include "llfontgl.h"
#include "llsdserialize.h"
#include "lltooltip.h"
#include "llbutton.h"

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

static const S32 MAX_VISIBLE_HISTORY = 10;
static const S32 LINE_GRAPH_HEIGHT = 240;
static const S32 MIN_BAR_HEIGHT = 3;

std::vector<TimeBlock*> ft_display_idx; // line of table entry for display purposes (for collapse)

typedef LLTreeDFSIter<TimeBlock, TimeBlock::child_const_iter> timer_tree_iterator_t;

BOOL LLFastTimerView::sAnalyzePerformance = FALSE;

static timer_tree_iterator_t begin_timer_tree(TimeBlock& id) 
{ 
	return timer_tree_iterator_t(&id, 
							boost::bind(boost::mem_fn(&TimeBlock::beginChildren), _1), 
							boost::bind(boost::mem_fn(&TimeBlock::endChildren), _1));
}

static timer_tree_iterator_t end_timer_tree() 
{ 
	return timer_tree_iterator_t(); 
}

S32 get_depth(const TimeBlock* blockp)
{
	S32 depth = 0;
	TimeBlock* timerp = blockp->getParent();
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
	mDisplayCenter(ALIGN_CENTER),
	mDisplayCalls(false),
	mDisplayHz(false),
	mScrollIndex(0),
	mHoverID(NULL),
	mHoverBarIndex(-1),
	mPrintStats(-1),
	mRecording(&get_frame_recording()),
	mPauseHistory(false)
{
	mTimerBars = new std::vector<TimerBar>[MAX_VISIBLE_HISTORY + 1];
}

LLFastTimerView::~LLFastTimerView()
{
	if (mRecording != &get_frame_recording())
	{
		delete mRecording;
	}
	mRecording = NULL;
	delete [] mTimerBars;
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
		if (mRecording != &get_frame_recording())
		{
			delete mRecording;
		}
		mRecording = &get_frame_recording();
		getChild<LLButton>("pause_btn")->setLabel(getString("pause"));
	}
	else
	{
		mRecording = new PeriodicRecording(get_frame_recording());
		mScrollIndex = 0;

		getChild<LLButton>("pause_btn")->setLabel(getString("run"));
	}

	mPauseHistory = pause_state;
}

BOOL LLFastTimerView::postBuild()
{
	LLButton& pause_btn = getChildRef<LLButton>("pause_btn");
	
	pause_btn.setCommitCallback(boost::bind(&LLFastTimerView::onPause, this));
	return TRUE;
}

BOOL LLFastTimerView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (mHoverTimer )
	{
		// right click collapses timers
		if (!mHoverTimer->getCollapsed())
		{
			mHoverTimer->setCollapsed(true);
		}
		else if (mHoverTimer->getParent())
		{
			mHoverTimer->getParent()->setCollapsed(true);
		}
		return TRUE;
	}
	else if (mBarRect.pointInRect(x, y))
	{
		S32 bar_idx = MAX_VISIBLE_HISTORY - ((y - mBarRect.mBottom) * (MAX_VISIBLE_HISTORY + 2) / mBarRect.getHeight());
		bar_idx = llclamp(bar_idx, 0, MAX_VISIBLE_HISTORY);
		mPrintStats = mScrollIndex + bar_idx;
		return TRUE;
	}
	return LLFloater::handleRightMouseDown(x, y, mask);
}

TimeBlock* LLFastTimerView::getLegendID(S32 y)
{
	S32 idx = (mBarRect.mTop - y) / (LLFontGL::getFontMonospace()->getLineHeight()+2) - 1;

	if (idx >= 0 && idx < (S32)ft_display_idx.size())
	{
		return ft_display_idx[idx];
	}
	
	return NULL;
}

BOOL LLFastTimerView::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	for(timer_tree_iterator_t it = begin_timer_tree(FTM_FRAME);
		it != end_timer_tree();
		++it)
	{
		(*it)->setCollapsed(false);
	}
	return TRUE;
}

BOOL LLFastTimerView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (x < mBarRect.mLeft) 
	{
		TimeBlock* idp = getLegendID(y);
		if (idp)
		{
			idp->setCollapsed(!idp->getCollapsed());
		}
	}
	else if (mHoverTimer)
	{
		//left click drills down by expanding timers
		mHoverTimer->setCollapsed(false);
	}
	else if (mask & MASK_ALT)
	{
		if (mask & MASK_CONTROL)
		{
			mDisplayHz = !mDisplayHz;	
		}
		else
		{
			mDisplayCalls = !mDisplayCalls;
		}
	}
	else if (mask & MASK_SHIFT)
	{
		if (++mDisplayMode > 3)
			mDisplayMode = 0;
	}
	else if (mask & MASK_CONTROL)
	{
		mDisplayCenter = (ChildAlignment)((mDisplayCenter + 1) % ALIGN_COUNT);
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
		mScrollIndex = llround( lerp * (F32)(mRecording->getNumPeriods() - MAX_VISIBLE_HISTORY));
		mScrollIndex = llclamp(	mScrollIndex, 0, (S32)mRecording->getNumPeriods());
		return TRUE;
	}
	mHoverTimer = NULL;
	mHoverID = NULL;

	if(mPauseHistory && mBarRect.pointInRect(x, y))
	{
		mHoverBarIndex = llmin((mBarRect.mTop - y) / (mBarRect.getHeight() / (MAX_VISIBLE_HISTORY + 2)) - 1,
								(S32)mRecording->getNumPeriods() - 1,
								MAX_VISIBLE_HISTORY);
		if (mHoverBarIndex == 0)
		{
			return TRUE;
		}
		else if (mHoverBarIndex == -1)
		{
			mHoverBarIndex = 0;
		}

		S32 i = 0;
		for(timer_tree_iterator_t it = begin_timer_tree(FTM_FRAME);
			it != end_timer_tree();
			++it, ++i)
		{
			// is mouse over bar for this timer?
			if (mTimerBars[mHoverBarIndex][i].mVisibleRect.pointInRect(x, y))
			{
				mHoverID = (*it);
				if (mHoverTimer != *it)
				{
					// could be that existing tooltip is for a parent and is thus
					// covering region for this new timer, go ahead and unblock 
					// so we can create a new tooltip
					LLToolTipMgr::instance().unblockToolTips();
					mHoverTimer = (*it);
				}

				mToolTipRect = mTimerBars[mHoverBarIndex][i].mVisibleRect;
			}

			if ((*it)->getCollapsed())
			{
				it.skipDescendants();
			}
		}
	}
	else if (x < mBarRect.mLeft) 
	{
		TimeBlock* timer_id = getLegendID(y);
		if (timer_id)
		{
			mHoverID = timer_id;
		}
	}
	
	return LLFloater::handleHover(x, y, mask);
}


static std::string get_tooltip(TimeBlock& timer, S32 history_index, PeriodicRecording& frame_recording)
{
	std::string tooltip;
	if (history_index == 0)
	{
		// by default, show average number of call
		tooltip = llformat("%s (%d ms, %d calls)", timer.getName().c_str(), (S32)LLUnit<LLUnits::Milliseconds, F64>(frame_recording.getPeriodMean(timer)).value(), (S32)frame_recording.getPeriodMean(timer.callCount()));
	}
	else
	{
		tooltip = llformat("%s (%d ms, %d calls)", timer.getName().c_str(), (S32)LLUnit<LLUnits::Milliseconds, F64>(frame_recording.getPrevRecording(history_index).getSum(timer)).value(), (S32)frame_recording.getPrevRecording(history_index).getSum(timer.callCount()));
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

			std::string tooltip = get_tooltip(*mHoverTimer, mHoverBarIndex > 0 ? mScrollIndex + mHoverBarIndex : 0, *mRecording);

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
		if (x < mBarRect.mLeft) 
		{
			TimeBlock* idp = getLegendID(y);
			if (idp)
			{
				LLToolTipMgr::instance().show(get_tooltip(*idp, 0, *mRecording));

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
							llmin((S32)mRecording->getNumPeriods(), (S32)mRecording->getNumPeriods() - MAX_VISIBLE_HISTORY));
	return TRUE;
}

static TimeBlock FTM_RENDER_TIMER("Timers", true);
static const S32 MARGIN = 10;
static const S32 LEGEND_WIDTH = 220;

static std::map<TimeBlock*, LLColor4> sTimerColors;

void LLFastTimerView::draw()
{
	LLFastTimer t(FTM_RENDER_TIMER);
	
	generateUniqueColors();

	// Draw the window background
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gl_rect_2d(getLocalRect(), LLColor4(0.f, 0.f, 0.f, 0.25f));
	
	S32 y = drawHelp(getRect().getHeight() - MARGIN);
	drawLegend(y - ((S32)LLFontGL::getFontMonospace()->getLineHeight() + 2));

	// update rectangle that includes timer bars
	const S32 LEGEND_WIDTH = 220;

	mBarRect.mLeft = MARGIN + LEGEND_WIDTH + 8;
	mBarRect.mTop = y;
	mBarRect.mRight = getRect().getWidth() - MARGIN;
	mBarRect.mBottom = MARGIN + LINE_GRAPH_HEIGHT;

	drawBars();
	drawLineGraph();
	printLineStats();
	LLView::draw();
		
	mAllTimeMax = llmax(mAllTimeMax, mRecording->getLastRecording().getSum(FTM_FRAME));
	mHoverID = NULL;
	mHoverBarIndex = -1;
}

void saveChart(const std::string& label, const char* suffix, LLImageRaw* scratch)
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
		std::ifstream is(base.c_str());
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
		std::ifstream is(target.c_str());
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
		ret[label]["TotalTime"] = time_stats[label].mSum;
		ret[label]["MeanTime"] = time_stats[label].getMean();
		ret[label]["MaxTime"] = time_stats[label].getMaxValue();
		ret[label]["MinTime"] = time_stats[label].getMinValue();
		ret[label]["StdDevTime"] = time_stats[label].getStdDev();
		
		ret[label]["Samples"] = sample_stats[label].mSum;
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
	std::ifstream base_is(baseline.c_str());
	std::ifstream target_is(target.c_str());
	if (!base_is.is_open() || !target_is.is_open())
	{
		llwarns << "'-analyzeperformance' error : baseline or current target file inexistent" << llendl;
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

	//output comparision
	std::ofstream os(output.c_str());

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
	if(TimeBlock::sLog)
	{
		doAnalysisDefault(baseline, target, output) ;
		return ;
	}

	if(TimeBlock::sMetricLog)
	{
		LLMetricPerformanceTesterBasic::doAnalysisMetrics(baseline, target, output) ;
		return ;
	}
}
void	LLFastTimerView::onClickCloseBtn()
{
	setVisible(false);
}

void LLFastTimerView::printLineStats()
{
	// Output stats for clicked bar to log
	if (mPrintStats >= 0)
	{
		std::string legend_stat;
		bool first = true;
		for(timer_tree_iterator_t it = begin_timer_tree(FTM_FRAME);
			it != end_timer_tree();
			++it)
		{
			TimeBlock* idp = (*it);

			if (!first)
			{
				legend_stat += ", ";
			}
			first = false;
			legend_stat += idp->getName();

			if (idp->getCollapsed())
			{
				it.skipDescendants();
			}
		}
		llinfos << legend_stat << llendl;

		std::string timer_stat;
		first = true;
		for(timer_tree_iterator_t it = begin_timer_tree(FTM_FRAME);
			it != end_timer_tree();
			++it)
		{
			TimeBlock* idp = (*it);

			if (!first)
			{
				timer_stat += ", ";
			}
			first = false;

			LLUnit<LLUnits::Seconds, F32> ticks;
			if (mPrintStats > 0)
			{
				ticks = mRecording->getPrevRecording(mPrintStats).getSum(*idp);
			}
			else
			{
				ticks = mRecording->getPeriodMean(*idp);
			}
			LLUnit<LLUnits::Milliseconds, F32> ms = ticks;

			timer_stat += llformat("%.1f",ms.value());

			if (idp->getCollapsed())
			{
				it.skipDescendants();
			}
		}
		llinfos << timer_stat << llendl;
		mPrintStats = -1;
	}
}

static LLFastTimer::DeclareTimer FTM_DRAW_LINE_GRAPH("Draw line graph");

void LLFastTimerView::drawLineGraph()
{
	LLFastTimer _(FTM_DRAW_LINE_GRAPH);
	//draw line graph history
	S32 x = mBarRect.mLeft;
	S32 y = LINE_GRAPH_HEIGHT;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	LLLocalClipRect clip(mGraphRect);

	//normalize based on last frame's maximum
	static LLUnit<LLUnits::Seconds, F32> max_time = 0.000001;
	static U32 max_calls = 0;
	static F32 alpha_interp = 0.f;

	//display y-axis range
	std::string axis_label;
	if (mDisplayCalls)
		axis_label = llformat("%d calls", (int)max_calls);
	else if (mDisplayHz)
		axis_label = llformat("%d Hz", (int)(1.f / max_time.value()));
	else
		axis_label = llformat("%4.2f ms", LLUnit<LLUnits::Milliseconds, F32>(max_time).value());

	x = mGraphRect.mRight - LLFontGL::getFontMonospace()->getWidth(axis_label)-5;
	y = mGraphRect.mTop - LLFontGL::getFontMonospace()->getLineHeight();

	LLFontGL::getFontMonospace()->renderUTF8(axis_label, 0, x, y, LLColor4::white,
		LLFontGL::LEFT, LLFontGL::TOP);

	//highlight visible range
	{
		S32 first_frame = mRecording->getNumPeriods() - mScrollIndex;
		S32 last_frame = first_frame - MAX_VISIBLE_HISTORY;

		F32 frame_delta = ((F32) (mGraphRect.getWidth()))/(mRecording->getNumPeriods()-1);

		F32 right = (F32) mGraphRect.mLeft + frame_delta*first_frame;
		F32 left = (F32) mGraphRect.mLeft + frame_delta*last_frame;

		gGL.color4f(0.5f,0.5f,0.5f,0.3f);
		gl_rect_2d((S32) left, mGraphRect.mTop, (S32) right, mGraphRect.mBottom);

		if (mHoverBarIndex > 0)
		{
			S32 bar_frame = first_frame - mHoverBarIndex - 1;
			F32 bar = (F32) mGraphRect.mLeft + frame_delta*bar_frame;

			gGL.color4f(0.5f,0.5f,0.5f,1);

			gGL.begin(LLRender::LINES);
			gGL.vertex2i((S32)bar, mGraphRect.mBottom);
			gGL.vertex2i((S32)bar, mGraphRect.mTop);
			gGL.end();
		}
	}

	LLUnit<LLUnits::Seconds, F32> cur_max = 0;
	U32 cur_max_calls = 0;
	for(timer_tree_iterator_t it = begin_timer_tree(FTM_FRAME);
		it != end_timer_tree();
		++it)
	{
		TimeBlock* idp = (*it);

		//fatten highlighted timer
		if (mHoverID == idp)
		{
			gGL.flush();
			glLineWidth(3);
		}

		const F32 * col = sTimerColors[idp].mV;// ft_display_table[idx].color->mV;

		F32 alpha = 1.f;

		if (mHoverID != NULL &&
			mHoverID != idp)
		{	//fade out non-highlighted timers
			if (idp->getParent() != mHoverID)
			{
				alpha = alpha_interp;
			}
		}

		gGL.color4f(col[0], col[1], col[2], alpha);				
		gGL.begin(LLRender::TRIANGLE_STRIP);
		for (U32 j = mRecording->getNumPeriods();
			j > 0;
			j--)
		{
			LLUnit<LLUnits::Seconds, F32> time = llmax(mRecording->getPrevRecording(j).getSum(*idp), LLUnit<LLUnits::Seconds, F64>(0.000001));
			U32 calls = mRecording->getPrevRecording(j).getSum(idp->callCount());

			if (alpha == 1.f)
			{ 
				//normalize to highlighted timer
				cur_max = llmax(cur_max, time);
				cur_max_calls = llmax(cur_max_calls, calls);
			}
			F32 x = mGraphRect.mRight - j * (F32)(mGraphRect.getWidth())/(mRecording->getNumPeriods()-1);
			F32 y = mDisplayHz 
				? mGraphRect.mBottom + (1.f / time.value()) * ((F32) mGraphRect.getHeight() / (1.f / max_time.value()))
				: mGraphRect.mBottom + time / max_time * (F32)mGraphRect.getHeight();
			gGL.vertex2f(x,y);
			gGL.vertex2f(x,mGraphRect.mBottom);
		}
		gGL.end();

		if (mHoverID == idp)
		{
			gGL.flush();
			glLineWidth(1);
		}

		if (idp->getCollapsed())
		{	
			//skip hidden timers
			it.skipDescendants();
		}
	}
	
	//interpolate towards new maximum
	max_time = lerp(max_time.value(), cur_max.value(), LLSmoothInterpolation::getInterpolant(0.1f));
	if (max_time - cur_max <= 1 ||  cur_max - max_time  <= 1)
	{
		max_time = llmax(LLUnit<LLUnits::Microseconds, F32>(1), LLUnit<LLUnits::Microseconds, F32>(cur_max));
	}

	max_calls = llround(lerp((F32)max_calls, (F32) cur_max_calls, LLSmoothInterpolation::getInterpolant(0.1f)));
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
		x = (mGraphRect.mRight + mGraphRect.mLeft)/2;
		y = mGraphRect.mBottom + 8;

		LLFontGL::getFontMonospace()->renderUTF8(
			mHoverID->getName(), 
			0, 
			x, y, 
			LLColor4::white,
			LLFontGL::LEFT, LLFontGL::BOTTOM);
	}					
}

void LLFastTimerView::drawLegend( S32 y )
{
	// draw legend
	S32 dx;
	S32 x = MARGIN;
	const S32 TEXT_HEIGHT = (S32)LLFontGL::getFontMonospace()->getLineHeight();

	{
		LLLocalClipRect clip(LLRect(MARGIN, y, LEGEND_WIDTH, MARGIN));
		S32 cur_line = 0;
		ft_display_idx.clear();
		std::map<TimeBlock*, S32> display_line;
		for (timer_tree_iterator_t it = begin_timer_tree(FTM_FRAME);
			it != timer_tree_iterator_t();
			++it)
		{
			TimeBlock* idp = (*it);
			display_line[idp] = cur_line;
			ft_display_idx.push_back(idp);
			cur_line++;

			x = MARGIN;

			LLRect bar_rect(x, y, x + TEXT_HEIGHT, y - TEXT_HEIGHT);
			S32 scale_offset = 0;
			if (idp == mHoverID)
			{
				scale_offset = llfloor(sinf(mHighlightTimer.getElapsedTimeF32() * 6.f) * 2.f);
			}
			bar_rect.stretch(scale_offset);
			gl_rect_2d(bar_rect, sTimerColors[idp]);

			LLUnit<LLUnits::Milliseconds, F32> ms = 0;
			S32 calls = 0;
			if (mHoverBarIndex > 0 && mHoverID)
			{
				S32 hidx = mScrollIndex + mHoverBarIndex;
				ms = mRecording->getPrevRecording(hidx).getSum(*idp);
				calls = mRecording->getPrevRecording(hidx).getSum(idp->callCount());
			}
			else
			{
				ms = LLUnit<LLUnits::Seconds, F64>(mRecording->getPeriodMean(*idp));
				calls = (S32)mRecording->getPeriodMean(idp->callCount());
			}

			std::string timer_label;
			if (mDisplayCalls)
			{
				timer_label = llformat("%s (%d)",idp->getName().c_str(),calls);
			}
			else
			{
				timer_label = llformat("%s [%.1f]",idp->getName().c_str(),ms.value());
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
				if (idp->getCollapsed() && !idp->getChildren().empty())
				{
					gl_line_2d(line_x+4, line_start_y-3, line_x+4, line_start_y+4, color);
				}
			}

			x += dx;
			BOOL is_child_of_hover_item = (idp == mHoverID);
			TimeBlock* next_parent = idp->getParent();
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

			if (idp->getCollapsed()) 
			{
				it.skipDescendants();
			}
		}
	}
}

void LLFastTimerView::generateUniqueColors()
{
	// generate unique colors
	{
		sTimerColors[&FTM_FRAME] = LLColor4::grey;

		F32 hue = 0.f;

		for (timer_tree_iterator_t it = begin_timer_tree(FTM_FRAME);
			it != timer_tree_iterator_t();
			++it)
		{
			TimeBlock* idp = (*it);

			const F32 HUE_INCREMENT = 0.23f;
			hue = fmodf(hue + HUE_INCREMENT, 1.f);
			// saturation increases with depth
			F32 saturation = clamp_rescale((F32)get_depth(idp), 0.f, 3.f, 0.f, 1.f);
			// lightness alternates with depth
			F32 lightness = get_depth(idp) % 2 ? 0.5f : 0.6f;

			LLColor4 child_color;
			child_color.setHSL(hue, saturation, lightness);

			sTimerColors[idp] = child_color;
		}
	}
}

S32 LLFastTimerView::drawHelp( S32 y )
{
	// Draw some help
	const S32 texth = (S32)LLFontGL::getFontMonospace()->getLineHeight();

	char modedesc[][32] = {
		"2 x Average ",
		"Max         ",
		"Recent Max  ",
		"100 ms      "
	};
	char centerdesc[][32] = {
		"Left      ",
		"Centered  ",
		"Ordered   "
	};

	std::string text;
	text = llformat("Full bar = %s [Click to pause/reset] [SHIFT-Click to toggle]",modedesc[mDisplayMode]);
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, MARGIN, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);

	y -= (texth + 2);
	text = llformat("Justification = %s [CTRL-Click to toggle]",centerdesc[mDisplayCenter]);
	LLFontGL::getFontMonospace()->renderUTF8(text, 0, MARGIN, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
	y -= (texth + 2);

	LLFontGL::getFontMonospace()->renderUTF8(std::string("[Right-Click log selected] [ALT-Click toggle counts] [ALT-SHIFT-Click sub hidden]"),
		0, MARGIN, y, LLColor4::white, LLFontGL::LEFT, LLFontGL::TOP);
	y -= (texth + 2);
	return y;
}

void LLFastTimerView::drawTicks()
{
	// Draw MS ticks
	{
		LLUnit<LLUnits::Milliseconds, U32> ms = mTotalTimeDisplay;
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

		by = LINE_GRAPH_HEIGHT-bar_height-dy-7;

		//line graph
		mGraphRect = LLRect(x_start-5, by, getRect().getWidth()-5, 5);

		gl_rect_2d(mGraphRect, FALSE);
	}
}

void LLFastTimerView::updateTotalTime()
{
	switch(mDisplayMode)
	{
	case 0:
		mTotalTimeDisplay = mRecording->getPeriodMean(FTM_FRAME)*2;
		break;
	case 1:
		mTotalTimeDisplay = mAllTimeMax;
		break;
	case 2:
		// Calculate the max total ticks for the current history
		mTotalTimeDisplay = mRecording->getPeriodMax(FTM_FRAME);
		break;
	default:
		mTotalTimeDisplay = LLUnit<LLUnits::Milliseconds, F32>(100);
		break;
	}

	mTotalTimeDisplay = LLUnit<LLUnits::Milliseconds, F32>(llceil(mTotalTimeDisplay.as<LLUnits::Milliseconds>().value() / 20.f) * 20.f);
}

void LLFastTimerView::drawBars()
{
	updateTotalTime();
	if (mTotalTimeDisplay <= 0.0) return;

	LLLocalClipRect clip(mBarRect);

	S32 bar_height = mBarRect.getHeight() / (MAX_VISIBLE_HISTORY + 2);
	S32 vpad = llmax(1, bar_height / 4); // spacing between bars
	bar_height -= vpad;

	drawTicks();
	S32 y = mBarRect.mTop - ((S32)LLFontGL::getFontMonospace()->getLineHeight() + 4);
	drawBorders(y, mBarRect.mLeft, bar_height, vpad);

	// Draw bars for each history entry
	// Special: -1 = show running average
	LLPointer<LLUIImage> bar_image = LLUI::getUIImage("Rounded_Square");
	gGL.getTexUnit(0)->bind(bar_image->getImage());
	const S32 histmax = llmin((S32)mRecording->getNumPeriods(), MAX_VISIBLE_HISTORY) + 1;

	for (S32 bar_index = 0; bar_index < histmax && y > LINE_GRAPH_HEIGHT; bar_index++)
	{
		S32 history_index = (bar_index > 0)
			? bar_index + mScrollIndex
			: -1;
		mTimerBars[bar_index].clear();
		mTimerBars[bar_index].reserve(LLInstanceTracker<LLTrace::TimeBlock>::instanceCount());

		updateTimerBarWidths(&FTM_FRAME, mTimerBars[bar_index], history_index, true);
		LLRect frame_bar_rect(mBarRect.mLeft, y, mBarRect.mLeft + mTimerBars[bar_index][0].mWidth, y-bar_height);
		mTimerBars[bar_index][0].mVisibleRect = frame_bar_rect;
		updateTimerBarFractions(&FTM_FRAME, 0, mTimerBars[bar_index]);
		drawBar(&FTM_FRAME, frame_bar_rect, mTimerBars[bar_index], 0, bar_image);
				
		y -= (bar_height + vpad);
		if (bar_index == 0)
			y -= bar_height;
	}
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

static LLFastTimer::DeclareTimer FTM_UPDATE_TIMER_BAR_WIDTHS("Update timer bar widths");

S32 LLFastTimerView::updateTimerBarWidths(LLTrace::TimeBlock* time_block, std::vector<TimerBar>& bars, S32 history_index, bool visible)
{
	LLFastTimer _(FTM_UPDATE_TIMER_BAR_WIDTHS);
	F32 self_time_frame_fraction = history_index == -1
		? (mRecording->getPeriodMean(time_block->selfTime()) / mTotalTimeDisplay) 
		: (mRecording->getPrevRecording(history_index).getSum(time_block->selfTime()) / mTotalTimeDisplay);

	S32 self_time_width = llround(self_time_frame_fraction * (F32)mBarRect.getWidth());
	S32 full_width = self_time_width;

	bool children_visible = visible;

	// reserve a spot for this bar to be rendered before its children
	// even though we don't know its size yet
	S32 bar_rect_index = bars.size();
	if (visible)
	{
		bars.push_back(TimerBar());
	}

	if (time_block->getCollapsed())
	{
		children_visible = false;
	}
	for (TimeBlock::child_iter it = time_block->beginChildren(), end_it = time_block->endChildren(); it != end_it; ++it)
	{
		full_width += updateTimerBarWidths(*it, bars, history_index, children_visible);
	}

	if (visible)
	{
		TimerBar& timer_bar = bars[bar_rect_index];

		timer_bar.mWidth = full_width;
		timer_bar.mSelfWidth = self_time_width;
		timer_bar.mColor = sTimerColors[time_block];

		BOOL is_child_of_hover_item = (time_block == mHoverID);
		TimeBlock* next_parent = time_block->getParent();
		while(!is_child_of_hover_item && next_parent)
		{
			is_child_of_hover_item = (mHoverID == next_parent);
			if (next_parent->getParent() == next_parent) break;
			next_parent = next_parent->getParent();
		}

		if (mHoverID != NULL 
			&& time_block != mHoverID 
			&& !is_child_of_hover_item)
		{
			timer_bar.mColor = lerp(timer_bar.mColor, LLColor4::grey, 0.8f);
		}
	}
	return full_width;
}

static LLFastTimer::DeclareTimer FTM_UPDATE_TIMER_BAR_FRACTIONS("Update timer bar fractions");

S32 LLFastTimerView::updateTimerBarFractions(LLTrace::TimeBlock* time_block, S32 timer_bar_index, std::vector<TimerBar>& bars)
{
	LLFastTimer _(FTM_UPDATE_TIMER_BAR_FRACTIONS);
	TimerBar& timer_bar = bars[timer_bar_index];
	S32 child_time_width = timer_bar.mWidth - timer_bar.mSelfWidth;
	LLRect children_rect = timer_bar.mVisibleRect;

	if (mDisplayCenter == ALIGN_CENTER)
	{
		children_rect.mLeft += timer_bar.mSelfWidth / 2;
	}
	else if (mDisplayCenter == ALIGN_RIGHT)
	{
		children_rect.mLeft += timer_bar.mSelfWidth;
	}
	children_rect.mRight = children_rect.mLeft + timer_bar.mWidth - timer_bar.mSelfWidth;

	if (children_rect.getHeight() > MIN_BAR_HEIGHT)
	{
		children_rect.mTop -= 1;
		children_rect.mBottom += 1;
	}
	timer_bar.mChildrenRect = children_rect;

	//now loop through children and figure out portion of bar image covered by each bar, now that we know the
	//sum of all children
	if (!time_block->getCollapsed())
	{
		F32 bar_fraction_start = 0.f;
		for (TimeBlock::child_iter it = time_block->beginChildren(), end_it = time_block->endChildren(); 
			it != end_it; 
			++it)
		{
			timer_bar_index++;

			TimerBar& child_timer_bar = bars[timer_bar_index];
			TimeBlock* child_time_block = *it;

			child_timer_bar.mStartFraction = bar_fraction_start;
			child_timer_bar.mEndFraction = child_time_width > 0
				? bar_fraction_start + (F32)child_timer_bar.mWidth / child_time_width
				: 1.f;
			child_timer_bar.mVisibleRect.set(children_rect.mLeft + llround(child_timer_bar.mStartFraction * children_rect.getWidth()), 
				children_rect.mTop, 
				children_rect.mLeft + llround(child_timer_bar.mEndFraction * children_rect.getWidth()), 
				children_rect.mBottom);

			timer_bar_index = updateTimerBarFractions(child_time_block, timer_bar_index, bars);

			bar_fraction_start = child_timer_bar.mEndFraction;
		}
	}
	return timer_bar_index;
}

S32 LLFastTimerView::drawBar(LLTrace::TimeBlock* time_block, LLRect bar_rect, std::vector<TimerBar>& bars, S32 bar_index, LLPointer<LLUIImage>& bar_image)
{
	TimerBar& timer_bar = bars[bar_index];

	// animate scale of bar when hovering over that particular timer
	if (bar_rect.getWidth() > 0)
	{
		LLRect render_rect(bar_rect);
		S32 scale_offset = 0;
		if (time_block == mHoverID)
		{
			scale_offset = llfloor(sinf(mHighlightTimer.getElapsedTimeF32() * 6.f) * 3.f);
			render_rect.mTop += scale_offset;
			render_rect.mBottom -= scale_offset;
		}

		gGL.color4fv(timer_bar.mColor.mV);
		gl_segmented_rect_2d_fragment_tex(render_rect,
			bar_image->getTextureWidth(), bar_image->getTextureHeight(), 
			16, 
			timer_bar.mStartFraction, timer_bar.mEndFraction);
	}

	if (!time_block->getCollapsed())
	{
		for (TimeBlock::child_iter it = time_block->beginChildren(), end_it = time_block->endChildren(); it != end_it; ++it)
		{
			++bar_index;
			bar_index = drawBar(*it, timer_bar.mChildrenRect, bars, bar_index, bar_image);
		}
	}

	return bar_index;
}
