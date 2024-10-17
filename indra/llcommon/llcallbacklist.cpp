/**
 * @file llcallbacklist.cpp
 * @brief A simple list of callback functions to call.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "lazyeventapi.h"
#include "llcallbacklist.h"
#include "llerror.h"
#include "llexception.h"
#include "llsdutil.h"
#include "tempset.h"
#include <boost/container_hash/hash.hpp>
#include <iomanip>
#include <vector>

//
// Member functions
//

/*****************************************************************************
*   LLCallbackList
*****************************************************************************/
LLCallbackList::LLCallbackList()
{
    // nothing
}

LLCallbackList::~LLCallbackList()
{
}

LLCallbackList::handle_t LLCallbackList::addFunction( callback_t func, void *data)
{
    if (!func)
    {
        return {};
    }

    // only add one callback per func/data pair
    //
    if (containsFunction(func, data))
    {
        return {};
    }

    auto handle = addFunction([func, data]{ func(data); });
    mLookup.emplace(callback_pair_t(func, data), handle);
    return handle;
}

LLCallbackList::handle_t LLCallbackList::addFunction( const callable_t& func )
{
    return mCallbackList.connect(func);
}

bool LLCallbackList::containsFunction( callback_t func, void *data)
{
    return mLookup.find(callback_pair_t(func, data)) != mLookup.end();
}

bool LLCallbackList::deleteFunction( callback_t func, void *data)
{
    auto found = mLookup.find(callback_pair_t(func, data));
    if (found != mLookup.end())
    {
        deleteFunction(found->second);
        mLookup.erase(found);
        return true;
    }
    else
    {
        return false;
    }
}

void LLCallbackList::deleteFunction( const handle_t& handle )
{
    handle.disconnect();
}

void LLCallbackList::deleteAllFunctions()
{
    mCallbackList = {};
    mLookup.clear();
}

void LLCallbackList::callFunctions()
{
    mCallbackList();
}

LLCallbackList::handle_t LLCallbackList::doOnIdleOneTime( const callable_t& func )
{
    // connect_extended() passes the connection to the callback
    return mCallbackList.connect_extended(
        [func](const handle_t& handle)
        {
            handle.disconnect();
            func();
        });
}

LLCallbackList::handle_t LLCallbackList::doOnIdleRepeating( const bool_func_t& func )
{
    return mCallbackList.connect_extended(
        [func](const handle_t& handle)
        {
            if (func())
            {
                handle.disconnect();
            }
        });
}

