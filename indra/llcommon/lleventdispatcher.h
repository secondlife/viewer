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
#include <boost/iterator/transform_iterator.hpp>
#include <typeinfo>
#include "llevents.h"

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

    /// Accept any C++ callable, typically a boost::bind() expression
    typedef boost::function<void(const LLSD&)> Callable;

    /**
     * Register a @a callable by @a name. The optional @a required parameter
     * is used to validate the structure of each incoming event (see
     * llsd_matches()).
     */
    void add(const std::string& name,
             const std::string& desc,
             const Callable& callable,
             const LLSD& required=LLSD());

    /**
     * Special case: a subclass of this class can pass an unbound member
     * function pointer without explicitly specifying the
     * <tt>boost::bind()</tt> expression.
     */
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(const LLSD&),
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }

    /// Overload for both const and non-const methods
    template <class CLASS>
    void add(const std::string& name,
             const std::string& desc,
             void (CLASS::*method)(const LLSD&) const,
             const LLSD& required=LLSD())
    {
        addMethod<CLASS>(name, desc, method, required);
    }

    /// Convenience: for LLEventDispatcher, not every callable needs a
    /// documentation string.
    template <typename CALLABLE>
    void add(const std::string& name,
             CALLABLE callable,
             const LLSD& required=LLSD())
    {
        add(name, "", callable, required);
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

    /// @name Iterate over defined names
    //@{
    typedef std::pair<std::string, std::string> NameDesc;

private:
    struct DispatchEntry
    {
        DispatchEntry(const Callable& func, const std::string& desc, const LLSD& required):
            mFunc(func),
            mDesc(desc),
            mRequired(required)
        {}
        Callable mFunc;
        std::string mDesc;
        LLSD mRequired;
    };
    typedef std::map<std::string, DispatchEntry> DispatchMap;

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

    /// Fetch the Callable for the specified name. If no such name was
    /// registered, return an empty() Callable.
    Callable get(const std::string& name) const;

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
    /// try to dispatch, return @c true if success
    bool attemptCall(const std::string& name, const LLSD& event) const;

    std::string mDesc, mKey;
    DispatchMap mDispatch;

    static NameDesc makeNameDesc(const DispatchMap::value_type& item)
    {
        return NameDesc(item.first, item.second.mDesc);
    }
};

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
