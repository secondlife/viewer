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
 *
 * The invoker machinery that constructs a boost::fusion argument list for use
 * with boost::fusion::invoke() is derived from
 * http://www.boost.org/doc/libs/1_45_0/libs/function_types/example/interpreter.hpp
 * whose license information is copied below:
 *
 * "(C) Copyright Tobias Schwinger
 *
 * Use modification and distribution are subject to the boost Software License,
 * Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)."
 */

#if ! defined(LL_LLEVENTDISPATCHER_H)
#define LL_LLEVENTDISPATCHER_H

// nil is too generic a term to be allowed to be a global macro. In
// particular, boost::fusion defines a 'class nil' (properly encapsulated in a
// namespace) that a global 'nil' macro breaks badly.
#if defined(nil)
// Capture the value of the macro 'nil', hoping int is an appropriate type.
static const int nil_(nil);
// Now forget the macro.
#undef nil
// Finally, reintroduce 'nil' as a properly-scoped alias for the previously-
// defined const 'nil_'. Make it static since otherwise it produces duplicate-
// symbol link errors later.
static const int& nil(nil_);
#endif

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/function_types/is_nonmember_callable_builtin.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/fusion/include/push_back.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/invoke.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>
#include <typeinfo>
#include "llevents.h"
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
    LLEventDispatcher(const std::string& desc, const std::string& key);
    virtual ~LLEventDispatcher();

    /// @name Register functions accepting(const LLSD&)
    //@{

    /// Accept any C++ callable with the right signature, typically a
    /// boost::bind() expression
    typedef boost::function<void(const LLSD&)> Callable;

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
             const LLSD& required=LLSD());

    /**
     * The case of a free function (or static method) accepting(const LLSD&)
     * could also be intercepted by the arbitrary-args overload below. Ensure
     * that it's directed to the Callable overload above instead.
     */
    void add(const std::string& name,
             const std::string& desc,
             void (*f)(const LLSD&),
             const LLSD& required=LLSD())
    {
        add(name, desc, Callable(f), required);
    }

    /**
     * Special case: a subclass of this class can pass an unbound member
     * function pointer (of an LLEventDispatcher subclass) without explicitly
     * specifying the <tt>boost::bind()</tt> expression. The passed @a method
     * accepts a single LLSD value, presumably containing other parameters.
     */
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

    //@}

    /// @name Register functions with arbitrary param lists
    //@{

    /**
     * Register a free function with arbitrary parameters. (This also works
     * for static class methods.)
     *
     * @note This supports functions with up to about 6 parameters -- after
     * that you start getting dismaying compile errors in which
     * boost::fusion::joint_view is mentioned a surprising number of times.
     *
     * When calling this name, pass an LLSD::Array. Each entry in turn will be
     * converted to the corresponding parameter type using LLSDParam.
     */
    template<typename Function>
    typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Function f);

    /**
     * Register a nonstatic class method with arbitrary parameters.
     *
     * @note This supports functions with up to about 6 parameters -- after
     * that you start getting dismaying compile errors in which
     * boost::fusion::joint_view is mentioned a surprising number of times.
     *
     * To cover cases such as a method on an LLSingleton we don't yet want to
     * instantiate, instead of directly storing an instance pointer, accept a
     * nullary callable returning a pointer/reference to the desired class
     * instance. If you already have an instance in hand,
     * boost::lambda::var(instance) or boost::lambda::constant(instance_ptr)
     * produce suitable callables.
     *
     * When calling this name, pass an LLSD::Array. Each entry in turn will be
     * converted to the corresponding parameter type using LLSDParam.
     */
    template<typename Method, typename InstanceGetter>
    typename boost::enable_if< boost::function_types::is_member_function_pointer<Method>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Method f,
                                           const InstanceGetter& getter);

    /**
     * Register a free function with arbitrary parameters. (This also works
     * for static class methods.)
     *
     * @note This supports functions with up to about 6 parameters -- after
     * that you start getting dismaying compile errors in which
     * boost::fusion::joint_view is mentioned a surprising number of times.
     *
     * Pass an LLSD::Array of parameter names, and optionally another
     * LLSD::Array of default parameter values, a la LLSDArgsMapper.
     *
     * When calling this name, pass an LLSD::Map. We will internally generate
     * an LLSD::Array using LLSDArgsMapper and then convert each entry in turn
     * to the corresponding parameter type using LLSDParam.
     */
    template<typename Function>
    typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Function f,
                                           const LLSD& params,
                                           const LLSD& defaults=LLSD());

    /**
     * Register a nonstatic class method with arbitrary parameters.
     *
     * @note This supports functions with up to about 6 parameters -- after
     * that you start getting dismaying compile errors in which
     * boost::fusion::joint_view is mentioned a surprising number of times.
     *
     * To cover cases such as a method on an LLSingleton we don't yet want to
     * instantiate, instead of directly storing an instance pointer, accept a
     * nullary callable returning a pointer/reference to the desired class
     * instance. If you already have an instance in hand,
     * boost::lambda::var(instance) or boost::lambda::constant(instance_ptr)
     * produce suitable callables.
     *
     * Pass an LLSD::Array of parameter names, and optionally another
     * LLSD::Array of default parameter values, a la LLSDArgsMapper.
     *
     * When calling this name, pass an LLSD::Map. We will internally generate
     * an LLSD::Array using LLSDArgsMapper and then convert each entry in turn
     * to the corresponding parameter type using LLSDParam.
     */
    template<typename Method, typename InstanceGetter>
    typename boost::enable_if< boost::function_types::is_member_function_pointer<Method>
                               >::type add(const std::string& name,
                                           const std::string& desc,
                                           Method f,
                                           const InstanceGetter& getter,
                                           const LLSD& params,
                                           const LLSD& defaults=LLSD());

    //@}    

    /// Unregister a callable
    bool remove(const std::string& name);

    /// Call a registered callable with an explicitly-specified name. If no
    /// such callable exists, die with LL_ERRS. If the @a event fails to match
    /// the @a required prototype specified at add() time, die with LL_ERRS.
    void operator()(const std::string& name, const LLSD& event) const;

    /// Call a registered callable with an explicitly-specified name and
    /// return <tt>true</tt>. If no such callable exists, return
    /// <tt>false</tt>. If the @a event fails to match the @a required
    /// prototype specified at add() time, die with LL_ERRS.
    bool try_call(const std::string& name, const LLSD& event) const;

    /// Extract the @a key value from the incoming @a event, and call the
    /// callable whose name is specified by that map @a key. If no such
    /// callable exists, die with LL_ERRS. If the @a event fails to match the
    /// @a required prototype specified at add() time, die with LL_ERRS.
    void operator()(const LLSD& event) const;

    /// Extract the @a key value from the incoming @a event, call the callable
    /// whose name is specified by that map @a key and return <tt>true</tt>.
    /// If no such callable exists, return <tt>false</tt>. If the @a event
    /// fails to match the @a required prototype specified at add() time, die
    /// with LL_ERRS.
    bool try_call(const LLSD& event) const;

    /// @name Iterate over defined names
    //@{
    typedef std::pair<std::string, std::string> NameDesc;

