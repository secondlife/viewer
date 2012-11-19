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
#include "llavatarname.h"
#include "llframetimer.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include <set>
#include <map>

#include "llexperiencecache.h"



namespace LLExperienceCache
{
	bool sRunning = false;
	std::string sLookupURL;

	typedef std::set<LLUUID> ask_queue_t;
	ask_queue_t sAskQueue;

	typedef std::map<LLUUID, F64> pending_queue_t;
	pending_queue_t sPendingQueue;


	cache_t sCache;

	LLFrameTimer sRequestTimer;



	void processExperience( const LLUUID& agent_id, const LLExperienceData& experience, bool add_to_cache ) 
	{
		if(add_to_cache)
		{
			sCache[agent_id]=experience;

			sPendingQueue.erase(agent_id);


			//signal
		}
	}

	void initClass( bool running )
	{
		sRunning = false;
	}

	const cache_t& getCached()
	{
		return sCache;
	}



	void importFile(std::istream& istr)
	{
		LLSD data;
		S32 parse_count = LLSDSerialize::fromXMLDocument(data, istr);
		if(parse_count < 1) return;

		LLSD agents = data["agents"];

		LLUUID agent_id;
		LLExperienceData experience;
		LLSD::map_const_iterator it = agents.beginMap();
		for(; it != agents.endMap() ; ++it)
		{
			agent_id.set(it->first);
			experience.fromLLSD( it->second);
			sCache[agent_id]=experience;
		}

		LL_INFOS("ExperienceCache") << "loaded " << sCache.size() << LL_ENDL;
	}

	void exportFile(std::ostream& ostr)
	{
		LLSD agents;

		cache_t::const_iterator it =sCache.begin();
		for( ; it != sCache.end() ; ++it)
		{
			agents[it->first.asString()] = it->second.asLLSD();
		}

		LLSD data;
		data["agents"] = agents;

		LLSDSerialize::toPrettyXML(data, ostr);
	}

	class LLExperienceResponder : public LLHTTPClient::Responder
	{
	public:
		LLExperienceResponder(const std::vector<LLUUID>& agent_ids)
			:mAgentIds(agent_ids)
		{

		}

		virtual void completedHeader(U32 status, const std::string& reason, const LLSD& content)
		{
			mHeaders = content;
		}

		virtual void result(const LLSD& content)
		{
			LLSD agents = content["agents"];
			LLSD::array_const_iterator it = agents.beginArray();
			for( /**/ ; it != agents.endArray(); ++it)
			{
				const LLSD& row = *it;
				LLUUID agent_id = row["id"].asUUID();

				LLExperienceData experience;

				if(experience.fromLLSD(row))
				{
					LL_DEBUGS("ExperienceCache") << __FUNCTION__ << "Received result for " << agent_id 
						<< "display '" << experience.mDisplayName << "'" << LL_ENDL ;

					processExperience(agent_id, experience, true);
				}
			}

			LLSD unresolved_agents = content["bad_ids"];
			S32 num_unresolved = unresolved_agents.size();
			if(num_unresolved > 0)
			{
				LL_DEBUGS("ExperienceCache") << __FUNCTION__ << "Ignoreing " << num_unresolved 
					<< " bad ids" << LL_ENDL ;
			}

			LL_DEBUGS("ExperienceCache") << __FUNCTION__ << sCache.size() << " cached experiences" << LL_ENDL;
		}

	private:
		std::vector<LLUUID> mAgentIds;
		LLSD mHeaders;
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
		if(sRunning) 
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

bool LLExperienceData::fromLLSD( const LLSD& sd )
{
	mDisplayName = sd["display_name"].asString();
	mDescription = sd["username"].asString();

	if(mDisplayName.empty() || mDescription.empty()) return false;

	mDescription += " % Hey, this is a description!";
	return true;
}

LLSD LLExperienceData::asLLSD() const
{
	LLSD sd;
	sd["display_name"] = mDisplayName;
	sd["username"] = mDescription.substr(0, llmin(mDescription.size(),mDescription.find(" %")));
	return sd;
}
