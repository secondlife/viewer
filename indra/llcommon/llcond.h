/**
 * @file   llcond.h
 * @author Nat Goodspeed
 * @date   2019-07-10
 * @brief  LLCond is a wrapper around condition_variable to encapsulate the
 *         obligatory condition_variable usage pattern. We also provide
 *         simplified versions LLScalarCond, LLBoolCond and LLOneShotCond.
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCOND_H)
#define LL_LLCOND_H

#include "llunits.h"
#include "llcoros.h"
#include LLCOROS_MUTEX_HEADER
#include "mutex.h"
#include <chrono>

/**
 * LLCond encapsulates the pattern required to use a condition_variable. It
 * bundles subject data, a mutex and a condition_variable: the three required
 * data objects. It provides wait() methods analogous to condition_variable,
 * but using the contained condition_variable and the contained mutex. It
 * provides modify() methods accepting an invocable to safely modify the
 * contained data and notify waiters. These methods implicitly perform the
 * required locking.
 *
 * The generic LLCond template assumes that DATA might be a struct or class.
 * For a scalar DATA type, consider LLScalarCond instead. For specifically
 * bool, consider LLBoolCond.
 *
 * Use of LLCoros::ConditionVariable makes LLCond work between
 * coroutines as well as between threads.
 */
template <typename DATA>
class LLCond
{
public:
    typedef DATA value_type;

private:
    // This is the DATA controlled by the condition_variable.
    value_type mData;
    // condition_variable must be used in conjunction with a mutex. Use
    // LLCoros::Mutex instead of std::mutex because the latter blocks
    // the entire calling thread, whereas the former blocks only the current
    // coroutine within the calling thread. Yet LLCoros::Mutex is safe to
    // use across threads as well: it subsumes std::mutex functionality.
    LLCoros::Mutex mMutex;
    // Use LLCoros::ConditionVariable for the same reason.
    LLCoros::ConditionVariable mCond;

public:
    /// LLCond can be explicitly initialized with a specific value for mData if
    /// desired.
    LLCond(const value_type& init=value_type()):
        mData(init)
    {}

    /// LLCond is move-only
    LLCond(const LLCond&) = delete;
    LLCond& operator=(const LLCond&) = delete;

    /// get() returns a const reference to the stored DATA. The only way to
    /// get a non-const reference -- to modify the stored DATA -- is via
    /// update_one() or update_all().
    const value_type& get() const { return mData; }

    /**
     * Pass update_one() an invocable accepting non-const (DATA&). The
     * invocable will presumably modify the referenced DATA. update_one()
     * will lock the mutex, call the invocable and then call notify_one() on
     * the condition_variable.
     *
     * For scalar DATA, it's simpler to use LLScalarCond::set_one(). Use
     * update_one() when DATA is a struct or class.
     */
    template <typename MODIFY>
    void update_one(MODIFY modify)
    {
        { // scope of lock can/should end before notify_one()
            LLCoros::LockType lk(mMutex);
            modify(mData);
        }
        mCond.notify_one();
    }

    /**
     * Pass update_all() an invocable accepting non-const (DATA&). The
     * invocable will presumably modify the referenced DATA. update_all()
     * will lock the mutex, call the invocable and then call notify_all() on
     * the condition_variable.
     *
     * For scalar DATA, it's simpler to use LLScalarCond::set_all(). Use
     * update_all() when DATA is a struct or class.
     */
    template <typename MODIFY>
    void update_all(MODIFY modify)
    {
        { // scope of lock can/should end before notify_all()
            LLCoros::LockType lk(mMutex);
            modify(mData);
        }
        mCond.notify_all();
    }

    /**
     * Pass wait() a predicate accepting (const DATA&), returning bool. The
     * predicate returns true when the condition for which it is waiting has
     * been satisfied, presumably determined by examining the referenced DATA.
     * wait() locks the mutex and, until the predicate returns true, calls
     * wait() on the condition_variable.
     */
    template <typename Pred>
    void wait(Pred pred)
    {
        LLCoros::LockType lk(mMutex);
        // We must iterate explicitly since the predicate accepted by
        // condition_variable::wait() requires a different signature:
        // condition_variable::wait() calls its predicate with no arguments.
        // Fortunately, the loop is straightforward.
        // We advise the caller to pass a predicate accepting (const DATA&).
        // But what if they instead pass a predicate accepting non-const
        // (DATA&)? Such a predicate could modify mData, which would be Bad.
        // Forbid that.
        while (! pred(const_cast<const value_type&>(mData)))
        {
            mCond.wait(lk);
        }
    }

