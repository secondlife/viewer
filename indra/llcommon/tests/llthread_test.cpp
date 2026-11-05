/**
 * @file   llthread_test.cpp
 * @date   2026-10-28
 * @brief  Test for LLThread start, exit and shutdown.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (c) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llthread.h"
// STL headers
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "llexception.h"
#include "stringize.h"
#include "workqueue.h"

using namespace std::chrono_literals;

namespace
{
    class TestThread : public LLThread
    {
    public:
        // If wait_for_quit, run() keeps going until asked to quit.
        TestThread(bool wait_for_quit):
            LLThread("TestThread"),
            mWaitForQuit(wait_for_quit)
        {}

        // setQuitting() is protected
        void requestQuit() { setQuitting(); }

        // Deliberately not atomic: run() writes it just before the thread
        // finishes, and other threads may read it once isStopped() is true.
        std::string mResult;
        std::atomic<bool> mStarted{ false };

    protected:
        void run() override
        {
            mStarted = true;
            if (mWaitForQuit)
            {
                while (!isQuitting())
                {
                    std::this_thread::sleep_for(1ms);
                }
            }
            mResult = "done";
        }

    private:
        bool mWaitForQuit;
    };

    // run() sleeps in checkPause() until runCondition() is true, and counts
    // each pass it makes, until it's asked to quit.
    class WaitingThread : public LLThread
    {
    public:
        WaitingThread():
            LLThread("WaitingThread")
        {}

        // Let run() make passes (true), or send it back to sleep (false).
        void setReady(bool ready)
        {
            lockData();
            mReady = ready;
            unlockData();
            wake();
        }

        std::atomic<bool> mStarted{ false };
        std::atomic<int> mPasses{ 0 };

    protected:
        // LLThread calls this with mDataLock locked
        bool runCondition() override { return mReady; }

        void run() override
        {
            mStarted = true;
            while (!isQuitting())
            {
                checkPause();
                if (isQuitting())
                {
                    break;
                }
                ++mPasses;
                std::this_thread::sleep_for(1ms);
            }
        }

    private:
        bool mReady = false;                // guarded by mDataLock
    };

    // run() waits until asked to quit, then reports through flags that the
    // test owns. ~LLThread() calls shutdown() only after this subclass has
    // been destroyed, so run() must not use anything the subclass owns once
    // it's asked to quit; these references are only ever read, so in
    // practice they stay usable until the base destructor returns.
    class UntilQuitThread : public LLThread
    {
    public:
        UntilQuitThread(std::atomic<bool>& started, std::atomic<bool>& finished):
            LLThread("UntilQuitThread"),
            mStarted(started),
            mFinished(finished)
        {}

    protected:
        void run() override
        {
            mStarted = true;
            while (!isQuitting())
            {
                std::this_thread::sleep_for(1ms);
            }
            mFinished = true;
        }

    private:
        std::atomic<bool>& mStarted;
        std::atomic<bool>& mFinished;
    };

    // run() just calls the function it was given
    class ThrowingThread : public LLThread
    {
    public:
        ThrowingThread(std::function<void()> body):
            LLThread("ThrowingThread"),
            mBody(std::move(body))
        {}

    protected:
        void run() override { mBody(); }

    private:
        std::function<void()> mBody;
    };

    template <typename FUNC>
    std::chrono::milliseconds time_of(FUNC func)
    {
        auto start = std::chrono::steady_clock::now();
        func();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
    }

    // Wait until counter has stayed the same for quiet. A thread that stops
    // counting has gone to sleep (as far as a test can tell: there's no way
    // to observe that directly). False if it's still counting at timeout.
    bool wait_for_quiet(const std::atomic<int>& counter,
                        std::chrono::milliseconds quiet = 50ms,
                        std::chrono::milliseconds timeout = 5000ms)
    {
        auto now = std::chrono::steady_clock::now();
        auto end = now + timeout;
        auto changed = now;
        int value = counter;
        while (now < end)
        {
            std::this_thread::sleep_for(1ms);
            now = std::chrono::steady_clock::now();
            int current = counter;
            if (current != value)
            {
                value = current;
                changed = now;
            }
            else if (now - changed >= quiet)
            {
                return true;
            }
        }
        return false;
    }

    template <typename PRED>
    bool wait_until(PRED pred, std::chrono::milliseconds timeout = 5000ms)
    {
        auto end = std::chrono::steady_clock::now() + timeout;
        while (!pred())
        {
            if (std::chrono::steady_clock::now() > end)
            {
                return false;
            }
            std::this_thread::sleep_for(1ms);
        }
        return true;
    }
} // anonymous namespace

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llthread_data
    {
    };
    typedef test_group<llthread_data> llthread_group;
    typedef llthread_group::object object;
    llthread_group llthreadgrp("LLThread");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("run() returns on its own");
        TestThread thread(false);
        ensure("stopped before start()", thread.isStopped());
        thread.start();
        ensure("never stopped", wait_until([&thread]() { return thread.isStopped(); }));
        ensure("crashed", !thread.isCrashed());
        // Once isStopped() is true, everything run() wrote must be visible
        // here. On x86 this passes even without that guarantee; run under
        // ThreadSanitizer to check it: a plain (non-atomic) mStatus makes
        // ThreadSanitizer report races on mStatus and mResult.
        ensure_equals("result", thread.mResult, "done");
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("shutdown() stops a running thread");
        TestThread thread(true);
        thread.start();
        ensure("never started", wait_until([&thread]() { return thread.mStarted.load(); }));
        ensure("stopped without being asked", !thread.isStopped());

        auto start = std::chrono::steady_clock::now();
        thread.shutdown();
        auto elapsed = std::chrono::steady_clock::now() - start;
        // shutdown() gives up and kills the thread after 6 s
        ensure("shutdown() waited for its timeout", elapsed < 3s);
        ensure("not stopped", thread.isStopped());
        ensure_equals("result", thread.mResult, "done");
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("asking a finished thread to quit");
        TestThread thread(false);
        thread.start();
        ensure("never stopped", wait_until([&thread]() { return thread.isStopped(); }));

        // A request to quit must not replace STOPPED with QUITTING, or
        // shutdown() would wait for a thread that has already gone. This
        // only checks the outcome: the thread has already stopped, so it
        // can't hit the race where the thread stops between setQuitting()'s
        // test and its store, which the compare-exchange closes.
        thread.requestQuit();
        ensure("quit request overwrote STOPPED", thread.isStopped());
        ensure("quitting after stopped", !thread.isQuitting());

        auto start = std::chrono::steady_clock::now();
        thread.shutdown();
        ensure("shutdown() of a finished thread waited", std::chrono::steady_clock::now() - start < 1s);
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("shutdown() wakes a thread sleeping in checkPause()");
        {
            // asleep because runCondition() is false
            WaitingThread thread;
            thread.start();
            ensure("never started", wait_until([&thread]() { return thread.mStarted.load(); }));
            // give run() time to reach checkPause() and go to sleep
            std::this_thread::sleep_for(50ms);
            ensure_equals("ran although not ready", thread.mPasses.load(), 0);

            // setQuitting() must wake it: once mStatus isn't RUNNING,
            // shouldSleep() is false
            ensure("shutdown() of a waiting thread waited for its timeout",
                   time_of([&thread]() { thread.shutdown(); }) < 3s);
            ensure("waiting thread not stopped", thread.isStopped());
        }
        {
            // asleep because paused
            WaitingThread thread;
            thread.setReady(true);
            thread.start();
            ensure("never ran", wait_until([&thread]() { return thread.mPasses.load() > 0; }));
            thread.pause();
            ensure("pause() didn't stop it", wait_for_quiet(thread.mPasses));

            ensure("shutdown() of a paused thread waited for its timeout",
                   time_of([&thread]() { thread.shutdown(); }) < 3s);
            ensure("paused thread not stopped", thread.isStopped());
        }
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("pause(), unpause() and wake()");
        WaitingThread thread;
        thread.start();
        ensure("never started", wait_until([&thread]() { return thread.mStarted.load(); }));
        std::this_thread::sleep_for(50ms);
        ensure_equals("ran although not ready", thread.mPasses.load(), 0);

        // setReady() makes runCondition() true and calls wake()
        thread.setReady(true);
        ensure("wake() didn't wake it", wait_until([&thread]() { return thread.mPasses.load() > 0; }));

        thread.pause();
        ensure("not paused", thread.isPaused());
        ensure("pause() didn't stop it", wait_for_quiet(thread.mPasses));
        int paused_at = thread.mPasses;

        thread.unpause();
        ensure("still paused", !thread.isPaused());
        ensure("unpause() didn't resume it",
               wait_until([&thread, paused_at]() { return thread.mPasses.load() > paused_at; }));

        // false runCondition() sends it back to sleep
        thread.setReady(false);
        ensure("runCondition() false didn't stop it", wait_for_quiet(thread.mPasses));

        thread.shutdown();
        ensure("not stopped", thread.isStopped());
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("shutdown() right after start()");
        // run() returns at once, so the thread sets STOPPED while shutdown()
        // is asking it to quit. Delaying shutdown() a little more each time
        // moves that request across the thread's exit, to try to land it
        // between setQuitting()'s test and its store. Without the
        // compare-exchange, QUITTING could overwrite STOPPED, and shutdown()
        // would wait 6 s and then cancel a thread that has already gone.
        // Whether that ever happens depends on timing, so passing doesn't
        // prove the race is gone.
        const int ITERATIONS = 200;
        for (int i = 0; i < ITERATIONS; ++i)
        {
            TestThread thread(false);
            thread.start();
            auto until = std::chrono::steady_clock::now() + std::chrono::microseconds(i);
            while (std::chrono::steady_clock::now() < until)
            {
                // spin: sleep_for() is far too coarse
            }
            auto elapsed = time_of([&thread]() { thread.shutdown(); });
            if (elapsed >= 1s)
            {
                fail(stringize("shutdown() #", i, " took ", elapsed.count(), " ms"));
            }
            ensure("not stopped", thread.isStopped());
        }
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("destroying a running thread stops it");
        std::atomic<bool> started{ false };
        std::atomic<bool> finished{ false };
        auto thread = std::make_unique<UntilQuitThread>(started, finished);
        thread->start();
        ensure("never started", wait_until([&started]() { return started.load(); }));

        // no shutdown(): ~LLThread() calls it
        ensure("~LLThread() waited for its timeout",
               time_of([&thread]() { thread.reset(); }) < 3s);
        // false if ~LLThread() gave up and cancelled the thread
        ensure("run() didn't finish", finished.load());
    }

    template<> template<>
    void object::test<8>()
    {
        set_test_name("an exception thrown by run()");
#if LL_WINDOWS
        // On Windows tryRun() catches only LLContinueError and std::bad_alloc;
        // anything else ends the process.
        skip("tryRun() doesn't pass exceptions to the main loop on Windows");
#else
        // tryRun() posts most exceptions to this queue, to be rethrown there
        LL::WorkQueue mainloop("mainloop");

        {
            // LLContinueError is logged and dropped
            ThrowingThread thread([]() { throw LLContinueError("continue from run()"); });
            thread.start();
            ensure("LLContinueError: never stopped",
                   wait_until([&thread]() { return thread.isStopped(); }));
            ensure_equals("LLContinueError posted to the main loop", mainloop.size(), size_t(0));
        }

        {
            // anything else is rethrown on the main loop
            ThrowingThread thread([]() { throw std::runtime_error("thrown by run()"); });
            thread.start();
            ensure("runtime_error: never stopped",
                   wait_until([&thread]() { return thread.isStopped(); }));
            // posted before the thread set STOPPED, so it's already queued
            ensure_equals("runtime_error not posted to the main loop", mainloop.size(), size_t(1));
            std::string what;
            try
            {
                mainloop.runOne();
            }
            catch (const std::runtime_error& e)
            {
                what = e.what();
            }
            ensure_equals("main loop didn't rethrow it", what, "thrown by run()");
        }
#endif
    }
} // namespace tut
