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

#ifndef LL_COPROCEDURE_MANAGER_H
#define LL_COPROCEDURE_MANAGER_H

#include "lleventcoro.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "lluuid.h"

class LLCoprocedurePool;

class LLCoprocedureManager : public LLSingleton < LLCoprocedureManager >
{
    friend class LLSingleton < LLCoprocedureManager > ;

public:
    typedef boost::function<U32(const std::string &)> SettingQuery_t;
    typedef boost::function<void(const std::string &, U32, const std::string &)> SettingUpdate_t;

    typedef boost::function<void(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &, const LLUUID &id)> CoProcedure_t;

    LLCoprocedureManager();
    virtual ~LLCoprocedureManager();

    /// Places the coprocedure on the queue for processing. 
    /// 
    /// @param name Is used for debugging and should identify this coroutine.
    /// @param proc Is a bound function to be executed 
    /// 
    /// @return This method returns a UUID that can be used later to cancel execution.
    LLUUID enqueueCoprocedure(const std::string &pool, const std::string &name, CoProcedure_t proc);

    /// Cancel a coprocedure. If the coprocedure is already being actively executed 
    /// this method calls cancelYieldingOperation() on the associated HttpAdapter
    /// If it has not yet been dequeued it is simply removed from the queue.
    void cancelCoprocedure(const LLUUID &id);

    /// Requests a shutdown of the upload manager. Passing 'true' will perform 
    /// an immediate kill on the upload coroutine.
    void shutdown(bool hardShutdown = false);

    void setPropertyMethods(SettingQuery_t queryfn, SettingUpdate_t updatefn);

    /// Returns the number of coprocedures in the queue awaiting processing.
    ///
    size_t countPending() const;
    size_t countPending(const std::string &pool) const;

    /// Returns the number of coprocedures actively being processed.
    ///
    size_t countActive() const;
    size_t countActive(const std::string &pool) const;

    /// Returns the total number of coprocedures either queued or in active processing.
    ///
    size_t count() const;
    size_t count(const std::string &pool) const;

private:

    typedef boost::shared_ptr<LLCoprocedurePool> poolPtr_t;
    typedef std::map<std::string, poolPtr_t> poolMap_t;

    poolMap_t mPoolMap;

    poolPtr_t initializePool(const std::string &poolName);

    SettingQuery_t mPropertyQueryFn;
    SettingUpdate_t mPropertyDefineFn;
};

#endif
