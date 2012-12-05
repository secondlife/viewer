/** 
 * @file llmutex.cpp
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

#include "llmutex.h"
#include "llthread.h"

//============================================================================

LLMutex::LLMutex(apr_pool_t *poolp) :
	mAPRMutexp(NULL), mCount(0), mLockingThread(NO_THREAD)
{
	//if (poolp)
	//{
	//	mIsLocalPool = FALSE;
	//	mAPRPoolp = poolp;
	//}
	//else
	{
		mIsLocalPool = TRUE;
		apr_pool_create(&mAPRPoolp, NULL); // Create a subpool for this thread
	}
	apr_thread_mutex_create(&mAPRMutexp, APR_THREAD_MUTEX_UNNESTED, mAPRPoolp);
}


LLMutex::~LLMutex()
{
#if MUTEX_DEBUG
	//bad assertion, the subclass LLSignal might be "locked", and that's OK
	//llassert_always(!isLocked()); // better not be locked!
#endif
	apr_thread_mutex_destroy(mAPRMutexp);
	mAPRMutexp = NULL;
	if (mIsLocalPool)
	{
		apr_pool_destroy(mAPRPoolp);
	}
}


void LLMutex::lock()
{
	if(isSelfLocked())
	{ //redundant lock
		mCount++;
		return;
	}
	
	apr_thread_mutex_lock(mAPRMutexp);
	
#if MUTEX_DEBUG
	// Have to have the lock before we can access the debug info
	U32 id = LLThread::currentID();
	if (mIsLocked[id] != FALSE)
		llerrs << "Already locked in Thread: " << id << llendl;
	mIsLocked[id] = TRUE;
#endif

	mLockingThread = LLThread::currentID();
}

void LLMutex::unlock()
{
	if (mCount > 0)
	{ //not the root unlock
		mCount--;
		return;
	}
	
#if MUTEX_DEBUG
	// Access the debug info while we have the lock
	U32 id = LLThread::currentID();
	if (mIsLocked[id] != TRUE)
		llerrs << "Not locked in Thread: " << id << llendl;	
	mIsLocked[id] = FALSE;
#endif

	mLockingThread = NO_THREAD;
	apr_thread_mutex_unlock(mAPRMutexp);
}

bool LLMutex::isLocked()
{
	apr_status_t status = apr_thread_mutex_trylock(mAPRMutexp);
	if (APR_STATUS_IS_EBUSY(status))
	{
		return true;
	}
	else
	{
		apr_thread_mutex_unlock(mAPRMutexp);
		return false;
	}
}

bool LLMutex::isSelfLocked()
{
	return mLockingThread == LLThread::currentID();
}

U32 LLMutex::lockingThread() const
{
	return mLockingThread;
}

//============================================================================

LLCondition::LLCondition(apr_pool_t *poolp) :
	LLMutex(poolp)
{
	// base class (LLMutex) has already ensured that mAPRPoolp is set up.

	apr_thread_cond_create(&mAPRCondp, mAPRPoolp);
}


LLCondition::~LLCondition()
{
	apr_thread_cond_destroy(mAPRCondp);
	mAPRCondp = NULL;
}


void LLCondition::wait()
{
	if (!isLocked())
	{ //mAPRMutexp MUST be locked before calling apr_thread_cond_wait
		apr_thread_mutex_lock(mAPRMutexp);
#if MUTEX_DEBUG
		// avoid asserts on destruction in non-release builds
		U32 id = LLThread::currentID();
		mIsLocked[id] = TRUE;
#endif
	}
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
