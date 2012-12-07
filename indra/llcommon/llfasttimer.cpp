/** 
 * @file llfasttimer.cpp
 * @brief Implementation of the fast timer.
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
#include "linden_common.h"

#include "llfasttimer.h"

#include "llmemory.h"
#include "llprocessor.h"
#include "llsingleton.h"
#include "lltreeiterators.h"
#include "llsdserialize.h"
#include "llunit.h"
#include "llsd.h"
#include "lltracerecording.h"

#include <boost/bind.hpp>
#include <queue>


#if LL_WINDOWS
#include "lltimer.h"
#elif LL_LINUX || LL_SOLARIS
#include <sys/time.h>
#include <sched.h>
#include "lltimer.h"
#elif LL_DARWIN
#include <sys/time.h>
#include "lltimer.h"	// get_clock_count()
#else 
#error "architecture not supported"
#endif

namespace LLTrace
{

//////////////////////////////////////////////////////////////////////////////
// statics

S32         TimeBlock::sCurFrameIndex   = -1;
S32         TimeBlock::sLastFrameIndex  = -1;
U64         TimeBlock::sLastFrameTime   = TimeBlock::getCPUClockCount64();
bool        TimeBlock::sPauseHistory    = 0;
bool        TimeBlock::sResetHistory    = 0;
bool        TimeBlock::sLog		     = false;
std::string TimeBlock::sLogName         = "";
bool        TimeBlock::sMetricLog       = false;

#if LL_LINUX || LL_SOLARIS
U64         TimeBlock::sClockResolution = 1000000000; // Nanosecond resolution
#else
U64         TimeBlock::sClockResolution = 1000000; // Microsecond resolution
#endif

LLThreadLocalPointer<CurTimerData> TimeBlock::sCurTimerData;

static LLMutex*			sLogLock = NULL;
static std::queue<LLSD> sLogQueue;


// FIXME: move these declarations to the relevant modules

// helper functions
typedef LLTreeDFSPostIter<TimeBlock, TimeBlock::child_const_iter> timer_tree_bottom_up_iterator_t;

static timer_tree_bottom_up_iterator_t begin_timer_tree_bottom_up(TimeBlock& id) 
{ 
	return timer_tree_bottom_up_iterator_t(&id, 
							boost::bind(boost::mem_fn(&TimeBlock::beginChildren), _1), 
							boost::bind(boost::mem_fn(&TimeBlock::endChildren), _1));
}

static timer_tree_bottom_up_iterator_t end_timer_tree_bottom_up() 
{ 
	return timer_tree_bottom_up_iterator_t(); 
}

typedef LLTreeDFSIter<TimeBlock, TimeBlock::child_const_iter> timer_tree_dfs_iterator_t;


static timer_tree_dfs_iterator_t begin_timer_tree(TimeBlock& id) 
{ 
	return timer_tree_dfs_iterator_t(&id, 
		boost::bind(boost::mem_fn(&TimeBlock::beginChildren), _1), 
							boost::bind(boost::mem_fn(&TimeBlock::endChildren), _1));
}

static timer_tree_dfs_iterator_t end_timer_tree() 
{ 
	return timer_tree_dfs_iterator_t(); 
}


// sort child timers by name
struct SortTimerByName
{
	bool operator()(const TimeBlock* i1, const TimeBlock* i2)
	{
		return i1->getName() < i2->getName();
	}
};

TimeBlock& TimeBlock::getRootTimer()
{
	static TimeBlock root_timer("root", true, NULL);
	return root_timer;
}

void TimeBlock::pushLog(LLSD log)
{
	LLMutexLock lock(sLogLock);

	sLogQueue.push(log);
}

void TimeBlock::setLogLock(LLMutex* lock)
{
	sLogLock = lock;
}


//static
#if (LL_DARWIN || LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))
U64 TimeBlock::countsPerSecond() // counts per second for the *64-bit* timer
{
	return sClockResolution;
}
#else // windows or x86-mac or x86-linux or x86-solaris
U64 TimeBlock::countsPerSecond() // counts per second for the *64-bit* timer
{
#if LL_FASTTIMER_USE_RDTSC || !LL_WINDOWS
	//getCPUFrequency returns MHz and sCPUClockFrequency wants to be in Hz
	static LLUnit<LLUnits::Hertz, U64> sCPUClockFrequency = LLProcessorInfo().getCPUFrequency();

#else
	// If we're not using RDTSC, each fast timer tick is just a performance counter tick.
	// Not redefining the clock frequency itself (in llprocessor.cpp/calculate_cpu_frequency())
	// since that would change displayed MHz stats for CPUs
	static bool firstcall = true;
	static U64 sCPUClockFrequency;
	if (firstcall)
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&sCPUClockFrequency);
		firstcall = false;
	}
#endif
	return sCPUClockFrequency.value();
}
#endif

TimeBlock::TimeBlock(const char* name, bool open, TimeBlock* parent)
:	TraceType(name),
	mCollapsed(true),
	mParent(NULL),
	mNeedsSorting(false)
{
	setCollapsed(!open);

	if (parent)
	{
		setParent(parent);
	}
	else
	{
		mParent = this;
	}
}

void TimeBlock::setParent(TimeBlock* parent)
{
	llassert_always(parent != this);
	llassert_always(parent != NULL);

	if (mParent)
	{
		//// subtract our accumulated from previous parent
		//for (S32 i = 0; i < HISTORY_NUM; i++)
		//{
		//	mParent->mCountHistory[i] -= mCountHistory[i];
		//}

		//// subtract average timing from previous parent
		//mParent->mCountAverage -= mCountAverage;

		std::vector<TimeBlock*>& children = mParent->getChildren();
		std::vector<TimeBlock*>::iterator found_it = std::find(children.begin(), children.end(), this);
		if (found_it != children.end())
		{
			children.erase(found_it);
		}
	}

	mParent = parent;
	if (parent)
	{
		parent->getChildren().push_back(this);
		parent->mNeedsSorting = true;
	}
}

S32 TimeBlock::getDepth()
{
	S32 depth = 0;
	TimeBlock* timerp = mParent;
	while(timerp)
	{
		depth++;
		if (timerp->getParent() == timerp) break;
		timerp = timerp->mParent;
	}
	return depth;
}

// static
void TimeBlock::processTimes()
{
	//void TimeBlock::buildHierarchy()
	{
		// set up initial tree
		{
			for (LLInstanceTracker<TimeBlock>::instance_iter it = LLInstanceTracker<TimeBlock>::beginInstances(), end_it = LLInstanceTracker<TimeBlock>::endInstances(); it != end_it; ++it)
			{
				TimeBlock& timer = *it;
				if (&timer == &TimeBlock::getRootTimer()) continue;

				// bootstrap tree construction by attaching to last timer to be on stack
				// when this timer was called
				if (timer.mParent == &TimeBlock::getRootTimer())
				{
					TimeBlockTreeNode& tree_node = sCurTimerData->mTimerTreeData[timer.getIndex()];

					if (tree_node.mLastCaller)
					{
						timer.setParent(tree_node.mLastCaller);
					}
					// no need to push up tree on first use, flag can be set spuriously
					tree_node.mMoveUpTree = false;
				}
			}
		}

		// bump timers up tree if they have been flagged as being in the wrong place
		// do this in a bottom up order to promote descendants first before promoting ancestors
		// this preserves partial order derived from current frame's observations
		for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(TimeBlock::getRootTimer());
			it != end_timer_tree_bottom_up();
			++it)
		{
			TimeBlock* timerp = *it;
			// skip root timer
			if (timerp == &TimeBlock::getRootTimer()) continue;
			TimeBlockTreeNode& tree_node = sCurTimerData->mTimerTreeData[timerp->getIndex()];

			if (tree_node.mMoveUpTree)
			{
				// since ancestors have already been visited, re-parenting won't affect tree traversal
				//step up tree, bringing our descendants with us
				LL_DEBUGS("FastTimers") << "Moving " << timerp->getName() << " from child of " << timerp->getParent()->getName() <<
					" to child of " << timerp->getParent()->getParent()->getName() << LL_ENDL;
				timerp->setParent(timerp->getParent()->getParent());
				tree_node.mMoveUpTree = false;

				// don't bubble up any ancestors until descendants are done bubbling up
				it.skipAncestors();
			}
		}

		// sort timers by time last called, so call graph makes sense
		for(timer_tree_dfs_iterator_t it = begin_timer_tree(TimeBlock::getRootTimer());
			it != end_timer_tree();
			++it)
		{
			TimeBlock* timerp = (*it);
			if (timerp->mNeedsSorting)
			{
				std::sort(timerp->getChildren().begin(), timerp->getChildren().end(), SortTimerByName());
			}
			timerp->mNeedsSorting = false;
		}
	}
	
	//void TimeBlock::accumulateTimings()
	{
		U64 cur_time = getCPUClockCount64();

		// root defined by parent pointing to self
		CurTimerData* cur_data = sCurTimerData.get();
		// walk up stack of active timers and accumulate current time while leaving timing structures active
		BlockTimer* cur_timer = cur_data->mCurTimer;
		TimeBlockAccumulator& accumulator = cur_data->mTimerData->getPrimaryAccumulator();
		while(cur_timer && cur_timer->mLastTimerData.mCurTimer != cur_timer)
		{
			U64 cumulative_time_delta = cur_time - cur_timer->mStartTime;
			U64 self_time_delta = cumulative_time_delta - cur_data->mChildTime;
			cur_data->mChildTime = 0;
			accumulator.mSelfTimeCounter += self_time_delta;
			accumulator.mTotalTimeCounter += cumulative_time_delta;

			cur_timer->mStartTime = cur_time;

			cur_data = &cur_timer->mLastTimerData;
			cur_data->mChildTime += cumulative_time_delta;
			if (cur_data->mTimerData)
			{
				accumulator = cur_data->mTimerData->getPrimaryAccumulator();
			}

			cur_timer = cur_timer->mLastTimerData.mCurTimer;
		}

		// traverse tree in DFS post order, or bottom up
		//for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(TimeBlock::getRootTimer());
		//	it != end_timer_tree_bottom_up();
		//	++it)
		//{
		//	TimeBlock* timerp = (*it);
		//	TimeBlockAccumulator& accumulator = timerp->getPrimaryAccumulator();
		//	timerp->mTreeTimeCounter = accumulator.mSelfTimeCounter;
		//	for (child_const_iter child_it = timerp->beginChildren(); child_it != timerp->endChildren(); ++child_it)
		//	{
		//		timerp->mTreeTimeCounter += (*child_it)->mTreeTimeCounter;
		//	}

		//S32 cur_frame = getCurFrameIndex();
		//if (cur_frame >= 0)
		//{
		//	// update timer history

		//	int hidx = getCurFrameIndex() % HISTORY_NUM;

		//	timerp->mCountHistory[hidx] = timerp->mTreeTimeCounter;
		//	timerp->mCountAverage       = ((U64)timerp->mCountAverage * cur_frame + timerp->mTreeTimeCounter) / (cur_frame+1);
		//	timerp->mCallHistory[hidx]  = accumulator.mCalls;
		//	timerp->mCallAverage        = ((U64)timerp->mCallAverage * cur_frame + accumulator.mCalls) / (cur_frame+1);
		//}
		//}
	}
}


std::vector<TimeBlock*>::const_iterator TimeBlock::beginChildren()
{ 
	return mChildren.begin(); 
}

std::vector<TimeBlock*>::const_iterator TimeBlock::endChildren()
{
	return mChildren.end();
}

std::vector<TimeBlock*>& TimeBlock::getChildren()
{
	return mChildren;
}

//static
void TimeBlock::nextFrame()
{
	get_clock_count(); // good place to calculate clock frequency
	U64 frame_time = TimeBlock::getCPUClockCount64();
	if ((frame_time - sLastFrameTime) >> 8 > 0xffffffff)
	{
		llinfos << "Slow frame, fast timers inaccurate" << llendl;
	}

	if (!sPauseHistory)
	{
		TimeBlock::processTimes();
		sLastFrameIndex = sCurFrameIndex++;
	}
	
	// get ready for next frame
	//void TimeBlock::resetFrame()
	{
		if (sLog)
		{ //output current frame counts to performance log

			static S32 call_count = 0;
			if (call_count % 100 == 0)
			{
				LL_DEBUGS("FastTimers") << "countsPerSecond: " << countsPerSecond() << LL_ENDL;
				LL_DEBUGS("FastTimers") << "LLProcessorInfo().getCPUFrequency() " << LLProcessorInfo().getCPUFrequency() << LL_ENDL;
				LL_DEBUGS("FastTimers") << "getCPUClockCount32() " << getCPUClockCount32() << LL_ENDL;
				LL_DEBUGS("FastTimers") << "getCPUClockCount64() " << getCPUClockCount64() << LL_ENDL;
				LL_DEBUGS("FastTimers") << "elapsed sec " << ((F64)getCPUClockCount64()) / (LLUnit<LLUnits::Hertz, F64>(LLProcessorInfo().getCPUFrequency())) << LL_ENDL;
			}
			call_count++;

			LLUnit<LLUnits::Seconds, F64> total_time(0);
			LLSD sd;

			{
				for (LLInstanceTracker<TimeBlock>::instance_iter it = LLInstanceTracker<TimeBlock>::beginInstances(), 
					end_it = LLInstanceTracker<TimeBlock>::endInstances(); 
					it != end_it; 
				++it)
				{
					TimeBlock& timer = *it;
					LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();
					sd[timer.getName()]["Time"] = (LLSD::Real) (frame_recording.getLastRecordingPeriod().getSum(timer).value());	
					sd[timer.getName()]["Calls"] = (LLSD::Integer) (frame_recording.getLastRecordingPeriod().getSum(timer.callCount()));

					// computing total time here because getting the root timer's getCountHistory
					// doesn't work correctly on the first frame
					total_time += frame_recording.getLastRecordingPeriod().getSum(timer);
				}
			}

			sd["Total"]["Time"] = (LLSD::Real) total_time.value();
			sd["Total"]["Calls"] = (LLSD::Integer) 1;

			{		
				LLMutexLock lock(sLogLock);
				sLogQueue.push(sd);
			}
		}

		// reset for next frame
		for (LLInstanceTracker<TimeBlock>::instance_iter it = LLInstanceTracker<TimeBlock>::beginInstances(),
			end_it = LLInstanceTracker<TimeBlock>::endInstances();
			it != end_it;
		++it)
		{
			TimeBlock& timer = *it;
			TimeBlockTreeNode& tree_node = sCurTimerData->mTimerTreeData[timer.getIndex()];

			tree_node.mLastCaller = NULL;
			tree_node.mMoveUpTree = false;
		}
	}
	sLastFrameTime = frame_time;
}

//static
void TimeBlock::dumpCurTimes()
{
	// accumulate timings, etc.
	processTimes();
	
	// walk over timers in depth order and output timings
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(TimeBlock::getRootTimer());
		it != end_timer_tree();
		++it)
	{
		LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();
		TimeBlock* timerp = (*it);
		LLUnit<LLUnits::Seconds, F64> total_time_ms = frame_recording.getLastRecordingPeriod().getSum(*timerp);
		U32 num_calls = frame_recording.getLastRecordingPeriod().getSum(timerp->callCount());

		// Don't bother with really brief times, keep output concise
		if (total_time_ms < 0.1) continue;

		std::ostringstream out_str;
		for (S32 i = 0; i < timerp->getDepth(); i++)
		{
			out_str << "\t";
		}

		out_str << timerp->getName() << " " 
			<< std::setprecision(3) << total_time_ms.as<LLUnits::Milliseconds, F32>().value() << " ms, "
			<< num_calls << " calls";

		llinfos << out_str.str() << llendl;
	}
}

//static
void TimeBlock::writeLog(std::ostream& os)
{
	while (!sLogQueue.empty())
	{
		LLSD& sd = sLogQueue.front();
		LLSDSerialize::toXML(sd, os);
		LLMutexLock lock(sLogLock);
		sLogQueue.pop();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TimeBlockAccumulator
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TimeBlockAccumulator::TimeBlockAccumulator() 
:	mSelfTimeCounter(0),
	mTotalTimeCounter(0),
	mCalls(0)
{}

void TimeBlockAccumulator::addSamples( const TimeBlockAccumulator& other )
{
	mSelfTimeCounter += other.mSelfTimeCounter;
	mTotalTimeCounter += other.mTotalTimeCounter;
	mCalls += other.mCalls;
}

void TimeBlockAccumulator::reset( const TimeBlockAccumulator* other )
{
	mTotalTimeCounter = 0;
	mSelfTimeCounter = 0;
	mCalls = 0;
}

TimeBlockTreeNode::TimeBlockTreeNode()
:	mLastCaller(NULL),
	mActiveCount(0),
	mMoveUpTree(false)
{}

} // namespace LLTrace
