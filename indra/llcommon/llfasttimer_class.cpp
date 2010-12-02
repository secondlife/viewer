/** 
 * @file llfasttimer_class.cpp
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

#include <boost/bind.hpp>


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

//////////////////////////////////////////////////////////////////////////////
// statics

S32 LLFastTimer::sCurFrameIndex = -1;
S32 LLFastTimer::sLastFrameIndex = -1;
U64 LLFastTimer::sLastFrameTime = LLFastTimer::getCPUClockCount64();
bool LLFastTimer::sPauseHistory = 0;
bool LLFastTimer::sResetHistory = 0;
LLFastTimer::CurTimerData LLFastTimer::sCurTimerData;
BOOL LLFastTimer::sLog = FALSE;
BOOL LLFastTimer::sMetricLog = FALSE;
LLMutex* LLFastTimer::sLogLock = NULL;
std::queue<LLSD> LLFastTimer::sLogQueue;

#define USE_RDTSC 0

#if LL_LINUX || LL_SOLARIS
U64 LLFastTimer::sClockResolution = 1000000000; // Nanosecond resolution
#else
U64 LLFastTimer::sClockResolution = 1000000; // Microsecond resolution
#endif

std::vector<LLFastTimer::FrameState>* LLFastTimer::sTimerInfos = NULL;
U64				LLFastTimer::sTimerCycles = 0;
U32				LLFastTimer::sTimerCalls = 0;


// FIXME: move these declarations to the relevant modules

// helper functions
typedef LLTreeDFSPostIter<LLFastTimer::NamedTimer, LLFastTimer::NamedTimer::child_const_iter> timer_tree_bottom_up_iterator_t;

static timer_tree_bottom_up_iterator_t begin_timer_tree_bottom_up(LLFastTimer::NamedTimer& id) 
{ 
	return timer_tree_bottom_up_iterator_t(&id, 
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::beginChildren), _1), 
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::endChildren), _1));
}

static timer_tree_bottom_up_iterator_t end_timer_tree_bottom_up() 
{ 
	return timer_tree_bottom_up_iterator_t(); 
}

typedef LLTreeDFSIter<LLFastTimer::NamedTimer, LLFastTimer::NamedTimer::child_const_iter> timer_tree_dfs_iterator_t;


static timer_tree_dfs_iterator_t begin_timer_tree(LLFastTimer::NamedTimer& id) 
{ 
	return timer_tree_dfs_iterator_t(&id, 
		boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::beginChildren), _1), 
							boost::bind(boost::mem_fn(&LLFastTimer::NamedTimer::endChildren), _1));
}

static timer_tree_dfs_iterator_t end_timer_tree() 
{ 
	return timer_tree_dfs_iterator_t(); 
}



// factory class that creates NamedTimers via static DeclareTimer objects
class NamedTimerFactory : public LLSingleton<NamedTimerFactory>
{
public:
	NamedTimerFactory()
		: mActiveTimerRoot(NULL),
		  mTimerRoot(NULL),
		  mAppTimer(NULL),
		  mRootFrameState(NULL)
	{}

	/*virtual */ void initSingleton()
	{
		mTimerRoot = new LLFastTimer::NamedTimer("root");

		mActiveTimerRoot = new LLFastTimer::NamedTimer("Frame");
		mActiveTimerRoot->setCollapsed(false);

		mRootFrameState = new LLFastTimer::FrameState(mActiveTimerRoot);
		mRootFrameState->mParent = &mTimerRoot->getFrameState();
		mActiveTimerRoot->setParent(mTimerRoot);

		mAppTimer = new LLFastTimer(mRootFrameState);
	}

	~NamedTimerFactory()
	{
		std::for_each(mTimers.begin(), mTimers.end(), DeletePairedPointer());

		delete mAppTimer;
		delete mActiveTimerRoot; 
		delete mTimerRoot;
		delete mRootFrameState;
	}

	LLFastTimer::NamedTimer& createNamedTimer(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return *found_it->second;
		}

		LLFastTimer::NamedTimer* timer = new LLFastTimer::NamedTimer(name);
		timer->setParent(mTimerRoot);
		mTimers.insert(std::make_pair(name, timer));

		return *timer;
	}

	LLFastTimer::NamedTimer* getTimerByName(const std::string& name)
	{
		timer_map_t::iterator found_it = mTimers.find(name);
		if (found_it != mTimers.end())
		{
			return found_it->second;
		}
		return NULL;
	}

	LLFastTimer::NamedTimer* getActiveRootTimer() { return mActiveTimerRoot; }
	LLFastTimer::NamedTimer* getRootTimer() { return mTimerRoot; }
	const LLFastTimer* getAppTimer() { return mAppTimer; }
	LLFastTimer::FrameState& getRootFrameState() { return *mRootFrameState; }

	typedef std::map<std::string, LLFastTimer::NamedTimer*> timer_map_t;
	timer_map_t::iterator beginTimers() { return mTimers.begin(); }
	timer_map_t::iterator endTimers() { return mTimers.end(); }
	S32 timerCount() { return mTimers.size(); }