    /**
     * Pass wait_for() a chrono::duration, indicating how long we're willing
     * to wait, and a predicate accepting (const DATA&), returning bool. The
     * predicate returns true when the condition for which it is waiting has
     * been satisfied, presumably determined by examining the referenced DATA.
     * wait_for() locks the mutex and, until the predicate returns true, calls
     * wait_for() on the condition_variable. wait_for() returns false if
     * condition_variable::wait_for() timed out without the predicate
     * returning true.
     */
    template <typename Rep, typename Period, typename Pred>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout_duration, Pred pred)
    {
        // Instead of replicating wait_until() logic, convert duration to
        // time_point and just call wait_until().
        // An implementation in which we repeatedly called
        // condition_variable::wait_for() with our passed duration would be
        // wrong! We'd keep pushing the timeout time farther and farther into
        // the future. This way, we establish a definite timeout time and
        // stick to it.
        return wait_until(std::chrono::steady_clock::now() + timeout_duration, pred);
    }

    /**
     * This wait_for() overload accepts F32Milliseconds as the duration. Any
     * duration unit defined in llunits.h is implicitly convertible to
     * F32Milliseconds. The semantics of this method are the same as the
     * generic wait_for() method.
     */
    template <typename Pred>
    bool wait_for(F32Milliseconds timeout_duration, Pred pred)
    {
        return wait_for(convert(timeout_duration), pred);
    }

protected:
    // convert F32Milliseconds to a chrono::duration
    auto convert(F32Milliseconds duration)
    {
        // std::chrono::milliseconds doesn't like to be constructed from a
        // float (F32), rubbing our nose in the thought that
        // std::chrono::duration::rep is probably integral. Therefore
        // converting F32Milliseconds to std::chrono::milliseconds would lose
        // precision. Use std::chrono::microseconds instead. Extract the F32
        // milliseconds from F32Milliseconds, scale to microseconds, construct
        // std::chrono::microseconds from that value.
        return std::chrono::microseconds{ std::chrono::microseconds::rep(duration.value() * 1000) };
    }

private:
    /**
     * Pass wait_until() a chrono::time_point, indicating the time at which we
     * should stop waiting, and a predicate accepting (const DATA&), returning
     * bool. The predicate returns true when the condition for which it is
     * waiting has been satisfied, presumably determined by examining the
     * referenced DATA. wait_until() locks the mutex and, until the predicate
     * returns true, calls wait_until() on the condition_variable.
     * wait_until() returns false if condition_variable::wait_until() timed
     * out without the predicate returning true.
     *
     * Originally this class and its subclasses published wait_until() methods
     * corresponding to each wait_for() method. But that raised all sorts of
     * fascinating questions about the time zone of the passed time_point:
     * local time? server time? UTC? The bottom line is that for LLCond
     * timeout purposes, we really shouldn't have to care -- timeout duration
     * is all we need. This private method remains because it's the simplest
     * way to support iteratively waiting across spurious wakeups while
     * honoring a fixed timeout.
     */
    template <typename Clock, typename Duration, typename Pred>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time, Pred pred)
    {
        LLCoros::LockType lk(mMutex);
        // We advise the caller to pass a predicate accepting (const DATA&).
        // But what if they instead pass a predicate accepting non-const
        // (DATA&)? Such a predicate could modify mData, which would be Bad.
        // Forbid that.
        while (! pred(const_cast<const value_type&>(mData)))
        {
            if (LLCoros::cv_status::timeout == mCond.wait_until(lk, timeout_time))
            {
                // It's possible that wait_until() timed out AND the predicate
                // became true more or less simultaneously. Even though
                // wait_until() timed out, check the predicate one more time.
                return pred(const_cast<const value_type&>(mData));
            }
        }
        return true;
    }
};

template <typename DATA>
class LLScalarCond: public LLCond<DATA>
{
    using super = LLCond<DATA>;

public:
    using typename super::value_type;
    using super::get;
    using super::wait;
    using super::wait_for;

    /// LLScalarCond can be explicitly initialized with a specific value for
    /// mData if desired.
    LLScalarCond(const value_type& init=value_type()):
        super(init)
    {}

    /// Pass set_one() a new value to which to update mData. set_one() will
    /// lock the mutex, update mData and then call notify_one() on the
    /// condition_variable.
    void set_one(const value_type& value)
    {
        super::update_one([&value](value_type& data){ data = value; });
    }

    /// Pass set_all() a new value to which to update mData. set_all() will
    /// lock the mutex, update mData and then call notify_all() on the
    /// condition_variable.
    void set_all(const value_type& value)
    {
        super::update_all([&value](value_type& data){ data = value; });
    }

