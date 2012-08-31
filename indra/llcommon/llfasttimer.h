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
#define DEBUG_FAST_TIMER_THREADS 1

class LLMutex;

#include <queue>
#include "llsd.h"

#define LL_FASTTIMER_USE_RDTSC 1


LL_COMMON_API void assert_main_thread();

class LL_COMMON_API LLFastTimer
{
public:
	class NamedTimer;

	struct LL_COMMON_API FrameState
	{
		FrameState();
		void setNamedTimer(NamedTimer* timerp) { mTimer = timerp; }

		U32 				mSelfTimeCounter;
		U32 				mCalls;
		FrameState*			mParent;		// info for caller timer
		FrameState*			mLastCaller;	// used to bootstrap tree construction
		NamedTimer*			mTimer;
		U16					mActiveCount;	// number of timers with this ID active on stack
		bool				mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
	};

	// stores a "named" timer instance to be reused via multiple LLFastTimer stack instances
	class LL_COMMON_API NamedTimer
	:	public LLInstanceTracker<NamedTimer>
	{
		friend class DeclareTimer;
	public:
		~NamedTimer();

		enum { HISTORY_NUM = 300 };

		const std::string& getName() const { return mName; }
		NamedTimer* getParent() const { return mParent; }
		void setParent(NamedTimer* parent);
		S32 getDepth();
		std::string getToolTip(S32 history_index = -1);

		typedef std::vector<NamedTimer*>::const_iterator child_const_iter;
		child_const_iter beginChildren();
		child_const_iter endChildren();
		std::vector<NamedTimer*>& getChildren();

		void setCollapsed(bool collapsed) { mCollapsed = collapsed; }
		bool getCollapsed() const { return mCollapsed; }

		U32 getCountAverage() const { return mCountAverage; }
		U32 getCallAverage() const { return mCallAverage; }

		U32 getHistoricalCount(S32 history_index = 0) const;
		U32 getHistoricalCalls(S32 history_index = 0) const;

		void setFrameState(FrameState* state) { mFrameState = state; state->setNamedTimer(this); }
		FrameState& getFrameState() const;

	private:
		friend class LLFastTimer;
		friend class NamedTimerFactory;

		//
		// methods
		//
		NamedTimer(const std::string& name);
		// recursive call to gather total time from children
		static void accumulateTimings();

		// updates cumulative times and hierarchy,
		// can be called multiple times in a frame, at any point
		static void processTimes();

		static void buildHierarchy();
		static void resetFrame();
		static void reset();

		//
		// members
		//
		FrameState*		mFrameState;

		std::string	mName;

		U32 		mTotalTimeCounter;

		U32 		mCountAverage;
		U32			mCallAverage;

		U32*		mCountHistory;
		U32*		mCallHistory;

		// tree structure
		NamedTimer*					mParent;				// NamedTimer of caller(parent)
		std::vector<NamedTimer*>	mChildren;
		bool						mCollapsed;				// don't show children
		bool						mNeedsSorting;			// sort children whenever child added
	};

	// used to statically declare a new named timer
	class LL_COMMON_API DeclareTimer
	:	public LLInstanceTracker<DeclareTimer>
	{
		friend class LLFastTimer;
	public:
		DeclareTimer(const std::string& name, bool open);
		DeclareTimer(const std::string& name);

		NamedTimer& getNamedTimer() { return mTimer; }

	private:
		FrameState		mFrameState;
		NamedTimer&		mTimer;
	};

public:
	LLFastTimer(LLFastTimer::FrameState* state);

	LL_FORCE_INLINE LLFastTimer(LLFastTimer::DeclareTimer& timer)
	:	mFrameState(&timer.mFrameState)
	{
#if FAST_TIMER_ON
		LLFastTimer::FrameState* frame_state = mFrameState;
		mStartTime = getCPUClockCount32();

		frame_state->mActiveCount++;
		frame_state->mCalls++;
		// keep current parent as long as it is active when we are
		frame_state->mMoveUpTree |= (frame_state->mParent->mActiveCount == 0);

		LLFastTimer::CurTimerData* cur_timer_data = &LLFastTimer::sCurTimerData;
		mLastTimerData = *cur_timer_data;
		cur_timer_data->mCurTimer = this;
		cur_timer_data->mFrameState = frame_state;
		cur_timer_data->mChildTime = 0;
#endif
#if DEBUG_FAST_TIMER_THREADS
#if !LL_RELEASE
		assert_main_thread();
#endif
#endif
	}

	LL_FORCE_INLINE ~LLFastTimer()
	{
#if FAST_TIMER_ON
		LLFastTimer::FrameState* frame_state = mFrameState;
		U32 total_time = getCPUClockCount32() - mStartTime;

		frame_state->mSelfTimeCounter += total_time - LLFastTimer::sCurTimerData.mChildTime;
		frame_state->mActiveCount--;

		// store last caller to bootstrap tree creation
		// do this in the destructor in case of recursion to get topmost caller
		frame_state->mLastCaller = mLastTimerData.mFrameState;

		// we are only tracking self time, so subtract our total time delta from parents
		mLastTimerData.mChildTime += total_time;

		LLFastTimer::sCurTimerData = mLastTimerData;
#endif
	}

public:
	static LLMutex*			sLogLock;
	static std::queue<LLSD> sLogQueue;
	static BOOL				sLog;
	static BOOL				sMetricLog;
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
	static const NamedTimer* getTimerByName(const std::string& name);

	struct CurTimerData
	{
		LLFastTimer*	mCurTimer;
		FrameState*		mFrameState;
		U32				mChildTime;
	};
	static CurTimerData		sCurTimerData;

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

	//inline U32 LLFastTimer::getCPUClockCount32()
	//{
	//	U64 time_stamp = __rdtsc();
	//	return (U32)(time_stamp >> 8);
	//}
	//
	//// return full timer value, *not* shifted by 8 bits
	//inline U64 LLFastTimer::getCPUClockCount64()
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
	//LL_COMMON_API U64 get_clock_count(); // in lltimer.cpp
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

	static U64 sClockResolution;

	static S32				sCurFrameIndex;
	static S32				sLastFrameIndex;
	static U64				sLastFrameTime;

	U32							mStartTime;
	LLFastTimer::FrameState*	mFrameState;
	LLFastTimer::CurTimerData	mLastTimerData;

};

typedef class LLFastTimer LLFastTimer;

#endif // LL_LLFASTTIMER_H
