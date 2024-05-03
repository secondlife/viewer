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

#include "llcallbacklist.h"

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
		mLookup.erase(found);
		deleteFunction(found->second);
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
*   LLLater
*****************************************************************************/
LLLater::LLLater() {}

// Call a given callable once at specified timestamp.
LLLater::handle_t LLLater::doAtTime(nullary_func_t callable, LLDate::timestamp time)
{
    bool first{ mQueue.empty() };
    // Pick token FIRST to store a self-reference in mQueue's managed node as
    // well as in mHandles. Pre-increment to distinguish 0 from any live
    // handle_t.
    token_t token{ ++mToken };
    auto handle{ mQueue.emplace(callable, time, token) };
    mHandles.emplace(token, handle);
    if (first)
    {
        // If this is our first entry, register for regular callbacks.
        mLive = LLCallbackList::instance().doOnIdleRepeating([this]{ return tick(); });
    }
    return handle_t{ token };
}

// Call a given callable once after specified interval.
LLLater::handle_t LLLater::doAfterInterval(nullary_func_t callable, F32 seconds)
{
    // Passing 0 is a slightly more expensive way of calling
    // LLCallbackList::doOnIdleOneTime(). Are we sure the caller is correct?
    // (If there's a valid use case, remove the llassert() and carry on.)
    llassert(seconds > 0);
    return doAtTime(callable, LLDate::now().secondsSinceEpoch() + seconds);
}

// For doPeriodically(), we need a struct rather than a lambda because a
// struct, unlike a lambda, has access to 'this'.
struct Periodic
{
    LLLater* mLater;
    bool_func_t mCallable;
    LLDate::timestamp mNext;
    F32 mSeconds;

    void operator()()
    {
        if (! mCallable())
        {
            // Returning false means please schedule another call.
            // Don't call doAfterInterval(), which rereads LLDate::now(),
            // since that would defer by however long it took us to wake
            // up and notice plus however long callable() took to run.
            mNext += mSeconds;
            mLater->doAtTime(*this, mNext);
        }
    }
};

// Call a given callable every specified number of seconds, until it returns true.
LLLater::handle_t LLLater::doPeriodically(bool_func_t callable, F32 seconds)
{
    // Passing seconds <= 0 will produce an infinite loop.
    llassert(seconds > 0);
    auto next{ LLDate::now().secondsSinceEpoch() + seconds };
    return doAtTime(Periodic{ this, callable, next, seconds }, next);
}

bool LLLater::isRunning(handle_t timer)
{
    // A default-constructed timer isn't running.
    // A timer we don't find in mHandles has fired or been canceled.
    return timer && mHandles.find(timer.token) != mHandles.end();
}

// Cancel a future timer set by doAtTime(), doAfterInterval(), doPeriodically()
bool LLLater::cancel(handle_t& timer)
{
    // For exception safety, capture and clear timer before canceling.
    // Once we've canceled this handle, don't retain the live handle.
    const handle_t ctimer{ timer };
    timer = handle_t();
    return cancel(ctimer);
}

bool LLLater::cancel(const handle_t& timer)
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
    // mHandles and validate against that collection. What about the pop()
    // call in tick()? How to map from the top() value back to the
    // corresponding handle_t? That's why we store func_at::mToken.

    // fibonacci_heap provides a pair of begin()/end() methods to iterate over
    // all nodes (NOT in heap order), plus a function to convert from such
    // iterators to handles. Without mHandles, that would be our only chance
    // to validate.
    auto found{ mHandles.find(timer.token) };
    if (found == mHandles.end())
    {
        // we don't recognize this handle -- maybe the timer has already
        // fired, maybe it was previously canceled.
        return false;
    }

    // erase from mQueue the handle_t referenced by timer.token
    mQueue.erase(found->second);
    // before erasing timer.token from mHandles
    mHandles.erase(found);
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

bool LLLater::tick()
{
    // Fetch current time only on entry, even though running some mQueue task
    // may take long enough that the next one after would become ready. We're
    // sharing this thread with everything else, and there's a risk we might
    // starve it if we have a sequence of tasks that take nontrivial time.
    auto now{ LLDate::now().secondsSinceEpoch() };
    auto cutoff{ now + TIMESLICE };
    while (! mQueue.empty())
    {
        auto& top{ mQueue.top() };
        if (top.mTime > now)
        {
            // we've hit an entry that's still in the future:
            // done with this tick(), but schedule another call
            return false;
        }
        if (LLDate::now().secondsSinceEpoch() > cutoff)
        {
            // we still have ready tasks, but we've already eaten too much
            // time this tick() -- defer until next tick() -- call again
            return false;
        }

        // Found a ready task. Hate to copy stuff, but -- what if the task
        // indirectly ends up trying to cancel a handle referencing its own
        // node in mQueue? If the task has any state, that would be Bad. Copy
        // the node before running it.
        auto current{ top };
        // remove the mHandles entry referencing this task
        mHandles.erase(current.mToken);
        // before removing the mQueue task entry itself
        mQueue.pop();
        // okay, NOW run 
        current.mFunc();
    }
    // queue is empty: stop callbacks
    return true;
}
