/**
 * @file   llcoros.cpp
 * @author Nat Goodspeed
 * @date   2009-06-03
 * @brief  Implementation for llcoros.
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
#include "llcoros.h"
// STL headers
// std headers
// external library headers
#include <boost/bind.hpp>
#include <boost/thread/tss.hpp>
// other Linden headers
#include "llevents.h"
#include "llerror.h"
#include "stringize.h"

namespace {

// do nothing, when we need nothing done
void no_cleanup(LLCoros::coro::self*) {}

// When the dcoroutine library calls a top-level callable, it implicitly
// passes coro::self& as the first parameter. All our consumer code used to
// explicitly pass coro::self& down through all levels of call stack, because
// at the leaf level we need it for context-switching. But since coroutines
// are based on cooperative switching, we can cause the top-level entry point
// to stash a static pointer to the currently-running coroutine, and manage it
// appropriately as we switch out and back in. That eliminates the need to
// pass it as an explicit parameter down through every level, which is
// unfortunately viral in nature. Finding it implicitly rather than explicitly
// allows minor maintenance in which a leaf-level function adds a new async
// I/O call that suspends the calling coroutine, WITHOUT having to propagate
// coro::self& through every function signature down to that point -- and of
// course through every other caller of every such function.
// We use a boost::thread_specific_ptr because each thread potentially has its
// own distinct pool of coroutines.
// This thread_specific_ptr does NOT own the 'self' object! It merely
// identifies it. For this reason we instantiate it with a no-op cleanup
// function.
static boost::thread_specific_ptr<LLCoros::coro::self>
sCurrentSelf(no_cleanup);

} // anonymous

//static
LLCoros::coro::self& llcoro::get_self()
{
    LLCoros::coro::self* current_self = sCurrentSelf.get();
    if (! current_self)
    {
        LL_ERRS("LLCoros") << "Calling get_self() from non-coroutine context!" << LL_ENDL;
    }
    return *current_self;
}

llcoro::Suspending::Suspending():
    mSuspended(sCurrentSelf.get())
{
    // For the duration of our time away from this coroutine, sCurrentSelf
    // must NOT refer to this coroutine.
    sCurrentSelf.reset();
}

llcoro::Suspending::~Suspending()
{
    // Okay, we're back, reinstate previous value of sCurrentSelf.
    sCurrentSelf.reset(mSuspended);
}

LLCoros::LLCoros():
    // MAINT-2724: default coroutine stack size too small on Windows.
    // Previously we used
    // boost::context::guarded_stack_allocator::default_stacksize();
    // empirically this is 64KB on Windows and Linux. Try quadrupling.
    mStackSize(256*1024)
{
    // Register our cleanup() method for "mainloop" ticks
    LLEventPumps::instance().obtain("mainloop").listen(
        "LLCoros", boost::bind(&LLCoros::cleanup, this, _1));
}

bool LLCoros::cleanup(const LLSD&)
{
    // Walk the mCoros map, checking and removing completed coroutines.
    for (CoroMap::iterator mi(mCoros.begin()), mend(mCoros.end()); mi != mend; )
    {
        // Has this coroutine exited (normal return, exception, exit() call)
        // since last tick?
        if (mi->second->exited())
        {
            LL_INFOS("LLCoros") << "LLCoros: cleaning up coroutine " << mi->first << LL_ENDL;
            // The erase() call will invalidate its passed iterator value --
            // so increment mi FIRST -- but pass its original value to
            // erase(). This is what postincrement is all about.
            mCoros.erase(mi++);
        }
        else
        {
            // Still live, just skip this entry as if incrementing at the top
            // of the loop as usual.
            ++mi;
        }
    }
    return false;
}

std::string LLCoros::generateDistinctName(const std::string& prefix) const
{
    // Allowing empty name would make getName()'s not-found return ambiguous.
    if (prefix.empty())
    {
        LL_ERRS("LLCoros") << "LLCoros::launch(): pass non-empty name string" << LL_ENDL;
    }

    // If the specified name isn't already in the map, just use that.
    std::string name(prefix);

    // Find the lowest numeric suffix that doesn't collide with an existing
    // entry. Start with 2 just to make it more intuitive for any interested
    // parties: e.g. "joe", "joe2", "joe3"...
    for (int i = 2; ; name = STRINGIZE(prefix << i++))
    {
        if (mCoros.find(name) == mCoros.end())
        {
            LL_INFOS("LLCoros") << "LLCoros: launching coroutine " << name << LL_ENDL;
            return name;
        }
    }
}

bool LLCoros::kill(const std::string& name)
{
    CoroMap::iterator found = mCoros.find(name);
    if (found == mCoros.end())
    {
        return false;
    }
    // Because this is a boost::ptr_map, erasing the map entry also destroys
    // the referenced heap object, in this case the boost::coroutine object,
    // which will terminate the coroutine.
    mCoros.erase(found);
    return true;
}

std::string LLCoros::getName() const
{
    // Walk the existing coroutines, looking for the current one.
    void* self_id = llcoro::get_self().get_id();
    for (CoroMap::const_iterator mi(mCoros.begin()), mend(mCoros.end()); mi != mend; ++mi)
    {
        namespace coro_private = boost::dcoroutines::detail;
        if (static_cast<void*>(coro_private::coroutine_accessor::get_impl(const_cast<coro&>(*mi->second)).get())
            == self_id)
        {
            return mi->first;
        }
    }
    return "";
}

void LLCoros::setStackSize(S32 stacksize)
{
    LL_INFOS("LLCoros") << "Setting coroutine stack size to " << stacksize << LL_ENDL;
    mStackSize = stacksize;
}

namespace {

// Top-level wrapper around caller's coroutine callable. This function accepts
// the coroutine library's implicit coro::self& parameter and sets sCurrentSelf
// but does not pass it down to the caller's callable.
void toplevel(LLCoros::coro::self& self, const LLCoros::callable_t& callable)
{
    sCurrentSelf.reset(&self);
    callable();
    sCurrentSelf.reset();
}

} // anonymous

/*****************************************************************************
*   MUST BE LAST
*****************************************************************************/
// Turn off MSVC optimizations for just LLCoros::launch() -- see
// DEV-32777. But MSVC doesn't support push/pop for optimization flags as it
// does for warning suppression, and we really don't want to force
// optimization ON for other code even in Debug or RelWithDebInfo builds.

#if LL_MSVC
// work around broken optimizations
#pragma warning(disable: 4748)
#pragma optimize("", off)
#endif // LL_MSVC

std::string LLCoros::launch(const std::string& prefix, const callable_t& callable)
{
    std::string name(generateDistinctName(prefix));
    // Wrap the caller's callable in our toplevel() function so we can manage
    // sCurrentSelf appropriately at startup and shutdown of each coroutine.
    coro* newCoro = new coro(boost::bind(toplevel, _1, callable), mStackSize);
    // Store it in our pointer map
    mCoros.insert(name, newCoro);
    /* Run the coroutine until its first wait, then return here */
    (*newCoro)(std::nothrow);
    return name;
}

#if LL_MSVC
// reenable optimizations
#pragma optimize("", on)
#endif // LL_MSVC
