/**
 * @file   lllistenerwrapper.h
 * @author Nat Goodspeed
 * @date   2009-11-30
 * @brief  Introduce LLListenerWrapper template
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLLISTENERWRAPPER_H)
#define LL_LLLISTENERWRAPPER_H

#include "llevents.h"               // LLListenerWrapperBase
#include <boost/visit_each.hpp>

/**
 * Template base class for coding wrappers for LLEventPump listeners.
 *
 * Derive your listener wrapper from LLListenerWrapper. You must use
 * LLLISTENER_WRAPPER_SUBCLASS() so your subclass will play nicely with
 * boost::visit_each (q.v.). That way boost::signals2 can still detect
 * derivation from LLEventTrackable, and so forth.
 */
template <typename LISTENER>
class LL_COMMON_API LLListenerWrapper: public LLListenerWrapperBase
{
public:
    /// Wrap an arbitrary listener object
    LLListenerWrapper(const LISTENER& listener):
        mListener(listener)
    {}

    /// call
    virtual bool operator()(const LLSD& event)
    {
        return mListener(event);
    }

    /// Allow boost::visit_each() to peek at our mListener.
    template <class V>
    void accept_visitor(V& visitor) const
    {
        using boost::visit_each;
        visit_each(visitor, mListener, 0);
    }

private:
    LISTENER mListener;
};

/**
 * Specialize boost::visit_each() (leveraging ADL) to peek inside an
 * LLListenerWrapper<T> to traverse its LISTENER. We borrow the
 * accept_visitor() pattern from boost::bind(), avoiding the need to make
 * mListener public.
 */
template <class V, typename T>
void visit_each(V& visitor, const LLListenerWrapper<T>& wrapper, int)
{
    wrapper.accept_visitor(visitor);
}

/// use this (sigh!) for each subclass of LLListenerWrapper<T> you write
#define LLLISTENER_WRAPPER_SUBCLASS(CLASS)                              \
template <class V, typename T>                                          \
void visit_each(V& visitor, const CLASS<T>& wrapper, int)               \
{                                                                       \
    visit_each(visitor, static_cast<const LLListenerWrapper<T>&>(wrapper), 0); \
}                                                                       \
                                                                        \
/* Have to state this explicitly, rather than using LL_TEMPLATE_CONVERTIBLE, */ \
/* because the source type is itself a template. */                     \
template <typename T>                                                   \
struct ll_template_cast_impl<const LLListenerWrapperBase*, const CLASS<T>*> \
{                                                                       \
    const LLListenerWrapperBase* operator()(const CLASS<T>* wrapper)    \
    {                                                                   \
        return wrapper;                                                 \
    }                                                                   \
}

/**
 * Make an instance of a listener wrapper. Every wrapper class must be a
 * template accepting a listener object of arbitrary type. In particular, the
 * type of a boost::bind() expression is deliberately undocumented. So we
 * can't just write Wrapper<CorrectType>(boost::bind(...)). Instead we must
 * write llwrap<Wrapper>(boost::bind(...)).
 */
template <template<typename> class WRAPPER, typename T>
WRAPPER<T> LL_COMMON_API llwrap(const T& listener)
{
    return WRAPPER<T>(listener);
}

/**
 * This LLListenerWrapper template subclass is used to report entry/exit to an
 * event listener, by changing this:
 * @code
 * someEventPump.listen("MyClass",
 *                      boost::bind(&MyClass::method, ptr, _1));
 * @endcode
 * to this:
 * @code
 * someEventPump.listen("MyClass",
 *                      llwrap<LLCoutListener>(
 *                      boost::bind(&MyClass::method, ptr, _1)));
 * @endcode
 */
template <class LISTENER>
class LL_COMMON_API LLCoutListener: public LLListenerWrapper<LISTENER>
{
    typedef LLListenerWrapper<LISTENER> super;

public:
    /// Wrap an arbitrary listener object
    LLCoutListener(const LISTENER& listener):
        super(listener)
    {}

    /// call
    virtual bool operator()(const LLSD& event)
    {
        std::cout << "Entering listener " << *super::mName << " with " << event << std::endl;
        bool handled = super::operator()(event);
        std::cout << "Leaving  listener " << *super::mName;
        if (handled)
        {
            std::cout << " (handled)";
        }
        std::cout << std::endl;
        return handled;
    }
};

LLLISTENER_WRAPPER_SUBCLASS(LLCoutListener);

/**
 * This LLListenerWrapper template subclass is used to log entry/exit to an
 * event listener, by changing this:
 * @code
 * someEventPump.listen("MyClass",
 *                      boost::bind(&MyClass::method, ptr, _1));
 * @endcode
 * to this:
 * @code
 * someEventPump.listen("MyClass",
 *                      llwrap<LLLogListener>(
 *                      boost::bind(&MyClass::method, ptr, _1)));
 * @endcode
 */
template <class LISTENER>
class LL_COMMON_API LLLogListener: public LLListenerWrapper<LISTENER>
{
    typedef LLListenerWrapper<LISTENER> super;

public:
    /// Wrap an arbitrary listener object
    LLLogListener(const LISTENER& listener):
        super(listener)
    {}

    /// call
    virtual bool operator()(const LLSD& event)
    {
        LL_DEBUGS("LLLogListener") << "Entering listener " << *super::mName << " with " << event << LL_ENDL;
        bool handled = super::operator()(event);
        LL_DEBUGS("LLLogListener") << "Leaving  listener " << *super::mName;
        if (handled)
        {
            LL_CONT << " (handled)";
        }
        LL_CONT << LL_ENDL;
        return handled;
    }
};

LLLISTENER_WRAPPER_SUBCLASS(LLLogListener);

#endif /* ! defined(LL_LLLISTENERWRAPPER_H) */
