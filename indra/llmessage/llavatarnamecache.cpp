/** 
 * @file llavatarnamecache.cpp
 * @brief Provides lookup of avatar SLIDs ("bobsmith123") and display names
 * ("James Cook") from avatar UUIDs.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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
#include "linden_common.h"

#include "llavatarnamecache.h"

#include "llcachename.h"	// *TODO: remove
#include "llframetimer.h"
#include "llhttpclient.h"
#include "llsd.h"

#include <map>
#include <set>

namespace LLAvatarNameCache
{
	// *TODO: Defaulted to true for demo, probably want false for initial
	// release and turn it on based on data from login.cgi
	bool sUseDisplayNames = true;

	// Base lookup URL for name service.
	// On simulator, loaded from indra.xml
	// On viewer, sent down from login.cgi
	// from login.cgi
	// Includes the trailing slash, like "http://pdp60.lindenlab.com:8000/"
	std::string sNameServiceURL;

	// accumulated agent IDs for next query against service
	typedef std::set<LLUUID> ask_queue_t;
	ask_queue_t sAskQueue;

	// agent IDs that have been requested, but with no reply
	// maps agent ID to frame time request was made
	typedef std::map<LLUUID, F32> pending_queue_t;
	pending_queue_t sPendingQueue;

	// Callbacks to fire when we received a name.
	// May have multiple callbacks for a single ID, which are
	// represented as multiple slots bound to the signal.
	// Avoid copying signals via pointers.
	typedef std::map<LLUUID, callback_signal_t*> signal_map_t;
	signal_map_t sSignalMap;

	// names we know about
	typedef std::map<LLUUID, LLAvatarName> cache_t;
	cache_t sCache;

	// Send bulk lookup requests a few times a second at most
	// only need per-frame timing resolution
	LLFrameTimer sRequestTimer;

	// Periodically clean out expired entries from the cache
	LLFrameTimer sEraseExpiredTimer;

	void processNameFromService(const LLSD& row);
	void requestNames();
	bool isRequestPending(const LLUUID& agent_id);

	// Erase expired names from cache
	void eraseExpired();
}

/* Sample response:
<?xml version="1.0"?>
<llsd>
  <map>
    <key>agents</key>
    <array>
      <map>
        <key>seconds_until_display_name_update</key>
        <integer/>
        <key>display_name</key>
        <string>MickBot390 LLQABot</string>
        <key>seconds_until_display_name_expires</key>
        <integer/>
        <key>sl_id</key>
        <string>mickbot390.llqabot</string>
        <key>id</key>
        <string>0012809d-7d2d-4c24-9609-af1230a37715</string>
        <key>is_display_name_default</key>
        <boolean>false</boolean>
      </map>
      <map>
        <key>seconds_until_display_name_update</key>
        <integer/>
        <key>display_name</key>
        <string>Bjork Gudmundsdottir</string>
        <key>seconds_until_display_name_expires</key>
        <integer>3600</integer>
        <key>sl_id</key>
        <string>sardonyx.linden</string>
        <key>id</key>
        <string>3941037e-78ab-45f0-b421-bd6e77c1804d</string>
        <key>is_display_name_default</key>
        <boolean>true</boolean>
      </map>
    </array>
  </map>
</llsd>
*/

class LLAvatarNameResponder : public LLHTTPClient::Responder
{
public:
	/*virtual*/ void result(const LLSD& content)
	{
		LLSD agents = content["agents"];
		LLSD::array_const_iterator it = agents.beginArray();
		for ( ; it != agents.endArray(); ++it)
		{
			const LLSD& entry = *it;
			LLAvatarNameCache::processNameFromService(entry);
		}
	}

	/*virtual*/ void error(U32 status, const std::string& reason)
	{
		llinfos << "LLAvatarNameResponder error " << status << " " << reason << llendl;
	}
};

// "expires" is seconds-from-epoch
void LLAvatarNameCache::processNameFromService(const LLSD& row)
{
	LLAvatarName av_name;
	av_name.mSLID = row["sl_id"].asString();
	av_name.mDisplayName = row["display_name"].asString();
	//av_name.mIsDisplayNameDefault = row["is_display_name_default"].asBoolean();

	U32 now = (U32)LLFrameTimer::getTotalSeconds();
	S32 seconds_until_expires = row["seconds_until_display_name_expires"].asInteger();
	av_name.mExpires = now + seconds_until_expires;

	// HACK for pretty stars
	//if (row["last_name"].asString() == "Linden")
	//{
	//	av_name.mBadge = "Person_Star";
	//}

	// Some avatars don't have explicit display names set
	if (av_name.mDisplayName.empty())
	{
		av_name.mDisplayName = av_name.mSLID;
	}

	// HACK: Legacy users have '.' in their SLID
	// JAMESDEBUG TODO: change to using is_display_name_default once that works
	std::string mangled_name = av_name.mDisplayName;
	for (U32 i = 0; i < mangled_name.size(); i++)
	{
		char c = mangled_name[i];
		if (c == ' ')
		{
			mangled_name[i] = '.';
		}
		else
		{
			mangled_name[i] = tolower(c);
		}
	}
	av_name.mIsDisplayNameDefault = (mangled_name == av_name.mSLID);

	// add to cache
	LLUUID agent_id = row["id"].asUUID();
	sCache[agent_id] = av_name;

	sPendingQueue.erase(agent_id);

	// signal everyone waiting on this name
	signal_map_t::iterator sig_it =	sSignalMap.find(agent_id);
	if (sig_it != sSignalMap.end())
	{
		callback_signal_t* signal = sig_it->second;
		(*signal)(agent_id, av_name);

		sSignalMap.erase(agent_id);

		delete signal;
		signal = NULL;
	}
}

