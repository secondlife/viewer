/** 
 * @file llthread.h
 * @brief Base classes for thread, mutex and condition handling.
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

#ifndef LL_LLTHREAD_H
#define LL_LLTHREAD_H

#include "llapp.h"
#include "llapr.h"
#include "apr_thread_cond.h"

class LLThread;
class LLMutex;
class LLCondition;

#if LL_WINDOWS
#define ll_thread_local __declspec(thread)
#else
#define ll_thread_local __thread
#endif

class LL_COMMON_API LLThread
{
private:
	static U32 sIDIter;

public:
	typedef enum e_thread_status
	{
		STOPPED = 0,	// The thread is not running.  Not started, or has exited its run function
		RUNNING = 1,	// The thread is currently running
		QUITTING= 2 	// Someone wants this thread to quit
	} EThreadStatus;

	LLThread(const std::string& name, apr_pool_t *poolp = NULL);
	virtual ~LLThread(); // Warning!  You almost NEVER want to destroy a thread unless it's in the STOPPED state.
	virtual void shutdown(); // stops the thread
	
	bool isQuitting() const { return (QUITTING == mStatus); }
	bool isStopped() const { return (STOPPED == mStatus); }
	
	static U32 currentID(); // Return ID of current thread
	static void yield(); // Static because it can be called by the main thread, which doesn't have an LLThread data structure.
	
public:
	// PAUSE / RESUME functionality. See source code for important usage notes.
	// Called from MAIN THREAD.
	void pause();
	void unpause();
	bool isPaused() { return isStopped() || mPaused == TRUE; }
	
	// Cause the thread to wake up and check its condition
	void wake();

	// Same as above, but to be used when the condition is already locked.
	void wakeLocked();

	// Called from run() (CHILD THREAD). Pause the thread if requested until unpaused.
	void checkPause();

	// this kicks off the apr thread
	void start(void);

	apr_pool_t *getAPRPool() { return mAPRPoolp; }
	LLVolatileAPRPool* getLocalAPRFilePool() { return mLocalAPRFilePoolp ; }

	U32 getID() const { return mID; }

private:
	BOOL				mPaused;
	
	// static function passed to APR thread creation routine
	static void *APR_THREAD_FUNC staticRun(apr_thread_t *apr_threadp, void *datap);

protected:
	std::string			mName;
	LLCondition*		mRunCondition;

	apr_thread_t		*mAPRThreadp;
	apr_pool_t			*mAPRPoolp;
	BOOL				mIsLocalPool;
	EThreadStatus		mStatus;
	U32					mID;

	//a local apr_pool for APRFile operations in this thread. If it exists, LLAPRFile::sAPRFilePoolp should not be used.
	//Note: this pool is used by APRFile ONLY, do NOT use it for any other purposes.
	//      otherwise it will cause severe memory leaking!!! --bao
	LLVolatileAPRPool  *mLocalAPRFilePoolp ; 

	void setQuitting();
	
	// virtual function overridden by subclass -- this will be called when the thread runs
	virtual void run(void) = 0; 
	
	// virtual predicate function -- returns true if the thread should wake up, false if it should sleep.
	virtual bool runCondition(void);

	// Lock/Unlock Run Condition -- use around modification of any variable used in runCondition()
	inline void lockData();
	inline void unlockData();
	
	// This is the predicate that decides whether the thread should sleep.  
	// It should only be called with mRunCondition locked, since the virtual runCondition() function may need to access
	// data structures that are thread-unsafe.
	bool shouldSleep(void) { return (mStatus == RUNNING) && (isPaused() || (!runCondition())); }

	// To avoid spurious signals (and the associated context switches) when the condition may or may not have changed, you can do the following:
	// mRunCondition->lock();
	// if(!shouldSleep())
	//     mRunCondition->signal();
	// mRunCondition->unlock();
};

//============================================================================

#define MUTEX_DEBUG (LL_DEBUG || LL_RELEASE_WITH_DEBUG_INFO)

class LL_COMMON_API LLMutex
{
public:
	typedef enum
	{
		NO_THREAD = 0xFFFFFFFF
	} e_locking_thread;

	LLMutex(apr_pool_t *apr_poolp); // NULL pool constructs a new pool for the mutex
	virtual ~LLMutex();
	
	void lock();		// blocks
	void unlock();
	bool isLocked(); 	// non-blocking, but does do a lock/unlock so not free
	bool isSelfLocked(); //return true if locked in a same thread
	U32 lockingThread() const; //get ID of locking thread
	
protected:
	apr_thread_mutex_t *mAPRMutexp;
	mutable U32			mCount;
	mutable U32			mLockingThread;
	
	apr_pool_t			*mAPRPoolp;
	BOOL				mIsLocalPool;
	
#if MUTEX_DEBUG
	std::map<U32, BOOL> mIsLocked;
#endif
};

// Actually a condition/mutex pair (since each condition needs to be associated with a mutex).
class LL_COMMON_API LLCondition : public LLMutex
{
public:
	LLCondition(apr_pool_t *apr_poolp); // Defaults to global pool, could use the thread pool as well.
	~LLCondition();
	
	void wait();		// blocks
	void signal();
	void broadcast();
	
protected:
	apr_thread_cond_t *mAPRCondp;
};

class LLMutexLock
{
public:
	LLMutexLock(LLMutex* mutex)
	{
		mMutex = mutex;
		
		if(mMutex)
			mMutex->lock();
	}
	~LLMutexLock()
	{
		if(mMutex)
			mMutex->unlock();
	}
private:
	LLMutex* mMutex;
};

//============================================================================

void LLThread::lockData()
{
	mRunCondition->lock();
}

void LLThread::unlockData()
{
	mRunCondition->unlock();
}


//============================================================================

// see llmemory.h for LLPointer<> definition

class LL_COMMON_API LLThreadSafeRefCount
{
public:
	static void initThreadSafeRefCount(); // creates sMutex
	static void cleanupThreadSafeRefCount(); // destroys sMutex
	
private:
	static LLMutex* sMutex;

private:
	LLThreadSafeRefCount(const LLThreadSafeRefCount&); // not implemented
	LLThreadSafeRefCount&operator=(const LLThreadSafeRefCount&); // not implemented

protected:
	virtual ~LLThreadSafeRefCount(); // use unref()
	
public:
	LLThreadSafeRefCount();
	
	void ref()
	{
		if (sMutex) sMutex->lock();
		mRef++; 
		if (sMutex) sMutex->unlock();
	} 

	S32 unref()
	{
		llassert(mRef >= 1);
		if (sMutex) sMutex->lock();
		S32 res = --mRef;
		if (sMutex) sMutex->unlock();
		if (0 == res) 
		{
			delete this; 
			return 0;
		}
		return res;
	}	
	S32 getNumRefs() const
	{
		return mRef;
	}

private: 
	S32	mRef; 
};

//============================================================================

// Simple responder for self destructing callbacks
// Pure virtual class
class LL_COMMON_API LLResponder : public LLThreadSafeRefCount
{
protected:
	virtual ~LLResponder();
public:
	virtual void completed(bool success) = 0;
};

//============================================================================

#endif // LL_LLTHREAD_H
