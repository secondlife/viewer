/** 
 * @file llavatarnamecache.h
 * @brief Provides lookup of avatar SLIDs ("bobsmith123") and display names
 * ("James Cook") from avatar UUIDs.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LLAVATARNAMECACHE_H
#define LLAVATARNAMECACHE_H

#include "llavatarname.h"	// for convenience
#include "llsingleton.h"
#include <boost/signals2.hpp>
#include <set>

class LLSD;
class LLUUID;

class LLAvatarNameCache : public LLSingleton<LLAvatarNameCache>
{
	LLSINGLETON(LLAvatarNameCache);
	~LLAvatarNameCache();
public:
	typedef boost::signals2::signal<void (void)> use_display_name_signal_t;

	// Import/export the name cache to file.
	bool importFile(std::istream& istr);
	void exportFile(std::ostream& ostr);

	// On the viewer, usually a simulator capabilities.
	// If empty, name cache will fall back to using legacy name lookup system.
	void setNameLookupURL(const std::string& name_lookup_url);

	// Do we have a valid lookup URL, i.e. are we trying to use the
	// more recent display name lookup system?
	bool hasNameLookupURL();
	void setUsePeopleAPI(bool use_api);
	bool usePeopleAPI();
	
	// Periodically makes a batch request for display names not already in
	// cache. Called once per frame.
	void idle();

	// If name is in cache, returns true and fills in provided LLAvatarName
	// otherwise returns false.
	static bool get(const LLUUID& agent_id, LLAvatarName *av_name);
	bool getName(const LLUUID& agent_id, LLAvatarName *av_name);

	// Callback types for get() below
	typedef boost::signals2::signal<
		void (const LLUUID& agent_id, const LLAvatarName& av_name)>
			callback_signal_t;
	typedef callback_signal_t::slot_type callback_slot_t;
	typedef boost::signals2::connection callback_connection_t;

	// Fetches name information and calls callbacks.
	// If name information is in cache, callbacks will be called immediately.
	static callback_connection_t get(const LLUUID& agent_id, callback_slot_t slot);
	callback_connection_t getNameCallback(const LLUUID& agent_id, callback_slot_t slot);

	// Set display name: flips the switch and triggers the callbacks.
	void setUseDisplayNames(bool use);
	
	void setUseUsernames(bool use);

	void insert(const LLUUID& agent_id, const LLAvatarName& av_name);
	void erase(const LLUUID& agent_id);

	// A way to find agent id by UUID, very slow, also unreliable
	// since it doesn't request names, just serch exsisting ones
	// that are likely not in cache.
	//
	// Todo: Find a way to remove this.
	// Curently this method is used for chat history and in some cases notices.
	LLUUID findIdByName(const std::string& name);

	/// Provide some fallback for agents that return errors.
	void handleAgentError(const LLUUID& agent_id);

	// Compute name expiration time from HTTP Cache-Control header,
	// or return default value, in seconds from epoch.
    F64 nameExpirationFromHeaders(const LLSD& headers);

	void addUseDisplayNamesCallback(const use_display_name_signal_t::slot_type& cb);

private:
    // Handle name response off network.
    void processName(const LLUUID& agent_id,
        const LLAvatarName& av_name);

    void requestNamesViaCapability();

    // Legacy name system callbacks
    static void legacyNameCallback(const LLUUID& agent_id,
        const std::string& full_name,
        bool is_group);
    static void legacyNameFetch(const LLUUID& agent_id,
        const std::string& full_name,
        bool is_group);

    void requestNamesViaLegacy();

    // Do a single callback to a given slot
    void fireSignal(const LLUUID& agent_id,
        const callback_slot_t& slot,
        const LLAvatarName& av_name);

    // Is a request in-flight over the network?
    bool isRequestPending(const LLUUID& agent_id);

    // Erase expired names from cache
    void eraseUnrefreshed();

    bool expirationFromCacheControl(const LLSD& headers, F64 *expires);

    // This is a coroutine.
    static void requestAvatarNameCache_(std::string url, std::vector<LLUUID> agentIds);

    void handleAvNameCacheSuccess(const LLSD &data, const LLSD &httpResult);

private:

    use_display_name_signal_t mUseDisplayNamesSignal;

    // Cache starts in a paused state until we can determine if the
    // current region supports display names.
    bool mRunning;

    // Use the People API (modern) for fetching name if true. Use the old legacy protocol if false.
    // For testing, there's a UsePeopleAPI setting that can be flipped (must restart viewer).
    bool mUsePeopleAPI;

    // Base lookup URL for name service.
    // On simulator, loaded from indra.xml
    // On viewer, usually a simulator capability (at People API team's request)
    // Includes the trailing slash, like "http://pdp60.lindenlab.com:8000/agents/"
    std::string mNameLookupURL;

    // Accumulated agent IDs for next query against service
    typedef std::set<LLUUID> ask_queue_t;
    ask_queue_t mAskQueue;

    // Agent IDs that have been requested, but with no reply.
    // Maps agent ID to frame time request was made.
    typedef std::map<LLUUID, F64> pending_queue_t;
    pending_queue_t mPendingQueue;

    // Callbacks to fire when we received a name.
    // May have multiple callbacks for a single ID, which are
    // represented as multiple slots bound to the signal.
    // Avoid copying signals via pointers.
    typedef std::map<LLUUID, callback_signal_t*> signal_map_t;
    signal_map_t mSignalMap;

    // The cache at last, i.e. avatar names we know about.
    typedef std::map<LLUUID, LLAvatarName> cache_t;
    cache_t mCache;

    // Time when unrefreshed cached names were checked last.
    F64 mLastExpireCheck;
};

// Parse a cache-control header to get the max-age delta-seconds.
// Returns true if header has max-age param and it parses correctly.
// Exported here to ease unit testing.
bool max_age_from_cache_control(const std::string& cache_control, S32 *max_age);

#endif
