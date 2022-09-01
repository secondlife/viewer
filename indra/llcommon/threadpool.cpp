/**
 * @file   threadpool.cpp
 * @author Nat Goodspeed
 * @date   2021-10-21
 * @brief  Implementation for threadpool.
 * 
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "threadpool.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "commoncontrol.h"
#include "llerror.h"
#include "llevents.h"
#include "llsd.h"
#include "stringize.h"

LL::ThreadPool::ThreadPool(const std::string& name, size_t threads, size_t capacity):
    super(name),
    mQueue(name, capacity),
    mName("ThreadPool:" + name),
    mThreadCount(getConfiguredWidth(name, threads))
{}

void LL::ThreadPool::start()
{
    for (size_t i = 0; i < mThreadCount; ++i)
    {
        std::string tname{ stringize(mName, ':', (i+1), '/', mThreadCount) };
        mThreads.emplace_back(tname, [this, tname]()
            {
                LL_PROFILER_SET_THREAD_NAME(tname.c_str());
                run(tname);
            });
    }
    // Listen on "LLApp", and when the app is shutting down, close the queue
    // and join the workers.
    LLEventPumps::instance().obtain("LLApp").listen(
        mName,
        [this](const LLSD& stat)
        {
            std::string status(stat["status"]);
            if (status != "running")
            {
                // viewer is starting shutdown -- proclaim the end is nigh!
                LL_DEBUGS("ThreadPool") << mName << " saw " << status << LL_ENDL;
                close();
            }
            return false;
        });
}

LL::ThreadPool::~ThreadPool()
{
    close();
}

void LL::ThreadPool::close()
{
    if (! mQueue.isClosed())
    {
        LL_DEBUGS("ThreadPool") << mName << " closing queue and joining threads" << LL_ENDL;
        mQueue.close();
        for (auto& pair: mThreads)
        {
            LL_DEBUGS("ThreadPool") << mName << " waiting on thread " << pair.first << LL_ENDL;
            pair.second.join();
        }
        LL_DEBUGS("ThreadPool") << mName << " shutdown complete" << LL_ENDL;
    }
}

void LL::ThreadPool::run(const std::string& name)
{
    LL_DEBUGS("ThreadPool") << name << " starting" << LL_ENDL;
    run();
    LL_DEBUGS("ThreadPool") << name << " stopping" << LL_ENDL;
}

void LL::ThreadPool::run()
{
    mQueue.runUntilClose();
}

//static
size_t LL::ThreadPool::getConfiguredWidth(const std::string& name, size_t dft)
{
    LLSD poolSizes;
    try
    {
        poolSizes = LL::CommonControl::get("Global", "ThreadPoolSizes");
        // "ThreadPoolSizes" is actually a map containing the sizes of
        // interest -- or should be, if this process has an
        // LLViewerControlListener instance and its settings include
        // "ThreadPoolSizes". If we failed to retrieve it, perhaps we're in a
        // program that doesn't define that, or perhaps there's no such
        // setting, or perhaps we're asking too early, before the LLEventAPI
        // itself has been instantiated. In any of those cases, it seems worth
        // warning.
        if (! poolSizes.isDefined())
        {
            // Note: we don't warn about absence of an override key for a
            // particular ThreadPool name, that's fine. This warning is about
            // complete absence of a ThreadPoolSizes setting, which we expect
            // in a normal viewer session.
            LL_WARNS("ThreadPool") << "No 'ThreadPoolSizes' setting for ThreadPool '"
                                   << name << "'" << LL_ENDL;
        }
    }
    catch (const LL::CommonControl::Error& exc)
    {
        // We don't want ThreadPool to *require* LLViewerControlListener.
        // Just log it and carry on.
        LL_WARNS("ThreadPool") << "Can't check 'ThreadPoolSizes': " << exc.what() << LL_ENDL;
    }

    LL_DEBUGS("ThreadPool") << "ThreadPoolSizes = " << poolSizes << LL_ENDL;
    // LLSD treats an undefined value as an empty map when asked to retrieve a
    // key, so we don't need this to be conditional.
    LLSD sizeSpec{ poolSizes[name] };
    // We retrieve sizeSpec as LLSD, rather than immediately as LLSD::Integer,
    // so we can distinguish the case when it's undefined.
    return sizeSpec.isInteger() ? sizeSpec.asInteger() : dft;
}

//static
size_t LL::ThreadPool::getWidth(const std::string& name, size_t dft)
{
    auto instance{ getInstance(name) };
    if (instance)
    {
        return instance->getWidth();
    }
    else
    {
        return getConfiguredWidth(name, dft);
    }
}
