/**
 * @file   lleventdispatcher.h
 * @author Nat Goodspeed
 * @date   2009-06-18
 * @brief  Central mechanism for dispatching events by string name. This is
 *         useful when you have a single LLEventPump listener on which you can
 *         request different operations, vs. instantiating a different
 *         LLEventPump for each such operation.
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

#if ! defined(LL_LLEVENTDISPATCHER_H)
#define LL_LLEVENTDISPATCHER_H

#include <boost/fiber/fss.hpp>
#include <boost/function_types/is_member_function_pointer.hpp>
#include <boost/function_types/is_nonmember_callable_builtin.hpp>
#include <boost/hof/is_invocable.hpp> // until C++17, when we get std::is_invocable
#include <boost/iterator/transform_iterator.hpp>
#include <functional>               // std::function
#include <memory>                   // std::unique_ptr
#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>                  // std::pair
#include "always_return.h"
#include "function_types.h"         // LL::function_arity
#include "llevents.h"
#include "llptrto.h"
#include "llsdutil.h"

class LLSD;

/**
 * Given an LLSD map, examine a string-valued key and call a corresponding
 * callable. This class is designed to be contained by an LLEventPump
 * listener class that will register some of its own methods, though any
 * callable can be used.
 */
class LL_COMMON_API LLEventDispatcher
{
public:
    /**
     * Pass description and the LLSD key used by try_call(const LLSD&) and
     * operator()(const LLSD&) to extract the name of the registered callable
     * to invoke.
     */
    LLEventDispatcher(const std::string& desc, const std::string& key);
    /**
     * Pass description, the LLSD key used by try_call(const LLSD&) and
     * operator()(const LLSD&) to extract the name of the registered callable
     * to invoke, and the LLSD key used by try_call(const LLSD&) and
     * operator()(const LLSD&) to extract arguments LLSD.
     */
    LLEventDispatcher(const std::string& desc, const std::string& key,
                      const std::string& argskey);
    virtual ~LLEventDispatcher();

    /// @name Register functions accepting(const LLSD&)
    //@{

    /// Accept any C++ callable with the right signature
    typedef std::function<LLSD(const LLSD&)> Callable;

    /**
     * Register a @a callable by @a name. The passed @a callable accepts a
     * single LLSD value and uses it in any way desired, e.g. extract
     * parameters and call some other function. The optional @a required
     * parameter is used to validate the structure of each incoming event (see
     * llsd_matches()).
     */
    void add(const std::string& name,
             const std::string& desc,
             const Callable& callable,
             const LLSD& required=LLSD())
    {
        addLLSD(name, desc, callable, required);
    }

    template <typename CALLABLE,
              typename=typename std::enable_if<
                  boost::hof::is_invocable<CALLABLE, LLSD>::value
             >::type>
    void add(const std::string& name,
             const std::string& desc,
             CALLABLE&& callable,
             const LLSD& required=LLSD())
    {
        addLLSD(
            name,
            desc,
            Callable(LL::make_always_return<LLSD>(std::forward<CALLABLE>(callable))),
            required);
    }

