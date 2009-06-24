/**
 * @file   lleventdispatcher.h
 * @author Nat Goodspeed
 * @date   2009-06-18
 * @brief  Central mechanism for dispatching events by string name. This is
 *         useful when you have a single LLEventPump listener on which you can
 *         request different operations, vs. instantiating a different
 *         LLEventPump for each such operation.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLEVENTDISPATCHER_H)
#define LL_LLEVENTDISPATCHER_H

#include <string>
#include <map>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <typeinfo>
#include "llevents.h"

class LLSD;
/*==========================================================================*|
class LLEventDispatcher;

namespace LLEventDetail
{
    /// For a given call to add(), decide whether we're being passed an
    /// unbound member function pointer or a plain callable.
    /// Default case.
    template <typename CALLABLE>
    struct AddCallable
    {
        void operator()(LLEventDispatcher& disp, const std::string& name,
                        const CALLABLE& callable, const LLSD& required);
    };

    /// Unbound member function pointer
    template <class CLASS>
    struct AddCallable<void (CLASS::*)(const LLSD&)>
    {
        typedef void (CLASS::*Method)(const LLSD&);
        void operator()(LLEventDispatcher& disp, const std::string& name,
                        Method method, const LLSD& required);
    };

    /// Unbound const member function pointer
    template <class CLASS>
    struct AddCallable<void (CLASS::*)(const LLSD&) const>
    {
        typedef void (CLASS::*Method)(const LLSD&) const;
        void operator()(LLEventDispatcher& disp, const std::string& name,
                        Method method, const LLSD& required);
    };
}
|*==========================================================================*/

/**
 * Given an LLSD map, examine a string-valued key and call a corresponding
 * callable. This class is designed to be contained by an LLEventPump
 * listener class that will register some of its own methods, though any
 * callable can be used.
 */
class LLEventDispatcher
{
public:
    LLEventDispatcher(const std::string& desc, const std::string& key);
    virtual ~LLEventDispatcher();

    /// Accept any C++ callable, typically a boost::bind() expression
    typedef boost::function<void(const LLSD&)> Callable;

    /**
     * Register a @a callable by @a name. The optional @a required parameter
     * is used to validate the structure of each incoming event (see
     * llsd_matches()).
     */
    void add(const std::string& name, const Callable& callable, const LLSD& required=LLSD());
/*==========================================================================*|
    {
        LLEventDetail::AddCallable<CALLABLE>()(*this, name, callable, required);
    }
|*==========================================================================*/

    /**
     * Special case: a subclass of this class can pass an unbound member
     * function pointer without explicitly specifying the
     * <tt>boost::bind()</tt> expression.
     */
    template <class CLASS>
    void add(const std::string& name, void (CLASS::*method)(const LLSD&),
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, method, required);
    }

    /// Overload for both const and non-const methods
    template <class CLASS>
    void add(const std::string& name, void (CLASS::*method)(const LLSD&) const,
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, method, required);
    }

    /// Unregister a callable
    bool remove(const std::string& name);

    /// Call a registered callable with an explicitly-specified name. If no
    /// such callable exists, die with LL_ERRS. If the @a event fails to match
    /// the @a required prototype specified at add() time, die with LL_ERRS.
    void operator()(const std::string& name, const LLSD& event) const;

    /// Extract the @a key value from the incoming @a event, and call the
    /// callable whose name is specified by that map @a key. If no such
    /// callable exists, die with LL_ERRS. If the @a event fails to match the
    /// @a required prototype specified at add() time, die with LL_ERRS.
    void operator()(const LLSD& event) const;

private:
    template <class CLASS, typename METHOD>
    void addMethod(const std::string& name, const METHOD& method, const LLSD& required)
    {
        CLASS* downcast = dynamic_cast<CLASS*>(this);
        if (! downcast)
        {
            addFail(name, typeid(CLASS).name());
        }
        else
        {
            add(name, boost::bind(method, downcast, _1), required);
        }
    }
    void addFail(const std::string& name, const std::string& classname) const;
    /// try to dispatch, return @c true if success
    bool attemptCall(const std::string& name, const LLSD& event) const;

    std::string mDesc, mKey;
    typedef std::map<std::string, std::pair<Callable, LLSD> > DispatchMap;
    DispatchMap mDispatch;
};

/*==========================================================================*|
/// Have to implement these template specialization methods after
/// LLEventDispatcher so they can use its methods
template <typename CALLABLE>
void LLEventDetail::AddCallable<CALLABLE>::operator()(
    LLEventDispatcher& disp, const std::string& name, const CALLABLE& callable, const LLSD& required)
{
    disp.addImpl(name, callable, required);
}

template <class CLASS>
void LLEventDetail::AddCallable<void (CLASS::*)(const LLSD&)>::operator()(
    LLEventDispatcher& disp, const std::string& name, const Method& method, const LLSD& required)
{
    CLASS* downcast = dynamic_cast<CLASS*>(&disp);
    if (! downcast)
    {
        disp.addFail(name, typeid(CLASS).name());
    }
    else
    {
        disp.addImpl(name, boost::bind(method, downcast, _1), required);
    }
}

/// Have to overload for both const and non-const methods
template <class CLASS>
void LLEventDetail::AddCallable<void (CLASS::*)(const LLSD&) const>::operator()(
    LLEventDispatcher& disp, const std::string& name, const Method& method, const LLSD& required)
{
    // I am severely bummed that I have, as yet, found no way short of a
    // macro to avoid replicating the (admittedly brief) body of this overload.
    CLASS* downcast = dynamic_cast<CLASS*>(&disp);
    if (! downcast)
    {
        disp.addFail(name, typeid(CLASS).name());
    }
    else
    {
        disp.addImpl(name, boost::bind(method, downcast, _1), required);
    }
}
|*==========================================================================*/

/**
 * Bundle an LLEventPump and a listener with an LLEventDispatcher. A class
 * that contains (or derives from) LLDispatchListener need only specify the
 * LLEventPump name and dispatch key, and add() its methods. Incoming events
 * will automatically be dispatched.
 */
class LLDispatchListener: public LLEventDispatcher
{
public:
    LLDispatchListener(const std::string& pumpname, const std::string& key);

private:
    bool process(const LLSD& event);

    LLEventStream mPump;
    LLTempBoundListener mBoundListener;
};

#endif /* ! defined(LL_LLEVENTDISPATCHER_H) */
