/**
 * @file   lleventcoro.h
 * @author Nat Goodspeed
 * @date   2009-04-29
 * @brief  Utilities to interface between coroutines and events.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLEVENTCORO_H)
#define LL_LLEVENTCORO_H

#include <boost/coroutine/coroutine.hpp>
#include <boost/coroutine/future.hpp>
#include <boost/optional.hpp>
#include <string>
#include <stdexcept>
#include "llevents.h"
#include "llerror.h"

/**
 * Like LLListenerOrPumpName, this is a class intended for parameter lists:
 * accept a <tt>const LLEventPumpOrPumpName&</tt> and you can accept either an
 * <tt>LLEventPump&</tt> or its string name. For a single parameter that could
 * be either, it's not hard to overload the function -- but as soon as you
 * want to accept two such parameters, this is cheaper than four overloads.
 */
class LLEventPumpOrPumpName
{
public:
    /// Pass an actual LLEventPump&
    LLEventPumpOrPumpName(LLEventPump& pump):
        mPump(pump)
    {}
    /// Pass the string name of an LLEventPump
    LLEventPumpOrPumpName(const std::string& pumpname):
        mPump(LLEventPumps::instance().obtain(pumpname))
    {}
    /// Pass string constant name of an LLEventPump. This override must be
    /// explicit, since otherwise passing <tt>const char*</tt> to a function
    /// accepting <tt>const LLEventPumpOrPumpName&</tt> would require two
    /// different implicit conversions: <tt>const char*</tt> -> <tt>const
    /// std::string&</tt> -> <tt>const LLEventPumpOrPumpName&</tt>.
    LLEventPumpOrPumpName(const char* pumpname):
        mPump(LLEventPumps::instance().obtain(pumpname))
    {}
    /// Unspecified: "I choose not to identify an LLEventPump."
    LLEventPumpOrPumpName() {}
    operator LLEventPump& () const { return *mPump; }
    LLEventPump& getPump() const { return *mPump; }
    operator bool() const { return mPump; }
    bool operator!() const { return ! mPump; }

private:
    boost::optional<LLEventPump&> mPump;
};

/// This is an adapter for a signature like void LISTENER(const LLSD&), which
/// isn't a valid LLEventPump listener: such listeners should return bool.
template <typename LISTENER>
class LLVoidListener
{
public:
    LLVoidListener(const LISTENER& listener):
        mListener(listener)
    {}
    bool operator()(const LLSD& event)
    {
        mListener(event);
        // don't swallow the event, let other listeners see it
        return false;
    }
private:
    LISTENER mListener;
};

/// LLVoidListener helper function to infer the type of the LISTENER
template <typename LISTENER>
LLVoidListener<LISTENER> voidlistener(const LISTENER& listener)
{
    return LLVoidListener<LISTENER>(listener);
}

namespace LLEventDetail
{
    /**
     * waitForEventOn() permits a coroutine to temporarily listen on an
     * LLEventPump any number of times. We don't really want to have to ask
     * the caller to label each such call with a distinct string; the whole
     * point of waitForEventOn() is to present a nice sequential interface to
     * the underlying LLEventPump-with-named-listeners machinery. So we'll use
     * LLEventPump::inventName() to generate a distinct name for each
     * temporary listener. On the other hand, because a given coroutine might
     * call waitForEventOn() any number of times, we don't really want to
     * consume an arbitrary number of generated inventName()s: that namespace,
     * though large, is nonetheless finite. So we memoize an invented name for
     * each distinct coroutine instance (each different 'self' object). We
     * can't know the type of 'self', because it depends on the coroutine
     * body's signature. So we cast its address to void*, looking for distinct
     * pointer values. Yes, that means that an early coroutine could cache a
     * value here, then be destroyed, only to be supplanted by a later
     * coroutine (of the same or different type), and we'll end up
     * "recognizing" the second one and reusing the listener name -- but
     * that's okay, since it won't collide with any listener name used by the
     * earlier coroutine since that earlier coroutine no longer exists.
     */
    LL_COMMON_API std::string listenerNameForCoro(const void* self);

