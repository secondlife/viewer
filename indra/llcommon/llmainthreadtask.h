/**
 * @file   llmainthreadtask.h
 * @author Nat Goodspeed
 * @date   2019-12-04
 * @brief  LLMainThreadTask dispatches work to the main thread. When invoked on
 *         the main thread, it performs the work inline.
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLMAINTHREADTASK_H)
#define LL_LLMAINTHREADTASK_H

#include "lleventtimer.h"
#include "llthread.h"
#include "lockstatic.h"
#include "llmake.h"
#include <future>
#include <type_traits>              // std::result_of
#include <boost/signals2/dummy_mutex.hpp>

/**
 * LLMainThreadTask provides a way to perform some task specifically on the
 * main thread, waiting for it to complete. A task consists of a C++ nullary
 * invocable (i.e. any callable that requires no arguments) with arbitrary
 * return type.
 *
 * Instead of instantiating LLMainThreadTask, pass your invocable to its
 * static dispatch() method. dispatch() returns the result of calling your
 * task. (Or, if your task throws an exception, dispatch() throws that
 * exception. See std::packaged_task.)
 *
 * When you call dispatch() on the main thread (as determined by
 * on_main_thread() in llthread.h), it simply calls your task and returns the
 * result.
 *
 * When you call dispatch() on a secondary thread, it instantiates an
 * LLEventTimer subclass scheduled immediately. Next time the main loop calls
 * LLEventTimer::updateClass(), your task will be run, and LLMainThreadTask
 * will fulfill a future with its result. Meanwhile the requesting thread
 * blocks on that future. As soon as it is set, the requesting thread wakes up
 * with the task result.
 *
 * Under some circumstances it's necessary for the calling thread to hold a
 * lock until the task has been scheduled -- yet important to release the lock
 * while waiting for the result. If you pass a LockStatic<T> to dispatch(),
 * a secondary thread will unlock it before blocking on the future. (The main
 * thread simply holds the lock for the duration of the task.)
 */
class LLMainThreadTask
{
private:
    // Don't instantiate this class -- use dispatch() instead.
    LLMainThreadTask() {}
    // If our caller doesn't explicitly pass a LockStatic<something>, make a
    // fake one.
    struct Static
    {
        boost::signals2::dummy_mutex mMutex;
    };
    typedef llthread::LockStatic<Static> LockStatic;

public:
    /// dispatch() is the only way to invoke this functionality.
    /// If you call it with a LockStatic<something>, dispatch() unlocks it
    /// before blocking for the result.
    template <typename Static, typename CALLABLE>
    static auto dispatch(llthread::LockStatic<Static>& lk, CALLABLE&& callable)
        -> decltype(callable())
    {
        if (on_main_thread())
        {
            // we're already running on the main thread, perfect
            return callable();
        }
        else
        {
            // It's essential to construct LLEventTimer subclass instances on
            // the heap because, on completion, LLEventTimer deletes them.
            // Once we enable C++17, we can use Class Template Argument
            // Deduction. Until then, use llmake_heap().
            auto* task = llmake_heap<Task>(std::forward<CALLABLE>(callable));
            // The moment we construct a new LLEventTimer subclass object, its
            // tick() method might get called. However, its tick() method
            // might depend on something locked by the passed LockStatic.
            // Unlock it so tick() can proceed.
            lk.unlock();
            auto future = task->mTask.get_future();
            // Now simply block on the future.
            return future.get();
        }
    }

    /// You can call dispatch() without a LockStatic<something>.
    template <typename CALLABLE>
    static auto dispatch(CALLABLE&& callable) -> decltype(callable())
    {
        LockStatic lk;
        return dispatch(lk, std::forward<CALLABLE>(callable));
    }

private:
    template <typename CALLABLE>
    struct Task: public LLEventTimer
    {
        Task(CALLABLE&& callable):
            // no wait time: call tick() next chance we get
            LLEventTimer(0),
            mTask(std::forward<CALLABLE>(callable))
        {}
        BOOL tick() override
        {
            // run the task on the main thread, will populate the future
            // obtained by get_future()
            mTask();
            // tell LLEventTimer we're done (one shot)
            return TRUE;
        }
        // Given arbitrary CALLABLE, which might be a lambda, how are we
        // supposed to obtain its signature for std::packaged_task? It seems
        // redundant to have to add an argument list to engage result_of, then
        // add the argument list again to complete the signature. At least we
        // only support a nullary CALLABLE.
        std::packaged_task<typename std::result_of<CALLABLE()>::type()> mTask;
    };
};

#endif /* ! defined(LL_LLMAINTHREADTASK_H) */
