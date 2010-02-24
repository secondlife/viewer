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

#include "llcachename.h"	// until we get our own web service
#include "llframetimer.h"
#include "llhttpclient.h"
#include "llsdserialize.h"	// JAMESDEBUG

#include <cctype>	// tolower()
#include <set>

namespace LLAvatarNameCache
{
	bool sUseDisplayNames = false;

	// accumulated agent IDs for next query against service
	typedef std::set<LLUUID> ask_queue_t;
	ask_queue_t sAskQueue;

	// agent IDs that have been requested, but with no reply
	// maps agent ID to frame time request was made
	typedef std::map<LLUUID, F32> pending_queue_t;
	pending_queue_t sPendingQueue;

	// names we know about
	typedef std::map<LLUUID, LLAvatarName> cache_t;
	cache_t sCache;

	// only need per-frame timing resolution
	LLFrameTimer sRequestTimer;

	bool isRequestPending(const LLUUID& agent_id);
}

class LLAvatarNameResponder : public LLHTTPClient::Responder
{
public:
	/*virtual*/ void result(const LLSD& content);
	/*virtual*/ void error(U32 status, const std::string& reason);
};

void LLAvatarNameResponder::result(const LLSD& content)
{
	//std::ostringstream debug;
	//LLSDSerialize::toPrettyXML(content, debug);
	//llinfos << "JAMESDEBUG " << debug.str() << llendl;

	U32 now = (U32)LLFrameTimer::getTotalSeconds();
	LLSD::array_const_iterator it = content.beginArray();
	for ( ; it != content.endArray(); ++it)
	{
		const LLSD& row = *it;

		LLAvatarName av_name;
		av_name.mSLID = row["slid"].asString();
		av_name.mDisplayName = row["display_name"].asString();
		av_name.mLastUpdate = now;

		// HACK for pretty stars
		if (row["last_name"].asString() == "Linden")
		{
			av_name.mBadge = "Person_Star";
		}

		// Some avatars don't have explicit display names set
		if (av_name.mDisplayName.empty())
		{
			// make up a display name
			std::string first_name = row["first_name"].asString();
			std::string last_name = row["last_name"].asString();
			av_name.mDisplayName =
				LLCacheName::buildFullName(first_name, last_name);
			av_name.mIsLegacy = true;
		}

		LLUUID agent_id = row["agent_id"].asUUID();
		LLAvatarNameCache::sCache[agent_id] = av_name;

		LLAvatarNameCache::sPendingQueue.erase(agent_id);

		llinfos << "JAMESDEBUG fetched " << av_name.mDisplayName << llendl;
	}
}

void LLAvatarNameResponder::error(U32 status, const std::string& reason)
{
	llinfos << "JAMESDEBUG error " << status << " " << reason << llendl;
}

// JAMESDEBUG re-enable when display names are turned on???
//static std::string slid_from_full_name(const std::string& full_name)
//{
//	std::string id = full_name;
//	std::string::size_type end = id.length();
//	for (std::string::size_type i = 0; i < end; ++i)
//	{
//		if (id[i] == ' ')
//			id[i] = '.';
//		else
//			id[i] = tolower(id[i]);
//	}
//	return id;
//}

void LLAvatarNameCache::initClass()
{
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

void LLAvatarNameCache::idle()
{
	const F32 SECS_BETWEEN_REQUESTS = 0.2f;  // JAMESDEBUG set to 0.1?
	if (sRequestTimer.checkExpirationAndReset(SECS_BETWEEN_REQUESTS))
	{
		return;
	}

	if (sAskQueue.empty())
	{
		return;
	}

	LLSD body;
	body["agent_ids"] = LLSD::emptyArray();
	LLSD& agent_ids = body["agent_ids"];

	ask_queue_t::const_iterator it = sAskQueue.begin();
	for ( ; it != sAskQueue.end(); ++it)
	{
		agent_ids.append( LLSD( *it ) );
	}

	//std::ostringstream debug;
	//LLSDSerialize::toPrettyXML(body, debug);
	//LL_INFOS("JAMESDEBUG") << debug.str() << LL_ENDL;

	// *TODO: configure the base URL for this
	std::string url = "http://pdp15.lindenlab.com:8050/my-service/agent/display-names/";
	LLHTTPClient::post(url, body, new LLAvatarNameResponder());

	// Move requests from Ask queue to Pending queue
	U32 now = (U32)LLFrameTimer::getTotalSeconds();
	for (it = sAskQueue.begin(); it != sAskQueue.end(); ++it)
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
		bool expired = (it->second < expire_time);
		return !expired;
	}
	return false;
}

bool LLAvatarNameCache::get(const LLUUID& agent_id, LLAvatarName *av_name)
{
	std::map<LLUUID,LLAvatarName>::iterator it = sCache.find(agent_id);
	if (it != sCache.end())
	{
		*av_name = it->second;
		return true;
	}

	// JAMESDEBUG Enable when we turn on display names???
	//std::string full_name;
	//if (gCacheName->getFullName(agent_id, full_name))
	//{
	//	av_name->mSLID = slid_from_full_name(full_name);
	//	av_name->mDisplayName = full_name;
	//	av_name->mBadge = "Generic_Person";
	//	return true;
	//}

	if (!isRequestPending(agent_id))
	{
		std::pair<ask_queue_t::iterator,bool> found = sAskQueue.insert(agent_id);
		if (found.second)
		{
			LL_INFOS("JAMESDEBUG") << "added to ask queue " << agent_id << LL_ENDL;
		}
	}

	return false;
}

void LLAvatarNameCache::get(const LLUUID& agent_id, name_cache_callback_t callback)
{
}

class LLSetNameResponder : public LLHTTPClient::Responder
{
public:
	LLUUID mAgentID;

	LLSetNameResponder(const LLUUID& agent_id) : mAgentID(agent_id) { }

	/*virtual*/ void result(const LLSD& content)
	{
		// force re-fetch
		LLAvatarNameCache::sCache.erase(mAgentID);
		llinfos << "JAMESDEBUG set names worked" << llendl;
	}

	/*virtual*/ void error(U32 status, const std::string& reason)
	{
		llinfos << "JAMESDEBUG set names failed " << status
			<< " reason " << reason << llendl;
	}
};

void LLAvatarNameCache::setDisplayName(const LLUUID& agent_id, const std::string& display_name)
{
	LLSD body;
	body["display_name"] = display_name;

	// *TODO: configure the base URL for this
	std::string url = "http://pdp15.lindenlab.com:8050/my-service/agent/";
	url += agent_id.asString();
	url += "/set-display-name/";
	LLHTTPClient::post(url, body, new LLSetNameResponder(agent_id));
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
