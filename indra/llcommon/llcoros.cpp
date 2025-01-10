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
#include "llerror.h"
#include "llevents.h"
#include "llexception.h"
#include "llsdutil.h"
#include "lltimer.h"
#include "stringize.h"
#include "scope_exit.h"

#if LL_WINDOWS
#include <excpt.h>
#endif

thread_local std::unordered_map<std::string, int> LLCoros::mPrefixMap;
thread_local std::unordered_map<std::string, LLCoros::id> LLCoros::mNameMap;

// static
bool LLCoros::on_main_coro()
{
    return (!instanceExists() || get_CoroData().isMain);
}

// static
bool LLCoros::on_main_thread_main_coro()
{
    return on_main_coro() && on_main_thread();
}

// static
LLCoros::CoroData& LLCoros::get_CoroData()
{
    CoroData* current{ nullptr };
    // be careful about attempted accesses in the final throes of app shutdown
    if (instanceExists())
    {
        current = instance().mCurrent.get();
    }
    // For the main() coroutine, the one NOT explicitly launched by launch(),
    // we never explicitly set mCurrent. Use a static CoroData instance with
    // canonical values.
    if (! current)
    {
        // We need not reset() the local_ptr to this instance; we'll simply
        // find it again every time we discover that current is null.
        current = &main_CoroData();
    }
    return *current;
}

LLCoros::CoroData& LLCoros::get_CoroData(id id)
{
    auto found = CoroData::getInstance(id);
    return found? *found : main_CoroData();
}

LLCoros::CoroData& LLCoros::main_CoroData()
{
    // tell CoroData we're "main"
    static thread_local CoroData sMain("");
    return sMain;
}

//static
LLCoros::coro::id LLCoros::get_self()
{
    return boost::this_fiber::get_id();
}

//static
void LLCoros::set_consuming(bool consuming)
{
    auto& data(get_CoroData());
    // DO NOT call this on the main() coroutine.
    llassert_always(! data.isMain);
    data.mConsuming = consuming;
}

//static
bool LLCoros::get_consuming()
{
    return get_CoroData().mConsuming;
}

// static
void LLCoros::setStatus(const std::string& status)
{
    get_CoroData().mStatus = status;
}

// static
std::string LLCoros::getStatus()
{
    return get_CoroData().mStatus;
}

LLCoros::LLCoros():
    // MAINT-2724: default coroutine stack size too small on Windows.
    // Previously we used
    // boost::context::guarded_stack_allocator::default_stacksize();
    // empirically this is insufficient.
    mStackSize(512*1024),
    // mCurrent does NOT own the current CoroData instance -- it simply
    // points to it. So initialize it with a no-op deleter.
    mCurrent{ [](CoroData*){} }
{
    auto& llapp{ LLEventPumps::instance().obtain("LLApp") };
    if (llapp.getListener("LLCoros") == LLBoundListener())
    {
        // chain our "LLCoros" pump onto "LLApp" pump: echo events posted to "LLApp"
        mConn = llapp.listen(
            "LLCoros",
            [](const LLSD& event)
            { return LLEventPumps::instance().obtain("LLCoros").post(event); });
    }
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
        // coroutines run once
        boost::this_fiber::yield();
    }
    printActiveCoroutines("after pumping");
}

std::string LLCoros::generateDistinctName(const std::string& prefix) const
{
    // Empty name would trigger CoroData's constructor's special case for the
    // main coroutine.
    if (prefix.empty())
    {
        LL_ERRS("LLCoros") << "LLCoros::launch(): pass non-empty name string" << LL_ENDL;
    }

    // If the specified name isn't already in the map, just use that.
    std::string name(prefix);
    // maintain a distinct int suffix for each prefix
    int& unique = mPrefixMap[prefix];

    // Until we find an unused name, append int suffix for uniqueness.
    while (mNameMap.find(name) != mNameMap.end())
    {
        name = stringize(prefix, unique++);
    }
    return name;
}

bool LLCoros::killreq(const std::string& name)
{
    auto foundName = mNameMap.find(name);
    if (foundName == mNameMap.end())
    {
        // couldn't find that name in map
        return false;
    }
    auto found = CoroData::getInstance(foundName->second);
    if (! found)
    {
        // found name, but CoroData with that ID key no longer exists
        return false;
    }
    // Next time the subject coroutine calls checkStop(), make it terminate.
    found->mKilledBy = getName();
    // But if it's waiting for something, notify anyone in a position to poke
    // it.
    LLEventPumps::instance().obtain("LLCoros").post(
        llsd::map("status", "killreq", "coro", name));
    return true;
}

//static
std::string LLCoros::getName()
{
    return get_CoroData().getName();
}

