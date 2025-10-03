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

#include "llwin32headers.h"

// Precompiled header
#include "linden_common.h"
// associated header
#include "llcoros.h"
// STL headers
// std headers
#include <atomic>
#include <stdexcept>
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
#include "llapp.h"
#include "lltimer.h"
#include "llevents.h"
#include "llerror.h"
#include "stringize.h"
#include "llexception.h"

#if LL_WINDOWS
#include <excpt.h>
#endif

// static
bool LLCoros::on_main_coro()
{
    if (!LLCoros::instanceExists() || LLCoros::getName().empty())
    {
        return true;
    }

    return false;
}

// static
bool LLCoros::on_main_thread_main_coro()
{
    return on_main_coro() && on_main_thread();
}

// static
LLCoros::CoroData& LLCoros::get_CoroData(const std::string& caller)
{
    CoroData* current{ nullptr };
    // be careful about attempted accesses in the final throes of app shutdown
    if (! wasDeleted())
    {
        current = instance().mCurrent.get();
    }
    // For the main() coroutine, the one NOT explicitly launched by launch(),
    // we never explicitly set mCurrent. Use a static CoroData instance with
    // canonical values.
    if (! current)
    {
        static std::atomic<int> which_thread(0);
        // Use alternate CoroData constructor.
        static thread_local CoroData sMain(which_thread++);
        // We need not reset() the local_ptr to this instance; we'll simply
        // find it again every time we discover that current is null.
        current = &sMain;
    }
    return *current;
}

//static
LLCoros::coro::id LLCoros::get_self()
{
    return boost::this_fiber::get_id();
}

//static
void LLCoros::set_consuming(bool consuming)
{
    CoroData& data(get_CoroData("set_consuming()"));
    // DO NOT call this on the main() coroutine.
    llassert_always(! data.mName.empty());
    data.mConsuming = consuming;
}

//static
bool LLCoros::get_consuming()
{
    return get_CoroData("get_consuming()").mConsuming;
}

// static
void LLCoros::setStatus(const std::string& status)
{
    get_CoroData("setStatus()").mStatus = status;
}

// static
std::string LLCoros::getStatus()
{
    return get_CoroData("getStatus()").mStatus;
}

LLCoros::LLCoros():
    // MAINT-2724: default coroutine stack size too small on Windows.
    // Previously we used
    // boost::context::guarded_stack_allocator::default_stacksize();
    // empirically this is insufficient.
    mStackSize(1024*1024),
    // mCurrent does NOT own the current CoroData instance -- it simply
    // points to it. So initialize it with a no-op deleter.
    mCurrent{ [](CoroData*){} }
{
}

LLCoros::~LLCoros()
{
}

