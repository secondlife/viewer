/**
 * @file   sync.h
 * @author Nat Goodspeed
 * @date   2019-03-13
 * @brief  Synchronize coroutines within a test program so we can observe side
 *         effects. Certain test programs test coroutine synchronization
 *         mechanisms. Such tests usually want to interleave coroutine
 *         executions in strictly stepwise fashion. This class supports that
 *         paradigm.
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_SYNC_H)
#define LL_SYNC_H

#include "llcond.h"
#include "lltut.h"
#include "stringize.h"
#include "llerror.h"
#include "llcoros.h"

/**
 * Instantiate Sync in any test in which we need to suspend one coroutine
 * until we're sure that another has had a chance to run. Simply calling
 * llcoro::suspend() isn't necessarily enough; that provides a chance for the
 * other to run, but doesn't guarantee that it has. If each coroutine is
 * consistent about calling Sync::bump() every time it wakes from any
 * suspension, Sync::yield() and yield_until() should at least ensure that
 * somebody else has had a chance to run.
 */
class Sync
{
    LLScalarCond<int> mCond{0};
    F32Milliseconds mTimeout;

public:
    Sync(F32Milliseconds timeout=F32Milliseconds(10.0f)):
        mTimeout(timeout)
    {}

    /**
     * Bump mCond by n steps -- ideally, do this every time a participating
     * coroutine wakes up from any suspension. The choice to bump() after
     * resumption rather than just before suspending is worth calling out:
     * this practice relies on the fact that condition_variable::notify_all()
     * merely marks a suspended coroutine ready to run, rather than
     * immediately resuming it. This way, though, even if a coroutine exits
     * before reaching its next suspend point, the other coroutine isn't
     * left waiting forever.
     */
    void bump(int n=1)
    {
        // Calling mCond.set_all(mCond.get() + n) would be great for
        // coroutines -- but not so good between kernel threads -- it would be
        // racy. Make the increment atomic by calling update_all(), which runs
        // the passed lambda within a mutex lock.
        int updated;
        mCond.update_all(
            [&n, &updated](int& data)
            {
                data += n;
                // Capture the new value for possible logging purposes.
                updated = data;
            });
        // In the multi-threaded case, this log message could be a bit
        // misleading, as it will be emitted after waiting threads have
        // already awakened. But emitting the log message within the lock
        // would seem to hold the lock longer than we really ought.
        LL_DEBUGS() << llcoro::logname() << " bump(" << n << ") -> " << updated << LL_ENDL;
    }

    /**
     * Set mCond to a specific n. Use of bump() and yield() is nicely
     * maintainable, since you can insert or delete matching operations in a
     * test function and have the rest of the Sync operations continue to
     * line up as before. But sometimes you need to get very specific, which
     * is where set() and yield_until() come in handy: less maintainable,
     * more precise.
     */
    void set(int n)
    {
        LL_DEBUGS() << llcoro::logname() << " set(" << n << ")" << LL_ENDL;
        mCond.set_all(n);
    }

    /// suspend until "somebody else" has bumped mCond by n steps
    void yield(int n=1)
    {
        return yield_until(STRINGIZE("Sync::yield_for(" << n << ") timed out after "
                                     << int(mTimeout.value()) << "ms"),
                           mCond.get() + n);
    }

    /// suspend until "somebody else" has bumped mCond to a specific value
    void yield_until(int until)
    {
        return yield_until(STRINGIZE("Sync::yield_until(" << until << ") timed out after "
                                     << int(mTimeout.value()) << "ms"),
                           until);
    }

private:
    void yield_until(const std::string& desc, int until)
    {
        std::string name(llcoro::logname());
        LL_DEBUGS() << name << " yield_until(" << until << ") suspending" << LL_ENDL;
        tut::ensure(name + ' ' + desc, mCond.wait_for_equal(mTimeout, until));
        // each time we wake up, bump mCond
        bump();
    }
};

#endif /* ! defined(LL_SYNC_H) */
