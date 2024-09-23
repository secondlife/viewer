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

/*****************************************************************************
*   WorkQueueBase
*****************************************************************************/
LL::WorkQueueBase::WorkQueueBase(const std::string& name):
    super(makeName(name))
{
    // TODO: register for "LLApp" events so we can implicitly close() on
    // viewer shutdown.
}

void LL::WorkQueueBase::runUntilClose()
{
    try
    {
        for (;;)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
            callWork(pop_());
        }
    }
    catch (const Closed&)
    {
    }
}

bool LL::WorkQueueBase::runPending()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    for (Work work; tryPop_(work); )
    {
        callWork(work);
    }
    return ! done();
}

bool LL::WorkQueueBase::runOne()
{
    Work work;
    if (tryPop_(work))
    {
        callWork(work);
    }
    return ! done();
}

bool LL::WorkQueueBase::runUntil(const TimePoint& until)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    // Should we subtract some slop to allow for typical Work execution time?
    // How much slop?
    // runUntil() is simply a time-bounded runPending().
    for (Work work; TimePoint::clock::now() < until && tryPop_(work); )
    {
        callWork(work);
    }
    return ! done();
}

std::string LL::WorkQueueBase::makeName(const std::string& name)
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

void LL::WorkQueueBase::callWork(const Work& work)
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

void LL::WorkQueueBase::error(const std::string& msg)
{
    LL_ERRS("WorkQueue") << msg << LL_ENDL;
}

void LL::WorkQueueBase::checkCoroutine(const std::string& method)
{
    // By convention, the default coroutine on each thread has an empty name
    // string. See also LLCoros::logname().
    if (LLCoros::getName().empty())
    {
        LLTHROW(Error("Do not call " + method + " from a thread's default coroutine"));
    }
}

/*****************************************************************************
*   WorkQueue
*****************************************************************************/
LL::WorkQueue::WorkQueue(const std::string& name, size_t capacity):
    super(name),
    mQueue(capacity)
{
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

bool LL::WorkQueue::post(const Work& callable)
{
    return mQueue.pushIfOpen(callable);
}

bool LL::WorkQueue::tryPost(const Work& callable)
{
    return mQueue.tryPush(callable);
}

LL::WorkQueue::Work LL::WorkQueue::pop_()
{
    return mQueue.pop();
}

bool LL::WorkQueue::tryPop_(Work& work)
{
    return mQueue.tryPop(work);
}

/*****************************************************************************
*   WorkSchedule
*****************************************************************************/
LL::WorkSchedule::WorkSchedule(const std::string& name, size_t capacity):
    super(name),
    mQueue(capacity)
{
}

void LL::WorkSchedule::close()
{
    mQueue.close();
}

size_t LL::WorkSchedule::size()
{
    return mQueue.size();
}

bool LL::WorkSchedule::isClosed()
{
    return mQueue.isClosed();
}

bool LL::WorkSchedule::done()
{
    return mQueue.done();
}

bool LL::WorkSchedule::post(const Work& callable)
{
    // Use TimePoint::clock::now() instead of TimePoint's representation of
    // the epoch because this WorkSchedule may contain a mix of past-due
    // TimedWork items and TimedWork items scheduled for the future. Sift this
    // new item into the correct place.
    return post(callable, TimePoint::clock::now());
}

bool LL::WorkSchedule::post(const Work& callable, const TimePoint& time)
{
    return mQueue.pushIfOpen(TimedWork(time, callable));
}

bool LL::WorkSchedule::tryPost(const Work& callable)
{
    return tryPost(callable, TimePoint::clock::now());
}

bool LL::WorkSchedule::tryPost(const Work& callable, const TimePoint& time)
{
    return mQueue.tryPush(TimedWork(time, callable));
}

LL::WorkSchedule::Work LL::WorkSchedule::pop_()
{
    return std::get<0>(mQueue.pop());
}

bool LL::WorkSchedule::tryPop_(Work& work)
{
    return mQueue.tryPop(work);
}