    /**
     * Special case: a subclass of this class can pass an unbound member
     * function pointer (of an LLEventDispatcher subclass) without explicitly
     * specifying a <tt>std::bind()</tt> expression. The passed @a method
     * accepts a single LLSD value, presumably containing other parameters.
     */
    template <typename R, class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)(const LLSD&),
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }

    /// Overload for both const and non-const methods. The passed @a method
    /// accepts a single LLSD value, presumably containing other parameters.
    template <typename R, class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)(const LLSD&) const,
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }

    // because the compiler can't match a method returning void to the above
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(const LLSD&),
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }

    /// Overload for both const and non-const methods. The passed @a method
    /// accepts a single LLSD value, presumably containing other parameters.
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(const LLSD&) const,
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }

    // non-const nullary method
    template <typename R, class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)())
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // const nullary method
    template <typename R, class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)() const)
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // non-const nullary method returning void
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)())
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // const nullary method returning void
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)() const)
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // non-const unary method (but method accepting LLSD should use the other add())
    // enable_if usage per https://stackoverflow.com/a/39913395/5533635
    template <typename R, class CLASS, typename ARG,
              typename = typename std::enable_if<
                  ! std::is_same<typename std::decay<ARG>::type, LLSD>::value
             >::type>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)(ARG))
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // const unary method (but method accepting LLSD should use the other add())
    template <typename R, class CLASS, typename ARG,
              typename = typename std::enable_if<
                  ! std::is_same<typename std::decay<ARG>::type, LLSD>::value
             >::type>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)(ARG) const)
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // non-const unary method returning void
    // enable_if usage per https://stackoverflow.com/a/39913395/5533635
    template <class CLASS, typename ARG,
              typename = typename std::enable_if<
                  ! std::is_same<typename std::decay<ARG>::type, LLSD>::value
             >::type>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(ARG))
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // const unary method returning void
    template <class CLASS, typename ARG,
              typename = typename std::enable_if<
                  ! std::is_same<typename std::decay<ARG>::type, LLSD>::value
             >::type>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(ARG) const)
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // non-const binary (or more) method
    template <typename R, class CLASS, typename ARG0, typename ARG1, typename... ARGS>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)(ARG0, ARG1, ARGS...))
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // const binary (or more) method
    template <typename R, class CLASS, typename ARG0, typename ARG1, typename... ARGS>
    void add(const std::string& name,
             const std::string& desc,
             R (CLASS::*method)(ARG0, ARG1, ARGS...) const)
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // non-const binary (or more) method returning void
    template <class CLASS, typename ARG0, typename ARG1, typename... ARGS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(ARG0, ARG1, ARGS...))
    {
        addVMethod<CLASS>(name, desc, method);
    }

    // const binary (or more) method returning void
    template <class CLASS, typename ARG0, typename ARG1, typename... ARGS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(ARG0, ARG1, ARGS...) const)
    {
        addVMethod<CLASS>(name, desc, method);
    }

    //@}

    /// @name Register functions with arbitrary param lists
    //@{

    /**
     * Register a free function with arbitrary parameters. (This also works
     * for static class methods.)
     *
     * When calling this name, pass an LLSD::Array. Each entry in turn will be
     * converted to the corresponding parameter type using LLSDParam.
     */
    template <typename CALLABLE,
              typename=typename std::enable_if<
                  ! boost::hof::is_invocable<CALLABLE, LLSD>()
             >::type>
    void add(const std::string& name,
             const std::string& desc,
             CALLABLE&& f)
    {
        addV(name, desc, f);
    }

    /**
     * Register a nonstatic class method with arbitrary parameters.
     *
     * To cover cases such as a method on an LLSingleton we don't yet want to
     * instantiate, instead of directly storing an instance pointer, accept a
     * nullary callable returning a pointer/reference to the desired class
     * instance.
     *
     * When calling this name, pass an LLSD::Array. Each entry in turn will be
     * converted to the corresponding parameter type using LLSDParam.
     */
    template<typename Method, typename InstanceGetter,
             typename = typename std::enable_if<
                 boost::function_types::is_member_function_pointer<Method>::value &&
                 ! std::is_convertible<InstanceGetter, LLSD>::value
             >::type>
    void add(const std::string& name, const std::string& desc, Method f,
             const InstanceGetter& getter);

    /**
     * Register a free function with arbitrary parameters. (This also works
     * for static class methods.)
     *
     * Pass an LLSD::Array of parameter names, and optionally another
     * LLSD::Array of default parameter values, a la LLSDArgsMapper.
     *
     * When calling this name, pass an LLSD::Map. We will internally generate
     * an LLSD::Array using LLSDArgsMapper and then convert each entry in turn
     * to the corresponding parameter type using LLSDParam.
     */
    template<typename Function,
             typename = typename std::enable_if<
                 boost::function_types::is_nonmember_callable_builtin<Function>::value &&
                 ! boost::hof::is_invocable<Function, LLSD>::value
             >::type>
    void add(const std::string& name, const std::string& desc, Function f,
             const LLSD& params, const LLSD& defaults=LLSD());

    /**
     * Register a nonstatic class method with arbitrary parameters.
     *
     * To cover cases such as a method on an LLSingleton we don't yet want to
     * instantiate, instead of directly storing an instance pointer, accept a
     * nullary callable returning a pointer/reference to the desired class
     * instance. If you already have an instance in hand,
     * boost::lambda::var(instance) or boost::lambda::constant(instance_ptr)
     * produce suitable callables.
     *
     * TODO: variant accepting a method of the containing class, no getter.
     *
     * Pass an LLSD::Array of parameter names, and optionally another
     * LLSD::Array of default parameter values, a la LLSDArgsMapper.
     *
     * When calling this name, pass an LLSD::Map. We will internally generate
     * an LLSD::Array using LLSDArgsMapper and then convert each entry in turn
     * to the corresponding parameter type using LLSDParam.
     */
    template<typename Method, typename InstanceGetter,
             typename = typename std::enable_if<
                 boost::function_types::is_member_function_pointer<Method>::value &&
                 ! std::is_convertible<InstanceGetter, LLSD>::value
             >::type>
    void add(const std::string& name, const std::string& desc, Method f,
             const InstanceGetter& getter, const LLSD& params,
             const LLSD& defaults=LLSD());

    //@}

    /// Unregister a callable
    bool remove(const std::string& name);

    /// Exception if an attempted call fails for any reason
    struct DispatchError: public LLException
    {
        DispatchError(const std::string& what): LLException(what) {}
    };

    /// Specific exception for an attempt to call a nonexistent name
    struct DispatchMissing: public DispatchError
    {
        DispatchMissing(const std::string& what): DispatchError(what) {}
    };

    /**
     * Call a registered callable with an explicitly-specified name,
     * converting its return value to LLSD (undefined for a void callable).
     * It is an error if no such callable exists. It is an error if the @a
     * event fails to match the @a required prototype specified at add()
     * time.
     *
     * @a event must be an LLSD array for a callable registered to accept its
     * arguments from such an array. It must be an LLSD map for a callable
     * registered to accept its arguments from such a map.
     */
    LLSD operator()(const std::string& name, const LLSD& event) const;

    /**
     * Call a registered callable with an explicitly-specified name and
     * return <tt>true</tt>. If no such callable exists, return
     * <tt>false</tt>. It is an error if the @a event fails to match the @a
     * required prototype specified at add() time.
     *
     * @a event must be an LLSD array for a callable registered to accept its
     * arguments from such an array. It must be an LLSD map for a callable
     * registered to accept its arguments from such a map.
     */
    bool try_call(const std::string& name, const LLSD& event) const;

    /**
     * Extract the @a key specified to our constructor from the incoming LLSD
     * map @a event, and call the callable whose name is specified by that @a
     * key's value, converting its return value to LLSD (undefined for a void
     * callable). It is an error if no such callable exists. It is an error if
     * the @a event fails to match the @a required prototype specified at
     * add() time.
     *
     * For a (non-nullary) callable registered to accept its arguments from an
     * LLSD array, the @a event map must contain the key @a argskey specified to
     * our constructor. The value of the @a argskey key must be an LLSD array
     * containing the arguments to pass to the callable named by @a key.
     *
     * For a callable registered to accept its arguments from an LLSD map, if
     * the @a event map contains the key @a argskey specified our constructor,
     * extract the value of the @a argskey key and use it as the arguments map.
     * If @a event contains no @a argskey key, use the whole @a event as the
     * arguments map.
     */
    LLSD operator()(const LLSD& event) const;

    /**
     * Extract the @a key specified to our constructor from the incoming LLSD
     * map @a event, call the callable whose name is specified by that @a
     * key's value and return <tt>true</tt>. If no such callable exists,
     * return <tt>false</tt>. It is an error if the @a event fails to match
     * the @a required prototype specified at add() time.
     *
     * For a (non-nullary) callable registered to accept its arguments from an
     * LLSD array, the @a event map must contain the key @a argskey specified to
     * our constructor. The value of the @a argskey key must be an LLSD array
     * containing the arguments to pass to the callable named by @a key.
     *
     * For a callable registered to accept its arguments from an LLSD map, if
     * the @a event map contains the key @a argskey specified our constructor,
     * extract the value of the @a argskey key and use it as the arguments map.
     * If @a event contains no @a argskey key, use the whole @a event as the
     * arguments map.
     */
    bool try_call(const LLSD& event) const;

    /// @name Iterate over defined names
    //@{
    typedef std::pair<std::string, std::string> NameDesc;