// static
std::string LLCoros::getName(id id)
{
    return get_CoroData(id).getName();
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
        for (const auto& cd : CoroData::instance_snapshot())
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
    // stack so that stack overflow will result in an access violation
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

// Top-level wrapper around caller's coroutine callable.
// Normally we like to pass strings and such by const reference -- but in this
// case, we WANT to copy both the name and the callable to our local stack!
void LLCoros::toplevel(std::string name, callable_t callable)
{
    // keep the CoroData on this top-level function's stack frame
    CoroData corodata(name);
    // set it as current
    mCurrent.reset(&corodata);

    LL_DEBUGS("LLCoros") << "entering " << name << LL_ENDL;
    // run the code the caller actually wants in the coroutine
    try
    {
        LL::scope_exit report{
            [&corodata]
            {
                bool allzero = true;
                for (const auto& [threshold, occurs] : corodata.mHistogram)
                {
                    if (occurs)
                    {
                        allzero = false;
                        break;
                    }
                }
                if (! allzero)
                {
                    LL_WARNS("LLCoros") << "coroutine " << corodata.mName;
                    const char* sep = " exceeded ";
                    for (const auto& [threshold, occurs] : corodata.mHistogram)
                    {
                        if (occurs)
                        {
                            LL_CONT << sep << threshold << " " << occurs << " times";
                            sep = ", ";
                        }
                    }
                    LL_ENDL;
                }
            }};
        LL::seh::catcher(callable);
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
        LOG_UNHANDLED_EXCEPTION("coroutine " + name);
    }
    catch (...)
    {
        // Stash any OTHER kind of uncaught exception in the rethrow() queue
        // to be rethrown by the main fiber.
        LL_WARNS("LLCoros") << "Capturing uncaught exception in coroutine "
                            << name << LL_ENDL;
        LLCoros::instance().saveException(name, std::current_exception());
    }
}

//static
void LLCoros::checkStop(callable_t cleanup)
{
    // don't replicate this 'if' test throughout the code below
    if (! cleanup)
    {
        cleanup = {[](){}};         // hey, look, I'm coding in Haskell!
    }

    if (wasDeleted())
    {
        cleanup();
        LLTHROW(Shutdown("LLCoros was deleted"));
    }

    // do this AFTER the check above, because get_CoroData() depends on the
    // local_ptr in our instance().
    auto& data(get_CoroData());
    if (data.isMain)
    {
        // Our Stop exception and its subclasses are intended to stop loitering
        // coroutines. Don't throw it from the main coroutine.
        return;
    }
    if (LLApp::isStopped())
    {
        cleanup();
        LLTHROW(Stopped("viewer is stopped"));
    }
    if (! LLApp::isRunning())
    {
        cleanup();
        LLTHROW(Stopping("viewer is stopping"));
    }
    if (! data.mKilledBy.empty())
    {
        // Someone wants to kill this coroutine
        cleanup();
        LLTHROW(Killed(stringize("coroutine ", data.getName(), " killed by ", data.mKilledBy)));
    }
}

LLBoundListener LLCoros::getStopListener(const std::string& caller, LLVoidListener cleanup)
{
    if (! cleanup)
        return {};

    // This overload only responds to viewer shutdown.
    return LLEventPumps::instance().obtain("LLCoros")
        .listen(
            LLEventPump::inventName(caller),
            [cleanup](const LLSD& event)
            {
                auto status{ event["status"].asString() };
                if (status != "running" && status != "killreq")
                {
                    cleanup(event);
                }
                return false;
            });
}

LLBoundListener LLCoros::getStopListener(const std::string& caller,
                                           const std::string& cnsmr,
                                           LLVoidListener cleanup)
{
    if (! cleanup)
        return {};

    std::string consumer{cnsmr};
    if (consumer.empty())
    {
        consumer = getName();
    }

    // This overload responds to viewer shutdown and to killreq(consumer).
    return LLEventPumps::instance().obtain("LLCoros")
        .listen(
            caller,
            [consumer, cleanup](const LLSD& event)
            {
                auto status{ event["status"].asString() };
                if (status == "killreq")
                {
                    if (event["coro"].asString() == consumer)
                    {
                        cleanup(event);
                    }
                }
                else if (status != "running")
                {
                    cleanup(event);
                }
                return false;
            });
}

LLCoros::CoroData::CoroData(const std::string& name):
    super(boost::this_fiber::get_id()),
    mName(name),
    mCreationTime(LLTimer::getTotalSeconds()),
    // Preset threshold times in mHistogram
    mHistogram{
        {0.004, 0},
        {0.040, 0},
        {0.400, 0},
        {1.000, 0}
    }
{
    // we expect the empty string for the main coroutine
    if (name.empty())
    {
        isMain = true;
        if (on_main_thread())
        {
            // main coroutine on main thread
            mName = "main";
        }
        else
        {
            // main coroutine on some other thread
            static std::atomic<int> main_no{ 0 };
            mName = stringize("main", ++main_no);
        }
    }
    // maintain LLCoros::mNameMap
    LLCoros::mNameMap.emplace(mName, getKey());
}

LLCoros::CoroData::~CoroData()
{
    // Don't try to erase the static main CoroData from our static
    // thread_local mNameMap; that could run into destruction order problems.
    if (! isMain)
    {
        LLCoros::mNameMap.erase(mName);
    }
}

std::string LLCoros::CoroData::getName() const
{
    if (mStatus.empty())
        return mName;
    else
        return stringize(mName, " (", mStatus, ")");
}
