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


#include <string>
#include <stdexcept>


struct apr_pool_t; // From apr_pools.h
class LLThreadSafeQueueImplementation; // See below.


//
// A general queue exception.
//
class LL_COMMON_API LLThreadSafeQueueError:
public std::runtime_error
{
public:
	LLThreadSafeQueueError(std::string const & message):
	std::runtime_error(message)
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


struct apr_queue_t; // From apr_queue.h


//
// Implementation details. 
//
class LL_COMMON_API LLThreadSafeQueueImplementation
{
public:
	LLThreadSafeQueueImplementation(apr_pool_t * pool, unsigned int capacity);
	~LLThreadSafeQueueImplementation();
	void pushFront(void * element);
	bool tryPushFront(void * element);
	void * popBack(void);
	bool tryPopBack(void *& element);
	size_t size();
	
private:
	bool mOwnsPool;
	apr_pool_t * mPool;
	apr_queue_t * mQueue;
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
	LLThreadSafeQueue(apr_pool_t * pool = 0, unsigned int capacity = 1024);
	
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
	LLThreadSafeQueueImplementation mImplementation;
};



// LLThreadSafeQueue
//-----------------------------------------------------------------------------


template<typename ElementT>
LLThreadSafeQueue<ElementT>::LLThreadSafeQueue(apr_pool_t * pool, unsigned int capacity):
	mImplementation(pool, capacity)
{
	; // No op.
}


template<typename ElementT>
void LLThreadSafeQueue<ElementT>::pushFront(ElementT const & element)
{
	ElementT * elementCopy = new ElementT(element);
	try {
		mImplementation.pushFront(elementCopy);
	} catch (LLThreadSafeQueueInterrupt) {
		delete elementCopy;
		throw;
	}
}


template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPushFront(ElementT const & element)
{
	ElementT * elementCopy = new ElementT(element);
	bool result = mImplementation.tryPushFront(elementCopy);
	if(!result) delete elementCopy;
	return result;
}


template<typename ElementT>
ElementT LLThreadSafeQueue<ElementT>::popBack(void)
{
	ElementT * element = reinterpret_cast<ElementT *> (mImplementation.popBack());
	ElementT result(*element);
	delete element;
	return result;
}


template<typename ElementT>
bool LLThreadSafeQueue<ElementT>::tryPopBack(ElementT & element)
{
	void * storedElement;
	bool result = mImplementation.tryPopBack(storedElement);
	if(result) {
		ElementT * elementPtr = reinterpret_cast<ElementT *>(storedElement); 
		element = *elementPtr;
		delete elementPtr;
	} else {
		; // No op.
	}
	return result;
}


template<typename ElementT>
size_t LLThreadSafeQueue<ElementT>::size(void)
{
	return mImplementation.size();
}


#endif
