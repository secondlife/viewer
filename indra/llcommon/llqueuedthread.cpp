/**
 * @file llqueuedthread.cpp
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
#include "llqueuedthread.h"

#include <chrono>

#include "llstl.h"
#include "lltimer.h"    // ms_sleep()
#include "llmutex.h"

//============================================================================

// MAIN THREAD
LLQueuedThread::LLQueuedThread(const std::string& name, bool threaded, bool should_pause) :
    LLThread(name),
    mIdleThread(true),
    mNextHandle(0),
    mStarted(false),
    mThreaded(threaded),
    mRequestQueue(name, 1024 * 1024)
{
    llassert(threaded); // not threaded implementation is deprecated
    mMainQueue = LL::WorkQueue::getInstance("mainloop");

    if (mThreaded)
    {
        if(should_pause)
        {
            pause() ; //call this before start the thread.
        }

        start();
    }
}

// MAIN THREAD
LLQueuedThread::~LLQueuedThread()
{
    if (!mThreaded)
    {
        endThread();
    }
    shutdown();
    // ~LLThread() will be called here
}

void LLQueuedThread::shutdown()
{
    setQuitting();

    unpause(); // MAIN THREAD
    if (mThreaded)
    {
        if (mRequestQueue.size() == 0)
        {
            mRequestQueue.close();
        }

        S32 timeout = 50;
        for ( ; timeout>0; timeout--)
        {
            if (isStopped())
            {
                break;
            }
            ms_sleep(100);
            LLThread::yield();
        }
        if (timeout == 0)
        {
            LL_WARNS() << "~LLQueuedThread (" << mName << ") timed out!" << LL_ENDL;
        }
    }
    else
    {
        mStatus = STOPPED;
    }

    S32 queued_count = 0;
    bool  has_active = false;
    lockData();
    for (auto it = mRequestHash.begin(); it != mRequestHash.end(); )
    {
        QueuedRequest* req = it->second;
        if (req->getStatus() == STATUS_INPROGRESS)
        {
            has_active = true;
            req->setFlags(FLAG_ABORT | FLAG_AUTO_COMPLETE);
            ++it;
            continue;
        }
        if (req->getStatus() == STATUS_QUEUED)
        {
            ++queued_count;
            req->setStatus(STATUS_ABORTED); // avoid assert in deleteRequest
        }
        it = mRequestHash.erase(it);
        req->deleteRequest();
    }
    unlockData();
    if (queued_count)
    {
        LL_WARNS() << "~LLQueuedThread() called with unpocessed requests: " << queued_count << LL_ENDL;
    }
    if (has_active)
    {
        LL_WARNS() << "~LLQueuedThread() called with active requests!" << LL_ENDL;
        ms_sleep(100); // last chance for request to finish
        printQueueStats();
    }

    mRequestQueue.close();
}

//----------------------------------------------------------------------------

// MAIN THREAD
// virtual
size_t LLQueuedThread::update(F32 max_time_ms)
{
    LL_PROFILE_ZONE_SCOPED;
    if (!mStarted)
    {
        if (!mThreaded)
        {
            startThread();
            mStarted = true;
        }
    }
    return updateQueue(max_time_ms);
}

size_t LLQueuedThread::updateQueue(F32 max_time_ms)
{
    LL_PROFILE_ZONE_SCOPED;
    // Frame Update
    if (mThreaded)
    {
        // schedule a call to threadedUpdate for every call to updateQueue
        if (!isQuitting())
        {
            mRequestQueue.post([=, this]()
                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("qt - update");
                    mIdleThread = false;
                    threadedUpdate();
                    mIdleThread = true;
                }
            );
        }

        if(getPending() > 0)
        {
            unpause();
        }
    }
    else
    {
        mRequestQueue.runFor(std::chrono::microseconds((int) (max_time_ms*1000.f)));
        threadedUpdate();
    }
    return getPending();
}

void LLQueuedThread::incQueue()
{
    // Something has been added to the queue
    if (!isPaused())
    {
        if (mThreaded)
        {
            wake(); // Wake the thread up if necessary.
        }
    }
}

//virtual
// May be called from any thread
size_t LLQueuedThread::getPending()
{
    return mRequestQueue.size();
}

// MAIN thread
void LLQueuedThread::waitOnPending()
{
    while(1)
    {
        update(0);

        if (mIdleThread)
        {
            break;
        }
        if (mThreaded)
        {
            yield();
        }
    }
    return;
}

// MAIN thread
void LLQueuedThread::printQueueStats()
{
    auto size = mRequestQueue.size();
    if (size > 0)
    {
        LL_INFOS() << llformat("Pending Requests:%d ", mRequestQueue.size()) << LL_ENDL;
    }
    else
    {
        LL_INFOS() << "Queued Thread Idle" << LL_ENDL;
    }
}

// MAIN thread
LLQueuedThread::handle_t LLQueuedThread::generateHandle()
{
    U32 res = ++mNextHandle;
    return res;
}

// MAIN thread
bool LLQueuedThread::addRequest(QueuedRequest* req)
{
    LL_PROFILE_ZONE_SCOPED;
    if (mStatus == QUITTING)
    {
        return false;
    }

    lockData();
    req->setStatus(STATUS_QUEUED);
    mRequestHash.emplace(req->getHandle(), req);
#if _DEBUG
//  LL_INFOS() << llformat("LLQueuedThread::Added req [%08d]",handle) << LL_ENDL;
#endif
    unlockData();

    llassert(!mDataLock->isSelfLocked());
    mRequestQueue.post([this, req]() { processRequest(req); });

    return true;
}

// MAIN thread
bool LLQueuedThread::waitForResult(LLQueuedThread::handle_t handle, bool auto_complete)
{
    LL_PROFILE_ZONE_SCOPED;
    llassert (handle != nullHandle());
    bool res = false;
    bool waspaused = isPaused();
    bool done = false;
    while(!done)
    {
        update(0); // unpauses
        lockData();
        auto it = mRequestHash.find(handle);
        if (it == mRequestHash.end())
        {
            done = true; // request does not exist
        }
        else if (it->second->getStatus() == STATUS_COMPLETE)
        {
            res = true;
            if (auto_complete)
            {
                QueuedRequest* req = it->second;
                mRequestHash.erase(it);
                req->deleteRequest();
//              check();
            }
            done = true;
        }
        unlockData();

        if (!done && mThreaded)
        {
            yield();
        }
    }
    if (waspaused)
    {
        pause();
    }
    return res;
}

// MAIN thread
LLQueuedThread::QueuedRequest* LLQueuedThread::getRequest(handle_t handle)
{
    if (handle == nullHandle())
    {
        return 0;
    }
    lockData();
    auto it = mRequestHash.find(handle);
    QueuedRequest* res = (it != mRequestHash.end()) ? it->second : nullptr;
    unlockData();
    return res;
}

LLQueuedThread::status_t LLQueuedThread::getRequestStatus(handle_t handle)
{
    status_t res = STATUS_EXPIRED;
    lockData();
    auto it = mRequestHash.find(handle);
    if (it != mRequestHash.end())
    {
        res = it->second->getStatus();
    }
    unlockData();
    return res;
}

void LLQueuedThread::abortRequest(handle_t handle, bool autocomplete)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    lockData();
    auto it = mRequestHash.find(handle);
    if (it != mRequestHash.end())
    {
        it->second->setFlags(FLAG_ABORT | (autocomplete ? FLAG_AUTO_COMPLETE : 0));
    }
    unlockData();
}

// MAIN thread
void LLQueuedThread::setFlags(handle_t handle, U32 flags)
{
    lockData();
    auto it = mRequestHash.find(handle);
    if (it != mRequestHash.end())
    {
        it->second->setFlags(flags);
    }
    unlockData();
}

bool LLQueuedThread::completeRequest(handle_t handle)
{
    LL_PROFILE_ZONE_SCOPED;
    bool res = false;
    lockData();
    auto it = mRequestHash.find(handle);
    if (it != mRequestHash.end())
    {
        QueuedRequest* req = it->second;
        llassert_always(req->getStatus() != STATUS_QUEUED);
        llassert_always(req->getStatus() != STATUS_INPROGRESS);
#if _DEBUG
//      LL_INFOS() << llformat("LLQueuedThread::Completed req [%08d]",handle) << LL_ENDL;
#endif
        mRequestHash.erase(it);
        req->deleteRequest();
//      check();
        res = true;
    }
    unlockData();
    return res;
}

bool LLQueuedThread::check()
{
    return true;
}

//============================================================================
// Runs on its OWN thread

void LLQueuedThread::processRequest(LLQueuedThread::QueuedRequest* req)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;

    mIdleThread = false;
    //threadedUpdate();

    // Get next request from pool
    lockData();

    if ((req->getFlags() & FLAG_ABORT) || (mStatus == QUITTING))
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("qtpr - abort");
        req->setStatus(STATUS_ABORTED);
        req->finishRequest(false);
        if (req->getFlags() & FLAG_AUTO_COMPLETE)
        {
            mRequestHash.erase(req->getHandle());
            req->deleteRequest();
//              check();
        }
        unlockData();
    }
    else
    {
        llassert_always(req->getStatus() == STATUS_QUEUED);

        if (req)
        {
            req->setStatus(STATUS_INPROGRESS);
        }
        unlockData();

        // This is the only place we will call req->setStatus() after
        // it has initially been seet to STATUS_QUEUED, so it is
        // safe to access req.
        if (req)
        {
            // process request
            bool complete = req->processRequest();

            if (complete)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("qtpr - complete");
                lockData();
                req->setStatus(STATUS_COMPLETE);
                req->finishRequest(true);
                if (req->getFlags() & FLAG_AUTO_COMPLETE)
                {
                    mRequestHash.erase(req->getHandle());
                    req->deleteRequest();
                    //              check();
                }
                unlockData();
            }
            else
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("qtpr - retry");
                //put back on queue and try again in 0.1ms
                lockData();
                req->setStatus(STATUS_QUEUED);

                unlockData();

                llassert(!mDataLock->isSelfLocked());

                using namespace std::chrono_literals;
                auto retry_time = LL::WorkQueue::TimePoint::clock::now() + 16ms;
                mRequestQueue.post([=, this]
                    {
                        LL_PROFILE_ZONE_NAMED("processRequest - retry");
                        if (LL::WorkQueue::TimePoint::clock::now() < retry_time)
                        {
                            auto sleep_time = std::chrono::duration_cast<std::chrono::milliseconds>(retry_time - LL::WorkQueue::TimePoint::clock::now());

                            if (sleep_time.count() > 0)
                            {
                                ms_sleep((U32)sleep_time.count());
                            }
                        }
                        processRequest(req);
                    });

            }
        }
    }

    mIdleThread = true;
}

// virtual
bool LLQueuedThread::runCondition()
{
    // mRunCondition must be locked here
    if (mRequestQueue.size() == 0 && mIdleThread)
        return false;
    else
        return true;
}

// virtual
void LLQueuedThread::run()
{
    // call checPause() immediately so we don't try to do anything before the class is fully constructed
    checkPause();
    startThread();
    mStarted = true;

    /*while (1)
    {
        LL_PROFILE_ZONE_SCOPED;
        // this will block on the condition until runCondition() returns true, the thread is unpaused, or the thread leaves the RUNNING state.
        checkPause();

        mIdleThread = false;

        threadedUpdate();

        auto pending_work = processNextRequest();

        if (pending_work == 0)
        {
            //LL_PROFILE_ZONE_NAMED("LLQueuedThread - sleep");
            mIdleThread = true;
            //ms_sleep(1);
        }
        //LLThread::yield(); // thread should yield after each request
    }*/
    mRequestQueue.runUntilClose();

    endThread();
    LL_INFOS() << "LLQueuedThread " << mName << " EXITING." << LL_ENDL;

}

// virtual
void LLQueuedThread::startThread()
{
}

// virtual
void LLQueuedThread::endThread()
{
}

// virtual
void LLQueuedThread::threadedUpdate()
{
}

//============================================================================

LLQueuedThread::QueuedRequest::QueuedRequest(LLQueuedThread::handle_t handle, U32 flags) :
    mHandle(handle),
    mStatus(STATUS_UNKNOWN),
    mFlags(flags)
{
}

LLQueuedThread::QueuedRequest::~QueuedRequest()
{
    if (mStatus != STATUS_DELETE)
    {
        // The only method to delete a request is deleteRequest(),
        // it should have set the status to STATUS_DELETE
        LL_ERRS() << "LLQueuedThread::QueuedRequest deleted with status " << mStatus << LL_ENDL;
    }
}

//virtual
void LLQueuedThread::QueuedRequest::finishRequest(bool completed)
{
}

//virtual
void LLQueuedThread::QueuedRequest::deleteRequest()
{
    llassert_always(mStatus != STATUS_INPROGRESS);
    setStatus(STATUS_DELETE);
    delete this;
}
