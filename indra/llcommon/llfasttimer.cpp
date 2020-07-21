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
#include "llunits.h"
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

bool        BlockTimer::sLog		     = false;
std::string BlockTimer::sLogName         = "";
bool        BlockTimer::sMetricLog       = false;

#if LL_LINUX || LL_SOLARIS
U64         BlockTimer::sClockResolution = 1000000000; // Nanosecond resolution
#else
U64         BlockTimer::sClockResolution = 1000000; // Microsecond resolution
#endif

static LLMutex*			sLogLock = NULL;
static std::queue<LLSD> sLogQueue;

block_timer_tree_df_iterator_t begin_block_timer_tree_df(BlockTimerStatHandle& id) 
{ 
	return block_timer_tree_df_iterator_t(&id, 
		boost::bind(boost::mem_fn(&BlockTimerStatHandle::beginChildren), _1), 
		boost::bind(boost::mem_fn(&BlockTimerStatHandle::endChildren), _1));
}

block_timer_tree_df_iterator_t end_block_timer_tree_df() 
{ 
	return block_timer_tree_df_iterator_t(); 
}

block_timer_tree_df_post_iterator_t begin_block_timer_tree_df_post(BlockTimerStatHandle& id) 
{ 
	return block_timer_tree_df_post_iterator_t(&id, 
							boost::bind(boost::mem_fn(&BlockTimerStatHandle::beginChildren), _1), 
							boost::bind(boost::mem_fn(&BlockTimerStatHandle::endChildren), _1));
}

block_timer_tree_df_post_iterator_t end_block_timer_tree_df_post() 
{ 
	return block_timer_tree_df_post_iterator_t(); 
}

block_timer_tree_bf_iterator_t begin_block_timer_tree_bf(BlockTimerStatHandle& id)
{ 
	return block_timer_tree_bf_iterator_t(&id, 
		boost::bind(boost::mem_fn(&BlockTimerStatHandle::beginChildren), _1), 
		boost::bind(boost::mem_fn(&BlockTimerStatHandle::endChildren), _1));
}

block_timer_tree_bf_iterator_t end_block_timer_tree_bf()
	{
	return block_timer_tree_bf_iterator_t(); 
	}

block_timer_tree_df_iterator_t begin_timer_tree(BlockTimerStatHandle& id) 
	{
	return block_timer_tree_df_iterator_t(&id, 
		boost::bind(boost::mem_fn(&BlockTimerStatHandle::beginChildren), _1), 
							boost::bind(boost::mem_fn(&BlockTimerStatHandle::endChildren), _1));
	}

block_timer_tree_df_iterator_t end_timer_tree() 
	{
	return block_timer_tree_df_iterator_t(); 
}


// sort child timers by name
struct SortTimerByName
	{
	bool operator()(const BlockTimerStatHandle* i1, const BlockTimerStatHandle* i2)
		{
		return i1->getName() < i2->getName();
		}
};

static BlockTimerStatHandle sRootTimer("root", NULL);
BlockTimerStatHandle& BlockTimer::getRootTimeBlock()
{
	return sRootTimer;
	}

void BlockTimer::pushLog(LLSD log)
{
	LLMutexLock lock(sLogLock);

	sLogQueue.push(log);
}

void BlockTimer::setLogLock(LLMutex* lock)
{
	sLogLock = lock;
}


//static
#if (LL_DARWIN || LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))
U64 BlockTimer::countsPerSecond()
{
	return sClockResolution;
}
#else // windows or x86-mac or x86-linux or x86-solaris
U64 BlockTimer::countsPerSecond()
{
#if LL_FASTTIMER_USE_RDTSC || !LL_WINDOWS
	//getCPUFrequency returns MHz and sCPUClockFrequency wants to be in Hz
	static LLUnit<U64, LLUnits::Hertz> sCPUClockFrequency = LLProcessorInfo().getCPUFrequency();
	return sCPUClockFrequency.value();
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
	return sCPUClockFrequency.value();
#endif
}
#endif

BlockTimerStatHandle::BlockTimerStatHandle(const char* name, const char* description)
:	StatType<TimeBlockAccumulator>(name, description)
{}

TimeBlockTreeNode& BlockTimerStatHandle::getTreeNode() const
{
	TimeBlockTreeNode* nodep = LLTrace::get_thread_recorder()->getTimeBlockTreeNode(getIndex());
	llassert(nodep);
	return *nodep;
}


