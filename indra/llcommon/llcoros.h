/**
 * @file   llcoros.h
 * @author Nat Goodspeed
 * @date   2009-06-02
 * @brief  Manage running boost::coroutine instances
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCOROS_H)
#define LL_LLCOROS_H

#include "llcoromutex.h"
#include "llevents.h"
#include "llexception.h"
#include "llinstancetracker.h"
#include "llsingleton.h"
#include <boost/fiber/fss.hpp>
#include <exception>
#include <functional>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>

namespace llcoro
{
class scheduler;
}

/**
 * Registry of named Boost.Fiber instances
 *
 * When the viewer first introduced the semi-independent execution agents now
 * called fibers, the term "fiber" had not yet become current, and the only
 * available libraries used the term "coroutine" instead. Within the viewer we
 * continue to use the term "coroutines," though at present they are actually
 * boost::fibers::fiber instances.
 *
 * Coroutines provide an alternative to the @c Responder pattern. Our typical
 * coroutine has @c void return, invoked in fire-and-forget mode: the handler
 * for some user gesture launches the coroutine and promptly returns to the
 * main loop. The coroutine initiates some action that will take multiple
 * frames (e.g. a capability request), waits for its result, processes it and
 * silently steals away.
 *
 * LLCoros is a Singleton collection of currently-active coroutine instances.
 * Each has a name. You ask LLCoros to launch a new coroutine with a suggested
 * name prefix; from your prefix it generates a distinct name, registers the
 * new coroutine and returns the actual name.
 *
 * The name can provide diagnostic info: we can look up the name of the
 * currently-running coroutine.
 */
class LL_COMMON_API LLCoros: public LLSingleton<LLCoros>
{
    LLSINGLETON(LLCoros);
    ~LLCoros();

    void cleanupSingleton() override;
public:
    // For debugging, return true if on the main coroutine for the current thread
    // Code that should not be executed from a coroutine should be protected by
    // llassert(LLCoros::on_main_coro())
    static bool on_main_coro();

    // For debugging, return true if on the main thread and not in a coroutine
    // Non-thread-safe code in the main loop should be protected by
    // llassert(LLCoros::on_main_thread_main_coro())
    static bool on_main_thread_main_coro();

    typedef boost::fibers::fiber coro;
    typedef coro::id id;
    /// Canonical callable type
    typedef std::function<void()> callable_t;

    /**
     * Create and start running a new coroutine with specified name. The name
     * string you pass is a suggestion; it will be tweaked for uniqueness. The
     * actual name is returned to you.
     *
     * Usage looks like this, for (e.g.) two coroutine parameters:
     * @code
     * class MyClass
     * {
     * public:
     *     ...
     *     // Do NOT NOT NOT accept reference params!
     *     // Pass by value only!
     *     void myCoroutineMethod(std::string, LLSD);
     *     ...
     * };
     * ...
     * std::string name = LLCoros::instance().launch(
     *    "mycoro", boost::bind(&MyClass::myCoroutineMethod, this,
     *                          "somestring", LLSD(17));
     * @endcode
     *
     * Your function/method can accept any parameters you want -- but ONLY BY
     * VALUE! Reference parameters are a BAD IDEA! You Have Been Warned. See
     * DEV-32777 comments for an explanation.
     *
     * Pass a nullary callable. It works to directly pass a nullary free
     * function (or static method); for other cases use a lambda expression,
     * std::bind() or boost::bind(). Of course, for a non-static class method,
     * the first parameter must be the class instance. Any other parameters
     * should be passed via the enclosing expression.
     *
     * launch() tweaks the suggested name so it won't collide with any
     * existing coroutine instance, creates the coroutine instance, registers
     * it with the tweaked name and runs it until its first wait. At that
     * point it returns the tweaked name.
     */
    std::string launch(const std::string& prefix, const callable_t& callable);

    /**
     * Ask the named coroutine to abort. Normally, when a coroutine either
     * runs to completion or terminates with an exception, LLCoros quietly
     * cleans it up. This is for use only when you must explicitly interrupt
     * one prematurely. Returns @c true if the specified name was found and
     * still running at the time.
     */
    bool killreq(const std::string& name);

