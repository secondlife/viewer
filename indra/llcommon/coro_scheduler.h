/**
 * @file   coro_scheduler.h
 * @author Nat Goodspeed
 * @date   2024-08-05
 * @brief  Custom scheduler for viewer's Boost.Fibers (aka coroutines)
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_CORO_SCHEDULER_H)
#define LL_CORO_SCHEDULER_H

#include "workqueue.h"
#include <boost/fiber/fiber.hpp>
#include <boost/fiber/algo/round_robin.hpp>

/**
 * llcoro::scheduler is specifically intended for the viewer's main thread.
 * Its role is to ensure that the main coroutine, responsible for UI
 * operations and coordinating everything else, doesn't get starved by
 * secondary coroutines -- however many of those there might be.
 *
 * The simple boost::fibers::algo::round_robin scheduler could result in
 * arbitrary time lag between resumptions of the main coroutine. Of course
 * every well-behaved viewer coroutine must be coded to yield before too much
 * real time has elapsed, but sheer volume of secondary coroutines could still
 * consume unreasonable real time before cycling back to the main coroutine.
 */

namespace llcoro
{

class scheduler: public boost::fibers::algo::round_robin
{
    using super = boost::fibers::algo::round_robin;
public:
    // If the main fiber is ready, and it's been at least this long since the
    // main fiber last ran, jump the main fiber to the head of the queue.
    static const F64 DEFAULT_TIMESLICE;

    scheduler();
    void awakened( boost::fibers::context*) noexcept override;
    boost::fibers::context* pick_next() noexcept override;

    static void use();

private:
    LL::WorkQueue::ptr_t getWorkQueue();

    // This is the fiber::id of the main fiber.
    boost::fibers::fiber::id mMainID;
    // This context* is nullptr while the main fiber is running or suspended,
    // but is set to the main fiber's context each time the main fiber is ready.
    boost::fibers::context* mMainCtx{};
    // Remember the context returned by the previous pick_next() call.
    boost::fibers::context* mPrevCtx{};
    // If it's been at least this long since the last time the main fiber got
    // control, jump it to the head of the queue.
    F64 mTimeslice{ DEFAULT_TIMESLICE };
    // Time when we resumed the most recently running fiber
    F64 mResumeTime{ 0 };
    // Timestamp as of the last time we suspended the main fiber.
    F64 mMainLast{ 0 };
    // Timestamp of start time
    F64 mStart{ 0 };
    // count context switches
    U64 mSwitches{ 0 };
    // WorkQueue for deferred logging
    LL::WorkQueue::weak_t mQueue;
};

} // namespace llcoro

#endif /* ! defined(LL_CORO_SCHEDULER_H) */
