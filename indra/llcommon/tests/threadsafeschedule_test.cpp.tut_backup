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
        auto now{ Queue::Clock::now() };
        queue.push(Queue::TimeTuple(now + 200ms, "ghi"s));
        // Given the various push() overloads, you have to match the type
        // exactly: conversions are ambiguous.
        queue.push(now, "abc"s);
        queue.push(now + 100ms, "def"s);
        queue.close();
        auto entry = queue.pop();
        ensure_equals("failed to pop first", std::get<0>(entry), "abc"s);
        entry = queue.pop();
        ensure_equals("failed to pop second", std::get<0>(entry), "def"s);
        ensure("queue not closed", queue.isClosed());
        ensure("queue prematurely done", ! queue.done());
        std::string s;
        bool popped = queue.tryPopFor(1s, s);
        ensure("failed to pop third", popped);
        ensure_equals("third is wrong", s, "ghi"s);
        popped = queue.tryPop(s);
        ensure("queue not empty", ! popped);
        ensure("queue not done", queue.done());
    }
} // namespace tut
