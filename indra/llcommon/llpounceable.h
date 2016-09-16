/**
 * @file   llpounceable.h
 * @author Nat Goodspeed
 * @date   2015-05-22
 * @brief  LLPounceable is tangentially related to a future: it's a holder for
 *         a value that may or may not exist yet. Unlike a future, though,
 *         LLPounceable freely allows reading the held value. (If the held
 *         type T does not have a distinguished "empty" value, consider using
 *         LLPounceable<boost::optional<T>>.)
 *
 *         LLPounceable::callWhenReady() is this template's claim to fame. It
 *         allows its caller to "pounce" on the held value as soon as it
 *         becomes non-empty. Call callWhenReady() with any C++ callable
 *         accepting T. If the held value is already non-empty, callWhenReady()
 *         will immediately call the callable with the held value. If the held
 *         value is empty, though, callWhenReady() will enqueue the callable
 *         for later. As soon as LLPounceable is assigned a non-empty held
 *         value, it will flush the queue of deferred callables.
 *
 *         Consider a global LLMessageSystem* gMessageSystem. Message system
 *         initialization happens at a very specific point during viewer
 *         initialization. Other subsystems want to register callbacks on the
 *         LLMessageSystem instance as soon as it's initialized, but their own
 *         initialization may precede that. If we define gMessageSystem to be
 *         an LLPounceable<LLMessageSystem*>, a subsystem can use
 *         callWhenReady() to either register immediately (if gMessageSystem
 *         is already up and runnning) or register as soon as gMessageSystem
 *         is set with a new, initialized instance.
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Copyright (c) 2015, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLPOUNCEABLE_H)
#define LL_LLPOUNCEABLE_H

#include "llsingleton.h"
#include <boost/noncopyable.hpp>
#include <boost/call_traits.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/unordered_map.hpp>
#include <boost/signals2/signal.hpp>

// Forward declare the user template, since we want to be able to point to it
// in some of its implementation classes.
template <typename T, class TAG>
class LLPounceable;

template <typename T, typename TAG>
struct LLPounceableTraits
{
    // Our "queue" is a signal object with correct signature.
    typedef boost::signals2::signal<void (typename boost::call_traits<T>::param_type)> signal_t;
    // Call callWhenReady() with any callable accepting T.
    typedef typename signal_t::slot_type func_t;
    // owner pointer type
    typedef LLPounceable<T, TAG>* owner_ptr;
};

// Tag types distinguish the two different implementations of LLPounceable's
// queue.
struct LLPounceableQueue {};
struct LLPounceableStatic {};

// generic LLPounceableQueueImpl deliberately omitted: only the above tags are
// legal
template <typename T, class TAG>
class LLPounceableQueueImpl;

// The implementation selected by LLPounceableStatic uses an LLSingleton
// because we can't count on a data member queue being initialized at the time
// we start getting callWhenReady() calls. This is that LLSingleton.
template <typename T>
class LLPounceableQueueSingleton:
    public LLSingleton<LLPounceableQueueSingleton<T> >
{
    LLSINGLETON_EMPTY_CTOR(LLPounceableQueueSingleton);

    typedef LLPounceableTraits<T, LLPounceableStatic> traits;
    typedef typename traits::owner_ptr owner_ptr;
    typedef typename traits::signal_t signal_t;

    // For a given held type T, every LLPounceable<T, LLPounceableStatic>
    // instance will call on the SAME LLPounceableQueueSingleton instance --
    // given how class statics work. We must keep a separate queue for each
    // LLPounceable instance. Use a hash map for that.
    typedef boost::unordered_map<owner_ptr, signal_t> map_t;

public:
    // Disambiguate queues belonging to different LLPounceables.
    signal_t& get(owner_ptr owner)
    {
        // operator[] has find-or-create semantics -- just what we want!
        return mMap[owner];
    }

private:
    map_t mMap;
};

// LLPounceableQueueImpl that uses the above LLSingleton
template <typename T>
class LLPounceableQueueImpl<T, LLPounceableStatic>
{
public:
    typedef LLPounceableTraits<T, LLPounceableStatic> traits;
    typedef typename traits::owner_ptr owner_ptr;
    typedef typename traits::signal_t signal_t;

    signal_t& get(owner_ptr owner) const
    {
        // this Impl contains nothing; it delegates to the Singleton
        return LLPounceableQueueSingleton<T>::instance().get(owner);
    }
};

// The implementation selected by LLPounceableQueue directly contains the
// queue of interest, suitable for an LLPounceable we can trust to be fully
// initialized when it starts getting callWhenReady() calls.
template <typename T>
class LLPounceableQueueImpl<T, LLPounceableQueue>
{
public:
    typedef LLPounceableTraits<T, LLPounceableQueue> traits;
    typedef typename traits::owner_ptr owner_ptr;
    typedef typename traits::signal_t signal_t;

    signal_t& get(owner_ptr)
    {
        return mQueue;
    }

private:
    signal_t mQueue;
};

// LLPounceable<T> is for an LLPounceable instance on the heap or the stack.
// LLPounceable<T, LLPounceableStatic> is for a static LLPounceable instance.
template <typename T, class TAG=LLPounceableQueue>
class LLPounceable: public boost::noncopyable
{
private:
    typedef LLPounceableTraits<T, TAG> traits;
    typedef typename traits::owner_ptr owner_ptr;
    typedef typename traits::signal_t signal_t;

public:
    typedef typename traits::func_t func_t;

    // By default, both the initial value and the distinguished empty value
    // are a default-constructed T instance. However you can explicitly
    // specify each.
    LLPounceable(typename boost::call_traits<T>::value_type init =boost::value_initialized<T>(),
                 typename boost::call_traits<T>::param_type empty=boost::value_initialized<T>()):
        mHeld(init),
        mEmpty(empty)
    {}

    // make read access to mHeld as cheap and transparent as possible
    operator T () const { return mHeld; }
    typename boost::remove_pointer<T>::type operator*() const { return *mHeld; }
    typename boost::call_traits<T>::value_type operator->() const { return mHeld; }
    // uncomment 'explicit' as soon as we allow C++11 compilation
    /*explicit*/ operator bool() const { return bool(mHeld); }
    bool operator!() const { return ! mHeld; }

    // support both assignment (dumb ptr idiom) and reset() (smart ptr)
    void operator=(typename boost::call_traits<T>::param_type value)
    {
        reset(value);
    }

    void reset(typename boost::call_traits<T>::param_type value)
    {
        mHeld = value;
        // If this new value is non-empty, flush anything pending in the queue.
        if (mHeld != mEmpty)
        {
            signal_t& signal(get_signal());
            signal(mHeld);
            signal.disconnect_all_slots();
        }
    }

    // our claim to fame
    void callWhenReady(const func_t& func)
    {
        if (mHeld != mEmpty)
        {
            // If the held value is already non-empty, immediately call func()
            func(mHeld);
        }
        else
        {
            // Held value still empty, queue func() for later. By default,
            // connect() enqueues slots in FIFO order.
            get_signal().connect(func);
        }
    }

private:
    signal_t& get_signal() { return mQueue.get(this); }

    // Store both the current and the empty value.
    // MAYBE: Might be useful to delegate to LLPounceableTraits the meaning of
    // testing for "empty." For some types we want operator!(); for others we
    // want to compare to a distinguished value.
    typename boost::call_traits<T>::value_type mHeld, mEmpty;
    // This might either contain the queue (LLPounceableQueue) or delegate to
    // an LLSingleton (LLPounceableStatic).
    LLPounceableQueueImpl<T, TAG> mQueue;
};

#endif /* ! defined(LL_LLPOUNCEABLE_H) */