/*****************************************************************************
*   LL::Timers
*****************************************************************************/
namespace LL
{

Timers::Timers() {}

// Call a given callable once at specified timestamp.
Timers::handle_t Timers::scheduleAt(nullary_func_t callable, LLDate::timestamp time)
{
    // tick() assumes you want to run periodically until you return true.
    // Schedule a task that returns true after a single call.
    return scheduleAtEvery(once(callable), time, 0);
}

// Call a given callable once after specified interval.
Timers::handle_t Timers::scheduleAfter(nullary_func_t callable, F32 seconds)
{
    return scheduleEvery(once(callable), seconds);
}

// Call a given callable every specified number of seconds, until it returns true.
Timers::handle_t Timers::scheduleEvery(bool_func_t callable, F32 seconds)
{
    return scheduleAtEvery(callable, now() + seconds, seconds);
}

Timers::handle_t Timers::scheduleAtEvery(bool_func_t callable,
                                             LLDate::timestamp time, F32 interval)
{
    // Pick token FIRST to store a self-reference in mQueue's managed node as
    // well as in mMeta. Pre-increment to distinguish 0 from any live
    // handle_t.
    token_t token{ ++mToken };
    // For the moment, store a default-constructed mQueue handle --
    // we'll fill in later.
    auto [iter, inserted] = mMeta.emplace(token,
                                          Metadata{ queue_t::handle_type(), time, interval });
    // It's important that our token is unique.
    llassert(inserted);

    // Remember whether this is the first entry in mQueue
    bool first{ mQueue.empty() };
    auto handle{ mQueue.emplace(callable, token, time) };
    // Now that we have an mQueue handle_type, store it in mMeta entry.
    iter->second.mHandle = handle;
    if (first && ! mLive.connected())
    {
        // If this is our first entry, register for regular callbacks.
        mLive = LLCallbackList::instance().doOnIdleRepeating([this]{ return tick(); });
    }
    // Make an Timers::handle_t from token.
    return { token };
}

bool Timers::isRunning(handle_t timer) const
{
    // A default-constructed timer isn't running.
    // A timer we don't find in mMeta has fired or been canceled.
    return timer && mMeta.find(timer.token) != mMeta.end();
}

F32 Timers::timeUntilCall(handle_t timer) const
{
    MetaMap::const_iterator found;
    if ((! timer) || (found = mMeta.find(timer.token)) == mMeta.end())
    {
        return 0.f;
    }
    else
    {
        return narrow(found->second.mTime - now());
    }
}

// Cancel a future timer set by scheduleAt(), scheduleAfter(), scheduleEvery()
bool Timers::cancel(handle_t& timer)
{
    // For exception safety, capture and clear timer before canceling.
    // Once we've canceled this handle, don't retain the live handle.
    const handle_t ctimer{ timer };
    timer = handle_t();
    return cancel(ctimer);
}

bool Timers::cancel(const handle_t& timer)
{
    if (! timer)
    {
        return false;
    }

    // fibonacci_heap documentation does not address the question of what
    // happens if you call erase() twice with the same handle. Is it a no-op?
    // Does it invalidate the heap? Is it UB?

    // Nor do we find any documented way to ask whether a given handle still
    // tracks a valid heap node. That's why we capture all returned handles in
    // mMeta and validate against that collection. What about the pop()
    // call in tick()? How to map from the top() value back to the
    // corresponding handle_t? That's why we store func_at::mToken.

    // fibonacci_heap provides a pair of begin()/end() methods to iterate over
    // all nodes (NOT in heap order), plus a function to convert from such
    // iterators to handles. Without mMeta, that would be our only chance
    // to validate.
    auto found{ mMeta.find(timer.token) };
    if (found == mMeta.end())
    {
        // we don't recognize this handle -- maybe the timer has already
        // fired, maybe it was previously canceled.
        return false;
    }

    // Funny case: what if the callback directly or indirectly reaches a
    // cancel() call for its own handle?
    if (found->second.mRunning)
    {
        // tick() has special logic to defer the actual deletion until the
        // callback has returned
        found->second.mCancel = true;
        // this handle does in fact reference a live timer,
        // which we're going to cancel when we get a chance
        return true;
    }

    // Erase from mQueue the handle_type referenced by timer.token.
    mQueue.erase(found->second.mHandle);
    // before erasing the mMeta entry
    mMeta.erase(found);
    if (mQueue.empty())
    {
        // If that was the last active timer, unregister for callbacks.
        //LLCallbackList::instance().deleteFunction(mLive);
        // Since we're in the source file that knows the true identity of an
        // LLCallbackList::handle_t, we don't even need to call instance().
        mLive.disconnect();
    }
    return true;
}

void Timers::setTimeslice(F32 timeslice)
{
    if (timeslice < MINIMUM_TIMESLICE)
    {
        // use stringize() so setprecision() affects only the temporary
        // ostream, not the common logging ostream
        LL_WARNS("Timers") << "LL::Timers::setTimeslice("
                           << stringize(std::setprecision(4), timeslice)
                           << ") less than "
                           << stringize(std::setprecision(4), MINIMUM_TIMESLICE)
                           << ", ignoring" << LL_ENDL;
    }
    else
    {
        mTimeslice = timeslice;
    }
}

bool Timers::tick()
{
    // Fetch current time only on entry, even though running some mQueue task
    // may take long enough that the next one after would become ready. We're
    // sharing this thread with everything else, and there's a risk we might
    // starve it if we have a sequence of tasks that take nontrivial time.
    auto now{ LLDate::now().secondsSinceEpoch() };
    auto cutoff{ now + mTimeslice };

    // Capture tasks we've processed but that want to be rescheduled.
    // Defer rescheduling them immediately to avoid getting stuck looping over
    // a recurring task with a nonpositive interval.
    std::vector<std::pair<MetaMap::iterator, func_at>> deferred;

    while (! mQueue.empty())
    {
        auto& top{ mQueue.top() };
        if (top.mTime > now)
        {
            // we've hit an entry that's still in the future:
            // done with this tick()
            break;
        }
        if (LLDate::now().secondsSinceEpoch() > cutoff)
        {
            // we still have ready tasks, but we've already eaten too much
            // time this tick() -- defer until next tick()
            break;
        }

        // Found a ready task. Look up its corresponding mMeta entry.
        auto meta{ mMeta.find(top.mToken) };
        llassert(meta != mMeta.end());
        bool done;
        {
            // Mark our mMeta entry so we don't cancel this timer while its
            // callback is running, but unmark it even in case of exception.
            TempSet running(meta->second.mRunning, true);
            // run the callback and capture its desire to end repetition
            try
            {
                done = top.mFunc();
            }
            catch (...)
            {
                // Don't crash if a timer callable throws.
                // But don't continue calling that callable, either.
                done = true;
                LOG_UNHANDLED_EXCEPTION("LL::Timers");
            }
        } // clear mRunning

        // If mFunc() returned true (all done, stop calling me) or
        // meta->mCancel (somebody tried to cancel this timer during the
        // callback call), then we're done: clean up both entries.
        if (done || meta->second.mCancel)
        {
            // remove the mMeta entry referencing this task
            mMeta.erase(meta);
        }
        else
        {
            // mFunc returned false, and nobody asked to cancel:
            // continue calling this task at a future time.
            meta->second.mTime += meta->second.mInterval;
            // capture this task to reschedule once we break loop
            deferred.push_back({meta, top});
            // update func_at's mTime to match meta's
            deferred.back().second.mTime = meta->second.mTime;
        }
        // Remove the mQueue entry regardless, or we risk stalling the
        // queue right here if we have a nonpositive interval.
        mQueue.pop();
    }

    // Now reschedule any tasks that need to be rescheduled.
    for (const auto& [meta, task] : deferred)
    {
        auto handle{ mQueue.push(task) };
        // track this new mQueue handle_type
        meta->second.mHandle = handle;
    }

    // If, after all the twiddling above, our queue ended up empty,
    // stop calling every tick.
    return mQueue.empty();
}

/*****************************************************************************
*   TimersListener
*****************************************************************************/

class TimersListener: public LLEventAPI
{
public:
    TimersListener(const LazyEventAPIParams& params): LLEventAPI(params) {}

