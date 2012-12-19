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
#include "llexperiencecache.h"

#include "llavatarname.h"
#include "llframetimer.h"
#include "llhttpclient.h"
#include "llsdserialize.h"
#include <set>
#include <map>
#include "boost/tokenizer.hpp"





namespace LLExperienceCache
{
	const std::string& MAP_KEY = PUBLIC_KEY;
	std::string sLookupURL;

	typedef std::map<LLUUID, std::string> ask_queue_t;
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

	void processExperience( const LLUUID& public_key, const LLSD& experience ) 
	{
		sCache[public_key]=experience;
		LLSD & row = sCache[public_key];

		if(row.has("expires"))
		{
			row["expires"] = row["expires"].asReal() + LLFrameTimer::getTotalSeconds();
		}

		if(row.has(PUBLIC_KEY))
		{
			sPendingQueue.erase(row[PUBLIC_KEY].asUUID());
		}

		if(row.has(PRIVATE_KEY))
		{
			sPendingQueue.erase(row[PRIVATE_KEY].asUUID());
		}

		if(row.has(CREATOR_KEY))
		{
			sPendingQueue.erase(row[CREATOR_KEY].asUUID());
		}

			
		//signal
		signal_map_t::iterator sig_it =	sSignalMap.find(public_key);
		if (sig_it != sSignalMap.end())
		{
			callback_signal_t* signal = sig_it->second;
			(*signal)(experience);

			sSignalMap.erase(public_key);

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
				LL_WARNS("ExperienceCache") 
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

		LLSD experiences = data["experiences"];

		LLUUID public_key;
		LLSD::map_const_iterator it = experiences.beginMap();
		for(; it != experiences.endMap() ; ++it)
		{
			public_key.set(it->first);
			sCache[public_key]=it->second;
		}

		LL_INFOS("ExperienceCache") << "loaded " << sCache.size() << LL_ENDL;
	}

	void exportFile(std::ostream& ostr)
	{
		LLSD experiences;

		cache_t::const_iterator it =sCache.begin();
		for( ; it != sCache.end() ; ++it)
		{
			if(!it->second.has(PUBLIC_KEY) || it->second[PUBLIC_KEY].asUUID().isNull() ||
				it->second.has("error"))
				continue;

			experiences[it->first.asString()] = it->second;
		}

		LLSD data;
		data["experiences"] = experiences;

		LLSDSerialize::toPrettyXML(data, ostr);
	}

	class LLExperienceResponder : public LLHTTPClient::Responder
	{
	public:
		LLExperienceResponder(const ask_queue_t& keys)
			:mKeys(keys)
		{

		}

		virtual void completedHeader(U32 status, const std::string& reason, const LLSD& content)
		{
			mHeaders = content;
		}

		virtual void result(const LLSD& content)
		{
			LLSD experiences = content["experience_keys"];
			LLSD::array_const_iterator it = experiences.beginArray();
			for( /**/ ; it != experiences.endArray(); ++it)
			{
				const LLSD& row = *it;
				LLUUID public_key = row[PUBLIC_KEY].asUUID();


				LL_INFOS("ExperienceCache") << "Received result for " << public_key 
					<< " display '" << row[LLExperienceCache::NAME].asString() << "'" << LL_ENDL ;

				processExperience(public_key, row);
			}

			LLSD error_ids = content["error_ids"];
			LLSD::map_const_iterator errIt = error_ids.beginMap();
			for( /**/ ; errIt != error_ids.endMap() ; ++errIt )
			{
				LLUUID id = LLUUID(errIt->first);			
				for( it = errIt->second.beginArray(); it != errIt->second.endArray() ; ++it)
				{
					LL_INFOS("ExperienceCache") << "Clearing error result for " << id 
						<< " of type '" << it->asString() << "'" << LL_ENDL ;

					erase(id, it->asString());
				}
			}

			LL_INFOS("ExperienceCache") << sCache.size() << " cached experiences" << LL_ENDL;
		}

		virtual void error(U32 status, const std::string& reason)
		{
 			LL_WARNS("ExperienceCache") << "Request failed "<<status<<" "<<reason<< LL_ENDL;
 			// We're going to construct a dummy record and cache it for a while,
 			// either briefly for a 503 Service Unavailable, or longer for other
 			// errors.
 			F64 retry_timestamp = errorRetryTimestamp(status);
 
 
 			// Add dummy records for all agent IDs in this request
 			ask_queue_t::const_iterator it = mKeys.begin();
 			for ( ; it != mKeys.end(); ++it)
			{
				LLSD exp;
				exp["expires"]=retry_timestamp;
				exp[it->second] = it->first;
				exp["key_type"] = it->second;
				exp["uuid"] = it->first;
				exp["error"] = (LLSD::Integer)status;
 				LLExperienceCache::processExperience(it->first, exp);
 			}

		}

		// Return time to retry a request that generated an error, based on
		// error type and headers.  Return value is seconds-since-epoch.
		F64 errorRetryTimestamp(S32 status)
		{

			// Retry-After takes priority
			LLSD retry_after = mHeaders["retry-after"];
			if (retry_after.isDefined())
			{
				// We only support the delta-seconds type
				S32 delta_seconds = retry_after.asInteger();
				if (delta_seconds > 0)
				{
					// ...valid delta-seconds
					return F64(delta_seconds);
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
				return SERVICE_UNAVAILABLE_DELAY;
			}
			else if (status == 499)
			{
				// ...we were probably too busy, retry quickly
				const F64 BUSY_DELAY = 10.0; // 10 seconds
				return BUSY_DELAY;

			}
			else
			{
				// ...other unexpected error
				const F64 DEFAULT_DELAY = 3600.0; // 1 hour
				return DEFAULT_DELAY;
			}
		}

	private:
		ask_queue_t mKeys;
		LLSD mHeaders;
	};

	void requestExperiences() 
	{
		if(sAskQueue.empty() || sLookupURL.empty())
			return;

		F64 now = LLFrameTimer::getTotalSeconds();

		const U32 EXP_URL_SEND_THRESHOLD = 3000;


		std::ostringstream ostr;

		ask_queue_t keys;

		ostr << sLookupURL;

		char arg='?';

		int request_count = 0;
		for(ask_queue_t::const_iterator it = sAskQueue.begin() ; it != sAskQueue.end() && request_count < sMaximumLookups; ++it)
		{
			const LLUUID& key = it->first;
			const std::string& key_type = it->second;

			ostr << arg << key_type << '=' << key.asString() ;
		
			keys[key]=key_type;
			request_count++;

			sPendingQueue[key] = now;

			arg='&';

			if(ostr.tellp() > EXP_URL_SEND_THRESHOLD)
			{
				LL_INFOS("ExperienceCache") <<  " query: " << ostr.str() << LL_ENDL;
				LLHTTPClient::get(ostr.str(), new LLExperienceResponder(keys));
				ostr.clear();
				ostr.str(sLookupURL);
				arg='?';
				keys.clear();
			}
		}

		if(ostr.tellp() > sLookupURL.size())
		{
			LL_INFOS("ExperienceCache") <<  " query: " << ostr.str() << LL_ENDL;
			LLHTTPClient::get(ostr.str(), new LLExperienceResponder(keys));
		}

		sAskQueue.clear();
	}

	bool isRequestPending(const LLUUID& public_key)
	{
		bool isPending = false;
		const F64 PENDING_TIMEOUT_SECS = 5.0 * 60.0;

		pending_queue_t::const_iterator it = sPendingQueue.find(public_key);

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
		if(!sLookupURL.empty())
		{
			sLookupURL += "id/";
		}
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

	struct FindByKey
	{
		FindByKey(const LLUUID& key, const std::string& key_type):mKey(key), mKeyType(key_type){}
		const LLUUID& mKey;
		const std::string& mKeyType;

		bool operator()(cache_t::value_type& experience)
		{
			return experience.second.has(mKeyType) && experience.second[mKeyType].asUUID() == mKey;
		}
	};


	cache_t::iterator Find(const LLUUID& key, const std::string& key_type)
	{
		LL_INFOS("ExperienceCache") << " searching for " << key << " of type " << key_type << LL_ENDL;
		if(key_type == MAP_KEY)
		{
			return sCache.find(key);
		}

		return std::find_if(sCache.begin(), sCache.end(), FindByKey(key, key_type));
	}


	void erase( const LLUUID& key, const std::string& key_type )
	{
		cache_t::iterator it = Find(key, key_type);
				
		if(it != sCache.end())
		{
			sCache.erase(it);
		}
	}

	void eraseExpired()
	{
		F64 now = LLFrameTimer::getTotalSeconds();
		cache_t::iterator it = sCache.begin();
		while (it != sCache.end())
		{
			cache_t::iterator cur = it;
			LLSD& exp = cur->second;
			++it;
			if(exp.has("expires") && exp["expires"].asReal() < now)
			{
				if(exp.has("key_type") && exp.has("uuid"))
				{
					fetch(exp["uuid"].asUUID(), exp["key_type"].asString(), true);
					sCache.erase(cur);
				}
				else if(exp.has(MAP_KEY))
				{
					LLUUID id = exp[MAP_KEY];
					if(!id.isNull())
					{
						fetch(id, MAP_KEY, true);
					}
				}
			}
		}
	}

	
	bool fetch( const LLUUID& key, const std::string& key_type, bool refresh/* = true*/ ) 
	{
		if(!key.isNull() && !isRequestPending(key) && (refresh || Find(key, key_type)==sCache.end()))
		{
			LL_INFOS("ExperienceCache") << " queue request for " << key_type << " " << key << LL_ENDL ;
			sAskQueue[key]=key_type;

			return true;
		}
		return false;
	}

	void insert(const LLSD& experience_data )
	{
		if(experience_data.has(MAP_KEY))
		{
			sCache[experience_data[MAP_KEY].asUUID()]=experience_data;
		}
		else
		{
			LL_WARNS("ExperienceCache") << ": Ignoring cache insert of experience which is missing " << MAP_KEY << LL_ENDL;
		}
	}

	bool get( const LLUUID& key, const std::string& key_type, LLSD& experience_data )
	{
		if(key.isNull()) return false;
		cache_t::const_iterator it = Find(key, key_type);
	
		if (it != sCache.end())
		{
			experience_data = it->second;
			return true;
		}
		
		fetch(key, key_type);

		return false;
	}


	void get( const LLUUID& key, const std::string& key_type, callback_slot_t slot )
	{
		if(key.isNull()) return;

		cache_t::const_iterator it = Find(key, key_type);
		if (it != sCache.end())
		{
			// ...name already exists in cache, fire callback now
			callback_signal_t signal;
			signal.connect(slot);
			
			signal(it->second);
			return;
		}

		fetch(key, key_type);

		// always store additional callback, even if request is pending
		signal_map_t::iterator sig_it = sSignalMap.find(key);
		if (sig_it == sSignalMap.end())
		{
			// ...new callback for this id
			callback_signal_t* signal = new callback_signal_t();
			signal->connect(slot);
			sSignalMap[key] = signal;
		}
		else
		{
			// ...existing callback, bind additional slot
			callback_signal_t* signal = sig_it->second;
			signal->connect(slot);
		}
	}

}
