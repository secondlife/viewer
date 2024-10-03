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

/*****************************************************************************
*   LLThreadSafeQueue
*****************************************************************************/
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

    // Limiting the number of pending items prevents unbounded growth of the
    // underlying queue.
    LLThreadSafeQueue(size_t capacity = 1024);
    virtual ~LLThreadSafeQueue() {}

    // Add an element to the queue (will block if the queue has reached
    // capacity).
    //
    // This call will raise an interrupt error if the queue is closed while
    // the caller is blocked.
    template <typename T>
    void push(T&& element);
    // legacy name
    void pushFront(ElementT const & element) { return push(element); }

    // Add an element to the queue (will block if the queue has reached
    // capacity). Return false if the queue is closed before push is possible.
    template <typename T>
    bool pushIfOpen(T&& element);

    // Try to add an element to the queue without blocking. Returns
    // true only if the element was actually added.
    template <typename T>
    bool tryPush(T&& element);
    // legacy name
    bool tryPushFront(ElementT const & element) { return tryPush(element); }

    // Try to add an element to the queue, blocking if full but with timeout
    // after specified duration. Returns true if the element was added.
    // There are potentially two different timeouts involved: how long to try
    // to lock the mutex, versus how long to wait for the queue to stop being
    // full. Careful settings for each timeout might be orders of magnitude
    // apart. However, this method conflates them.
    template <typename Rep, typename Period, typename T>
    bool tryPushFor(const std::chrono::duration<Rep, Period>& timeout,
                    T&& element);
    // legacy name
    template <typename Rep, typename Period>
    bool tryPushFrontFor(const std::chrono::duration<Rep, Period>& timeout,
                         ElementT const & element) { return tryPushFor(timeout, element); }

    // Try to add an element to the queue, blocking if full but with
    // timeout at specified time_point. Returns true if the element was added.
    template <typename Clock, typename Duration, typename T>
    bool tryPushUntil(const std::chrono::time_point<Clock, Duration>& until,
                      T&& element);
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
    bool tryPopUntil(const std::chrono::time_point<Clock, Duration>& until,
                     ElementT& element);
    // no legacy name because this is a newer method

    // Returns the size of the queue.
    size_t size();

    //Returns the capacity of the queue.
    U32 capacity() { return mCapacity; }

    // closes the queue:
    // - every subsequent push() call will throw LLThreadSafeQueueInterrupt
    // - every subsequent tryPush() call will return false
    // - pop() calls will return normally until the queue is drained, then
    //   every subsequent pop() will throw LLThreadSafeQueueInterrupt
    // - tryPop() calls will return normally until the queue is drained,
    //   then every subsequent tryPop() call will return false
    void close();

    // producer end: are we prevented from pushing any additional items?
    bool isClosed();
    // consumer end: are we done, is the queue entirely drained?
    bool done();

protected:
    typedef QueueT queue_type;
    QueueT mStorage;
    size_t mCapacity;
    bool mClosed;

    boost::fibers::timed_mutex mLock;
    typedef std::unique_lock<decltype(mLock)> lock_t;
    boost::fibers::condition_variable_any mCapacityCond;
    boost::fibers::condition_variable_any mEmptyCond;

    enum pop_result { EMPTY, DONE, WAITING, POPPED };
    // implementation logic, suitable for passing to tryLockUntil()
    template <typename Clock, typename Duration>
    pop_result tryPopUntil_(lock_t& lock,
                            const std::chrono::time_point<Clock, Duration>& until,
                            ElementT& element);
    // if we're able to lock immediately, do so and run the passed callable,
    // which must accept lock_t& and return bool
    template <typename CALLABLE>
    bool tryLock(CALLABLE&& callable);
    // if we're able to lock before the passed time_point, do so and run the
    // passed callable, which must accept lock_t& and return bool
    template <typename Clock, typename Duration, typename CALLABLE>
    bool tryLockUntil(const std::chrono::time_point<Clock, Duration>& until,
                      CALLABLE&& callable);
    // while lock is locked, really push the passed element, if we can
    template <typename T>
    bool push_(lock_t& lock, T&& element);
    // while lock is locked, really pop the head element, if we can
    pop_result pop_(lock_t& lock, ElementT& element);
    // Is the current head element ready to pop? We say yes; subclass can
    // override as needed.
    virtual bool canPop(const ElementT& head) const { return true; }
};

