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
#include "llerror.h"
#include "llevents.h"
#include "stringize.h"

LL::ThreadPool::ThreadPool(const std::string& name, size_t threads, size_t capacity):
    super(name),
    mQueue(name, capacity),
    mName("ThreadPool:" + name),
    mThreadCount(threads)
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
