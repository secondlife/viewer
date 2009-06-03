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

/// Base class for each coroutine
struct LLCoroBase
{
    LLCoroBase() {}
    virtual ~LLCoroBase() {}

    virtual bool exited() const = 0;
    template <typename COROUTINE_SELF>
    bool owns_self(const COROUTINE_SELF& self) const
    {
        return owns_self_id(self.get_id());
    }

    virtual bool owns_self_id(const void* self_id) const = 0;
};

/// Template subclass to accommodate different boost::coroutine signatures
template <typename COROUTINE>
struct LLCoro: public LLCoroBase
{
    template <typename CALLABLE>
    LLCoro(const CALLABLE& callable):
        mCoro(callable)
    {}

    virtual bool exited() const { return mCoro.exited(); }

    COROUTINE mCoro;

    virtual bool owns_self_id(const void* self_id) const
    {
        namespace coro_private = boost::coroutines::detail;
        return static_cast<void*>(coro_private::coroutine_accessor::get_impl(const_cast<COROUTINE&>(mCoro)).get())
            == self_id;
    }
};

/**
 * Registry of named Boost.Coroutine instances
 *
 * The Boost.Coroutine library supports the general case of a coroutine accepting
 * arbitrary parameters and yielding multiple (sets of) results. For such use
 * cases, it's natural for the invoking code to retain the coroutine instance:
 * the consumer repeatedly calls back into the coroutine until it yields its
 * next result.
 *
 * Our typical coroutine usage is a bit different, though. For us, coroutines
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
class LLCoros: public LLSingleton<LLCoros>
{
public:
    /*------------------------------ launch() ------------------------------*/
    /**
     * Create and start running a new coroutine with specified name. The name
     * string you pass is a suggestion; it will be tweaked for uniqueness. The
     * actual name is returned to you.
     *
     * Usage looks like this, for (e.g.) two coroutine parameters:
     * @code
     * typedef boost::coroutines::coroutine<void(const std::string&, const LLSD&)> coro_type;
     * std::string name = LLCoros::instance().launch<coro_type>(
     *    "mycoro", boost::bind(&MyClass::method, this, _1, _2, _3),
     *    "somestring", LLSD(17));
     * @endcode
     *
     * In other words, you must specify:
     *
     * * the desired <tt>boost::coroutines::coroutine</tt> type, to whose
     *   signature the initial <tt>coro_type::self&</tt> parameter is
     *   implicitly added
     * * the suggested name string for the new coroutine instance
     * * the callable to be run, e.g. <tt>boost::bind()</tt> expression for a
     *   class method -- not forgetting to add _1 for the
     *   <tt>coro_type::self&</tt> parameter
     * * the actual parameters to be passed to that callable after the
     *   implicit <tt>coro_type::self&</tt> parameter
     *
     * launch() tweaks the suggested name so it won't collide with any
     * existing coroutine instance, creates the coroutine instance, registers
     * it with the tweaked name and runs it until its first wait. At that
     * point it returns the tweaked name.
     *
     * Use of a typedef for the coroutine type is recommended, because you
     * must restate it for the callable's first parameter.
     *
     * @note
     * launch() only accepts const-reference parameters. Once we can assume
     * C++0x features on every platform, we'll have so-called "perfect
     * forwarding" and variadic templates and other such ponies, and can
     * support an arbitrary number of truly arbitrary parameter types. But for
     * now, we'll stick with const reference params. N.B. Passing a non-const
     * reference to a local variable into a coroutine seems like a @em really
     * bad idea: the local variable will be destroyed during the lifetime of
     * the coroutine.
     */
    // Use the preprocessor to generate launch() overloads accepting 0, 1,
    // ..., BOOST_COROUTINE_ARG_MAX const ref params of arbitrary type.
#define BOOST_PP_LOCAL_MACRO(n)                                         \
    template <typename COROUTINE, typename CALLABLE                     \
              BOOST_PP_COMMA_IF(n)                                      \
              BOOST_PP_ENUM_PARAMS(n, typename T)>                      \
    std::string launch(const std::string& prefix, const CALLABLE& callable \
                       BOOST_PP_COMMA_IF(n)                             \
                       BOOST_PP_ENUM_BINARY_PARAMS(n, const T, & p))    \
    {                                                                   \
        std::string name(generateDistinctName(prefix));                 \
        LLCoro<COROUTINE>* ptr = new LLCoro<COROUTINE>(callable);       \
        mCoros.insert(name, ptr);                                       \
        /* Run the coroutine until its first wait, then return here */  \
        ptr->mCoro(std::nothrow                                         \
                   BOOST_PP_COMMA_IF(n)                                 \
                   BOOST_PP_ENUM_PARAMS(n, p));                         \
        return name;                                                    \
    }

#define BOOST_PP_LOCAL_LIMITS (0, BOOST_COROUTINE_ARG_MAX)
#include BOOST_PP_LOCAL_ITERATE()
#undef BOOST_PP_LOCAL_MACRO
#undef BOOST_PP_LOCAL_LIMITS
    /*----------------------- end of launch() family -----------------------*/

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
    std::string generateDistinctName(const std::string& prefix) const;
    bool cleanup(const LLSD&);

    typedef boost::ptr_map<std::string, LLCoroBase> CoroMap;
    CoroMap mCoros;
};

#endif /* ! defined(LL_LLCOROS_H) */