private:
    struct DispatchEntry
    {
        DispatchEntry(const std::string& desc);
        virtual ~DispatchEntry() {} // suppress MSVC warning, sigh

        std::string mDesc;

        virtual void call(const std::string& desc, const LLSD& event) const = 0;
        virtual LLSD addMetadata(LLSD) const = 0;
    };
    // Tried using boost::ptr_map<std::string, DispatchEntry>, but ptr_map<>
    // wants its value type to be "clonable," even just to dereference an
    // iterator. I don't want to clone entries -- if I have to copy an entry
    // around, I want it to continue pointing to the same DispatchEntry
    // subclass object. However, I definitely want DispatchMap to destroy
    // DispatchEntry if no references are outstanding at the time an entry is
    // removed. This looks like a job for boost::shared_ptr.
    typedef std::map<std::string, boost::shared_ptr<DispatchEntry> > DispatchMap;

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

private:
    template <class CLASS, typename METHOD>
    void addMethod(const std::string& name, const std::string& desc,
                   const METHOD& method, const LLSD& required)
    {
        CLASS* downcast = dynamic_cast<CLASS*>(this);
        if (! downcast)
        {
            addFail(name, typeid(CLASS).name());
        }
        else
        {
            add(name, desc, boost::bind(method, downcast, _1), required);
        }
    }
    void addFail(const std::string& name, const std::string& classname) const;

    std::string mDesc, mKey;
    DispatchMap mDispatch;

    static NameDesc makeNameDesc(const DispatchMap::value_type& item)
    {
        return NameDesc(item.first, item.second->mDesc);
    }

    struct LLSDDispatchEntry;
    struct ParamsDispatchEntry;
    struct ArrayParamsDispatchEntry;
    struct MapParamsDispatchEntry;

    // Step 2 of parameter analysis. Instantiating invoker<some_function_type>
    // implicitly sets its From and To parameters to the (compile time) begin
    // and end iterators over that function's parameter types.
    template< typename Function
              , class From = typename boost::mpl::begin< boost::function_types::parameter_types<Function> >::type
              , class To   = typename boost::mpl::end< boost::function_types::parameter_types<Function> >::type
              >
    struct invoker;

    // deliver LLSD arguments one at a time
    typedef boost::function<LLSD()> args_source;
    // obtain args from an args_source to build param list and call target
    // function
    typedef boost::function<void(const args_source&)> invoker_function;

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
// Step 3 of parameter analysis, the recursive case.
template<typename Function, class From, class To>
struct LLEventDispatcher::invoker
{
    template<typename T>
    struct remove_cv_ref
        : boost::remove_cv< typename boost::remove_reference<T>::type >
    { };

