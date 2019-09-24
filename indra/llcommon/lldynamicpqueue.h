/** 
 * @file lldynamicpqueue.h
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_DYNAMICPQUEUE_H
#define LL_DYNAMICPQUEUE_H

#ifdef LL_TEST_lldynamicpqueue
#include <iostream>
#endif 
#include <set>
#include <vector>
#include "stdtypes.h"
#include "llpreprocessor.h"
#include "lluuid.h"
#include "llmutex.h"

#if LL_MSVC
// boost Fibonacci heap defines multiple copy constructors (one const one not) Microsoft issues this as an "informal" warning.
#pragma warning( push )
#pragma warning( disable : 4521 4390)   // https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4521?view=vs-2019
#endif

#include <boost/heap/fibonacci_heap.hpp>
// #include <boost/heap/binomial_heap.hpp>
#include <boost/heap/policies.hpp>

/**
 * LLDynamicPriorityQueue
 *
 * There are a number of cases where we want to queue a series of request for later action.  
 * We may also want to assign a priority to those requests so that requests with a higher priority 
 * are serviced from the queue before those of a lower priority. It is difficult however to change
 * an item's priority (or remove it entirely) once it has been enqueued.
 *
 * The dynamic priority queue allows elements enqueued to change their priority after they have 
 * been placed on the queue.  The more requests made for an item the more important it is considered
 * and the sooner it comes to the top.  (Texture caching in the viewer is one such situation.)
 * 
 *------------------------------------------------------------------------
 * Sample use:

 // This would be the item queued
 struct QueuedItemType
 {
     typedef std::shared_ptr<QueuedItemType>    ptr_t;

     QueuedItemType(std::string &name) :
         mName(name),
         mID()
     {
        mID.generate();
     }

     std::string mName;
     LLUUID      mID;

 };

// This is a functor type provided to the dynamic queue template 
// in order to extract a UUID from the item. 
struct get_item_id
{
    LLUUID operator()(const QueuedItemType::ptr_t &item)
    {
        return item->mID;
    }
};

// declares a dynamic queue of QueuedItemType pointers
typedef LLDynamicPriorityQueue <QueuedItemType::ptr_t, get_item_id>  dynamic_queue_t;

 *------------------------------------------------------------------------
 *
 */
template <class ITEMT>
struct default_priority_change
{
    /**
    * Default priority modification functor.
    *  If increasing the priority add the old and new.
    *  If decreasing the priority subtract the two.  If the bump is greater than the old priority return 0.
    */
    U32 operator()(bool increase, const ITEMT &, U32 priority, U32 bump)
    {
        if (increase)
            return priority + bump;
        else
            return (bump <= priority) ? (priority - bump) : 0;
    }
};


template <class ITEMT,              // Queued item type
    class IDFN,                     // Identity getter for ITEMT.  Must be of the form <UUID (const ITEMT &)>  taking an ITEMT and returning its UUID.
    class PRIOFN = default_priority_change<ITEMT> >   // Priority update function.  Must be of the form <U32 (PriorityChange, const ITEMT &, U32, U32)>
class LLDynamicPriorityQueue
{
    struct HeapEntry
    {
        typedef std::shared_ptr<HeapEntry> ptr_t;

        LLUUID  mId; 
        U32     mPriority;
        ITEMT   mItem;

        bool operator < (typename const HeapEntry &other) const
        {   
            return mPriority < other.mPriority;
        }
    };

    struct compare_entry
    {
        bool operator()(typename const HeapEntry::ptr_t &a, typename const HeapEntry::ptr_t &b) const
        {
            return (*a < *b);
        }
    };


    // Fibonacci heaps are used with intent here.  Fibonacci heaps provide for efficient reordering.
    typedef boost::heap::fibonacci_heap<typename HeapEntry::ptr_t, 
            boost::heap::compare< compare_entry >,
            boost::heap::stable<true> >                             queueheap_t;
    typedef typename queueheap_t::handle_type                       queuehandle_t;
    typedef std::map<LLUUID, typename queuehandle_t>                queuemapping_t;
    typedef IDFN                                                    get_id_fn_t;
    typedef PRIOFN                                                  update_priority_fn_t;
public:
    /**
     * Construct a new Dynamic Priority Queue.  
     *   If threadsafe is set to true it will also allocate a mutex to control access 
     * to the queue.
     */
    LLDynamicPriorityQueue(bool threadsafe = false) :
        mPriorityHeap(),
        mEntryMapping(),
        mMutexp()
    {
        if (threadsafe)
            mMutexp.reset(new LLMutex);
    }
    ~LLDynamicPriorityQueue() {}

    /**
     * Place an item on the queue with the given priority.  Or, if the item is already queued increase 
     * the priority of the previously queued item.  The higher an item's priority, the more quickly it
     * is popped off the queue. 
     * Returns the UUID associated with the enqueued item.
     */
    LLUUID enqueue(const ITEMT &item, U32 priority = 1)
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        LLUUID id;
        id = get_id_fn_t()(item);

        queuemapping_t::iterator it = mEntryMapping.find(id);

        if (it == mEntryMapping.end())
        {   // No item with this id is in the queue.  Create a new entry. 
            HeapEntry::ptr_t entry(std::make_shared<HeapEntry>());
            entry->mId = id;
            entry->mPriority = (priority) ? priority : 1;   // if 0 priority then use a minimum priority of 1
            entry->mItem = item;
            
            queuehandle_t handle = mPriorityHeap.push(entry);
            mEntryMapping[id] = handle;
        }
        else
        {   // The item has already been queued, increase its priority by the passed amount. 
            queuehandle_t handle((*it).second);

            (*handle)->mPriority = update_priority_fn_t()(true, (*handle)->mItem, (*handle)->mPriority, priority);
            mPriorityHeap.update(handle); 
        }