private:
    struct DispatchEntry
    {
        DispatchEntry(LLEventDispatcher* parent, const std::string& desc);
        virtual ~DispatchEntry() {} // suppress MSVC warning, sigh

        // store a plain dumb back-pointer because the parent
        // LLEventDispatcher manages the lifespan of each DispatchEntry
        // subclass instance -- not the other way around
        LLEventDispatcher* mParent;
        std::string mDesc;

        virtual LLSD call(const std::string& desc, const LLSD& event,
                          bool fromMap, const std::string& argskey) const = 0;
        virtual LLSD getMetadata() const = 0;

        template <typename... ARGS>
        [[noreturn]] void callFail(ARGS&&... args) const
        {
            mParent->callFail<LLEventDispatcher::DispatchError>(std::forward<ARGS>(args)...);
        }
    };
    typedef std::map<std::string, std::unique_ptr<DispatchEntry> > DispatchMap;

public:
    /// We want the flexibility to redefine what data we store per name,
    /// therefore our public interface doesn't expose DispatchMap iterators,
    /// or DispatchMap itself, or DispatchEntry. Instead we explicitly
    /// transform each DispatchMap item to NameDesc on dereferencing.
    typedef boost::transform_iterator<NameDesc(*)(const DispatchMap::value_type&), DispatchMap::const_iterator> const_iterator;
    const_iterator begin() const
    {
        return boost::make_transform_iterator(mDispatch.begin(), makeNameDesc);
    }
    const_iterator end() const
    {
        return boost::make_transform_iterator(mDispatch.end(), makeNameDesc);
    }
    //@}

    /// Get information about a specific Callable
    LLSD getMetadata(const std::string& name) const;

    /// Retrieve the LLSD key we use for one-arg <tt>operator()</tt> method
    std::string getDispatchKey() const { return mKey; }
    /// Retrieve the LLSD key we use for non-map arguments
    std::string getArgsKey() const { return mArgskey; }

    /// description of this instance's leaf class and description
    friend std::ostream& operator<<(std::ostream&, const LLEventDispatcher&);

