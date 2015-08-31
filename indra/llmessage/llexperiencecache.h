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
#include "llsd.h"
#include <boost/signals2.hpp>
#include <boost/function.hpp>

class LLSD;
class LLUUID;


class LLExperienceCache: public LLSingleton < LLExperienceCache >
{
    friend class LLSingleton < LLExperienceCache > ;

public:
    typedef boost::function<void(const LLSD &)> Callback_t;

    void erase(const LLUUID& key);
    bool fetch(const LLUUID& key, bool refresh = false);
    void insert(const LLSD& experience_data);
    const LLSD& get(const LLUUID& key);

    // If name information is in cache, callback will be called immediately.
    void get(const LLUUID& key, Callback_t slot);

private:
    // Callback types for get() 
    typedef boost::signals2::signal < void(const LLSD &) > callback_signal_t;
	typedef boost::shared_ptr<callback_signal_t> signal_ptr;
	// May have multiple callbacks for a single ID, which are
	// represented as multiple slots bound to the signal.
	// Avoid copying signals via pointers.
	typedef std::map<LLUUID, signal_ptr> signal_map_t;
	typedef std::map<LLUUID, LLSD> cache_t;
	
	typedef std::set<LLUUID> ask_queue_t;
	
	
	//--------------------------------------------
	static const std::string PRIVATE_KEY;	// "private_id"
	static const std::string MISSING;       // "DoesNotExist"
	
	static const std::string AGENT_ID;      // "agent_id"
	static const std::string GROUP_ID;      // "group_id"
	static const std::string EXPERIENCE_ID;	// "public_id"
	static const std::string NAME;			// "name"
	static const std::string PROPERTIES;	// "properties"
	static const std::string EXPIRES;		// "expiration"  
	static const std::string DESCRIPTION;	// "description"
	static const std::string QUOTA;         // "quota"
	static const std::string MATURITY;      // "maturity"
	static const std::string METADATA;      // "extended_metadata"
	static const std::string SLURL;         // "slurl"
	
	// should be in sync with experience-api/experiences/models.py
	static const int PROPERTY_INVALID;		// 1 << 0
	static const int PROPERTY_PRIVILEGED;	// 1 << 3
	static const int PROPERTY_GRID;			// 1 << 4
	static const int PROPERTY_PRIVATE;		// 1 << 5
	static const int PROPERTY_DISABLED;		// 1 << 6  
	static const int PROPERTY_SUSPENDED;	// 1 << 7
	
	// default values
	static const F64 DEFAULT_EXPIRATION; 	// 600.0
	static const S32 DEFAULT_QUOTA; 		// 128 this is megabytes
	
//--------------------------------------------
    LLExperienceCache();
    virtual ~LLExperienceCache();

    void exportFile(std::ostream& ostr) const;
    void importFile(std::istream& istr);

//--------------------------------------------
    void processExperience(const LLUUID& public_key, const LLSD& experience);

//--------------------------------------------
	cache_t			sCache;
	signal_map_t	sSignalMap;	
	ask_queue_t		sAskQueue;
	
	void eraseExpired();
	
	void setLookupURL(const std::string& lookup_url);
	bool hasLookupURL();

	void setMaximumLookups(int maximumLookups);

	void idle();
	void bootstrap(const LLSD& legacyKeys, int initialExpiration);
	

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
