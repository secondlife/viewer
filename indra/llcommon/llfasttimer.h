/** 
 * @file llfasttimer.h
 * @brief Declaration of a fast timer.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_FASTTIMER_H
#define LL_FASTTIMER_H

#include "llinstancetracker.h"

#define FAST_TIMER_ON 1
#define TIME_FAST_TIMERS 0

#if LL_WINDOWS
// because MS has different signatures for these functions in winnt.h
// need to rename them to avoid conflicts
#define _interlockedbittestandset _renamed_interlockedbittestandset
#define _interlockedbittestandreset _renamed_interlockedbittestandreset
#include <intrin.h>
#undef _interlockedbittestandset
#undef _interlockedbittestandreset

#define LL_INLINE __forceinline
// shift off lower 8 bits for lower resolution but longer term timing
// on 1Ghz machine, a 32-bit word will hold ~1000 seconds of timing
inline U32 get_cpu_clock_count_32()
{
	U64 time_stamp = __rdtsc();
	return (U32)(time_stamp >> 8);
}

// return full timer value, *not* shifted by 8 bits
inline U64 get_cpu_clock_count_64()
{
	return __rdtsc();
}
#else
#define LL_INLINE
#endif // LL_WINDOWS

#if (LL_LINUX || LL_SOLARIS || LL_DARWIN) && (defined(__i386__) || defined(__amd64__))
inline U32 get_cpu_clock_count_32()
{																	
	U64 x;															
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));					
	return (U32)x >> 8;													
}

inline U32 get_cpu_clock_count_64()
{																	
	U64 x;
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
	return x;
}
#endif

#if ( LL_DARWIN && !(defined(__i386__) || defined(__amd64__))) || (LL_SOLARIS && defined(__sparc__))
//
// Mac PPC (deprecated) & Solaris SPARC implementation of CPU clock
//
// Just use gettimeofday implementation for now

inline U32 get_cpu_clock_count_32()
{
	return (U32)get_clock_count();
}

inline U32 get_cpu_clock_count_64()
{																	
	return get_clock_count();
}
#endif

class LLMutex;

#include <queue>
#include "llsd.h"

class LL_COMMON_API LLFastTimer
{
public:

	class NamedTimer;

	struct LL_COMMON_API FrameState
	{
		FrameState(NamedTimer* timerp);

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

		enum { HISTORY_NUM = 60 };

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

		static NamedTimer& getRootNamedTimer();

		S32 getFrameStateIndex() const { return mFrameStateIndex; }

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
		S32			mFrameStateIndex;

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

		static void updateCachedPointers();

	private:
		NamedTimer&		mTimer;
		FrameState*		mFrameState; 
	};

public:
	LLFastTimer(LLFastTimer::FrameState* state);

	LL_INLINE LLFastTimer(LLFastTimer::DeclareTimer& timer)
	:	mFrameState(timer.mFrameState)
	{
#if TIME_FAST_TIMERS
		U64 timer_start = get_cpu_clock_count_64();
#endif
#if FAST_TIMER_ON
		LLFastTimer::FrameState* frame_state = mFrameState;
		mStartTime = get_cpu_clock_count_32();

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
#if TIME_FAST_TIMERS
		U64 timer_end = get_cpu_clock_count_64();
		sTimerCycles += timer_end - timer_start;
#endif
	}

	LL_INLINE ~LLFastTimer()
	{
#if TIME_FAST_TIMERS
		U64 timer_start = get_cpu_clock_count_64();
#endif
#if FAST_TIMER_ON
		LLFastTimer::FrameState* frame_state = mFrameState;
		U32 total_time = get_cpu_clock_count_32() - mStartTime;

		frame_state->mSelfTimeCounter += total_time - LLFastTimer::sCurTimerData.mChildTime;
		frame_state->mActiveCount--;

		// store last caller to bootstrap tree creation
		// do this in the destructor in case of recursion to get topmost caller
		frame_state->mLastCaller = mLastTimerData.mFrameState;

		// we are only tracking self time, so subtract our total time delta from parents
		mLastTimerData.mChildTime += total_time;

		LLFastTimer::sCurTimerData = mLastTimerData;
#endif
#if TIME_FAST_TIMERS
		U64 timer_end = get_cpu_clock_count_64();
		sTimerCycles += timer_end - timer_start;
		sTimerCalls++;
#endif	
	}

public:
	static LLMutex*			sLogLock;
	static std::queue<LLSD> sLogQueue;
	static BOOL				sLog;
	static BOOL				sMetricLog;
	static bool 			sPauseHistory;
	static bool 			sResetHistory;
	static U64				sTimerCycles;
	static U32				sTimerCalls;

	typedef std::vector<FrameState> info_list_t;
	static info_list_t& getFrameStateList();

	
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
	static S32				sCurFrameIndex;
	static S32				sLastFrameIndex;
	static U64				sLastFrameTime;
	static info_list_t*		sTimerInfos;

	U32							mStartTime;
	LLFastTimer::FrameState*	mFrameState;
	LLFastTimer::CurTimerData	mLastTimerData;

};

typedef class LLFastTimer LLFastTimer;

#endif // LL_LLFASTTIMER_H
