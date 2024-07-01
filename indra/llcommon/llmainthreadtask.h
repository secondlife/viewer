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

#include "llthread.h"
#include "workqueue.h"

/**
 * LLMainThreadTask provides a way to perform some task specifically on the
 * main thread, waiting for it to complete. A task consists of a C++ nullary
 * invocable (i.e. any callable that requires no arguments) with arbitrary
 * return type.
 *
 * Instead of instantiating LLMainThreadTask, pass your invocable to its
 * static dispatch() method. dispatch() returns the result of calling your
 * task. (Or, if your task throws an exception, dispatch() throws that
 * exception.)
 *
 * When you call dispatch() on the main thread (as determined by
 * on_main_thread() in llthread.h), it simply calls your task and returns the
 * result.
 *
 * When you call dispatch() on a secondary thread, it posts your task to
 * gMainloopWork, the WorkQueue serviced by the main thread, using
 * WorkQueue::waitForResult() to block the caller. Next time the main loop
 * calls gMainloopWork.runFor(), your task will be run, and waitForResult()
 * will return its result.
 */
class LLMainThreadTask
{
private:
    // Don't instantiate this class -- use dispatch() instead.
    LLMainThreadTask() {}

public:
    /// dispatch() is the only way to invoke this functionality.
    template <typename CALLABLE>
    static auto dispatch(CALLABLE&& callable) -> decltype(callable())
    {
        if (on_main_thread())
        {
            // we're already running on the main thread, perfect
            return callable();
        }
        else
        {
            auto queue{ LL::WorkQueue::getInstance("mainloop") };
            // If this needs a null check and a message, please introduce a
            // method in the .cpp file so consumers of this header don't drag
            // in llerror.h.
            // Use waitForResult_() so dispatch() can be used even from the
            // calling thread's default coroutine.
            return queue->waitForResult_(std::forward<CALLABLE>(callable));
        }
    }
};

#endif /* ! defined(LL_LLMAINTHREADTASK_H) */