private:
	timer_map_t mTimers;

	LLFastTimer::NamedTimer*		mActiveTimerRoot;
	LLFastTimer::NamedTimer*		mTimerRoot;
	LLFastTimer*						mAppTimer;
	LLFastTimer::FrameState*		mRootFrameState;
};

void update_cached_pointers_if_changed()
{
	// detect when elements have moved and update cached pointers
	static LLFastTimer::FrameState* sFirstTimerAddress = NULL;
	if (&*(LLFastTimer::getFrameStateList().begin()) != sFirstTimerAddress)
	{
		LLFastTimer::DeclareTimer::updateCachedPointers();
	}
	sFirstTimerAddress = &*(LLFastTimer::getFrameStateList().begin());
}

LLFastTimer::DeclareTimer::DeclareTimer(const std::string& name, bool open )
:	mTimer(NamedTimerFactory::instance().createNamedTimer(name))
{
	mTimer.setCollapsed(!open);
	mFrameState = &mTimer.getFrameState();
	update_cached_pointers_if_changed();
}

LLFastTimer::DeclareTimer::DeclareTimer(const std::string& name)
:	mTimer(NamedTimerFactory::instance().createNamedTimer(name))
{
	mFrameState = &mTimer.getFrameState();
	update_cached_pointers_if_changed();
}

// static
void LLFastTimer::DeclareTimer::updateCachedPointers()
{
	DeclareTimer::LLInstanceTrackerScopedGuard guard;
	// propagate frame state pointers to timer declarations
	for (DeclareTimer::instance_iter it = guard.beginInstances();
		it != guard.endInstances();
		++it)
	{
		// update cached pointer
		it->mFrameState = &it->mTimer.getFrameState();
	}
}

