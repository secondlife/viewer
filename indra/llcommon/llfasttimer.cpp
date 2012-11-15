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

S32 BlockTimer::sCurFrameIndex = -1;
S32 BlockTimer::sLastFrameIndex = -1;
U64 BlockTimer::sLastFrameTime = BlockTimer::getCPUClockCount64();
bool BlockTimer::sPauseHistory = 0;
bool BlockTimer::sResetHistory = 0;
LLThreadLocalPointer<CurTimerData> BlockTimer::sCurTimerData;
bool BlockTimer::sLog = false;
std::string BlockTimer::sLogName = "";
bool BlockTimer::sMetricLog = false;
static LLMutex*			sLogLock = NULL;
static std::queue<LLSD> sLogQueue;

#if LL_LINUX || LL_SOLARIS
U64 BlockTimer::sClockResolution = 1000000000; // Nanosecond resolution
#else
U64 BlockTimer::sClockResolution = 1000000; // Microsecond resolution
#endif

// FIXME: move these declarations to the relevant modules

// helper functions
typedef LLTreeDFSPostIter<BlockTimer, BlockTimer::child_const_iter> timer_tree_bottom_up_iterator_t;

static timer_tree_bottom_up_iterator_t begin_timer_tree_bottom_up(BlockTimer& id) 
{ 
	return timer_tree_bottom_up_iterator_t(&id, 
							boost::bind(boost::mem_fn(&BlockTimer::beginChildren), _1), 
							boost::bind(boost::mem_fn(&BlockTimer::endChildren), _1));
}

static timer_tree_bottom_up_iterator_t end_timer_tree_bottom_up() 
{ 
	return timer_tree_bottom_up_iterator_t(); 
}

typedef LLTreeDFSIter<BlockTimer, BlockTimer::child_const_iter> timer_tree_dfs_iterator_t;


static timer_tree_dfs_iterator_t begin_timer_tree(BlockTimer& id) 
{ 
	return timer_tree_dfs_iterator_t(&id, 
		boost::bind(boost::mem_fn(&BlockTimer::beginChildren), _1), 
							boost::bind(boost::mem_fn(&BlockTimer::endChildren), _1));
}

static timer_tree_dfs_iterator_t end_timer_tree() 
{ 
	return timer_tree_dfs_iterator_t(); 
}

BlockTimer& BlockTimer::getRootTimer()
{
	static BlockTimer root_timer("root", true, NULL);
	return root_timer;
}

void BlockTimer::pushLog(LLSD log)
{
	LLMutexLock lock(sLogLock);

	sLogQueue.push(log);
}


//static
#if (LL_DARWIN || LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))
U64 BlockTimer::countsPerSecond() // counts per second for the *32-bit* timer
{
	return sClockResolution >> 8;
}
#else // windows or x86-mac or x86-linux or x86-solaris
U64 BlockTimer::countsPerSecond() // counts per second for the *32-bit* timer
{
#if LL_FASTTIMER_USE_RDTSC || !LL_WINDOWS
	//getCPUFrequency returns MHz and sCPUClockFrequency wants to be in Hz
	static LLUnit<LLUnits::Hertz, U64> sCPUClockFrequency = LLProcessorInfo().getCPUFrequency();

	// we drop the low-order byte in our timers, so report a lower frequency
#else
	// If we're not using RDTSC, each fasttimer tick is just a performance counter tick.
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
	return sCPUClockFrequency >> 8;
}
#endif

BlockTimer::BlockTimer(const char* name, bool open, BlockTimer* parent)
:	TraceType(name),
	mCollapsed(true),
	mParent(NULL),
	mTreeTimeCounter(0),
	mCountAverage(0),
	mCallAverage(0),
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

	mCountHistory = new U32[HISTORY_NUM];
	memset(mCountHistory, 0, sizeof(U32) * HISTORY_NUM);
	mCallHistory = new U32[HISTORY_NUM];
	memset(mCallHistory, 0, sizeof(U32) * HISTORY_NUM);
}

BlockTimer::~BlockTimer()
{
	delete[] mCountHistory;
	delete[] mCallHistory;
}

