/**
 * @file   lleventfilter.h
 * @author Nat Goodspeed
 * @date   2009-03-05
 * @brief  Define LLEventFilter: LLEventStream subclass with conditions
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

#if ! defined(LL_LLEVENTFILTER_H)
#define LL_LLEVENTFILTER_H

#include "llevents.h"
#include "stdtypes.h"
#include "lltimer.h"
#include "llsdutil.h"
#include <boost/function.hpp>

class LLEventTimer;
class LLDate;

/**
 * Generic base class
 */
class LL_COMMON_API LLEventFilter: public LLEventStream
{
public:
    /// construct a standalone LLEventFilter
    LLEventFilter(const std::string& name="filter", bool tweak=true):
        LLEventStream(name, tweak)
    {}
    /// construct LLEventFilter and connect it to the specified LLEventPump
    LLEventFilter(LLEventPump& source, const std::string& name="filter", bool tweak=true);

    /// Post an event to all listeners
    virtual bool post(const LLSD& event) = 0;

private:
    LLTempBoundListener mSource;
};

/**
 * Pass through only events matching a specified pattern
 */
class LLEventMatching: public LLEventFilter
{
public:
    /// Pass an LLSD map with keys and values the incoming event must match
    LLEventMatching(const LLSD& pattern);
    /// instantiate and connect
    LLEventMatching(LLEventPump& source, const LLSD& pattern);

    /// Only pass through events matching the pattern
    virtual bool post(const LLSD& event);

private:
    LLSD mPattern;
};

/**
 * Wait for an event to be posted. If no such event arrives within a specified
 * time, take a specified action. See LLEventTimeout for production
 * implementation.
 *
 * @NOTE This is an abstract base class so that, for testing, we can use an
 * alternate "timer" that doesn't actually consume real time.
 */
class LL_COMMON_API LLEventTimeoutBase: public LLEventFilter
{
public:
    /// construct standalone
    LLEventTimeoutBase();
    /// construct and connect
    LLEventTimeoutBase(LLEventPump& source);

    /// Callable, can be constructed with boost::bind()
    typedef boost::function<void()> Action;

    /**
     * Start countdown timer for the specified number of @a seconds. Forward
     * all events. If any event arrives before timer expires, cancel timer. If
     * no event arrives before timer expires, take specified @a action.
     *
     * This is a one-shot timer. Once it has either expired or been canceled,
     * it is inert until another call to actionAfter().
     *
     * Calling actionAfter() while an existing timer is running cheaply
     * replaces that original timer. Thus, a valid use case is to detect
     * idleness of some event source by calling actionAfter() on each new
     * event. A rapid sequence of events will keep the timer from expiring;
     * the first gap in events longer than the specified timer will fire the
     * specified Action.
     *
     * Any post() call cancels the timer. To be satisfied with only a
     * particular event, chain on an LLEventMatching that only passes such
     * events:
     *
     * @code
     * event                                                 ultimate
     * source ---> LLEventMatching ---> LLEventTimeout  ---> listener
     * @endcode
     *
     * @NOTE
     * The implementation relies on frequent events on the LLEventPump named
     * "mainloop".
     */
    void actionAfter(F32 seconds, const Action& action);

    /**
     * Like actionAfter(), but where the desired Action is LL_ERRS
     * termination. Pass the timeout time and the desired LL_ERRS @a message.
     *
     * This method is useful when, for instance, some async API guarantees an
     * event, whether success or failure, within a stated time window.
     * Instantiate an LLEventTimeout listening to that API and call
     * errorAfter() on each async request with a timeout comfortably longer
     * than the API's time guarantee (much longer than the anticipated
     * "mainloop" granularity).
     *
     * Then if the async API breaks its promise, the program terminates with
     * the specified LL_ERRS @a message. The client of the async API can
     * therefore assume the guarantee is upheld.
     *
     * @NOTE
     * errorAfter() is implemented in terms of actionAfter(), so all remarks
     * about calling actionAfter() also apply to errorAfter().
     */
    void errorAfter(F32 seconds, const std::string& message);

