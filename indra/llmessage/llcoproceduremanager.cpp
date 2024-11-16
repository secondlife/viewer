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
// SL-14399: When we teleport to a brand-new simulator, the coprocedure queue
// gets absolutely slammed with fetch requests. Make this queue effectively
// unlimited.
const U32 LLCoprocedureManager::DEFAULT_QUEUE_SIZE = 1024*1024;

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

    /// Returns the number of coprocedures in the queue awaiting processing.
    ///
    inline size_t countPending() const
    {
        return mPending;
    }

    /// Returns the number of coprocedures actively being processed.
    ///
    inline size_t countActive() const
    {
        return mActiveCoprocsCount;
    }

    /// Returns the total number of coprocedures either queued or in active processing.
    ///
    inline S32 count() const
    {
        return static_cast<S32>(countPending() + countActive());
    }

    void close();

private:
    struct QueuedCoproc
    {
        typedef std::shared_ptr<QueuedCoproc> ptr_t;

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
    // fiber running right now)
    typedef boost::fibers::buffered_channel<QueuedCoproc::ptr_t>  CoprocQueue_t;
    // Use shared_ptr to control the lifespan of our CoprocQueue_t instance
    // because the consuming coroutine might outlive this LLCoprocedurePool
    // instance.
    typedef std::shared_ptr<CoprocQueue_t> CoprocQueuePtr;

    std::string     mPoolName;
    size_t          mPoolSize, mActiveCoprocsCount, mPending;
    CoprocQueuePtr  mPendingCoprocs;
    LLTempBoundListener mStatusListener;

    typedef std::map<std::string, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t> CoroAdapterMap_t;
    LLCore::HttpRequest::policy_t mHTTPPolicy;

    CoroAdapterMap_t mCoroMapping;

    void coprocedureInvokerCoro(CoprocQueuePtr pendingCoprocs,
                                LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter);
};

//=========================================================================
LLCoprocedureManager::LLCoprocedureManager()
{
}

LLCoprocedureManager::~LLCoprocedureManager()
{
    close();
}

void LLCoprocedureManager::initializePool(const std::string &poolName)
{
    poolMap_t::iterator it = mPoolMap.find(poolName);

    if (it != mPoolMap.end())
    {
        // Pools are not supposed to be initialized twice
        // Todo: ideally restrict init to STATE_FIRST
        LL_ERRS() << "Pool is already present " << poolName << LL_ENDL;
        return;
    }

    // Attempt to look up a pool size in the configuration.  If found use that
    std::string keyName = "PoolSize" + poolName;
    int size = 0;

    LL_ERRS_IF(poolName.empty(), "CoprocedureManager") << "Poolname must not be empty" << LL_ENDL;
    LL_INFOS("CoprocedureManager") << "Initializing pool " << poolName << LL_ENDL;

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
}

//-------------------------------------------------------------------------
LLUUID LLCoprocedureManager::enqueueCoprocedure(const std::string &pool, const std::string &name, CoProcedure_t proc)
{
    // Attempt to find the pool and enqueue the procedure.  If the pool does
    // not exist, create it.
    poolMap_t::iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
    {
        // initializing pools in enqueueCoprocedure is not thread safe,
        // at the moment pools need to be initialized manually
        LL_ERRS() << "Uninitialized pool " << pool << LL_ENDL;
    }

    poolPtr_t targetPool = it->second;
    return targetPool->enqueueCoprocedure(name, proc);
}

void LLCoprocedureManager::setPropertyMethods(SettingQuery_t queryfn, SettingUpdate_t updatefn)
{
    // functions to discover and store the pool sizes
    // Might be a better idea to make an initializePool(name, size) to init everything externally
    mPropertyQueryFn = queryfn;
    mPropertyDefineFn = updatefn;

    initializePool("Upload");
    initializePool("AIS"); // it might be better to have some kind of on-demand initialization for AIS
    // "ExpCache" pool gets initialized in LLExperienceCache
    // asset storage pool gets initialized in LLViewerAssetStorage
}

//-------------------------------------------------------------------------
size_t LLCoprocedureManager::countPending() const
{
    size_t count = 0;
    for (const auto& pair : mPoolMap)
    {
        count += pair.second->countPending();
    }
    return count;
}