//static
#if (LL_DARWIN || LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))
U64 LLFastTimer::countsPerSecond() // counts per second for the *32-bit* timer
{
	return sClockResolution >> 8;
}
#else // windows or x86-mac or x86-linux or x86-solaris
U64 LLFastTimer::countsPerSecond() // counts per second for the *32-bit* timer
{
#if USE_RDTSC || !LL_WINDOWS
	//getCPUFrequency returns MHz and sCPUClockFrequency wants to be in Hz
	static U64 sCPUClockFrequency = U64(LLProcessorInfo().getCPUFrequency()*1000000.0);

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

LLFastTimer::FrameState::FrameState(LLFastTimer::NamedTimer* timerp)
:	mActiveCount(0),
	mCalls(0),
	mSelfTimeCounter(0),
	mParent(NULL),
	mLastCaller(NULL),
	mMoveUpTree(false),
	mTimer(timerp)
{}


LLFastTimer::NamedTimer::NamedTimer(const std::string& name)
:	mName(name),
	mCollapsed(true),
	mParent(NULL),
	mTotalTimeCounter(0),
	mCountAverage(0),
	mCallAverage(0),
	mNeedsSorting(false)
{
	info_list_t& frame_state_list = getFrameStateList();
	mFrameStateIndex = frame_state_list.size();
	getFrameStateList().push_back(FrameState(this));

	mCountHistory = new U32[HISTORY_NUM];
	memset(mCountHistory, 0, sizeof(U32) * HISTORY_NUM);
	mCallHistory = new U32[HISTORY_NUM];
	memset(mCallHistory, 0, sizeof(U32) * HISTORY_NUM);
}

LLFastTimer::NamedTimer::~NamedTimer()
{
	delete[] mCountHistory;
	delete[] mCallHistory;
}

std::string LLFastTimer::NamedTimer::getToolTip(S32 history_idx)
{
	if (history_idx < 0)
	{
		// by default, show average number of calls
		return llformat("%s (%d calls)", getName().c_str(), (S32)getCallAverage());
	}
	else
	{
		return llformat("%s (%d calls)", getName().c_str(), (S32)getHistoricalCalls(history_idx));
	}
}

void LLFastTimer::NamedTimer::setParent(NamedTimer* parent)
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

		std::vector<NamedTimer*>& children = mParent->getChildren();
		std::vector<NamedTimer*>::iterator found_it = std::find(children.begin(), children.end(), this);
		if (found_it != children.end())
		{
			children.erase(found_it);
		}
	}

	mParent = parent;
	if (parent)
	{
		getFrameState().mParent = &parent->getFrameState();
		parent->getChildren().push_back(this);
		parent->mNeedsSorting = true;
	}
}

S32 LLFastTimer::NamedTimer::getDepth()
{
	S32 depth = 0;
	NamedTimer* timerp = mParent;
	while(timerp)
	{
		depth++;
		timerp = timerp->mParent;
	}
	return depth;
}

// static
void LLFastTimer::NamedTimer::processTimes()
{
	if (sCurFrameIndex < 0) return;

	buildHierarchy();
	accumulateTimings();
}

// sort timer info structs by depth first traversal order
struct SortTimersDFS
{
	bool operator()(const LLFastTimer::FrameState& i1, const LLFastTimer::FrameState& i2)
	{
		return i1.mTimer->getFrameStateIndex() < i2.mTimer->getFrameStateIndex();
	}
};

// sort child timers by name
struct SortTimerByName
{
	bool operator()(const LLFastTimer::NamedTimer* i1, const LLFastTimer::NamedTimer* i2)
	{
		return i1->getName() < i2->getName();
	}
};

//static
void LLFastTimer::NamedTimer::buildHierarchy()
{
	if (sCurFrameIndex < 0 ) return;

	// set up initial tree
	{
		NamedTimer::LLInstanceTrackerScopedGuard guard;
		for (instance_iter it = guard.beginInstances();
		     it != guard.endInstances();
		     ++it)
		{
			NamedTimer& timer = *it;
			if (&timer == NamedTimerFactory::instance().getRootTimer()) continue;
			
			// bootstrap tree construction by attaching to last timer to be on stack
			// when this timer was called
			if (timer.getFrameState().mLastCaller && timer.mParent == NamedTimerFactory::instance().getRootTimer())
			{
				timer.setParent(timer.getFrameState().mLastCaller->mTimer);
				// no need to push up tree on first use, flag can be set spuriously
				timer.getFrameState().mMoveUpTree = false;
			}
		}
	}

	// bump timers up tree if they've been flagged as being in the wrong place
	// do this in a bottom up order to promote descendants first before promoting ancestors
	// this preserves partial order derived from current frame's observations
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		NamedTimer* timerp = *it;
		// skip root timer
		if (timerp == NamedTimerFactory::instance().getRootTimer()) continue;

		if (timerp->getFrameState().mMoveUpTree)
		{
			// since ancestors have already been visited, reparenting won't affect tree traversal
			//step up tree, bringing our descendants with us
			//llinfos << "Moving " << timerp->getName() << " from child of " << timerp->getParent()->getName() <<
			//	" to child of " << timerp->getParent()->getParent()->getName() << llendl;
			timerp->setParent(timerp->getParent()->getParent());
			timerp->getFrameState().mMoveUpTree = false;

			// don't bubble up any ancestors until descendants are done bubbling up
			it.skipAncestors();
		}
	}

	// sort timers by time last called, so call graph makes sense
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		if (timerp->mNeedsSorting)
		{
			std::sort(timerp->getChildren().begin(), timerp->getChildren().end(), SortTimerByName());
		}
		timerp->mNeedsSorting = false;
	}
}