        return id;
    }

    /**
     * Decrease the priority of an item on the queue.  If the priority would fall to 0 or below
     * the item is removed from the queue.
     */
    void forget(LLUUID item_id, U32 priority = 1)
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        queuemapping_t::iterator it = mEntryMapping.find(item_id);

        if (it == mEntryMapping.end())
        {
            return;
        }

        queuehandle_t handle((*it).second);

        (*handle)->mPriority = update_priority_fn_t()(false, (*handle)->mItem, (*handle)->mPriority, priority);

        if (!(*handle)->mPriority)
        {
            mPriorityHeap.erase(handle);
            mEntryMapping.erase(it);
        }
        else
        {
            mPriorityHeap.update(handle);
        }
    }

    void forget(ITEMT item, U32 priority = 1)
    {
        forget(get_id_fn_t()(item), priority);
    }

    /**
     * Absolutely remove the indicated item from the queue.
     */
    void remove(LLUUID item_id)
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        queuemapping_t::iterator it = mEntryMapping.find(item_id);

        if (it == mEntryMapping.end())
            return;

        queuehandle_t handle((*it).second);
        mPriorityHeap.erase(handle);
        mEntryMapping.erase(it);
    }

    void remove(const ITEMT item)
    {
        remove(get_id_fn_t()(item));
    }

    /**
     * Increase or decrease the priority of an item
     */
    void priorityAdjust(LLUUID item_id, S32 adjustment)
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        if (!isQueued(item_id))
            return;

        queuemapping_t::iterator it = mEntryMapping.find(item_id);
        if (it == mEntryMapping.end())
            return;

        queuehandle_t handle((*it).second);
        if (adjustment > 0)
        {
            (*handle)->mPriority += adjustment;
            mPriorityHeap.increase(handle);
        }
        else if (adjustment < 0)
        {
            adjustment = -adjustment;
            if (adjustment < (*handle)->mPriority)
            {
                (*handle)->mPriority -= adjustment;
                mPriorityHeap.decrease(handle);
            }
            else
            {
                mPriorityHeap.erase(handle);
                mEntryMapping.erase(it);
            }
        }
    }
    /**
     * Remove all items from the queue.
     */
    void clear()
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        mPriorityHeap.clear();
        mEntryMapping.clear();
    }

    /**
     * Test if the queue is empty. Return true if it is.
     */
    bool empty() const
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        return mPriorityHeap.empty();
    }

    /**
     * Return the number of items in the queue. 
     */
    size_t size() const
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        return mPriorityHeap.size();
    }

    /**
     * Test if an item is currently in the queue
     */
    bool isQueued(LLUUID item_id) const
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        return (mEntryMapping.find(item_id) != mEntryMapping.end());
    }

    /**
     * Peek at the first item (the item with the highest priority) on the queue.
     * If the queue is empty return a default constructed ITEMT
     */
    ITEMT top() const
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        if (mPriorityHeap.empty())
            return ITEMT();

        return mPriorityHeap.top()->mItem;
    }

    /**
     * Peek at the priority of the top item on the queue.
     */
    U32 topPriority() const
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        if (mPriorityHeap.empty())
            return 0;
        return mPriorityHeap.top()->mPriority;
    }

    /**
     * Pop the first item (the item with the highest priority) from the queue and return it.
     * If there is nothing on the queue return a default constructed ITEMT
     */
    ITEMT pop()
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.

        if (mPriorityHeap.empty())
            return ITEMT();

        HeapEntry::ptr_t entry(mPriorityHeap.top());

        mPriorityHeap.pop();
        mEntryMapping.erase(entry->mId);
        return entry->mItem;
    }

    /**
     * Take an explicit lock on the queue.  Allows for multiple operations 
     */
    void lock()
    {
        if (mMutexp)
            mMutexp->lock();
    }

    void unlock()
    {
        if (mMutexp && mMutexp->isSelfLocked())
            mMutexp->unlock();
    }


#ifdef LL_TEST_lldynamicpqueue
    void debug_dump(std::ostream &os)
    {
        os << "Ordered dump: [ ";
        bool first(true);
        queueheap_t::ordered_iterator oit = mPriorityHeap.ordered_begin();
        while (oit != mPriorityHeap.ordered_end())
        {
            if (!first)
                os << ", ";
            first = false;
            os << "(" << (*oit)->mPriority << ")" << (*oit)->mId;
            ++oit;
        }
        os << "]" << std::endl;
//         os << "Normal iterator:" << "[ ";
//          first = true;
//         queueheap_t::iterator uit = mPriorityHeap.begin();
//         while (uit != mPriorityHeap.end())
//         {
//             if (!first)
//                 os << ", ";
//             first = false;
//             os << "(" << (*uit)->mPriority << ")" << (*uit)->mId;
//             ++uit;
//         }
//         os << "]" << std::endl;
    }
#endif

private:
    queueheap_t             mPriorityHeap;
    queuemapping_t          mEntryMapping;
    LLMutex::uptr_t         mMutexp;
};

#if LL_MSVC
#pragma warning( pop )
#endif
#endif