private:
    void addLLSD(const std::string& name,
                 const std::string& desc,
                 const Callable& callable,
                 const LLSD& required);

    template <class CLASS, typename METHOD,
              typename std::enable_if<
                  std::is_base_of<LLEventDispatcher, CLASS>::value,
                  bool
              >::type=true>
    void addMethod(const std::string& name, const std::string& desc,
                   const METHOD& method, const LLSD& required)
    {
        // Why two overloaded addMethod() methods, discriminated with
        // std::is_base_of? It might seem simpler to use dynamic_cast and test
        // for nullptr. The trouble is that it doesn't work for LazyEventAPI
        // deferred registration: we get nullptr even for a method of an
        // LLEventAPI subclass.
        CLASS* downcast = static_cast<CLASS*>(this);
        add(name,
            desc,
            Callable(LL::make_always_return<LLSD>(
                         [downcast, method]
                         (const LLSD& args)
                         {
                             return (downcast->*method)(args);
                         })),
            required);
    }

    template <class CLASS, typename METHOD,
              typename std::enable_if<
                  ! std::is_base_of<LLEventDispatcher, CLASS>::value,
                  bool
              >::type=true>
    void addMethod(const std::string& name, const std::string& desc,
                   const METHOD&, const LLSD&)
    {
        addFail(name, typeid(CLASS).name());
    }

    template <class CLASS, typename METHOD>
    void addVMethod(const std::string& name, const std::string& desc,
                    const METHOD& method)
    {
        CLASS* downcast = dynamic_cast<CLASS*>(this);
        if (! downcast)
        {
            addFail(name, typeid(CLASS).name());
        }
        else
        {
            // add() arbitrary method plus InstanceGetter, where the
            // InstanceGetter in this case returns 'this'. We don't need to
            // worry about binding 'this' because, once this LLEventDispatcher
            // is destroyed, the DispatchEntry goes away too.
            add(name, desc, method, [downcast](){ return downcast; });
        }
    }

    template <typename Function>
    void addV(const std::string& name, const std::string& desc, Function f);

    void addFail(const std::string& name, const char* classname) const;
    LLSD try_call(const std::string& key, const std::string& name,
                  const LLSD& event) const;

