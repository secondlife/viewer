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

#include <boost/dcoroutine/coroutine.hpp>
#include <boost/dcoroutine/future.hpp>
#include "llsingleton.h"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/function.hpp>
#include <boost/thread/tss.hpp>
#include <string>
#include <stdexcept>

// forward-declare helper class
namespace llcoro
{
class Suspending;
}

/**
 * Registry of named Boost.Coroutine instances
 *
 * The Boost.Coroutine library supports the general case of a coroutine
 * accepting arbitrary parameters and yielding multiple (sets of) results. For
 * such use cases, it's natural for the invoking code to retain the coroutine
 * instance: the consumer repeatedly calls into the coroutine, perhaps passing
 * new parameter values, prompting it to yield its next result.
 *
 * Our typical coroutine usage is different, though. For us, coroutines
 * provide an alternative to the @c Responder pattern. Our typical coroutine
 * has @c void return, invoked in fire-and-forget mode: the handler for some
 * user gesture launches the coroutine and promptly returns to the main loop.
 * The coroutine initiates some action that will take multiple frames (e.g. a
 * capability request), waits for its result, processes it and silently steals
 * away.
 *
 * This usage poses two (related) problems:
 *
 * # Who should own the coroutine instance? If it's simply local to the
 *   handler code that launches it, return from the handler will destroy the
 *   coroutine object, terminating the coroutine.
 * # Once the coroutine terminates, in whatever way, who's responsible for
 *   cleaning up the coroutine object?
 *
 * LLCoros is a Singleton collection of currently-active coroutine instances.
 * Each has a name. You ask LLCoros to launch a new coroutine with a suggested
 * name prefix; from your prefix it generates a distinct name, registers the
 * new coroutine and returns the actual name.
 *
 * The name can be used to kill off the coroutine prematurely, if needed. It
 * can also provide diagnostic info: we can look up the name of the
 * currently-running coroutine.
 *
 * Finally, the next frame ("mainloop" event) after the coroutine terminates,
 * LLCoros will notice its demise and destroy it.
 */
class LL_COMMON_API LLCoros: public LLSingleton<LLCoros>
{
public:
    /// Canonical boost::dcoroutines::coroutine signature we use
    typedef boost::dcoroutines::coroutine<void()> coro;
    /// Canonical callable type
    typedef boost::function<void()> callable_t;

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
     * function (or static method); for all other cases use boost::bind(). Of
     * course, for a non-static class method, the first parameter must be the
     * class instance. Any other parameters should be passed via the bind()
     * expression.
     *
     * launch() tweaks the suggested name so it won't collide with any
     * existing coroutine instance, creates the coroutine instance, registers
     * it with the tweaked name and runs it until its first wait. At that
     * point it returns the tweaked name.
     */
    std::string launch(const std::string& prefix, const callable_t& callable);

    /**
     * Abort a running coroutine by name. Normally, when a coroutine either
     * runs to completion or terminates with an exception, LLCoros quietly
     * cleans it up. This is for use only when you must explicitly interrupt
     * one prematurely. Returns @c true if the specified name was found and
     * still running at the time.
     */
    bool kill(const std::string& name);

    /**
     * From within a coroutine, look up the (tweaked) name string by which
     * this coroutine is registered. Returns the empty string if not found
     * (e.g. if the coroutine was launched by hand rather than using
     * LLCoros::launch()).
     */
    std::string getName() const;

    /// for delayed initialization
    void setStackSize(S32 stacksize);

    /// get the current coro::self& for those who really really care
    static coro::self& get_self();

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
     * Please do NOT directly use boost::dcoroutines::future! It is essential
     * to maintain the "current" coroutine at every context switch. This
     * Future wraps the essential boost::dcoroutines::future functionality
     * with that maintenance.
     */
    template <typename T>
    class Future;

private:
    LLCoros();
    friend class LLSingleton<LLCoros>;
    friend class llcoro::Suspending;
    std::string generateDistinctName(const std::string& prefix) const;
    bool cleanup(const LLSD&);
    struct CoroData;
    static void no_cleanup(CoroData*);
    static void toplevel(coro::self& self, CoroData* data, const callable_t& callable);
    static CoroData& get_CoroData(const std::string& caller);

    S32 mStackSize;

    // coroutine-local storage, as it were: one per coro we track
    struct CoroData
    {
        CoroData(CoroData* prev, const std::string& name,
                 const callable_t& callable, S32 stacksize);

        // The boost::dcoroutines library supports asymmetric coroutines. Every
        // time we context switch out of a coroutine, we pass control to the
        // previously-active one (or to the non-coroutine stack owned by the
        // thread). So our management of the "current" coroutine must be able to
        // restore the previous value when we're about to switch away.
        CoroData* mPrev;
        // tweaked name of the current coroutine
        const std::string mName;
        // the actual coroutine instance
        LLCoros::coro mCoro;
        // set_consuming() state
        bool mConsuming;
        // When the dcoroutine library calls a top-level callable, it implicitly
        // passes coro::self& as the first parameter. All our consumer code used
        // to explicitly pass coro::self& down through all levels of call stack,
        // because at the leaf level we need it for context-switching. But since
        // coroutines are based on cooperative switching, we can cause the
        // top-level entry point to stash a pointer to the currently-running
        // coroutine, and manage it appropriately as we switch out and back in.
        // That eliminates the need to pass it as an explicit parameter down
        // through every level, which is unfortunately viral in nature. Finding it
        // implicitly rather than explicitly allows minor maintenance in which a
        // leaf-level function adds a new async I/O call that suspends the calling
        // coroutine, WITHOUT having to propagate coro::self& through every
        // function signature down to that point -- and of course through every
        // other caller of every such function.
        LLCoros::coro::self* mSelf;
    };
    typedef boost::ptr_map<std::string, CoroData> CoroMap;
    CoroMap mCoros;

    // identify the current coroutine's CoroData
    static boost::thread_specific_ptr<LLCoros::CoroData> sCurrentCoro;
};

namespace llcoro
{

/// Instantiate one of these in a block surrounding any leaf point when
/// control literally switches away from this coroutine.
class Suspending
{
public:
    Suspending();
    ~Suspending();

private:
    LLCoros::CoroData* mSuspended;
};

} // namespace llcoro

template <typename T>
class LLCoros::Future
{
    typedef boost::dcoroutines::future<T> dfuture;

public:
    Future():
        mFuture(get_self())
    {}

    typedef typename boost::dcoroutines::make_callback_result<dfuture>::type callback_t;

    callback_t make_callback()
    {
        return boost::dcoroutines::make_callback(mFuture);
    }

#ifndef LL_LINUX
    explicit
#endif
    operator bool() const
    {
        return bool(mFuture);
    }

    bool operator!() const
    {
        return ! mFuture;
    }

    T get()
    {
        // instantiate Suspending to manage the "current" coroutine
        llcoro::Suspending suspended;
        return *mFuture;
    }

private:
    dfuture mFuture;
};

#endif /* ! defined(LL_LLCOROS_H) */
