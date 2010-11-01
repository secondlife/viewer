/** 
 * @file llcachename.h
 * @brief A cache of names from UUIDs.
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

#ifndef LL_LLCACHENAME_H
#define LL_LLCACHENAME_H

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

class LLMessageSystem;
class LLHost;
class LLUUID;


typedef boost::signals2::signal<void (const LLUUID& id,
                                      const std::string& name,
                                      bool is_group)> LLCacheNameSignal;
typedef LLCacheNameSignal::slot_type LLCacheNameCallback;

// Old callback with user data for compatability
typedef void (*old_callback_t)(const LLUUID&, const std::string&, bool, void*);

// Here's the theory:
// If you request a name that isn't in the cache, it returns "waiting"
// and requests the data.  After the data arrives, you get that on 
// subsequent calls.
// If the data hasn't been updated in an hour, it requests it again,
// but keeps giving you the old value until new data arrives.
// If you haven't requested the data in an hour, it releases it.
class LLCacheName
{
public:
	LLCacheName(LLMessageSystem* msg);
	LLCacheName(LLMessageSystem* msg, const LLHost& upstream_host);
	~LLCacheName();

	// registers the upstream host
	// for viewers, this is the currently connected simulator
	// for simulators, this is the data server
	void setUpstream(const LLHost& upstream_host);

	boost::signals2::connection addObserver(const LLCacheNameCallback& callback);

	// storing cache on disk; for viewer, in name.cache
	bool importFile(std::istream& istr);
	void exportFile(std::ostream& ostr);

	// If available, copies name ("bobsmith123" or "James Linden") into string
	// If not available, copies the string "waiting".
	// Returns TRUE iff available.
	BOOL getFullName(const LLUUID& id, std::string& full_name);

	// Reverse lookup of UUID from name
	BOOL getUUID(const std::string& first, const std::string& last, LLUUID& id);
	BOOL getUUID(const std::string& fullname, LLUUID& id);

	// IDEVO Temporary code
	// Clean up new-style "bobsmith123 Resident" names to "bobsmith123" for display
	static std::string buildFullName(const std::string& first, const std::string& last);

	// Clean up legacy "bobsmith123 Resident" to "bobsmith123"
	// If name does not contain "Resident" returns it unchanged.
	static std::string cleanFullName(const std::string& full_name);
	
	// Converts a standard legacy name to a username
	// "bobsmith123 Resident" -> "bobsmith"
	// "Random Linden" -> "random.linden"
	static std::string buildUsername(const std::string& name);
	
	// Converts a complete display name to a legacy name
	// if possible, otherwise returns the input
	// "Alias (random.linden)" -> "Random Linden"
	// "Something random" -> "Something random"
	static std::string buildLegacyName(const std::string& name);
	
	// If available, this method copies the group name into the string
	// provided. The caller must allocate at least
	// DB_GROUP_NAME_BUF_SIZE characters. If not available, this
	// method copies the string "waiting". Returns TRUE iff available.
	BOOL getGroupName(const LLUUID& id, std::string& group);

	// Call the callback with the group or avatar name.
	// If the data is currently available, may call the callback immediatly
	// otherwise, will request the data, and will call the callback when
	// available.  There is no garuntee the callback will ever be called.
	boost::signals2::connection get(const LLUUID& id, bool is_group, const LLCacheNameCallback& callback);

	// Convenience method for looking up a group name, so you can
	// tell the difference between avatar lookup and group lookup
	// in global searches
	boost::signals2::connection getGroup(const LLUUID& group_id, const LLCacheNameCallback& callback);

	// LEGACY
	boost::signals2::connection get(const LLUUID& id, bool is_group, old_callback_t callback, void* user_data);
	// This method needs to be called from time to time to send out
	// requests.
	void processPending();

	// Expire entries created more than "secs" seconds ago.
	void deleteEntriesOlderThan(S32 secs);

	// Debugging
	void dump();		// Dumps the contents of the cache
	void dumpStats();	// Dumps the sizes of the cache and associated queues.
	void clear();		// Deletes all entries from the cache

	static std::string getDefaultName();

	// Returns "Resident", the default last name for SLID-based accounts
	// that have no last name.
	static std::string getDefaultLastName();

	static void localizeCacheName(std::string key, std::string value);
	static std::map<std::string, std::string> sCacheName;
private:

	class Impl;
	Impl& impl;
};



extern LLCacheName* gCacheName;

#endif
