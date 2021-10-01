/** 
 * @file llthreadsafequeue.h
 * @brief Queue protected with mutexes for cross-thread use
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

#include "llcoros.h"
#include LLCOROS_MUTEX_HEADER
#include <boost/fiber/timed_mutex.hpp>
#include LLCOROS_CONDVAR_HEADER
#include "llexception.h"
#include "mutex.h"
#include <chrono>
#include <queue>
#include <string>

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

/**
 * Implements a thread safe FIFO.
 */
// Let the default std::queue default to underlying std::deque. Override if
// desired.
template<typename ElementT, typename QueueT=std::queue<ElementT>>
class LLThreadSafeQueue
{
public:
	typedef ElementT value_type;

	// If the pool is set to NULL one will be allocated and managed by this
	// queue.
	LLThreadSafeQueue(U32 capacity = 1024);

	// Add an element to the queue (will block if the queue has
	// reached capacity).
	//
	// This call will raise an interrupt error if the queue is closed while
	// the caller is blocked.
	void push(ElementT const& element);
	// legacy name
	void pushFront(ElementT const & element) { return push(element); }

	// Try to add an element to the queue without blocking. Returns
	// true only if the element was actually added.
	bool tryPush(ElementT const& element);
	// legacy name
	bool tryPushFront(ElementT const & element) { return tryPush(element); }

	// Try to add an element to the queue, blocking if full but with timeout
	// after specified duration. Returns true if the element was added.
	// There are potentially two different timeouts involved: how long to try
	// to lock the mutex, versus how long to wait for the queue to stop being
	// full. Careful settings for each timeout might be orders of magnitude
	// apart. However, this method conflates them.
	template <typename Rep, typename Period>
	bool tryPushFor(const std::chrono::duration<Rep, Period>& timeout,
					ElementT const & element);
	// legacy name
	template <typename Rep, typename Period>
	bool tryPushFrontFor(const std::chrono::duration<Rep, Period>& timeout,
						 ElementT const & element) { return tryPushFor(timeout, element); }

	// Try to add an element to the queue, blocking if full but with
	// timeout at specified time_point. Returns true if the element was added.
	template <typename Clock, typename Duration>
	bool tryPushUntil(const std::chrono::time_point<Clock, Duration>& timeout,
					  ElementT const& element);
	// no legacy name because this is a newer method

	// Pop the element at the head of the queue (will block if the queue is
	// empty).
	//
	// This call will raise an interrupt error if the queue is closed while
	// the caller is blocked.
	ElementT pop(void);
	// legacy name
	ElementT popBack(void) { return pop(); }

	// Pop an element from the head of the queue if there is one available.
	// Returns true only if an element was popped.
	bool tryPop(ElementT & element);
	// legacy name
	bool tryPopBack(ElementT & element) { return tryPop(element); }

	// Pop the element at the head of the queue, blocking if empty, with
	// timeout after specified duration. Returns true if an element was popped.
	template <typename Rep, typename Period>
	bool tryPopFor(const std::chrono::duration<Rep, Period>& timeout, ElementT& element);
	// no legacy name because this is a newer method

	// Pop the element at the head of the queue, blocking if empty, with
	// timeout at specified time_point. Returns true if an element was popped.
	template <typename Clock, typename Duration>
	bool tryPopUntil(const std::chrono::time_point<Clock, Duration>& timeout,
					 ElementT& element);
	// no legacy name because this is a newer method

	// Returns the size of the queue.
	size_t size();

	// closes the queue:
	// - every subsequent push() call will throw LLThreadSafeQueueInterrupt
	// - every subsequent tryPush() call will return false
	// - pop() calls will return normally until the queue is drained, then
	//   every subsequent pop() will throw LLThreadSafeQueueInterrupt
	// - tryPop() calls will return normally until the queue is drained,
	//   then every subsequent tryPop() call will return false
	void close();

	// detect closed state
	bool isClosed();
	// inverse of isClosed()
	explicit operator bool();

protected:
	typedef QueueT queue_type;
	QueueT mStorage;
	U32 mCapacity;
	bool mClosed;

	boost::fibers::timed_mutex mLock;
	typedef std::unique_lock<decltype(mLock)> lock_t;
	boost::fibers::condition_variable_any mCapacityCond;
	boost::fibers::condition_variable_any mEmptyCond;
};

// LLThreadSafeQueue
//-----------------------------------------------------------------------------

