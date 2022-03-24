/** 
 * @file llthreadsafequeue.h
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

#ifndef LL_LLTHREADSAFEQUEUE_H
#define LL_LLTHREADSAFEQUEUE_H

#include "llexception.h"
#include <deque>
#include <string>
#include <chrono>
#include "mutex.h"
#include "llcoros.h"
#include LLCOROS_MUTEX_HEADER
#include <boost/fiber/timed_mutex.hpp>
#include LLCOROS_CONDVAR_HEADER

//
// A general queue exception.
//
class LL_COMMON_API LLThreadSafeQueueError:
	public LLException
{
public:
	LLThreadSafeQueueError(std::string const & message):
		LLException(message)
	{
		; // No op.
	}
};


//
// An exception raised when blocking operations are interrupted.
//
class LL_COMMON_API LLThreadSafeQueueInterrupt:
	public LLThreadSafeQueueError
{
public:
	LLThreadSafeQueueInterrupt(void):
		LLThreadSafeQueueError("queue operation interrupted")
	{
		; // No op.
	}
};

//
// Implements a thread safe FIFO.
//
template<typename ElementT>
class LLThreadSafeQueue
{
public:
	typedef ElementT value_type;
	
	// If the pool is set to NULL one will be allocated and managed by this
	// queue.
	LLThreadSafeQueue(U32 capacity = 1024);
	
	// Add an element to the front of queue (will block if the queue has
	// reached capacity).
	//
	// This call will raise an interrupt error if the queue is closed while
	// the caller is blocked.
	void pushFront(ElementT const & element);
	
	// Try to add an element to the front of queue without blocking. Returns
	// true only if the element was actually added.
	bool tryPushFront(ElementT const & element);

	// Try to add an element to the front of queue, blocking if full but with
	// timeout. Returns true if the element was added.
	// There are potentially two different timeouts involved: how long to try
	// to lock the mutex, versus how long to wait for the queue to stop being
	// full. Careful settings for each timeout might be orders of magnitude
	// apart. However, this method conflates them.
	template <typename Rep, typename Period>
	bool tryPushFrontFor(const std::chrono::duration<Rep, Period>& timeout,
						 ElementT const & element);

	// Pop the element at the end of the queue (will block if the queue is
	// empty).
	//
	// This call will raise an interrupt error if the queue is closed while
	// the caller is blocked.
	ElementT popBack(void);
	
	// Pop an element from the end of the queue if there is one available.
	// Returns true only if an element was popped.
	bool tryPopBack(ElementT & element);
	
	// Returns the size of the queue.
	size_t size();

	// closes the queue:
	// - every subsequent pushFront() call will throw LLThreadSafeQueueInterrupt
	// - every subsequent tryPushFront() call will return false
	// - popBack() calls will return normally until the queue is drained, then
	//   every subsequent popBack() will throw LLThreadSafeQueueInterrupt
	// - tryPopBack() calls will return normally until the queue is drained,
	//   then every subsequent tryPopBack() call will return false
	void close();

	// detect closed state
	bool isClosed();
	// inverse of isClosed()
	explicit operator bool();

private:
	std::deque< ElementT > mStorage;
	U32 mCapacity;
	bool mClosed;

	boost::fibers::timed_mutex mLock;
	typedef std::unique_lock<decltype(mLock)> lock_t;
	boost::fibers::condition_variable_any mCapacityCond;
	boost::fibers::condition_variable_any mEmptyCond;
};

// LLThreadSafeQueue
//-----------------------------------------------------------------------------

template<typename ElementT>
LLThreadSafeQueue<ElementT>::LLThreadSafeQueue(U32 capacity) :
    mCapacity(capacity),
    mClosed(false)
{
}


template<typename ElementT>
void LLThreadSafeQueue<ElementT>::pushFront(ElementT const & element)
{
    lock_t lock1(mLock);
    while (true)
    {
        if (mClosed)
        {
            LLTHROW(LLThreadSafeQueueInterrupt());
        }

        if (mStorage.size() < mCapacity)
        {
            mStorage.push_front(element);
            lock1.unlock();
            mEmptyCond.notify_one();
            return;
        }

        // Storage Full. Wait for signal.
        mCapacityCond.wait(lock1);
    }
}


template <typename ElementT>
template <typename Rep, typename Period>
bool LLThreadSafeQueue<ElementT>::tryPushFrontFor(const std::chrono::duration<Rep, Period>& timeout,
                                                  ElementT const & element)
{
    // Convert duration to time_point: passing the same timeout duration to
    // each of multiple calls is wrong.
    auto endpoint = std::chrono::steady_clock::now() + timeout;

    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock_until(endpoint))
        return false;

    while (true)
    {
        if (mClosed)
        {
            return false;
        }

        if (mStorage.size() < mCapacity)
        {
            mStorage.push_front(element);
            lock1.unlock();
            mEmptyCond.notify_one();
            return true;
        }

        // Storage Full. Wait for signal.
        if (LLCoros::cv_status::timeout == mCapacityCond.wait_until(lock1, endpoint))
        {
            // timed out -- formally we might recheck both conditions above
            return false;
        }
        // If we didn't time out, we were notified for some reason. Loop back
        // to check.
    }
}


template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPushFront(ElementT const & element)
{
    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock())
        return false;

    if (mClosed)
        return false;

    if (mStorage.size() >= mCapacity)
        return false;

    mStorage.push_front(element);
    lock1.unlock();
    mEmptyCond.notify_one();
    return true;
}


template<typename ElementT>
ElementT LLThreadSafeQueue<ElementT>::popBack(void)
{
    lock_t lock1(mLock);
    while (true)
    {
        if (!mStorage.empty())
        {
            ElementT value = mStorage.back();
            mStorage.pop_back();
            lock1.unlock();
            mCapacityCond.notify_one();
            return value;
        }

        if (mClosed)
        {
            LLTHROW(LLThreadSafeQueueInterrupt());
        }

        // Storage empty. Wait for signal.
        mEmptyCond.wait(lock1);
    }
}


template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPopBack(ElementT & element)
{
    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock())
        return false;

    // no need to check mClosed: tryPopBack() behavior when the queue is
    // closed is implemented by simple inability to push any new elements
    if (mStorage.empty())
        return false;

    element = mStorage.back();
    mStorage.pop_back();
    lock1.unlock();
    mCapacityCond.notify_one();
    return true;
}


template<typename ElementT>
size_t LLThreadSafeQueue<ElementT>::size(void)
{
    lock_t lock(mLock);
    return mStorage.size();
}

template<typename ElementT>
void LLThreadSafeQueue<ElementT>::close()
{
    lock_t lock(mLock);
    mClosed = true;
    lock.unlock();
    // wake up any blocked popBack() calls
    mEmptyCond.notify_all();
    // wake up any blocked pushFront() calls
    mCapacityCond.notify_all();
}

template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::isClosed()
{
    lock_t lock(mLock);
    return mClosed && mStorage.size() == 0;
}

template<typename ElementT>
LLThreadSafeQueue<ElementT>::operator bool()
{
    return ! isClosed();
}

#endif