    /**
     * Like actionAfter(), but where the desired Action is a particular event
     * for all listeners. Pass the timeout time and the desired @a event data.
     * 
     * Suppose the timeout should only be satisfied by a particular event, but
     * the ultimate listener must see all other incoming events as well, plus
     * the timeout @a event if any:
     * 
     * @code
     * some        LLEventMatching                           LLEventMatching
     * event  ---> for particular  ---> LLEventTimeout  ---> for timeout
     * source      event                                     event \
     *       \                                                      \ ultimate
     *        `-----------------------------------------------------> listener
     * @endcode
     * 
     * Since a given listener can listen on more than one LLEventPump, we can
     * set things up so it sees the set union of events from LLEventTimeout
     * and the original event source. However, as LLEventTimeout passes
     * through all incoming events, the "particular event" that satisfies the
     * left LLEventMatching would reach the ultimate listener twice. So we add
     * an LLEventMatching that only passes timeout events.
     *
     * @NOTE
     * eventAfter() is implemented in terms of actionAfter(), so all remarks
     * about calling actionAfter() also apply to eventAfter().
     */
    void eventAfter(F32 seconds, const LLSD& event);

    /// Pass event through, canceling the countdown timer
    virtual bool post(const LLSD& event);

    /// Cancel timer without event
    void cancel();

    /// Is this timer currently running?
    bool running() const;

protected:
    virtual void setCountdown(F32 seconds) = 0;
    virtual bool countdownElapsed() const = 0;

private:
    bool tick(const LLSD&);

    LLTempBoundListener mMainloop;
    Action mAction;
};

/**
 * Production implementation of LLEventTimoutBase.
 * 
 * @NOTE: Caution should be taken when using the LLEventTimeout(LLEventPump &) 
 * constructor to ensure that the upstream event pump is not an LLEventMaildrop
 * or any other kind of store and forward pump which may have events outstanding.
 * Using this constructor will cause the upstream event pump to fire any pending
 * events and could result in the invocation of a virtual method before the timeout
 * has been fully constructed. The timeout should instead be connected upstream
 * from the event pump and attached using the listen method.
 * See llcoro::suspendUntilEventOnWithTimeout() for an example.
 */
 
class LL_COMMON_API LLEventTimeout: public LLEventTimeoutBase
{
public:
    LLEventTimeout();
    LLEventTimeout(LLEventPump& source);

    /// using LLEventTimeout as namespace for free functions
    /// Post event to specified LLEventPump every period seconds. Delete
    /// returned LLEventTimer* to cancel.
    static LLEventTimer* post_every(F32 period, const std::string& pump, const LLSD& data);
    /// Post event to specified LLEventPump at specified future time. Call
    /// LLEventTimer::getInstance(returned pointer) to check whether it's still
    /// pending; if so, delete the pointer to cancel.
    static LLEventTimer* post_at(const LLDate& time, const std::string& pump, const LLSD& data);
    /// Post event to specified LLEventPump after specified interval. Call
    /// LLEventTimer::getInstance(returned pointer) to check whether it's still
    /// pending; if so, delete the pointer to cancel.
    static LLEventTimer* post_after(F32 interval, const std::string& pump, const LLSD& data);

protected:
    virtual void setCountdown(F32 seconds);
    virtual bool countdownElapsed() const;

private:
    LLTimer mTimer;
};

/**
 * LLEventBatch: accumulate post() events (LLSD blobs) into an LLSD Array
 * until the array reaches a certain size, then call listeners with the Array
 * and clear it back to empty.
 */
class LL_COMMON_API LLEventBatch: public LLEventFilter
{
public:
    // pass batch size
    LLEventBatch(std::size_t size);
    // construct and connect
    LLEventBatch(LLEventPump& source, std::size_t size);

    // force out the pending batch
    void flush();

    // accumulate an event and flush() when big enough
    virtual bool post(const LLSD& event);

    // query or reset batch size
    std::size_t getSize() const { return mBatchSize; }
    void setSize(std::size_t size);

private:
    LLSD mBatch;
    std::size_t mBatchSize;
};

/**
 * LLEventThrottleBase: construct with a time interval. Regardless of how
 * frequently you call post(), LLEventThrottle will pass on an event to
 * its listeners no more often than once per specified interval.
 *
 * A new event after more than the specified interval will immediately be
 * passed along to listeners. But subsequent events will be delayed until at
 * least one time interval since listeners were last called. Consider the
 * sequence below. Suppose we have an LLEventThrottle constructed with an
 * interval of 3 seconds. The numbers on the left are timestamps in seconds
 * relative to an arbitrary reference point.
 *
 *  1: post(): event immediately passed to listeners, next no sooner than 4
 *  2: post(): deferred: waiting for 3 seconds to elapse
 *  3: post(): deferred
 *  4: no post() call, but event delivered to listeners; next no sooner than 7
 *  6: post(): deferred
 *  7: no post() call, but event delivered; next no sooner than 10
 * 12: post(): immediately passed to listeners, next no sooner than 15
 * 17: post(): immediately passed to listeners, next no sooner than 20
 *
 * For a deferred event, the LLSD blob delivered to listeners is from the most
 * recent deferred post() call. However, a sender may obtain the previous
 * event blob by calling pending(), modifying it as desired and post()ing the
 * new value. (See LLEventBatchThrottle.) Each time an event is delivered to
 * listeners, the pending() value is reset to isUndefined().
 *
 * You may also call flush() to immediately pass along any deferred events to
 * all listeners.
 *
 * @NOTE This is an abstract base class so that, for testing, we can use an
 * alternate "timer" that doesn't actually consume real time. See
 * LLEventThrottle.
 */
