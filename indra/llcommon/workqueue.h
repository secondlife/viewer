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
#include "llexception.h"
#include "llinstancetracker.h"
#include "llinstancetrackersubclass.h"
#include "threadsafeschedule.h"
#include <chrono>
#include <exception>                // std::current_exception
#include <functional>               // std::function
#include <string>

namespace LL
{

/*****************************************************************************
*   WorkQueueBase: API for WorkQueue and WorkSchedule
*****************************************************************************/
    /**
     * A typical WorkQueue has a string name that can be used to find it.
     */
    class WorkQueueBase: public LLInstanceTracker<WorkQueueBase, std::string>
    {
    private:
        using super = LLInstanceTracker<WorkQueueBase, std::string>;

    public:
        using Work = std::function<void()>;
        using Closed = LLThreadSafeQueueInterrupt;
        // for runFor()
        using TimePoint = std::chrono::steady_clock::time_point;

        struct Error: public LLException
        {
            Error(const std::string& what): LLException(what) {}
        };

        /**
         * You may omit the WorkQueueBase name, in which case a unique name is
         * synthesized; for practical purposes that makes it anonymous.
         */
        WorkQueueBase(const std::string& name, bool auto_shutdown);

        virtual ~WorkQueueBase();

        /**
         * Since the point of WorkQueue is to pass work to some other worker
         * thread(s) asynchronously, it's important that it continue to exist
         * until the worker thread(s) have drained it. To communicate that
         * it's time for them to quit, close() the queue.
         */
        virtual void close() = 0;

        /**
         * WorkQueue supports multiple producers and multiple consumers. In
         * the general case it's misleading to test size(), since any other
         * thread might change it the nanosecond the lock is released. On that
         * basis, some might argue against publishing a size() method at all.
         *
         * But there are two specific cases in which a test based on size()
         * might be reasonable:
         *
         * * If you're the only producer, noticing that size() == 0 is
         *   meaningful.
         * * If you're the only consumer, noticing that size() > 0 is
         *   meaningful.
         */
        virtual size_t size() = 0;
        /// producer end: are we prevented from pushing any additional items?
        virtual bool isClosed() = 0;
        /// consumer end: are we done, is the queue entirely drained?
        virtual bool done() = 0;

        /*---------------------- fire and forget API -----------------------*/

        /**
         * post work, unless the queue is closed before we can post
         */
        virtual bool post(const Work&) = 0;

        /**
         * post work, unless the queue is full
         */
        virtual bool tryPost(const Work&) = 0;

        /**
         * Post work to another WorkQueue, which may or may not still exist
         * and be open. Support any post() overload. Return true if we were
         * able to post.
         */
        template <typename... ARGS>
        static bool postMaybe(weak_t target, ARGS&&... args);

        /*------------------------- handshake API --------------------------*/

        /**
         * Post work to another WorkQueue, requesting a specific callback to
         * be run on this WorkQueue on completion. Optional final argument is
         * TimePoint for WorkSchedule.
         *
         * Returns true if able to post, false if the other WorkQueue is
         * inaccessible.
         */
        template <typename CALLABLE, typename FOLLOWUP, typename... ARGS>
        bool postTo(weak_t target, CALLABLE&& callable, FOLLOWUP&& callback,
                    ARGS&&... args);

        /**
         * Post work to another WorkQueue, blocking the calling coroutine
         * until then, returning the result to caller on completion. Optional
         * final argument is TimePoint for WorkSchedule.
         *
         * In general, we assume that each thread's default coroutine is busy
         * servicing its WorkQueue or whatever. To try to prevent mistakes, we
         * forbid calling waitForResult() from a thread's default coroutine.
         */
        template <typename CALLABLE, typename... ARGS>
        auto waitForResult(CALLABLE&& callable, ARGS&&... args);

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
            LL_PROFILE_ZONE_SCOPED;
            return runUntil(TimePoint::clock::now() + timeslice);
        }

        /**
         * runUntil() is just like runFor(), only with a specific end time
         * instead of a timeslice duration.
         */
        bool runUntil(const TimePoint& until);

    protected:
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
        struct WaitForResult;
        /// specialize for CALLABLE returning void
        template <typename CALLABLE>
        struct WaitForResult<CALLABLE, void>;

