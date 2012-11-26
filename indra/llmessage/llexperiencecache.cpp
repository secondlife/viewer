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
#include "boost\tokenizer.hpp"

#include "llexperiencecache.h"



namespace LLExperienceCache
{
	std::string sLookupURL;

	typedef std::set<LLUUID> ask_queue_t;
	ask_queue_t sAskQueue;

	typedef std::map<LLUUID, F64> pending_queue_t;
	pending_queue_t sPendingQueue;

	cache_t sCache;
	int sMaximumLookups = 10;

	LLFrameTimer sRequestTimer;

	// Periodically clean out expired entries from the cache
	LLFrameTimer sEraseExpiredTimer;

	// May have multiple callbacks for a single ID, which are
	// represented as multiple slots bound to the signal.
	// Avoid copying signals via pointers.
	typedef std::map<LLUUID, callback_signal_t*> signal_map_t;
	signal_map_t sSignalMap;


	bool max_age_from_cache_control(const std::string& cache_control, S32 *max_age);
	void eraseExpired();

	void processExperience( const LLUUID& agent_id, const LLExperienceData& experience ) 
	{
		sCache[agent_id]=experience;

		sPendingQueue.erase(agent_id);
			
		//signal
		signal_map_t::iterator sig_it =	sSignalMap.find(agent_id);
		if (sig_it != sSignalMap.end())
		{
			callback_signal_t* signal = sig_it->second;
			(*signal)(agent_id, experience);

			sSignalMap.erase(agent_id);

			delete signal;
		}
	}

	void initClass( )
	{
	}

	const cache_t& getCached()
	{
		return sCache;
	}

	void setMaximumLookups( int maximumLookups)
	{
		sMaximumLookups = maximumLookups;
	}


	bool expirationFromCacheControl(LLSD headers, F64 *expires)
	{
		// Allow the header to override the default
		LLSD cache_control_header = headers["cache-control"];
		if (cache_control_header.isDefined())
		{
			S32 max_age = 0;
			std::string cache_control = cache_control_header.asString();
			if (max_age_from_cache_control(cache_control, &max_age))
			{
				LL_DEBUGS("ExperienceCache") 
					<< "got expiration from headers, max_age " << max_age 
					<< LL_ENDL;
				F64 now = LLFrameTimer::getTotalSeconds();
				*expires = now + (F64)max_age;
				return true;
			}
		}
		return false;
	}


	static const std::string MAX_AGE("max-age");
	static const boost::char_separator<char> EQUALS_SEPARATOR("=");
	static const boost::char_separator<char> COMMA_SEPARATOR(",");