protected:
    // raise specified EXCEPTION with specified stringize(ARGS)
    template <typename EXCEPTION, typename... ARGS>
    [[noreturn]] void callFail(ARGS&&... args) const;
    template <typename EXCEPTION, typename... ARGS>
    [[noreturn]] static
    void sCallFail(ARGS&&... args);

    // Manage transient state, e.g. which registered callable we're attempting
    // to call, for error reporting
    class SetState
    {
    public:
        template <typename... ARGS>
        SetState(const LLEventDispatcher* self, ARGS&&... args):
            mSelf(self)
        {
            mSet = mSelf->setState(*this, stringize(std::forward<ARGS>(args)...));
        }
        // RAII class: forbid both copy and move
        SetState(const SetState&) = delete;
        SetState(SetState&&) = delete;
        SetState& operator=(const SetState&) = delete;
        SetState& operator=(SetState&&) = delete;
        virtual ~SetState()
        {
            // if we're the ones who succeeded in setting state, clear it
            if (mSet)
            {
                mSelf->setState(*this, {});
            }
        }

    private:
        const LLEventDispatcher* mSelf;
        bool mSet;
    };

private:
    std::string mDesc, mKey, mArgskey;
    DispatchMap mDispatch;
    // transient state: must be fiber_specific since multiple threads and/or
    // multiple fibers may be calling concurrently. Make it mutable so we can
    // use SetState even within const methods.
    mutable boost::fibers::fiber_specific_ptr<std::string> mState;

    std::string getState() const;
    // setState() requires SetState& because only the SetState class should
    // call it. Make it const so we can use SetState even within const methods.
    bool setState(SetState&, const std::string& state) const;

    static NameDesc makeNameDesc(const DispatchMap::value_type& item)
    {
        return NameDesc(item.first, item.second->mDesc);
    }

    class LLSDArgsMapper;
    struct LLSDDispatchEntry;
    struct ParamsDispatchEntry;
    struct ArrayParamsDispatchEntry;
    struct MapParamsDispatchEntry;

    // call target function with args from LLSD array
    typedef std::function<LLSD(const LLSD&)> invoker_function;

    template <typename Function>
    invoker_function make_invoker(Function f);
    template <typename Method, typename InstanceGetter>
    invoker_function make_invoker(Method f, const InstanceGetter& getter);
    void addArrayParamsDispatchEntry(const std::string& name,
                                     const std::string& desc,
                                     const invoker_function& invoker,
                                     LLSD::Integer arity);
    void addMapParamsDispatchEntry(const std::string& name,
                                   const std::string& desc,
                                   const invoker_function& invoker,
                                   const LLSD& params,
                                   const LLSD& defaults);
};

/*****************************************************************************
*   LLEventDispatcher template implementation details
*****************************************************************************/
template <typename Function>
void LLEventDispatcher::addV(const std::string& name, const std::string& desc, Function f)
{
    // Construct an invoker_function, a callable accepting const LLSD&.
    // Add to DispatchMap an ArrayParamsDispatchEntry that will handle the
    // caller's LLSD::Array.
    addArrayParamsDispatchEntry(name, desc, make_invoker(f),
                                LL::function_arity<Function>::value);
}

template<typename Method, typename InstanceGetter, typename>
void LLEventDispatcher::add(const std::string& name, const std::string& desc, Method f,
                            const InstanceGetter& getter)
{
    // Subtract 1 from the compile-time arity because the getter takes care of
    // the first parameter. We only need (arity - 1) additional arguments.
    addArrayParamsDispatchEntry(name, desc, make_invoker(f, getter),
                                LL::function_arity<Method>::value - 1);
}

template<typename Function, typename>
void LLEventDispatcher::add(const std::string& name, const std::string& desc, Function f,
                            const LLSD& params, const LLSD& defaults)
{
    // See comments for previous is_nonmember_callable_builtin add().
    addMapParamsDispatchEntry(name, desc, make_invoker(f), params, defaults);
}