        static void checkCoroutine(const std::string& method);
        static void error(const std::string& msg);
        static std::string makeName(const std::string& name);
        void callWork(const Work& work);

    private:
        virtual Work pop_() = 0;
        virtual bool tryPop_(Work&) = 0;

        // Name used for the LLApp event listener (empty if not registered)
        std::string mListenerName;
    };

/*****************************************************************************
*   WorkQueue: no timestamped task support
*****************************************************************************/
    class WorkQueue: public LLInstanceTrackerSubclass<WorkQueue, WorkQueueBase>
    {
    private:
        using super = LLInstanceTrackerSubclass<WorkQueue, WorkQueueBase>;

    public:
        /**
         * You may omit the WorkQueue name, in which case a unique name is
         * synthesized; for practical purposes that makes it anonymous.
         */
        WorkQueue(const std::string& name = std::string(), size_t capacity=1024, bool auto_shutdown = true);

        /**
         * Since the point of WorkQueue is to pass work to some other worker
         * thread(s) asynchronously, it's important that it continue to exist
         * until the worker thread(s) have drained it. To communicate that
         * it's time for them to quit, close() the queue.
         */
        void close() override;

        /**
         * WorkQueue supports multiple producers and multiple consumers. In
         * the general case it's misleading to test size(), since any other
         * thread might change it the nanosecond the lock is released. On that
         * basis, some might argue against publishing a size() method at all.
         *
         * But there are two specific cases in which a test based on size()
         * might be reasonable:
         *
         * * If you're the only producer, noticing that size() == 0 is
         *   meaningful.
         * * If you're the only consumer, noticing that size() > 0 is
         *   meaningful.
         */
        size_t size() override;
        /// producer end: are we prevented from pushing any additional items?
        bool isClosed() override;
        /// consumer end: are we done, is the queue entirely drained?
        bool done() override;

        /*---------------------- fire and forget API -----------------------*/

        /**
         * post work, unless the queue is closed before we can post
         */
        bool post(const Work&) override;

        /**
         * post work, unless the queue is full
         */
        bool tryPost(const Work&) override;

    private:
        using Queue = LLThreadSafeQueue<Work>;
        Queue mQueue;

        Work pop_() override;
        bool tryPop_(Work&) override;
    };