size_t LLCoprocedureManager::countPending(const std::string &pool) const
{
    poolMap_t::const_iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
        return 0;
    return it->second->countPending();
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

size_t LLCoprocedureManager::count() const
{
    size_t count = 0;
    for (const auto& pair : mPoolMap)
    {
        count += pair.second->count();
    }
    return count;
}

size_t LLCoprocedureManager::count(const std::string &pool) const
{
    poolMap_t::const_iterator it = mPoolMap.find(pool);

    if (it == mPoolMap.end())
        return 0;
    return it->second->count();
}

void LLCoprocedureManager::close()
{
    for(auto & poolEntry : mPoolMap)
    {
        poolEntry.second->close();
    }
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
    mActiveCoprocsCount(0),
    mPending(0),
    mPendingCoprocs(std::make_shared<CoprocQueue_t>(LLCoprocedureManager::DEFAULT_QUEUE_SIZE)),
    mHTTPPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID),
    mCoroMapping()
{
    try
    {
        // Store in our LLTempBoundListener so that when the LLCoprocedurePool is
        // destroyed, we implicitly disconnect from this LLEventPump.
        // Monitors application status.
        mStatusListener = LLCoros::getStopListener(
            poolName + "_pool", // Make sure it won't repeat names from lleventcoro
            [pendingCoprocs = mPendingCoprocs, poolName](const LLSD& event)
            {
                LL_INFOS("CoProcMgr") << "Pool " << poolName
                                      << " closing queue because status " << event
                                      << LL_ENDL;
                // This should ensure that all waiting coprocedures in this
                // pool will wake up and terminate.
                pendingCoprocs->close();
            });
    }
    catch (const LLEventPump::DupListenerName &)
    {
        // This shounldn't be possible since LLCoprocedurePool is supposed to have unique names,
        // yet it somehow did happen, as result pools got '_pool' suffix and this catch.
        //
        // If this somehow happens again it is better to crash later on shutdown due to pump
        // not stopping coroutine and see warning in logs than on startup or during login.
        LL_WARNS("CoProcMgr") << "Attempted to register duplicate listener name: " << poolName
                              << "_pool. Failed to start listener." << LL_ENDL;

        llassert(0); // Fix Me! Ignoring missing listener!
    }

    for (size_t count = 0; count < mPoolSize; ++count)
    {
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter( mPoolName + "Adapter", mHTTPPolicy));

        std::string pooledCoro = LLCoros::instance().launch(
            "LLCoprocedurePool("+mPoolName+")::coprocedureInvokerCoro",
            boost::bind(&LLCoprocedurePool::coprocedureInvokerCoro, this,
                        mPendingCoprocs, httpAdapter));

        mCoroMapping.insert(CoroAdapterMap_t::value_type(pooledCoro, httpAdapter));
    }

    LL_INFOS("CoProcMgr") << "Created coprocedure pool named \"" << mPoolName << "\" with " << size << " items, queue max " << LLCoprocedureManager::DEFAULT_QUEUE_SIZE << LL_ENDL;
}

LLCoprocedurePool::~LLCoprocedurePool()
{
}

//-------------------------------------------------------------------------
LLUUID LLCoprocedurePool::enqueueCoprocedure(const std::string &name, LLCoprocedurePool::CoProcedure_t proc)
{
    LLUUID id(LLUUID::generateNewID());

    if (mPoolName == "AIS")
    {
        // Fetch is going to be spammy.
        LL_DEBUGS("CoProcMgr", "Inventory") << "Coprocedure(" << name << ") enqueuing with id=" << id.asString() << " in pool \"" << mPoolName
                                   << "\" at "
                              << mPending << LL_ENDL;

        if (mPending >= (LLCoprocedureManager::DEFAULT_QUEUE_SIZE - 1))
        {
            // If it's all used up (not supposed to happen,
            // fetched should cap it), we are going to crash
            LL_WARNS("CoProcMgr", "Inventory") << "About to run out of queue space for Coprocedure(" << name
                                               << ") enqueuing with id=" << id.asString() << " Already pending:" << mPending << LL_ENDL;
        }
    }
    else
    {
        LL_INFOS("CoProcMgr") << "Coprocedure(" << name << ") enqueuing with id=" << id.asString() << " in pool \"" << mPoolName << "\" at "
                              << mPending << LL_ENDL;
    }
    auto pushed = mPendingCoprocs->try_push(std::make_shared<QueuedCoproc>(name, id, proc));
    if (pushed == boost::fibers::channel_op_status::success)
    {
        ++mPending;
        return id;
    }

    // Here we didn't succeed in pushing. Shutdown could be the reason.
    if (pushed == boost::fibers::channel_op_status::closed)
    {
        LL_WARNS("CoProcMgr") << "Discarding coprocedure '" << name << "' because shutdown" << LL_ENDL;
        return {};
    }

    // The queue should never fill up.
    LL_ERRS("CoProcMgr") << "Enqueue into '" << name << "' failed (" << unsigned(pushed) << ")" << LL_ENDL;
    return {};                      // never executed, pacify the compiler
}

