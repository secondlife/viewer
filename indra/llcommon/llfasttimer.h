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

U64 get_cpu_clock_count();

class LLMutex;

#include <queue>
#include "llsd.h"


class LLFastTimer
{
public:
	// stores a "named" timer instance to be reused via multiple LLFastTimer stack instances
	class NamedTimer 
	:	public LLInstanceTracker<NamedTimer>
	{
	public:
		~NamedTimer();

		enum { HISTORY_NUM = 60 };

		const std::string& getName() { return mName; }
		NamedTimer* getParent() { return mParent; }
		void setParent(NamedTimer* parent);
		S32 getDepth();
		std::string getToolTip(S32 history_index = -1);

		typedef std::vector<NamedTimer*>::const_iterator child_const_iter;
		child_const_iter beginChildren();
		child_const_iter endChildren();
		std::vector<NamedTimer*>& getChildren();

		void setCollapsed(bool collapsed) { mCollapsed = collapsed; }
		bool getCollapsed() const { return mCollapsed; }

		U64 getCountAverage() const { return mCountAverage; }
		U64 getCallAverage() const { return mCallAverage; }

		U64 getHistoricalCount(S32 history_index = 0) const;
		U64 getHistoricalCalls(S32 history_index = 0) const;

		static NamedTimer& getRootNamedTimer();

		struct FrameState
		{
			FrameState(NamedTimer* timerp);

			U64 		mSelfTimeCounter;
			U64			mLastStartTime;		// most recent time when this timer was started
			U32 		mCalls;
			FrameState*	mParent;		// info for caller timer
			FrameState*	mLastCaller;	// used to bootstrap tree construction
			NamedTimer*	mTimer;
			U16			mActiveCount;	// number of timers with this ID active on stack
			bool		mMoveUpTree;	// needs to be moved up the tree of timers at the end of frame
		};

		FrameState& getFrameStateFast() const
		{
			return (*sTimerInfos)[mFrameStateIndex];
		}

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

		typedef std::vector<FrameState> info_list_t;
		static info_list_t& getFrameStateList();
		static void createFrameStateList(); // must call before any call to getFrameStateList()
		
		//
		// members
		//
		S32			mFrameStateIndex;

		std::string	mName;

		U64 		mTotalTimeCounter;

		U64 		mCountAverage;
		U64			mCallAverage;

		U64*		mCountHistory;
		U64*		mCallHistory;

		// tree structure
		NamedTimer*					mParent;				// NamedTimer of caller(parent)
		std::vector<NamedTimer*>	mChildren;
		bool						mCollapsed;				// don't show children
		bool						mNeedsSorting;			// sort children whenever child added

		static info_list_t* sTimerInfos;
	};

	// used to statically declare a new named timer
	class DeclareTimer
	{
	public:
		DeclareTimer(const std::string& name, bool open);
		DeclareTimer(const std::string& name);

		// convertable to NamedTimer::FrameState for convenient usage of LLFastTimer(declared_timer)
		operator NamedTimer::FrameState&() { return mNamedTimer.getFrameStateFast(); }
	private:
		NamedTimer& mNamedTimer;
	};


public:
	enum RootTimerMarker { ROOT };
	
	static LLMutex* sLogLock;
	static std::queue<LLSD> sLogQueue;
	static BOOL sLog;
	static BOOL sMetricLog;

	LLFastTimer(RootTimerMarker);

	LLFastTimer(NamedTimer::FrameState& timer)
	:	mFrameState(&timer)
	{
#if FAST_TIMER_ON
		NamedTimer::FrameState* frame_state = mFrameState;
		U64 cur_time = get_cpu_clock_count();
		frame_state->mLastStartTime = cur_time;
		mStartSelfTime = cur_time;

		frame_state->mActiveCount++;
		frame_state->mCalls++;
		// keep current parent as long as it is active when we are
		frame_state->mMoveUpTree |= (frame_state->mParent->mActiveCount == 0);
	
		mLastTimer = sCurTimer;
		sCurTimer = this;
#endif
	}

	~LLFastTimer()
	{
#if FAST_TIMER_ON
		NamedTimer::FrameState* frame_state = mFrameState;
		U64 cur_time = get_cpu_clock_count();
		frame_state->mSelfTimeCounter += cur_time - mStartSelfTime;

		frame_state->mActiveCount--;
		LLFastTimer* last_timer = mLastTimer;
		sCurTimer = last_timer;

		// store last caller to bootstrap tree creation
		frame_state->mLastCaller = last_timer->mFrameState;

		// we are only tracking self time, so subtract our total time delta from parents
		U64 total_time = cur_time - frame_state->mLastStartTime;
		last_timer->mStartSelfTime += total_time;
#endif
	}


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

public:
	static bool 		sPauseHistory;
	static bool 		sResetHistory;
	
private:
	typedef std::vector<LLFastTimer*> timer_stack_t;
	static LLFastTimer*		sCurTimer;
	static S32				sCurFrameIndex;
	static S32				sLastFrameIndex;

	static F64				sCPUClockFrequency;
	U64						mStartSelfTime;	// start time + time of all child timers
	NamedTimer::FrameState*	mFrameState;
	LLFastTimer*			mLastTimer;
};

#endif // LL_LLFASTTIMER_H
