/** 
 * @file llcachename.h
 * @brief A cache of names from UUIDs.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCACHENAME_H
#define LL_LLCACHENAME_H

class LLMessageSystem;
class LLHost;
class LLUUID;

// agent_id/group_id, first_name, last_name, is_group, user_data
typedef void (*LLCacheNameCallback)(const LLUUID&, const char*, const char*, BOOL, void*);

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

	void addObserver(LLCacheNameCallback callback);
	void removeObserver(LLCacheNameCallback callback);

	void cancelCallback(const LLUUID& id, LLCacheNameCallback callback, void* user_data = NULL);

	// storing cache on disk; for viewer, in name.cache
	void importFile(FILE* fp);
	void exportFile(FILE* fp);

	// If available, copies the first and last name into the strings provided.
	// first must be at least DB_FIRST_NAME_BUF_SIZE characters.
	// last must be at least DB_LAST_NAME_BUF_SIZE characters.
	// If not available, copies the string "waiting".
	// Returns TRUE iff available.
	BOOL getName(const LLUUID& id, char* first, char* last);
	
	// If available, this method copies the group name into the string
	// provided. The caller must allocate at least
	// DB_GROUP_NAME_BUF_SIZE characters. If not available, this
	// method copies the string "waiting". Returns TRUE iff available.
	BOOL getGroupName(const LLUUID& id, char* group);

	// Call the callback with the group or avatar name.
	// If the data is currently available, may call the callback immediatly
	// otherwise, will request the data, and will call the callback when
	// available.  There is no garuntee the callback will ever be called.
	void get(const LLUUID& id, BOOL is_group, LLCacheNameCallback callback, void* user_data = NULL);
	
	// LEGACY
	void getName(const LLUUID& id, LLCacheNameCallback callback, void* user_data = NULL)
			{ get(id, FALSE, callback, user_data); }

	// This method needs to be called from time to time to send out
	// requests.
	void processPending();

	// Expire entries created more than "secs" seconds ago.
	void deleteEntriesOlderThan(S32 secs);

	// Debugging
	void dump();		// Dumps the contents of the cache
	void dumpStats();	// Dumps the sizes of the cache and associated queues.

	static LLString getDefaultName();

private:
	class Impl;
	Impl& impl;
};



extern LLCacheName* gCacheName;

#endif
