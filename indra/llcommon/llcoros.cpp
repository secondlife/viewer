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

#if LL_WINDOWS
#include <excpt.h>
#endif

namespace {
void no_op() {}
} // anonymous namespace

// Do nothing, when we need nothing done. This is a static member of LLCoros
// because CoroData is a private nested class.
void LLCoros::no_cleanup(CoroData*) {}

// CoroData for the currently-running coroutine. Use a thread_specific_ptr
// because each thread potentially has its own distinct pool of coroutines.
LLCoros::Current::Current()
{
    // Use a function-static instance so this thread_specific_ptr is
    // instantiated on demand. Since we happen to know it's consumed by
    // LLSingleton, this is likely to happen before the runtime has finished
    // initializing module-static data. For the same reason, we can't package
    // this pointer in an LLSingleton.

    // This thread_specific_ptr does NOT own the CoroData object! That's owned
    // by LLCoros::mCoros. It merely identifies it. For this reason we
    // instantiate it with a no-op cleanup function.
    static boost::thread_specific_ptr<LLCoros::CoroData> sCurrent(LLCoros::no_cleanup);

    // If this is the first time we're accessing sCurrent for the running
    // thread, its get() will be NULL. This could be a problem, in that
    // llcoro::get_id() would return the same (NULL) token value for the "main
    // coroutine" in every thread, whereas what we really want is a distinct
    // value for every distinct stack in the process. So if get() is NULL,
    // give it a heap CoroData: this ensures that llcoro::get_id() will return
    // distinct values.
    // This tactic is "leaky": sCurrent explicitly does not destroy any
    // CoroData to which it points, and we do NOT enter these "main coroutine"
    // CoroData instances in the LLCoros::mCoros map. They are dummy entries,
    // and they will leak at process shutdown: one CoroData per thread.
    if (! sCurrent.get())
    {
        // It's tempting to provide a distinct name for each thread's "main
        // coroutine." But as getName() has always returned the empty string
        // to mean "not in a coroutine," empty string should suffice here --
        // and truthfully the additional (thread-safe!) machinery to ensure
        // uniqueness just doesn't feel worth the trouble.
        // We use a no-op callable and a minimal stack size because, although
        // CoroData's constructor in fact initializes its mCoro with a
        // coroutine with that stack size, no one ever actually enters it by
        // calling mCoro().
        sCurrent.reset(new CoroData(0,  // no prev
                                    "", // not a named coroutine
                                    no_op,  // no-op callable
                                    1024)); // stacksize moot
    }

    mCurrent = &sCurrent;
}

//static
LLCoros::CoroData& LLCoros::get_CoroData(const std::string& caller)
{
    CoroData* current = Current();
    // With the dummy CoroData set in LLCoros::Current::Current(), this
    // pointer should never be NULL.
    llassert_always(current);
    return *current;
}

//static
LLCoros::coro::self& LLCoros::get_self()
{
    CoroData& current = get_CoroData("get_self()");
    if (! current.mSelf)
    {
        LL_ERRS("LLCoros") << "Calling get_self() from non-coroutine context!" << LL_ENDL;
    }
    return *current.mSelf;
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

llcoro::Suspending::Suspending()
{
    LLCoros::Current current;
    // Remember currently-running coroutine: we're about to suspend it.
    mSuspended = current;
    // Revert Current to the value it had at the moment we last switched
    // into this coroutine.
    current.reset(mSuspended->mPrev);
}

llcoro::Suspending::~Suspending()
{
    LLCoros::Current current;
    // Okay, we're back, update our mPrev
    mSuspended->mPrev = current;
    // and reinstate our Current.
    current.reset(mSuspended);
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
    return Current()->mName;
}

void LLCoros::setStackSize(S32 stacksize)
{
    LL_DEBUGS("LLCoros") << "Setting coroutine stack size to " << stacksize << LL_ENDL;
    mStackSize = stacksize;
}

#if LL_WINDOWS

static const U32 STATUS_MSC_EXCEPTION = 0xE06D7363; // compiler specific

U32 exception_filter(U32 code, struct _EXCEPTION_POINTERS *exception_infop)
{
    if (code == STATUS_MSC_EXCEPTION)
    {
        // C++ exception, go on
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else
    {
        // handle it
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

void LLCoros::winlevel(const callable_t& callable)
{
    __try
    {
        callable();
    }
    __except (exception_filter(GetExceptionCode(), GetExceptionInformation()))
    {
        // convert to C++ styled exception
        // Note: it might be better to use _se_set_translator
        // if you want exception to inherit full callstack
        char integer_string[32];
        sprintf(integer_string, "SEH, code: %lu\n", GetExceptionCode());
        throw std::exception(integer_string);
    }
}

#endif

// Top-level wrapper around caller's coroutine callable. This function accepts
// the coroutine library's implicit coro::self& parameter and saves it, but
// does not pass it down to the caller's callable.
void LLCoros::toplevel(coro::self& self, CoroData* data, const callable_t& callable)
{
    // capture the 'self' param in CoroData
    data->mSelf = &self;
    // run the code the caller actually wants in the coroutine
    try
    {
#if LL_WINDOWS
        winlevel(callable);
#else
        callable();
#endif
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
    // data->mPrev, but this is our last chance to reset Current.
    Current().reset(data->mPrev);
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
    // Current appropriately at startup and shutdown of each coroutine.
    mCoro(boost::bind(toplevel, _1, this, callable), stacksize),
    // don't consume events unless specifically directed
    mConsuming(false),
    mSelf(0)
{
}

std::string LLCoros::launch(const std::string& prefix, const callable_t& callable)
{
    std::string name(generateDistinctName(prefix));
    Current current;
    // pass the current value of Current as previous context
    CoroData* newCoro = new CoroData(current, name, callable, mStackSize);
    // Store it in our pointer map
    mCoros.insert(name, newCoro);
    // also set it as current
    current.reset(newCoro);
    /* Run the coroutine until its first wait, then return here */
    (newCoro->mCoro)(std::nothrow);
    return name;
}

#if LL_MSVC
// reenable optimizations
#pragma optimize("", on)
#endif // LL_MSVC
