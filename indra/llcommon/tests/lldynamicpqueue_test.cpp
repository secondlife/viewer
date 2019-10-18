/** 
 * @file lldeadmantimer_test.cpp
 * @brief Tests for the LLDeadmanTimer class.
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
#include <array>
#include "../lldynamicpqueue.h"

#include "../test/lltut.h"

struct QueuedItemType1
{
    typedef std::shared_ptr<QueuedItemType1>    ptr_t;

    QueuedItemType1(const std::string &name, U32 counter) :
        mName(name),
        mCounter(counter),
        mID()
    {
        mID.generate();
    }

    std::string mName;
    U32         mCounter;
    LLUUID      mID;

};


struct get_item_id
{
    LLUUID operator()(const QueuedItemType1::ptr_t &item)
    {
        return item->mID;
    }
};

namespace tut
{
typedef std::vector<QueuedItemType1::ptr_t> test_items_1_t;


// produces a vector of test items.  mCounter for test items should be [0, count) 
void generate_test_items(U32 count, test_items_1_t &items)
{
    items.clear();
    items.reserve(count);

    for (U32 index = 0; index < count; ++index)
    {
        std::stringstream name;
        name << "Test Item #" << (index + 1);

        items.push_back(std::make_shared<QueuedItemType1>(name.str(), index));
    }

    std::cout << items.size() << " test items generated: " << std::endl;
    std::cout << "[ ";
    for (auto item : items)
    {
        std::cout << item->mID << ", ";
    }
    std::cout << "]" << std::endl;
}

struct dynamic_p_queue_test
{
    dynamic_p_queue_test()
	{
	}
};

typedef test_group<dynamic_p_queue_test> dynamic_p_queue_group_t;
typedef dynamic_p_queue_group_t::object dynamic_p_queue_object_t;
tut::dynamic_p_queue_group_t dynamic_p_queue_instance("LLDynamicPriorityQueue");

typedef LLDynamicPriorityQueue < QueuedItemType1::ptr_t, get_item_id>  testing_queue_1_t;

// Basic queue operations.
template<> template<>
void dynamic_p_queue_object_t::test<1>()
{
    test_items_1_t      test_data;
    generate_test_items(5, test_data);

    //-------------------------------------------
    // Create a new priority queue. 
    testing_queue_1_t   test_q;

    ensure("Queue constructed should be empty!", test_q.empty());

    // Add 5 items to it with the default priority.
    for (auto item: test_data)
    {
        test_q.enqueue(item);
        //test_q.debug_dump(std::cout);
    }

    ensure_equals("Priority Q should have all items.", test_q.size(), test_data.size());

    S32 pop_count(0);
    // Peek at the first item.
    QueuedItemType1::ptr_t item = test_q.top(); 

    ensure_equals("Priority Q should not have changed sizes.", test_q.size(), test_data.size());
    ensure_equals("Top should match first item in list.", item->mID, test_data[0]->mID);

    // Pop them off, they should come in order.
    std::array<S32, 5> expected{ { 0, 1, 2, 3, 4 } };
    for (S32 idx : expected)
    {
        QueuedItemType1::ptr_t popped = test_q.pop();
        ensure_equals("Popped item out of order", popped->mID, test_data[idx]->mID);
        ++pop_count;
    }

    ensure("Queue should now be empty!", test_q.empty());
    ensure_equals("Items popped not the same as items pushed", pop_count, test_data.size());

    QueuedItemType1::ptr_t top_empty = test_q.top();
    ensure("Non null top result on empty queue", !top_empty);

    QueuedItemType1::ptr_t pop_empty = test_q.pop();
    ensure("Non null pop result on empty queue", !pop_empty);
}

// Push with increasing priorities.
template<> template<>
void dynamic_p_queue_object_t::test<2>()
{
    test_items_1_t      test_data;
    generate_test_items(5, test_data);

    //-------------------------------------------
    // Create a new priority queue. 
    testing_queue_1_t   test_q;

    ensure("Queue constructed should be empty!", test_q.empty());

    // Add 5 items to it with increasing priority.
    U32 priority(1);
    for (auto item: test_data)
    {
        test_q.enqueue(item, priority++);
        //test_q.debug_dump(std::cout);
    }

    ensure_equals("Priority Q should have all items.", test_q.size(), test_data.size());

    S32 pop_count(0);
    // Peek at the first item.
    QueuedItemType1::ptr_t item = test_q.top(); 

    ensure_equals("Priority Q should not have changed sizes.", test_q.size(), test_data.size());
    ensure_equals("Top should match last item in list.", item->mID, test_data[ test_data.size()-1 ]->mID);

    std::array<S32, 5> expected{ { 4, 3, 2, 1, 0 } };
    for (S32 idx : expected)
    {
        QueuedItemType1::ptr_t popped = test_q.pop();
        ensure_equals("Popped item out of order", popped->mID, test_data[idx]->mID);
        //test_q.debug_dump(std::cout);
        ++pop_count;
    }

    ensure("Queue should now be empty!", test_q.empty());
    ensure_equals("Items popped not the same as items pushed", pop_count, test_data.size());

    QueuedItemType1::ptr_t top_empty = test_q.top();
    ensure("Non null top result on empty queue", !top_empty);

    QueuedItemType1::ptr_t pop_empty = test_q.pop();
    ensure("Non null pop result on empty queue", !pop_empty);
}

// Multiple requests bump priority.
template<> template<>
void dynamic_p_queue_object_t::test<3>()
{
    test_items_1_t      test_data;
    generate_test_items(5, test_data);

    //-------------------------------------------
    // Create a new priority queue. 
    testing_queue_1_t   test_q;

    ensure("Queue constructed should be empty!", test_q.empty());

    // Add 5 items to the queue
    for (auto item : test_data)
    {
        test_q.enqueue(item);
    }

    ensure_equals("Priority Q should have all items.", test_q.size(), test_data.size());

    // Peek at the first item.
    QueuedItemType1::ptr_t item = test_q.top();
    ensure_equals("Top should match last item in list.", item->mID, test_data[0]->mID);

    // re-queue the 3rd item
    test_q.enqueue(test_data[3]);

    item = test_q.top();
    ensure_equals("Top should match the 3rd item in list.", item->mID, test_data[3]->mID);

    // re-queue the 4th item
    test_q.enqueue(test_data[4]);
    item = test_q.top();
    ensure_equals("Top should still match the 3rd item in list.", item->mID, test_data[3]->mID);

    // re-re-queue the 4th item.
    test_q.enqueue(test_data[4]);
    item = test_q.top();
    ensure_equals("Top should match the 4rd item in list.", item->mID, test_data[4]->mID);
}

// Forgetting to priority 0 will erase the entry
template<> template<>
void dynamic_p_queue_object_t::test<4>()
{
    test_items_1_t      test_data;
    generate_test_items(5, test_data);

    //-------------------------------------------
    // Create a new priority queue. 
    testing_queue_1_t   test_q;

    ensure("Queue constructed should be empty!", test_q.empty());

    // Add 5 items to the queue
    for (auto item : test_data)
    {
        test_q.enqueue(item);
    }

    ensure_equals("Priority Q should have all items.", test_q.size(), test_data.size());

    // Peek at the first item.
    QueuedItemType1::ptr_t item = test_q.top();
    ensure_equals("Top should match last item in list.", item->mID, test_data[0]->mID);

    // forget the 3rd item
    test_q.forget(test_data[3]->mID);

    ensure_equals("Priority Q should have forgotten one.", test_q.size(), test_data.size() - 1);

    // Pop them off, they should come in order. (and 3 should be missing)
    std::array<S32, 4> expected{ { 0, 1, 2, 4 } };
    for (S32 idx : expected)
    {
        QueuedItemType1::ptr_t popped = test_q.pop();
        ensure_equals("Popped item out of order", popped->mID, test_data[idx]->mID);
        ensure_not_equals("Item 3 should be gone", popped->mID, test_data[3]->mID);
    }
}

// Forgetting to priority >0 will back it up.
template<> template<>
void dynamic_p_queue_object_t::test<5>()
{
    test_items_1_t      test_data;
    generate_test_items(5, test_data);

    //-------------------------------------------
    // Create a new priority queue. 
    testing_queue_1_t   test_q;

    ensure("Queue constructed should be empty!", test_q.empty());

    // Add 5 items to the queue
    for (auto item : test_data)
    {
        test_q.enqueue(item);
    }

    ensure_equals("Priority Q should have all items.", test_q.size(), test_data.size());

    test_q.enqueue(test_data[3]);
    test_q.enqueue(test_data[4]);

    // Peek at the first item.
    QueuedItemType1::ptr_t item = test_q.top();
    ensure_equals("Top should match item in list.", item->mID, test_data[3]->mID);

    // forget the 3rd item
    test_q.forget(test_data[3]->mID);

    ensure_equals("Priority Q should not have changed size.", test_q.size(), test_data.size()); // nothing erased.

    // Pop them off, they should come in order. (and 4 should be first)
    std::array<S32, 5> expected{ { 4, 0, 1, 2, 3 } };
    for (S32 idx: expected)
    {
        QueuedItemType1::ptr_t popped = test_q.pop();
        ensure_equals("Popped item out of order", popped->mID, test_data[idx]->mID);
    }
}

// Absolute remove
template<> template<>
void dynamic_p_queue_object_t::test<6>()
{
    test_items_1_t      test_data;
    generate_test_items(5, test_data);

    //-------------------------------------------
    // Create a new priority queue. 
    testing_queue_1_t   test_q;

    ensure("Queue constructed should be empty!", test_q.empty());

    // Add 5 items to the queue
    for (auto item : test_data)
    {
        test_q.enqueue(item);
    }

    ensure_equals("Priority Q should have all items.", test_q.size(), test_data.size());

    test_q.enqueue(test_data[2]);

    // forget the 3rd item
    test_q.remove(test_data[2]->mID);

    ensure_equals("Priority Q should not have changed size.", test_q.size(), test_data.size() - 1); // one item erased

    // Pop them off, they should come in order. (and 2 should be missing)
    std::array<S32, 4> expected{ { 0, 1, 3, 4 } };
    for (S32 idx : expected)
    {
        QueuedItemType1::ptr_t popped = test_q.pop();
        ensure_equals("Popped item out of order", popped->mID, test_data[idx]->mID);
        ensure_not_equals("Item 2 should be missing.", popped->mID, test_data[2]->mID);
    }
}

} // end namespace tut
