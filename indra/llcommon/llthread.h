/** 
 * @file llthread.h
 * @brief Base classes for thread, mutex and condition handling.
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

#ifndef LL_LLTHREAD_H
#define LL_LLTHREAD_H

#include "llapp.h"
#include "apr_thread_cond.h"

class LLThread;
class LLMutex;
class LLCondition;

class LL_COMMON_API LLThread
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

class LL_COMMON_API LLMutex
{
public:
	LLMutex(apr_pool_t *apr_poolp); // NULL pool constructs a new pool for the mutex
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