    /**
     * From within a coroutine, look up the (tweaked) name string by which
     * this coroutine is registered.
     */
    static std::string getName();

    /**
     * Given an id, return the name of that coroutine.
     */
    static std::string getName(id);

    /**
     * rethrow() is called by the thread's main fiber to propagate an
     * exception from any coroutine into the main fiber, where it can engage
     * the normal unhandled-exception machinery, up to and including crash
     * reporting.
     *
     * LLCoros maintains a queue of otherwise-uncaught exceptions from
     * terminated coroutines. Each call to rethrow() pops the first of those
     * and rethrows it. When the queue is empty (normal case), rethrow() is a
     * no-op.
     */
    void rethrow();

    /**
     * For delayed initialization. To be clear, this will only affect
     * coroutines launched @em after this point. The underlying facility
     * provides no way to alter the stack size of any running coroutine.
     */
    void setStackSize(S32 stacksize);

    /// diagnostic
    void printActiveCoroutines(const std::string& when=std::string());

    /// get the current coro::id for those who really really care
    static id get_self();

    /**
     * Most coroutines, most of the time, don't "consume" the events for which
     * they're suspending. This way, an arbitrary number of listeners (whether
     * coroutines or simple callbacks) can be registered on a particular
     * LLEventPump, every listener responding to each of the events on that
     * LLEventPump. But a particular coroutine can assert that it will consume
     * each event for which it suspends. (See also llcoro::postAndSuspend(),
     * llcoro::VoidListener)
     */
    static void set_consuming(bool consuming);
    static bool get_consuming();

    /**
     * RAII control of the consuming flag
     */
    class OverrideConsuming
    {
    public:
        OverrideConsuming(bool consuming):
            mPrevConsuming(get_consuming())
        {
            set_consuming(consuming);
        }
        OverrideConsuming(const OverrideConsuming&) = delete;
        ~OverrideConsuming()
        {
            set_consuming(mPrevConsuming);
        }

    private:
        bool mPrevConsuming;
    };

    /// set string coroutine status for diagnostic purposes
    static void setStatus(const std::string& status);
    static std::string getStatus();

    /// RAII control of status
    class TempStatus
    {
    public:
        TempStatus(const std::string& status):
            mOldStatus(getStatus())
        {
            setStatus(status);
        }
        TempStatus(const TempStatus&) = delete;
        TempStatus& operator=(const TempStatus&) = delete;
        ~TempStatus()
        {
            setStatus(mOldStatus);
        }

    private:
        std::string mOldStatus;
    };

    /// thrown by checkStop()
    // It may sound ironic that Stop is derived from LLContinueError, but the
    // point is that LLContinueError is the category of exception that should
    // not immediately crash the viewer. Stop and its subclasses are to tell
    // coroutines to terminate, e.g. because the viewer is shutting down. We
    // do not want any such exception to crash the viewer.
    struct Stop: public LLContinueError
    {
        Stop(const std::string& what): LLContinueError(what) {}
    };

    /// someone wants to kill this specific coroutine
    struct Killed: public Stop
    {
        Killed(const std::string& what): Stop(what) {}
    };

    /// early shutdown stages
    struct Stopping: public Stop
    {
        Stopping(const std::string& what): Stop(what) {}
    };

    /// cleaning up
    struct Stopped: public Stop
    {
        Stopped(const std::string& what): Stop(what) {}
    };

    /// cleaned up -- not much survives!
    struct Shutdown: public Stop
    {
        Shutdown(const std::string& what): Stop(what) {}
    };

    /// Call this intermittently if there's a chance your coroutine might
    /// still be running at application shutdown. Throws one of the Stop
    /// subclasses if the caller needs to terminate. Pass a cleanup function
    /// if you need to execute that cleanup before terminating.
    /// Of course, if your cleanup function throws, that will be the exception
    /// propagated by checkStop().
    static void checkStop(callable_t cleanup={});