/*****************************************************************************
*   PriorityQueueAdapter
*****************************************************************************/
namespace LL
{
    /**
     * std::priority_queue's API is almost like std::queue, intentionally of
     * course, but you must access the element about to pop() as top() rather
     * than as front(). Make an adapter for use with LLThreadSafeQueue.
     */
    template <typename T, typename Container=std::vector<T>,
              typename Compare=std::less<typename Container::value_type>>
    class PriorityQueueAdapter
    {
    public:
        // publish all the same types
        typedef std::priority_queue<T, Container, Compare> queue_type;
        typedef typename queue_type::container_type  container_type;
        typedef typename queue_type::value_compare   value_compare;
        typedef typename queue_type::value_type      value_type;
        typedef typename queue_type::size_type       size_type;
        typedef typename queue_type::reference       reference;
        typedef typename queue_type::const_reference const_reference;

        // Although std::queue defines both const and non-const front()
        // methods, std::priority_queue defines only const top().
        const_reference front() const { return mQ.top(); }
        // std::priority_queue has no equivalent to back(), so it's good that
        // LLThreadSafeQueue doesn't use it.

        // All the rest of these merely forward to the corresponding
        // queue_type methods.
        bool empty() const                 { return mQ.empty(); }
        size_type size() const             { return mQ.size(); }
        void push(const value_type& value) { mQ.push(value); }
        void push(value_type&& value)      { mQ.push(std::move(value)); }
        template <typename... Args>
        void emplace(Args&&... args)       { mQ.emplace(std::forward<Args>(args)...); }
        void pop()                         { mQ.pop(); }

    private:
        queue_type mQ;
    };
} // namespace LL


/*****************************************************************************
*   LLThreadSafeQueue implementation
*****************************************************************************/
template<typename ElementT, typename QueueT>
LLThreadSafeQueue<ElementT, QueueT>::LLThreadSafeQueue(size_t capacity) :
    mCapacity(capacity),
    mClosed(false)
{
}


// if we're able to lock immediately, do so and run the passed callable, which
// must accept lock_t& and return bool
template <typename ElementT, typename QueueT>
template <typename CALLABLE>
bool LLThreadSafeQueue<ElementT, QueueT>::tryLock(CALLABLE&& callable)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock())
        return false;

    return std::forward<CALLABLE>(callable)(lock1);
}


// if we're able to lock before the passed time_point, do so and run the
// passed callable, which must accept lock_t& and return bool
template <typename ElementT, typename QueueT>
template <typename Clock, typename Duration, typename CALLABLE>
bool LLThreadSafeQueue<ElementT, QueueT>::tryLockUntil(
    const std::chrono::time_point<Clock, Duration>& until,
    CALLABLE&& callable)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lock_t lock1(mLock, std::defer_lock);
    if (!lock1.try_lock_until(until))
        return false;

    return std::forward<CALLABLE>(callable)(lock1);
}


// while lock is locked, really push the passed element, if we can
template <typename ElementT, typename QueueT>
template <typename T>
bool LLThreadSafeQueue<ElementT, QueueT>::push_(lock_t& lock, T&& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    if (mStorage.size() >= mCapacity)
        return false;

    mStorage.push(std::forward<T>(element));
    lock.unlock();
    // now that we've pushed, if somebody's been waiting to pop, signal them
    mEmptyCond.notify_one();
    return true;
}


template <typename ElementT, typename QueueT>
template <typename T>
bool LLThreadSafeQueue<ElementT, QueueT>::pushIfOpen(T&& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lock_t lock1(mLock);
    while (true)
    {
        // On the producer side, it doesn't matter whether the queue has been
        // drained or not: the moment either end calls close(), further push()
        // operations will fail.
        if (mClosed)
            return false;

        if (push_(lock1, std::forward<T>(element)))
            return true;

        // Storage Full. Wait for signal.
        mCapacityCond.wait(lock1);
    }
}


template <typename ElementT, typename QueueT>
template<typename T>
void LLThreadSafeQueue<ElementT, QueueT>::push(T&& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    if (! pushIfOpen(std::forward<T>(element)))
    {
        LLTHROW(LLThreadSafeQueueInterrupt());
    }
}


template<typename ElementT, typename QueueT>
template<typename T>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPush(T&& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    return tryLock(
        [this, element=std::move(element)](lock_t& lock)
        {
            if (mClosed)
                return false;
            return push_(lock, std::move(element));
        });
}


template <typename ElementT, typename QueueT>
template <typename Rep, typename Period, typename T>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPushFor(
    const std::chrono::duration<Rep, Period>& timeout,
    T&& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    // Convert duration to time_point: passing the same timeout duration to
    // each of multiple calls is wrong.
    return tryPushUntil(std::chrono::steady_clock::now() + timeout,
                        std::forward<T>(element));
}