template<typename Method, typename InstanceGetter, typename>
void LLEventDispatcher::add(const std::string& name, const std::string& desc, Method f,
                            const InstanceGetter& getter,
                            const LLSD& params, const LLSD& defaults)
{
    addMapParamsDispatchEntry(name, desc, make_invoker(f, getter), params, defaults);
}

template <typename Function>
LLEventDispatcher::invoker_function
LLEventDispatcher::make_invoker(Function f)
{
    // Return an invoker_function that accepts (const LLSD& args).
    return [f](const LLSD& args)
    {
        // When called, call always_return<LLSD>, directing it to call
        // f(expanded args). always_return<LLSD> guarantees we'll get an LLSD
        // value back, even if it's undefined because 'f' doesn't return a
        // type convertible to LLSD.
        return LL::always_return<LLSD>(
            [f, args]
            ()
            {
                return LL::apply(f, args);
            });
    };
}

template <typename Method, typename InstanceGetter>
LLEventDispatcher::invoker_function
LLEventDispatcher::make_invoker(Method f, const InstanceGetter& getter)
{
    return [f, getter](const LLSD& args)
    {
        // always_return<LLSD>() immediately calls the lambda we pass, and
        // returns LLSD whether our passed lambda returns void or non-void.
        return LL::always_return<LLSD>(
            [f, getter, args]
            ()
            {
                // function_arity<member function> includes its implicit 'this' pointer
                constexpr auto arity = LL::function_arity<
                    typename std::remove_reference<Method>::type>::value - 1;

                // Use bind_front() to bind the method to (a pointer to) the object
                // returned by getter(). It's okay to capture and bind a pointer
                // because this bind_front() object will last only as long as this
                // lambda call.
                return LL::apply_n<arity>(LL::bind_front(f, LL::get_ptr(getter())), args);
            });
    };
}

/*****************************************************************************
*   LLDispatchListener
*****************************************************************************/
/**
 * Bundle an LLEventPump and a listener with an LLEventDispatcher. A class
 * that contains (or derives from) LLDispatchListener need only specify the
 * LLEventPump name and dispatch key, and add() its methods. Each incoming
 * event ("request") will automatically be dispatched.
 *
 * If the request contains a "reply" key specifying the LLSD::String name of
 * an LLEventPump to which to respond, LLDispatchListener will attempt to send
 * a response to that LLEventPump.
 *
 * If some error occurs (e.g. nonexistent callable name, wrong params) and
 * "reply" is present, LLDispatchListener will send a response map to the
 * specified LLEventPump containing an "error" key whose value is the relevant
 * error message. If "reply" is not present, the DispatchError exception will
 * propagate. Since LLDispatchListener bundles an LLEventStream, which
 * attempts the call immediately on receiving the post() call, there's a
 * reasonable chance that the exception will highlight the post() call that
 * triggered the error.
 *
 * If LLDispatchListener successfully calls the target callable, but no
 * "reply" key is present, any value returned by that callable is discarded.
 * If a "reply" key is present, but the target callable is void -- or it
 * returns LLSD::isUndefined() -- no response is sent. If a void callable
 * wants to send a response, it must do so explicitly.
 *
 * If the target callable returns a type convertible to LLSD (and, if it
 * directly returns LLSD, the return value isDefined()), and if a "reply" key
 * is present in the request, LLDispatchListener will post the returned value
 * to the "reply" LLEventPump. If the returned value is an LLSD map, it will
 * merge the echoed "reqid" key into the map and send that. Otherwise, it will
 * send an LLSD map containing "reqid" and a "data" key whose value is the
 * value returned by the target callable.
 *
 * (It is inadvisable for a target callable to return an LLSD map containing
 * keys "data", "reqid" or "error", as that will confuse the invoker.)
 *
 * Normally the request will specify the value of the dispatch key as an
 * LLSD::String naming the target callable. Alternatively, several such calls
 * may be "batched" as described below.
 *
 * If the value of the dispatch key is itself an LLSD map (a "request map"),
 * each map key must name a target callable, and the value of that key must
 * contain the parameters to pass to that callable. If a "reply" key is
 * present in the request, the response map will contain a key for each of the
 * keys in the request map. The value of every such key is the value returned
 * by the target callable.
 *
 * (Avoid naming any target callable in the LLDispatchListener "data", "reqid"
 * or "error" to avoid confusion.)
 *
 * Since LLDispatchListener calls the target callables specified by a request
 * map in arbitrary order, this form assumes that the batched operations are
 * independent of each other. LLDispatchListener will attempt every call, even
 * if some attempts produce errors. If any keys in the request map produce
 * errors, LLDispatchListener builds a composite error message string
 * collecting the relevant messages. The corresponding keys will be missing
 * from the response map. As in the single-callable case, absent a "reply" key
 * in the request, this error message will be thrown as a DispatchError. With
 * a "reply" key, it will be returned as the value of the "error" key. This
 * form can indicate partial success: some request keys might have
 * return-value keys in the response, others might have message text in the
 * "error" key.
 *
 * If a specific call sequence is required, the value of the dispatch key may
 * instead be an LLSD array (a "request array"). Each entry in the request
 * array ("request entry") names a target callable, to be called in
 * array-index sequence. Arguments for that callable may be specified in
 * either of two ways.
 *
 * The request entry may itself be a two-element array, whose [0] is an
 * LLSD::String naming the target callable and whose [1] contains the
 * arguments to pass to that callable.
 *
 * Alternatively, the request entry may be an LLSD::String naming the target
 * callable, in which case the request must contain an arguments key (optional
 * third constructor argument) whose value is an array matching the request
 * array. The arguments for the request entry's target callable are found at
 * the same index in the arguments key array.
 *
 * If a "reply" key is present in the request, the response map will contain a
 * "data" key whose value is an array. Each entry in that response array will
 * contain the result from the corresponding request entry.
 *
 * This form assumes that any of the batched operations might depend on the
 * success of a previous operation in the same batch. The @emph first error
 * encountered will terminate the sequence. The error message might either be
 * thrown as DispatchError or, given a "reply" key, returned as the "error"
 * key in the response map. This form can indicate partial success: the first
 * few request entries might have return-value entries in the "data" response
 * array, along with an "error" key whose value is the error message that
 * stopped the sequence.
 */
