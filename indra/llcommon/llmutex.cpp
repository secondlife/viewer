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
#include "llcoros.h"


//---------------------------------------------------------------------
//
// LLMutex
//
LLMutex::LLMutex() :
 mCount(0)
{
}

LLMutex::~LLMutex()
{
}

void LLMutex::lock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;

    // LLMutex is not coroutine aware and should not be used from a coroutine
    // If your code is running in a coroutine, you should use LLCoros::Mutex instead
    // NOTE:  If the stack trace you're staring at contains non-thread-safe code,
    // you should use LLAppViewer::instance().postToMainThread() to shuttle execution
    // back to the main loop.
    // NOTE: If you got here from seeing this assert in your log and you're not seeing
    // a stack trace that points here, put a breakpoint in on_main_coro and try again.
    llassert(LLCoros::on_main_coro());

    if(isSelfLocked())
    { //redundant lock
        mCount++;
        return;
    }

    mMutex.lock();

#if MUTEX_DEBUG
    // Have to have the lock before we can access the debug info
    auto id = LLThread::currentID();
    if (mIsLocked[id])
        LL_ERRS() << "Already locked in Thread: " << id << LL_ENDL;
    mIsLocked[id] = true;
#endif

    mLockingThread = LLThread::currentID();
}

void LLMutex::unlock()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;

    if (mCount > 0)
    { //not the root unlock
        mCount--;
        return;
    }

#if MUTEX_DEBUG
    // Access the debug info while we have the lock
    auto id = LLThread::currentID();
    if (!mIsLocked[id])
        LL_ERRS() << "Not locked in Thread: " << id << LL_ENDL;
    mIsLocked[id] = false;
#endif

    mLockingThread = LLThread::id_t();
    mMutex.unlock();
}

bool LLMutex::isLocked()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    if (isSelfLocked())
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
    if (mIsLocked[id])
        LL_ERRS() << "Already locked in Thread: " << id << LL_ENDL;
    mIsLocked[id] = true;
#endif

    mLockingThread = LLThread::currentID();
    return true;
}


//---------------------------------------------------------------------
//
// LLSharedMutex
//
LLSharedMutex::LLSharedMutex()
: mLockingThreads(2) // Reserve 2 slots in the map hash table
, mIsShared(false)
{
}

bool LLSharedMutex::isLocked() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    std::lock_guard<std::mutex> lock(mLockMutex);

    return !mLockingThreads.empty();
}

bool LLSharedMutex::isThreadLocked() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    LLThread::id_t current_thread = LLThread::currentID();
    std::lock_guard<std::mutex> lock(mLockMutex);

    const_iterator it = mLockingThreads.find(current_thread);
    return it != mLockingThreads.end();
}

void LLSharedMutex::lockShared()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    LLThread::id_t current_thread = LLThread::currentID();

    mLockMutex.lock();
    iterator it = mLockingThreads.find(current_thread);
    if (it != mLockingThreads.end())
    {
        it->second++;
    }
    else
    {
        // Acquire the mutex immediately if the mutex is not locked exclusively
        // or enter a locking state if the mutex is already locked exclusively
        mLockMutex.unlock();
        mSharedMutex.lock_shared();
        mLockMutex.lock();
        // Continue after acquiring the mutex
        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mIsShared = true;
    }
    mLockMutex.unlock();
}

void LLSharedMutex::lockExclusive()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    LLThread::id_t current_thread = LLThread::currentID();

    mLockMutex.lock();
    iterator it = mLockingThreads.find(current_thread);
    if (it != mLockingThreads.end())
    {
        if (mIsShared)
        {
            // The mutex is already locked in the current thread
            // but this lock is SHARED (not EXCLISIVE)
            // We can't lock it again, the lock stays shared
            // This can lead to a collision (theoretically)
            llassert_always(!"The current thread is already locked SHARED and can't be locked EXCLUSIVE");
        }
        it->second++;
    }
    else
    {
        // Acquire the mutex immediately if mLockingThreads is empty
        // or enter a locking state if mLockingThreads is not empty
        mLockMutex.unlock();
        mSharedMutex.lock();
        mLockMutex.lock();
        // Continue after acquiring the mutex (and possible quitting the locking state)
        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mIsShared = false;
    }
    mLockMutex.unlock();
}

bool LLSharedMutex::trylockShared()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    LLThread::id_t current_thread = LLThread::currentID();
    std::lock_guard<std::mutex> lock(mLockMutex);

    iterator it = mLockingThreads.find(current_thread);
    if (it != mLockingThreads.end())
    {
        it->second++;
    }
    else
    {
        if (!mSharedMutex.try_lock_shared())
            return false;

        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mIsShared = true;
    }

    return true;
}

bool LLSharedMutex::trylockExclusive()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    LLThread::id_t current_thread = LLThread::currentID();
    std::lock_guard<std::mutex> lock(mLockMutex);

    if (mLockingThreads.size() == 1 && mLockingThreads.begin()->first == current_thread)
    {
        mLockingThreads.begin()->second++;
    }
    else
    {
        if (!mSharedMutex.try_lock())
            return false;

        mLockingThreads.emplace(std::make_pair(current_thread, 1));
        mIsShared = false;
    }

    return true;
}

void LLSharedMutex::unlockShared()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    LLThread::id_t current_thread = LLThread::currentID();
    std::lock_guard<std::mutex> lock(mLockMutex);

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
            mSharedMutex.unlock_shared();
        }
    }
}

void LLSharedMutex::unlockExclusive()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    LLThread::id_t current_thread = LLThread::currentID();
    std::lock_guard<std::mutex> lock(mLockMutex);

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
            mSharedMutex.unlock();
        }
    }
}


//---------------------------------------------------------------------
//
// LLCondition
//
LLCondition::LLCondition() :
    LLMutex()
{
}

LLCondition::~LLCondition()
{
}

void LLCondition::wait()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    std::unique_lock< std::mutex > lock(mMutex);
    mCond.wait(lock);
}

void LLCondition::signal()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    mCond.notify_one();
}

void LLCondition::broadcast()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    mCond.notify_all();
}


//---------------------------------------------------------------------
//
// LLMutexTrylock
//
LLMutexTrylock::LLMutexTrylock(LLMutex* mutex)
    : mMutex(mutex),
    mLocked(false)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    if (mMutex)
        mLocked = mMutex->trylock();
}

LLMutexTrylock::LLMutexTrylock(LLMutex* mutex, U32 aTries, U32 delay_ms)
    : mMutex(mutex),
    mLocked(false)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    if (mMutex && mLocked)
        mMutex->unlock();
}


//---------------------------------------------------------------------
//
// LLScopedLock
//
LLScopedLock::LLScopedLock(std::mutex* mutex) : mMutex(mutex)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    if(mLocked)
    {
        mMutex->unlock();
        mLocked = false;
    }
}

//============================================================================
