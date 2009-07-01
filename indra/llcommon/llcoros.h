/**
 * @file   llcoros.h
 * @author Nat Goodspeed
 * @date   2009-06-02
 * @brief  Manage running boost::coroutine instances
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCOROS_H)
#define LL_LLCOROS_H

#include <boost/coroutine/coroutine.hpp>
#include "llsingleton.h"
#include <boost/ptr_container/ptr_map.hpp>
#include <string>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <stdexcept>

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
    /// Canonical boost::coroutines::coroutine signature we use
    typedef boost::coroutines::coroutine<void()> coro;
    /// Canonical 'self' type
    typedef coro::self self;

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
     *     // Do NOT NOT NOT accept reference params other than 'self'!
     *     // Pass by value only!
     *     void myCoroutineMethod(LLCoros::self& self, std::string, LLSD);
     *     ...
     * };
     * ...
     * std::string name = LLCoros::instance().launch(
     *    "mycoro", boost::bind(&MyClass::myCoroutineMethod, this, _1,
     *                          "somestring", LLSD(17));
     * @endcode
     *
     * Your function/method must accept LLCoros::self& as its first parameter.
     * It can accept any other parameters you want -- but ONLY BY VALUE!
     * Other reference parameters are a BAD IDEA! You Have Been Warned. See
     * DEV-32777 comments for an explanation.
     *
     * Pass a callable that accepts the single LLCoros::self& parameter. It
     * may work to pass a free function whose only parameter is 'self'; for
     * all other cases use boost::bind(). Of course, for a non-static class
     * method, the first parameter must be the class instance. Use the
     * placeholder _1 for the 'self' parameter. Any other parameters should be
     * passed via the bind() expression.
     *
     * launch() tweaks the suggested name so it won't collide with any
     * existing coroutine instance, creates the coroutine instance, registers
     * it with the tweaked name and runs it until its first wait. At that
     * point it returns the tweaked name.
     */
    template <typename CALLABLE>
    std::string launch(const std::string& prefix, const CALLABLE& callable)
    {
        return launchImpl(prefix, new coro(callable));
    }

    /**
     * Abort a running coroutine by name. Normally, when a coroutine either
     * runs to completion or terminates with an exception, LLCoros quietly
     * cleans it up. This is for use only when you must explicitly interrupt
     * one prematurely. Returns @c true if the specified name was found and
     * still running at the time.
     */
    bool kill(const std::string& name);

    /**
     * From within a coroutine, pass its @c self object to look up the
     * (tweaked) name string by which this coroutine is registered. Returns
     * the empty string if not found (e.g. if the coroutine was launched by
     * hand rather than using LLCoros::launch()).
     */
    template <typename COROUTINE_SELF>
    std::string getName(const COROUTINE_SELF& self) const
    {
        return getNameByID(self.get_id());
    }

    /// getName() by self.get_id()
    std::string getNameByID(const void* self_id) const;

private:
    friend class LLSingleton<LLCoros>;
    LLCoros();
    std::string launchImpl(const std::string& prefix, coro* newCoro);
    std::string generateDistinctName(const std::string& prefix) const;
    bool cleanup(const LLSD&);

    typedef boost::ptr_map<std::string, coro> CoroMap;
    CoroMap mCoros;
};

#endif /* ! defined(LL_LLCOROS_H) */