    // Forbid a script from requesting callbacks too quickly.
    static constexpr LLSD::Real MINTIMER{ 0.010 };

    void scheduleAfter(const LLSD& params);
    void scheduleEvery(const LLSD& params);
    LLSD cancel(const LLSD& params);
    LLSD isRunning(const LLSD& params);
    LLSD timeUntilCall(const LLSD& params);

private:
    // We use the incoming reqid to distinguish different timers -- but reqid
    // by itself is not unique! Each reqid is local to a calling script.
    // Distinguish scripts by reply-pump name, then reqid within script.
    // "Additional specializations for std::pair and the standard container
    // types, as well as utility functions to compose hashes are available in
    // boost::hash."
    // https://en.cppreference.com/w/cpp/utility/hash
    using HandleKey = std::pair<LLSD::String, LLSD::Integer>;
    using HandleMap = std::unordered_map<HandleKey, Timers::temp_handle_t,
                                         boost::hash<HandleKey>>;
    HandleMap mHandles;
};

void TimersListener::scheduleAfter(const LLSD& params)
{
    // Timer creation functions respond immediately with the reqid of the
    // created timer, as well as later when the timer fires. That lets the
    // requester invoke cancel, isRunning or timeUntilCall.
    Response response(LLSD(), params);
    LLSD::Real after{ params["after"] };
    if (after < MINTIMER)
    {
        return response.error(stringize("after must be at least ", MINTIMER));
    }

    HandleKey key{ params["reply"], params["reqid"] };
    mHandles.emplace(
        key,
        Timers::instance().scheduleAfter(
            [this, params, key]
            {
                // we don't need any content save for the "reqid"
                sendReply({}, params);
                // ditch mHandles entry
                mHandles.erase(key);
            },
            narrow(after)));
}

void TimersListener::scheduleEvery(const LLSD& params)
{
    // Timer creation functions respond immediately with the reqid of the
    // created timer, as well as later when the timer fires. That lets the
    // requester invoke cancel, isRunning or timeUntilCall.
    Response response(LLSD(), params);
    LLSD::Real every{ params["every"] };
    if (every < MINTIMER)
    {
        return response.error(stringize("every must be at least ", MINTIMER));
    }

    mHandles.emplace(
        HandleKey{ params["reply"], params["reqid"] },
        Timers::instance().scheduleEvery(
            [params, i=0]() mutable
            {
                // we don't need any content save for the "reqid"
                sendReply(llsd::map("i", i++), params);
                // we can't use a handshake -- always keep the ball rolling
                return false;
            },
            narrow(every)));
}

LLSD TimersListener::cancel(const LLSD& params)
{
    auto found{ mHandles.find({params["reply"], params["id"]}) };
    bool ok = false;
    if (found != mHandles.end())
    {
        ok = true;
        Timers::instance().cancel(found->second);
        mHandles.erase(found);
    }
    return llsd::map("ok", ok);
}

LLSD TimersListener::isRunning(const LLSD& params)
{
    auto found{ mHandles.find({params["reply"], params["id"]}) };
    bool running = false;
    if (found != mHandles.end())
    {
        running = Timers::instance().isRunning(found->second);
    }
    return llsd::map("running", running);
}

LLSD TimersListener::timeUntilCall(const LLSD& params)
{
    auto found{ mHandles.find({params["reply"], params["id"]}) };
    bool ok = false;
    LLSD::Real remaining = 0;
    if (found != mHandles.end())
    {
        ok = true;
        remaining = Timers::instance().timeUntilCall(found->second);
    }
    return llsd::map("ok", ok, "remaining", remaining);
}

class TimersRegistrar: public LazyEventAPI<TimersListener>
{
    using super = LazyEventAPI<TimersListener>;
    using super::listener;

public:
    TimersRegistrar():
        super("Timers", "Provide access to viewer timer functionality.")
    {
        add("scheduleAfter",
R"-(Create a timer with ID "reqid". Post response after "after" seconds.)-",
            &listener::scheduleAfter,
            llsd::map("reqid", LLSD::Integer(), "after", LLSD::Real()));
        add("scheduleEvery",
R"-(Create a timer with ID "reqid". Post response every "every" seconds
until cancel().)-",
            &listener::scheduleEvery,
            llsd::map("reqid", LLSD::Integer(), "every", LLSD::Real()));
        add("cancel",
R"-(Cancel the timer with ID "id". Respond "ok"=true if "id" identifies
a live timer.)-",
            &listener::cancel,
            llsd::map("reqid", LLSD::Integer(), "id", LLSD::Integer()));
        add("isRunning",
R"-(Query the timer with ID "id": respond "running"=true if "id" identifies
a live timer.)-",
            &listener::isRunning,
            llsd::map("reqid", LLSD::Integer(), "id", LLSD::Integer()));
        add("timeUntilCall",
R"-(Query the timer with ID "id": if "id" identifies a live timer, respond
"ok"=true, "remaining"=seconds with the time left before timer expiry;
otherwise "ok"=false, "remaining"=0.)-",
            &listener::timeUntilCall,
            llsd::map("reqid", LLSD::Integer()));
    }
};
static TimersRegistrar registrar;

} // namespace LL
