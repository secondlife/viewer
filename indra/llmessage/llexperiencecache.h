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

#include <string>

class LLUUID;


class LLExperienceData
{
public:
	bool fromLLSD(const LLSD& sd);
	LLSD asLLSD() const;


	std::string mDisplayName;
	std::string mDescription;
};



namespace LLExperienceCache
{
	void setLookupURL(const std::string& lookup_url);
	bool hasLookupURL();


	void idle();
	void exportFile(std::ostream& ostr);
	void importFile(std::istream& istr);
	void initClass(bool running);

	void erase(const LLUUID& agent_id);
	void fetch(const LLUUID& agent_id);
	void insert(const LLUUID& agent_id, const LLExperienceData& experience_data);
	bool get(const LLUUID& agent_id, LLExperienceData* experience_data);

	typedef std::map<LLUUID, LLExperienceData> cache_t;

	const cache_t& getCached();
};

#endif // LL_LLEXPERIENCECACHE_H