// Instead of containing an LLEventStream, LLDispatchListener derives from it.
// This allows an LLEventPumps::PumpFactory to return a pointer to an
// LLDispatchListener (subclass) instance, and still have ~LLEventPumps()
// properly clean it up.
class LL_COMMON_API LLDispatchListener:
    public LLEventDispatcher,
    public LLEventStream
{
public:
    /// LLEventPump name, dispatch key [, arguments key (see LLEventDispatcher)]
    template <typename... ARGS>
    LLDispatchListener(const std::string& pumpname, const std::string& key,
                       ARGS&&... args);
    virtual ~LLDispatchListener() {}

private:
    bool process(const LLSD& event) const;
    void call_one(const LLSD& name, const LLSD& event) const;
    void call_map(const LLSD& reqmap, const LLSD& event) const;
    void call_array(const LLSD& reqarray, const LLSD& event) const;
    void reply(const LLSD& reply, const LLSD& request) const;

    LLTempBoundListener mBoundListener;
    static std::string mReplyKey;
};

template <typename... ARGS>
LLDispatchListener::LLDispatchListener(const std::string& pumpname, const std::string& key,
                                       ARGS&&... args):
    // pass through any additional arguments to LLEventDispatcher ctor
    LLEventDispatcher(pumpname, key, std::forward<ARGS>(args)...),
    // Do NOT tweak the passed pumpname. In practice, when someone
    // instantiates a subclass of our LLEventAPI subclass, they intend to
    // claim that LLEventPump name in the global LLEventPumps namespace. It
    // would be mysterious and distressing if we allowed name tweaking, and
    // someone else claimed pumpname first for a completely unrelated
    // LLEventPump. Posted events would never reach our subclass listener
    // because we would have silently changed its name; meanwhile listeners
    // (if any) on that other LLEventPump would be confused by the events
    // intended for our subclass.
    LLEventStream(pumpname, false),
    mBoundListener(listen("self", [this](const LLSD& event){ return process(event); }))
{
}

#endif /* ! defined(LL_LLEVENTDISPATCHER_H) */
