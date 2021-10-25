/**
 * @file   workqueue.h
 * @author Nat Goodspeed
 * @date   2021-09-30
 * @brief  Queue used for inter-thread work passing.
 * 
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_WORKQUEUE_H)
#define LL_WORKQUEUE_H

#include "llcoros.h"
#include "llinstancetracker.h"
#include "threadsafeschedule.h"
#include <chrono>
#include <functional>               // std::function
#if __cplusplus >= 201703
#include <optional>
namespace stdopt = std;
#else
#include <boost/optional.hpp>
namespace stdopt = boost;
#endif
#include <string>
#include <utility>                  // std::pair
#include <vector>

namespace LL
{
    /**
     * A typical WorkQueue has a string name that can be used to find it.
     */
    class WorkQueue: public LLInstanceTracker<WorkQueue, std::string>
    {
    private:
        using super = LLInstanceTracker<WorkQueue, std::string>;

    public:
        using Work = std::function<void()>;

    private:
        using Queue = ThreadSafeSchedule<Work>;
        // helper for postEvery()
        template <typename Rep, typename Period, typename CALLABLE>
        class BackJack;

    public:
        using TimePoint = Queue::TimePoint;
        using TimedWork = Queue::TimeTuple;
        using Closed    = Queue::Closed;
        template <typename T>
        using optional  = stdopt::optional<T>;

        /**
         * You may omit the WorkQueue name, in which case a unique name is
         * synthesized; for practical purposes that makes it anonymous.
         */
        WorkQueue(const std::string& name = std::string());

        /**
         * Since the point of WorkQueue is to pass work to some other worker
         * thread(s) asynchronously, it's important that the WorkQueue continue
         * to exist until the worker thread(s) have drained it. To communicate
         * that it's time for them to quit, close() the queue.
         */
        void close();

        /// producer end: are we prevented from pushing any additional items?
        bool isClosed();
        /// consumer end: are we done, is the queue entirely drained?
        bool done();

        /*---------------------- fire and forget API -----------------------*/

        /// fire-and-forget, but at a particular (future?) time
        template <typename CALLABLE>
        void post(const TimePoint& time, CALLABLE&& callable)
        {
            // Defer reifying an arbitrary CALLABLE until we hit this method.
            // All other methods should accept CALLABLEs of arbitrary type to
            // avoid multiple levels of std::function indirection.
            mQueue.push(TimedWork(time, std::move(callable)));
        }

        /// fire-and-forget
        template <typename CALLABLE>
        void post(CALLABLE&& callable)
        {
            // We use TimePoint::clock::now() instead of TimePoint's
            // representation of the epoch because this WorkQueue may contain
            // a mix of past-due TimedWork items and TimedWork items scheduled
            // for the future. Sift this new item into the correct place.
            post(TimePoint::clock::now(), std::move(callable));
        }

        /**
         * Launch a callable returning bool that will trigger repeatedly at
         * specified interval, until the callable returns false.
         *
         * If you need to signal that callable from outside, DO NOT bind a
         * reference to a simple bool! That's not thread-safe. Instead, bind
         * an LLCond variant, e.g. LLOneShotCond or LLBoolCond.
         */
        template <typename Rep, typename Period, typename CALLABLE>
        void postEvery(const std::chrono::duration<Rep, Period>& interval,
                       CALLABLE&& callable);

        /*------------------------- handshake API --------------------------*/

        /**
         * Post work to another WorkQueue to be run at a specified time,
         * requesting a specific callback to be run on this WorkQueue on
         * completion.
         *
         * Returns true if able to post, false if the other WorkQueue is
         * inaccessible.
         */
        // Apparently some Microsoft header file defines a macro CALLBACK? The
        // natural template argument name CALLBACK produces very weird Visual
        // Studio compile errors that seem utterly unrelated to this source
        // code.
        template <typename CALLABLE, typename FOLLOWUP>
        bool postTo(weak_t target,
                    const TimePoint& time, CALLABLE&& callable, FOLLOWUP&& callback);

        /**
         * Post work to another WorkQueue, requesting a specific callback to
         * be run on this WorkQueue on completion.
         *
         * Returns true if able to post, false if the other WorkQueue is
         * inaccessible.
         */
        template <typename CALLABLE, typename FOLLOWUP>
        bool postTo(weak_t target, CALLABLE&& callable, FOLLOWUP&& callback)
        {
            return postTo(target, TimePoint::clock::now(),
                          std::move(callable), std::move(callback));
        }

        /**
         * Post work to another WorkQueue to be run at a specified time,
         * blocking the calling coroutine until then, returning the result to
         * caller on completion.
         *
         * REQUIRED:
         *
         * * The calling thread is the thread servicing 'this' WorkQueue.
         * * The calling coroutine is not the @em coroutine servicing this
         *   WorkQueue. We block the calling coroutine until the result is
         *   available. If this same coroutine is responsible for checking the
         *   local WorkQueue, the result will never be dequeued. In practice,
         *   to try to prevent mistakes, we forbid calling runOn() from a
         *   thread's default coroutine.
         *
         * Returns result if able to post, empty optional if the other
         * WorkQueue is inaccessible.
         *
         * If the passed callable has void return, runOn() returns bool true
         * if able to post, false if the other WorkQueue is inaccessible.
         */
        template <typename CALLABLE>
        auto runOn(weak_t target, const TimePoint& time, CALLABLE&& callable);

        /**
         * Post work to another WorkQueue, blocking the calling coroutine
         * until then, returning the result to caller on completion.
         *
         * REQUIRED:
         *
         * * The calling thread is the thread servicing 'this' WorkQueue.
         * * The calling coroutine is not the @em coroutine servicing this
         *   WorkQueue. We block the calling coroutine until the result is
         *   available. If this same coroutine is responsible for checking the
         *   local WorkQueue, the result will never be dequeued. In practice,
         *   to try to prevent mistakes, we forbid calling runOn() from a
         *   thread's default coroutine.
         *
         * Returns result if able to post, empty optional if the other
         * WorkQueue is inaccessible.
         *
         * If the passed callable has void return, runOn() returns bool true
         * if able to post, false if the other WorkQueue is inaccessible.
         */
        template <typename CALLABLE>
        auto runOn(weak_t target, CALLABLE&& callable)
        {
            return runOn(target, TimePoint::clock::now(), std::move(callable));
        }

        /*--------------------------- worker API ---------------------------*/

        /**
         * runUntilClose() pulls TimedWork items off this WorkQueue until the
         * queue is closed, at which point it returns. This would be the
         * typical entry point for a simple worker thread.
         */
        void runUntilClose();

        /**
         * runPending() runs all TimedWork items that are ready to run. It
         * returns true if the queue remains open, false if the queue has been
         * closed. This could be used by a thread whose primary purpose is to
         * serve the queue, but also wants to do other things with its idle time.
         */
        bool runPending();

        /**
         * runOne() runs at most one ready TimedWork item -- zero if none are
         * ready. It returns true if the queue remains open, false if the
         * queue has been closed.
         */
        bool runOne();

        /**
         * runFor() runs a subset of ready TimedWork items, until the
         * timeslice has been exceeded. It returns true if the queue remains
         * open, false if the queue has been closed. This could be used by a
         * busy main thread to lend a bounded few CPU cycles to this WorkQueue
         * without risking the WorkQueue blowing out the length of any one
         * frame.
         */
        template <typename Rep, typename Period>
        bool runFor(const std::chrono::duration<Rep, Period>& timeslice)
        {
            return runUntil(TimePoint::clock::now() + timeslice);
        }

        /**
         * runUntil() is just like runFor(), only with a specific end time
         * instead of a timeslice duration.
         */
        bool runUntil(const TimePoint& until);

    private:
        template <typename CALLABLE, typename FOLLOWUP>
        static auto makeReplyLambda(CALLABLE&& callable, FOLLOWUP&& callback);
        /// general case: arbitrary C++ return type
        template <typename CALLABLE, typename FOLLOWUP, typename RETURNTYPE>
        struct MakeReplyLambda;
        /// specialize for CALLABLE returning void
        template <typename CALLABLE, typename FOLLOWUP>
        struct MakeReplyLambda<CALLABLE, FOLLOWUP, void>;

        /// general case: arbitrary C++ return type
        template <typename CALLABLE, typename RETURNTYPE>
        struct RunOn;
        /// specialize for CALLABLE returning void
        template <typename CALLABLE>
        struct RunOn<CALLABLE, void>;

        static void checkCoroutine(const std::string& method);
        static void error(const std::string& msg);
        static std::string makeName(const std::string& name);
        void callWork(const Queue::DataTuple& work);
        void callWork(const Work& work);
        Queue mQueue;
    };

    /**
     * BackJack is, in effect, a hand-rolled lambda, binding a WorkQueue, a
     * CALLABLE that returns bool, a TimePoint and an interval at which to
     * relaunch it. As long as the callable continues returning true, BackJack
     * keeps resubmitting it to the target WorkQueue.
     */
    // Why is BackJack a class and not a lambda? Because, unlike a lambda, a
    // class method gets its own 'this' pointer -- which we need to resubmit
    // the whole BackJack callable.
    template <typename Rep, typename Period, typename CALLABLE>
    class WorkQueue::BackJack
    {
    public:
        // bind the desired data
        BackJack(weak_t target,
                 const TimePoint& start,
                 const std::chrono::duration<Rep, Period>& interval,
                 CALLABLE&& callable):
            mTarget(target),
            mStart(start),
            mInterval(interval),
            mCallable(std::move(callable))
        {}

        // Call by target WorkQueue -- note that although WE require a
        // callable returning bool, WorkQueue wants a void callable. We
        // consume the bool.
        void operator()()
        {
            // If mCallable() throws an exception, don't catch it here: if it
            // throws once, it's likely to throw every time, so it's a waste
            // of time to arrange to call it again.
            if (mCallable())
            {
                // Modify mStart to the new start time we desire. If we simply
                // added mInterval to now, we'd get actual timings of
                // (mInterval + slop), where 'slop' is the latency between the
                // previous mStart and the WorkQueue actually calling us.
                // Instead, add mInterval to mStart so that at least we
                // register our intent to fire at exact mIntervals.
                mStart += mInterval;

                // We're being called at this moment by the target WorkQueue.
                // Assume it still exists, rather than checking the result of
                // lock().
                // Resubmit the whole *this callable: that's why we're a class
                // rather than a lambda. Allow moving *this so we can carry a
                // move-only callable; but naturally this statement must be
                // the last time we reference this instance, which may become
                // moved-from.
                try
                {
                    mTarget.lock()->post(mStart, std::move(*this));
                }
                catch (const Closed&)
                {
                    // Once this queue is closed, oh well, just stop
                }
            }
        }

    private:
        weak_t mTarget;
        TimePoint mStart;
        std::chrono::duration<Rep, Period> mInterval;
        CALLABLE mCallable;
    };

    template <typename Rep, typename Period, typename CALLABLE>
    void WorkQueue::postEvery(const std::chrono::duration<Rep, Period>& interval,
                              CALLABLE&& callable)
    {
        if (interval.count() <= 0)
        {
            // It's essential that postEvery() be called with a positive
            // interval, since each call to BackJack posts another instance of
            // itself at (start + interval) and we order by target time. A
            // zero or negative interval would result in that BackJack
            // instance going to the head of the queue every time, immediately
            // ready to run. Effectively that would produce an infinite loop,
            // a denial of service on this WorkQueue.
            error("postEvery(interval) may not be 0");
        }
        // Instantiate and post a suitable BackJack, binding a weak_ptr to
        // self, the current time, the desired interval and the desired
        // callable.
        post(
            BackJack<Rep, Period, CALLABLE>(
                 getWeak(), TimePoint::clock::now(), interval, std::move(callable)));
    }

    /// general case: arbitrary C++ return type
    template <typename CALLABLE, typename FOLLOWUP, typename RETURNTYPE>
    struct WorkQueue::MakeReplyLambda
    {
        auto operator()(CALLABLE&& callable, FOLLOWUP&& callback)
        {
            // Call the callable in any case -- but to minimize
            // copying the result, immediately bind it into the reply
            // lambda. The reply lambda also binds the original
            // callback, so that when we, the originating WorkQueue,
            // finally receive and process the reply lambda, we'll
            // call the bound callback with the bound result -- on the
            // same thread that originally called postTo().
            return
                [result = std::forward<CALLABLE>(callable)(),
                 callback = std::move(callback)]
                ()
                { callback(std::move(result)); };
        }
    };

    /// specialize for CALLABLE returning void
    template <typename CALLABLE, typename FOLLOWUP>
    struct WorkQueue::MakeReplyLambda<CALLABLE, FOLLOWUP, void>
    {
        auto operator()(CALLABLE&& callable, FOLLOWUP&& callback)
        {
            // Call the callable, which produces no result.
            std::forward<CALLABLE>(callable)();
            // This reply lambda binds the original callback, so
            // that when we, the originating WorkQueue, finally
            // receive and process the reply lambda, we'll call
            // the bound callback -- on the same thread that
            // originally called postTo().
            return [callback = std::move(callback)](){ callback(); };
        }
    };

    template <typename CALLABLE, typename FOLLOWUP>
    auto WorkQueue::makeReplyLambda(CALLABLE&& callable, FOLLOWUP&& callback)
    {
        return MakeReplyLambda<CALLABLE, FOLLOWUP,
                               decltype(std::forward<CALLABLE>(callable)())>()
            (std::move(callable), std::move(callback));
    }

    template <typename CALLABLE, typename FOLLOWUP>
    bool WorkQueue::postTo(weak_t target,
                           const TimePoint& time, CALLABLE&& callable, FOLLOWUP&& callback)
    {
        // We're being asked to post to the WorkQueue at target.
        // target is a weak_ptr: have to lock it to check it.
        auto tptr = target.lock();
        if (! tptr)
            // can't post() if the target WorkQueue has been destroyed
            return false;

        // Here we believe target WorkQueue still exists. Post to it a
        // lambda that packages our callable, our callback and a weak_ptr
        // to this originating WorkQueue.
        tptr->post(
            time,
            [reply = super::getWeak(),
             callable = std::move(callable),
             callback = std::move(callback)]
            ()
            {
                // Make a reply lambda to repost to THIS WorkQueue.
                // Delegate to makeReplyLambda() so we can partially
                // specialize on void return.
                auto rlambda = makeReplyLambda(std::move(callable), std::move(callback));
                // Check if this originating WorkQueue still exists.
                // Remember, the outer lambda is now running on a thread
                // servicing the target WorkQueue, and real time has
                // elapsed since postTo()'s tptr->post() call.
                // reply is a weak_ptr: have to lock it to check it.
                auto rptr = reply.lock();
                if (rptr)
                {
                    // Only post reply lambda if the originating WorkQueue
                    // still exists. If not -- who would we tell? Log it?
                    try
                    {
                        rptr->post(std::move(rlambda));
                    }
                    catch (const Closed&)
                    {
                        // Originating WorkQueue might still exist, but
                        // might be Closed. Same thing: just discard the
                        // callback.
                    }
                }
            });
        // looks like we were able to post()
        return true;
    }

    /// general case: arbitrary C++ return type
    template <typename CALLABLE, typename RETURNTYPE>
    struct WorkQueue::RunOn
    {
        optional<RETURNTYPE> operator()(WorkQueue* self, weak_t target,
                                        const TimePoint& time, CALLABLE&& callable)
        {
            LLCoros::Promise<RETURNTYPE> promise;
            if (! self->postTo(
                    target,
                    time,
                    std::forward<CALLABLE>(callable),
                    // We dare to bind a reference to Promise because it's
                    // specifically intended for cross-thread synchronization.
                    [&promise]
                    (RETURNTYPE&& result)
                    {
                        promise.set_value(std::forward<RETURNTYPE>(result));
                    }))
            {
                // we couldn't even postTo(): return empty optional
                return {};
            }
            // we were able to post
            auto future{ LLCoros::getFuture(promise) };
            return { future.get(); }
        }
    };

    /// specialize for CALLABLE returning void
    template <typename CALLABLE>
    struct WorkQueue::RunOn<CALLABLE, void>
    {
        bool operator()(WorkQueue* self, weak_t target,
                        const TimePoint& time, CALLABLE&& callable)
        {
            LLCoros::Promise<void> promise;
            if (! self->postTo(
                    target,
                    time,
                    std::forward<CALLABLE>(callable),
                    // &promise is designed for cross-thread access
                    [&promise](){ promise.set_value(); }))
            {
                // we couldn't postTo()
                return false;
            }
            // we were able to post
            auto future{ LLCoros::getFuture(promise) };
            // block until set_value()
            future.get();
            return true;
        }
    };

    template <typename CALLABLE>
    auto WorkQueue::runOn(weak_t target, const TimePoint& time, CALLABLE&& callable)
    {
        checkCoroutine("runOn()");
        return RunOn<CALLABLE, decltype(std::forward<CALLABLE>(callable)())>()
            (this, target, time, std::forward<CALLABLE>(callable));
    }

} // namespace LL

#endif /* ! defined(LL_WORKQUEUE_H) */
