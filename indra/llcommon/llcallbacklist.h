/**
 * @file llcallbacklist.h
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

#ifndef LL_LLCALLBACKLIST_H
#define LL_LLCALLBACKLIST_H

#include "lldate.h"
#include "llsingleton.h"
#include "llstl.h"
#include <boost/container_hash/hash.hpp>
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <unordered_map>

/*****************************************************************************
*   LLCallbackList: callbacks every idle tick (every callFunctions() call)
*****************************************************************************/
class LLCallbackList: public LLSingleton<LLCallbackList>
{
    LLSINGLETON(LLCallbackList);
public:
    typedef void (*callback_t)(void*);

    typedef boost::signals2::signal<void()> callback_list_t;
    typedef callback_list_t::slot_type callable_t;
    typedef boost::signals2::connection handle_t;
    typedef boost::signals2::scoped_connection temp_handle_t;
    typedef std::function<bool ()> bool_func_t;

    ~LLCallbackList();

    handle_t addFunction( callback_t func, void *data = NULL );     // register a callback, which will be called as func(data)
    handle_t addFunction( const callable_t& func );
    bool containsFunction( callback_t func, void *data = NULL );    // true if list already contains the function/data pair
    bool deleteFunction( callback_t func, void *data = NULL );      // removes the first instance of this function/data pair from the list, false if not found
    void deleteFunction( const handle_t& handle );
    void callFunctions();                                           // calls all functions
    void deleteAllFunctions();

    handle_t doOnIdleOneTime( const callable_t& func );
    handle_t doOnIdleRepeating( const bool_func_t& func );
    bool isRunning(const handle_t& handle) const { return handle.connected(); };

    static void test();

protected:
    callback_list_t mCallbackList;

    // "Additional specializations for std::pair and the standard container
    // types, as well as utility functions to compose hashes are available in
    // boost::hash."
    // https://en.cppreference.com/w/cpp/utility/hash
    typedef std::pair< callback_t,void* >   callback_pair_t;
    typedef std::unordered_map<callback_pair_t, handle_t,
        boost::hash<callback_pair_t>> lookup_table;
    lookup_table mLookup;
};

/*-------------------- legacy names in global namespace --------------------*/
#define gIdleCallbacks (LLCallbackList::instance())

using nullary_func_t = LLCallbackList::callable_t;
using bool_func_t    = LLCallbackList::bool_func_t;

// Call a given callable once in idle loop.
inline
LLCallbackList::handle_t doOnIdleOneTime(nullary_func_t callable)
{
    return gIdleCallbacks.doOnIdleOneTime(callable);
}

// Repeatedly call a callable in idle loop until it returns true.
inline
LLCallbackList::handle_t doOnIdleRepeating(bool_func_t callable)
{
    return gIdleCallbacks.doOnIdleRepeating(callable);
}

/*****************************************************************************
*   LL::Timers: callbacks at some future time
*****************************************************************************/
namespace LL
{

class Timers: public LLSingleton<Timers>
{
    LLSINGLETON(Timers);

    using token_t = U32;

    // Define a struct for our priority queue entries, instead of using
    // a tuple, because we need to define the comparison operator.
    struct func_at
    {
        // callback to run when this timer fires
        bool_func_t mFunc;
        // key to look up metadata in mHandles
        token_t mToken;
        // time at which this timer is supposed to fire
        LLDate::timestamp mTime;

        func_at(const bool_func_t& func, token_t token, LLDate::timestamp tm):
            mFunc(func),
            mToken(token),
            mTime(tm)
        {}

        friend bool operator<(const func_at& lhs, const func_at& rhs)
        {
            // use greater-than because we want fibonacci_heap to select the
            // EARLIEST time as the top()
            return lhs.mTime > rhs.mTime;
        }
    };

    // Accept default stable<false>: when two funcs have the same timestamp,
    // we don't care in what order they're called.
    // Specify constant_time_size<false>: we don't need to optimize the size()
    // method, iow we don't need to store and maintain a count of entries.
    typedef boost::heap::fibonacci_heap<func_at, boost::heap::constant_time_size<false>>
        queue_t;

public:
    // If tasks that come ready during a given tick() take longer than this,
    // defer any subsequent ready tasks to a future tick() call.
    static constexpr F32 DEFAULT_TIMESLICE{ 0.005f };
    // Setting timeslice to be less than MINIMUM_TIMESLICE could lock up
    // Timers processing, causing it to believe it's exceeded the allowable
    // time every tick before processing ANY queue items.
    static constexpr F32 MINIMUM_TIMESLICE{ 0.001f };

    class handle_t
    {
    private:
        friend class Timers;
        token_t token;
    public:
        handle_t(token_t token=0): token(token) {}
        bool operator==(const handle_t& rhs) const { return this->token == rhs.token; }
        explicit operator bool() const { return bool(token); }
        bool operator!() const { return ! bool(*this); }
    };

