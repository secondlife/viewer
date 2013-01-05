/** 
 * @file llmutex.h
 * @brief Base classes for mutex and condition handling.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLMUTEX_H
#define LL_LLMUTEX_H

#include "llapr.h"
#include "apr_thread_cond.h"

//============================================================================

#define MUTEX_DEBUG (LL_DEBUG || LL_RELEASE_WITH_DEBUG_INFO)

class LL_COMMON_API LLMutex
{
public:
	typedef enum
	{
		NO_THREAD = 0xFFFFFFFF
	} e_locking_thread;

	LLMutex(apr_pool_t *apr_poolp = NULL); // NULL pool constructs a new pool for the mutex
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

#endif // LL_LLTHREAD_H
