/** 
 * @file llrun.cpp
 * @author Phoenix
 * @date 2006-02-16
 * @brief Implementation of the LLRunner and related classes
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llrun.h"

#include "llframetimer.h"

static const LLRunner::run_handle_t INVALID_RUN_HANDLE = 0;

/** 
 * LLRunner
 */
LLRunner::LLRunner() :
	mNextHandle(1)
{
}

LLRunner::~LLRunner()
{
	mRunOnce.clear();
	mRunEvery.clear();
}

S32 LLRunner::run()
{
	// We collect all of the runnables which should be run. Since the
	// runnables are allowed to adjust the run list, we need to copy
	// them into a temporary structure which then iterates over them
	// to call out of this method into the runnables.
	F64 now = LLFrameTimer::getTotalSeconds();
	run_list_t run_now;

	// Collect the run once. We erase the matching ones now because
	// it's easier. If we find a reason to keep them around for a
	// while, we can restructure this method.
	LLRunner::run_list_t::iterator iter = mRunOnce.begin();
	for( ; iter != mRunOnce.end(); )
	{
		if(now > (*iter).mNextRunAt)
		{
			run_now.push_back(*iter);
			iter = mRunOnce.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	// Collect the ones that repeat.
	iter = mRunEvery.begin();
	LLRunner::run_list_t::iterator end = mRunEvery.end();
	for( ; iter != end; ++iter )
	{
		if(now > (*iter).mNextRunAt)
		{
			(*iter).mNextRunAt = now + (*iter).mIncrement;
			run_now.push_back(*iter);
		}
	}

	// Now, run them.
	iter = run_now.begin();
	end = run_now.end();
	for( ; iter != end; ++iter )
	{
		(*iter).mRunnable->run(this, (*iter).mHandle);
	}
	return run_now.size();
}

LLRunner::run_handle_t LLRunner::addRunnable(
	run_ptr_t runnable,
	ERunSchedule schedule,
	F64 seconds)
{
	if(!runnable) return INVALID_RUN_HANDLE;
	run_handle_t handle = mNextHandle++;
	F64 next_run = LLFrameTimer::getTotalSeconds() + seconds;
	LLRunInfo info(handle, runnable, schedule, next_run, seconds);
	switch(schedule)
	{
	case RUN_IN:
		// We could optimize this a bit by sorting this on entry.
		mRunOnce.push_back(info);
		break;
	case RUN_EVERY:
		mRunEvery.push_back(info);
		break;
	default:
		handle = INVALID_RUN_HANDLE;
		break;
	}
	return handle;
}

LLRunner::run_ptr_t LLRunner::removeRunnable(LLRunner::run_handle_t handle)
{
	LLRunner::run_ptr_t rv;
	LLRunner::run_list_t::iterator iter = mRunOnce.begin();
	LLRunner::run_list_t::iterator end = mRunOnce.end();
	for( ; iter != end; ++iter)
	{
		if((*iter).mHandle == handle)
		{
			rv = (*iter).mRunnable;
			mRunOnce.erase(iter);
			return rv;
		}
	}

	iter = mRunEvery.begin();
	end = mRunEvery.end();
	for( ; iter != end; ++iter)
	{
		if((*iter).mHandle == handle)
		{
			rv = (*iter).mRunnable;
			mRunEvery.erase(iter);
			return rv;
		}
	}
	return rv;
}

/** 
 * LLRunner::LLRunInfo
 */
LLRunner::LLRunInfo::LLRunInfo(
	run_handle_t handle,
	run_ptr_t runnable,
	ERunSchedule schedule,
	F64 next_run_after,
	F64 increment) :
	mHandle(handle),
	mRunnable(runnable),
	mSchedule(schedule),
	mNextRunAt(next_run_after),
	mIncrement(increment)
{
}

LLRunnable::LLRunnable()
{ }

// virtual
LLRunnable::~LLRunnable()
{ }