    /**
     * Implement behavior described for postAndWait()'s @a replyPumpNamePath
     * parameter:
     *
     * * If <tt>path.isUndefined()</tt>, do nothing.
     * * If <tt>path.isString()</tt>, @a dest is an LLSD map: store @a value
     *   into <tt>dest[path.asString()]</tt>.
     * * If <tt>path.isInteger()</tt>, @a dest is an LLSD array: store @a
     *   value into <tt>dest[path.asInteger()]</tt>.
     * * If <tt>path.isArray()</tt>, iteratively apply the rules above to step
     *   down through the structure of @a dest. The last array entry in @a
     *   path specifies the entry in the lowest-level structure in @a dest
     *   into which to store @a value.
     *
     * @note
     * In the degenerate case in which @a path is an empty array, @a dest will
     * @em become @a value rather than @em containing it.
     */
    LL_COMMON_API void storeToLLSDPath(LLSD& dest, const LLSD& path, const LLSD& value);
} // namespace LLEventDetail

/**
 * Post specified LLSD event on the specified LLEventPump, then wait for a
 * response on specified other LLEventPump. This is more than mere
 * convenience: the difference between this function and the sequence
 * @code
 * requestPump.post(myEvent);
 * LLSD reply = waitForEventOn(self, replyPump);
 * @endcode
 * is that the sequence above fails if the reply is posted immediately on
 * @a replyPump, that is, before <tt>requestPump.post()</tt> returns. In the
 * sequence above, the running coroutine isn't even listening on @a replyPump
 * until <tt>requestPump.post()</tt> returns and @c waitForEventOn() is
 * entered. Therefore, the coroutine completely misses an immediate reply
 * event, making it wait indefinitely.
 *
 * By contrast, postAndWait() listens on the @a replyPump @em before posting
 * the specified LLSD event on the specified @a requestPump.
 *
 * @param self The @c self object passed into a coroutine
 * @param event LLSD data to be posted on @a requestPump
 * @param requestPump an LLEventPump on which to post @a event. Pass either
 * the LLEventPump& or its string name. However, if you pass a
 * default-constructed @c LLEventPumpOrPumpName, we skip the post() call.
 * @param replyPump an LLEventPump on which postAndWait() will listen for a
 * reply. Pass either the LLEventPump& or its string name. The calling
 * coroutine will wait until that reply arrives. (If you're concerned about a
 * reply that might not arrive, please see also LLEventTimeout.)
 * @param replyPumpNamePath specifies the location within @a event in which to
 * store <tt>replyPump.getName()</tt>. This is a strictly optional convenience
 * feature; obviously you can store the name in @a event "by hand" if desired.
 * @a replyPumpNamePath can be specified in any of four forms:
 * * @c isUndefined() (default-constructed LLSD object): do nothing. This is
 *   the default behavior if you omit @a replyPumpNamePath.
 * * @c isInteger(): @a event is an array. Store <tt>replyPump.getName()</tt>
 *   in <tt>event[replyPumpNamePath.asInteger()]</tt>.
 * * @c isString(): @a event is a map. Store <tt>replyPump.getName()</tt> in
 *   <tt>event[replyPumpNamePath.asString()]</tt>.
 * * @c isArray(): @a event has several levels of structure, e.g. map of
 *   maps, array of arrays, array of maps, map of arrays, ... Store
 *   <tt>replyPump.getName()</tt> in
 *   <tt>event[replyPumpNamePath[0]][replyPumpNamePath[1]]...</tt> In other
 *   words, examine each array entry in @a replyPumpNamePath in turn. If it's an
 *   <tt>LLSD::String</tt>, the current level of @a event is a map; step down to
 *   that map entry. If it's an <tt>LLSD::Integer</tt>, the current level of @a
 *   event is an array; step down to that array entry. The last array entry in
 *   @a replyPumpNamePath specifies the entry in the lowest-level structure in
 *   @a event into which to store <tt>replyPump.getName()</tt>.
 */
template <typename SELF>
LLSD postAndWait(SELF& self, const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                 const LLEventPumpOrPumpName& replyPump, const LLSD& replyPumpNamePath=LLSD())
{
    // declare the future
    boost::coroutines::future<LLSD> future(self);
    // make a callback that will assign a value to the future, and listen on
    // the specified LLEventPump with that callback
    std::string listenerName(LLEventDetail::listenerNameForCoro(&self));
    LLTempBoundListener connection(
        replyPump.getPump().listen(listenerName,
                                   voidlistener(boost::coroutines::make_callback(future))));
    // skip the "post" part if requestPump is default-constructed
    if (requestPump)
    {
        // If replyPumpNamePath is non-empty, store the replyPump name in the
        // request event.
        LLSD modevent(event);
        LLEventDetail::storeToLLSDPath(modevent, replyPumpNamePath, replyPump.getPump().getName());
        LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << listenerName
                                 << " posting to " << requestPump.getPump().getName()
                                 << ": " << modevent << LL_ENDL;
        requestPump.getPump().post(modevent);
    }
    LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << listenerName
                             << " about to wait on LLEventPump " << replyPump.getPump().getName()
                             << LL_ENDL;
    // trying to dereference ("resolve") the future makes us wait for it
    LLSD value(*future);
    LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << listenerName
                             << " resuming with " << value << LL_ENDL;
    // returning should disconnect the connection
    return value;
}

