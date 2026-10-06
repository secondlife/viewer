/**
 * @file llcachename.cpp
 * @brief A hierarchical cache of first and last names queried based on UUID.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llcachename.h"

// linden library includes
#include "lldbstrings.h"
#include "llframetimer.h"
#include "llhost.h"
#include "llmutex.h"
#include "llrand.h"
#include "llsdserialize.h"
#include "lluuid.h"
#include "message.h"

#include <boost/regex.hpp>

// llsd serialization constants
static const std::string AGENTS("agents");
static const std::string GROUPS("groups");
static const std::string CTIME("ctime");
static const std::string FIRST("first");
static const std::string LAST("last");
static const std::string NAME("name");

// We track name requests in flight for up to this long.
// We won't re-request a name during this time
const U32 PENDING_TIMEOUT_SECS = 5 * 60;

// Globals
LLCacheName* gCacheName = NULL;

/// ---------------------------------------------------------------------------
/// class LLCacheNameEntry
/// ---------------------------------------------------------------------------

class LLCacheNameEntry
{
public:
    LLCacheNameEntry();

public:
    bool mIsGroup;
    U32 mCreateTime;    // unix time_t
    // IDEVO TODO collapse names to one field, which will eliminate
    // many string compares on "Resident"
    std::string mFirstName;
    std::string mLastName;
    std::string mGroupName;
};

LLCacheNameEntry::LLCacheNameEntry()
    : mIsGroup(false),
      mCreateTime(0)
{
}


class PendingReply
{
public:
    LLUUID              mID;
    LLCacheNameSignal   mSignal;
    LLHost              mHost;

    PendingReply(const LLUUID& id, const LLHost& host)
        : mID(id), mHost(host)
    {
    }

    boost::signals2::connection setCallback(const LLCacheNameCallback& cb)
    {
        return mSignal.connect(cb);
    }

    void done()         { mID.setNull(); }
    bool isDone() const { return mID.isNull(); }
};

class ReplySender
{
public:
    ReplySender(LLMessageSystem* msg);
    ~ReplySender();

    void send(const LLUUID& id,
        const LLCacheNameEntry& entry, const LLHost& host);

private:
    void flush();

    LLMessageSystem*    mMsg;
    bool                mPending;
    bool                mCurrIsGroup;
    LLHost              mCurrHost;
};

ReplySender::ReplySender(LLMessageSystem* msg)
    : mMsg(msg), mPending(false), mCurrIsGroup(false)
{ }

ReplySender::~ReplySender()
{
    flush();
}

void ReplySender::send(const LLUUID& id,
    const LLCacheNameEntry& entry, const LLHost& host)
{
    if (mPending)
    {
        if (mCurrIsGroup != entry.mIsGroup
        ||  mCurrHost != host)
        {
            flush();
        }
    }

    if (!mPending)
    {
        mPending = true;
        mCurrIsGroup = entry.mIsGroup;
        mCurrHost = host;

        if(mCurrIsGroup)
            mMsg->newMessageFast(_PREHASH_UUIDGroupNameReply);
        else
            mMsg->newMessageFast(_PREHASH_UUIDNameReply);
    }

    mMsg->nextBlockFast(_PREHASH_UUIDNameBlock);
    mMsg->addUUIDFast(_PREHASH_ID, id);
    if(mCurrIsGroup)
    {
        mMsg->addStringFast(_PREHASH_GroupName, entry.mGroupName);
    }
    else
    {
        mMsg->addStringFast(_PREHASH_FirstName, entry.mFirstName);
        mMsg->addStringFast(_PREHASH_LastName, entry.mLastName);
    }

    if(mMsg->isSendFullFast(_PREHASH_UUIDNameBlock))
    {
        flush();
    }
}

void ReplySender::flush()
{
    if (mPending)
    {
        mMsg->sendReliable(mCurrHost);
        mPending = false;
    }
}

using AskQueue     = std::set<LLUUID>;
using ReplyQueue   = std::list<PendingReply*>;
using PendingQueue = std::unordered_map<LLUUID, U32>;
using Cache        = std::unordered_map<LLUUID, LLCacheNameEntry*>;
using ReverseCache = std::unordered_map<std::string, LLUUID>;

// A name/group-name reply received by processUUIDReply() (which may run
// directly on LLUDPReceiverThread). Queued so mSignal can be fired later
// from the main thread by processPendingSignals(), instead of invoking
// observer callbacks directly on the UDP thread.
struct PendingSignal
{
    LLUUID      mID;
    std::string mName;
    bool        mIsGroup;
};
using SignalQueue = std::vector<PendingSignal>;

//
// mSignal is deliberately NOT protected by mMutex: observers may call back
// into LLCacheName (e.g. getFullName()), and LLMutex is not safe to
// re-enter from the same call chain, so the mutex must always be released
// before mSignal (or any PendingReply::mSignal) is invoked.
//
// LLMessageSystem's outbound message builder (used by newMessageFast(),
// nextBlockFast() is not safe to drive concurrently from two threads.
// To avoid that, processUUIDRequest() always defers them via addPending(),
// so all actual message building and sending remains confined to
// processPendingReplies() and processPendingAsks() on main thread.
class LLCacheName::Impl
{
public:
    LLMessageSystem*    mMsg;
    LLHost              mUpstreamHost;

    mutable LLMutex     mMutex;
        // protects mCache, mReverseCache, mAskNameQueue, mAskGroupQueue,
        // mPendingQueue, mReplyQueue, and mUpstreamHost. See class comment.

    Cache               mCache;
        // the map of UUIDs to names
    ReverseCache        mReverseCache;
        // map of names to UUIDs

    AskQueue            mAskNameQueue;
    AskQueue            mAskGroupQueue;
        // UUIDs to ask our upstream host about

    PendingQueue        mPendingQueue;
        // UUIDs that have been requested but are not in cache yet.

    ReplyQueue          mReplyQueue;
        // requests awaiting replies from us

    SignalQueue         mSignalQueue;
    // (id, name, isGroup) tuples queued by processUUIDReply() (which
    // may run on LLUDPReceiverThread) for mSignal to be fired on
    // later, from the main thread via processPendingSignals().

    LLCacheNameSignal   mSignal;

    LLFrameTimer        mProcessTimer;

    Impl(LLMessageSystem* msg);
    ~Impl();

    // Caller must hold mMutex.
    bool getName(const LLUUID& id, std::string& first, std::string& last, cache_map_t& default_names);

    // Caller must hold mMutex.
    boost::signals2::connection addPending(const LLUUID& id, const LLCacheNameCallback& callback);
    // Caller must hold mMutex.
    void addPending(const LLUUID& id, const LLHost& host);

    // Only ever called from the main thread (LLCacheName::processPending()).
    void processPendingAsks();
    // Only ever called from the main thread (LLCacheName::processPending()).
    void processPendingReplies();
    // Only ever called from the main thread (LLCacheName::processPending()).
    // Fires mSignal for entries queued by processUUIDReply().
    void processPendingSignals();
    // Only ever called from the main thread; caller must hold mMutex.
    void sendRequest(const char* msg_name, const AskQueue& queue);
    // Caller must hold mMutex.
    bool isRequestPending(const LLUUID& id);

    // Message system callbacks. Must not touch anything not
    // protected by mMutex, and must not build/send messages.
    void processUUIDRequest(LLMessageSystem* msg, bool isGroup);
    void processUUIDReply(LLMessageSystem* msg, bool isGroup);

    static void handleUUIDNameRequest(LLMessageSystem* msg, void** userdata);
    static void handleUUIDNameReply(LLMessageSystem* msg, void** userdata);
    static void handleUUIDGroupNameRequest(LLMessageSystem* msg, void** userdata);
    static void handleUUIDGroupNameReply(LLMessageSystem* msg, void** userdata);
};


/// --------------------------------------------------------------------------
/// class LLCacheName
/// ---------------------------------------------------------------------------

LLCacheName::LLCacheName(LLMessageSystem* msg)
    : impl(* new Impl(msg))
    { }

LLCacheName::LLCacheName(LLMessageSystem* msg, const LLHost& upstream_host)
    : impl(* new Impl(msg))
{
    mCacheName["waiting"] = "(Loading...)";
    mCacheName["nobody"] = "(nobody)";
    mCacheName["none"] = "(none)";
    setUpstream(upstream_host);
}

LLCacheName::~LLCacheName()
{
    if (gMessageSystem != nullptr)
    {
        gMessageSystem->setHandlerFuncThrdFast(_PREHASH_UUIDNameRequest, nullptr, nullptr);
        gMessageSystem->setHandlerFuncThrdFast(_PREHASH_UUIDNameReply, nullptr, nullptr);
        gMessageSystem->setHandlerFuncThrdFast(_PREHASH_UUIDGroupNameRequest, nullptr, nullptr);
        gMessageSystem->setHandlerFuncThrdFast(_PREHASH_UUIDGroupNameReply, nullptr, nullptr);
    }
    delete &impl;
}

LLCacheName::Impl::Impl(LLMessageSystem* msg)
    : mMsg(msg), mUpstreamHost(LLHost())
{
    // Safe to run directly on LLUDPReceiverThread: all shared state these
    // handlers touch is protected by mMutex, and neither handler builds or
    // sends an outbound message itself (see class comment above).
    mMsg->setHandlerFuncThrdFast(
        _PREHASH_UUIDNameRequest, handleUUIDNameRequest, (void**)this);
    mMsg->setHandlerFuncThrdFast(
        _PREHASH_UUIDNameReply, handleUUIDNameReply, (void**)this);
    mMsg->setHandlerFuncThrdFast(
        _PREHASH_UUIDGroupNameRequest, handleUUIDGroupNameRequest, (void**)this);
    mMsg->setHandlerFuncThrdFast(
        _PREHASH_UUIDGroupNameReply, handleUUIDGroupNameReply, (void**)this);
}


LLCacheName::Impl::~Impl()
{
    std::for_each(mCache.begin(), mCache.end(), DeletePairedPointer());
    mCache.clear();
    for_each(mReplyQueue.begin(), mReplyQueue.end(), DeletePointer());
    mReplyQueue.clear();
}

boost::signals2::connection LLCacheName::Impl::addPending(const LLUUID& id, const LLCacheNameCallback& callback)
{
    PendingReply* reply = new PendingReply(id, LLHost());
    boost::signals2::connection res = reply->setCallback(callback);
    mReplyQueue.push_back(reply);
    return res;
}

void LLCacheName::Impl::addPending(const LLUUID& id, const LLHost& host)
{
    PendingReply* reply = new PendingReply(id, host);
    mReplyQueue.push_back(reply);
}

void LLCacheName::setUpstream(const LLHost& upstream_host)
{
    LLMutexLock lock(&impl.mMutex);
    impl.mUpstreamHost = upstream_host;
}

boost::signals2::connection LLCacheName::addObserver(const LLCacheNameCallback& callback)
{
    return impl.mSignal.connect(callback);
}

bool LLCacheName::importFile(std::istream& istr)
{
    LLSD data;
    if(LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXMLDocument(data, istr))
    {
        return false;
    }

    // We'll expire entries more than a week old
    U32 now = (U32)time(NULL);
    const U32 SECS_PER_DAY = 60 * 60 * 24;
    U32 delete_before_time = now - (7 * SECS_PER_DAY);

    LLMutexLock lock(&impl.mMutex);

    // iterate over the agents
    S32 count = 0;
    LLSD agents = data[AGENTS];
    LLSD::map_iterator iter = agents.beginMap();
    LLSD::map_iterator end = agents.endMap();
    for( ; iter != end; ++iter)
    {
        LLUUID id((*iter).first);
        LLSD agent = (*iter).second;
        U32 ctime = (U32)agent[CTIME].asInteger();
        if(ctime < delete_before_time) continue;

        LLCacheNameEntry* entry = new LLCacheNameEntry();
        entry->mIsGroup = false;
        entry->mCreateTime = ctime;
        entry->mFirstName = agent[FIRST].asString();
        entry->mLastName = agent[LAST].asString();
        impl.mCache[id] = entry;
        std::string fullname = buildFullName(entry->mFirstName, entry->mLastName);
        impl.mReverseCache[fullname] = id;

        ++count;
    }
    LL_INFOS() << "LLCacheName loaded " << count << " agent names" << LL_ENDL;

    count = 0;
    LLSD groups = data[GROUPS];
    iter = groups.beginMap();
    end = groups.endMap();
    for( ; iter != end; ++iter)
    {
        LLUUID id((*iter).first);
        LLSD group = (*iter).second;
        U32 ctime = (U32)group[CTIME].asInteger();
        if(ctime < delete_before_time) continue;

        LLCacheNameEntry* entry = new LLCacheNameEntry();
        entry->mIsGroup = true;
        entry->mCreateTime = ctime;
        entry->mGroupName = group[NAME].asString();
        impl.mCache[id] = entry;
        impl.mReverseCache[entry->mGroupName] = id;
        ++count;
    }
    LL_INFOS() << "LLCacheName loaded " << count << " group names" << LL_ENDL;
    return true;
}

void LLCacheName::exportFile(std::ostream& ostr)
{
    LLSD data;

    LLMutexLock lock(&impl.mMutex);
    Cache::iterator iter = impl.mCache.begin();
    Cache::iterator end = impl.mCache.end();
    for( ; iter != end; ++iter)
    {
        // Only write entries for which we have valid data.
        LLCacheNameEntry* entry = iter->second;
        if(!entry
           || (std::string::npos != entry->mFirstName.find('?'))
           || (std::string::npos != entry->mGroupName.find('?')))
        {
            continue;
        }

        // store it
        LLUUID id = iter->first;
        std::string id_str = id.asString();
        // IDEVO TODO: Should we store SLIDs with last name "Resident" or not?
        if(!entry->mFirstName.empty() && !entry->mLastName.empty())
        {
            data[AGENTS][id_str][FIRST] = entry->mFirstName;
            data[AGENTS][id_str][LAST] = entry->mLastName;
            data[AGENTS][id_str][CTIME] = (S32)entry->mCreateTime;
        }
        else if(entry->mIsGroup && !entry->mGroupName.empty())
        {
            data[GROUPS][id_str][NAME] = entry->mGroupName;
            data[GROUPS][id_str][CTIME] = (S32)entry->mCreateTime;
        }
    }

    LLSDSerialize::toPrettyXML(data, ostr);
}


bool LLCacheName::Impl::getName(const LLUUID& id, std::string& first, std::string& last, cache_map_t &default_names)
{
    // Caller holds mMutex.
    if(id.isNull())
    {
        first = default_names["nobody"];
        last.clear();
        return true;
    }

    LLCacheNameEntry* entry = get_ptr_in_map(mCache, id );
    if (entry)
    {
        first = entry->mFirstName;
        last =  entry->mLastName;
        return true;
    }
    else
    {
        first = default_names["waiting"];
        last.clear();
        if (!isRequestPending(id))
        {
            mAskNameQueue.insert(id);
        }
        return false;
    }

}

// static
void LLCacheName::localizeCacheName(std::string key, std::string value)
{
    if (!key.empty() && !value.empty())
        mCacheName[key]=value;
    else
        LL_WARNS()<< " Error localizing cache key " << key << " To "<< value<<LL_ENDL;
}

bool LLCacheName::getFullName(const LLUUID& id, std::string& fullname)
{
    std::string first_name, last_name;
    bool res;
    {
        LLMutexLock lock(&impl.mMutex);
        res = impl.getName(id, first_name, last_name, mCacheName);
    }
    fullname = buildFullName(first_name, last_name);
    return res;
}



bool LLCacheName::getGroupName(const LLUUID& id, std::string& group)
{
    if(id.isNull())
    {
        group = mCacheName["none"];
        return true;
    }

    LLMutexLock lock(&impl.mMutex);
    LLCacheNameEntry* entry = get_ptr_in_map(impl.mCache,id);
    if (entry && entry->mGroupName.empty())
    {
        // COUNTER-HACK to combat James' HACK in exportFile()...
        // this group name was loaded from a name cache that did not
        // bother to save the group name ==> we must ask for it
        LL_DEBUGS() << "LLCacheName queuing HACK group request: " << id << LL_ENDL;
        entry = NULL;
    }

    if (entry)
    {
        group = entry->mGroupName;
        return true;
    }
    else
    {
        group = mCacheName["waiting"];
        if (!impl.isRequestPending(id))
        {
            impl.mAskGroupQueue.insert(id);
        }
        return false;
    }
}

bool LLCacheName::getUUID(const std::string& first, const std::string& last, LLUUID& id)
{
    std::string full_name = buildFullName(first, last);
    return getUUID(full_name, id);
}

bool LLCacheName::getUUID(const std::string& full_name, LLUUID& id)
{
    LLMutexLock lock(&impl.mMutex);
    ReverseCache::iterator iter = impl.mReverseCache.find(full_name);
    if (iter != impl.mReverseCache.end())
    {
        id = iter->second;
        return true;
    }
    else
    {
        return false;
    }
}

//static
std::string LLCacheName::buildFullName(const std::string& first, const std::string& last)
{
    std::string fullname = first;
    if (!last.empty()
        && last != "Resident")
    {
        fullname += ' ';
        fullname += last;
    }
    return fullname;
}

//static
std::string LLCacheName::cleanFullName(const std::string& full_name)
{
    return full_name.substr(0, full_name.find(" Resident"));
}

//static
// Transform hard-coded name provided by server to a more legible username
std::string LLCacheName::buildUsername(const std::string& full_name)
{
    // rare, but handle hard-coded error names returned from server
    if (full_name == "(\?\?\?) (\?\?\?)")
    {
        return "(\?\?\?)";
    }

    std::string::size_type index = full_name.find(' ');

    if (index != std::string::npos)
    {
        std::string username;
        username = full_name.substr(0, index);
        std::string lastname = full_name.substr(index+1);

        if (lastname != "Resident")
        {
            username = username + "." + lastname;
        }

        LLStringUtil::toLower(username);
        return username;
    }

    // if the input wasn't a correctly formatted legacy name, just return it
    // cleaned up from a potential terminal "Resident"
    std::string clean_name = cleanFullName(full_name);
    LLStringUtil::toLower(clean_name);
    return clean_name;
}

//static
std::string LLCacheName::buildLegacyName(const std::string& complete_name)
{
    //boost::regexp was showing up in the crashreporter, so doing
    //painfully manual parsing using substr. LF
    auto open_paren = complete_name.rfind(" (");
    auto close_paren = complete_name.rfind(')');

    if (open_paren != std::string::npos &&
        close_paren == complete_name.length()-1)
    {
        auto length = close_paren - open_paren - 2;
        std::string legacy_name = complete_name.substr(open_paren+2, length);

        if (legacy_name.length() > 0)
        {
            std::string cap_letter = legacy_name.substr(0, 1);
            LLStringUtil::toUpper(cap_letter);
            legacy_name = cap_letter + legacy_name.substr(1);

            auto separator = legacy_name.find('.');

            if (separator != std::string::npos)
            {
                std::string last_name = legacy_name.substr(separator+1);
                legacy_name = legacy_name.substr(0, separator);

                if (last_name.length() > 0)
                {
                    cap_letter = last_name.substr(0, 1);
                    LLStringUtil::toUpper(cap_letter);
                    legacy_name = legacy_name + " " + cap_letter + last_name.substr(1);
                }
            }

            return legacy_name;
        }
    }

    return complete_name;
}

// This is a little bit kludgy. LLCacheNameCallback is a slot instead of a function pointer.
//  The reason it is a slot is so that the legacy get() function below can bind an old callback
//  and pass it as a slot. The reason it isn't a std::function is so that trackable behavior
//  doesn't get lost. As a result, we have to bind the slot to a signal to call it, even when
//  we call it immediately. -Steve
// NOTE: Even though passing first and last name is a bit of extra overhead, it eliminates the
//  potential need for any parsing should any code need to handle first and last name independently.
boost::signals2::connection LLCacheName::get(const LLUUID& id, bool is_group, const LLCacheNameCallback& callback)
{
    boost::signals2::connection res;

    if(id.isNull())
    {
        LLCacheNameSignal signal;
        signal.connect(callback);
        signal(id, mCacheName["nobody"], is_group);
        return res;
    }

    // entry, if found, is copied out so the callback can be invoked outside
    // the lock (it may call back into LLCacheName).
    bool found = false;
    bool entry_is_group = false;
    std::string entry_group_name;
    std::string entry_full_name;
    {
        LLMutexLock lock(&impl.mMutex);
        LLCacheNameEntry* entry = get_ptr_in_map(impl.mCache, id);
        if (entry)
        {
            found = true;
            entry_is_group = entry->mIsGroup;
            if (entry_is_group)
            {
                entry_group_name = entry->mGroupName;
            }
            else
            {
                entry_full_name = buildFullName(entry->mFirstName, entry->mLastName);
            }
        }
        else
        {
            // id not found in map so we must queue the callback call until available.
            if (!impl.isRequestPending(id))
            {
                if (is_group)
                {
                    impl.mAskGroupQueue.insert(id);
                }
                else
                {
                    impl.mAskNameQueue.insert(id);
                }
            }
            res = impl.addPending(id, callback);
        }
    }

    if (found)
    {
        LLCacheNameSignal signal;
        signal.connect(callback);
        // id found in map therefore we can call the callback immediately.
        if (entry_is_group)
        {
            signal(id, entry_group_name, entry_is_group);
        }
        else
        {
            signal(id, entry_full_name, entry_is_group);
        }
    }

    return res;
}

boost::signals2::connection LLCacheName::getGroup(const LLUUID& group_id,
                                                  const LLCacheNameCallback& callback)
{
    return get(group_id, true, callback);
}

boost::signals2::connection LLCacheName::get(const LLUUID& id, bool is_group, old_callback_t callback, void* user_data)
{
    return get(id, is_group, boost::bind(callback, _1, _2, _3, user_data));
}

void LLCacheName::processPending()
{
    const F32 SECS_BETWEEN_PROCESS = 0.1f;
    if(!impl.mProcessTimer.checkExpirationAndReset(SECS_BETWEEN_PROCESS))
    {
        return;
    }

    {
        LLMutexLock lock(&impl.mMutex);
        if (!impl.mUpstreamHost.isOk())
        {
            LL_DEBUGS() << "LLCacheName::processPending() - bad upstream host."
                << LL_ENDL;
            return;
        }
    }

    // processPendingAsks()/processPendingReplies() are only ever called
    // from the main thread (here), so they own message building/sending.
    impl.processPendingAsks();
    impl.processPendingReplies();
    impl.processPendingSignals();
}

void LLCacheName::deleteEntriesOlderThan(S32 secs)
{
    U32 now = (U32)time(NULL);
    U32 expire_time = now - secs;

    LLMutexLock lock(&impl.mMutex);
    for(Cache::iterator iter = impl.mCache.begin(); iter != impl.mCache.end(); )
    {
        Cache::iterator curiter = iter++;
        LLCacheNameEntry* entry = curiter->second;
        if (entry->mCreateTime < expire_time)
        {
            delete entry;
            impl.mCache.erase(curiter);
        }
    }

    // These are pending requests that we never heard back from.
    U32 pending_expire_time = now - PENDING_TIMEOUT_SECS;
    for(PendingQueue::iterator p_iter = impl.mPendingQueue.begin();
        p_iter != impl.mPendingQueue.end(); )
    {
        PendingQueue::iterator p_curitor = p_iter++;

        if (p_curitor->second < pending_expire_time)
        {
            impl.mPendingQueue.erase(p_curitor);
        }
    }
}


void LLCacheName::dump()
{
    LLMutexLock lock(&impl.mMutex);
    for (Cache::iterator iter = impl.mCache.begin(),
             end = impl.mCache.end();
         iter != end; iter++)
    {
        LLCacheNameEntry* entry = iter->second;
        if (entry->mIsGroup)
        {
            LL_INFOS()
                << iter->first << " = (group) "
                << entry->mGroupName
                << " @ " << entry->mCreateTime
                << LL_ENDL;
        }
        else
        {
            LL_INFOS()
                << iter->first << " = "
                << buildFullName(entry->mFirstName, entry->mLastName)
                << " @ " << entry->mCreateTime
                << LL_ENDL;
        }
    }
}

void LLCacheName::dumpStats()
{
    LLMutexLock lock(&impl.mMutex);
    LL_INFOS() << "Queue sizes: "
            << " Cache=" << impl.mCache.size()
            << " AskName=" << impl.mAskNameQueue.size()
            << " AskGroup=" << impl.mAskGroupQueue.size()
            << " Pending=" << impl.mPendingQueue.size()
            << " Reply=" << impl.mReplyQueue.size()
//          << " Observers=" << impl.mSignal.size()
            << LL_ENDL;
}

void LLCacheName::clear()
{
    LLMutexLock lock(&impl.mMutex);
    std::for_each(impl.mCache.begin(), impl.mCache.end(), DeletePairedPointer());
    impl.mCache.clear();
}

//static
std::string LLCacheName::getDefaultName()
{
    return mCacheName["waiting"];
}

//static
std::string LLCacheName::getDefaultLastName()
{
    return "Resident";
}

void LLCacheName::Impl::processPendingAsks()
{
    // Only called from the main thread; owns mMsg's builder for the
    // duration of both sendRequest() calls.
    LLMutexLock lock(&mMutex);
    sendRequest(_PREHASH_UUIDNameRequest, mAskNameQueue);
    sendRequest(_PREHASH_UUIDGroupNameRequest, mAskGroupQueue);
    mAskNameQueue.clear();
    mAskGroupQueue.clear();
}

void LLCacheName::Impl::processPendingReplies()
{
    // Only called from the main thread. Copy out what's needed to invoke
    // callbacks/signals without holding mMutex (they may call back into
    // LLCacheName), then re-lock to do the actual sends and queue cleanup.
    struct ReplyInfo
    {
        PendingReply* reply;
        bool          isGroup;
        std::string   name;
    };
    std::vector<ReplyInfo> to_signal;

    {
        LLMutexLock lock(&mMutex);
        for (ReplyQueue::iterator it = mReplyQueue.begin(); it != mReplyQueue.end(); ++it)
        {
            PendingReply* reply = *it;
            LLCacheNameEntry* entry = get_ptr_in_map(mCache, reply->mID);
            if (!entry) continue;

            if (!entry->mIsGroup)
            {
                to_signal.push_back({ reply, false,
                    LLCacheName::buildFullName(entry->mFirstName, entry->mLastName) });
            }
            else
            {
                to_signal.push_back({ reply, true, entry->mGroupName });
            }
        }
    }

    // First call all the callbacks, because they might send messages.
    // Todo: needs cleanup logic, otherwise invalid ids might stay here indefinitely
    for (auto& info : to_signal)
    {
        (info.reply->mSignal)(info.reply->mID, info.name, info.isGroup);
    }

    // Forward on all replies, if needed. This is the only place mMsg's
    // builder is used for reply traffic, and it only ever runs here, on
    // the main thread.
    LLMutexLock lock(&mMutex);
    ReplySender sender(mMsg);
    for(ReplyQueue::iterator it = mReplyQueue.begin(); it != mReplyQueue.end(); ++it)
    {
        PendingReply* reply = *it;
        LLCacheNameEntry* entry = get_ptr_in_map(mCache, reply->mID);
        if(!entry) continue;

        if (reply->mHost.isOk())
        {
            sender.send(reply->mID, *entry, reply->mHost);
        }

        reply->done();
    }

    for(ReplyQueue::iterator it = mReplyQueue.begin(); it != mReplyQueue.end(); )
    {
        ReplyQueue::iterator curit = it++;
        PendingReply* reply = *curit;
        if (reply->isDone())
        {
            delete reply;
            mReplyQueue.erase(curit);
        }
    }
}

void LLCacheName::Impl::processPendingSignals()
{
    // Only called from the main thread. Copy out what's needed to invoke
    // mSignal without holding mMutex (observers may call back into
    // LLCacheName), then clear the queue.
    SignalQueue to_signal;
    {
        LLMutexLock lock(&mMutex);
        to_signal.swap(mSignalQueue);
    }

    for (auto& info : to_signal)
    {
        mSignal(info.mID, info.mName, info.mIsGroup);
    }
}

void LLCacheName::Impl::sendRequest(
    const char* msg_name,
    const AskQueue& queue)
{
    // Caller holds mMutex, and is always the main thread (processPendingAsks()).
    if(queue.empty())
    {
        return;
    }

    bool start_new_message = true;
    AskQueue::const_iterator it = queue.begin();
    AskQueue::const_iterator end = queue.end();
    for(; it != end; ++it)
    {
        if(start_new_message)
        {
            start_new_message = false;
            mMsg->newMessageFast(msg_name);
        }
        mMsg->nextBlockFast(_PREHASH_UUIDNameBlock);
        mMsg->addUUIDFast(_PREHASH_ID, (*it));

        if(mMsg->isSendFullFast(_PREHASH_UUIDNameBlock))
        {
            start_new_message = true;
            mMsg->sendReliable(mUpstreamHost);
        }
    }
    if(!start_new_message)
    {
        mMsg->sendReliable(mUpstreamHost);
    }
}

bool LLCacheName::Impl::isRequestPending(const LLUUID& id)
{
    // Caller holds mMutex.
    U32 now = (U32)time(NULL);
    U32 expire_time = now - PENDING_TIMEOUT_SECS;

    PendingQueue::iterator iter = mPendingQueue.find(id);

    if (iter == mPendingQueue.end()
        || (iter->second < expire_time) )
    {
        mPendingQueue[id] = now;
        return false;
    }

    return true;
}

void LLCacheName::Impl::processUUIDRequest(LLMessageSystem* msg, bool isGroup)
{
    // May run directly on LLUDPReceiverThread. Must not build or send any
    // outbound message here -- always defer via addPending() instead, so
    // the actual reply is sent later from processPendingReplies() on the
    // main thread.
    LLMutexLock lock(&mMutex);

    // You should only get this message if the cache is at the simulator
    // level, hence having an upstream provider.
    if (!mUpstreamHost.isOk())
    {
        LL_WARNS() << "LLCacheName - got UUID name/group request, but no upstream provider!" << LL_ENDL;
        return;
    }

    // Note: not fully synchronized with the main thread
    LLHost fromHost = msg->getSender();

    S32 count = msg->getNumberOfBlocksFast(_PREHASH_UUIDNameBlock);
    for(S32 i = 0; i < count; ++i)
    {
        LLUUID id;
        msg->getUUIDFast(_PREHASH_UUIDNameBlock, _PREHASH_ID, id, i);
        LLCacheNameEntry* entry = get_ptr_in_map(mCache, id);
        if(entry)
        {
            if (isGroup != entry->mIsGroup)
            {
                LL_WARNS() << "LLCacheName - Asked for "
                        << (isGroup ? "group" : "user") << " name, "
                        << "but found "
                        << (entry->mIsGroup ? "group" : "user")
                        << ": " << id << LL_ENDL;
            }
            else
            {
                // ...it's in the cache: queue it up to be sent back as a
                // reply from the main thread.
                addPending(id, fromHost);
            }
        }
        else
        {
            if (!isRequestPending(id))
            {
                if (isGroup)
                {
                    mAskGroupQueue.insert(id);
                }
                else
                {
                    mAskNameQueue.insert(id);
                }
            }

            addPending(id, fromHost);
        }
    }
}



void LLCacheName::Impl::processUUIDReply(LLMessageSystem* msg, bool isGroup)
{
    // Runs directly on LLUDPReceiverThread.
    // Note that callbacks must happen on main thread
    LLMutexLock lock(&mMutex);

    S32 count = msg->getNumberOfBlocksFast(_PREHASH_UUIDNameBlock);
    for (S32 i = 0; i < count; ++i)
    {
        LLUUID id;
        msg->getUUIDFast(_PREHASH_UUIDNameBlock, _PREHASH_ID, id, i);
        LLCacheNameEntry* entry = get_ptr_in_map(mCache, id);
        if (!entry)
        {
            entry = new LLCacheNameEntry;
            mCache[id] = entry;
        }

        mPendingQueue.erase(id);

        entry->mIsGroup = isGroup;
        entry->mCreateTime = (U32)time(NULL);
        if (!isGroup)
        {
            msg->getStringFast(_PREHASH_UUIDNameBlock, _PREHASH_FirstName, entry->mFirstName, i);
            msg->getStringFast(_PREHASH_UUIDNameBlock, _PREHASH_LastName, entry->mLastName, i);
        }
        else
        {   // is group
            msg->getStringFast(_PREHASH_UUIDNameBlock, _PREHASH_GroupName, entry->mGroupName, i);
            LLStringFn::replace_ascii_controlchars(entry->mGroupName, LL_UNKNOWN_CHAR);
        }

        if (!isGroup)
        {
            // NOTE: Very occasionally the server sends down a full name
            // in the first name field with an empty last name, for example,
            // first = "Ladanie1 Resident", last = "".
            // I cannot reproduce this, nor can I find a bug in the server code.
            // Ensure "Resident" does not appear via cleanFullName, because
            // buildFullName only checks last name. JC
            std::string full_name;
            if (entry->mLastName.empty())
            {
                full_name = cleanFullName(entry->mFirstName);

                //fix what we are putting in the cache
                entry->mFirstName = full_name;
                entry->mLastName = "Resident";
            }
            else
            {
                full_name = LLCacheName::buildFullName(entry->mFirstName, entry->mLastName);
            }
            mReverseCache[full_name] = id;
            mSignalQueue.push_back({ id, full_name, false });
        }
        else
        {
            mReverseCache[entry->mGroupName] = id;
            mSignalQueue.push_back({ id, entry->mGroupName, true });
        }
    }
}



// static call back functions

void LLCacheName::Impl::handleUUIDNameReply(LLMessageSystem* msg, void** userData)
{
    ((LLCacheName::Impl*)userData)->processUUIDReply(msg, false);
}

void LLCacheName::Impl::handleUUIDNameRequest(LLMessageSystem* msg, void** userData)
{
    ((LLCacheName::Impl*)userData)->processUUIDRequest(msg, false);
}

void LLCacheName::Impl::handleUUIDGroupNameRequest(LLMessageSystem* msg, void** userData)
{
    ((LLCacheName::Impl*)userData)->processUUIDRequest(msg, true);
}

void LLCacheName::Impl::handleUUIDGroupNameReply(LLMessageSystem* msg, void** userData)
{
    ((LLCacheName::Impl*)userData)->processUUIDReply(msg, true);
}
