/** 
 * @file llexperiencecache.h
 * @brief Caches information relating to experience keys
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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



#ifndef LL_LLEXPERIENCECACHE_H
#define LL_LLEXPERIENCECACHE_H

#include "linden_common.h"
#include "llsingleton.h"
#include "llframetimer.h"
#include "llsd.h"
#include "llcorehttputil.h"
#include <boost/signals2.hpp>
#include <boost/function.hpp>

class LLSD;
class LLUUID;


class LLExperienceCache: public LLSingleton < LLExperienceCache >
{
    LLSINGLETON(LLExperienceCache);

public:
    typedef boost::function<std::string(const std::string &)> CapabilityQuery_t;
    typedef boost::function<void(const LLSD &)> ExperienceGetFn_t;

    void setCapabilityQuery(CapabilityQuery_t queryfn);
    void cleanup();

    //-------------------------------------------
    // Cache methods 
    void erase(const LLUUID& key);
    bool fetch(const LLUUID& key, bool refresh = false);
    void insert(const LLSD& experience_data);
    const LLSD& get(const LLUUID& key);
    void get(const LLUUID& key, ExperienceGetFn_t slot); // If name information is in cache, callback will be called immediately.

    bool isRequestPending(const LLUUID& public_key);

    //-------------------------------------------
    void fetchAssociatedExperience(const LLUUID& objectId, const LLUUID& itemId, ExperienceGetFn_t fn);
    void fetchAssociatedExperience(const LLUUID& objectId, const LLUUID& itemId, std::string url, ExperienceGetFn_t fn);
    void findExperienceByName(const std::string text, int page, ExperienceGetFn_t fn);
    void getGroupExperiences(const LLUUID &groupId, ExperienceGetFn_t fn);

    // the Get/Set Region Experiences take a CapabilityQuery to get the capability since 
    // the region being queried may not be the region that the agent is standing on.
    void getRegionExperiences(CapabilityQuery_t regioncaps, ExperienceGetFn_t fn);
    void setRegionExperiences(CapabilityQuery_t regioncaps, const LLSD &experiences, ExperienceGetFn_t fn);

    void getExperiencePermission(const LLUUID &experienceId, ExperienceGetFn_t fn);
    void setExperiencePermission(const LLUUID &experienceId, const std::string &permission, ExperienceGetFn_t fn);
    void forgetExperiencePermission(const LLUUID &experienceId, ExperienceGetFn_t fn);

    void getExperienceAdmin(const LLUUID &experienceId, ExperienceGetFn_t fn);

    void updateExperience(LLSD updateData, ExperienceGetFn_t fn);
        //-------------------------------------------
    static const std::string NAME;          // "name"
    static const std::string EXPERIENCE_ID; // "public_id"
    static const std::string AGENT_ID;      // "agent_id"
    static const std::string GROUP_ID;      // "group_id"
    static const std::string PROPERTIES;    // "properties"
    static const std::string EXPIRES;       // "expiration"  
    static const std::string DESCRIPTION;   // "description"
    static const std::string QUOTA;         // "quota"
    static const std::string MATURITY;      // "maturity"
    static const std::string METADATA;      // "extended_metadata"
    static const std::string SLURL;         // "slurl"

    static const std::string MISSING;       // "DoesNotExist"

    // should be in sync with experience-api/experiences/models.py
    static const int PROPERTY_INVALID;      // 1 << 0
    static const int PROPERTY_PRIVILEGED;   // 1 << 3
    static const int PROPERTY_GRID;         // 1 << 4
    static const int PROPERTY_PRIVATE;      // 1 << 5
    static const int PROPERTY_DISABLED;     // 1 << 6  
    static const int PROPERTY_SUSPENDED;    // 1 << 7

private:
    virtual ~LLExperienceCache();

    virtual void initSingleton();

    typedef boost::function<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &, LLCore::HttpRequest::ptr_t, std::string)> permissionInvoker_fn;

    // Callback types for get() 
    typedef boost::signals2::signal < void(const LLSD &) > callback_signal_t;
    typedef boost::shared_ptr<callback_signal_t> signal_ptr;
    // May have multiple callbacks for a single ID, which are
    // represented as multiple slots bound to the signal.
    // Avoid copying signals via pointers.
    typedef std::map<LLUUID, signal_ptr> signal_map_t;
    typedef std::map<LLUUID, LLSD> cache_t;
    
    typedef std::set<LLUUID> RequestQueue_t;
    typedef std::map<LLUUID, F64> PendingQueue_t;

    //--------------------------------------------
    static const std::string PRIVATE_KEY;   // "private_id"
    
    // default values
    static const F64 DEFAULT_EXPIRATION;    // 600.0
    static const S32 DEFAULT_QUOTA;         // 128 this is megabytes
    static const int SEARCH_PAGE_SIZE;
    
//--------------------------------------------
    void processExperience(const LLUUID& public_key, const LLSD& experience);

//--------------------------------------------
    cache_t         mCache;
    signal_map_t    mSignalMap; 
    RequestQueue_t  mRequestQueue;
    PendingQueue_t  mPendingQueue;

    LLFrameTimer    mEraseExpiredTimer;    // Periodically clean out expired entries from the cache
    CapabilityQuery_t mCapability;
    std::string     mCacheFileName;
    static bool     sShutdown; // control for coroutines, they exist out of LLExperienceCache's scope, so they need a static control

    void idleCoro();
    void eraseExpired();
    void requestExperiencesCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &, std::string, RequestQueue_t);
    void requestExperiences();

    void fetchAssociatedExperienceCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &, LLUUID, LLUUID, std::string, ExperienceGetFn_t);
    void findExperienceByNameCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &, std::string, int, ExperienceGetFn_t);
    void getGroupExperiencesCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &, LLUUID , ExperienceGetFn_t);
    void regionExperiencesCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, CapabilityQuery_t regioncaps, bool update, LLSD experiences, ExperienceGetFn_t fn);
    void experiencePermissionCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, permissionInvoker_fn invokerfn, std::string url, ExperienceGetFn_t fn);
    void getExperienceAdminCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, LLUUID experienceId, ExperienceGetFn_t fn);
    void updateExperienceCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, LLSD updateData, ExperienceGetFn_t fn);

    void bootstrap(const LLSD& legacyKeys, int initialExpiration);
    void exportFile(std::ostream& ostr) const;
    void importFile(std::istream& istr);

    // 
    const cache_t& getCached();

    // maps an experience private key to the experience id
    LLUUID getExperienceId(const LLUUID& private_key, bool null_if_not_found=false);

    //=====================================================================
    inline friend std::ostream &operator << (std::ostream &os, const LLExperienceCache &cache)
    {
        cache.exportFile(os);
        return os;
    }

    inline friend std::istream &operator >> (std::istream &is, LLExperienceCache &cache)
    {
        cache.importFile(is);
        return is;
    }
};

#endif // LL_LLEXPERIENCECACHE_H
