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
#include <boost/fiber/fiber.hpp>
#ifndef BOOST_DISABLE_ASSERTS
#define UNDO_BOOST_DISABLE_ASSERTS
// with Boost 1.65.1, needed for Mac with this specific header
#define BOOST_DISABLE_ASSERTS
#endif
#include <boost/fiber/protected_fixedsize_stack.hpp>
#ifdef UNDO_BOOST_DISABLE_ASSERTS
#undef UNDO_BOOST_DISABLE_ASSERTS
#undef BOOST_DISABLE_ASSERTS
#endif
// other Linden headers
#include "lltimer.h"
#include "llevents.h"
#include "llerror.h"
#include "stringize.h"
#include "llexception.h"

#if LL_WINDOWS
#include <excpt.h>
#endif


const LLCoros::CoroData& LLCoros::get_CoroData(const std::string& caller) const
{
    CoroData* current = mCurrent.get();
    // For the main() coroutine, the one NOT explicitly launched by launch(),
    // we never explicitly set mCurrent. Use a static CoroData instance with
    // canonical values.
    if (! current)
    {
        // It's tempting to provide a distinct name for each thread's "main
        // coroutine." But as getName() has always returned the empty string
        // to mean "not in a coroutine," empty string should suffice here.
        static CoroData sMain("");
        // We need not reset() the local_ptr to this read-only data: reuse the
        // same instance for every thread's main coroutine.
        current = &sMain;
    }
    return *current;
}

LLCoros::CoroData& LLCoros::get_CoroData(const std::string& caller)
{
    // reuse const implementation, just cast away const-ness of result
    return const_cast<CoroData&>(const_cast<const LLCoros*>(this)->get_CoroData(caller));
}

//static
LLCoros::coro::id LLCoros::get_self()
{
    return boost::this_fiber::get_id();
}

//static
void LLCoros::set_consuming(bool consuming)
{
    CoroData& data(LLCoros::instance().get_CoroData("set_consuming()"));
    // DO NOT call this on the main() coroutine.
    llassert_always(! data.mName.empty());
    data.mConsuming = consuming;
}

//static
bool LLCoros::get_consuming()
{
    return LLCoros::instance().get_CoroData("get_consuming()").mConsuming;
}

LLCoros::LLCoros():
    // MAINT-2724: default coroutine stack size too small on Windows.
    // Previously we used
    // boost::context::guarded_stack_allocator::default_stacksize();
    // empirically this is insufficient.
#if ADDRESS_SIZE == 64
    mStackSize(512*1024)
#else
    mStackSize(256*1024)
#endif
{
}

std::string LLCoros::generateDistinctName(const std::string& prefix) const
{
    static int unique = 0;

    // Allowing empty name would make getName()'s not-found return ambiguous.
    if (prefix.empty())
    {
        LL_ERRS("LLCoros") << "LLCoros::launch(): pass non-empty name string" << LL_ENDL;
    }

    // If the specified name isn't already in the map, just use that.
    std::string name(prefix);

    // Until we find an unused name, append a numeric suffix for uniqueness.
    while (mCoros.find(name) != mCoros.end())
    {
        name = STRINGIZE(prefix << unique++);
    }
    return name;
}

/*==========================================================================*|
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
|*==========================================================================*/

std::string LLCoros::getName() const
{
    return get_CoroData("getName()").mName;
}

void LLCoros::setStackSize(S32 stacksize)
{
    LL_DEBUGS("LLCoros") << "Setting coroutine stack size to " << stacksize << LL_ENDL;
    mStackSize = stacksize;
}

void LLCoros::printActiveCoroutines()
{
    LL_INFOS("LLCoros") << "Number of active coroutines: " << (S32)mCoros.size() << LL_ENDL;
    if (mCoros.size() > 0)
    {
        LL_INFOS("LLCoros") << "-------------- List of active coroutines ------------";
        CoroMap::iterator iter;
        CoroMap::iterator end = mCoros.end();
        F64 time = LLTimer::getTotalSeconds();
        for (iter = mCoros.begin(); iter != end; iter++)
        {
            F64 life_time = time - iter->second->mCreationTime;
            LL_CONT << LL_NEWLINE << "Name: " << iter->first << " life: " << life_time;
        }
        LL_CONT << LL_ENDL;
        LL_INFOS("LLCoros") << "-----------------------------------------------------" << LL_ENDL;
    }
}

std::string LLCoros::launch(const std::string& prefix, const callable_t& callable)
{
    std::string name(generateDistinctName(prefix));
    // 'dispatch' means: enter the new fiber immediately, returning here only
    // when the fiber yields for whatever reason.
    // std::allocator_arg is a flag to indicate that the following argument is
    // a StackAllocator.
    // protected_fixedsize_stack sets a guard page past the end of the new
    // stack so that stack underflow will result in an access violation
    // instead of weird, subtle, possibly undiagnosed memory stomps.
    boost::fibers::fiber newCoro(boost::fibers::launch::dispatch,
                                 std::allocator_arg,
                                 boost::fibers::protected_fixedsize_stack(mStackSize),
                                 [this, &name, &callable](){ toplevel(name, callable); });
    // You have two choices with a fiber instance: you can join() it or you
    // can detach() it. If you try to destroy the instance before doing
    // either, the program silently terminates. We don't need this handle.
    newCoro.detach();
    return name;
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

// Top-level wrapper around caller's coroutine callable.
void LLCoros::toplevel(const std::string& name, const callable_t& callable)
{
    CoroData* corodata = new CoroData(name);
    // Store it in our pointer map. Oddly, must cast away const-ness of key.
    mCoros.insert(const_cast<std::string&>(name), corodata);
    // also set it as current
    mCurrent.reset(corodata);

    // run the code the caller actually wants in the coroutine
    try
    {
#if LL_WINDOWS && LL_RELEASE_FOR_DOWNLOAD
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
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << corodata->mName));
    }
    catch (...)
    {
        // Any OTHER kind of uncaught exception will cause the viewer to
        // crash, hopefully informatively.
        CRASH_ON_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << corodata->mName));
    }
}

LLCoros::CoroData::CoroData(const std::string& name):
    mName(name),
    // don't consume events unless specifically directed
    mConsuming(false),
    mCreationTime(LLTimer::getTotalSeconds())
{
}

void LLCoros::delete_CoroData(CoroData* cdptr)
{
    // This custom cleanup function is necessarily static. Find and bind the
    // LLCoros instance.
    LLCoros& self(LLCoros::instance());
    // We set mCurrent on entry to a new fiber, expecting that the
    // corresponding entry has already been stored in mCoros. It is an
    // error if we do not find that entry.
    CoroMap::iterator found = self.mCoros.find(cdptr->mName);
    if (found == self.mCoros.end())
    {
        LL_ERRS("LLCoros") << "Coroutine '" << cdptr->mName << "' terminated "
                           << "without being stored in LLCoros::mCoros"
                           << LL_ENDL;
    }

    // Oh good, we found the mCoros entry. Erase it. Because it's a ptr_map,
    // that will implicitly delete this CoroData.
    self.mCoros.erase(found);
}