//static
void LLFastTimer::NamedTimer::accumulateTimings()
{
	U32 cur_time = getCPUClockCount32();

	// walk up stack of active timers and accumulate current time while leaving timing structures active
	LLFastTimer* cur_timer = sCurTimerData.mCurTimer;
	// root defined by parent pointing to self
	CurTimerData* cur_data = &sCurTimerData;
	while(cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		U32 cumulative_time_delta = cur_time - cur_timer->mStartTime;
		U32 self_time_delta = cumulative_time_delta - cur_data->mChildTime;
		cur_data->mChildTime = 0;
		cur_timer->mFrameState->mSelfTimeCounter += self_time_delta;
		cur_timer->mStartTime = cur_time;

		cur_data = &cur_timer->mLastTimerData;
		cur_data->mChildTime += cumulative_time_delta;

		cur_timer = cur_timer->mLastTimerData.mCurTimer;
	}

	// traverse tree in DFS post order, or bottom up
	for(timer_tree_bottom_up_iterator_t it = begin_timer_tree_bottom_up(*NamedTimerFactory::instance().getActiveRootTimer());
		it != end_timer_tree_bottom_up();
		++it)
	{
		NamedTimer* timerp = (*it);
		timerp->mTotalTimeCounter = timerp->getFrameState().mSelfTimeCounter;
		for (child_const_iter child_it = timerp->beginChildren(); child_it != timerp->endChildren(); ++child_it)
		{
			timerp->mTotalTimeCounter += (*child_it)->mTotalTimeCounter;
		}

		S32 cur_frame = sCurFrameIndex;
		if (cur_frame >= 0)
		{
			// update timer history
			int hidx = cur_frame % HISTORY_NUM;

			timerp->mCountHistory[hidx] = timerp->mTotalTimeCounter;
			timerp->mCountAverage = ((U64)timerp->mCountAverage * cur_frame + timerp->mTotalTimeCounter) / (cur_frame+1);
			timerp->mCallHistory[hidx] = timerp->getFrameState().mCalls;
			timerp->mCallAverage = ((U64)timerp->mCallAverage * cur_frame + timerp->getFrameState().mCalls) / (cur_frame+1);
		}
	}
}

// static
void LLFastTimer::NamedTimer::resetFrame()
{
	if (sLog)
	{ //output current frame counts to performance log

		F64 iclock_freq = 1000.0 / countsPerSecond(); // good place to calculate clock frequency

		F64 total_time = 0;
		LLSD sd;

		{
			NamedTimer::LLInstanceTrackerScopedGuard guard;
			for (NamedTimer::instance_iter it = guard.beginInstances();
			     it != guard.endInstances();
			     ++it)
			{
				NamedTimer& timer = *it;
				FrameState& info = timer.getFrameState();
				sd[timer.getName()]["Time"] = (LLSD::Real) (info.mSelfTimeCounter*iclock_freq);	
				sd[timer.getName()]["Calls"] = (LLSD::Integer) info.mCalls;
				
				// computing total time here because getting the root timer's getCountHistory
				// doesn't work correctly on the first frame
				total_time = total_time + info.mSelfTimeCounter * iclock_freq;
			}
		}

		sd["Total"]["Time"] = (LLSD::Real) total_time;
		sd["Total"]["Calls"] = (LLSD::Integer) 1;

		{		
			LLMutexLock lock(sLogLock);
			sLogQueue.push(sd);
		}
	}


	// tag timers by position in depth first traversal of tree
	S32 index = 0;
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
		
		timerp->mFrameStateIndex = index;
		index++;

		llassert_always(timerp->mFrameStateIndex < (S32)getFrameStateList().size());
	}

	// sort timers by dfs traversal order to improve cache coherency
	std::sort(getFrameStateList().begin(), getFrameStateList().end(), SortTimersDFS());

	// update pointers into framestatelist now that we've sorted it
	DeclareTimer::updateCachedPointers();

	// reset for next frame
	{
		NamedTimer::LLInstanceTrackerScopedGuard guard;
		for (NamedTimer::instance_iter it = guard.beginInstances();
		     it != guard.endInstances();
		     ++it)
		{
			NamedTimer& timer = *it;
			
			FrameState& info = timer.getFrameState();
			info.mSelfTimeCounter = 0;
			info.mCalls = 0;
			info.mLastCaller = NULL;
			info.mMoveUpTree = false;
			// update parent pointer in timer state struct
			if (timer.mParent)
			{
				info.mParent = &timer.mParent->getFrameState();
			}
		}
	}

	//sTimerCycles = 0;
	//sTimerCalls = 0;
}

