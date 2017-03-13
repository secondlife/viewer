/** 
 * @file llthread.cpp
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010-2013, Linden Research, Inc.
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
#include "llmutex.h"

#include "lltimer.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"

#if LL_LINUX || LL_SOLARIS
#include <sched.h>
#endif


#ifdef LL_WINDOWS
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void set_thread_name( DWORD dwThreadID, const char* threadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		::RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
#endif


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

U32 LL_THREAD_LOCAL sThreadID = 0;

U32 LLThread::sIDIter = 0;


LL_COMMON_API void assert_main_thread()
{
	static U32 s_thread_id = LLThread::currentID();
	if (LLThread::currentID() != s_thread_id)
	{
		LL_WARNS() << "Illegal execution from thread id " << (S32) LLThread::currentID()
			<< " outside main thread " << (S32) s_thread_id << LL_ENDL;
	}
}

void LLThread::registerThreadID()
{
	sThreadID = ++sIDIter;
}

//
// Handed to the APR thread creation function
//
void *APR_THREAD_FUNC LLThread::staticRun(apr_thread_t *apr_threadp, void *datap)
{
	LLThread *threadp = (LLThread *)datap;

#ifdef LL_WINDOWS
	set_thread_name(-1, threadp->mName.c_str());
#endif

	// for now, hard code all LLThreads to report to single master thread recorder, which is known to be running on main thread
	threadp->mRecorder = new LLTrace::ThreadRecorder(*LLTrace::get_master_thread_recorder());

	sThreadID = threadp->mID;

	// Run the user supplied function
	threadp->run();

	//LL_INFOS() << "LLThread::staticRun() Exiting: " << threadp->mName << LL_ENDL;
	
	delete threadp->mRecorder;
	threadp->mRecorder = NULL;
	
	// We're done with the run function, this thread is done executing now.
	//NB: we are using this flag to sync across threads...we really need memory barriers here
	threadp->mStatus = STOPPED;

	return NULL;
}

LLThread::LLThread(const std::string& name, apr_pool_t *poolp) :
	mPaused(FALSE),
	mName(name),
	mAPRThreadp(NULL),
	mStatus(STOPPED),
	mRecorder(NULL)
{

	mID = ++sIDIter;

	// Thread creation probably CAN be paranoid about APR being initialized, if necessary
	if (poolp)
	{
		mIsLocalPool = FALSE;
		mAPRPoolp = poolp;
	}
	else
	{
		mIsLocalPool = TRUE;
		apr_pool_create(&mAPRPoolp, NULL); // Create a subpool for this thread
	}
	mRunCondition = new LLCondition(mAPRPoolp);
	mDataLock = new LLMutex(mAPRPoolp);
	mLocalAPRFilePoolp = NULL ;
}


LLThread::~LLThread()
{
	shutdown();

	if(mLocalAPRFilePoolp)
	{
		delete mLocalAPRFilePoolp ;
		mLocalAPRFilePoolp = NULL ;
	}
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

			//LL_INFOS() << "LLThread::~LLThread() Killing thread " << mName << " Status: " << mStatus << LL_ENDL;
			// Now wait a bit for the thread to exit
			// It's unclear whether I should even bother doing this - this destructor
			// should never get called unless we're already stopped, really...
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
			//LL_WARNS() << "LLThread::~LLThread() exiting thread before clean exit!" << LL_ENDL;
			// Put a stake in its heart.
			delete mRecorder;

			apr_thread_exit(mAPRThreadp, -1);
			return;
		}
		mAPRThreadp = NULL;
	}

	delete mRunCondition;
	mRunCondition = NULL;

	delete mDataLock;
	mDataLock = NULL;
	
	if (mIsLocalPool && mAPRPoolp)
	{
		apr_pool_destroy(mAPRPoolp);
		mAPRPoolp = 0;
	}

	if (mRecorder)
	{
		// missed chance to properly shut down recorder (needs to be done in thread context)
		// probably due to abnormal thread termination
		// so just leak it and remove it from parent
		LLTrace::get_master_thread_recorder()->removeChildRecorder(mRecorder);
	}
}


void LLThread::start()
{
	llassert(isStopped());
	
	// Set thread state to running
	mStatus = RUNNING;

	apr_status_t status =
		apr_thread_create(&mAPRThreadp, NULL, staticRun, (void *)this, mAPRPoolp);
	
	if(status == APR_SUCCESS)
	{	
		// We won't bother joining
		apr_thread_detach(mAPRThreadp);
	}
	else
	{
		mStatus = STOPPED;
		LL_WARNS() << "failed to start thread " << mName << LL_ENDL;
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
		mPaused = 1;		// Does not need to be atomic since this is only set/unset from the main thread
	}	
}

void LLThread::unpause()
{
	if (mPaused)
	{
		mPaused = 0;
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
	mDataLock->lock();

	// This is in a while loop because the pthread API allows for spurious wakeups.
	while(shouldSleep())
	{
		mDataLock->unlock();
		mRunCondition->wait(); // unlocks mRunCondition
		mDataLock->lock();
		// mRunCondition is locked when the thread wakes up
	}
	
 	mDataLock->unlock();
}

//============================================================================

void LLThread::setQuitting()
{
	mDataLock->lock();
	if (mStatus == RUNNING)
	{
		mStatus = QUITTING;
	}
	mDataLock->unlock();
	wake();
}

// static
U32 LLThread::currentID()
{
	return sThreadID;
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
	mDataLock->lock();
	if(!shouldSleep())
	{
		mRunCondition->signal();
	}
	mDataLock->unlock();
}

void LLThread::wakeLocked()
{
	if(!shouldSleep())
	{
		mRunCondition->signal();
	}
}

//============================================================================

//----------------------------------------------------------------------------

//static
LLMutex* LLThreadSafeRefCount::sMutex = 0;

//static
void LLThreadSafeRefCount::initThreadSafeRefCount()
{
	if (!sMutex)
	{
		sMutex = new LLMutex(0);
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

LLThreadSafeRefCount::LLThreadSafeRefCount(const LLThreadSafeRefCount& src)
{
	mRef = 0;
}

LLThreadSafeRefCount::~LLThreadSafeRefCount()
{ 
	if (mRef != 0)
	{
		LL_ERRS() << "deleting non-zero reference" << LL_ENDL;
	}
}

//============================================================================

LLResponder::~LLResponder()
{
}

//============================================================================