    // apply() accepts an arbitrary boost::fusion sequence as args. It
    // examines the next parameter type in the parameter-types sequence
    // bounded by From and To, obtains the next LLSD object from the passed
    // args_source and constructs an LLSDParam of appropriate type to try
    // to convert the value. It then recurs with the next parameter-types
    // iterator, passing the args sequence thus far.
    template<typename Args>
    static inline
    void apply(Function func, const args_source& argsrc, Args const & args)
    {
        typedef typename boost::mpl::deref<From>::type arg_type;
        typedef typename boost::mpl::next<From>::type next_iter_type;
        typedef typename remove_cv_ref<arg_type>::type plain_arg_type;

        invoker<Function, next_iter_type, To>::apply
        ( func, argsrc, boost::fusion::push_back(args, LLSDParam<plain_arg_type>(argsrc())));
    }

    // Special treatment for instance (first) parameter of a non-static member
    // function. Accept the instance-getter callable, calling that to produce
    // the first args value. Since we know we're at the top of the recursion
    // chain, we need not also require a partial args sequence from our caller.
    template <typename InstanceGetter>
    static inline
    void method_apply(Function func, const args_source& argsrc, const InstanceGetter& getter)
    {
        typedef typename boost::mpl::next<From>::type next_iter_type;

        // Instead of grabbing the first item from argsrc and making an
        // LLSDParam of it, call getter() and pass that as the instance param.
        invoker<Function, next_iter_type, To>::apply
        ( func, argsrc, boost::fusion::push_back(boost::fusion::nil(), boost::ref(getter())));
    }
};

// Step 4 of parameter analysis, the leaf case. When the general
// invoker<Function, From, To> logic has advanced From until it matches To,
// the compiler will pick this template specialization.
template<typename Function, class To>
struct LLEventDispatcher::invoker<Function,To,To>
{
    // the argument list is complete, now call the function
    template<typename Args>
    static inline
    void apply(Function func, const args_source&, Args const & args)
    {
        boost::fusion::invoke(func, args);
    }
};

template<typename Function>
typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Function f)
{
    // Construct an invoker_function, a callable accepting const args_source&.
    // Add to DispatchMap an ArrayParamsDispatchEntry that will handle the
    // caller's LLSD::Array.
    addArrayParamsDispatchEntry(name, desc, make_invoker(f),
                                boost::function_types::function_arity<Function>::value);
}

template<typename Method, typename InstanceGetter>
typename boost::enable_if< boost::function_types::is_member_function_pointer<Method> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Method f,
                       const InstanceGetter& getter)
{
    // Subtract 1 from the compile-time arity because the getter takes care of
    // the first parameter. We only need (arity - 1) additional arguments.
    addArrayParamsDispatchEntry(name, desc, make_invoker(f, getter),
                                boost::function_types::function_arity<Method>::value - 1);
}

template<typename Function>
typename boost::enable_if< boost::function_types::is_nonmember_callable_builtin<Function> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Function f,
                       const LLSD& params, const LLSD& defaults)
{
    // See comments for previous is_nonmember_callable_builtin add().
    addMapParamsDispatchEntry(name, desc, make_invoker(f), params, defaults);
}

template<typename Method, typename InstanceGetter>
typename boost::enable_if< boost::function_types::is_member_function_pointer<Method> >::type
LLEventDispatcher::add(const std::string& name, const std::string& desc, Method f,
                       const InstanceGetter& getter,
                       const LLSD& params, const LLSD& defaults)
{
    addMapParamsDispatchEntry(name, desc, make_invoker(f, getter), params, defaults);
}

template <typename Function>
LLEventDispatcher::invoker_function
LLEventDispatcher::make_invoker(Function f)
{
    // Step 1 of parameter analysis, the top of the recursion. Passing a
    // suitable f (see add()'s enable_if condition) to this method causes it
    // to infer the function type; specifying that function type to invoker<>
    // causes it to fill in the begin/end MPL iterators over the function's
    // list of parameter types.
    // While normally invoker::apply() could infer its template type from the
    // boost::fusion::nil parameter value, here we must be explicit since
    // we're boost::bind()ing it rather than calling it directly.
    return boost::bind(&invoker<Function>::template apply<boost::fusion::nil>,
                       f,
                       _1,
                       boost::fusion::nil());
}

template <typename Method, typename InstanceGetter>
LLEventDispatcher::invoker_function
LLEventDispatcher::make_invoker(Method f, const InstanceGetter& getter)
{
    // Use invoker::method_apply() to treat the instance (first) arg specially.
    return boost::bind(&invoker<Method>::template method_apply<InstanceGetter>,
                       f,
                       _1,
                       getter);
}

/*****************************************************************************
*   LLDispatchListener
*****************************************************************************/
/**
 * Bundle an LLEventPump and a listener with an LLEventDispatcher. A class
 * that contains (or derives from) LLDispatchListener need only specify the
 * LLEventPump name and dispatch key, and add() its methods. Incoming events
 * will automatically be dispatched.
 */
class LL_COMMON_API LLDispatchListener: public LLEventDispatcher
{
public:
    LLDispatchListener(const std::string& pumpname, const std::string& key);

    std::string getPumpName() const { return mPump.getName(); }

private:
    bool process(const LLSD& event);

    LLEventStream mPump;
    LLTempBoundListener mBoundListener;
};

#endif /* ! defined(LL_LLEVENTDISPATCHER_H) */
