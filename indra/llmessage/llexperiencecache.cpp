/** 
 * @file llexperiencecache.cpp
 * @brief llexperiencecache and related class definitions
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

#include "linden_common.h"
#include "llframetimer.h"
#include "llhttpclient.h"
#include <set>
#include <map>

#include "llexperiencecache.h"

class LLExperienceData
{
public:
	std::string mDisplayName;
};



namespace LLExperienceCache
{
	bool sRunning = true;
	std::string sLookupURL;

	typedef std::set<LLUUID> ask_queue_t;
	ask_queue_t sAskQueue;

	typedef std::map<LLUUID, F64> pending_queue_t;
	pending_queue_t sPendingQueue;


	typedef std::map<LLUUID, LLExperienceData> cache_t;
	cache_t sCache;

	LLFrameTimer sRequestTimer;


	class LLExperienceResponder : public LLHTTPClient::Responder
	{
	public:
		LLExperienceResponder(std::vector<LLUUID> agent_ids)
		{

		}
	};

	void requestExperiences() 
	{
		if(sAskQueue.empty())
			return;

		F64 now = LLFrameTimer::getTotalSeconds();

		const U32 NAME_URL_MAX = 4096;
		const U32 NAME_URL_SEND_THRESHOLD = 3000;

		std::string url;
		url.reserve(NAME_URL_MAX);

		std::vector<LLUUID> agent_ids;
		agent_ids.reserve(128);

		url += sLookupURL;

		std::string arg="?ids=";

		for(ask_queue_t::const_iterator it = sAskQueue.begin(); it != sAskQueue.end() ; ++it)
		{
			const LLUUID& agent_id = *it;
		
			url += arg;
			url += agent_id.asString();
			agent_ids.push_back(agent_id);

			sPendingQueue[agent_id] = now;

			arg[0]='&';

			if(url.size() > NAME_URL_SEND_THRESHOLD)
			{
				 LLHTTPClient::get(url, new LLExperienceResponder(agent_ids));
				 url = sLookupURL;
				 arg[0]='?';
				 agent_ids.clear();
			}
		}

		if(url.size() > sLookupURL.size())
		{
			LLHTTPClient::get(url, new LLExperienceResponder(agent_ids));
		}

		sAskQueue.clear();
	}

	bool isRequestPending(const LLUUID& agent_id)
	{
		bool isPending = false;
		const F64 PENDING_TIMEOUT_SECS = 5.0 * 60.0;

		pending_queue_t::const_iterator it = sPendingQueue.find(agent_id);

		if(it != sPendingQueue.end())
		{
			F64 expire_time = LLFrameTimer::getTotalSeconds() - PENDING_TIMEOUT_SECS;
			isPending = (it->second > expire_time);
		}

		return isPending;
	}


	void setLookupURL( const std::string& lookup_url )
	{
		sLookupURL = lookup_url;
	}

	bool hasLookupURL()
	{
		return !sLookupURL.empty();
	}

	void idle()
	{
		sRunning = true;

		if(!sAskQueue.empty())
		{
			requestExperiences();
		}
	}

	void erase( const LLUUID& agent_id )
	{
		sCache.erase(agent_id);
	}

	void fetch( const LLUUID& agent_id ) 
	{
		LL_DEBUGS("ExperienceCache") << __FUNCTION__ << "queue request for agent" << agent_id << LL_ENDL ;
		sAskQueue.insert(agent_id);
	}

	void insert( const LLUUID& agent_id, const LLExperienceData& experience_data )
	{
		sCache[agent_id]=experience_data;
	}

	bool get( const LLUUID& agent_id, LLExperienceData* experience_data )
	{
		if(!sRunning) 
		{

			cache_t::const_iterator it = sCache.find(agent_id);
			if (it != sCache.end())
			{
				llassert(experience_data);
				*experience_data = it->second;
				return true;
			}
		}
		
		if(!isRequestPending(agent_id))
		{
			fetch(agent_id);
		}

		return false;
	}


}
