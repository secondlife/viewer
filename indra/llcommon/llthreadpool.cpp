/** 
 * @file llthreadpool.cpp
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "linden_common.h"
#include "llthreadpool.h"

#include "llstl.h"
#include "lltimer.h"	// ms_sleep()
#include "lltracethreadrecorder.h"

//============================================================================
namespace
{
    class ThreadStopRequest : public LLThreadPool::ThreadRequest
    {
    public:
        virtual LLUUID getRequestId() const override
        {
            return STOP_REQUEST_ID;
        }

        virtual bool execute(U32 /*priority*/) override
        {
            return false;
        }

        static const U32    STOP_REQUEST_PRIORITY;
        static const LLUUID STOP_REQUEST_ID;
    };

    const U32    ThreadStopRequest::STOP_REQUEST_PRIORITY(0xFFFFFFFF);
    const LLUUID ThreadStopRequest::STOP_REQUEST_ID("00000000-0000-0000-000000000000"); /*TODO generate a UUID*/
}

//============================================================================
LLThreadPool::LLThreadPool(const std::string name) :
    mPoolName(name),
    mPoolSize(2),
    mPool(),
    mRequestQueue(true),
    mQueueReady()
{ 
}

void LLThreadPool::initSingleton_()
{
    LL_INFOS("THREADPOOL") << "Initializing thread pool \"" << mPoolName << "\" with " << mPoolSize << " workers." << LL_ENDL;

    mQueueReady.reset(new LLCondition());

    for (S32 idx = 0; idx < mPoolSize; ++idx)
    {
        std::stringstream thread_name;
        thread_name << mPoolName << "#" << idx;

        LL_INFOS("THREADPOOL") << "Creating pool thread \"" << thread_name.str() << "\"" << LL_ENDL;
        PooledThread::ptr_t thread(std::make_shared<PooledThread>(thread_name.str(), mQueueReady, mRequestQueue, this));

        mPool.insert(thread);
    }
}

void LLThreadPool::cleanupSingleton_()
{
    LL_DEBUGS("THREADPOOL") << "Cleanup on thread pool \"" << mPoolName << "\"" << LL_ENDL;

    mRequestQueue.enqueue(std::make_shared<ThreadStopRequest>(), ThreadStopRequest::STOP_REQUEST_PRIORITY);
    mQueueReady->broadcast();

    // Sleep for a moment give things a chance to exit.
    ms_sleep(100 * mPoolSize);

    for (PooledThread::ptr_t thread : mPool)
    {
        thread->shutdown(); /*TODO: I should look at this and see if I can make it async.  The stop request above should stop everyone... but this cleans up*/
    }

    for (PooledThread::ptr_t thread : mPool)
    {
        if (!thread->isStopped())
        {
            LL_WARNS("THREADPOOL") << "Thread failed to stop!" << LL_ENDL;
        }
    }

    mPool.clear();
    mQueueReady.reset();
}

void LLThreadPool::startPool()
{
    for (auto thread : mPool)
    {
        thread->start();
    }
}

void LLThreadPool::clearThreadRequests()
{
    mRequestQueue.clear();
}

LLUUID LLThreadPool::queueRequest(const ThreadRequest::ptr_t &request, U32 priority)
{
    LLUUID result;
    result = mRequestQueue.enqueue(request, priority);
    mQueueReady->signal();

    return result;
}

void LLThreadPool::dropRequest(LLUUID request_id)
{
    mRequestQueue.lock();
    if (mRequestQueue.isQueued(request_id))
    {
        mRequestQueue.remove(request_id);
    }
    else
    {
        /*TODO*/ // track executing requests so that we can signal them to quit. 
    }
    mRequestQueue.unlock();
}

void LLThreadPool::adjustRequest(LLUUID request_id, S32 adjustment)
{
    mRequestQueue.lock();
    if (mRequestQueue.isQueued(request_id))
    {
        mRequestQueue.priorityAdjust(request_id, adjustment);
    }
    else
    {
        /*TODO*/ // track executing requests so that we can signal them to quit. 
    }
    mRequestQueue.unlock();
}

size_t LLThreadPool::requestCount() const
{
    return mRequestQueue.size();
}

bool LLThreadPool::checkRequest(LLUUID request_id) const
{
    return mRequestQueue.isQueued(request_id);
}

//============================================================================
LLThreadPool::PooledThread::PooledThread(std::string &name, LLCondition::uptr_t &queue_ready, request_queue_t &queue, LLThreadPool *owner) :
    LLThread(name),
    mQueueReady(queue_ready),
    mRequestQueue(queue),
    mOwningPool(owner)
{
}

//-----------------------------------------------------------------------------
void LLThreadPool::PooledThread::run()
{
    LL_INFOS("THREADPOOL") << "Pooled thread \"" << mName << "\" starting." << LL_ENDL;
    do 
    {
        if (mRequestQueue.empty())
        {   // nothing to do.. wait to get poked.
            mQueueReady->wait();
        }
        //_INFOS("THREADPOOL") << "Pooled thread \"" << mName << "\" may have a job..." << LL_ENDL;

        mRequestQueue.lock();   // We do not want the queue changing underneath us.
        ThreadRequest::ptr_t request = mRequestQueue.top();
        if (!request)
        {   // An empty request.
            mRequestQueue.pop(); //  get it out of there before we continue. 
            mRequestQueue.unlock();
            continue;
        }
        if (request->getRequestId() == ThreadStopRequest::STOP_REQUEST_ID)
        {   // note that we break out here before popping so that the top request stays on the head of the queue.
            mRequestQueue.unlock();
            break;
        }
        U32 priority = mRequestQueue.topPriority();
        mRequestQueue.pop();
        mRequestQueue.unlock();

        /*TODO*/ // make a note that we are executing
        request->mPoolController = mOwningPool;
        if (request->preexecute(priority))
        {
            if (request->execute(priority))
            {
                request->postexecute(priority);
            }

        }
        request->mPoolController = nullptr;

        /*TODO*/ // make a note that we are no longer executing

    } while (true);

    LL_INFOS("THREADPOOL") << "Pooled thread \"" << mName << "\" stopping." << LL_ENDL;
}

//============================================================================
LLUUID LLThreadPool::ThreadRequest::queueRequest(const ThreadRequest::ptr_t &request, U32 priority)
{
    if (!mPoolController)
        return LLUUID();
    return mPoolController->queueRequest(request, priority);
}

void LLThreadPool::ThreadRequest::dropRequest(LLUUID request_id)
{
    if (!mPoolController)
        return;
    mPoolController->dropRequest(request_id);
}

size_t LLThreadPool::ThreadRequest::requestCount() const
{
    if (!mPoolController)
        return 0;
    return mPoolController->requestCount();
}

bool LLThreadPool::ThreadRequest::checkRequest(LLUUID request_id) const
{
    if (!mPoolController)
        return 0;
    return mPoolController->checkRequest(request_id);
}