    /// Call getStopListener() at the source end of a queue, promise or other
    /// resource on which coroutines will wait, so that shutdown can wake up
    /// consuming coroutines. @a caller should distinguish who's calling. The
    /// passed @a cleanup function must close the queue, break the promise or
    /// otherwise cause waiting consumers to wake up in an abnormal way. It's
    /// advisable to store the returned LLBoundListener in an
    /// LLTempBoundListener, or otherwise arrange to disconnect it.
    static LLBoundListener getStopListener(const std::string& caller, LLVoidListener cleanup);

    /// This getStopListener() overload is like the two-argument one, for use
    /// when we know the name of the only coroutine that will wait on the
    /// resource in question. Pass @a consumer as the empty string if the
    /// consumer coroutine is the same as the calling coroutine. Unlike the
    /// two-argument getStopListener(), this one also responds to
    /// killreq(target).
    static LLBoundListener getStopListener(const std::string& caller,
                                           const std::string& consumer,
                                           LLVoidListener cleanup);

    /**
     * LLCoros aliases for promise and future, for backwards compatibility.
     * These have been hoisted out to the llcoro namespace.
     */
    template <typename T>
    using Promise = llcoro::Promise<T>;
    template <typename T>
    using Future = llcoro::Future<T>;
    template <typename T>
    static Future<T> getFuture(Promise<T>& promise) { return promise.get_future(); }

    // use mutex, lock, condition_variable suitable for coroutines
    using Mutex = llcoro::Mutex;
    using RMutex = llcoro::RMutex;
    // LockType is deprecated; see llcoromutex.h
    using LockType = llcoro::LockType;
    using cv_status = llcoro::cv_status;
    using ConditionVariable = llcoro::ConditionVariable;

    /// for data local to each running coroutine
    template <typename T>
    using local_ptr = boost::fibers::fiber_specific_ptr<T>;

private:
    friend class llcoro::scheduler;

    std::string generateDistinctName(const std::string& prefix) const;
    void toplevel(std::string name, callable_t callable);
    struct CoroData;
    static CoroData& get_CoroData();
    static CoroData& get_CoroData(id);
    static CoroData& main_CoroData();
    void saveException(const std::string& name, std::exception_ptr exc);

    LLTempBoundListener mConn;

    struct ExceptionData
    {
        ExceptionData(const std::string& nm, std::exception_ptr exc):
            name(nm),
            exception(exc)
        {}
        // name of coroutine that originally threw this exception
        std::string name;
        // the thrown exception
        std::exception_ptr exception;
    };
    std::queue<ExceptionData> mExceptionQueue;

    S32 mStackSize;

    // coroutine-local storage, as it were: one per coro we track
    struct CoroData: public LLInstanceTracker<CoroData, id>
    {
        using super = LLInstanceTracker<CoroData, id>;

        CoroData(const std::string& name);
        ~CoroData();

        std::string getName() const;

        bool isMain{ false };
        // tweaked name of the current coroutine
        std::string mName;
        // set_consuming() state -- don't consume events unless specifically directed
        bool mConsuming{ false };
        // killed by which coroutine
        std::string mKilledBy;
        // setStatus() state
        std::string mStatus;
        F64 mCreationTime; // since epoch
        // Histogram of how many times this coroutine's timeslice exceeds
        // certain thresholds. mHistogram is pre-populated with those
        // thresholds as keys. If k0 is one threshold key and k1 is the next,
        // mHistogram[k0] is the number of times a coroutine timeslice tn ran
        // (k0 <= tn < k1). A timeslice less than mHistogram.begin()->first is
        // fine; we don't need to record those.
        std::map<F64, U32> mHistogram;
    };

    // Identify the current coroutine's CoroData. This local_ptr isn't static
    // because it's a member of an LLSingleton, and we rely on it being
    // cleaned up in proper dependency order.
    local_ptr<CoroData> mCurrent;

    // ensure name uniqueness
    static thread_local std::unordered_map<std::string, int> mPrefixMap;
    // lookup by name
    static thread_local std::unordered_map<std::string, id> mNameMap;
};

#endif /* ! defined(LL_LLCOROS_H) */
