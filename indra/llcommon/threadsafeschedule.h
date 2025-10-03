/**
 * @file   threadsafeschedule.h
 * @author Nat Goodspeed
 * @date   2021-10-02
 * @brief  ThreadSafeSchedule is an ordered queue in which every item has an
 *         associated timestamp.
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_THREADSAFESCHEDULE_H)
#define LL_THREADSAFESCHEDULE_H

#include "chrono.h"
#include "llexception.h"
#include "llthreadsafequeue.h"
#include "tuple.h"
#include <chrono>
#include <tuple>

namespace LL
{
    namespace ThreadSafeSchedulePrivate
    {
        using TimePoint = std::chrono::steady_clock::time_point;
        // Bundle consumer's data with a TimePoint to order items by timestamp.
        template <typename... Args>
        using TimestampedTuple = std::tuple<TimePoint, Args...>;

        // comparison functor for TimedTuples -- see TimedQueue comments
        struct ReverseTupleOrder
        {
            template <typename Tuple>
            bool operator()(const Tuple& left, const Tuple& right) const
            {
                return std::get<0>(left) > std::get<0>(right);
            }
        };

        template <typename... Args>
        using TimedQueue = PriorityQueueAdapter<
            TimestampedTuple<Args...>,
            // std::vector is the default storage for std::priority_queue,
            // have to restate to specify comparison template parameter
            std::vector<TimestampedTuple<Args...>>,
            // std::priority_queue uses a counterintuitive comparison
            // behavior: the default std::less comparator is used to present
            // the *highest* value as top(). So to sort by earliest timestamp,
            // we must invert by using >.
            ReverseTupleOrder>;
    } // namespace ThreadSafeSchedulePrivate

    /**
     * ThreadSafeSchedule is an ordered LLThreadSafeQueue in which every item
     * is given an associated timestamp. That is, TimePoint is implicitly
     * prepended to the std::tuple with the specified types.
     *
     * Items are popped in increasing chronological order. Moreover, any item
     * with a timestamp in the future is held back until
     * std::chrono::steady_clock reaches that timestamp.
     */
    template <typename... Args>
    class ThreadSafeSchedule:
        public LLThreadSafeQueue<ThreadSafeSchedulePrivate::TimestampedTuple<Args...>,
                                 ThreadSafeSchedulePrivate::TimedQueue<Args...>>
    {
    public:
        using DataTuple = std::tuple<Args...>;
        using TimeTuple = ThreadSafeSchedulePrivate::TimestampedTuple<Args...>;

    private:
        using super = LLThreadSafeQueue<TimeTuple, ThreadSafeSchedulePrivate::TimedQueue<Args...>>;
        using lock_t = typename super::lock_t;
        // VS 2017 needs this due to a bug:
        // https://developercommunity.visualstudio.com/t/cannot-access-protected-enumerator-of-enclosing-cl/203430
        enum pop_result { EMPTY=super::EMPTY, DONE=super::DONE, WAITING=super::WAITING, POPPED=super::POPPED };

    public:
        using Closed = LLThreadSafeQueueInterrupt;
        using TimePoint = ThreadSafeSchedulePrivate::TimePoint;
        using Clock = TimePoint::clock;

        ThreadSafeSchedule(size_t capacity=1024):
            super(capacity)
        {}

        /*----------------------------- push() -----------------------------*/
        /// explicitly pass TimeTuple
        using super::push;

        /// pass DataTuple with implicit now
        // This could be ambiguous for Args with a single type. Unfortunately
        // we can't enable_if an individual method with a condition based on
        // the *class* template arguments, only on that method's template
        // arguments. We could specialize this class for the single-Args case;
        // we could minimize redundancy by breaking out a common base class...
        void push(const DataTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            push(tuple_cons(Clock::now(), tuple));
        }

        /// individually pass each component of the TimeTuple
        void push(const TimePoint& time, Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            push(TimeTuple(time, std::forward<Args>(args)...));
        }

        /// individually pass every component except the TimePoint (implies now)
        // This could be ambiguous if the first specified template parameter
        // type is also TimePoint. We could try to disambiguate, but a simpler
        // approach would be for the caller to explicitly construct DataTuple
        // and call that overload.
        void push(Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            push(Clock::now(), std::forward<Args>(args)...);
        }

        /*--------------------------- tryPush() ----------------------------*/
        /// explicit TimeTuple
        using super::tryPush;

        /// DataTuple with implicit now
        bool tryPush(const DataTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPush(tuple_cons(Clock::now(), tuple));
        }

        /// individually pass components
        bool tryPush(const TimePoint& time, Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPush(TimeTuple(time, std::forward<Args>(args)...));
        }

        /// individually pass components with implicit now
        bool tryPush(Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPush(Clock::now(), std::forward<Args>(args)...);
        }

        /*-------------------------- tryPushFor() --------------------------*/
        /// explicit TimeTuple
        using super::tryPushFor;

        /// DataTuple with implicit now
        template <typename Rep, typename Period>
        bool tryPushFor(const std::chrono::duration<Rep, Period>& timeout,
                        const DataTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPushFor(timeout, tuple_cons(Clock::now(), tuple));
        }

        /// individually pass components
        template <typename Rep, typename Period>
        bool tryPushFor(const std::chrono::duration<Rep, Period>& timeout,
                        const TimePoint& time, Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPushFor(TimeTuple(time, std::forward<Args>(args)...));
        }

        /// individually pass components with implicit now
        template <typename Rep, typename Period>
        bool tryPushFor(const std::chrono::duration<Rep, Period>& timeout,
                        Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPushFor(Clock::now(), std::forward<Args>(args)...);
        }

        /*------------------------- tryPushUntil() -------------------------*/
        /// explicit TimeTuple
        using super::tryPushUntil;

        /// DataTuple with implicit now
        template <typename Clock, typename Duration>
        bool tryPushUntil(const std::chrono::time_point<Clock, Duration>& until,
                          const DataTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPushUntil(until, tuple_cons(Clock::now(), tuple));
        }

        /// individually pass components
        template <typename Clock, typename Duration>
        bool tryPushUntil(const std::chrono::time_point<Clock, Duration>& until,
                          const TimePoint& time, Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPushUntil(until, TimeTuple(time, std::forward<Args>(args)...));
        }

        /// individually pass components with implicit now
        template <typename Clock, typename Duration>
        bool tryPushUntil(const std::chrono::time_point<Clock, Duration>& until,
                          Args&&... args)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tryPushUntil(until, Clock::now(), std::forward<Args>(args)...);
        }

        /*----------------------------- pop() ------------------------------*/
        // Our consumer may or may not care about the timestamp associated
        // with each popped item, so we allow retrieving either DataTuple or
        // TimeTuple. One potential use would be to observe, and possibly
        // adjust for, the time lag between the item time and the actual
        // current time.

        /// pop DataTuple by value
        // It would be great to notice when sizeof...(Args) == 1 and directly
        // return the first (only) value, instead of making pop()'s caller
        // call std::get<0>(value). See push(DataTuple) remarks for why we
        // haven't yet jumped through those hoops.
        DataTuple pop()
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            return tuple_cdr(popWithTime());
        }

        /// pop TimeTuple by value
        TimeTuple popWithTime()
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            lock_t lock(super::mLock);
            // We can't just sit around waiting forever, given that there may
            // be items in the queue that are not yet ready but will *become*
            // ready in the near future. So in fact, with this class, every
            // pop() becomes a tryPopUntil(), constrained to the timestamp of
            // the head item. It almost doesn't matter what we specify for the
            // caller's time constraint -- all we really care about is the
            // head item's timestamp. Since pop() and popWithTime() are
            // defined to wait until either an item becomes available or the
            // queue is closed, loop until one of those things happens. The
            // constraint we pass just determines how often we'll loop while
            // waiting.
            TimeTuple tt;
            while (true)
            {
                // Pick a point suitably far into the future.
                TimePoint until = TimePoint::clock::now() + std::chrono::hours(24);
                pop_result popped = tryPopUntil_(lock, until, tt);
                if (popped == POPPED)
                    return tt;

                // DONE: throw, just as super::pop() does
                if (popped == DONE)
                {
                    LLTHROW(LLThreadSafeQueueInterrupt());
                }
                // WAITING: we've still got items to drain.
                // EMPTY: not closed, so it's worth waiting for more items.
                // Either way, loop back to wait.
            }
        }

        // We can use tryPop(TimeTuple&) just as it stands; the only behavior
        // difference is in our canPop() override method.
        using super::tryPop;

        /// tryPop(DataTuple&)
        bool tryPop(DataTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            TimeTuple tt;
            if (! super::tryPop(tt))
                return false;
            tuple = tuple_cdr(std::move(tt));
            return true;
        }

        /// for when Args has exactly one type
        bool tryPop(typename std::tuple_element<1, TimeTuple>::type& value)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            TimeTuple tt;
            if (! super::tryPop(tt))
                return false;
            value = std::get<1>(std::move(tt));
            return true;
        }

        /// tryPopFor()
        template <typename Rep, typename Period, typename Tuple>
        bool tryPopFor(const std::chrono::duration<Rep, Period>& timeout, Tuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            // It's important to use OUR tryPopUntil() implementation, rather
            // than delegating immediately to our base class.
            return tryPopUntil(Clock::now() + timeout, tuple);
        }

        /// tryPopUntil(TimeTuple&)
        template <typename Clock, typename Duration>
        bool tryPopUntil(const std::chrono::time_point<Clock, Duration>& until,
                         TimeTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            // super::tryPopUntil() wakes up when an item becomes available or
            // we hit 'until', whichever comes first. Thing is, the current
            // head of the queue could become ready sooner than either of
            // those events, and we need to deliver it as soon as it does.
            // Don't wait past the TimePoint of the head item.
            // Naturally, lock the queue before peeking at mStorage.
            return super::tryLockUntil(
                until,
                [this, until, &tuple](lock_t& lock)
                {
                    // Use our time_point_cast to allow for 'until' that's a
                    // time_point type other than TimePoint.
                    return POPPED ==
                        tryPopUntil_(lock, LL::time_point_cast<TimePoint>(until), tuple);
                });
        }

        pop_result tryPopUntil_(lock_t& lock, const TimePoint& until, TimeTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            TimePoint adjusted = until;
            if (! super::mStorage.empty())
            {
                LL_PROFILE_ZONE_NAMED("tpu - adjust");
                // use whichever is earlier: the head item's timestamp, or
                // the caller's limit
                adjusted = min(std::get<0>(super::mStorage.front()), adjusted);
            }
            // now delegate to base-class tryPopUntil_()
            pop_result popped;
            {
                LL_PROFILE_ZONE_NAMED("tpu - super");
                while ((popped = pop_result(super::tryPopUntil_(lock, adjusted, tuple))) == WAITING)
                {
                    // If super::tryPopUntil_() returns WAITING, it means there's
                    // a head item, but it's not yet time. But it's worth looping
                    // back to recheck.
                }
            }
            return popped;
        }

        /// tryPopUntil(DataTuple&)
        template <typename Clock, typename Duration>
        bool tryPopUntil(const std::chrono::time_point<Clock, Duration>& until,
                         DataTuple& tuple)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            TimeTuple tt;
            if (! tryPopUntil(until, tt))
                return false;
            tuple = tuple_cdr(std::move(tt));
            return true;
        }

        /// for when Args has exactly one type
        template <typename Clock, typename Duration>
        bool tryPopUntil(const std::chrono::time_point<Clock, Duration>& until,
                         typename std::tuple_element<1, TimeTuple>::type& value)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            TimeTuple tt;
            if (! tryPopUntil(until, tt))
                return false;
            value = std::get<1>(std::move(tt));
            return true;
        }

        /*------------------------------ etc. ------------------------------*/
        // We can't hide items that aren't yet ready because we can't traverse
        // the underlying priority_queue: it has no iterators, only top(). So
        // a consumer could observe size() > 0 and yet tryPop() returns false.
        // Shrug, in a multi-consumer scenario that would be expected behavior.
        using super::size;
        // open/closed state
        using super::close;
        using super::isClosed;
        using super::done;

    private:
        // this method is called by base class pop_() every time we're
        // considering whether to deliver the current head element
        bool canPop(const TimeTuple& head) const override
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            // an item with a future timestamp isn't yet ready to pop
            // (should we add some slop for overhead?)
            return std::get<0>(head) <= Clock::now();
        }
    };

} // namespace LL

#endif /* ! defined(LL_THREADSAFESCHEDULE_H) */