    /**
     * Pass wait_equal() a value for which to wait. wait_equal() locks the
     * mutex and, until the stored DATA equals that value, calls wait() on the
     * condition_variable.
     */
    void wait_equal(const value_type& value)
    {
        super::wait([&value](const value_type& data){ return (data == value); });
    }

    /**
     * Pass wait_for_equal() a chrono::duration, indicating how long we're
     * willing to wait, and a value for which to wait. wait_for_equal() locks
     * the mutex and, until the stored DATA equals that value, calls
     * wait_for() on the condition_variable. wait_for_equal() returns false if
     * condition_variable::wait_for() timed out without the stored DATA being
     * equal to the passed value.
     */
    template <typename Rep, typename Period>
    bool wait_for_equal(const std::chrono::duration<Rep, Period>& timeout_duration,
                        const value_type& value)
    {
        return super::wait_for(timeout_duration,
                               [&value](const value_type& data){ return (data == value); });
    }

    /**
     * This wait_for_equal() overload accepts F32Milliseconds as the duration.
     * Any duration unit defined in llunits.h is implicitly convertible to
     * F32Milliseconds. The semantics of this method are the same as the
     * generic wait_for_equal() method.
     */
    bool wait_for_equal(F32Milliseconds timeout_duration, const value_type& value)
    {
        return wait_for_equal(super::convert(timeout_duration), value);
    }

    /**
     * Pass wait_unequal() a value from which to move away. wait_unequal()
     * locks the mutex and, until the stored DATA no longer equals that value,
     * calls wait() on the condition_variable.
     */
    void wait_unequal(const value_type& value)
    {
        super::wait([&value](const value_type& data){ return (data != value); });
    }

    /**
     * Pass wait_for_unequal() a chrono::duration, indicating how long we're
     * willing to wait, and a value from which to move away.
     * wait_for_unequal() locks the mutex and, until the stored DATA no longer
     * equals that value, calls wait_for() on the condition_variable.
     * wait_for_unequal() returns false if condition_variable::wait_for()
     * timed out with the stored DATA still being equal to the passed value.
     */
    template <typename Rep, typename Period>
    bool wait_for_unequal(const std::chrono::duration<Rep, Period>& timeout_duration,
                          const value_type& value)
    {
        return super::wait_for(timeout_duration,
                               [&value](const value_type& data){ return (data != value); });
    }

    /**
     * This wait_for_unequal() overload accepts F32Milliseconds as the duration.
     * Any duration unit defined in llunits.h is implicitly convertible to
     * F32Milliseconds. The semantics of this method are the same as the
     * generic wait_for_unequal() method.
     */
    bool wait_for_unequal(F32Milliseconds timeout_duration, const value_type& value)
    {
        return wait_for_unequal(super::convert(timeout_duration), value);
    }

protected:
    using super::convert;
};

/// Using bool as LLScalarCond's DATA seems like a particularly useful case
using LLBoolCond = LLScalarCond<bool>;

/// LLOneShotCond -- init false, set (and wait for) true
class LLOneShotCond: public LLBoolCond
{
    using super = LLBoolCond;

public:
    using typename super::value_type;
    using super::get;
    using super::wait;
    using super::wait_for;
    using super::wait_equal;
    using super::wait_for_equal;
    using super::wait_unequal;
    using super::wait_for_unequal;

    /// The bool stored in LLOneShotCond is initially false
    LLOneShotCond(): super(false) {}

    /// LLOneShotCond assumes that nullary set_one() means to set its bool true
    void set_one(bool value=true)
    {
        super::set_one(value);
    }

    /// LLOneShotCond assumes that nullary set_all() means to set its bool true
    void set_all(bool value=true)
    {
        super::set_all(value);
    }

    /**
     * wait() locks the mutex and, until the stored bool is true, calls wait()
     * on the condition_variable.
     */
    void wait()
    {
        super::wait_unequal(false);
    }

    /**
     * Pass wait_for() a chrono::duration, indicating how long we're willing
     * to wait. wait_for() locks the mutex and, until the stored bool is true,
     * calls wait_for() on the condition_variable. wait_for() returns false if
     * condition_variable::wait_for() timed out without the stored bool being
     * true.
     */
    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout_duration)
    {
        return super::wait_for_unequal(timeout_duration, false);
    }

    /**
     * This wait_for() overload accepts F32Milliseconds as the duration.
     * Any duration unit defined in llunits.h is implicitly convertible to
     * F32Milliseconds. The semantics of this method are the same as the
     * generic wait_for() method.
     */
    bool wait_for(F32Milliseconds timeout_duration)
    {
        return wait_for(super::convert(timeout_duration));
    }
};

#endif /* ! defined(LL_LLCOND_H) */
