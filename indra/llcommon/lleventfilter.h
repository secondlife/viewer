/**
 * @file   lleventfilter.h
 * @author Nat Goodspeed
 * @date   2009-03-05
 * @brief  Define LLEventFilter: LLEventStream subclass with conditions
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLEVENTFILTER_H)
#define LL_LLEVENTFILTER_H

#include "llevents.h"
#include "stdtypes.h"
#include "lltimer.h"
#include <boost/function.hpp>

/**
 * Generic base class
 */
class LLEventFilter: public LLEventStream
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
class LLEventTimeoutBase: public LLEventFilter
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

protected:
    virtual void setCountdown(F32 seconds) = 0;
    virtual bool countdownElapsed() const = 0;

private:
    bool tick(const LLSD&);

    LLBoundListener mMainloop;
    Action mAction;
};

/// Production implementation of LLEventTimoutBase
class LLEventTimeout: public LLEventTimeoutBase
{
public:
    LLEventTimeout();
    LLEventTimeout(LLEventPump& source);

protected:
    virtual void setCountdown(F32 seconds);
    virtual bool countdownElapsed() const;

private:
    LLTimer mTimer;
};

#endif /* ! defined(LL_LLEVENTFILTER_H) */
