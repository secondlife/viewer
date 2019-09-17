/** 
 * @file lldynamicqueue.h
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

#ifndef LL_LLDYNAMICQUEUE_H
#define LL_LLDYNAMICQUEUE_H

#include <iostream>
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
     typedef std::shared_ptr<QueuedItemType1>    ptr_t;

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
    LLUUID operator()(const QueuedItemType1::ptr_t &item)
    {
        return item->mID;
    }
};

// declares a dynamic queue of QueuedItemType pointers
typedef LLDynamicPriorityQueue <QueuedItemType::ptr_t, get_item_id>  dynamic_queue_t;

 *------------------------------------------------------------------------
 *
 */
template <class ITEMT,              // Queued item type
    class IDFN>                     // Identity getter for ITEMT    
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

    typedef boost::heap::fibonacci_heap<typename HeapEntry::ptr_t, 
            boost::heap::compare< compare_entry >,
            boost::heap::stable<true> >                             queueheap_t;
    typedef typename queueheap_t::handle_type                       queuehandle_t;
    typedef std::map<LLUUID, typename queuehandle_t>                queuemapping_t;
    typedef IDFN                                                    get_id_fn_t;

public:

    /**
     * Construct a new Dynamic Priority Queue.  
     *   If threadsafe is set to true it will also allocate a mutex to control access 
     * to the queue.
     */
    LLDynamicPriorityQueue(bool threadsafe = false) :
        mGetIdFn(get_id_fn_t()),
        mPriorityHeap(),
        mEntryMapping(),
        mMutexp(nullptr)
    {
        if (threadsafe)
            mMutexp = new LLMutex;
    }
    ~LLDynamicPriorityQueue() 
    {
        delete mMutexp;
    }

    /**
     * Place an item on the queue with the given priority.  Or, if the item is already queued increase 
     * the priority of the previously queued item.  Returns the UUID associated with the enqueued item.
     */
    LLUUID enqueue(const ITEMT &item, U32 priority = 1)
    {
        LLMutexLock lock(mMutexp);  // scoped lock is a noop with a nullptr mutex.
        LLUUID id;
        id = mGetIdFn(item);

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
            (*handle)->mPriority += priority;   // if desired priority bump is 0 then priority remains unchanged.
            mPriorityHeap.increase(handle);
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

        if ((*handle)->mPriority <= priority)
        {
            mPriorityHeap.erase(handle);
            mEntryMapping.erase(it);
        }
        else
        {
            (*handle)->mPriority -= priority;
            mPriorityHeap.decrease(handle);
        }
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


    bool is_stable() const
    {
        return mPriorityHeap.is_stable;
    }

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


private:
    queueheap_t             mPriorityHeap;
    queuemapping_t          mEntryMapping;
    get_id_fn_t             mGetIdFn;
    LLMutex *               mMutexp;
};

#if LL_MSVC
#pragma warning( pop )
#endif
#endif