//static
void LLFastTimer::NamedTimer::reset()
{
	resetFrame(); // reset frame data

	// walk up stack of active timers and reset start times to current time
	// effectively zeroing out any accumulated time
	U32 cur_time = getCPUClockCount32();

	// root defined by parent pointing to self
	CurTimerData* cur_data = &sCurTimerData;
	LLFastTimer* cur_timer = cur_data->mCurTimer;
	while(cur_timer->mLastTimerData.mCurTimer != cur_timer)
	{
		cur_timer->mStartTime = cur_time;
		cur_data->mChildTime = 0;

		cur_data = &cur_timer->mLastTimerData;
		cur_timer = cur_data->mCurTimer;
	}

	// reset all history
	{
		NamedTimer::LLInstanceTrackerScopedGuard guard;
		for (NamedTimer::instance_iter it = guard.beginInstances();
		     it != guard.endInstances();
		     ++it)
		{
			NamedTimer& timer = *it;
			if (&timer != NamedTimerFactory::instance().getRootTimer()) 
			{
				timer.setParent(NamedTimerFactory::instance().getRootTimer());
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

//static 
LLFastTimer::info_list_t& LLFastTimer::getFrameStateList() 
{ 
	if (!sTimerInfos) 
	{ 
		sTimerInfos = new info_list_t(); 
	} 
	return *sTimerInfos; 
}


U32 LLFastTimer::NamedTimer::getHistoricalCount(S32 history_index) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % LLFastTimer::NamedTimer::HISTORY_NUM;
	return mCountHistory[history_idx];
}

U32 LLFastTimer::NamedTimer::getHistoricalCalls(S32 history_index ) const
{
	S32 history_idx = (getLastFrameIndex() + history_index) % LLFastTimer::NamedTimer::HISTORY_NUM;
	return mCallHistory[history_idx];
}

LLFastTimer::FrameState& LLFastTimer::NamedTimer::getFrameState() const
{
	llassert_always(mFrameStateIndex >= 0);
	if (this == NamedTimerFactory::instance().getActiveRootTimer()) 
	{
		return NamedTimerFactory::instance().getRootFrameState();
	}
	return getFrameStateList()[mFrameStateIndex];
}

// static
LLFastTimer::NamedTimer& LLFastTimer::NamedTimer::getRootNamedTimer()
{ 
	return *NamedTimerFactory::instance().getActiveRootTimer(); 
}

std::vector<LLFastTimer::NamedTimer*>::const_iterator LLFastTimer::NamedTimer::beginChildren()
{ 
	return mChildren.begin(); 
}

std::vector<LLFastTimer::NamedTimer*>::const_iterator LLFastTimer::NamedTimer::endChildren()
{
	return mChildren.end();
}

std::vector<LLFastTimer::NamedTimer*>& LLFastTimer::NamedTimer::getChildren()
{
	return mChildren;
}

//static
void LLFastTimer::nextFrame()
{
	countsPerSecond(); // good place to calculate clock frequency
	U64 frame_time = getCPUClockCount64();
	if ((frame_time - sLastFrameTime) >> 8 > 0xffffffff)
	{
		llinfos << "Slow frame, fast timers inaccurate" << llendl;
	}

	if (sPauseHistory)
	{
		sResetHistory = true;
	}
	else if (sResetHistory)
	{
		sLastFrameIndex = 0;
		sCurFrameIndex = 0;
		sResetHistory = false;
	}
	else // not paused
	{
		NamedTimer::processTimes();
		sLastFrameIndex = sCurFrameIndex++;
	}
	
	// get ready for next frame
	NamedTimer::resetFrame();
	sLastFrameTime = frame_time;
}

//static
void LLFastTimer::dumpCurTimes()
{
	// accumulate timings, etc.
	NamedTimer::processTimes();
	
	F64 clock_freq = (F64)countsPerSecond();
	F64 iclock_freq = 1000.0 / clock_freq; // clock_ticks -> milliseconds

	// walk over timers in depth order and output timings
	for(timer_tree_dfs_iterator_t it = begin_timer_tree(*NamedTimerFactory::instance().getRootTimer());
		it != end_timer_tree();
		++it)
	{
		NamedTimer* timerp = (*it);
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
void LLFastTimer::reset()
{
	NamedTimer::reset();
}


//static
void LLFastTimer::writeLog(std::ostream& os)
{
	while (!sLogQueue.empty())
	{
		LLSD& sd = sLogQueue.front();
		LLSDSerialize::toXML(sd, os);
		LLMutexLock lock(sLogLock);
		sLogQueue.pop();
	}
}

//static
const LLFastTimer::NamedTimer* LLFastTimer::getTimerByName(const std::string& name)
{
	return NamedTimerFactory::instance().getTimerByName(name);
}

LLFastTimer::LLFastTimer(LLFastTimer::FrameState* state)
:	mFrameState(state)
{
	U32 start_time = getCPUClockCount32();
	mStartTime = start_time;
	mFrameState->mActiveCount++;
	LLFastTimer::sCurTimerData.mCurTimer = this;
	LLFastTimer::sCurTimerData.mFrameState = mFrameState;
	LLFastTimer::sCurTimerData.mChildTime = 0;
	mLastTimerData = LLFastTimer::sCurTimerData;
}


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
#if USE_RDTSC
U32 LLFastTimer::getCPUClockCount32()
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
U64 LLFastTimer::getCPUClockCount64()
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
// These use QueryPerformanceCounter, which is arguably fine and also works on amd architectures.
U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(get_clock_count()>>8);
}

U64 LLFastTimer::getCPUClockCount64()
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
U64 LLFastTimer::getCPUClockCount64()
{
	struct timespec tp;
	
#ifdef CLOCK_MONOTONIC // MONOTONIC supported at build-time?
	if (-1 == clock_gettime(CLOCK_MONOTONIC,&tp)) // if MONOTONIC isn't supported at runtime then ouch, try REALTIME
#endif
		clock_gettime(CLOCK_REALTIME,&tp);

	return (tp.tv_sec*LLFastTimer::sClockResolution)+tp.tv_nsec;        
}

U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(LLFastTimer::getCPUClockCount64() >> 8);
}
#endif // (LL_LINUX || LL_SOLARIS) && !(defined(__i386__) || defined(__amd64__))


#if (LL_LINUX || LL_SOLARIS || LL_DARWIN) && (defined(__i386__) || defined(__amd64__))
//
// Mac+Linux+Solaris FAST x86 implementation of CPU clock
U32 LLFastTimer::getCPUClockCount32()
{
	U64 x;
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
	return (U32)(x >> 8);
}

U64 LLFastTimer::getCPUClockCount64()
{
	U64 x;
	__asm__ volatile (".byte 0x0f, 0x31": "=A"(x));
	return x;
}
#endif


#if ( LL_DARWIN && !(defined(__i386__) || defined(__amd64__)))
//
// Mac PPC (deprecated) implementation of CPU clock
//
// Just use gettimeofday implementation for now

U32 LLFastTimer::getCPUClockCount32()
{
	return (U32)(get_clock_count()>>8);
}

U64 LLFastTimer::getCPUClockCount64()
{
	return get_clock_count();
}
#endif