/// Wait for the next event on the specified LLEventPump. Pass either the
/// LLEventPump& or its string name.
template <typename SELF>
LLSD waitForEventOn(SELF& self, const LLEventPumpOrPumpName& pump)
{
    // This is now a convenience wrapper for postAndWait().
    return postAndWait(self, LLSD(), LLEventPumpOrPumpName(), pump);
}

/// return type for two-pump variant of waitForEventOn()
typedef std::pair<LLSD, int> LLEventWithID;

namespace LLEventDetail
{
    /**
     * This helper is specifically for the two-pump version of waitForEventOn().
     * We use a single future object, but we want to listen on two pumps with it.
     * Since we must still adapt from (the callable constructed by)
     * boost::coroutines::make_callback() (void return) to provide an event
     * listener (bool return), we've adapted LLVoidListener for the purpose. The
     * basic idea is that we construct a distinct instance of WaitForEventOnHelper
     * -- binding different instance data -- for each of the pumps. Then, when a
     * pump delivers an LLSD value to either WaitForEventOnHelper, it can combine
     * that LLSD with its discriminator to feed the future object.
     */
    template <typename LISTENER>
    class WaitForEventOnHelper
    {
    public:
        WaitForEventOnHelper(const LISTENER& listener, int discriminator):
            mListener(listener),
            mDiscrim(discriminator)
        {}
        // this signature is required for an LLEventPump listener
        bool operator()(const LLSD& event)
        {
            // our future object is defined to accept LLEventWithID
            mListener(LLEventWithID(event, mDiscrim));
            // don't swallow the event, let other listeners see it
            return false;
        }
    private:
        LISTENER mListener;
        const int mDiscrim;
    };

    /// WaitForEventOnHelper type-inference helper
    template <typename LISTENER>
    WaitForEventOnHelper<LISTENER> wfeoh(const LISTENER& listener, int discriminator)
    {
        return WaitForEventOnHelper<LISTENER>(listener, discriminator);
    }
} // namespace LLEventDetail

/**
 * This function waits for a reply on either of two specified LLEventPumps.
 * Otherwise, it closely resembles postAndWait(); please see the documentation
 * for that function for detailed parameter info.
 *
 * While we could have implemented the single-pump variant in terms of this
 * one, there's enough added complexity here to make it worthwhile to give the
 * single-pump variant its own straightforward implementation. Conversely,
 * though we could use preprocessor logic to generate n-pump overloads up to
 * BOOST_COROUTINE_WAIT_MAX, we don't foresee a use case. This two-pump
 * overload exists because certain event APIs are defined in terms of a reply
 * LLEventPump and an error LLEventPump.
 *
 * The LLEventWithID return value provides not only the received event, but
 * the index of the pump on which it arrived (0 or 1).
 *
 * @note
 * I'd have preferred to overload the name postAndWait() for both signatures.
 * But consider the following ambiguous call:
 * @code
 * postAndWait(self, LLSD(), requestPump, replyPump, "someString");
 * @endcode
 * "someString" could be converted to either LLSD (@a replyPumpNamePath for
 * the single-pump function) or LLEventOrPumpName (@a replyPump1 for two-pump
 * function).
 *
 * It seems less burdensome to write postAndWait2() than to write either
 * LLSD("someString") or LLEventOrPumpName("someString").
 */