void LLCoros::cleanupSingleton()
{
    // Some of the coroutines (like voice) will depend onto
    // origin singletons, so clean coros before deleting those

    printActiveCoroutines("at entry to ~LLCoros()");
    // Other LLApp status-change listeners do things like close
    // work queues and inject the Stop exception into pending
    // promises, to force coroutines waiting on those things to
    // notice and terminate. The only problem is that by the time
    // LLApp sets "quitting" status, the main loop has stopped
    // pumping the fiber scheduler with yield() calls. A waiting
    // coroutine still might not wake up until after resources on
    // which it depends have been freed. Pump it a few times
    // ourselves. Of course, stop pumping as soon as the last of
    // the coroutines has terminated.
    for (size_t count = 0; count < 10 && CoroData::instanceCount() > 0; ++count)
    {
        // don't use llcoro::suspend() because that module depends
        // on this one
        // This will yield current(main) thread and will let active
        // corutines run once
        boost::this_fiber::yield();
    }
    printActiveCoroutines("after pumping");
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
    while (CoroData::getInstance(name))
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

//static
std::string LLCoros::getName()
{
    return get_CoroData("getName()").mName;
}

//static
std::string LLCoros::logname()
{
    LLCoros::CoroData& data(get_CoroData("logname()"));
    return data.mName.empty()? data.getKey() : data.mName;
}

void LLCoros::saveException(const std::string& name, std::exception_ptr exc)
{
    mExceptionQueue.emplace(name, exc);
}

void LLCoros::rethrow()
{
    if (! mExceptionQueue.empty())
    {
        ExceptionData front = mExceptionQueue.front();
        mExceptionQueue.pop();
        LL_WARNS("LLCoros") << "Rethrowing exception from coroutine " << front.name << LL_ENDL;
        std::rethrow_exception(front.exception);
    }
}

void LLCoros::setStackSize(S32 stacksize)
{
    LL_DEBUGS("LLCoros") << "Setting coroutine stack size to " << stacksize << LL_ENDL;
    mStackSize = stacksize;
}

void LLCoros::printActiveCoroutines(const std::string& when)
{
    LL_INFOS("LLCoros") << "Number of active coroutines " << when
                        << ": " << CoroData::instanceCount() << LL_ENDL;
    if (CoroData::instanceCount() > 0)
    {
        LL_INFOS("LLCoros") << "-------------- List of active coroutines ------------";
        F64 time = LLTimer::getTotalSeconds();
        for (auto& cd : CoroData::instance_snapshot())
        {
            F64 life_time = time - cd.mCreationTime;
            LL_CONT << LL_NEWLINE
                    << cd.getKey() << ' ' << cd.mStatus << " life: " << life_time;
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

    try
    {
        boost::fibers::fiber newCoro(boost::fibers::launch::dispatch,
            std::allocator_arg,
            boost::fibers::protected_fixedsize_stack(mStackSize),
            [this, &name, &callable]() { toplevel(name, callable); });

        // You have two choices with a fiber instance: you can join() it or you
        // can detach() it. If you try to destroy the instance before doing
        // either, the program silently terminates. We don't need this handle.
        newCoro.detach();
    }
    catch (std::bad_alloc&)
    {
        // Out of memory on stack allocation?
        LLError::LLUserWarningMsg::showOutOfMemory();
        printActiveCoroutines();
        LL_ERRS("LLCoros") << "Bad memory allocation in LLCoros::launch(" << prefix << ")!" << LL_ENDL;
    }

    return name;
}

namespace
{

#if LL_WINDOWS

static const U32 STATUS_MSC_EXCEPTION = 0xE06D7363; // compiler specific

U32 exception_filter(U32 code, struct _EXCEPTION_POINTERS* exception_infop)
{
    if (LLApp::instance()->reportCrashToBugsplat((void*)exception_infop))
    {
        // Handled
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else if (code == STATUS_MSC_EXCEPTION)
    {
        // C++ exception, go on
        return EXCEPTION_CONTINUE_SEARCH;
    }
    else
    {
        // handle it, convert to std::exception
        return EXCEPTION_EXECUTE_HANDLER;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void cpphandle(const LLCoros::callable_t& callable, const std::string& name)
{
    // SE and C++ can not coexists, thus two handlers
    try
    {
        callable();
    }
    catch (const LLCoros::Stop& exc)
    {
        LL_INFOS("LLCoros") << "coroutine " << name << " terminating because "
            << exc.what() << LL_ENDL;
    }
    catch (const LLContinueError&)
    {
        // Any uncaught exception derived from LLContinueError will be caught
        // here and logged. This coroutine will terminate but the rest of the
        // viewer will carry on.
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << name));
    }
}

void sehandle(const LLCoros::callable_t& callable, const std::string& name)
{
    __try
    {
        // handle stop and continue exceptions first
        cpphandle(callable, name);
    }
    __except (exception_filter(GetExceptionCode(), GetExceptionInformation()))
    {
        // convert to C++ styled exception
        // Note: it might be better to use _se_set_translator
        // if you want exception to inherit full callstack
        char integer_string[512];
        sprintf(integer_string, "SEH, code: %lu\n", GetExceptionCode());
        throw std::exception(integer_string);
    }
}
#endif // LL_WINDOWS
} // anonymous namespace

// Top-level wrapper around caller's coroutine callable.
// Normally we like to pass strings and such by const reference -- but in this
// case, we WANT to copy both the name and the callable to our local stack!
void LLCoros::toplevel(std::string name, callable_t callable)
{
    // keep the CoroData on this top-level function's stack frame
    CoroData corodata(name);
    // set it as current
    mCurrent.reset(&corodata);

#ifdef LL_WINDOWS
    // can not use __try directly, toplevel requires unwinding, thus use of a wrapper
    sehandle(callable, name);
#else // LL_WINDOWS
    // run the code the caller actually wants in the coroutine
    try
    {
        callable();
    }
    catch (const Stop& exc)
    {
        LL_INFOS("LLCoros") << "coroutine " << name << " terminating because "
                            << exc.what() << LL_ENDL;
    }
    catch (const LLContinueError&)
    {
        // Any uncaught exception derived from LLContinueError will be caught
        // here and logged. This coroutine will terminate but the rest of the
        // viewer will carry on.
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << name));
    }
    catch (...)
    {
        // Stash any OTHER kind of uncaught exception in the rethrow() queue
        // to be rethrown by the main fiber.
        LL_WARNS("LLCoros") << "Capturing uncaught exception in coroutine "
                            << name << LL_ENDL;
        LLCoros::instance().saveException(name, std::current_exception());
    }
#endif // else LL_WINDOWS
}

//static
void LLCoros::checkStop()
{
    if (wasDeleted())
    {
        LLTHROW(Shutdown("LLCoros was deleted"));
    }
    // do this AFTER the check above, because getName() depends on
    // get_CoroData(), which depends on the local_ptr in our instance().
    if (getName().empty())
    {
        // Our Stop exception and its subclasses are intended to stop loitering
        // coroutines. Don't throw it from the main coroutine.
        return;
    }
    if (LLApp::isStopped())
    {
        LLTHROW(Stopped("viewer is stopped"));
    }
    if (! LLApp::isRunning())
    {
        LLTHROW(Stopping("viewer is stopping"));
    }
}

LLCoros::CoroData::CoroData(const std::string& name):
    LLInstanceTracker<CoroData, std::string>(name),
    mName(name),
    // don't consume events unless specifically directed
    mConsuming(false),
    mCreationTime(LLTimer::getTotalSeconds())
{
}

LLCoros::CoroData::CoroData(int n):
    // This constructor is used for the thread_local instance belonging to the
    // default coroutine on each thread. We must give each one a different
    // LLInstanceTracker key because LLInstanceTracker's map spans all
    // threads, but we want the default coroutine on each thread to have the
    // empty string as its visible name because some consumers test for that.
    LLInstanceTracker<CoroData, std::string>("main" + stringize(n)),
    mName(),
    mConsuming(false),
    mCreationTime(LLTimer::getTotalSeconds())
{
}