/*****************************************************************************
*   WorkSchedule: add support for timestamped tasks
*****************************************************************************/
    class WorkSchedule: public LLInstanceTrackerSubclass<WorkSchedule, WorkQueueBase>
    {
    private:
        using super = LLInstanceTrackerSubclass<WorkSchedule, WorkQueueBase>;
        using Queue = ThreadSafeSchedule<Work>;
        // helper for postEvery()
        template <typename Rep, typename Period, typename CALLABLE>
        class BackJack;

    public:
        using TimePoint = Queue::TimePoint;
        using TimedWork = Queue::TimeTuple;

        /**
         * You may omit the WorkSchedule name, in which case a unique name is
         * synthesized; for practical purposes that makes it anonymous.
         */
        WorkSchedule(const std::string& name = std::string(), size_t capacity=1024, bool auto_shutdown = true);

        /**
         * Since the point of WorkSchedule is to pass work to some other worker
         * thread(s) asynchronously, it's important that the WorkSchedule continue
         * to exist until the worker thread(s) have drained it. To communicate
         * that it's time for them to quit, close() the queue.
         */
        void close() override;

        /**
         * WorkSchedule supports multiple producers and multiple consumers. In
         * the general case it's misleading to test size(), since any other
         * thread might change it the nanosecond the lock is released. On that
         * basis, some might argue against publishing a size() method at all.
         *
         * But there are two specific cases in which a test based on size()
         * might be reasonable:
         *
         * * If you're the only producer, noticing that size() == 0 is
         *   meaningful.
         * * If you're the only consumer, noticing that size() > 0 is
         *   meaningful.
         */
        size_t size() override;
        /// producer end: are we prevented from pushing any additional items?
        bool isClosed() override;
        /// consumer end: are we done, is the queue entirely drained?
        bool done() override;

        /*---------------------- fire and forget API -----------------------*/

        /**
         * post work, unless the queue is closed before we can post
         */
        bool post(const Work& callable) override;

        /**
         * post work for a particular time, unless the queue is closed before
         * we can post
         */
        bool post(const Work& callable, const TimePoint& time);

        /**
         * post work, unless the queue is full
         */
        bool tryPost(const Work& callable) override;

        /**
         * post work for a particular time, unless the queue is full
         */
        bool tryPost(const Work& callable, const TimePoint& time);

        /**
         * Launch a callable returning bool that will trigger repeatedly at
         * specified interval, until the callable returns false.
         *
         * If you need to signal that callable from outside, DO NOT bind a
         * reference to a simple bool! That's not thread-safe. Instead, bind
         * an LLCond variant, e.g. LLOneShotCond or LLBoolCond.
         */
        template <typename Rep, typename Period, typename CALLABLE>
        bool postEvery(const std::chrono::duration<Rep, Period>& interval,
                       CALLABLE&& callable);

    private:
        Queue mQueue;

        Work pop_() override;
        bool tryPop_(Work&) override;
    };

    /**
     * BackJack is, in effect, a hand-rolled lambda, binding a WorkSchedule, a
     * CALLABLE that returns bool, a TimePoint and an interval at which to
     * relaunch it. As long as the callable continues returning true, BackJack
     * keeps resubmitting it to the target WorkQueue.
     */
    // Why is BackJack a class and not a lambda? Because, unlike a lambda, a
    // class method gets its own 'this' pointer -- which we need to resubmit
    // the whole BackJack callable.
    template <typename Rep, typename Period, typename CALLABLE>
    class WorkSchedule::BackJack
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

        // This operator() method, called by target WorkSchedule, is what
        // makes this object a Work item. Although WE require a callable
        // returning bool, WorkSchedule wants a void callable. We consume the
        // bool.
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

                // We're being called at this moment by the target WorkSchedule.
                // Assume it still exists, rather than checking the result of
                // lock().
                // Resubmit the whole *this callable: that's why we're a class
                // rather than a lambda. Allow moving *this so we can carry a
                // move-only callable; but naturally this statement must be
                // the last time we reference this instance, which may become
                // moved-from.
                auto target{ std::dynamic_pointer_cast<WorkSchedule>(mTarget.lock()) };
                // Discard bool return: once this queue is closed, oh well,
                // just stop
                target->post(std::move(*this), mStart);
            }
        }

    private:
        weak_t mTarget;
        TimePoint mStart;
        std::chrono::duration<Rep, Period> mInterval;
        CALLABLE mCallable;
    };

    template <typename Rep, typename Period, typename CALLABLE>
    bool WorkSchedule::postEvery(const std::chrono::duration<Rep, Period>& interval,
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
        return post(
            BackJack<Rep, Period, CALLABLE>(
                 getWeak(), TimePoint::clock::now(), interval, std::move(callable)));
    }

    /// general case: arbitrary C++ return type
    template <typename CALLABLE, typename FOLLOWUP, typename RETURNTYPE>
    struct WorkQueueBase::MakeReplyLambda
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
                mutable { callback(std::move(result)); };
        }
    };

    /// specialize for CALLABLE returning void
    template <typename CALLABLE, typename FOLLOWUP>
    struct WorkQueueBase::MakeReplyLambda<CALLABLE, FOLLOWUP, void>
    {
        auto operator()(CALLABLE&& callable, FOLLOWUP&& callback)
        {
            // Call the callable, which produces no result.
            std::forward<CALLABLE>(callable)();
            // Our completion callback is simply the caller's callback.
            return std::move(callback);
        }
    };

    template <typename CALLABLE, typename FOLLOWUP>
    auto WorkQueueBase::makeReplyLambda(CALLABLE&& callable, FOLLOWUP&& callback)
    {
        return MakeReplyLambda<CALLABLE, FOLLOWUP,
                               decltype(std::forward<CALLABLE>(callable)())>()
            (std::move(callable), std::move(callback));
    }

    template <typename CALLABLE, typename FOLLOWUP, typename... ARGS>
    bool WorkQueueBase::postTo(weak_t target, CALLABLE&& callable, FOLLOWUP&& callback,
                               ARGS&&... args)
    {
        LL_PROFILE_ZONE_SCOPED;
        // We're being asked to post to the WorkQueue at target.
        // target is a weak_ptr: have to lock it to check it.
        auto tptr = target.lock();
        if (! tptr)
            // can't post() if the target WorkQueue has been destroyed
            return false;

        // Here we believe target WorkQueue still exists. Post to it a
        // lambda that packages our callable, our callback and a weak_ptr
        // to this originating WorkQueue.
        return tptr->post(
            [reply = super::getWeak(),
             callable = std::move(callable),
             callback = std::move(callback)]
            () mutable
            {
                // Use postMaybe() below in case this originating WorkQueue
                // has been closed or destroyed. Remember, the outer lambda is
                // now running on a thread servicing the target WorkQueue, and
                // real time has elapsed since postTo()'s tptr->post() call.
                try
                {
                    // Make a reply lambda to repost to THIS WorkQueue.
                    // Delegate to makeReplyLambda() so we can partially
                    // specialize on void return.
                    postMaybe(reply, makeReplyLambda(std::move(callable), std::move(callback)));
                }
                catch (...)
                {
                    // Either variant of makeReplyLambda() is responsible for
                    // calling the caller's callable. If that throws, return
                    // the exception to the originating thread.
                    postMaybe(
                        reply,
                        // Bind the current exception to transport back to the
                        // originating WorkQueue. Once there, rethrow it.
                        [exc = std::current_exception()](){ std::rethrow_exception(exc); });
                }
            },
            // if caller passed a TimePoint, pass it along to post()
            std::forward<ARGS>(args)...);
    }

    template <typename... ARGS>
    bool WorkQueueBase::postMaybe(weak_t target, ARGS&&... args)
    {
        LL_PROFILE_ZONE_SCOPED;
        // target is a weak_ptr: have to lock it to check it
        auto tptr = target.lock();
        if (tptr)
        {
            return tptr->post(std::forward<ARGS>(args)...);
        }
        // target no longer exists
        return false;
    }

    /// general case: arbitrary C++ return type
    template <typename CALLABLE, typename RETURNTYPE>
    struct WorkQueueBase::WaitForResult
    {
        template <typename... ARGS>
        auto operator()(WorkQueueBase* self, CALLABLE&& callable, ARGS&&... args)
        {
            LLCoros::Promise<RETURNTYPE> promise;
            bool posted = self->post(
                // We dare to bind a reference to Promise because it's
                // specifically designed for cross-thread communication.
                [&promise, callable = std::move(callable)]()
                mutable {
                    try
                    {
                        // call the caller's callable and trigger promise with result
                        promise.set_value(callable());
                    }
                    catch (...)
                    {
                        promise.set_exception(std::current_exception());
                    }
                },
                // if caller passed a TimePoint, pass it to post()
                std::forward<ARGS>(args)...);
            if (! posted)
            {
                LLTHROW(WorkQueueBase::Closed());
            }
            auto future{ LLCoros::getFuture(promise) };
            // now, on the calling thread, wait for that result
            LLCoros::TempStatus st("waiting for WorkQueue::waitForResult()");
            return future.get();
        }
    };

    /// specialize for CALLABLE returning void
    template <typename CALLABLE>
    struct WorkQueueBase::WaitForResult<CALLABLE, void>
    {
        template <typename... ARGS>
        void operator()(WorkQueueBase* self, CALLABLE&& callable, ARGS&&... args)
        {
            LLCoros::Promise<void> promise;
            bool posted = self->post(
                // &promise is designed for cross-thread access
                [&promise, callable = std::move(callable)]()
                mutable {
                    try
                    {
                        callable();
                        promise.set_value();
                    }
                    catch (...)
                    {
                        promise.set_exception(std::current_exception());
                    }
                },
                // if caller passed a TimePoint, pass it to post()
                std::forward<ARGS>(args)...);
            if (! posted)
            {
                LLTHROW(WorkQueueBase::Closed());
            }
            auto future{ LLCoros::getFuture(promise) };
            // block until set_value()
            LLCoros::TempStatus st("waiting for void WorkQueue::waitForResult()");
            future.get();
        }
    };

    template <typename CALLABLE, typename... ARGS>
    auto WorkQueueBase::waitForResult(CALLABLE&& callable, ARGS&&... args)
    {
        checkCoroutine("waitForResult()");
        // derive callable's return type so we can specialize for void
        return WaitForResult<CALLABLE, decltype(std::forward<CALLABLE>(callable)())>()
            (this, std::forward<CALLABLE>(callable), std::forward<ARGS>(args)...);
    }

} // namespace LL

#endif /* ! defined(LL_WORKQUEUE_H) */
