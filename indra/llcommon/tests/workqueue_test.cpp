/**
 * @file   workqueue_test.cpp
 * @author Nat Goodspeed
 * @date   2021-10-07
 * @brief  Test for workqueue.
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "workqueue.h"
// STL headers
// std headers
#include <chrono>
#include <deque>
// external library headers
// other Linden headers
#include "../test/lldoctest.h"
#include "../test/catch_and_store_what_in.h"
#include "llcond.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llstring.h"
#include "stringize.h"

using namespace LL;
using namespace std::literals::chrono_literals; // ms suffix
using namespace std::literals::string_literals; // s suffix

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct workqueue_data
{

        WorkSchedule queue{"queue"
};

TEST_CASE_FIXTURE(workqueue_data, "test_1")
{

        set_test_name("name");
        ensure_equals("didn't capture name", queue.getKey(), "queue");
        CHECK_MESSAGE(WorkSchedule::getInstance("queue", "not findable") == queue.getWeak().lock());
        WorkSchedule q2;
        CHECK_MESSAGE(LLStringUtil::startsWith(q2.getKey(, "has no name"), "WorkQueue"));
    
}

TEST_CASE_FIXTURE(workqueue_data, "test_2")
{

        set_test_name("post");
        bool wasRun{ false 
}

TEST_CASE_FIXTURE(workqueue_data, "test_3")
{

        set_test_name("postEvery");
        // record of runs
        using Shared = std::deque<WorkSchedule::TimePoint>;
        // This is an example of how to share data between the originator of
        // postEvery(work) and the work item itself, since usually a WorkSchedule
        // is used to dispatch work to a different thread. Neither of them
        // should call any of LLCond's wait methods: you don't want to stall
        // either the worker thread or the originating thread (conventionally
        // main). Use LLCond or a subclass even if all you want to do is
        // signal the work item that it can quit; consider LLOneShotCond.
        LLCond<Shared> data;
        auto start = WorkSchedule::TimePoint::clock::now();
        // 2s seems like a long time to wait, since it directly impacts the
        // duration of this test program. Unfortunately GitHub's Mac runners
        // are pretty wimpy, and we're getting spurious "too late" errors just
        // because the thread doesn't wake up as soon as we want.
        auto interval = 2s;
        queue.postEvery(
            interval,
            [&data, count = 0]
            () mutable
            {
                // record the timestamp at which this instance is running
                data.update_one(
                    [](Shared& data)
                    {
                        data.push_back(WorkSchedule::TimePoint::clock::now());
                    
}

TEST_CASE_FIXTURE(workqueue_data, "test_4")
{

        set_test_name("postTo");
        WorkSchedule main("main");
        auto qptr = WorkSchedule::getInstance("queue");
        int result = 0;
        main.postTo(
            qptr,
            [](){ return 17; 
}

TEST_CASE_FIXTURE(workqueue_data, "test_5")
{

        set_test_name("postTo with void return");
        WorkSchedule main("main");
        auto qptr = WorkSchedule::getInstance("queue");
        std::string observe;
        main.postTo(
            qptr,
            // The ONLY reason we can get away with binding a reference to
            // 'observe' in our work callable is because we're directly
            // calling qptr->runOne() on this same thread. It would be a
            // mistake to do that if some other thread were servicing 'queue'.
            [&observe](){ observe = "queue"; 
}

TEST_CASE_FIXTURE(workqueue_data, "test_6")
{

        set_test_name("waitForResult");
        std::string stored;
        // Try to call waitForResult() on this thread's main coroutine. It
        // should throw because the main coroutine must service the queue.
        auto what{ catch_what<WorkSchedule::Error>(
                [this, &stored](){ stored = queue.waitForResult(
                        [](){ return "should throw"; 
}

} // TEST_SUITE
