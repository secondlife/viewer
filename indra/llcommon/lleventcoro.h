/**
 * @file   lleventcoro.h
 * @author Nat Goodspeed
 * @date   2009-04-29
 * @brief  Utilities to interface between coroutines and events.
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

#if ! defined(LL_LLEVENTCORO_H)
#define LL_LLEVENTCORO_H

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
    operator bool() const { return bool(mPump); }
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
     * each distinct coroutine instance.
     */
    std::string listenerNameForCoro();

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
 * LLSD reply = waitForEventOn(replyPump);
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
LLSD postAndWait(const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                 const LLEventPumpOrPumpName& replyPump, const LLSD& replyPumpNamePath=LLSD());

/// Wait for the next event on the specified LLEventPump. Pass either the
/// LLEventPump& or its string name.
inline
LLSD waitForEventOn(const LLEventPumpOrPumpName& pump)
{
    // This is now a convenience wrapper for postAndWait().
    return postAndWait(LLSD(), LLEventPumpOrPumpName(), pump);
}

/// return type for two-pump variant of waitForEventOn()
typedef std::pair<LLSD, int> LLEventWithID;

namespace LLEventDetail
{
    /**
     * This helper is specifically for the two-pump version of waitForEventOn().
     * We use a single future object, but we want to listen on two pumps with it.
     * Since we must still adapt from (the callable constructed by)
     * boost::dcoroutines::make_callback() (void return) to provide an event
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
 * postAndWait(LLSD(), requestPump, replyPump, "someString");
 * @endcode
 * "someString" could be converted to either LLSD (@a replyPumpNamePath for
 * the single-pump function) or LLEventOrPumpName (@a replyPump1 for two-pump
 * function).
 *
 * It seems less burdensome to write postAndWait2() than to write either
 * LLSD("someString") or LLEventOrPumpName("someString").
 */
LLEventWithID postAndWait2(const LLSD& event,
                           const LLEventPumpOrPumpName& requestPump,
                           const LLEventPumpOrPumpName& replyPump0,
                           const LLEventPumpOrPumpName& replyPump1,
                           const LLSD& replyPump0NamePath=LLSD(),
                           const LLSD& replyPump1NamePath=LLSD());

/**
 * Wait for the next event on either of two specified LLEventPumps.
 */
inline
LLEventWithID
waitForEventOn(const LLEventPumpOrPumpName& pump0, const LLEventPumpOrPumpName& pump1)
{
    // This is now a convenience wrapper for postAndWait2().
    return postAndWait2(LLSD(), LLEventPumpOrPumpName(), pump0, pump1);
}

/**
 * Helper for the two-pump variant of waitForEventOn(), e.g.:
 *
 * @code
 * LLSD reply = errorException(waitForEventOn(replyPump, errorPump),
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
     */
    LLSD wait()
    {
        return ::waitForEventOn(mPump);
    }

    LLSD postAndWait(const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                     const LLSD& replyPumpNamePath=LLSD())
    {
        return ::postAndWait(event, requestPump, mPump, replyPumpNamePath);
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

    /// waitForEventOn(either of our two LLEventPumps)
    LLEventWithID wait()
    {
        return waitForEventOn(mPump0, mPump1);
    }

    /// errorException(wait())
    LLSD waitWithException()
    {
        return errorException(wait(), std::string("Error event on ") + getName1());
    }

    /// errorLog(wait())
    LLSD waitWithLog()
    {
        return errorLog(wait(), std::string("Error event on ") + getName1());
    }

    LLEventWithID postAndWait(const LLSD& event,
                              const LLEventPumpOrPumpName& requestPump,
                              const LLSD& replyPump0NamePath=LLSD(),
                              const LLSD& replyPump1NamePath=LLSD())
    {
        return postAndWait2(event, requestPump, mPump0, mPump1,
                            replyPump0NamePath, replyPump1NamePath);
    }

    LLSD postAndWaitWithException(const LLSD& event,
                                  const LLEventPumpOrPumpName& requestPump,
                                  const LLSD& replyPump0NamePath=LLSD(),
                                  const LLSD& replyPump1NamePath=LLSD())
    {
        return errorException(postAndWait(event, requestPump,
                                          replyPump0NamePath, replyPump1NamePath),
                              std::string("Error event on ") + getName1());
    }

    LLSD postAndWaitWithLog(const LLSD& event,
                            const LLEventPumpOrPumpName& requestPump,
                            const LLSD& replyPump0NamePath=LLSD(),
                            const LLSD& replyPump1NamePath=LLSD())
    {
        return errorLog(postAndWait(event, requestPump,
                                    replyPump0NamePath, replyPump1NamePath),
                        std::string("Error event on ") + getName1());
    }

private:
    LLEventStream mPump0, mPump1;
};

#endif /* ! defined(LL_LLEVENTCORO_H) */
