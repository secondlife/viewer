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
#include <chrono>
#include <exception>
// std headers
// external library headers
#include <boost/fiber/operations.hpp>
// other Linden headers
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llerror.h"
#include "llcoros.h"
#include "stringize.h"

namespace
{

/**
 * suspendUntilEventOn() permits a coroutine to temporarily listen on an
 * LLEventPump any number of times. We don't really want to have to ask
 * the caller to label each such call with a distinct string; the whole
 * point of suspendUntilEventOn() is to present a nice sequential interface to
 * the underlying LLEventPump-with-named-listeners machinery. So we'll use
 * LLEventPump::inventName() to generate a distinct name for each
 * temporary listener. On the other hand, because a given coroutine might
 * call suspendUntilEventOn() any number of times, we don't really want to
 * consume an arbitrary number of generated inventName()s: that namespace,
 * though large, is nonetheless finite. So we memoize an invented name for
 * each distinct coroutine instance.
 */
std::string listenerNameForCoro()
{
    // If this coroutine was launched by LLCoros::launch(), find that name.
    std::string name(LLCoros::getName());
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
 * Implement behavior described for postAndSuspend()'s @a replyPumpNamePath
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
void storeToLLSDPath(LLSD& dest, const LLSD& path, const LLSD& value)
{
    if (path.isUndefined())
    {
        // no-op case
        return;
    }

    // Drill down to where we should store 'value'.
    llsd::drill(dest, path) = value;
}

} // anonymous

void llcoro::suspend()
{
    LLCoros::checkStop();
    LLCoros::TempStatus st("waiting one tick");
    boost::this_fiber::yield();
}

void llcoro::suspendUntilTimeout(float seconds)
{
    LLCoros::checkStop();
    // The fact that we accept non-integer seconds means we should probably
    // use granularity finer than one second. However, given the overhead of
    // the rest of our processing, it seems silly to use granularity finer
    // than a millisecond.
    LLCoros::TempStatus st(STRINGIZE("waiting for " << seconds << "s"));
    boost::this_fiber::sleep_for(std::chrono::milliseconds(long(seconds * 1000)));
}

namespace
{

// returns a listener on replyPumpP, also on "mainloop" -- both should be
// stored in LLTempBoundListeners on the caller's stack frame
std::pair<LLBoundListener, LLBoundListener>
postAndSuspendSetup(const std::string& callerName,
                    const std::string& listenerName,
                    LLCoros::Promise<LLSD>& promise,
                    const LLSD& event,
                    const LLEventPumpOrPumpName& requestPumpP,
                    const LLEventPumpOrPumpName& replyPumpP,
                    const LLSD& replyPumpNamePath)
{
    // Before we get any farther -- should we be stopping instead of
    // suspending?
    LLCoros::checkStop();
    // Get the consuming attribute for THIS coroutine, the one that's about to
    // suspend. Don't call get_consuming() in the lambda body: that would
    // return the consuming attribute for some other coroutine, most likely
    // the main routine.
    bool consuming(LLCoros::get_consuming());
    // listen on the specified LLEventPump with a lambda that will assign a
    // value to the promise, thus fulfilling its future
    llassert_always_msg(replyPumpP, ("replyPump required for " + callerName));
    LLEventPump& replyPump(replyPumpP.getPump());
    // The relative order of the two listen() calls below would only matter if
    // "LLApp" were an LLEventMailDrop. But if we ever go there, we'd want to
    // notice the pending LLApp status first.
    LLBoundListener stopper(
        LLEventPumps::instance().obtain("LLApp").listen(
            listenerName,
            [&promise, listenerName](const LLSD& status)
            {
                // anything except "running" should wake up the waiting
                // coroutine
                auto& statsd = status["status"];
                if (statsd.asString() != "running")
                {
                    LL_DEBUGS("lleventcoro") << listenerName
                                             << " spotted status " << statsd
                                             << ", throwing Stopping" << LL_ENDL;
                    try
                    {
                        promise.set_exception(
                            std::make_exception_ptr(
                                LLCoros::Stopping("status " + statsd.asString())));
                    }
                    catch (const boost::fibers::promise_already_satisfied&)
                    {
                        LL_WARNS("lleventcoro") << listenerName
                                                << " couldn't throw Stopping "
                                                   "because promise already set" << LL_ENDL;
                    }
                }
                // do not consume -- every listener must see status
                return false;
            }));
    LLBoundListener connection(
        replyPump.listen(
            listenerName,
            [&promise, consuming, listenerName](const LLSD& result)
            {
                try
                {
                    promise.set_value(result);
                    // We did manage to propagate the result value to the
                    // (real) listener. If we're supposed to indicate that
                    // we've consumed it, do so.
                    return consuming;
                }
                catch(boost::fibers::promise_already_satisfied & ex)
                {
                    LL_DEBUGS("lleventcoro") << "promise already satisfied in '"
                        << listenerName << "': "  << ex.what() << LL_ENDL;
                    // We could not propagate the result value to the
                    // listener.
                    return false;
                }
            }));

    // skip the "post" part if requestPump is default-constructed
    if (requestPumpP)
    {
        LLEventPump& requestPump(requestPumpP.getPump());
        // If replyPumpNamePath is non-empty, store the replyPump name in the
        // request event.
        LLSD modevent(event);
        storeToLLSDPath(modevent, replyPumpNamePath, replyPump.getName());
        LL_DEBUGS("lleventcoro") << callerName << ": coroutine " << listenerName
                                 << " posting to " << requestPump.getName()
                                 << LL_ENDL;

        // *NOTE:Mani - Removed because modevent could contain user's hashed passwd.
        //                         << ": " << modevent << LL_ENDL;
        requestPump.post(modevent);
    }
    LL_DEBUGS("lleventcoro") << callerName << ": coroutine " << listenerName
                             << " about to wait on LLEventPump " << replyPump.getName()
                             << LL_ENDL;
    return { connection, stopper };
}

} // anonymous

LLSD llcoro::postAndSuspend(const LLSD& event, const LLEventPumpOrPumpName& requestPump,
                 const LLEventPumpOrPumpName& replyPump, const LLSD& replyPumpNamePath)
{
    LLCoros::Promise<LLSD> promise;
    std::string listenerName(listenerNameForCoro());

    // Store both connections into LLTempBoundListeners so we implicitly
    // disconnect on return from this function.
    auto connections =
        postAndSuspendSetup("postAndSuspend()", listenerName, promise,
                            event, requestPump, replyPump, replyPumpNamePath);
    LLTempBoundListener connection(connections.first), stopper(connections.second);

    // declare the future
    LLCoros::Future<LLSD> future = LLCoros::getFuture(promise);
    // calling get() on the future makes us wait for it
    LLCoros::TempStatus st(STRINGIZE("waiting for " << replyPump.getPump().getName()));
    LLSD value(future.get());
    LL_DEBUGS("lleventcoro") << "postAndSuspend(): coroutine " << listenerName
                             << " resuming with " << value << LL_ENDL;
    // returning should disconnect the connection
    return value;
}

LLSD llcoro::postAndSuspendWithTimeout(const LLSD& event,
                                       const LLEventPumpOrPumpName& requestPump,
                                       const LLEventPumpOrPumpName& replyPump,
                                       const LLSD& replyPumpNamePath,
                                       F32 timeout, const LLSD& timeoutResult)
{
    LLCoros::Promise<LLSD> promise;
    std::string listenerName(listenerNameForCoro());

    // Store both connections into LLTempBoundListeners so we implicitly
    // disconnect on return from this function.
    auto connections =
        postAndSuspendSetup("postAndSuspendWithTimeout()", listenerName, promise,
                            event, requestPump, replyPump, replyPumpNamePath);
    LLTempBoundListener connection(connections.first), stopper(connections.second);

    // declare the future
    LLCoros::Future<LLSD> future = LLCoros::getFuture(promise);
    // wait for specified timeout
    boost::fibers::future_status status;
    {
        LLCoros::TempStatus st(STRINGIZE("waiting for " << replyPump.getPump().getName()
                                         << " for " << timeout << "s"));
        status = future.wait_for(std::chrono::milliseconds(long(timeout * 1000)));
    }
    // if the future is NOT yet ready, return timeoutResult instead
    if (status == boost::fibers::future_status::timeout)
    {
        LL_DEBUGS("lleventcoro") << "postAndSuspendWithTimeout(): coroutine " << listenerName
                                 << " timed out after " << timeout << " seconds,"
                                 << " resuming with " << timeoutResult << LL_ENDL;
        return timeoutResult;
    }
    else
    {
        llassert_always(status == boost::fibers::future_status::ready);

        // future is now ready, no more waiting
        LLSD value(future.get());
        LL_DEBUGS("lleventcoro") << "postAndSuspendWithTimeout(): coroutine " << listenerName
                                 << " resuming with " << value << LL_ENDL;
        // returning should disconnect the connection
        return value;
    }
}
