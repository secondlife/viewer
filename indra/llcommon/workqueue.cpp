/**
 * @file   workqueue.cpp
 * @author Nat Goodspeed
 * @date   2021-10-06
 * @brief  Implementation for WorkQueue.
 * 
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "workqueue.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llcoros.h"
#include LLCOROS_MUTEX_HEADER
#include "llerror.h"
#include "llexception.h"
#include "stringize.h"

using Mutex = LLCoros::Mutex;
using Lock  = LLCoros::LockType;

LL::WorkQueue::WorkQueue(const std::string& name, size_t capacity):
    super(makeName(name)),
    mQueue(capacity)
{
    // TODO: register for "LLApp" events so we can implicitly close() on
    // viewer shutdown.
}

void LL::WorkQueue::close()
{
    mQueue.close();
}

size_t LL::WorkQueue::size()
{
    return mQueue.size();
}

bool LL::WorkQueue::isClosed()
{
    return mQueue.isClosed();
}

bool LL::WorkQueue::done()
{
    return mQueue.done();
}

void LL::WorkQueue::runUntilClose()
{
    try
    {
        for (;;)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            callWork(mQueue.pop());
        }
    }
    catch (const Queue::Closed&)
    {
    }
}

bool LL::WorkQueue::runPending()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    for (Work work; mQueue.tryPop(work); )
    {
        callWork(work);
    }
    return ! mQueue.done();
}

bool LL::WorkQueue::runOne()
{
    Work work;
    if (mQueue.tryPop(work))
    {
        callWork(work);
    }
    return ! mQueue.done();
}

bool LL::WorkQueue::runUntil(const TimePoint& until)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    // Should we subtract some slop to allow for typical Work execution time?
    // How much slop?
    // runUntil() is simply a time-bounded runPending().
    for (Work work; TimePoint::clock::now() < until && mQueue.tryPop(work); )
    {
        callWork(work);
    }
    return ! mQueue.done();
}

std::string LL::WorkQueue::makeName(const std::string& name)
{
    if (! name.empty())
        return name;

    static U32 discriminator = 0;
    static Mutex mutex;
    U32 num;
    {
        // Protect discriminator from concurrent access by different threads.
        // It can't be thread_local, else two racing threads will come up with
        // the same name.
        Lock lk(mutex);
        num = discriminator++;
    }
    return STRINGIZE("WorkQueue" << num);
}

void LL::WorkQueue::callWork(const Queue::DataTuple& work)
{
    // ThreadSafeSchedule::pop() always delivers a tuple, even when
    // there's only one data field per item, as for us.
    callWork(std::get<0>(work));
}

void LL::WorkQueue::callWork(const Work& work)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    try
    {
        work();
    }
    catch (...)
    {
        // No matter what goes wrong with any individual work item, the worker
        // thread must go on! Log our own instance name with the exception.
        LOG_UNHANDLED_EXCEPTION(getKey());
    }
}

void LL::WorkQueue::error(const std::string& msg)
{
    LL_ERRS("WorkQueue") << msg << LL_ENDL;
}

void LL::WorkQueue::checkCoroutine(const std::string& method)
{
    // By convention, the default coroutine on each thread has an empty name
    // string. See also LLCoros::logname().
    if (LLCoros::getName().empty())
    {
        LLTHROW(Error("Do not call " + method + " from a thread's default coroutine"));
    }
}
