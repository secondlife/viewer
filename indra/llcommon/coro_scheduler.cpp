/**
 * @file   coro_scheduler.cpp
 * @author Nat Goodspeed
 * @date   2024-08-05
 * @brief  Implementation for llcoro::scheduler.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "coro_scheduler.h"
// STL headers
// std headers
#include <iomanip>
// external library headers
#include <boost/fiber/operations.hpp>
// other Linden headers
#include "llcallbacklist.h"
#include "llcoros.h"
#include "lldate.h"
#include "llerror.h"

namespace llcoro
{

const F64 scheduler::DEFAULT_TIMESLICE{ LL::Timers::DEFAULT_TIMESLICE };

const std::string qname("General");

scheduler::scheduler():
    // Since use_scheduling_algorithm() must be called before any other
    // Boost.Fibers operations, we can assume that the calling fiber is in
    // fact the main fiber.
    mMainID(boost::this_fiber::get_id()),
    mStart(LLDate::now().secondsSinceEpoch()),
    mQueue(LL::WorkQueue::getInstance(qname))
{}

void scheduler::awakened( boost::fibers::context* ctx) noexcept
{
    if (ctx->get_id() == mMainID)
    {
        // If the fiber that just came ready is the main fiber, record its
        // pointer.
        llassert(! mMainCtx);
        mMainCtx = ctx;
    }
    // Delegate to round_robin::awakened() as usual, even for the main fiber.
    // This way, as long as other fibers don't take too long, we can just let
    // normal round_robin processing pass control to the main fiber.
    super::awakened(ctx);
}

boost::fibers::context* scheduler::pick_next() noexcept
{
    auto now = LLDate::now().secondsSinceEpoch();
    // count calls to pick_next()
    ++mSwitches;
    // pick_next() is called when the previous fiber has suspended, and we
    // need to pick another. Did the previous pick_next() call pick the main
    // fiber? (Or is this the first pick_next() call?) If so, it's the main
    // fiber that just suspended.
    if ((! mPrevCtx) || mPrevCtx->get_id() == mMainID)
    {
        mMainLast = now;
    }
    else
    {
        // How long did we spend in the fiber that just suspended?
        // Don't bother with long runs of the main fiber, since (a) it happens
        // pretty often and (b) it's moderately likely that we've reached here
        // from the canonical yield at the top of mainloop, and what we'd want
        // to know about is whatever the main fiber was doing in the
        // *previous* iteration of mainloop.
        F64 elapsed{ now - mResumeTime };
        LLCoros::CoroData& data{ LLCoros::get_CoroData(mPrevCtx->get_id()) };
        // Find iterator to the first mHistogram key greater than elapsed.
        auto past = data.mHistogram.upper_bound(elapsed);
        // If the smallest key (mHistogram.begin()->first) is greater than
        // elapsed, then we need not bother with this timeslice.
        if (past != data.mHistogram.begin())
        {
            // Here elapsed was greater than at least one key. Back off to the
            // previous entry and increment that count. If it's end(), backing
            // off gets us the last entry -- assuming mHistogram isn't empty.
            llassert(! data.mHistogram.empty());
            ++(--past)->second;
            LL::WorkQueue::ptr_t queue{ getWorkQueue() };
            // make sure the queue exists
            if (queue)
            {
                // If it proves difficult to track down *why* the fiber spent so
                // much time, consider also binding and reporting
                // boost::stacktrace::stacktrace().
                queue->post(
                    [name=data.getName(), elapsed]
                    {
                        LL_WARNS_ONCE("LLCoros.scheduler")
                            << "Coroutine " << name << " ran for "
                            << elapsed << " seconds" << LL_ENDL;
                    });
            }
        }
    }

    boost::fibers::context* next;

    // When the main fiber is ready, and it's been more than mTimeslice since
    // the main fiber last ran, it's time to intervene.
    F64 elapsed(now - mMainLast);
    if (mMainCtx && elapsed > mTimeslice)
    {
        // We claim that the main fiber is not only stored in mMainCtx, but is
        // also queued (somewhere) in our ready list.
        llassert(mMainCtx->ready_is_linked());
        // The usefulness of a doubly-linked list is that, given only a
        // pointer to an item, we can unlink it.
        mMainCtx->ready_unlink();
        // Instead of delegating to round_robin::pick_next() to pop the head
        // of the queue, override by returning mMainCtx.
        next = mMainCtx;

        /*------------------------- logging stuff --------------------------*/
        // Unless this log tag is enabled, don't even bother posting.
        LL_DEBUGS("LLCoros.scheduler") << " ";
        // This feature is inherently hard to verify. The logging in the
        // lambda below seems useful, but also seems like a lot of overhead
        // for a coroutine context switch. Try posting the logging lambda to a
        // ThreadPool to offload that overhead. However, if this is still
        // taking an unreasonable amount of context-switch time, this whole
        // passage could be skipped.

        // Record this event for logging, but push it off to a thread pool to
        // perform that work.
        LL::WorkQueue::ptr_t queue{ getWorkQueue() };
        // The work queue we're looking for might not exist right now.
        if (queue)
        {
            // Bind values. Do NOT bind 'this' to avoid cross-thread access!
            // It would be interesting to know from what queue position we
            // unlinked the main fiber, out of how many in the ready list.
            // Unfortunately round_robin::rqueue_ is private, not protected,
            // so we have no access.
            queue->post(
                [switches=mSwitches, start=mStart, elapsed, now]
                {
                    U32 runtime(U32(now) - U32(start));
                    U32 minutes(runtime / 60u);
                    U32 seconds(runtime % 60u);
                    // use stringize to avoid lasting side effects to the
                    // logging ostream
                    LL_DEBUGS("LLCoros.scheduler")
                        << "At time "
                        << stringize(minutes, ":", std::setw(2), std::setfill('0'), seconds)
                        << " (" << switches << " switches), coroutines took "
                        << stringize(std::setprecision(4), elapsed)
                        << " sec, main coroutine jumped queue"
                        << LL_ENDL;
                });
        }
        LL_ENDL;
        /*----------------------- end logging stuff ------------------------*/
    }
    else
    {
        // Either the main fiber isn't yet ready, or it hasn't yet been
        // mTimeslice seconds since the last time the main fiber ran. Business
        // as usual.
        next = super::pick_next();
    }

    // super::pick_next() could also have returned the main fiber, which is
    // why this is a separate test instead of being folded into the override
    // case above.
    if (next && next->get_id() == mMainID)
    {
        // we're about to resume the main fiber: it's no longer "ready"
        mMainCtx = nullptr;
    }
    mPrevCtx = next;
    // remember when we resumed this fiber so our next call can measure how
    // long the previous resumption was
    mResumeTime = LLDate::now().secondsSinceEpoch();
    return next;
}

LL::WorkQueue::ptr_t scheduler::getWorkQueue()
{
    // Cache a weak_ptr to our target work queue, presuming that
    // std::weak_ptr::lock() is cheaper than WorkQueue::getInstance().
    LL::WorkQueue::ptr_t queue{ mQueue.lock() };
    // We probably started before the relevant WorkQueue was created.
    if (! queue)
    {
        // Try again to locate the specified WorkQueue.
        queue = LL::WorkQueue::getInstance(qname);
        mQueue = queue;
    }
    return queue;
}

void scheduler::use()
{
    boost::fibers::use_scheduling_algorithm<scheduler>();
}

} // namespace llcoro
