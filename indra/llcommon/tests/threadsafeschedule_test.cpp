/**
 * @file   threadsafeschedule_test.cpp
 * @author Nat Goodspeed
 * @date   2021-10-04
 * @brief  Test for threadsafeschedule.
 * 
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "threadsafeschedule.h"
// STL headers
// std headers
#include <chrono>
// external library headers
// other Linden headers
#include "../test/lltut.h"

using namespace std::literals::chrono_literals; // ms suffix
using namespace std::literals::string_literals; // s suffix
using Queue = LL::ThreadSafeSchedule<std::string>;

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct threadsafeschedule_data
    {
        Queue queue;
    };
    typedef test_group<threadsafeschedule_data> threadsafeschedule_group;
    typedef threadsafeschedule_group::object object;
    threadsafeschedule_group threadsafeschedulegrp("threadsafeschedule");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("push");
        // Simply calling push() a few times might result in indeterminate
        // delivery order if the resolution of steady_clock is coarser than
        // the real time required for each push() call. Explicitly increment
        // the timestamp for each one -- but since we're passing explicit
        // timestamps, make the queue reorder them.
        queue.push(Queue::TimeTuple(Queue::Clock::now() + 20ms, "ghi"));
        queue.push("abc"s);
        queue.push(Queue::Clock::now() + 10ms, "def");
        queue.close();
        auto entry = queue.pop();
        ensure_equals("failed to pop first", std::get<0>(entry), "abc"s);
        entry = queue.pop();
        ensure_equals("failed to pop second", std::get<0>(entry), "def"s);
        ensure("queue not closed", queue.isClosed());
        ensure("queue prematurely done", ! queue.done());
        entry = queue.pop();
        ensure_equals("failed to pop third", std::get<0>(entry), "ghi"s);
        bool popped = queue.tryPop(entry);
        ensure("queue not empty", ! popped);
        ensure("queue not done", queue.done());
    }
} // namespace tut