    // Call a given callable once at specified timestamp.
    handle_t scheduleAt(nullary_func_t callable, LLDate::timestamp time);

    // Call a given callable once after specified interval.
    handle_t scheduleAfter(nullary_func_t callable, F32 seconds);

    // Call a given callable every specified number of seconds, until it returns true.
    handle_t scheduleEvery(bool_func_t callable, F32 seconds);

    // test whether specified handle is still live
    bool isRunning(handle_t timer) const;
    // check remaining time
    F32 timeUntilCall(handle_t timer) const;

    // Cancel a future timer set by scheduleAt(), scheduleAfter(), scheduleEvery().
    // Return true if and only if the handle corresponds to a live timer.
    bool cancel(const handle_t& timer);
    // If we're canceling a non-const handle_t, also clear it so we need not
    // cancel again.
    bool cancel(handle_t& timer);

    F32  getTimeslice() const { return mTimeslice; }
    void setTimeslice(F32 timeslice);

    // Store a handle_t returned by scheduleAt(), scheduleAfter() or
    // scheduleEvery() in a temp_handle_t to cancel() automatically on
    // destruction of the temp_handle_t.
    class temp_handle_t
    {
    public:
        temp_handle_t() = default;
        temp_handle_t(const handle_t& hdl): mHandle(hdl) {}
        temp_handle_t(const temp_handle_t&) = delete;
        temp_handle_t(temp_handle_t&&) = default;
        temp_handle_t& operator=(const handle_t& hdl)
        {
            // initializing a new temp_handle_t, then swapping it into *this,
            // takes care of destroying any previous mHandle
            temp_handle_t replacement(hdl);
            swap(replacement);
            return *this;
        }
        temp_handle_t& operator=(const temp_handle_t&) = delete;
        temp_handle_t& operator=(temp_handle_t&&) = default;
        ~temp_handle_t()
        {
            cancel();
        }

        // temp_handle_t should be usable wherever handle_t is
        operator handle_t() const { return mHandle; }
        // If we're dealing with a non-const temp_handle_t, pass a reference
        // to our handle_t member (e.g. to Timers::cancel()).
        operator handle_t&() { return mHandle; }

        // For those in the know, provide a cancel() method of our own that
        // avoids Timers::instance() lookup when mHandle isn't live.
        bool cancel()
        {
            if (! mHandle)
            {
                return false;
            }
            else
            {
                return Timers::instance().cancel(mHandle);
            }
        }

        void swap(temp_handle_t& other) noexcept
        {
            std::swap(this->mHandle, other.mHandle);
        }

    private:
        handle_t mHandle;
    };

private:
    handle_t scheduleAtEvery(bool_func_t callable, LLDate::timestamp time, F32 interval);
    LLDate::timestamp now() const { return LLDate::now().secondsSinceEpoch(); }
    // wrap a nullary_func_t with a bool_func_t that will only execute once
    bool_func_t once(nullary_func_t callable)
    {
        return [callable]
        {
            callable();
            return true;
        };
    }
    bool tick();

    // NOTE: We don't lock our data members because it doesn't make sense to
    // register cross-thread callbacks. If we start wanting to use Timers on
    // threads other than the main thread, it would make more sense to make
    // our data members thread_local than to lock them.

    // the heap aka priority queue
    queue_t mQueue;

    // metadata about a given task
    struct Metadata
    {
        // handle to mQueue entry
        queue_t::handle_type mHandle;
        // time at which this timer is supposed to fire
        LLDate::timestamp mTime;
        // interval at which this timer is supposed to fire repeatedly
        F32 mInterval{ 0 };
        // mFunc is currently running: don't delete this entry
        bool mRunning{ false };
        // cancel() was called while mFunc was running: deferred cancel
        bool mCancel{ false };
    };

    using MetaMap = std::unordered_map<token_t, Metadata>;
    MetaMap mMeta;
    token_t mToken{ 0 };
    // While mQueue is non-empty, register for regular callbacks.
    LLCallbackList::temp_handle_t mLive;
    F32 mTimeslice{ DEFAULT_TIMESLICE };
};

} // namespace LL

/*-------------------- legacy names in global namespace --------------------*/
// Call a given callable once after specified interval.
inline
LL::Timers::handle_t doAfterInterval(nullary_func_t callable, F32 seconds)
{
    return LL::Timers::instance().scheduleAfter(callable, seconds);
}

// Call a given callable every specified number of seconds, until it returns true.
inline
LL::Timers::handle_t doPeriodically(bool_func_t callable, F32 seconds)
{
    return LL::Timers::instance().scheduleEvery(callable, seconds);
}

#endif