void LLAvatarNameCache::requestNames()
{
	// URL format is like:
	// http://pdp60.lindenlab.com:8000/agents/?ids=3941037e-78ab-45f0-b421-bd6e77c1804d&ids=0012809d-7d2d-4c24-9609-af1230a37715&ids=0019aaba-24af-4f0a-aa72-6457953cf7f0
	//
	// Apache can handle URLs of 4096 chars, but let's be conservative
	const U32 NAME_URL_MAX = 4096;
	const U32 NAME_URL_SEND_THRESHOLD = 3000;
	std::string url;
	url.reserve(NAME_URL_MAX);

	ask_queue_t::const_iterator it = sAskQueue.begin();
	for ( ; it != sAskQueue.end(); ++it)
	{
		if (url.empty())
		{
			// ...starting new request
			url += sNameServiceURL;
			url += "agents/?ids=";
		}
		else
		{
			// ...continuing existing request
			url += "&ids=";
		}
		url += it->asString();

		if (url.size() > NAME_URL_SEND_THRESHOLD)
		{
			//llinfos << "requestNames " << url << llendl;
			LLHTTPClient::get(url, new LLAvatarNameResponder());
			url.clear();
		}
	}

	if (!url.empty())
	{
		//llinfos << "requestNames " << url << llendl;
		LLHTTPClient::get(url, new LLAvatarNameResponder());
		url.clear();
	}
}

void LLAvatarNameCache::initClass(const std::string& name_service_url)
{
	setNameServiceURL(name_service_url);
}

void LLAvatarNameCache::cleanupClass()
{
}

void LLAvatarNameCache::importFile(std::istream& istr)
{
}

void LLAvatarNameCache::exportFile(std::ostream& ostr)
{
}

void LLAvatarNameCache::setNameServiceURL(const std::string& name_service_url)
{
	sNameServiceURL = name_service_url;
}

void LLAvatarNameCache::idle()
{
	// 100 ms is the threshold for "user speed" operations, so we can
	// stall for about that long to batch up requests.
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

	if (sAskQueue.empty())
	{
		return;
	}

	requestNames();

	// Move requests from Ask queue to Pending queue
	U32 now = (U32)LLFrameTimer::getTotalSeconds();
	ask_queue_t::const_iterator it = sAskQueue.begin();
	for ( ; it != sAskQueue.end(); ++it)
	{
		sPendingQueue[*it] = now;
	}
	sAskQueue.clear();
}

bool LLAvatarNameCache::isRequestPending(const LLUUID& agent_id)
{
	const U32 PENDING_TIMEOUT_SECS = 5 * 60;
	U32 now = (U32)LLFrameTimer::getTotalSeconds();
	U32 expire_time = now - PENDING_TIMEOUT_SECS;

	pending_queue_t::const_iterator it = sPendingQueue.find(agent_id);
	if (it != sPendingQueue.end())
	{
		bool request_expired = (it->second < expire_time);
		return !request_expired;
	}
	return false;
}

void LLAvatarNameCache::eraseExpired()
{
	U32 now = (U32)LLFrameTimer::getTotalSeconds();
	cache_t::iterator it = sCache.begin();
	while (it != sCache.end())
	{
		cache_t::iterator cur = it;
		++it;
		const LLAvatarName& av_name = cur->second;
		if (av_name.mExpires < now)
		{
			sCache.erase(cur);
		}
	}
}

bool LLAvatarNameCache::get(const LLUUID& agent_id, LLAvatarName *av_name)
{
	std::map<LLUUID,LLAvatarName>::iterator it = sCache.find(agent_id);
	if (it != sCache.end())
	{
		*av_name = it->second;
		return true;
	}

	if (!isRequestPending(agent_id))
	{
		sAskQueue.insert(agent_id);
	}

	return false;
}

void LLAvatarNameCache::get(const LLUUID& agent_id, callback_slot_t slot)
{
	std::map<LLUUID,LLAvatarName>::iterator it = sCache.find(agent_id);
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

// JAMESDEBUG TODO: Eliminate and only route changes through simulator
class LLSetNameResponder : public LLHTTPClient::Responder
{
public:
	LLUUID mAgentID;
	LLAvatarNameCache::set_name_signal_t mSignal;

	LLSetNameResponder(const LLUUID& agent_id,
					   const LLAvatarNameCache::set_name_slot_t& slot)
	:	mAgentID(agent_id),
		mSignal()
	{
		mSignal.connect(slot);
	}

	/*virtual*/ void result(const LLSD& content)
	{
		// force re-fetch
		LLAvatarNameCache::sCache.erase(mAgentID);

		mSignal(true, "", content);
	}

	/*virtual*/ void error(U32 status, const std::string& reason)
	{
		llinfos << "LLSetNameResponder failed " << status
			<< " reason " << reason << llendl;

		mSignal(false, reason, LLSD());
	}
};

// JAMESDEBUG TODO: Eliminate and only route changes through simulator
void LLAvatarNameCache::setDisplayName(const LLUUID& agent_id, 
									   const std::string& display_name,
									   const set_name_slot_t& slot)
{
	LLSD body;
	body["display_name"] = display_name;

	std::string url = sNameServiceURL + "agent/";
	url += agent_id.asString();
	LLHTTPClient::post(url, body, new LLSetNameResponder(agent_id, slot));
}

void LLAvatarNameCache::toggleDisplayNames()
{
	sUseDisplayNames = !sUseDisplayNames;
	// flush our cache
	sCache.clear();
	// force re-lookups
	if (gCacheName)
	{
		gCacheName->clear();
	}
}

bool LLAvatarNameCache::useDisplayNames()
{
	return sUseDisplayNames;
}

void LLAvatarNameCache::erase(const LLUUID& agent_id)
{
	sCache.erase(agent_id);
}