void BlockTimer::setParent(BlockTimer* parent)
{
	llassert_always(parent != this);
	llassert_always(parent != NULL);

	if (mParent)
	{
		// subtract our accumulated from previous parent
		for (S32 i = 0; i < HISTORY_NUM; i++)
		{
			mParent->mCountHistory[i] -= mCountHistory[i];
		}

		// subtract average timing from previous parent
		mParent->mCountAverage -= mCountAverage;

		std::vector<BlockTimer*>& children = mParent->getChildren();
		std::vector<BlockTimer*>::iterator found_it = std::find(children.begin(), children.end(), this);
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

S32 BlockTimer::getDepth()
{
	S32 depth = 0;
	BlockTimer* timerp = mParent;
	while(timerp)
	{
		depth++;
		if (timerp->getParent() == timerp) break;
		timerp = timerp->mParent;
	}
	return depth;
}

// static
void BlockTimer::processTimes()
{
	if (getCurFrameIndex() < 0) return;

	buildHierarchy();
	accumulateTimings();
}

// sort child timers by name
struct SortTimerByName
{
	bool operator()(const BlockTimer* i1, const BlockTimer* i2)
	{
		return i1->getName() < i2->getName();
	}
};

//static
void BlockTimer::buildHierarchy()
{
	if (getCurFrameIndex() < 0 ) return;

	// set up initial tree
	{
		for (LLInstanceTracker<BlockTimer>::instance_iter it = LLInstanceTracker<BlockTimer>::beginInstances(), end_it = LLInstanceTracker<BlockTimer>::endInstances(); it != end_it; ++it)
		{
			BlockTimer& timer = *it;
			if (&timer == &BlockTimer::getRootTimer()) continue;
			
			// bootstrap tree construction by attaching to last timer to be on stack
			// when this timer was called
			if (timer.getPrimaryAccumulator().mLastCaller && timer.mParent == &BlockTimer::getRootTimer())
			{
				timer.setParent(timer.getPrimaryAccumulator().mLastCaller);
				// no need to push up tree on first use, flag can be set spuriously
				timer.getPrimaryAccumulator().mMoveUpTree = false;
			}
		}
	}

	// bump timers up tree if they've been flagged as being in the wrong place
	// do this in a bottom up order to promote descendants first before promoting ancestors
	// this preserves partial order derived from current frame's observations
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(BlockTimer::getRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		BlockTimer* timerp = *it;
		// skip root timer
		if (timerp == &BlockTimer::getRootTimer()) continue;

		if (timerp->getPrimaryAccumulator().mMoveUpTree)
		{
			// since ancestors have already been visited, re-parenting won't affect tree traversal
			//step up tree, bringing our descendants with us
			LL_DEBUGS("FastTimers") << "Moving " << timerp->getName() << " from child of " << timerp->getParent()->getName() <<
				" to child of " << timerp->getParent()->getParent()->getName() << LL_ENDL;
			timerp->setParent(timerp->getParent()->getParent());
			timerp->getPrimaryAccumulator().mMoveUpTree = false;

			// don't bubble up any ancestors until descendants are done bubbling up
			it.skipAncestors();
		}
	}

	// sort timers by time last called, so call graph makes sense
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(BlockTimer::getRootTimer());
		it != end_timer_tree();
		++it)
	{
		BlockTimer* timerp = (*it);
		if (timerp->mNeedsSorting)
		{
			std::sort(timerp->getChildren().begin(), timerp->getChildren().end(), SortTimerByName());
		}
		timerp->mNeedsSorting = false;
	}
}

//static
void BlockTimer::accumulateTimings()
{
	U32 cur_time = getCPUClockCount32();

	// walk up stack of active timers and accumulate current time while leaving timing structures active
	Time* cur_timer = sCurTimerData->mCurTimer;
	// root defined by parent pointing to self
	CurTimerData* cur_data = sCurTimerData.get();
	TimerAccumulator& accumulator = sCurTimerData->mTimerData->getPrimaryAccumulator();
	while(cur_timer && cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		U32 cumulative_time_delta = cur_time - cur_timer->mStartTime;
		U32 self_time_delta = cumulative_time_delta - cur_data->mChildTime;
		cur_data->mChildTime = 0;
		accumulator.mSelfTimeCounter += self_time_delta;
		accumulator.mTotalTimeCounter += cumulative_time_delta;

		cur_timer->mStartTime = cur_time;

		cur_data = &cur_timer->mLastTimerData;
		cur_data->mChildTime += cumulative_time_delta;
		accumulator = cur_data->mTimerData->getPrimaryAccumulator();

		cur_timer = cur_timer->mLastTimerData.mCurTimer;
	}

	// traverse tree in DFS post order, or bottom up
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(BlockTimer::getRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		BlockTimer* timerp = (*it);
		TimerAccumulator& accumulator = timerp->getPrimaryAccumulator();
		timerp->mTreeTimeCounter = accumulator.mSelfTimeCounter;
		for (child_const_iter child_it = timerp->beginChildren(); child_it != timerp->endChildren(); ++child_it)
		{
			timerp->mTreeTimeCounter += (*child_it)->mTreeTimeCounter;
		}

		S32 cur_frame = getCurFrameIndex();
		if (cur_frame >= 0)
		{
			// update timer history
			int hidx = cur_frame % HISTORY_NUM;

			timerp->mCountHistory[hidx] = timerp->mTreeTimeCounter;
			timerp->mCountAverage       = ((U64)timerp->mCountAverage * cur_frame + timerp->mTreeTimeCounter) / (cur_frame+1);
			timerp->mCallHistory[hidx]  = accumulator.mCalls;
			timerp->mCallAverage        = ((U64)timerp->mCallAverage * cur_frame + accumulator.mCalls) / (cur_frame+1);
		}
	}
}

// static
void BlockTimer::resetFrame()
{
	if (sLog)
	{ //output current frame counts to performance log

		static S32 call_count = 0;
		if (call_count % 100 == 0)
		{
			LL_DEBUGS("FastTimers") << "countsPerSecond (32 bit): " << countsPerSecond() << LL_ENDL;
			LL_DEBUGS("FastTimers") << "get_clock_count (64 bit): " << get_clock_count() << LL_ENDL;
			LL_DEBUGS("FastTimers") << "LLProcessorInfo().getCPUFrequency() " << LLProcessorInfo().getCPUFrequency() << LL_ENDL;
			LL_DEBUGS("FastTimers") << "getCPUClockCount32() " << getCPUClockCount32() << LL_ENDL;
			LL_DEBUGS("FastTimers") << "getCPUClockCount64() " << getCPUClockCount64() << LL_ENDL;
			LL_DEBUGS("FastTimers") << "elapsed sec " << ((F64)getCPUClockCount64()) / (LLUnit<LLUnits::Hertz, F64>(LLProcessorInfo().getCPUFrequency())) << LL_ENDL;
		}
		call_count++;

		F64 iclock_freq = 1000.0 / countsPerSecond(); // good place to calculate clock frequency

		F64 total_time = 0;
		LLSD sd;

		{
			for (LLInstanceTracker<BlockTimer>::instance_iter it = LLInstanceTracker<BlockTimer>::beginInstances(), 
					end_it = LLInstanceTracker<BlockTimer>::endInstances(); 
				it != end_it; 
				++it)
			{
				BlockTimer& timer = *it;
				TimerAccumulator& accumulator = timer.getPrimaryAccumulator();
				sd[timer.getName()]["Time"] = (LLSD::Real) (accumulator.mSelfTimeCounter*iclock_freq);	
				sd[timer.getName()]["Calls"] = (LLSD::Integer) accumulator.mCalls;
				
				// computing total time here because getting the root timer's getCountHistory
				// doesn't work correctly on the first frame
				total_time = total_time + accumulator.mSelfTimeCounter * iclock_freq;
			}
		}

		sd["Total"]["Time"] = (LLSD::Real) total_time;
		sd["Total"]["Calls"] = (LLSD::Integer) 1;

		{		
			LLMutexLock lock(sLogLock);
			sLogQueue.push(sd);
		}
	}

	// reset for next frame
	for (LLInstanceTracker<BlockTimer>::instance_iter it = LLInstanceTracker<BlockTimer>::beginInstances(),
			end_it = LLInstanceTracker<BlockTimer>::endInstances();
		it != end_it;
		++it)
	{
		BlockTimer& timer = *it;
		TimerAccumulator& accumulator = timer.getPrimaryAccumulator();
		accumulator.mSelfTimeCounter = 0;
		accumulator.mCalls = 0;
		accumulator.mLastCaller = NULL;
		accumulator.mMoveUpTree = false;
	}
}

//static
void BlockTimer::reset()
{
	resetFrame(); // reset frame data

	// walk up stack of active timers and reset start times to current time
	// effectively zeroing out any accumulated time
	U32 cur_time = getCPUClockCount32();

	// root defined by parent pointing to self
	CurTimerData* cur_data = sCurTimerData.get();
	Time* cur_timer = cur_data->mCurTimer;
	while(cur_timer && cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		cur_timer->mStartTime = cur_time;
		cur_data->mChildTime = 0;

		cur_data = &cur_timer->mLastTimerData;
		cur_timer = cur_data->mCurTimer;
	}

	// reset all history
	{
		for (LLInstanceTracker<BlockTimer>::instance_iter it = LLInstanceTracker<BlockTimer>::beginInstances(), 
				end_it = LLInstanceTracker<BlockTimer>::endInstances(); 
			it != end_it; 
			++it)
		{
			BlockTimer& timer = *it;
			if (&timer != &BlockTimer::getRootTimer()) 
			{
				timer.setParent(&BlockTimer::getRootTimer());
			}
			
			timer.mCountAverage = 0;
			timer.mCallAverage = 0;
			memset(timer.mCountHistory, 0, sizeof(U32) * HISTORY_NUM);
			memset(timer.mCallHistory, 0, sizeof(U32) * HISTORY_NUM);
		}
	}

	sLastFrameIndex = 0;
	sCurFrameIndex = 0;
}

U32 BlockTimer::getHistoricalCount(S32 history_index) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % HISTORY_NUM;
	return mCountHistory[history_idx];
}