	bool max_age_from_cache_control(const std::string& cache_control, S32 *max_age)
	{
		// Split the string on "," to get a list of directives
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		tokenizer directives(cache_control, COMMA_SEPARATOR);

		tokenizer::iterator token_it = directives.begin();
		for ( ; token_it != directives.end(); ++token_it)
		{
			// Tokens may have leading or trailing whitespace
			std::string token = *token_it;
			LLStringUtil::trim(token);

			if (token.compare(0, MAX_AGE.size(), MAX_AGE) == 0)
			{
				// ...this token starts with max-age, so let's chop it up by "="
				tokenizer subtokens(token, EQUALS_SEPARATOR);
				tokenizer::iterator subtoken_it = subtokens.begin();

				// Must have a token
				if (subtoken_it == subtokens.end()) return false;
				std::string subtoken = *subtoken_it;

				// Must exactly equal "max-age"
				LLStringUtil::trim(subtoken);
				if (subtoken != MAX_AGE) return false;

				// Must have another token
				++subtoken_it;
				if (subtoken_it == subtokens.end()) return false;
				subtoken = *subtoken_it;

				// Must be a valid integer
				// *NOTE: atoi() returns 0 for invalid values, so we have to
				// check the string first.
				// *TODO: Do servers ever send "0000" for zero?  We don't handle it
				LLStringUtil::trim(subtoken);
				if (subtoken == "0")
				{
					*max_age = 0;
					return true;
				}
				S32 val = atoi( subtoken.c_str() );
				if (val > 0 && val < S32_MAX)
				{
					*max_age = val;
					return true;
				}
				return false;
			}
		}
		return false;
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

					processExperience(agent_id, experience);
				}
			}

			LLSD unresolved_agents = content["bad_ids"];
			S32 num_unresolved = unresolved_agents.size();
			if(num_unresolved > 0)
			{
				LL_DEBUGS("ExperienceCache") << __FUNCTION__ << "Ignoring " << num_unresolved 
					<< " bad ids" << LL_ENDL ;
			}

			LL_DEBUGS("ExperienceCache") << __FUNCTION__ << sCache.size() << " cached experiences" << LL_ENDL;
		}

		void error(U32 status, const std::string& reason)
		{
			// We're going to construct a dummy record and cache it for a while,
			// either briefly for a 503 Service Unavailable, or longer for other
			// errors.
			F64 retry_timestamp = errorRetryTimestamp(status);

			LLExperienceData experience;
			experience.mDisplayName = LLExperienceCache::DUMMY_NAME;
			experience.mDescription = LLExperienceCache::DUMMY_NAME;
			experience.mExpires = retry_timestamp;

			// Add dummy records for all agent IDs in this request
			std::vector<LLUUID>::const_iterator it = mAgentIds.begin();
			for ( ; it != mAgentIds.end(); ++it)
			{
				LLExperienceCache::processExperience((*it), experience);
			}
		}

		// Return time to retry a request that generated an error, based on
		// error type and headers.  Return value is seconds-since-epoch.
		F64 errorRetryTimestamp(S32 status)
		{
			F64 now = LLFrameTimer::getTotalSeconds();

			// Retry-After takes priority
			LLSD retry_after = mHeaders["retry-after"];
			if (retry_after.isDefined())
			{
				// We only support the delta-seconds type
				S32 delta_seconds = retry_after.asInteger();
				if (delta_seconds > 0)
				{
					// ...valid delta-seconds
					return now + F64(delta_seconds);
				}
			}

			// If no Retry-After, look for Cache-Control max-age
			F64 expires = 0.0;
			if (LLExperienceCache::expirationFromCacheControl(mHeaders, &expires))
			{
				return expires;
			}

			// No information in header, make a guess
			if (status == 503)
			{
				// ...service unavailable, retry soon
				const F64 SERVICE_UNAVAILABLE_DELAY = 600.0; // 10 min
				return now + SERVICE_UNAVAILABLE_DELAY;
			}
			else
			{
				// ...other unexpected error
				const F64 DEFAULT_DELAY = 3600.0; // 1 hour
				return now + DEFAULT_DELAY;
			}
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

		int request_count = 0;
		for(ask_queue_t::const_iterator it = sAskQueue.begin() ; it != sAskQueue.end() && request_count < sMaximumLookups; ++it)
		{
			const LLUUID& agent_id = *it;
		
			url += arg;
			url += agent_id.asString();
			agent_ids.push_back(agent_id);
			request_count++;

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

		const F32 SECS_BETWEEN_REQUESTS = 0.1f;
		if (!sRequestTimer.checkExpirationAndReset(SECS_BETWEEN_REQUESTS))
		{
			return;
		}

		// Must be large relative to above
		const F32 ERASE_EXPIRED_TIMEOUT = 60.f; // seconds
		if (sEraseExpiredTimer.checkExpirationAndReset(ERASE_EXPIRED_TIMEOUT))
		{
			eraseExpired();
		}


		if(!sAskQueue.empty())
		{
			requestExperiences();
		}
	}

	void erase( const LLUUID& agent_id )
	{
		sCache.erase(agent_id);
	}

	void eraseExpired()
	{
		S32 expired_count = 0;
		F64 now = LLFrameTimer::getTotalSeconds();
		cache_t::iterator it = sCache.begin();
		while (it != sCache.end())
		{
			cache_t::iterator cur = it;
			++it;
			const LLExperienceData& experience = cur->second;
			if (experience.mExpires < now)
			{
				sCache.erase(cur);
				expired_count++;
			}
		}
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
		cache_t::const_iterator it = sCache.find(agent_id);
		if (it != sCache.end())
		{
			llassert(experience_data);
			*experience_data = it->second;
			return true;
		}
		
		if(!isRequestPending(agent_id))
		{
			fetch(agent_id);
		}

		return false;
	}



	void get( const LLUUID& agent_id, callback_slot_t slot )
	{
		cache_t::const_iterator it = sCache.find(agent_id);
		if (it != sCache.end())
		{
			// ...name already exists in cache, fire callback now
			callback_signal_t signal;
			signal.connect(slot);
			signal(agent_id, it->second);
			return;
		}

		// schedule a request
		if (!isRequestPending(agent_id))
		{
			sAskQueue.insert(agent_id);
		}

		// always store additional callback, even if request is pending
		signal_map_t::iterator sig_it = sSignalMap.find(agent_id);
		if (sig_it == sSignalMap.end())
		{
			// ...new callback for this id
			callback_signal_t* signal = new callback_signal_t();
			signal->connect(slot);
			sSignalMap[agent_id] = signal;
		}
		else
		{
			// ...existing callback, bind additional slot
			callback_signal_t* signal = sig_it->second;
			signal->connect(slot);
		}
	}

}

static const std::string EXPERIENCE_NAME("username");
static const std::string EXPERIENCE_DESCRIPTION("display_name");
static const std::string EXPERIENCE_EXPIRATION("display_name_expires");

bool LLExperienceData::fromLLSD( const LLSD& sd )
{
	mDisplayName = sd[EXPERIENCE_NAME].asString();
	mDescription = sd[EXPERIENCE_DESCRIPTION].asString();
	LLDate expiration = sd[EXPERIENCE_EXPIRATION];
	mExpires = expiration.secondsSinceEpoch();

	if(mDisplayName.empty() || mDescription.empty()) return false;

	mDescription += " % Hey, this is a description!";
	return true;
}

LLSD LLExperienceData::asLLSD() const
{
	LLSD sd;
	sd[EXPERIENCE_NAME] = mDisplayName;
	sd[EXPERIENCE_DESCRIPTION] = mDescription.substr(0, llmin(mDescription.size(),mDescription.find(" %")));
	sd[EXPERIENCE_EXPIRATION] = LLDate(mExpires);

	return sd;
}
