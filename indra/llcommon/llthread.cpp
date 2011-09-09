/** 
 * @file llthread.cpp
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
#include "llapr.h"

#include "apr_portable.h"

#include "llthread.h"

#include "lltimer.h"

#if LL_LINUX || LL_SOLARIS
#include <sched.h>
#endif

#if !LL_DARWIN
U32 ll_thread_local local_thread_ID = 0;
#endif 

U32 LLThread::sIDIter = 0;

//----------------------------------------------------------------------------
// Usage:
// void run_func(LLThread* thread)
// {
// }
// LLThread* thread = new LLThread();
// thread->run(run_func);
// ...
// thread->setQuitting();
// while(!timeout)
// {
//   if (thread->isStopped())
//   {
//     delete thread;
//     break;
//   }
// }
// 
//----------------------------------------------------------------------------

LL_COMMON_API void assert_main_thread()
{
	static U32 s_thread_id = LLThread::currentID();
	if (LLThread::currentID() != s_thread_id)
	{
		llerrs << "Illegal execution outside main thread." << llendl;
	}
}

//
// Handed to the APR thread creation function
//
void *APR_THREAD_FUNC LLThread::staticRun(apr_thread_t *apr_threadp, void *datap)
{
	LLThread *threadp = (LLThread *)datap;

#if !LL_DARWIN
	local_thread_ID = threadp->mID;
#endif

	// Create a thread local data.
	LLThreadLocalData::create(threadp);

	// Run the user supplied function
	threadp->run();

	//llinfos << "LLThread::staticRun() Exiting: " << threadp->mName << llendl;
	
	// We're done with the run function, this thread is done executing now.
	threadp->mStatus = STOPPED;

	return NULL;
}


LLThread::LLThread(std::string const& name) :
	mPaused(false),
	mName(name),
	mAPRThreadp(NULL),
	mStatus(STOPPED),
	mThreadLocalData(NULL)
{
	mID = ++sIDIter; //flaw: assume this is called only in the main thread!

	mRunCondition = new LLCondition;
}


LLThread::~LLThread()
{
	shutdown();
}

void LLThread::shutdown()
{
	// Warning!  If you somehow call the thread destructor from itself,
	// the thread will die in an unclean fashion!
	if (mAPRThreadp)
	{
		if (!isStopped())
		{
			// The thread isn't already stopped
			// First, set the flag that indicates that we're ready to die
			setQuitting();

			//llinfos << "LLThread::~LLThread() Killing thread " << mName << " Status: " << mStatus << llendl;
			// Now wait a bit for the thread to exit
			// It's unclear whether I should even bother doing this - this destructor
			// should netver get called unless we're already stopped, really...
			S32 counter = 0;
			const S32 MAX_WAIT = 600;
			while (counter < MAX_WAIT)
			{
				if (isStopped())
				{
					break;
				}
				// Sleep for a tenth of a second
				ms_sleep(100);
				yield();
				counter++;
			}
		}

		if (!isStopped())
		{
			// This thread just wouldn't stop, even though we gave it time
			//llwarns << "LLThread::shutdown() exiting thread before clean exit!" << llendl;
			// Put a stake in its heart.
			apr_thread_exit(mAPRThreadp, -1);
			return;
		}
		mAPRThreadp = NULL;
	}

	delete mRunCondition;
	mRunCondition = 0;
}

void LLThread::start()
{
	llassert(isStopped());
	
	// Set thread state to running
	mStatus = RUNNING;

	apr_status_t status =
		apr_thread_create(&mAPRThreadp, NULL, staticRun, (void *)this, tldata().mRootPool());
	
	if(status == APR_SUCCESS)
	{	
		// We won't bother joining
		apr_thread_detach(mAPRThreadp);
	}
	else
	{
		mStatus = STOPPED;
		llwarns << "failed to start thread " << mName << llendl;
		ll_apr_warn_status(status);
	}
}

//============================================================================
// Called from MAIN THREAD.

// Request that the thread pause/resume.
// The thread will pause when (and if) it calls checkPause()
void LLThread::pause()
{
	if (!mPaused)
	{
		// this will cause the thread to stop execution as soon as checkPause() is called
		mPaused = true;		// Does not need to be atomic since this is only set/unset from the main thread
	}	
}

void LLThread::unpause()
{
	if (mPaused)
	{
		mPaused = false;
	}

	wake(); // wake up the thread if necessary
}

// virtual predicate function -- returns true if the thread should wake up, false if it should sleep.
bool LLThread::runCondition(void)
{
	// by default, always run.  Handling of pause/unpause is done regardless of this function's result.
	return true;
}

//============================================================================
// Called from run() (CHILD THREAD).
// Stop thread execution if requested until unpaused.
void LLThread::checkPause()
{
	mRunCondition->lock();

	// This is in a while loop because the pthread API allows for spurious wakeups.
	while(shouldSleep())
	{
		mRunCondition->wait(); // unlocks mRunCondition
		// mRunCondition is locked when the thread wakes up
	}
	
 	mRunCondition->unlock();
}

//============================================================================

void LLThread::setQuitting()
{
	mRunCondition->lock();
	if (mStatus == RUNNING)
	{
		mStatus = QUITTING;
	}
	mRunCondition->unlock();
	wake();
}

// static
U32 LLThread::currentID()
{
	return (U32)apr_os_thread_current();
}

// static
void LLThread::yield()
{
#if LL_LINUX || LL_SOLARIS
	sched_yield(); // annoyingly, apr_thread_yield  is a noop on linux...
#else
	apr_thread_yield();
#endif
}

void LLThread::wake()
{
	mRunCondition->lock();
	if(!shouldSleep())
	{
		mRunCondition->signal();
	}
	mRunCondition->unlock();
}

void LLThread::wakeLocked()
{
	if(!shouldSleep())
	{
		mRunCondition->signal();
	}
}

#ifdef SHOW_ASSERT
// This allows the use of llassert(is_main_thread()) to assure the current thread is the main thread.
static apr_os_thread_t main_thread_id;
LL_COMMON_API bool is_main_thread(void) { return apr_os_thread_equal(main_thread_id, apr_os_thread_current()); }
#endif

// The thread private handle to access the LLThreadLocalData instance.
apr_threadkey_t* LLThreadLocalData::sThreadLocalDataKey;

//static
void LLThreadLocalData::init(void)
{
	// Only do this once.
	if (sThreadLocalDataKey)
	{
		return;
	}

	apr_status_t status = apr_threadkey_private_create(&sThreadLocalDataKey, &LLThreadLocalData::destroy, LLAPRRootPool::get()());
	ll_apr_assert_status(status);   // Or out of memory, or system-imposed limit on the
									// total number of keys per process {PTHREAD_KEYS_MAX}
									// has been exceeded.

	// Create the thread-local data for the main thread (this function is called by the main thread).
	LLThreadLocalData::create(NULL);

#ifdef SHOW_ASSERT
	// This function is called by the main thread.
	main_thread_id = apr_os_thread_current();
#endif
}

// This is called once for every thread when the thread is destructed.
//static
void LLThreadLocalData::destroy(void* thread_local_data)
{
	delete static_cast<LLThreadLocalData*>(thread_local_data);
}

//static
void LLThreadLocalData::create(LLThread* threadp)
{
	LLThreadLocalData* new_tld = new LLThreadLocalData;
	if (threadp)
	{
		threadp->mThreadLocalData = new_tld;
	}
	apr_status_t status = apr_threadkey_private_set(new_tld, sThreadLocalDataKey);
	llassert_always(status == APR_SUCCESS);
}

//static
LLThreadLocalData& LLThreadLocalData::tldata(void)
{
	if (!sThreadLocalDataKey)
	{
		LLThreadLocalData::init();
	}

	void* data;
	apr_status_t status = apr_threadkey_private_get(&data, sThreadLocalDataKey);
	llassert_always(status == APR_SUCCESS);
	return *static_cast<LLThreadLocalData*>(data);
}

//============================================================================

LLCondition::LLCondition(LLAPRPool& parent) : LLMutex(parent)
{
	apr_thread_cond_create(&mAPRCondp, mPool());
}


LLCondition::~LLCondition()
{
	apr_thread_cond_destroy(mAPRCondp);
	mAPRCondp = NULL;
}


void LLCondition::wait()
{
	apr_thread_cond_wait(mAPRCondp, mAPRMutexp);
}

void LLCondition::signal()
{
	apr_thread_cond_signal(mAPRCondp);
}

void LLCondition::broadcast()
{
	apr_thread_cond_broadcast(mAPRCondp);
}

//============================================================================
LLMutexBase::LLMutexBase() :
	mLockingThread(NO_THREAD),
	mCount(0)
{
}

void LLMutexBase::lock() 
{ 
#if LL_DARWIN
	if (mLockingThread == LLThread::currentID())
#else
	if (mLockingThread == local_thread_ID)
#endif
	{ //redundant lock
		mCount++;
		return;
	}

	apr_thread_mutex_lock(mAPRMutexp); 

#if LL_DARWIN
	mLockingThread = LLThread::currentID();
#else
	mLockingThread = local_thread_ID;
#endif
}

void LLMutexBase::unlock() 
{ 
	if (mCount > 0)
	{ //not the root unlock
		mCount--;
		return;
	}
	mLockingThread = NO_THREAD;

	apr_thread_mutex_unlock(mAPRMutexp); 
}

//----------------------------------------------------------------------------

//static
LLMutex* LLThreadSafeRefCount::sMutex = 0;

//static
void LLThreadSafeRefCount::initThreadSafeRefCount()
{
	if (!sMutex)
	{
		sMutex = new LLMutex;
	}
}

//static
void LLThreadSafeRefCount::cleanupThreadSafeRefCount()
{
	delete sMutex;
	sMutex = NULL;
}
	

//----------------------------------------------------------------------------

LLThreadSafeRefCount::LLThreadSafeRefCount() :
	mRef(0)
{
}

LLThreadSafeRefCount::~LLThreadSafeRefCount()
{ 
	if (mRef != 0)
	{
		llerrs << "deleting non-zero reference" << llendl;
	}
}

//============================================================================

LLResponder::~LLResponder()
{
}

//============================================================================
