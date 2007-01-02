/** 
 * @file llthread.h
 * @brief Base classes for thread, mutex and condition handling.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTHREAD_H
#define LL_LLTHREAD_H

#include "llapr.h"
#include "llapp.h"

#include "apr-1/apr_thread_cond.h"

class LLThread;
class LLMutex;
class LLCondition;

class LLThread
{
public:
	typedef enum e_thread_status
	{
		STOPPED = 0,	// The thread is not running.  Not started, or has exited its run function
		RUNNING = 1,	// The thread is currently running
		QUITTING= 2 	// Someone wants this thread to quit
	} EThreadStatus;

	LLThread(const std::string& name, apr_pool_t *poolp = NULL);
	virtual ~LLThread(); // Warning!  You almost NEVER want to destroy a thread unless it's in the STOPPED state.
		
	static void yield(); // Static because it can be called by the main thread, which doesn't have an LLThread data structure.


	bool isQuitting() const;
	bool isStopped() const;
	
	// PAUSE / RESUME functionality. See source code for important usage notes.
public:
	// Called from MAIN THREAD.
	void pause();
	void unpause();
	bool isPaused() { return mPaused ? true : false; }
	
	// Cause the thread to wake up and check its condition
	void wake();

	// Same as above, but to be used when the condition is already locked.
	void wakeLocked();

	// Called from run() (CHILD THREAD). Pause the thread if requested until unpaused.
	void checkPause();

	// this kicks off the apr thread
	void start(void);

	apr_pool_t *getAPRPool() { return mAPRPoolp; }
	
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

class LLMutex
{
public:
	LLMutex(apr_pool_t *apr_poolp); // Defaults to global pool, could use the thread pool as well.
	~LLMutex();
	
	void lock();		// blocks
	void unlock();
	bool isLocked(); 	// non-blocking, but does do a lock/unlock so not free
	
protected:
	apr_thread_mutex_t *mAPRMutexp;
	apr_pool_t			*mAPRPoolp;
	BOOL				mIsLocalPool;
};

// Actually a condition/mutex pair (since each condition needs to be associated with a mutex).
class LLCondition : public LLMutex
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
		mMutex->lock();
	}
	~LLMutexLock()
	{
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

#endif // LL_LLTHREAD_H
