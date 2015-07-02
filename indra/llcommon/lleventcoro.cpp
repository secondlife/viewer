/**
 * @file   lleventcoro.cpp
 * @author Nat Goodspeed
 * @date   2009-04-29
 * @brief  Implementation for lleventcoro.
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

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventcoro.h"
// STL headers
#include <map>
// std headers
// external library headers
#include <boost/dcoroutine/coroutine.hpp>
#include <boost/dcoroutine/future.hpp>
// other Linden headers
#include "llsdserialize.h"
#include "llerror.h"
#include "llcoros.h"

namespace
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
std::string listenerNameForCoro()
{
    // If this coroutine was launched by LLCoros::launch(), find that name.
    std::string name(LLCoros::instance().getName());
    if (! name.empty())
    {
        return name;
    }
    // this is the first time we've been called for this coroutine instance
    name = LLEventPump::inventName("coro");
    LL_INFOS("LLEventCoro") << "listenerNameForCoro(): inventing coro name '"
                            << name << "'" << LL_ENDL;
    return name;
}

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
void storeToLLSDPath(LLSD& dest, const LLSD& rawPath, const LLSD& value)
{
    if (rawPath.isUndefined())
    {
        // no-op case
        return;
    }

    // Arrange to treat rawPath uniformly as an array. If it's not already an
    // array, store it as the only entry in one.
    LLSD path;
    if (rawPath.isArray())
    {
        path = rawPath;
    }
    else
    {
        path.append(rawPath);
    }

    // Need to indicate a current destination -- but that current destination
    // needs to change as we step through the path array. Where normally we'd
    // use an LLSD& to capture a subscripted LLSD lvalue, this time we must
    // instead use a pointer -- since it must be reassigned.
    LLSD* pdest = &dest;

    // Now loop through that array
    for (LLSD::Integer i = 0; i < path.size(); ++i)
    {
        if (path[i].isString())
        {
            // *pdest is an LLSD map
            pdest = &((*pdest)[path[i].asString()]);
        }
        else if (path[i].isInteger())
        {
            // *pdest is an LLSD array
            pdest = &((*pdest)[path[i].asInteger()]);
        }
        else
        {
            // What do we do with Real or Array or Map or ...?
            // As it's a coder error -- not a user error -- rub the coder's
            // face in it so it gets fixed.
            LL_ERRS("lleventcoro") << "storeToLLSDPath(" << dest << ", " << rawPath << ", " << value
                                   << "): path[" << i << "] bad type " << path[i].type() << LL_ENDL;
        }
    }

    // Here *pdest is where we should store value.
    *pdest = value;
}

} // anonymous

LLSD llcoro::postAndWait(const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                         const LLEventPumpOrPumpName& replyPump, const LLSD& replyPumpNamePath)
{
    // declare the future
    boost::dcoroutines::future<LLSD> future(llcoro::get_self());
    // make a callback that will assign a value to the future, and listen on
    // the specified LLEventPump with that callback
    std::string listenerName(listenerNameForCoro());
    LLTempBoundListener connection(
        replyPump.getPump().listen(listenerName,
                                   voidlistener(boost::dcoroutines::make_callback(future))));
    // skip the "post" part if requestPump is default-constructed
    if (requestPump)
    {
        // If replyPumpNamePath is non-empty, store the replyPump name in the
        // request event.
        LLSD modevent(event);
        storeToLLSDPath(modevent, replyPumpNamePath, replyPump.getPump().getName());
        LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << listenerName
                                 << " posting to " << requestPump.getPump().getName()
                                 << LL_ENDL;

        // *NOTE:Mani - Removed because modevent could contain user's hashed passwd.
        //                         << ": " << modevent << LL_ENDL;
        requestPump.getPump().post(modevent);
    }
    LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << listenerName
                             << " about to wait on LLEventPump " << replyPump.getPump().getName()
                             << LL_ENDL;
    // trying to dereference ("resolve") the future makes us wait for it
    LLSD value;
    {
        // instantiate Suspending to manage the "current" coroutine
        llcoro::Suspending suspended;
        value = *future;
    } // destroy Suspending as soon as we're back
    LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << listenerName
                             << " resuming with " << value << LL_ENDL;
    // returning should disconnect the connection
    return value;
}

namespace
{

/**
 * This helper is specifically for the two-pump version of waitForEventOn().
 * We use a single future object, but we want to listen on two pumps with it.
 * Since we must still adapt from (the callable constructed by)
 * boost::dcoroutines::make_callback() (void return) to provide an event
 * listener (bool return), we've adapted VoidListener for the purpose. The
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

} // anonymous

namespace llcoro
{

LLEventWithID postAndWait2(const LLSD& event,
                           const LLEventPumpOrPumpName& requestPump,
                           const LLEventPumpOrPumpName& replyPump0,
                           const LLEventPumpOrPumpName& replyPump1,
                           const LLSD& replyPump0NamePath,
                           const LLSD& replyPump1NamePath)
{
    // declare the future
    boost::dcoroutines::future<LLEventWithID> future(llcoro::get_self());
    // either callback will assign a value to this future; listen on
    // each specified LLEventPump with a callback
    std::string name(listenerNameForCoro());
    LLTempBoundListener connection0(
        replyPump0.getPump().listen(name + "a",
                                    wfeoh(boost::dcoroutines::make_callback(future), 0)));
    LLTempBoundListener connection1(
        replyPump1.getPump().listen(name + "b",
                                    wfeoh(boost::dcoroutines::make_callback(future), 1)));
    // skip the "post" part if requestPump is default-constructed
    if (requestPump)
    {
        // If either replyPumpNamePath is non-empty, store the corresponding
        // replyPump name in the request event.
        LLSD modevent(event);
        storeToLLSDPath(modevent, replyPump0NamePath,
                        replyPump0.getPump().getName());
        storeToLLSDPath(modevent, replyPump1NamePath,
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
    LLEventWithID value;
    {
        // instantiate Suspending to manage "current" coroutine
        llcoro::Suspending suspended;
        value = *future;
    } // destroy Suspending as soon as we're back
    LL_DEBUGS("lleventcoro") << "postAndWait(): coroutine " << name
                             << " resuming with (" << value.first << ", " << value.second << ")"
                             << LL_ENDL;
    // returning should disconnect both connections
    return value;
}

LLSD errorException(const LLEventWithID& result, const std::string& desc)
{
    // If the result arrived on the error pump (pump 1), instead of
    // returning it, deliver it via exception.
    if (result.second)
    {
        throw LLErrorEvent(desc, result.first);
    }
    // That way, our caller knows a simple return must be from the reply
    // pump (pump 0).
    return result.first;
}

LLSD errorLog(const LLEventWithID& result, const std::string& desc)
{
    // If the result arrived on the error pump (pump 1), log it as a fatal
    // error.
    if (result.second)
    {
        LL_ERRS("errorLog") << desc << ":" << std::endl;
        LLSDSerialize::toPrettyXML(result.first, LL_CONT);
        LL_CONT << LL_ENDL;
    }
    // A simple return must therefore be from the reply pump (pump 0).
    return result.first;
}

} // namespace llcoro
