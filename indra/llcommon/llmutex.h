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

#include "stdtypes.h"
#include <boost/noncopyable.hpp>
#include "mutex.h"
#include <condition_variable>

//============================================================================

class LL_COMMON_API LLMutex
{
public:
	using mutex_t = std::recursive_mutex;

	LLMutex();
	virtual ~LLMutex();

	void lock();		// blocks
	bool trylock();		// non-blocking, returns true if lock held.
	void unlock();		// undefined behavior when called on mutex not being held
	bool isLocked(); 	// non-blocking, but does do a lock/unlock so not free

protected:
	mutex_t					mMutex;
};

// Actually a condition/mutex pair (since each condition needs to be associated with a mutex).
class LL_COMMON_API LLCondition : public LLMutex
{
public:
	LLCondition();
	~LLCondition();

	void wait();		// blocks
	void signal();
	void broadcast();

protected:
	std::condition_variable_any mCond;
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

// Scoped locking class similar in function to LLMutexLock but uses
// the trylock() method to conditionally acquire lock without
// blocking.  Caller resolves the resulting condition by calling
// the isLocked() method and either punts or continues as indicated.
//
// Mostly of interest to callers needing to avoid stalls who can
// guarantee another attempt at a later time.

class LLMutexTrylock
{
public:
	LLMutexTrylock(LLMutex* mutex);
	LLMutexTrylock(LLMutex* mutex, U32 aTries, U32 delay_ms = 10);
	~LLMutexTrylock();

	bool isLocked() const
	{
		return mLocked;
	}
	
private:
	LLMutex*	mMutex;
	bool		mLocked;
};

/**
* @class LLScopedLockFor
* @brief Small class to help lock and unlock mutexes.
*
* The constructor handles the lock, and the destructor handles
* the unlock. Instances of this class are <b>not</b> thread safe.
*/
template <class MUTEX>
class LL_COMMON_API LLScopedLockFor : private boost::noncopyable
{
public:
    /**
    * @brief Constructor which accepts a mutex, and locks it.
    *
    * @param mutex An allocated mutex. If you pass in NULL,
    * this wrapper will not lock.
    */
    LLScopedLockFor(MUTEX* mutex);

    /**
    * @brief Destructor which unlocks the mutex if still locked.
    */
    ~LLScopedLockFor();

    /**
    * @brief Check lock.
    */
    bool isLocked() const { return mLocked; }

    /**
    * @brief This method unlocks the mutex.
    */
    void unlock();

protected:
    bool mLocked;
    MUTEX* mMutex;
};

//---------------------------------------------------------------------
//
// LLScopedLockFor
//
template <class MUTEX>
LLScopedLockFor<MUTEX>::LLScopedLockFor(MUTEX* mutex) : mMutex(mutex)
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

template <class MUTEX>
LLScopedLockFor<MUTEX>::~LLScopedLockFor()
{
	unlock();
}

template <class MUTEX>
void LLScopedLockFor<MUTEX>::unlock()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD
	if(mLocked)
	{
		mMutex->unlock();
		mLocked = false;
	}
}

// In C++17, LLScopedLockFor() should be able to infer the MUTEX template
// param from its constructor argument. Until then...
using LLScopedLock = LLScopedLockFor<std::mutex>;

#endif // LL_LLMUTEX_H