//-------------------------------------------------------------------------
void LLCoprocedurePool::coprocedureInvokerCoro(
    CoprocQueuePtr pendingCoprocs,
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter)
{
    std::string prevtask;
    for (;;)
    {
        // It is VERY IMPORTANT that we instantiate a new ptr_t just before
        // the pop_wait_for() call below. When this ptr_t was declared at
        // function scope (outside the for loop), NickyD correctly diagnosed a
        // mysterious hang condition due to:
        // - the second time through the loop, the ptr_t held the last pointer
        //   to the previous QueuedCoproc, which indirectly held the last
        //   LLPointer to an LLInventoryCallback instance
        // - while holding the lock on pendingCoprocs, pop_wait_for() assigned
        //   the popped value to the ptr_t variable
        // - assignment destroyed the previous value of that variable, which
        //   indirectly destroyed the LLInventoryCallback
        // - whose destructor called ~LLRequestServerAppearanceUpdateOnDestroy()
        // - which called LLAppearanceMgr::requestServerAppearanceUpdate()
        // - which called enqueueCoprocedure()
        // - which tried to acquire the lock on pendingCoprocs... alas.
        // Using a fresh, clean ptr_t ensures that no previous value is
        // destroyed during pop_wait_for().
        QueuedCoproc::ptr_t coproc;
        boost::fibers::channel_op_status status;
        // Each time control reaches our custom coroutine scheduler, we check
        // how long the previous coroutine ran before yielding, and report
        // coroutines longer than a certain cutoff. But these coprocedure pool
        // coroutines are generic; the only way we know what work they're
        // doing is the task 'status' set by LLCoros::setStatus(). But what if
        // the coroutine runs the task to completion and returns to waiting?
        // It does no good to report that "waiting" ran long. So each time we
        // enter "waiting" status, also report the *previous* task name.
        std::string waiting = "waiting", newstatus;
        if (prevtask.empty())
        {
            newstatus = waiting;
        }
        else
        {
            newstatus = stringize("done ", prevtask, "; ", waiting);
        }
        LLCoros::setStatus(newstatus);
        status = pendingCoprocs->pop_wait_for(coproc, std::chrono::seconds(10));
        if (status == boost::fibers::channel_op_status::closed)
        {
            break;
        }

        if(status == boost::fibers::channel_op_status::timeout)
        {
            LL_DEBUGS_ONCE("CoProcMgr") << "pool '" << mPoolName << "' waiting." << LL_ENDL;
            prevtask.clear();
            continue;
        }
        // we actually popped an item
        --mPending;
        mActiveCoprocsCount++;

        LL_DEBUGS("CoProcMgr") << "Dequeued and invoking coprocedure(" << coproc->mName << ") with id=" << coproc->mId.asString() << " in pool \"" << mPoolName << "\" (" << mPending << " left)" << LL_ENDL;

        try
        {
            // set "status" of pool coroutine to the name of the coproc task
            prevtask = coproc->mName;
            LLCoros::setStatus(prevtask);
            coproc->mProc(httpAdapter, coproc->mId);
        }
        catch (const LLCoros::Stop &e)
        {
            LL_INFOS("LLCoros") << "coprocedureInvokerCoro terminating because "
                << e.what() << LL_ENDL;
            throw; // let toplevel handle this as LLContinueError
        }
        catch (...)
        {
            LOG_UNHANDLED_EXCEPTION(STRINGIZE("Coprocedure('" << coproc->mName
                                              << "', id=" << coproc->mId.asString()
                                              << ") in pool '" << mPoolName << "'"));
            // must NOT omit this or we deplete the pool
            mActiveCoprocsCount--;
            continue;
        }

        LL_DEBUGS("CoProcMgr") << "Finished coprocedure(" << coproc->mName << ")" << " in pool \"" << mPoolName << "\"" << LL_ENDL;

        mActiveCoprocsCount--;
    }
}

void LLCoprocedurePool::close()
{
    mPendingCoprocs->close();
}
