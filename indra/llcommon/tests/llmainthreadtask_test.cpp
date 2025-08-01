/**
 * @file   llmainthreadtask_test.cpp
 * @author Nat Goodspeed
 * @date   2019-12-05
 * @brief  Test for llmainthreadtask.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llmainthreadtask.h"
// STL headers
// std headers
#include <atomic>
// external library headers
// other Linden headers
#include "../test/lldoctest.h"
#include "../test/sync.h"
#include "llthread.h"               // on_main_thread()
#include "lleventtimer.h"
#include "lockstatic.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct llmainthreadtask_data
{

        // 5-second timeout
        Sync mSync{F32Milliseconds(5000.0f)
};

TEST_CASE_FIXTURE(llmainthreadtask_data, "test_1")
{

        set_test_name("inline");
        bool ran = false;
        bool result = LLMainThreadTask::dispatch(
            [&ran]()->bool{
                ran = true;
                return true;
            
}

TEST_CASE_FIXTURE(llmainthreadtask_data, "test_2")
{

        set_test_name("cross-thread");
        skip("This test is prone to build-time hangs");
        std::atomic_bool result(false);
        // wrapping our thread lambda in a packaged_task will catch any
        // exceptions it might throw and deliver them via future
        std::packaged_task<void()> thread_work(
            [this, &result](){
                // unblock test<2>()'s yield_until(1)
                mSync.set(1);
                // dispatch work to main thread -- should block here
                bool on_main(
                    LLMainThreadTask::dispatch(
                        []()->bool{
                            // have to lock static mutex to set static data
                            LockStatic()->ran = true;
                            // indicate whether task was run on the main thread
                            return on_main_thread();
                        
}

} // TEST_SUITE
