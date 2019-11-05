/** 
 * @file llthreadpool.h
 * @brief
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

#ifndef LL_THREADPOOL_H
#define LL_THREADPOOL_H

#include "lldynamicpqueue.h"
#include <string>
#include <map>
#include <set>

#include "llsingleton.h"
#include "llatomic.h"

#include "llthread.h"
#include "llsimplehash.h"

//========================================================================

class LLThreadPool
{
    friend class PooledThread;
    LOG_CLASS(LLThreadPool);
	//--------------------------------------------------------------------
public:
    class ThreadRequest
    {
        friend class LLThreadPool;
        LOG_CLASS(ThreadRequest);
    public:
        typedef std::shared_ptr<ThreadRequest>  ptr_t;

        ThreadRequest(LLUUID id = LLUUID()) :
            mRequestId(id),
            mPoolController(nullptr)
        { }

        virtual ~ThreadRequest()                    {  };

        virtual LLUUID  getRequestId() const         { return mRequestId; }
        virtual bool    execute(U32 priority)       { return true; }
        virtual bool    preexecute(U32 priority)    { return true; }
        virtual void    postexecute(U32 priority)   {  }

    protected:
        LLUUID          queueRequest(const ThreadRequest::ptr_t &request, U32 priority);
        void            dropRequest(LLUUID request_id);
        size_t          requestCount() const;
        bool            checkRequest(LLUUID request_id) const;

        LLUUID          mRequestId;

        LLThreadPool *  mPoolController;
    };
    //--------------------------------------------------------------------

    LLThreadPool(const std::string name); 

    virtual ~LLThreadPool() 
    { }

    //--------------------------------------------------------------------
    std::string getPoolName() const { return mPoolName; }

protected:
    void        initSingleton_();
    void        cleanupSingleton_();

    void        startPool();
    void        clearThreadRequests();

    void        setPoolSize(U32 size) { mPoolSize = size; }
    U32         getPoolSize() const { return mPoolSize; }

    LLUUID      queueRequest(const ThreadRequest::ptr_t &request, U32 priority);
    void        dropRequest(LLUUID request_id);
    void        adjustRequest(LLUUID request_id, S32 adjustment);
    void        setRequest(LLUUID request_id, U32 priority);
    size_t      requestCount() const;
    bool        checkRequest(LLUUID request_id) const;

private:
    struct get_thread_request_id;
    typedef LLDynamicPriorityQueue<ThreadRequest::ptr_t, get_thread_request_id> request_queue_t;

    class PooledThread : public LLThread
    {
    public:
        typedef std::shared_ptr<PooledThread>   ptr_t;

        PooledThread(const std::string &name, LLCondition::uptr_t &queue_ready, request_queue_t &queue, LLThreadPool *owner);

        virtual void run() override; 

    private:
        LLThreadPool *          mOwningPool;
        LLCondition::uptr_t &   mQueueReady;
        request_queue_t &       mRequestQueue;
    };

    struct get_thread_request_id
    {
        LLUUID operator()(const ThreadRequest::ptr_t &item)  { return item->getRequestId(); }
    };

    typedef std::set<PooledThread::ptr_t>  thread_pool_t;

    std::string         mPoolName;
    U32                 mPoolSize;

    thread_pool_t       mPool;

    request_queue_t     mRequestQueue;
    LLCondition::uptr_t mQueueReady;
};

#endif // LL_LLQUEUEDTHREAD_H