template <typename ElementT, typename QueueT>
template <typename Clock, typename Duration, typename T>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPushUntil(
    const std::chrono::time_point<Clock, Duration>& until,
    T&& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    return tryLockUntil(
        until,
        [this, until, element=std::move(element)](lock_t& lock)
        {
            while (true)
            {
                if (mClosed)
                {
                    return false;
                }

                if (push_(lock, std::move(element)))
                    return true;

                // Storage Full. Wait for signal.
                if (LLCoros::cv_status::timeout == mCapacityCond.wait_until(lock, until))
                {
                    // timed out -- formally we might recheck both conditions above
                    return false;
                }
                // If we didn't time out, we were notified for some reason. Loop back
                // to check.
            }
        });
}


// while lock is locked, really pop the head element, if we can
template <typename ElementT, typename QueueT>
typename LLThreadSafeQueue<ElementT, QueueT>::pop_result
LLThreadSafeQueue<ElementT, QueueT>::pop_(lock_t& lock, ElementT& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    // If mStorage is empty, there's no head element.
    if (mStorage.empty())
        return mClosed? DONE : EMPTY;

    // If there's a head element, pass it to canPop() to see if it's ready to pop.
    if (! canPop(mStorage.front()))
        return WAITING;

    // std::queue::front() is the element about to pop()
    element = mStorage.front();
    mStorage.pop();
    lock.unlock();
    // now that we've popped, if somebody's been waiting to push, signal them
    mCapacityCond.notify_one();
    return POPPED;
}


template<typename ElementT, typename QueueT>
ElementT LLThreadSafeQueue<ElementT, QueueT>::pop(void)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lock_t lock1(mLock);
    ElementT value;
    while (true)
    {
        // On the consumer side, we always try to pop before checking mClosed
        // so we can finish draining the queue.
        pop_result popped = pop_(lock1, value);
        if (popped == POPPED)
            // don't use std::move when returning local value because
            // it prevents the compiler from optimizing with copy elision
            //return std::move(value);
            return value;

        // Once the queue is DONE, there will never be any more coming.
        if (popped == DONE)
        {
            LLTHROW(LLThreadSafeQueueInterrupt());
        }

        // If we didn't pop because WAITING, i.e. canPop() returned false,
        // then even if the producer end has been closed, there's still at
        // least one item to drain: wait for it. Or we might be EMPTY, with
        // the queue still open. Either way, wait for signal.
        mEmptyCond.wait(lock1);
    }
}


template<typename ElementT, typename QueueT>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPop(ElementT & element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    return tryLock(
        [this, &element](lock_t& lock)
        {
            // conflate EMPTY, DONE, WAITING: tryPop() behavior when the queue
            // is closed is implemented by simple inability to push any new
            // elements
            return pop_(lock, element) == POPPED;
        });
}


template <typename ElementT, typename QueueT>
template <typename Rep, typename Period>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPopFor(
    const std::chrono::duration<Rep, Period>& timeout,
    ElementT& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    // Convert duration to time_point: passing the same timeout duration to
    // each of multiple calls is wrong.
    return tryPopUntil(std::chrono::steady_clock::now() + timeout, element);
}


template <typename ElementT, typename QueueT>
template <typename Clock, typename Duration>
bool LLThreadSafeQueue<ElementT, QueueT>::tryPopUntil(
    const std::chrono::time_point<Clock, Duration>& until,
    ElementT& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    return tryLockUntil(
        until,
        [this, until, &element](lock_t& lock)
        {
            // conflate EMPTY, DONE, WAITING
            return tryPopUntil_(lock, until, element) == POPPED;
        });
}


// body of tryPopUntil(), called once we have the lock
template <typename ElementT, typename QueueT>
template <typename Clock, typename Duration>
typename LLThreadSafeQueue<ElementT, QueueT>::pop_result
LLThreadSafeQueue<ElementT, QueueT>::tryPopUntil_(
    lock_t& lock,
    const std::chrono::time_point<Clock, Duration>& until,
    ElementT& element)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    while (true)
    {
        pop_result popped = pop_(lock, element);
        if (popped == POPPED || popped == DONE)
        {
            // If we succeeded, great! If we've drained the last item, so be
            // it. Either way, break the loop and tell caller.
            return popped;
        }

        // EMPTY or WAITING: wait for signal.
        if (LLCoros::cv_status::timeout == mEmptyCond.wait_until(lock, until))
        {
            // timed out -- formally we might recheck
            // as it is, break loop
            return popped;
        }
        // If we didn't time out, we were notified for some reason. Loop back
        // to check.
    }
}


template<typename ElementT, typename QueueT>
size_t LLThreadSafeQueue<ElementT, QueueT>::size(void)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lock_t lock(mLock);
    return mStorage.size();
}


template<typename ElementT, typename QueueT>
void LLThreadSafeQueue<ElementT, QueueT>::close()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lock_t lock(mLock);
    return mClosed;
}


template<typename ElementT, typename QueueT>
bool LLThreadSafeQueue<ElementT, QueueT>::done()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lock_t lock(mLock);
    return mClosed && mStorage.empty();
}

#endif