class LL_COMMON_API LLEventThrottleBase: public LLEventFilter
{
public:
    // pass time interval
    LLEventThrottleBase(F32 interval);
    // construct and connect
    LLEventThrottleBase(LLEventPump& source, F32 interval);

    // force out any deferred events
    void flush();

    // retrieve (aggregate) deferred event since last event sent to listeners
    LLSD pending() const;

    // register an event, may be either passed through or deferred
    virtual bool post(const LLSD& event);

    // query or reset interval
    F32 getInterval() const { return mInterval; }
    void setInterval(F32 interval);

    // deferred posts
    std::size_t getPostCount() const { return mPosts; }

    // time until next event would be passed through, 0.0 if now
    F32 getDelay() const;

protected:
    // Implement these time-related methods for a valid LLEventThrottleBase
    // subclass (see LLEventThrottle). For testing, we use a subclass that
    // doesn't involve actual elapsed time.
    virtual void alarmActionAfter(F32 interval, const LLEventTimeoutBase::Action& action) = 0;
    virtual bool alarmRunning() const = 0;
    virtual void alarmCancel() = 0;
    virtual void timerSet(F32 interval) = 0;
    virtual F32  timerGetRemaining() const = 0;

private:
    // remember throttle interval
    F32 mInterval;
    // count post() calls since last flush()
    std::size_t mPosts;
    // pending event data from most recent deferred event
    LLSD mPending;
};

/**
 * Production implementation of LLEventThrottle.
 */
class LLEventThrottle: public LLEventThrottleBase
{
public:
    LLEventThrottle(F32 interval);
    LLEventThrottle(LLEventPump& source, F32 interval);

private:
    virtual void alarmActionAfter(F32 interval, const LLEventTimeoutBase::Action& action) /*override*/;
    virtual bool alarmRunning() const /*override*/;
    virtual void alarmCancel() /*override*/;
    virtual void timerSet(F32 interval) /*override*/;
    virtual F32  timerGetRemaining() const /*override*/;

    // use this to arrange a deferred flush() call
    LLEventTimeout mAlarm;
    // use this to track whether we're within mInterval of last flush()
    LLTimer mTimer;
};

/**
 * LLEventBatchThrottle: like LLEventThrottle, it's reluctant to pass events
 * to listeners more often than once per specified time interval -- but only
 * reluctant, since exceeding the specified batch size limit can cause it to
 * deliver accumulated events sooner. Like LLEventBatch, it accumulates
 * pending events into an LLSD Array, optionally flushing when the batch grows
 * to a certain size.
 */
class LLEventBatchThrottle: public LLEventThrottle
{
public:
    // pass time interval and (optionally) max batch size; 0 means batch can
    // grow arbitrarily large
    LLEventBatchThrottle(F32 interval, std::size_t size = 0);
    // construct and connect
    LLEventBatchThrottle(LLEventPump& source, F32 interval, std::size_t size = 0);

    // append a new event to current batch
    virtual bool post(const LLSD& event);

    // query or reset batch size
    std::size_t getSize() const { return mBatchSize; }
    void setSize(std::size_t size);

private:
    std::size_t mBatchSize;
};

/**
 * LLStoreListener self-registers on the LLEventPump of interest, and
 * unregisters on destruction. As long as it exists, a particular element is
 * extracted from every event that comes through the upstream LLEventPump and
 * stored into the target variable.
 *
 * This is implemented as a subclass of LLEventFilter, though strictly
 * speaking it isn't really a "filter" at all: it never passes incoming events
 * to its own listeners, if any.
 *
 * TBD: A variant based on output iterators that stores and then increments
 * the iterator. Useful with boost::coroutine2!
 */
template <typename T>
class LLStoreListener: public LLEventFilter
{
public:
    // pass target and optional path to element
    LLStoreListener(T& target, const LLSD& path=LLSD(), bool consume=false):
        LLEventFilter("store"),
        mTarget(target),
        mPath(path),
        mConsume(consume)
    {}
    // construct and connect
    LLStoreListener(LLEventPump& source, T& target, const LLSD& path=LLSD(), bool consume=false):
        LLEventFilter(source, "store"),
        mTarget(target),
        mPath(path),
        mConsume(consume)
    {}

