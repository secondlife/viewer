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
#include "lltracethreadrecorder.h"

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

bool        TimeBlock::sLog		     = false;
std::string TimeBlock::sLogName         = "";
bool        TimeBlock::sMetricLog       = false;

#if LL_LINUX || LL_SOLARIS
U64         TimeBlock::sClockResolution = 1000000000; // Nanosecond resolution
#else
U64         TimeBlock::sClockResolution = 1000000; // Microsecond resolution
#endif

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

TimeBlock& TimeBlock::getRootTimeBlock()
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
U64 TimeBlock::countsPerSecond()
{
	return sClockResolution;
}
#else // windows or x86-mac or x86-linux or x86-solaris
U64 TimeBlock::countsPerSecond()
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
:	TraceType<TimeBlockAccumulator>(name),
	mCollapsed(true)
{
	setCollapsed(!open);
}

TimeBlockTreeNode& TimeBlock::getTreeNode() const
{
	TimeBlockTreeNode* nodep = LLTrace::get_thread_recorder()->getTimeBlockTreeNode(getIndex());
	llassert(nodep);
	return *nodep;
}

// static
void TimeBlock::processTimes()
{
	get_clock_count(); // good place to calculate clock frequency
	U64 cur_time = getCPUClockCount64();

	// set up initial tree
	for (LLInstanceTracker<TimeBlock>::instance_iter it = LLInstanceTracker<TimeBlock>::beginInstances(), end_it = LLInstanceTracker<TimeBlock>::endInstances(); 
		it != end_it; 
		++it)
	{
		TimeBlock& timer = *it;
		if (&timer == &TimeBlock::getRootTimeBlock()) continue;

		// bootstrap tree construction by attaching to last timer to be on stack
		// when this timer was called
		if (timer.getParent() == &TimeBlock::getRootTimeBlock())
		{
			TimeBlockAccumulator* accumulator = timer.getPrimaryAccumulator();

			if (accumulator->mLastCaller)
			{
				timer.setParent(accumulator->mLastCaller);
				accumulator->mParent = accumulator->mLastCaller;
			}
			// no need to push up tree on first use, flag can be set spuriously
			accumulator->mMoveUpTree = false;
		}
	}

	// bump timers up tree if they have been flagged as being in the wrong place
	// do this in a bottom up order to promote descendants first before promoting ancestors
	// this preserves partial order derived from current frame's observations
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(TimeBlock::getRootTimeBlock());
		it != end_timer_tree_bottom_up();
		++it)
	{
		TimeBlock* timerp = *it;

		// sort timers by time last called, so call graph makes sense
		TimeBlockTreeNode& tree_node = timerp->getTreeNode();
		if (tree_node.mNeedsSorting)
		{
			std::sort(tree_node.mChildren.begin(), tree_node.mChildren.end(), SortTimerByName());
		}

		// skip root timer
		if (timerp != &TimeBlock::getRootTimeBlock())
		{
			TimeBlockAccumulator* accumulator = timerp->getPrimaryAccumulator();

			if (accumulator->mMoveUpTree)
			{
				// since ancestors have already been visited, re-parenting won't affect tree traversal
				//step up tree, bringing our descendants with us
				LL_DEBUGS("FastTimers") << "Moving " << timerp->getName() << " from child of " << timerp->getParent()->getName() <<
					" to child of " << timerp->getParent()->getParent()->getName() << LL_ENDL;
				timerp->setParent(timerp->getParent()->getParent());
				accumulator->mParent = timerp->getParent();
				accumulator->mMoveUpTree = false;

				// don't bubble up any ancestors until descendants are done bubbling up
				// as ancestors may call this timer only on certain paths, so we want to resolve
				// child-most block locations before their parents
				it.skipAncestors();
			}
		}
	}

	// walk up stack of active timers and accumulate current time while leaving timing structures active
	BlockTimerStackRecord* stack_record			= ThreadTimerStack::getInstance();
	BlockTimer* cur_timer						= stack_record->mActiveTimer;
	TimeBlockAccumulator* accumulator = stack_record->mTimeBlock->getPrimaryAccumulator();
	
	// root defined by parent pointing to self
	while(cur_timer && cur_timer->mParentTimerData.mActiveTimer != cur_timer)
	{
		U64 cumulative_time_delta = cur_time - cur_timer->mStartTime;
		accumulator->mTotalTimeCounter += cumulative_time_delta - (accumulator->mTotalTimeCounter - cur_timer->mBlockStartTotalTimeCounter);
		accumulator->mSelfTimeCounter += cumulative_time_delta - stack_record->mChildTime;
		stack_record->mChildTime = 0;

		cur_timer->mStartTime = cur_time;
		cur_timer->mBlockStartTotalTimeCounter = accumulator->mTotalTimeCounter;

		stack_record = &cur_timer->mParentTimerData;
		accumulator = stack_record->mTimeBlock->getPrimaryAccumulator();
		cur_timer = stack_record->mActiveTimer;

		stack_record->mChildTime += cumulative_time_delta;
	}

	// reset for next frame
	for (LLInstanceTracker<TimeBlock>::instance_iter it = LLInstanceTracker<TimeBlock>::beginInstances(),
			end_it = LLInstanceTracker<TimeBlock>::endInstances();
		it != end_it;
		++it)
	{
		TimeBlock& timer = *it;
		TimeBlockAccumulator* accumulator = timer.getPrimaryAccumulator();

		accumulator->mLastCaller = NULL;
		accumulator->mMoveUpTree = false;
	}
}