template <typename SELF>
LLEventWithID postAndWait2(SELF& self, const LLSD& event,
                           const LLEventPumpOrPumpName& requestPump,
                           const LLEventPumpOrPumpName& replyPump0,
                           const LLEventPumpOrPumpName& replyPump1,
                           const LLSD& replyPump0NamePath=LLSD(),
                           const LLSD& replyPump1NamePath=LLSD())
{
    // declare the future
    boost::coroutines::future<LLEventWithID> future(self);
    // either callback will assign a value to this future; listen on
    // each specified LLEventPump with a callback
    std::string name(LLEventDetail::listenerNameForCoro(&self));
    LLTempBoundListener connection0(
        replyPump0.getPump().listen(name + "a",
                               LLEventDetail::wfeoh(boost::coroutines::make_callback(future), 0)));
    LLTempBoundListener connection1(
        replyPump1.getPump().listen(name + "b",
                               LLEventDetail::wfeoh(boost::coroutines::make_callback(future), 1)));
    // skip the "post" part if requestPump is default-constructed
    if (requestPump)
    {
        // If either replyPumpNamePath is non-empty, store the corresponding
        // replyPump name in the request event.
        LLSD modevent(event);
        LLEventDetail::storeToLLSDPath(modevent, replyPump0NamePath,
                                       replyPump0.getPump().getName());
        LLEventDetail::storeToLLSDPath(modevent, replyPump1NamePath,
                                       replyPump1.getPump().getName());
        LL_DEBUGS("lleventcoro") << "postAndWait2(): coroutine " << name
                                 << " posting to " << requestPump.getPump().getName()
                                 << ": " << modevent << LL_ENDL;
        requestPump.getPump().post(modevent);
    }
    LL_DEBUGS("lleventcoro") << "postAndWait2(): coroutine " << name
                             << " about to wait on LLEventPumps " << replyPump0.getPump().getName()
                             << ", " << replyPump1.getPump().getName() << LL_ENDL;
    // trying to dereference ("resolve") the future makes us wait for it
    LLEventWithID value(*future);
    LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << name
                             << " resuming with (" << value.first << ", " << value.second << ")"
                             << LL_ENDL;
    // returning should disconnect both connections
    return value;
}

/**
 * Wait for the next event on either of two specified LLEventPumps.
 */
template <typename SELF>
LLEventWithID
waitForEventOn(SELF& self,
               const LLEventPumpOrPumpName& pump0, const LLEventPumpOrPumpName& pump1)
{
    // This is now a convenience wrapper for postAndWait2().
    return postAndWait2(self, LLSD(), LLEventPumpOrPumpName(), pump0, pump1);
}

/**
 * Helper for the two-pump variant of waitForEventOn(), e.g.:
 *
 * @code
 * LLSD reply = errorException(waitForEventOn(self, replyPump, errorPump),
 *                             "error response from login.cgi");
 * @endcode
 *
 * Examines an LLEventWithID, assuming that the second pump (pump 1) is
 * listening for an error indication. If the incoming data arrived on pump 1,
 * throw an LLErrorEvent exception. If the incoming data arrived on pump 0,
 * just return it. Since a normal return can only be from pump 0, we no longer
 * need the LLEventWithID's discriminator int; we can just return the LLSD.
 *
 * @note I'm not worried about introducing the (fairly generic) name
 * errorException() into global namespace, because how many other overloads of
 * the same name are going to accept an LLEventWithID parameter?
 */
LLSD errorException(const LLEventWithID& result, const std::string& desc);

/**
 * Exception thrown by errorException(). We don't call this LLEventError
 * because it's not an error in event processing: rather, this exception
 * announces an event that bears error information (for some other API).
 */
class LL_COMMON_API LLErrorEvent: public std::runtime_error
{
public:
    LLErrorEvent(const std::string& what, const LLSD& data):
        std::runtime_error(what),
        mData(data)
    {}
    virtual ~LLErrorEvent() throw() {}

    LLSD getData() const { return mData; }

private:
    LLSD mData;
};

/**
 * Like errorException(), save that this trips a fatal error using LL_ERRS
 * rather than throwing an exception.
 */
LL_COMMON_API LLSD errorLog(const LLEventWithID& result, const std::string& desc);

/**
 * Certain event APIs require the name of an LLEventPump on which they should
 * post results. While it works to invent a distinct name and let
 * LLEventPumps::obtain() instantiate the LLEventPump as a "named singleton,"
 * in a certain sense it's more robust to instantiate a local LLEventPump and
 * provide its name instead. This class packages the following idiom:
 *
 * 1. Instantiate a local LLCoroEventPump, with an optional name prefix.
 * 2. Provide its actual name to the event API in question as the name of the
 *    reply LLEventPump.
 * 3. Initiate the request to the event API.
 * 4. Call your LLEventTempStream's wait() method to wait for the reply.
 * 5. Let the LLCoroEventPump go out of scope.
 */