U32 BlockTimer::getHistoricalCalls(S32 history_index ) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % HISTORY_NUM;
	return mCallHistory[history_idx];
}

std::vector<BlockTimer*>::const_iterator BlockTimer::beginChildren()
{ 
	return mChildren.begin(); 
}

std::vector<BlockTimer*>::const_iterator BlockTimer::endChildren()
{
	return mChildren.end();
}

std::vector<BlockTimer*>& BlockTimer::getChildren()
{
	return mChildren;
}

//static
void BlockTimer::nextFrame()
{
	BlockTimer::countsPerSecond(); // good place to calculate clock frequency
	U64 frame_time = BlockTimer::getCPUClockCount64();
	if ((frame_time - sLastFrameTime) >> 8 > 0xffffffff)
	{
		llinfos << "Slow frame, fast timers inaccurate" << llendl;
	}

	if (!sPauseHistory)
	{
		BlockTimer::processTimes();
		sLastFrameIndex = sCurFrameIndex++;
	}
	
	// get ready for next frame
	BlockTimer::resetFrame();
	sLastFrameTime = frame_time;
}

//static
void Time::dumpCurTimes()
{
	// accumulate timings, etc.
	BlockTimer::processTimes();
	
	F64 clock_freq = (F64)BlockTimer::countsPerSecond();
	F64 iclock_freq = 1000.0 / clock_freq; // clock_ticks -> milliseconds

	// walk over timers in depth order and output timings
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(BlockTimer::getRootTimer());
		it != end_timer_tree();
		++it)
	{
		BlockTimer* timerp = (*it);
		F64 total_time_ms = ((F64)timerp->getHistoricalCount(0) * iclock_freq);
		// Don't bother with really brief times, keep output concise
		if (total_time_ms < 0.1) continue;

		std::ostringstream out_str;
		for (S32 i = 0; i < timerp->getDepth(); i++)
		{
			out_str << "\t";
		}


		out_str << timerp->getName() << " " 
			<< std::setprecision(3) << total_time_ms << " ms, "
			<< timerp->getHistoricalCalls(0) << " calls";

		llinfos << out_str.str() << llendl;
	}
}

//static
void Time::writeLog(std::ostream& os)
{
	while (!sLogQueue.empty())
	{
		LLSD& sd = sLogQueue.front();
		LLSDSerialize::toXML(sd, os);
		LLMutexLock lock(sLogLock);
		sLogQueue.pop();
	}
}


void LLTrace::TimerAccumulator::addSamples( const LLTrace::TimerAccumulator& other )
{
	mSelfTimeCounter += other.mSelfTimeCounter;
	mTotalTimeCounter += other.mTotalTimeCounter;
	mCalls += other.mCalls;
	if (!mLastCaller)
	{
		mLastCaller = other.mLastCaller;
	}

	//mActiveCount stays the same;
	mMoveUpTree |= other.mMoveUpTree;
}

void LLTrace::TimerAccumulator::reset( const LLTrace::TimerAccumulator* other )
{
	mTotalTimeCounter = 0;
	mSelfTimeCounter = 0;
	mCalls = 0;
}
}
