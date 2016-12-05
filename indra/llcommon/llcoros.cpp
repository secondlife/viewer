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
// other Linden headers
#include "llevents.h"
#include "llerror.h"
#include "stringize.h"
#include "llexception.h"

// do nothing, when we need nothing done
void LLCoros::no_cleanup(CoroData*) {}

// CoroData for the currently-running coroutine. Use a thread_specific_ptr
// because each thread potentially has its own distinct pool of coroutines.
// This thread_specific_ptr does NOT own the CoroData object! That's owned by
// LLCoros::mCoros. It merely identifies it. For this reason we instantiate
// it with a no-op cleanup function.
boost::thread_specific_ptr<LLCoros::CoroData>
LLCoros::sCurrentCoro(LLCoros::no_cleanup);

//static
LLCoros::CoroData& LLCoros::get_CoroData(const std::string& caller)
{
    CoroData* current = sCurrentCoro.get();
    if (! current)
    {
        LL_ERRS("LLCoros") << "Calling " << caller << " from non-coroutine context!" << LL_ENDL;
    }
    return *current;
}

//static
LLCoros::coro::self& LLCoros::get_self()
{
    return *get_CoroData("get_self()").mSelf;
}

//static
void LLCoros::set_consuming(bool consuming)
{
    get_CoroData("set_consuming()").mConsuming = consuming;
}

//static
bool LLCoros::get_consuming()
{
    return get_CoroData("get_consuming()").mConsuming;
}

llcoro::Suspending::Suspending():
    mSuspended(LLCoros::sCurrentCoro.get())
{
    // Revert mCurrentCoro to the value it had at the moment we last switched
    // into this coroutine.
    LLCoros::sCurrentCoro.reset(mSuspended->mPrev);
}

llcoro::Suspending::~Suspending()
{
    // Okay, we're back, update our mPrev
    mSuspended->mPrev = LLCoros::sCurrentCoro.get();
    // and reinstate our sCurrentCoro.
    LLCoros::sCurrentCoro.reset(mSuspended);
}

LLCoros::LLCoros():
    // MAINT-2724: default coroutine stack size too small on Windows.
    // Previously we used
    // boost::context::guarded_stack_allocator::default_stacksize();
    // empirically this is 64KB on Windows and Linux. Try quadrupling.
#if ADDRESS_SIZE == 64
    mStackSize(512*1024)
#else
    mStackSize(256*1024)
#endif
{
    // Register our cleanup() method for "mainloop" ticks
    LLEventPumps::instance().obtain("mainloop").listen(
        "LLCoros", boost::bind(&LLCoros::cleanup, this, _1));
}

bool LLCoros::cleanup(const LLSD&)
{
    static std::string previousName;
    static int previousCount = 0;
    // Walk the mCoros map, checking and removing completed coroutines.
    for (CoroMap::iterator mi(mCoros.begin()), mend(mCoros.end()); mi != mend; )
    {
        // Has this coroutine exited (normal return, exception, exit() call)
        // since last tick?
        if (mi->second->mCoro.exited())
        {
            if (previousName != mi->first)
            { 
                previousName = mi->first;
                previousCount = 1;
            }
            else
            {
                ++previousCount;
            }
               
            if ((previousCount < 5) || !(previousCount % 50))
            {
                if (previousCount < 5)
                    LL_DEBUGS("LLCoros") << "LLCoros: cleaning up coroutine " << mi->first << LL_ENDL;
                else
                    LL_DEBUGS("LLCoros") << "LLCoros: cleaning up coroutine " << mi->first << "("<< previousCount << ")" << LL_ENDL;

            }
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
    static std::string previousName;
    static int previousCount = 0;

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
            if (previousName != name)
            {
                previousName = name;
                previousCount = 1;
            }
            else
            {
                ++previousCount;
            }

            if ((previousCount < 5) || !(previousCount % 50))
            {
                if (previousCount < 5)
                    LL_DEBUGS("LLCoros") << "LLCoros: launching coroutine " << name << LL_ENDL;
                else
                    LL_DEBUGS("LLCoros") << "LLCoros: launching coroutine " << name << "(" << previousCount << ")" << LL_ENDL;

            }

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
    CoroData* current = sCurrentCoro.get();
    if (! current)
    {
        // not in a coroutine
        return "";
    }
    return current->mName;
}

void LLCoros::setStackSize(S32 stacksize)
{
    LL_DEBUGS("LLCoros") << "Setting coroutine stack size to " << stacksize << LL_ENDL;
    mStackSize = stacksize;
}

// Top-level wrapper around caller's coroutine callable. This function accepts
// the coroutine library's implicit coro::self& parameter and sets sCurrentSelf
// but does not pass it down to the caller's callable.
void LLCoros::toplevel(coro::self& self, CoroData* data, const callable_t& callable)
{
    // capture the 'self' param in CoroData
    data->mSelf = &self;
    // run the code the caller actually wants in the coroutine
    try
    {
        callable();
    }
    catch (const LLContinueError&)
    {
        // Any uncaught exception derived from LLContinueError will be caught
        // here and logged. This coroutine will terminate but the rest of the
        // viewer will carry on.
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << data->mName));
    }
    catch (...)
    {
        // Any OTHER kind of uncaught exception will cause the viewer to
        // crash, hopefully informatively.
        CRASH_ON_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << data->mName));
    }
    // This cleanup isn't perfectly symmetrical with the way we initially set
    // data->mPrev, but this is our last chance to reset mCurrentCoro.
    sCurrentCoro.reset(data->mPrev);
}

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
#pragma warning(disable: 4355) // 'this' used in initializer list: yes, intentionally
#pragma optimize("", off)
#endif // LL_MSVC

LLCoros::CoroData::CoroData(CoroData* prev, const std::string& name,
                            const callable_t& callable, S32 stacksize):
    mPrev(prev),
    mName(name),
    // Wrap the caller's callable in our toplevel() function so we can manage
    // sCurrentCoro appropriately at startup and shutdown of each coroutine.
    mCoro(boost::bind(toplevel, _1, this, callable), stacksize),
    // don't consume events unless specifically directed
    mConsuming(false),
    mSelf(0)
{
}

std::string LLCoros::launch(const std::string& prefix, const callable_t& callable)
{
    std::string name(generateDistinctName(prefix));
    // pass the current value of sCurrentCoro as previous context
    CoroData* newCoro = new CoroData(sCurrentCoro.get(), name,
                                     callable, mStackSize);
    // Store it in our pointer map
    mCoros.insert(name, newCoro);
    // also set it as current
    sCurrentCoro.reset(newCoro);
    /* Run the coroutine until its first wait, then return here */
    (newCoro->mCoro)(std::nothrow);
    return name;
}

#if LL_MSVC
// reenable optimizations
#pragma optimize("", on)
#endif // LL_MSVC
