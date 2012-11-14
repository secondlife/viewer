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

#define FAST_TIMER_ON 1

class LLMutex;

#include "lltrace.h"

#define LL_FASTTIMER_USE_RDTSC 1

namespace LLTrace
{

class Time
{
public:
	typedef Time self_t;
	typedef class BlockTimer DeclareTimer;

public:
	Time(BlockTimer& timer);
	~Time();

public:
	static bool				sLog;
	static bool				sMetricLog;
	static std::string		sLogName;
	static bool 			sPauseHistory;
	static bool 			sResetHistory;

	// call this once a frame to reset timers
	static void nextFrame();

	// dumps current cumulative frame stats to log
	// call nextFrame() to reset timers
	static void dumpCurTimes();

	// call this to reset timer hierarchy, averages, etc.
	static void reset();

	static U64 countsPerSecond();
	static S32 getLastFrameIndex() { return sLastFrameIndex; }
	static S32 getCurFrameIndex() { return sCurFrameIndex; }

	static void writeLog(std::ostream& os);

	struct CurTimerData
	{
		Time*				mCurTimer;
		BlockTimer*			mTimerData;
		U64					mChildTime;
	};
	static LLThreadLocalPointer<CurTimerData>	sCurTimerData;

private:


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

	//inline U32 Time::getCPUClockCount32()
	//{
	//	U64 time_stamp = __rdtsc();
	//	return (U32)(time_stamp >> 8);
	//}
	//
	//// return full timer value, *not* shifted by 8 bits
	//inline U64 Time::getCPUClockCount64()
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

	static U64	sClockResolution;
	static S32	sCurFrameIndex;
	static S32	sLastFrameIndex;
	static U64	sLastFrameTime;

	U64							mStartTime;
	Time::CurTimerData	mLastTimerData;
};

struct TimerAccumulator
{
	void addSamples(const TimerAccumulator& other);
	void reset(const TimerAccumulator* other);

	//
	// members
	//
	U64 						mSelfTimeCounter,
								mTotalTimeCounter;
	U32 						mCalls;
	BlockTimer*					mLastCaller;	// used to bootstrap tree construction
	U16							mActiveCount;	// number of timers with this ID active on stack
	bool						mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
};

// stores a "named" timer instance to be reused via multiple Time stack instances
class BlockTimer 
:	public TraceType<TimerAccumulator>,
	public LLInstanceTracker<BlockTimer>
{
public:
	BlockTimer(const char* name, bool open = false, BlockTimer* parent = &getRootTimer());
	~BlockTimer();

	enum { HISTORY_NUM = 300 };

	BlockTimer* getParent() const { return mParent; }
	void setParent(BlockTimer* parent);
	S32 getDepth();

	typedef std::vector<BlockTimer*>::const_iterator child_const_iter;
	child_const_iter beginChildren();
	child_const_iter endChildren();
	std::vector<BlockTimer*>& getChildren();

	void setCollapsed(bool collapsed)	{ mCollapsed = collapsed; }
	bool getCollapsed() const			{ return mCollapsed; }

	U32 getCountAverage() const { return mCountAverage; }
	U32 getCallAverage() const	{ return mCallAverage; }

	U32 getHistoricalCount(S32 history_index = 0) const;
	U32 getHistoricalCalls(S32 history_index = 0) const;

	static BlockTimer& getRootTimer();

private:
	friend class Time;

	// recursive call to gather total time from children
	static void accumulateTimings();

	// updates cumulative times and hierarchy,
	// can be called multiple times in a frame, at any point
	static void processTimes();

	static void buildHierarchy();
	static void resetFrame();
	static void reset();

	// sum of recorded self time and tree time of all children timers (might not match actual recorded time of children if topology is incomplete
	U32 						mTreeTimeCounter; 

	U32 						mCountAverage;
	U32							mCallAverage;

	U32*						mCountHistory;
	U32*						mCallHistory;

	// tree structure
	BlockTimer*					mParent;				// BlockTimer of caller(parent)
	std::vector<BlockTimer*>	mChildren;
	bool						mCollapsed;				// don't show children
	bool						mNeedsSorting;			// sort children whenever child added
};

LL_FORCE_INLINE Time::Time(BlockTimer& timer)
{
#if FAST_TIMER_ON
	mStartTime = getCPUClockCount64();

	TimerAccumulator& accumulator = timer.getPrimaryAccumulator();
	accumulator.mActiveCount++;
	accumulator.mCalls++;
	// keep current parent as long as it is active when we are
	accumulator.mMoveUpTree |= (timer.mParent->getPrimaryAccumulator().mActiveCount == 0);

	CurTimerData* cur_timer_data = Time::sCurTimerData.get();
	// store top of stack
	mLastTimerData = *cur_timer_data;
	// push new information
	cur_timer_data->mCurTimer = this;
	cur_timer_data->mTimerData = &timer;
	cur_timer_data->mChildTime = 0;
#endif
}

LL_FORCE_INLINE Time::~Time()
{
#if FAST_TIMER_ON
	U64 total_time = getCPUClockCount64() - mStartTime;
	CurTimerData* cur_timer_data = Time::sCurTimerData.get();
	TimerAccumulator& accumulator = cur_timer_data->mTimerData->getPrimaryAccumulator();
	accumulator.mSelfTimeCounter += total_time - cur_timer_data->mChildTime;
	accumulator.mTotalTimeCounter += total_time;
	accumulator.mActiveCount--;

	// store last caller to bootstrap tree creation
	// do this in the destructor in case of recursion to get topmost caller
	accumulator.mLastCaller = mLastTimerData.mTimerData;

	// we are only tracking self time, so subtract our total time delta from parents
	mLastTimerData.mChildTime += total_time;

	*sCurTimerData = mLastTimerData;
#endif
}

}

typedef LLTrace::Time LLFastTimer;

#endif // LL_LLFASTTIMER_H
