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

#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable:4265)
#endif
// 'std::_Pad' : class has virtual functions, but destructor is not virtual
#include <mutex>
#include <condition_variable>

#if LL_WINDOWS
#pragma warning (pop)
#endif

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
	// This call will raise an interrupt error if the queue is deleted while
	// the caller is blocked.
	void pushFront(ElementT const & element);
	
	// Try to add an element to the front ofqueue without blocking. Returns
	// true only if the element was actually added.
	bool tryPushFront(ElementT const & element);
	
	// Pop the element at the end of the queue (will block if the queue is
	// empty).
	//
	// This call will raise an interrupt error if the queue is deleted while
	// the caller is blocked.
	ElementT popBack(void);
	
	// Pop an element from the end of the queue if there is one available.
	// Returns true only if an element was popped.
	bool tryPopBack(ElementT & element);
	
	// Returns the size of the queue.
	size_t size();

private:
	std::deque< ElementT > mStorage;
	U32 mCapacity;

	std::mutex mLock;
	std::condition_variable mCapacityCond;
	std::condition_variable mEmptyCond;
};

// LLThreadSafeQueue
//-----------------------------------------------------------------------------

template<typename ElementT>
LLThreadSafeQueue<ElementT>::LLThreadSafeQueue(U32 capacity) :
mCapacity(capacity)
{
}


template<typename ElementT>
void LLThreadSafeQueue<ElementT>::pushFront(ElementT const & element)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock1(mLock);

        if (mStorage.size() < mCapacity)
        {
            mStorage.push_front(element);
            mEmptyCond.notify_one();
            return;
        }

        // Storage Full. Wait for signal.
        mCapacityCond.wait(lock1);
    }
}


template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPushFront(ElementT const & element)
{
    std::unique_lock<std::mutex> lock1(mLock, std::defer_lock);
    if (!lock1.try_lock())
        return false;

    if (mStorage.size() >= mCapacity)
        return false;

    mStorage.push_front(element);
    mEmptyCond.notify_one();
    return true;
}


template<typename ElementT>
ElementT LLThreadSafeQueue<ElementT>::popBack(void)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock1(mLock);

        if (!mStorage.empty())
        {
            ElementT value = mStorage.back();
            mStorage.pop_back();
            mCapacityCond.notify_one();
            return value;
        }

        // Storage empty. Wait for signal.
        mEmptyCond.wait(lock1);
    }
}


template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPopBack(ElementT & element)
{
    std::unique_lock<std::mutex> lock1(mLock, std::defer_lock);
    if (!lock1.try_lock())
        return false;

    if (mStorage.empty())
        return false;

    element = mStorage.back();
    mStorage.pop_back();
    mCapacityCond.notify_one();
    return true;
}


template<typename ElementT>
size_t LLThreadSafeQueue<ElementT>::size(void)
{
    std::lock_guard<std::mutex> lock(mLock);
    return mStorage.size();
}

#endif
