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

#include "llmutex.h"
#include "llthread.h"
#include "lltimer.h"

//============================================================================

LLMutex::LLMutex() :
 mCount(0)
{
}


LLMutex::~LLMutex()
{
}


void LLMutex::lock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	if(isSelfLocked())
	{ //redundant lock
		mCount++;
		return;
	}
	
	mMutex.lock();
	
#if MUTEX_DEBUG
	// Have to have the lock before we can access the debug info
	auto id = LLThread::currentID();
	if (mIsLocked[id] != FALSE)
		LL_ERRS() << "Already locked in Thread: " << id << LL_ENDL;
	mIsLocked[id] = TRUE;
#endif

	mLockingThread = LLThread::currentID();
}

void LLMutex::unlock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	if (mCount > 0)
	{ //not the root unlock
		mCount--;
		return;
	}
	
#if MUTEX_DEBUG
	// Access the debug info while we have the lock
	auto id = LLThread::currentID();
	if (mIsLocked[id] != TRUE)
		LL_ERRS() << "Not locked in Thread: " << id << LL_ENDL;	
	mIsLocked[id] = FALSE;
#endif

	mLockingThread = LLThread::id_t();
	mMutex.unlock();
}

bool LLMutex::isLocked()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	if (!mMutex.try_lock())
	{
		return true;
	}
	else
	{
		mMutex.unlock();
		return false;
	}
}

bool LLMutex::isSelfLocked()
{
	return mLockingThread == LLThread::currentID();
}

LLThread::id_t LLMutex::lockingThread() const
{
	return mLockingThread;
}

bool LLMutex::trylock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	if(isSelfLocked())
	{ //redundant lock
		mCount++;
		return true;
	}
	
	if (!mMutex.try_lock())
	{
		return false;
	}
	
#if MUTEX_DEBUG
	// Have to have the lock before we can access the debug info
	auto id = LLThread::currentID();
	if (mIsLocked[id] != FALSE)
		LL_ERRS() << "Already locked in Thread: " << id << LL_ENDL;
	mIsLocked[id] = TRUE;
#endif

	mLockingThread = LLThread::currentID();
	return true;
}

//============================================================================

LLSharedMutex::LLSharedMutex()
: mShared(false)
{
}

LLSharedMutex::~LLSharedMutex()
{
}

bool LLSharedMutex::isLocked() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLMutexLock lock(const_cast<LLMutex*>(&mLockMutex));

    return !mLockingThreads.empty();
}

bool LLSharedMutex::isThreadLocked() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLThread::id_t current_thread = LLThread::currentID();
    LLMutexLock lock(const_cast<LLMutex*>(&mLockMutex));

    const_iterator it = mLockingThreads.find(current_thread);
    return it != mLockingThreads.end();
}

void LLSharedMutex::lock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLThread::id_t current_thread = LLThread::currentID();
    LLMutexLock lock(&mLockMutex);

    if (mLockingThreads.size() == 1 && mLockingThreads.begin()->first == current_thread)
    {
        mLockingThreads.begin()->second++;
    }
    else
    {
        // Acquire the mutex immediately if mLockingThreads is empty
        // or enter a locking state if mLockingThreads is not empty
        lock.unlock();
        mMutex.lock();
        lock.lock();
        // Continue after acquiring the mutex (and possible quitting the locking state)
        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mShared = false;
    }
}

void LLSharedMutex::lockShared()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLThread::id_t current_thread = LLThread::currentID();
    LLMutexLock lock(&mLockMutex);

    iterator it = mLockingThreads.find(current_thread);
    if (it != mLockingThreads.end())
    {
        it->second++;
    }
    else
    {
        // Acquire the mutex immediately if the mutex is not locked exclusively
        // or enter a locking state if the mutex is already locked exclusively
        lock.unlock();
        mMutex.lock_shared();
        lock.lock();
        // Continue after acquiring the mutex
        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mShared = true;
    }
}

bool LLSharedMutex::trylock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLThread::id_t current_thread = LLThread::currentID();
    LLMutexLock lock(&mLockMutex);

    if (mLockingThreads.size() == 1 && mLockingThreads.begin()->first == current_thread)
    {
        mLockingThreads.begin()->second++;
    }
    else
    {
        if (!mMutex.try_lock())
            return false;

        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mShared = false;
    }

    return true;
}

bool LLSharedMutex::trylockShared()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLThread::id_t current_thread = LLThread::currentID();
    LLMutexLock lock(&mLockMutex);

    iterator it = mLockingThreads.find(current_thread);
    if (it != mLockingThreads.end())
    {
        it->second++;
    }
    else
    {
        if (!mMutex.try_lock_shared())
            return false;

        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mShared = true;
    }

    return true;
}

void LLSharedMutex::unlock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLThread::id_t current_thread = LLThread::currentID();
    LLMutexLock lock(const_cast<LLMutex*>(&mLockMutex));

    iterator it = mLockingThreads.find(current_thread);
    if (it != mLockingThreads.end())
    {
        if (it->second > 1)
        {
            it->second--;
        }
        else
        {
            mLockingThreads.erase(it);
            mMutex.unlock();
        }
    }
}

void LLSharedMutex::unlockShared()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    LLThread::id_t current_thread = LLThread::currentID();
    LLMutexLock lock(const_cast<LLMutex*>(&mLockMutex));

    iterator it = mLockingThreads.find(current_thread);
    if (it != mLockingThreads.end())
    {
        if (it->second > 1)
        {
            it->second--;
        }
        else
        {
            mLockingThreads.erase(it);
            mMutex.unlock_shared();
        }
    }
}


//============================================================================

LLCondition::LLCondition() :
	LLMutex()
{
}


LLCondition::~LLCondition()
{
}


void LLCondition::wait()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	std::unique_lock< std::mutex > lock(mMutex);
	mCond.wait(lock);
}

void LLCondition::signal()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	mCond.notify_one();
}

void LLCondition::broadcast()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	mCond.notify_all();
}



LLMutexTrylock::LLMutexTrylock(LLMutex* mutex)
    : mMutex(mutex),
    mLocked(false)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    if (mMutex)
        mLocked = mMutex->trylock();
}

LLMutexTrylock::LLMutexTrylock(LLMutex* mutex, U32 aTries, U32 delay_ms)
    : mMutex(mutex),
    mLocked(false)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    if (!mMutex)
        return;

    for (U32 i = 0; i < aTries; ++i)
    {
        mLocked = mMutex->trylock();
        if (mLocked)
            break;
        ms_sleep(delay_ms);
    }
}

LLMutexTrylock::~LLMutexTrylock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
    if (mMutex && mLocked)
        mMutex->unlock();
}


//---------------------------------------------------------------------
//
// LLScopedLock
//
LLScopedLock::LLScopedLock(std::mutex* mutex) : mMutex(mutex)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	if(mutex)
	{
		mutex->lock();
		mLocked = true;
	}
	else
	{
		mLocked = false;
	}
}

LLScopedLock::~LLScopedLock()
{
	unlock();
}

void LLScopedLock::unlock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	if(mLocked)
	{
		mMutex->unlock();
		mLocked = false;
	}
}

//============================================================================