void BlockTimer::bootstrapTimerTree()
{
	for (auto& base : BlockTimerStatHandle::instance_snapshot())
	{
		// because of indirect derivation from LLInstanceTracker, have to downcast
		BlockTimerStatHandle& timer = static_cast<BlockTimerStatHandle&>(base);
		if (&timer == &BlockTimer::getRootTimeBlock()) continue;

		// bootstrap tree construction by attaching to last timer to be on stack
		// when this timer was called
		if (timer.getParent() == &BlockTimer::getRootTimeBlock())
		{
			TimeBlockAccumulator& accumulator = timer.getCurrentAccumulator();

			if (accumulator.mLastCaller)
			{
				timer.setParent(accumulator.mLastCaller);
				accumulator.mParent = accumulator.mLastCaller;
			}
			// no need to push up tree on first use, flag can be set spuriously
			accumulator.mMoveUpTree = false;
		}
	}
}

// bump timers up tree if they have been flagged as being in the wrong place
// do this in a bottom up order to promote descendants first before promoting ancestors
// this preserves partial order derived from current frame's observations
void BlockTimer::incrementalUpdateTimerTree()
{
	for(block_timer_tree_df_post_iterator_t it = begin_block_timer_tree_df_post(BlockTimer::getRootTimeBlock());
		it != end_block_timer_tree_df_post();
		++it)
	{
		BlockTimerStatHandle* timerp = *it;

		// sort timers by time last called, so call graph makes sense
		TimeBlockTreeNode& tree_node = timerp->getTreeNode();
		if (tree_node.mNeedsSorting)
{
			std::sort(tree_node.mChildren.begin(), tree_node.mChildren.end(), SortTimerByName());
}

		// skip root timer
		if (timerp != &BlockTimer::getRootTimeBlock())
{
			TimeBlockAccumulator& accumulator = timerp->getCurrentAccumulator();

			if (accumulator.mMoveUpTree)
{
				// since ancestors have already been visited, re-parenting won't affect tree traversal
			//step up tree, bringing our descendants with us
			LL_DEBUGS("FastTimers") << "Moving " << timerp->getName() << " from child of " << timerp->getParent()->getName() <<
				" to child of " << timerp->getParent()->getParent()->getName() << LL_ENDL;
			timerp->setParent(timerp->getParent()->getParent());
				accumulator.mParent = timerp->getParent();
				accumulator.mMoveUpTree = false;

			// don't bubble up any ancestors until descendants are done bubbling up
				// as ancestors may call this timer only on certain paths, so we want to resolve
				// child-most block locations before their parents
			it.skipAncestors();
		}
	}
	}
}


void BlockTimer::updateTimes()
	{
	// walk up stack of active timers and accumulate current time while leaving timing structures active
	BlockTimerStackRecord* stack_record	= LLThreadLocalSingletonPointer<BlockTimerStackRecord>::getInstance();
	if (!stack_record) return;

	U64 cur_time = getCPUClockCount64();
	BlockTimer* cur_timer				= stack_record->mActiveTimer;
	TimeBlockAccumulator* accumulator	= &stack_record->mTimeBlock->getCurrentAccumulator();

	while(cur_timer 
		&& cur_timer->mParentTimerData.mActiveTimer != cur_timer) // root defined by parent pointing to self
		{
		U64 cumulative_time_delta = cur_time - cur_timer->mStartTime;
		cur_timer->mStartTime = cur_time;

		accumulator->mTotalTimeCounter += cumulative_time_delta;
		accumulator->mSelfTimeCounter += cumulative_time_delta - stack_record->mChildTime;
		stack_record->mChildTime = 0;

		stack_record = &cur_timer->mParentTimerData;
		accumulator  = &stack_record->mTimeBlock->getCurrentAccumulator();
		cur_timer    = stack_record->mActiveTimer;

		stack_record->mChildTime += cumulative_time_delta;
	}
}

static LLTrace::BlockTimerStatHandle FTM_PROCESS_TIMES("Process FastTimer Times");

// not thread safe, so only call on main thread
//static
void BlockTimer::processTimes()
{
#if LL_TRACE_ENABLED
	LL_RECORD_BLOCK_TIME(FTM_PROCESS_TIMES);
	get_clock_count(); // good place to calculate clock frequency

	// set up initial tree
	bootstrapTimerTree();

	incrementalUpdateTimerTree();

	updateTimes();

	// reset for next frame
	for (auto& base : BlockTimerStatHandle::instance_snapshot())
	{
		// because of indirect derivation from LLInstanceTracker, have to downcast
		BlockTimerStatHandle& timer = static_cast<BlockTimerStatHandle&>(base);
		TimeBlockAccumulator& accumulator = timer.getCurrentAccumulator();

		accumulator.mLastCaller = NULL;
		accumulator.mMoveUpTree = false;
	}
#endif
}

