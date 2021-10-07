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
#include "llerror.h"
#include "llexception.h"
#include "stringize.h"

LL::WorkQueue::WorkQueue(const std::string& name):
    super(makeName(name))
{
    // TODO: register for "LLApp" events so we can implicitly close() on
    // viewer shutdown.
}

void LL::WorkQueue::close()
{
    mQueue.close();
}

void LL::WorkQueue::runUntilClose()
{
    try
    {
        for (;;)
        {
            callWork(mQueue.pop());
        }
    }
    catch (const Queue::Closed&)
    {
    }
}

bool LL::WorkQueue::runPending()
{
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
    // Should we subtract some slop to allow for typical Work execution time?
    // How much slop?
    Work work;
    while (TimePoint::clock::now() < until && mQueue.tryPopUntil(until, work))
    {
        callWork(work);
    }
    return ! mQueue.done();
}

std::string LL::WorkQueue::makeName(const std::string& name)
{
    if (! name.empty())
        return name;

    static thread_local U32 discriminator = 0;
    return STRINGIZE("WorkQueue" << discriminator++);
}

void LL::WorkQueue::callWork(const Queue::DataTuple& work)
{
    // ThreadSafeSchedule::pop() always delivers a tuple, even when
    // there's only one data field per item, as for us.
    callWork(std::get<0>(work));
}

void LL::WorkQueue::callWork(const Work& work)
{
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
