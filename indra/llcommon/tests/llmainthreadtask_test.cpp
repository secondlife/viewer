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
#include "../test/lltut.h"
#include "../test/sync.h"
#include "llthread.h"               // on_main_thread()
#include "lleventtimer.h"
#include "lockstatic.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llmainthreadtask_data
    {
        // 2-second timeout
        Sync mSync{F32Milliseconds(2000.0f)};

        llmainthreadtask_data()
        {
            // we're not testing the result; this is just to cache the
            // initial thread as the main thread.
            on_main_thread();
        }
    };
    typedef test_group<llmainthreadtask_data> llmainthreadtask_group;
    typedef llmainthreadtask_group::object object;
    llmainthreadtask_group llmainthreadtaskgrp("llmainthreadtask");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("inline");
        bool ran = false;
        bool result = LLMainThreadTask::dispatch(
            [&ran]()->bool{
                ran = true;
                return true;
            });
        ensure("didn't run lambda", ran);
        ensure("didn't return result", result);
    }

    struct StaticData
    {
        std::mutex mMutex;          // LockStatic looks for mMutex
        bool ran{false};
    };
    typedef llthread::LockStatic<StaticData> LockStatic;

    template<> template<>
    void object::test<2>()
    {
        set_test_name("cross-thread");
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
                        }));
                // wait for test<2>() to unblock us again
                mSync.yield_until(3);
                result = on_main;
            });
        auto thread_result = thread_work.get_future();
        std::thread thread;
        try
        {
            // run thread_work
            thread = std::thread(std::move(thread_work));
            // wait for thread to set(1)
            mSync.yield_until(1);
            // try to acquire the lock, should block because thread has it
            LockStatic lk;
            // wake up when dispatch() unlocks the static mutex
            ensure("shouldn't have run yet", !lk->ran);
            ensure("shouldn't have returned yet", !result);
            // unlock so the task can acquire the lock
            lk.unlock();
            // run the task -- should unblock thread, which will immediately block
            // on mSync
            LLEventTimer::updateClass();
            // 'lk', having unlocked, can no longer be used to access; relock with
            // a new LockStatic instance
            ensure("should now have run", LockStatic()->ran);
            ensure("returned too early", !result);
            // okay, let thread perform the assignment
            mSync.set(3);
        }
        catch (...)
        {
            // A test failure exception anywhere in the try block can cause
            // the test program to terminate without explanation when
            // ~thread() finds that 'thread' is still joinable. We could
            // either join() or detach() it -- but since it might be blocked
            // waiting for something from the main thread that now can never
            // happen, it's safer to detach it.
            thread.detach();
            throw;
        }
        // 'thread' should be all done now
        thread.join();
        // deliver any exception thrown by thread_work
        thread_result.get();
        ensure("ran changed", LockStatic()->ran);
        ensure("didn't run on main thread", result);
    }
} // namespace tut