template<typename ElementT, typename QueueT>
LLThreadSafeQueue<ElementT, QueueT>::LLThreadSafeQueue(U32 capacity) :
    mCapacity(capacity),
    mClosed(false)
{
}


template<typename ElementT, typename QueueT>
void LLThreadSafeQueue<ElementT, QueueT>::push(ElementT const & element)
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
            mStorage.push(element);
            lock1.unlock();
            mEmptyCond.notify_one();
            return;
        }

        // Storage Full. Wait for signal.
        mCapacityCond.wait(lock1);
    }
}


template <typename ElementT, typename QueueT>
template <typename Rep, typename Period>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPushFor(
    const std::chrono::duration<Rep, Period>& timeout,
    ElementT const & element)
{
    // Convert duration to time_point: passing the same timeout duration to
    // each of multiple calls is wrong.
    return tryPushUntil(std::chrono::steady_clock::now() + timeout, element);
}


template <typename ElementT, typename QueueT>
template <typename Clock, typename Duration>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPushUntil(
    const std::chrono::time_point<Clock, Duration>& endpoint,
    ElementT const& element)
{
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
            mStorage.push(element);
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


template<typename ElementT, typename QueueT>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPush(ElementT const & element)
{
    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock())
        return false;

    if (mClosed)
        return false;

    if (mStorage.size() >= mCapacity)
        return false;

    mStorage.push(element);
    lock1.unlock();
    mEmptyCond.notify_one();
    return true;
}


template<typename ElementT, typename QueueT>
ElementT LLThreadSafeQueue<ElementT, QueueT>::pop(void)
{
    lock_t lock1(mLock);
    while (true)
    {
        if (!mStorage.empty())
        {
            // std::queue::front() is the element about to pop()
            ElementT value = mStorage.front();
            mStorage.pop();
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


template<typename ElementT, typename QueueT>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPop(ElementT & element)
{
    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock())
        return false;

    // no need to check mClosed: tryPop() behavior when the queue is
    // closed is implemented by simple inability to push any new elements
    if (mStorage.empty())
        return false;

    // std::queue::front() is the element about to pop()
    element = mStorage.front();
    mStorage.pop();
    lock1.unlock();
    mCapacityCond.notify_one();
    return true;
}


template <typename ElementT, typename QueueT>
template <typename Rep, typename Period>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPopFor(
    const std::chrono::duration<Rep, Period>& timeout,
    ElementT& element)
{
    // Convert duration to time_point: passing the same timeout duration to
    // each of multiple calls is wrong.
    return tryPopUntil(std::chrono::steady_clock::now() + timeout, element);
}


template <typename ElementT, typename QueueT>
template <typename Clock, typename Duration>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPopUntil(
    const std::chrono::time_point<Clock, Duration>& endpoint,
    ElementT& element)
{
    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock_until(endpoint))
        return false;

    while (true)
    {
        if (!mStorage.empty())
        {
            // std::queue::front() is the element about to pop()
            element = mStorage.front();
            mStorage.pop();
            lock1.unlock();
            mCapacityCond.notify_one();
            return true;
        }

        if (mClosed)
        {
            return false;
        }

        // Storage empty. Wait for signal.
        if (LLCoros::cv_status::timeout == mEmptyCond.wait_until(lock1, endpoint))
        {
            // timed out -- formally we might recheck both conditions above
            return false;
        }
        // If we didn't time out, we were notified for some reason. Loop back
        // to check.
    }
}


template<typename ElementT, typename QueueT>
size_t LLThreadSafeQueue<ElementT, QueueT>::size(void)
{
    lock_t lock(mLock);
    return mStorage.size();
}

template<typename ElementT, typename QueueT>
void LLThreadSafeQueue<ElementT, QueueT>::close()
{
    lock_t lock(mLock);
    mClosed = true;
    lock.unlock();
    // wake up any blocked pop() calls
    mEmptyCond.notify_all();
    // wake up any blocked push() calls
    mCapacityCond.notify_all();
}

template<typename ElementT, typename QueueT>
bool LLThreadSafeQueue<ElementT, QueueT>::isClosed()
{
    lock_t lock(mLock);
    return mClosed && mStorage.size() == 0;
}

template<typename ElementT, typename QueueT>
LLThreadSafeQueue<ElementT, QueueT>::operator bool()
{
    return ! isClosed();
}

#endif