std::vector<TimeBlock*>::iterator TimeBlock::beginChildren()
{ 
	return getTreeNode().mChildren.begin(); 
}

std::vector<TimeBlock*>::iterator TimeBlock::endChildren()
{
	return getTreeNode().mChildren.end();
}

std::vector<TimeBlock*>& TimeBlock::getChildren()
{
	return getTreeNode().mChildren;
}

//static
void TimeBlock::logStats()
{
	// get ready for next frame
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

}

//static
void TimeBlock::dumpCurTimes()
{
	LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();
	LLTrace::Recording& last_frame_recording = frame_recording.getLastRecordingPeriod();

	// walk over timers in depth order and output timings
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(TimeBlock::getRootTimeBlock());
		it != end_timer_tree();
		++it)
	{
		TimeBlock* timerp = (*it);
		LLUnit<LLUnits::Seconds, F64> total_time_ms = last_frame_recording.getSum(*timerp);
		U32 num_calls = last_frame_recording.getSum(timerp->callCount());

		// Don't bother with really brief times, keep output concise
		if (total_time_ms < 0.1) continue;

		std::ostringstream out_str;
		TimeBlock* parent_timerp = timerp;
		while(parent_timerp && parent_timerp != parent_timerp->getParent())
		{
			out_str << "\t";
			parent_timerp = parent_timerp->getParent();
		}

		out_str << timerp->getName() << " " 
			<< std::setprecision(3) << total_time_ms.as<LLUnits::Milliseconds>().value() << " ms, "
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
:	mTotalTimeCounter(0),
	mSelfTimeCounter(0),
	mStartTotalTimeCounter(0),
	mCalls(0),
	mLastCaller(NULL),
	mActiveCount(0),
	mMoveUpTree(false),
	mParent(NULL)
{}

void TimeBlockAccumulator::addSamples( const TimeBlockAccumulator& other )
{
	mTotalTimeCounter += other.mTotalTimeCounter - other.mStartTotalTimeCounter;
	mSelfTimeCounter += other.mSelfTimeCounter;
	mCalls += other.mCalls;
	mLastCaller = other.mLastCaller;
	mActiveCount = other.mActiveCount;
	mMoveUpTree = other.mMoveUpTree;
	mParent = other.mParent;
}

void TimeBlockAccumulator::reset( const TimeBlockAccumulator* other )
{
	mCalls = 0;
	mSelfTimeCounter = 0;

	if (other)
	{
		mStartTotalTimeCounter = other->mTotalTimeCounter;
		mTotalTimeCounter = mStartTotalTimeCounter;

		mLastCaller = other->mLastCaller;
		mActiveCount = other->mActiveCount;
		mMoveUpTree = other->mMoveUpTree;
		mParent = other->mParent;
	}
	else
	{
		mStartTotalTimeCounter = mTotalTimeCounter;
	}
}

LLUnit<LLUnits::Seconds, F64> BlockTimer::getElapsedTime()
{
	U64 total_time = TimeBlock::getCPUClockCount64() - mStartTime;

	return (F64)total_time / (F64)TimeBlock::countsPerSecond();
}


} // namespace LLTrace
