/**
 * @file llfasttimer.h
 * @brief Declaration of a fast timer.
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

#ifndef LL_FASTTIMER_H
#define LL_FASTTIMER_H

#include "llinstancetracker.h"
#include "lltrace.h"

#define FAST_TIMER_ON 1
#define LL_FASTTIMER_USE_RDTSC 1

class LLMutex;

namespace LLTrace
{

struct BlockTimerStackRecord
{
	class BlockTimer*	mActiveTimer;
	class TimeBlock*	mTimeBlock;
	U64					mChildTime;
};

class ThreadTimerStack 
:	public BlockTimerStackRecord, 
	public LLThreadLocalSingleton<ThreadTimerStack>
{
	friend class LLThreadLocalSingleton<ThreadTimerStack>;
	ThreadTimerStack() 
	{}

public:
	ThreadTimerStack& operator=(const BlockTimerStackRecord& other)
	{
		BlockTimerStackRecord::operator=(other);
		return *this;
	}
};

class BlockTimer
{
public:
	friend class TimeBlock;
	typedef BlockTimer self_t;
	typedef class TimeBlock DeclareTimer;

	BlockTimer(TimeBlock& timer);
	~BlockTimer();

	LLUnit<LLUnits::Seconds, F64> getElapsedTime();

private:

	U64						mStartTime;
	U64						mStartTotalTimeCounter;
	BlockTimerStackRecord	mParentTimerData;
};

// stores a "named" timer instance to be reused via multiple BlockTimer stack instances
class TimeBlock 
:	public TraceType<TimeBlockAccumulator>,
	public LLInstanceTracker<TimeBlock>
{
public:
	TimeBlock(const char* name, bool open = false, TimeBlock* parent = &getRootTimeBlock());

	TimeBlockTreeNode& getTreeNode() const;
	TimeBlock* getParent() const { return getTreeNode().getParent(); }
	void setParent(TimeBlock* parent) { getTreeNode().setParent(parent); }

	typedef std::vector<TimeBlock*>::iterator child_iter;
	typedef std::vector<TimeBlock*>::const_iterator child_const_iter;
	child_iter beginChildren();
	child_iter endChildren();
	std::vector<TimeBlock*>& getChildren();

	void setCollapsed(bool collapsed)	{ mCollapsed = collapsed; }
	bool getCollapsed() const			{ return mCollapsed; }

	TraceType<TimeBlockAccumulator::CallCountAspect>& callCount() 
	{ 
		return static_cast<TraceType<TimeBlockAccumulator::CallCountAspect>&>(*(TraceType<TimeBlockAccumulator>*)this);
	}

	TraceType<TimeBlockAccumulator::SelfTimeAspect>& selfTime() 
	{ 
		return static_cast<TraceType<TimeBlockAccumulator::SelfTimeAspect>&>(*(TraceType<TimeBlockAccumulator>*)this);
	}

	static TimeBlock& getRootTimeBlock();
	static void pushLog(LLSD sd);
	static void setLogLock(LLMutex* mutex);
	static void writeLog(std::ostream& os);

	// dumps current cumulative frame stats to log
	// call nextFrame() to reset timers
	static void dumpCurTimes();

	//////////////////////////////////////////////////////////////////////////////
	//
	// Important note: These implementations must be FAST!
	//


#if LL_WINDOWS
	//
	// Windows implementation of CPU clock
	//

	//
	// NOTE: put back in when we aren't using platform sdk anymore
	//
	// because MS has different signatures for these functions in winnt.h
	// need to rename them to avoid conflicts
	//#define _interlockedbittestandset _renamed_interlockedbittestandset
	//#define _interlockedbittestandreset _renamed_interlockedbittestandreset
	//#include <intrin.h>
	//#undef _interlockedbittestandset
	//#undef _interlockedbittestandreset

	//inline U32 TimeBlock::getCPUClockCount32()
	//{
	//	U64 time_stamp = __rdtsc();
	//	return (U32)(time_stamp >> 8);
	//}
	//
	//// return full timer value, *not* shifted by 8 bits
	//inline U64 TimeBlock::getCPUClockCount64()
	//{
	//	return __rdtsc();
	//}

	// shift off lower 8 bits for lower resolution but longer term timing
	// on 1Ghz machine, a 32-bit word will hold ~1000 seconds of timing
#if LL_FASTTIMER_USE_RDTSC
	static U32 getCPUClockCount32()
	{
		U32 ret_val;
		__asm
		{
			_emit   0x0f
				_emit   0x31
				shr eax,8
				shl edx,24
				or eax, edx
				mov dword ptr [ret_val], eax
		}
		return ret_val;
	}

	// return full timer value, *not* shifted by 8 bits
	static U64 getCPUClockCount64()
	{
		U64 ret_val;
		__asm
		{
			_emit   0x0f
				_emit   0x31
				mov eax,eax
				mov edx,edx
				mov dword ptr [ret_val+4], edx
				mov dword ptr [ret_val], eax
		}
		return ret_val;
	}

#else
	//U64 get_clock_count(); // in lltimer.cpp
	// These use QueryPerformanceCounter, which is arguably fine and also works on AMD architectures.
	static U32 getCPUClockCount32()
	{
		return (U32)(get_clock_count()>>8);
	}

	static U64 getCPUClockCount64()
	{
		return get_clock_count();
	}

#endif

#endif


#if (LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))
	//
	// Linux and Solaris implementation of CPU clock - non-x86.
	// This is accurate but SLOW!  Only use out of desperation.
	//
	// Try to use the MONOTONIC clock if available, this is a constant time counter
	// with nanosecond resolution (but not necessarily accuracy) and attempts are
	// made to synchronize this value between cores at kernel start. It should not
	// be affected by CPU frequency. If not available use the REALTIME clock, but
	// this may be affected by NTP adjustments or other user activity affecting
	// the system time.
	static U64 getCPUClockCount64()
	{
		struct timespec tp;

#ifdef CLOCK_MONOTONIC // MONOTONIC supported at build-time?
		if (-1 == clock_gettime(CLOCK_MONOTONIC,&tp)) // if MONOTONIC isn't supported at runtime then ouch, try REALTIME
#endif
			clock_gettime(CLOCK_REALTIME,&tp);

		return (tp.tv_sec*sClockResolution)+tp.tv_nsec;        
	}

	static U32 getCPUClockCount32()
	{
		return (U32)(getCPUClockCount64() >> 8);
	}

#endif // (LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))


#if (LL_LINUX || LL_SOLARIS || LL_DARWIN) && (defined(__i386__) || defined(__amd64__))
	//
	// Mac+Linux+Solaris FAST x86 implementation of CPU clock
	static U32 getCPUClockCount32()
	{
		U64 x;
		__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
		return (U32)(x >> 8);
	}

	static U64 getCPUClockCount64()
	{
		U64 x;
		__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
		return x;
	}

#endif

	static U64 countsPerSecond();

	// updates cumulative times and hierarchy,
	// can be called multiple times in a frame, at any point
	static void processTimes();

	// call this once a frame to periodically log timers
	static void logStats();

	bool						mCollapsed;				// don't show children

	// statics
	static std::string							sLogName;
	static bool									sMetricLog,
												sLog;	
	static U64									sClockResolution;
};

LL_FORCE_INLINE BlockTimer::BlockTimer(TimeBlock& timer)
{
#if FAST_TIMER_ON
	mStartTime = TimeBlock::getCPUClockCount64();

	BlockTimerStackRecord* cur_timer_data = ThreadTimerStack::getIfExists();
	TimeBlockAccumulator* accumulator = timer.getPrimaryAccumulator();
	accumulator->mActiveCount++;
	mStartTotalTimeCounter = accumulator->mTotalTimeCounter;
	// keep current parent as long as it is active when we are
	accumulator->mMoveUpTree |= (accumulator->mParent->getPrimaryAccumulator()->mActiveCount == 0);

	// store top of stack
	mParentTimerData = *cur_timer_data;
	// push new information
	cur_timer_data->mActiveTimer = this;
	cur_timer_data->mTimeBlock = &timer;
	cur_timer_data->mChildTime = 0;
#endif
}

LL_FORCE_INLINE BlockTimer::~BlockTimer()
{
#if FAST_TIMER_ON
	U64 total_time = TimeBlock::getCPUClockCount64() - mStartTime;
	BlockTimerStackRecord* cur_timer_data = ThreadTimerStack::getIfExists();
	TimeBlockAccumulator* accumulator = cur_timer_data->mTimeBlock->getPrimaryAccumulator();

	accumulator->mCalls++;
	accumulator->mTotalTimeCounter += total_time - (accumulator->mTotalTimeCounter - mStartTotalTimeCounter);
	accumulator->mSelfTimeCounter += total_time - cur_timer_data->mChildTime;
	accumulator->mActiveCount--;

	// store last caller to bootstrap tree creation
	// do this in the destructor in case of recursion to get topmost caller
	accumulator->mLastCaller = mParentTimerData.mTimeBlock;

	// we are only tracking self time, so subtract our total time delta from parents
	mParentTimerData.mChildTime += total_time;

	//pop stack
	*cur_timer_data = mParentTimerData;
#endif
}

}

typedef LLTrace::BlockTimer LLFastTimer; 

#endif // LL_LLFASTTIMER_H
