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
#include "../test/lldoctest.h"

using namespace std::literals::chrono_literals; // ms suffix
using namespace std::literals::string_literals; // s suffix
using Queue = LL::ThreadSafeSchedule<std::string>;

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct threadsafeschedule_data
{

        Queue queue;
    
};

TEST_CASE_FIXTURE(threadsafeschedule_data, "test_1")
{

        set_test_name("push");
        // Simply calling push() a few times might result in indeterminate
        // delivery order if the resolution of steady_clock is coarser than
        // the real time required for each push() call. Explicitly increment
        // the timestamp for each one -- but since we're passing explicit
        // timestamps, make the queue reorder them.
        auto now{ Queue::Clock::now() 
}

} // TEST_SUITE
