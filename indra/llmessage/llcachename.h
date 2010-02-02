/** 
 * @file llcachename.h
 * @brief A cache of names from UUIDs.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

	// janky old format. Remove after a while. Phoenix. 2008-01-30
	void importFile(LLFILE* fp);

	// storing cache on disk; for viewer, in name.cache
	bool importFile(std::istream& istr);
	void exportFile(std::ostream& ostr);

	// If available, copies the first and last name into the strings provided.
	// first must be at least DB_FIRST_NAME_BUF_SIZE characters.
	// last must be at least DB_LAST_NAME_BUF_SIZE characters.
	// If not available, copies the string "waiting".
	// Returns TRUE iff available.
	BOOL getName(const LLUUID& id, std::string& first, std::string& last);
	BOOL getFullName(const LLUUID& id, std::string& fullname);
	
	// Reverse lookup of UUID from name
	BOOL getUUID(const std::string& first, const std::string& last, LLUUID& id);
	BOOL getUUID(const std::string& fullname, LLUUID& id);

	// IDEVO Temporary code
	// Clean up new-style "bobsmith123 Resident" names to "bobsmith123" for display
	static std::string buildFullname(const std::string& first, const std::string& last);
	
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

	static std::string getDefaultName();
	static void LocalizeCacheName(std::string key, std::string value);
	static std::map<std::string, std::string> sCacheName;
private:

	class Impl;
	Impl& impl;
};



extern LLCacheName* gCacheName;

#endif