class LL_COMMON_API LLCoroEventPump
{
public:
    LLCoroEventPump(const std::string& name="coro"):
        mPump(name, true)           // allow tweaking the pump instance name
    {}
    /// It's typical to request the LLEventPump name to direct an event API to
    /// send its response to this pump.
    std::string getName() const { return mPump.getName(); }
    /// Less typically, we'd request the pump itself for some reason.
    LLEventPump& getPump() { return mPump; }

    /**
     * Wait for an event on this LLEventPump.
     *
     * @note
     * The other major usage pattern we considered was to bind @c self at
     * LLCoroEventPump construction time, which would avoid passing the
     * parameter to each wait() call. But if we were going to bind @c self as
     * a class member, we'd need to specify a class template parameter
     * indicating its type. The big advantage of passing it to the wait() call
     * is that the type can be implicit.
     */
    template <typename SELF>
    LLSD wait(SELF& self)
    {
        return waitForEventOn(self, mPump);
    }

    template <typename SELF>
    LLSD postAndWait(SELF& self, const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                     const LLSD& replyPumpNamePath=LLSD())
    {
        return ::postAndWait(self, event, requestPump, mPump, replyPumpNamePath);
    }

private:
    LLEventStream mPump;
};

/**
 * Other event APIs require the names of two different LLEventPumps: one for
 * success response, the other for error response. Extend LLCoroEventPump
 * for the two-pump use case.
 */
class LL_COMMON_API LLCoroEventPumps
{
public:
    LLCoroEventPumps(const std::string& name="coro",
                     const std::string& suff0="Reply",
                     const std::string& suff1="Error"):
        mPump0(name + suff0, true),   // allow tweaking the pump instance name
        mPump1(name + suff1, true)
    {}
    /// request pump 0's name
    std::string getName0() const { return mPump0.getName(); }
    /// request pump 1's name
    std::string getName1() const { return mPump1.getName(); }
    /// request both names
    std::pair<std::string, std::string> getNames() const
    {
        return std::pair<std::string, std::string>(mPump0.getName(), mPump1.getName());
    }

    /// request pump 0
    LLEventPump& getPump0() { return mPump0; }
    /// request pump 1
    LLEventPump& getPump1() { return mPump1; }

    /// waitForEventOn(self, either of our two LLEventPumps)
    template <typename SELF>
    LLEventWithID wait(SELF& self)
    {
        return waitForEventOn(self, mPump0, mPump1);
    }

    /// errorException(wait(self))
    template <typename SELF>
    LLSD waitWithException(SELF& self)
    {
        return errorException(wait(self), std::string("Error event on ") + getName1());
    }

    /// errorLog(wait(self))
    template <typename SELF>
    LLSD waitWithLog(SELF& self)
    {
        return errorLog(wait(self), std::string("Error event on ") + getName1());
    }

    template <typename SELF>
    LLEventWithID postAndWait(SELF& self, const LLSD& event,
                              const LLEventPumpOrPumpName& requestPump,
                              const LLSD& replyPump0NamePath=LLSD(),
                              const LLSD& replyPump1NamePath=LLSD())
    {
        return postAndWait2(self, event, requestPump, mPump0, mPump1,
                            replyPump0NamePath, replyPump1NamePath);
    }

    template <typename SELF>
    LLSD postAndWaitWithException(SELF& self, const LLSD& event,
                                  const LLEventPumpOrPumpName& requestPump,
                                  const LLSD& replyPump0NamePath=LLSD(),
                                  const LLSD& replyPump1NamePath=LLSD())
    {
        return errorException(postAndWait(self, event, requestPump,
                                          replyPump0NamePath, replyPump1NamePath),
                              std::string("Error event on ") + getName1());
    }

    template <typename SELF>
    LLSD postAndWaitWithLog(SELF& self, const LLSD& event,
                            const LLEventPumpOrPumpName& requestPump,
                            const LLSD& replyPump0NamePath=LLSD(),
                            const LLSD& replyPump1NamePath=LLSD())
    {
        return errorLog(postAndWait(self, event, requestPump,
                                    replyPump0NamePath, replyPump1NamePath),
                        std::string("Error event on ") + getName1());
    }

private:
    LLEventStream mPump0, mPump1;
};

#endif /* ! defined(LL_LLEVENTCORO_H) */
