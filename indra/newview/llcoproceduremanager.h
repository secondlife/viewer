/**
* @file llcoproceduremanager.h
* @author Rider Linden
* @brief Singleton class for managing asset uploads to the sim.
*
* $LicenseInfo:firstyear=2015&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2015, Linden Research, Inc.
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

#ifndef LL_UPLOAD_MANAGER_H
#define LL_UPLOAD_MANAGER_H

#include "lleventcoro.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "lluuid.h"

class LLCoprocedureManager : public LLSingleton < LLCoprocedureManager >
{
public:
    typedef boost::function<void(LLCoros::self &, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &, const LLUUID &id)> CoProcedure_t;

    virtual ~LLCoprocedureManager();

    /// Places the coprocedure on the queue for processing. 
    /// 
    /// @param name Is used for debugging and should identify this coroutine.
    /// @param proc Is a bound function to be executed 
    /// 
    /// @return This method returns a UUID that can be used later to cancel execution.
    LLUUID enqueueCoprocedure(const std::string &name, CoProcedure_t proc);

    /// Cancel a coprocedure. If the coprocedure is already being actively executed 
    /// this method calls cancelYieldingOperation() on the associated HttpAdapter
    /// If it has not yet been dequeued it is simply removed from the queue.
    void cancelCoprocedure(const LLUUID &id);

    /// Requests a shutdown of the upload manager. Passing 'true' will perform 
    /// an immediate kill on the upload coroutine.
    void shutdown(bool hardShutdown = false);

    /// Returns the number of coprocedures in the queue awaiting processing.
    ///
    inline size_t countPending() const
    {
        return mPendingCoprocs.size();
    }

    /// Returns the number of coprocedures actively being processed.
    ///
    inline size_t countActive() const
    {
        return mActiveCoprocs.size();
    }

    /// Returns the total number of coprocedures either queued or in active processing.
    ///
    inline size_t count() const
    {
        return countPending() + countActive();
    }

protected:
    LLCoprocedureManager();

private:
    struct QueuedCoproc
    {
        typedef boost::shared_ptr<QueuedCoproc> ptr_t;

        QueuedCoproc(const std::string &name, const LLUUID &id, CoProcedure_t proc):
            mName(name),
            mId(id),
            mProc(proc)
        {}

        std::string mName;
        LLUUID mId;
        CoProcedure_t mProc;
    };
    
    // we use a deque here rather than std::queue since we want to be able to 
    // iterate through the queue and potentially erase an entry from the middle.
    typedef std::deque<QueuedCoproc::ptr_t>  CoprocQueue_t;  
    typedef std::map<LLUUID, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t> ActiveCoproc_t;

    CoprocQueue_t   mPendingCoprocs;
    ActiveCoproc_t  mActiveCoprocs;
    bool            mShutdown;
    LLEventStream   mWakeupTrigger;


    typedef std::map<std::string, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t> CoroAdapterMap_t;
    LLCore::HttpRequest::policy_t mHTTPPolicy;

    CoroAdapterMap_t mCoroMapping;

    void coprocedureInvokerCoro(LLCoros::self& self, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter);
};

#endif
