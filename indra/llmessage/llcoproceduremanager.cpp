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

#include "llwin32headers.h"

#include "linden_common.h"

#include "llcoproceduremanager.h"

#include <chrono>

#include <boost/assign.hpp>
#include <boost/fiber/buffered_channel.hpp>

#include "llexception.h"
#include "stringize.h"

//=========================================================================
// Map of pool sizes for known pools
static const std::map<std::string, U32> DefaultPoolSizes{
	{std::string("Upload"),  1},
    {std::string("AIS"),     1},
    // *TODO: Rider for the moment keep AIS calls serialized otherwise the COF will tend to get out of sync.
};

static const U32 DEFAULT_POOL_SIZE = 5;
static const U32 DEFAULT_QUEUE_SIZE = 4096;

//=========================================================================
class LLCoprocedurePool: private boost::noncopyable
{
public:
    typedef LLCoprocedureManager::CoProcedure_t CoProcedure_t;

    LLCoprocedurePool(const std::string &name, size_t size);
    ~LLCoprocedurePool();

    /// Places the coprocedure on the queue for processing. 
    /// 
    /// @param name Is used for debugging and should identify this coroutine.
    /// @param proc Is a bound function to be executed 
    /// 
    /// @return This method returns a UUID that can be used later to cancel execution.
    LLUUID enqueueCoprocedure(const std::string &name, CoProcedure_t proc);

    /// Returns the number of coprocedures actively being processed.
    ///
    inline size_t countActive() const
    {
        return mActiveCoprocs.size();
    }

    void close();
    
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

    // we use a buffered_channel here rather than unbuffered_channel since we want to be able to 
    // push values without blocking,even if there's currently no one calling a pop operation (due to
	// fibber running right now)
    typedef boost::fibers::buffered_channel<QueuedCoproc::ptr_t>  CoprocQueue_t;
    typedef std::map<LLUUID, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t> ActiveCoproc_t;

    std::string     mPoolName;
    size_t          mPoolSize;
    CoprocQueue_t   mPendingCoprocs;
    ActiveCoproc_t  mActiveCoprocs;
    LLTempBoundListener mStatusListener;

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
    for(auto & poolEntry : mPoolMap)
    {
        poolEntry.second->close();
    }
}

LLCoprocedureManager::poolPtr_t LLCoprocedureManager::initializePool(const std::string &poolName)
{
    // Attempt to look up a pool size in the configuration.  If found use that
    std::string keyName = "PoolSize" + poolName;
    int size = 0;

    LL_ERRS_IF(poolName.empty(), "CoprocedureManager") << "Poolname must not be empty" << LL_ENDL;

    if (mPropertyQueryFn)
    {
        size = mPropertyQueryFn(keyName);
    }

    if (size == 0)
    {
        // if not found grab the know default... if there is no known 
        // default use a reasonable number like 5.
        auto it = DefaultPoolSizes.find(poolName);
        size = (it != DefaultPoolSizes.end()) ? it->second : DEFAULT_POOL_SIZE;

        if (mPropertyDefineFn)
        {
            mPropertyDefineFn(keyName, size, "Coroutine Pool size for " + poolName);
        }

        LL_WARNS("CoProcMgr") << "LLCoprocedureManager: No setting for \"" << keyName << "\" setting pool size to default of " << size << LL_ENDL;
    }

    poolPtr_t pool(new LLCoprocedurePool(poolName, size));
    LL_ERRS_IF(!pool, "CoprocedureManager") << "Unable to create pool named \"" << poolName << "\" FATAL!" << LL_ENDL;

    bool inserted = mPoolMap.emplace(poolName, pool).second;
    LL_ERRS_IF(!inserted, "CoprocedureManager") << "Unable to add pool named \"" << poolName << "\" to map. FATAL!" << LL_ENDL;

    return pool;
}

//-------------------------------------------------------------------------
LLUUID LLCoprocedureManager::enqueueCoprocedure(const std::string &pool, const std::string &name, CoProcedure_t proc)
{
    // Attempt to find the pool and enqueue the procedure.  If the pool does 
    // not exist, create it.
    poolMap_t::iterator it = mPoolMap.find(pool);

    poolPtr_t targetPool = (it != mPoolMap.end()) ? it->second : initializePool(pool);

    return targetPool->enqueueCoprocedure(name, proc);
}