std::vector<BlockTimerStatHandle*>::iterator BlockTimerStatHandle::beginChildren()
		{
	return getTreeNode().mChildren.begin(); 
		}

std::vector<BlockTimerStatHandle*>::iterator BlockTimerStatHandle::endChildren()
		{
	return getTreeNode().mChildren.end();
}

std::vector<BlockTimerStatHandle*>& BlockTimerStatHandle::getChildren()
{
	return getTreeNode().mChildren;
	}

bool BlockTimerStatHandle::hasChildren()
{
	return ! getTreeNode().mChildren.empty();
}

// static
void BlockTimer::logStats()
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
			LL_DEBUGS("FastTimers") << "elapsed sec " << ((F64)getCPUClockCount64() / (F64HertzImplicit)LLProcessorInfo().getCPUFrequency()) << LL_ENDL;
		}
		call_count++;

		F64Seconds total_time(0);
		LLSD sd;

		{
			for (auto& base : BlockTimerStatHandle::instance_snapshot())
			{
				// because of indirect derivation from LLInstanceTracker, have to downcast
				BlockTimerStatHandle& timer = static_cast<BlockTimerStatHandle&>(base);
				LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();
				sd[timer.getName()]["Time"] = (LLSD::Real) (frame_recording.getLastRecording().getSum(timer).value());	
				sd[timer.getName()]["Calls"] = (LLSD::Integer) (frame_recording.getLastRecording().getSum(timer.callCount()));
				
				// computing total time here because getting the root timer's getCountHistory
				// doesn't work correctly on the first frame
				total_time += frame_recording.getLastRecording().getSum(timer);
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
void BlockTimer::dumpCurTimes()
{
	LLTrace::PeriodicRecording& frame_recording = LLTrace::get_frame_recording();
	LLTrace::Recording& last_frame_recording = frame_recording.getLastRecording();

	// walk over timers in depth order and output timings
	for(block_timer_tree_df_iterator_t it = begin_timer_tree(BlockTimer::getRootTimeBlock());
		it != end_timer_tree();
		++it)
	{
		BlockTimerStatHandle* timerp = (*it);
		F64Seconds total_time = last_frame_recording.getSum(*timerp);
		U32 num_calls = last_frame_recording.getSum(timerp->callCount());

		// Don't bother with really brief times, keep output concise
		if (total_time < F32Milliseconds(0.1f)) continue;

		std::ostringstream out_str;
		BlockTimerStatHandle* parent_timerp = timerp;
		while(parent_timerp && parent_timerp != parent_timerp->getParent())
		{
			out_str << "\t";
			parent_timerp = parent_timerp->getParent();
		}

		out_str << timerp->getName() << " " 
			<< std::setprecision(3) << total_time.valueInUnits<LLUnits::Milliseconds>() << " ms, "
			<< num_calls << " calls";

		LL_INFOS() << out_str.str() << LL_ENDL;
}
}

//static
void BlockTimer::writeLog(std::ostream& os)
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
	mCalls(0),
	mLastCaller(NULL),
	mActiveCount(0),
	mMoveUpTree(false),
	mParent(NULL)
{}

void TimeBlockAccumulator::addSamples( const TimeBlockAccumulator& other, EBufferAppendType append_type )
{
#if LL_TRACE_ENABLED
	// we can't merge two unrelated time block samples, as that will screw with the nested timings
	// due to the call hierarchy of each thread
	llassert(append_type == SEQUENTIAL);
	mTotalTimeCounter += other.mTotalTimeCounter;
	mSelfTimeCounter += other.mSelfTimeCounter;
	mCalls += other.mCalls;
	mLastCaller = other.mLastCaller;
	mActiveCount = other.mActiveCount;
	mMoveUpTree = other.mMoveUpTree;
	mParent = other.mParent;
#endif
}

void TimeBlockAccumulator::reset( const TimeBlockAccumulator* other )
{
	mCalls = 0;
	mSelfTimeCounter = 0;
	mTotalTimeCounter = 0;

	if (other)
{
		mLastCaller = other->mLastCaller;
		mActiveCount = other->mActiveCount;
		mMoveUpTree = other->mMoveUpTree;
		mParent = other->mParent;
	}
}

F64Seconds BlockTimer::getElapsedTime()
{
	U64 total_time = getCPUClockCount64() - mStartTime;

	return F64Seconds((F64)total_time / (F64)BlockTimer::countsPerSecond());
}


} // namespace LLTrace