    // Calling post() with an LLSD event extracts the element indicated by
    // path, then stores it to mTarget.
    virtual bool post(const LLSD& event)
    {
        // Extract the element specified by 'mPath' from 'event'. To perform a
        // generic type-appropriate store through mTarget, construct an
        // LLSDParam<T> and store that, thus engaging LLSDParam's custom
        // conversions.
        mTarget = LLSDParam<T>(llsd::drill(event, mPath));
        return mConsume;
    }

private:
    T& mTarget;
    const LLSD mPath;
    const bool mConsume;
};

/*****************************************************************************
*   LLEventLogProxy
*****************************************************************************/
/**
 * LLEventLogProxy is a little different than the other LLEventFilter
 * subclasses declared in this header file, in that it completely wraps the
 * passed LLEventPump (both input and output) instead of simply processing its
 * output. Of course, if someone directly posts to the wrapped LLEventPump by
 * looking up its string name in LLEventPumps, LLEventLogProxy can't intercept
 * that post() call. But as long as consuming code is willing to access the
 * LLEventLogProxy instance instead of the wrapped LLEventPump, all event data
 * both post()ed and received is logged.
 *
 * The proxy role means that LLEventLogProxy intercepts more of LLEventPump's
 * API than a typical LLEventFilter subclass.
 */
class LLEventLogProxy: public LLEventFilter
{
    typedef LLEventFilter super;
public:
    /**
     * Construct LLEventLogProxy, wrapping the specified LLEventPump.
     * Unlike a typical LLEventFilter subclass, the name parameter is @emph
     * not optional because typically you want LLEventLogProxy to completely
     * replace the wrapped LLEventPump. So you give the subject LLEventPump
     * some other name and give the LLEventLogProxy the name that would have
     * been used for the subject LLEventPump.
     */
    LLEventLogProxy(LLEventPump& source, const std::string& name, bool tweak=false);

    /// register a new listener
    LLBoundListener listen_impl(const std::string& name, const LLEventListener& target,
                                const NameList& after, const NameList& before);

    /// Post an event to all listeners
    virtual bool post(const LLSD& event) /* override */;

private:
    /// This method intercepts each call to any target listener. We pass it
    /// the listener name and the caller's intended target listener plus the
    /// posted LLSD event.
    bool listener(const std::string& name,
                  const LLEventListener& target,
                  const LLSD& event) const;

    LLEventPump& mPump;
    LLSD::Integer mCounter{0};
};

/**
 * LLEventPumpHolder<T> is a helper for LLEventLogProxyFor<T>. It simply
 * stores an instance of T, presumably a subclass of LLEventPump. We derive
 * LLEventLogProxyFor<T> from LLEventPumpHolder<T>, ensuring that
 * LLEventPumpHolder's contained mWrappedPump is fully constructed before
 * passing it to LLEventLogProxyFor's LLEventLogProxy base class constructor.
 * But since LLEventPumpHolder<T> presents none of the LLEventPump API,
 * LLEventLogProxyFor<T> inherits its methods unambiguously from
 * LLEventLogProxy.
 */
template <class T>
class LLEventPumpHolder
{
protected:
    LLEventPumpHolder(const std::string& name, bool tweak=false):
        mWrappedPump(name, tweak)
    {}
    T mWrappedPump;
};

/**
 * LLEventLogProxyFor<T> is a wrapper around any of the LLEventPump subclasses.
 * Instantiating an LLEventLogProxy<T> instantiates an internal T. Otherwise
 * it behaves like LLEventLogProxy.
 */
template <class T>
class LLEventLogProxyFor: private LLEventPumpHolder<T>, public LLEventLogProxy
{
    // We derive privately from LLEventPumpHolder because it's an
    // implementation detail of LLEventLogProxyFor. The only reason it's a
    // base class at all is to guarantee that it's constructed first so we can
    // pass it to our LLEventLogProxy base class constructor.
    typedef LLEventPumpHolder<T> holder;
    typedef LLEventLogProxy super;

public:
    LLEventLogProxyFor(const std::string& name, bool tweak=false):
        // our wrapped LLEventPump subclass instance gets a name suffix
        // because that's not the LLEventPump we want consumers to obtain when
        // they ask LLEventPumps for this name
        holder(name + "-", tweak),
        // it's our LLEventLogProxy that gets the passed name
        super(holder::mWrappedPump, name, tweak)
    {}
};

#endif /* ! defined(LL_LLEVENTFILTER_H) */