void LLCoprocedureManager::setPropertyMethods(SettingQuery_t queryfn, SettingUpdate_t updatefn)
{
    mPropertyQueryFn = queryfn;
    mPropertyDefineFn = updatefn;
}

size_t LLCoprocedureManager::countActive() const
{
    size_t count = 0;
    for (poolMap_t::const_iterator it = mPoolMap.begin(); it != mPoolMap.end(); ++it)
    {
        count += it->second->countActive();
    }
    return count;
}

size_t LLCoprocedureManager::countActive(const std::string &pool) const
{
    poolMap_t::const_iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
    {
        return 0;
    }
    return it->second->countActive();
}

void LLCoprocedureManager::close(const std::string &pool)
{
    poolMap_t::iterator it = mPoolMap.find(pool);
    if (it != mPoolMap.end())
    {
        it->second->close();
    }
}

//=========================================================================
LLCoprocedurePool::LLCoprocedurePool(const std::string &poolName, size_t size):
    mPoolName(poolName),
    mPoolSize(size),
    mPendingCoprocs(DEFAULT_QUEUE_SIZE),
    mHTTPPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID),
    mCoroMapping()
{
    // store in our LLTempBoundListener so that when the LLCoprocedurePool is
    // destroyed, we implicitly disconnect from this LLEventPump
    mStatusListener = LLEventPumps::instance().obtain("LLApp").listen(
        poolName,
        [this, poolName](const LLSD& status)
        {
            auto& statsd = status["status"];
            if (statsd.asString() != "running")
            {
                LL_INFOS("CoProcMgr") << "Pool " << poolName
                                      << " closing queue because status " << statsd
                                      << LL_ENDL;
                // This should ensure that all waiting coprocedures in this
                // pool will wake up and terminate.
                mPendingCoprocs.close();
            }
            return false;
        });

    for (size_t count = 0; count < mPoolSize; ++count)
    {
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter( mPoolName + "Adapter", mHTTPPolicy));

        std::string pooledCoro = LLCoros::instance().launch("LLCoprocedurePool("+mPoolName+")::coprocedureInvokerCoro",
            boost::bind(&LLCoprocedurePool::coprocedureInvokerCoro, this, httpAdapter));

        mCoroMapping.insert(CoroAdapterMap_t::value_type(pooledCoro, httpAdapter));
    }

    LL_INFOS("CoProcMgr") << "Created coprocedure pool named \"" << mPoolName << "\" with " << size << " items." << LL_ENDL;
}

LLCoprocedurePool::~LLCoprocedurePool() 
{
}

//-------------------------------------------------------------------------
LLUUID LLCoprocedurePool::enqueueCoprocedure(const std::string &name, LLCoprocedurePool::CoProcedure_t proc)
{
    LLUUID id(LLUUID::generateNewID());

    mPendingCoprocs.push(QueuedCoproc::ptr_t(new QueuedCoproc(name, id, proc)));
    LL_INFOS("CoProcMgr") << "Coprocedure(" << name << ") enqueued with id=" << id.asString() << " in pool \"" << mPoolName << "\"" << LL_ENDL;

    return id;
}

//-------------------------------------------------------------------------
void LLCoprocedurePool::coprocedureInvokerCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter)
{
    QueuedCoproc::ptr_t coproc;
    boost::fibers::channel_op_status status;
    for (;;)
    {
        {
            LLCoros::TempStatus st("waiting for work for 10s");
            status = mPendingCoprocs.pop_wait_for(coproc, std::chrono::seconds(10));
        }
        if (status == boost::fibers::channel_op_status::closed)
        {
            break;
        }

        if(status == boost::fibers::channel_op_status::timeout)
        {
            LL_INFOS_ONCE() << "pool '" << mPoolName << "' stalled." << LL_ENDL;
            continue;
        }

        ActiveCoproc_t::iterator itActive = mActiveCoprocs.insert(ActiveCoproc_t::value_type(coproc->mId, httpAdapter)).first;

        LL_DEBUGS("CoProcMgr") << "Dequeued and invoking coprocedure(" << coproc->mName << ") with id=" << coproc->mId.asString() << " in pool \"" << mPoolName << "\"" << LL_ENDL;

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

        LL_DEBUGS("CoProcMgr") << "Finished coprocedure(" << coproc->mName << ")" << " in pool \"" << mPoolName << "\"" << LL_ENDL;

        mActiveCoprocs.erase(itActive);
    }
}

void LLCoprocedurePool::close()
{
    mPendingCoprocs.close();
}
