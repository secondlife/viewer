/**
* @file LLCoprocedurePool.cpp
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

#include "linden_common.h" 
#include "llcoproceduremanager.h"
#include "llexception.h"
#include "stringize.h"
#include <boost/assign.hpp>

//=========================================================================
// Map of pool sizes for known pools
// *TODO$: When C++11 this can be initialized here as follows:
// = {{"AIS", 25}, {"Upload", 1}}
static std::map<std::string, U32> DefaultPoolSizes = 
    boost::assign::map_list_of
        (std::string("Upload"),  1)
        (std::string("AIS"),     1);    
        // *TODO: Rider for the moment keep AIS calls serialized otherwise the COF will tend to get out of sync.

#define DEFAULT_POOL_SIZE 5

//=========================================================================
class LLCoprocedurePool: private boost::noncopyable
{
public:
    typedef LLCoprocedureManager::CoProcedure_t CoProcedure_t;

    LLCoprocedurePool(const std::string &name, size_t size);
    virtual ~LLCoprocedurePool();

    /// Places the coprocedure on the queue for processing. 
    /// 
    /// @param name Is used for debugging and should identify this coroutine.
    /// @param proc Is a bound function to be executed 
    /// 
    /// @return This method returns a UUID that can be used later to cancel execution.
    LLUUID enqueueCoprocedure(const std::string &name, CoProcedure_t proc);

    /// Cancel a coprocedure. If the coprocedure is already being actively executed 
    /// this method calls cancelSuspendedOperation() on the associated HttpAdapter
    /// If it has not yet been dequeued it is simply removed from the queue.
    bool cancelCoprocedure(const LLUUID &id);

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

private:
    struct QueuedCoproc
    {
        typedef boost::shared_ptr<QueuedCoproc> ptr_t;

        QueuedCoproc(const std::string &name, const LLUUID &id, CoProcedure_t proc) :
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

    std::string     mPoolName;
    size_t          mPoolSize;
    CoprocQueue_t   mPendingCoprocs;
    ActiveCoproc_t  mActiveCoprocs;
    bool            mShutdown;
    LLEventStream   mWakeupTrigger;

    typedef std::map<std::string, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t> CoroAdapterMap_t;
    LLCore::HttpRequest::policy_t mHTTPPolicy;

    CoroAdapterMap_t mCoroMapping;

    void coprocedureInvokerCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter);

};

//=========================================================================
LLCoprocedureManager::LLCoprocedureManager()
{
}

LLCoprocedureManager::~LLCoprocedureManager()
{

}

LLCoprocedureManager::poolPtr_t LLCoprocedureManager::initializePool(const std::string &poolName)
{
    // Attempt to look up a pool size in the configuration.  If found use that
    std::string keyName = "PoolSize" + poolName;
    int size = 0;

    if (poolName.empty())
        LL_ERRS("CoprocedureManager") << "Poolname must not be empty" << LL_ENDL;

    if (mPropertyQueryFn && !mPropertyQueryFn.empty())
    {
        size = mPropertyQueryFn(keyName);
    }

    if (size == 0)
    {   // if not found grab the know default... if there is no known 
        // default use a reasonable number like 5.
        std::map<std::string, U32>::iterator it = DefaultPoolSizes.find(poolName);
        if (it == DefaultPoolSizes.end())
            size = DEFAULT_POOL_SIZE;
        else
            size = (*it).second;

        if (mPropertyDefineFn && !mPropertyDefineFn.empty())
            mPropertyDefineFn(keyName, size, "Coroutine Pool size for " + poolName);
        LL_WARNS() << "LLCoprocedureManager: No setting for \"" << keyName << "\" setting pool size to default of " << size << LL_ENDL;
    }

    poolPtr_t pool(new LLCoprocedurePool(poolName, size));
    mPoolMap.insert(poolMap_t::value_type(poolName, pool));

    if (!pool)
        LL_ERRS("CoprocedureManager") << "Unable to create pool named \"" << poolName << "\" FATAL!" << LL_ENDL;
    return pool;
}

//-------------------------------------------------------------------------
LLUUID LLCoprocedureManager::enqueueCoprocedure(const std::string &pool, const std::string &name, CoProcedure_t proc)
{
    // Attempt to find the pool and enqueue the procedure.  If the pool does 
    // not exist, create it.
    poolPtr_t targetPool;
    poolMap_t::iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
    {
        targetPool = initializePool(pool);
    }
    else
    {
        targetPool = (*it).second;
    }

    return targetPool->enqueueCoprocedure(name, proc);
}

void LLCoprocedureManager::cancelCoprocedure(const LLUUID &id)
{
    for (poolMap_t::const_iterator it = mPoolMap.begin(); it != mPoolMap.end(); ++it)
    {
        if ((*it).second->cancelCoprocedure(id))
            return;
    }
    LL_INFOS() << "Coprocedure not found." << LL_ENDL;
}

void LLCoprocedureManager::shutdown(bool hardShutdown)
{
    for (poolMap_t::const_iterator it = mPoolMap.begin(); it != mPoolMap.end(); ++it)
    {
        (*it).second->shutdown(hardShutdown);
    }
    mPoolMap.clear();
}

void LLCoprocedureManager::setPropertyMethods(SettingQuery_t queryfn, SettingUpdate_t updatefn)
{
    mPropertyQueryFn = queryfn;
    mPropertyDefineFn = updatefn;
}

//-------------------------------------------------------------------------
size_t LLCoprocedureManager::countPending() const
{
    size_t count = 0;
    for (poolMap_t::const_iterator it = mPoolMap.begin(); it != mPoolMap.end(); ++it)
    {
        count += (*it).second->countPending();
    }
    return count;
}

size_t LLCoprocedureManager::countPending(const std::string &pool) const
{
    poolMap_t::const_iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
        return 0;
    return (*it).second->countPending();
}

size_t LLCoprocedureManager::countActive() const
{
    size_t count = 0;
    for (poolMap_t::const_iterator it = mPoolMap.begin(); it != mPoolMap.end(); ++it)
    {
        count += (*it).second->countActive();
    }
    return count;
}

size_t LLCoprocedureManager::countActive(const std::string &pool) const
{
    poolMap_t::const_iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
        return 0;
    return (*it).second->countActive();
}

size_t LLCoprocedureManager::count() const
{
    size_t count = 0;
    for (poolMap_t::const_iterator it = mPoolMap.begin(); it != mPoolMap.end(); ++it)
    {
        count += (*it).second->count();
    }
    return count;
}

size_t LLCoprocedureManager::count(const std::string &pool) const
{
    poolMap_t::const_iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
        return 0;
    return (*it).second->count();
}

//=========================================================================
LLCoprocedurePool::LLCoprocedurePool(const std::string &poolName, size_t size):
    mPoolName(poolName),
    mPoolSize(size),
    mPendingCoprocs(),
    mShutdown(false),
    mWakeupTrigger("CoprocedurePool" + poolName, true),
    mCoroMapping(),
    mHTTPPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID)
{
    for (size_t count = 0; count < mPoolSize; ++count)
    {
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter( mPoolName + "Adapter", mHTTPPolicy));

        std::string pooledCoro = LLCoros::instance().launch("LLCoprocedurePool("+mPoolName+")::coprocedureInvokerCoro",
            boost::bind(&LLCoprocedurePool::coprocedureInvokerCoro, this, httpAdapter));

        mCoroMapping.insert(CoroAdapterMap_t::value_type(pooledCoro, httpAdapter));
    }

    LL_INFOS() << "Created coprocedure pool named \"" << mPoolName << "\" with " << size << " items." << LL_ENDL;

    mWakeupTrigger.post(LLSD());
}

LLCoprocedurePool::~LLCoprocedurePool() 
{
    shutdown();
}

//-------------------------------------------------------------------------
void LLCoprocedurePool::shutdown(bool hardShutdown)
{
    CoroAdapterMap_t::iterator it;

    for (it = mCoroMapping.begin(); it != mCoroMapping.end(); ++it)
    {
        if (hardShutdown)
        {
            LLCoros::instance().kill((*it).first);
        }
        if ((*it).second)
        {
            (*it).second->cancelSuspendedOperation();
        }
    }

    mShutdown = true;
    mCoroMapping.clear();
    mPendingCoprocs.clear();
}

//-------------------------------------------------------------------------
LLUUID LLCoprocedurePool::enqueueCoprocedure(const std::string &name, LLCoprocedurePool::CoProcedure_t proc)
{
    LLUUID id(LLUUID::generateNewID());

    mPendingCoprocs.push_back(QueuedCoproc::ptr_t(new QueuedCoproc(name, id, proc)));
    LL_INFOS() << "Coprocedure(" << name << ") enqueued with id=" << id.asString() << " in pool \"" << mPoolName << "\"" << LL_ENDL;

    mWakeupTrigger.post(LLSD());

    return id;
}

bool LLCoprocedurePool::cancelCoprocedure(const LLUUID &id)
{
    // first check the active coroutines.  If there, remove it and return.
    ActiveCoproc_t::iterator itActive = mActiveCoprocs.find(id);
    if (itActive != mActiveCoprocs.end())
    {
        LL_INFOS() << "Found and canceling active coprocedure with id=" << id.asString() << " in pool \"" << mPoolName << "\"" << LL_ENDL;
        (*itActive).second->cancelSuspendedOperation();
        mActiveCoprocs.erase(itActive);
        return true;
    }

    for (CoprocQueue_t::iterator it = mPendingCoprocs.begin(); it != mPendingCoprocs.end(); ++it)
    {
        if ((*it)->mId == id)
        {
            LL_INFOS() << "Found and removing queued coroutine(" << (*it)->mName << ") with Id=" << id.asString() << " in pool \"" << mPoolName << "\"" << LL_ENDL;
            mPendingCoprocs.erase(it);
            return true;
        }
    }

    LL_INFOS() << "Coprocedure with Id=" << id.asString() << " was not found in pool \"" << mPoolName << "\"" << LL_ENDL;
    return false;
}

//-------------------------------------------------------------------------
void LLCoprocedurePool::coprocedureInvokerCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter)
{
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    while (!mShutdown)
    {
        llcoro::suspendUntilEventOn(mWakeupTrigger);
        if (mShutdown)
            break;
        
        while (!mPendingCoprocs.empty())
        {
            QueuedCoproc::ptr_t coproc = mPendingCoprocs.front();
            mPendingCoprocs.pop_front();
            ActiveCoproc_t::iterator itActive = mActiveCoprocs.insert(ActiveCoproc_t::value_type(coproc->mId, httpAdapter)).first;

            LL_INFOS() << "Dequeued and invoking coprocedure(" << coproc->mName << ") with id=" << coproc->mId.asString() << " in pool \"" << mPoolName << "\"" << LL_ENDL;

            try
            {
                coproc->mProc(httpAdapter, coproc->mId);
            }
            catch (...)
            {
                LOG_UNHANDLED_EXCEPTION(STRINGIZE("Coprocedure('" << coproc->mName
                                                  << "', id=" << coproc->mId.asString()
                                                  << ") in pool '" << mPoolName << "'"));
                // must NOT omit this or we deplete the pool
                mActiveCoprocs.erase(itActive);
                throw;
            }

            LL_INFOS() << "Finished coprocedure(" << coproc->mName << ")" << " in pool \"" << mPoolName << "\"" << LL_ENDL;

            mActiveCoprocs.erase(itActive);
        }
    }
}
