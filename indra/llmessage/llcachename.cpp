/** 
 * @file llcachename.cpp
 * @brief A hierarchical cache of first and last names queried based on UUID.
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

#include "linden_common.h"

#include "llcachename.h"

// linden library includes
#include "lldbstrings.h"
#include "llframetimer.h"
#include "llhost.h"
#include "llrand.h"
#include "llsdserialize.h"
#include "lluuid.h"
#include "message.h"
#include "llmemtype.h"

// llsd serialization constants
static const std::string AGENTS("agents");
static const std::string GROUPS("groups");
static const std::string CTIME("ctime");
static const std::string FIRST("first");
static const std::string LAST("last");
static const std::string NAME("name");

// We track name requests in flight for up to this long.
// We won't re-request a name during this time
const U32 PENDING_TIMEOUT_SECS = 5 * 60;

// File version number
const S32 CN_FILE_VERSION = 2;

// Globals
LLCacheName* gCacheName = NULL;
std::map<std::string, std::string> LLCacheName::sCacheName;

/// ---------------------------------------------------------------------------
/// class LLCacheNameEntry
/// ---------------------------------------------------------------------------

class LLCacheNameEntry
{
public:
	LLCacheNameEntry();

public:
	bool mIsGroup;
	U32 mCreateTime;	// unix time_t
	std::string mFirstName;		// IDEVO TODO collapse to one field
	std::string mLastName;
	std::string mGroupName;
};

LLCacheNameEntry::LLCacheNameEntry()
	: mIsGroup(false),
	  mCreateTime(0)
{
}


class PendingReply
{
public:
	LLUUID				mID;
	LLCacheNameSignal	mSignal;
	LLHost				mHost;
	
	PendingReply(const LLUUID& id, const LLHost& host)
		: mID(id), mHost(host)
	{
	}
	
	boost::signals2::connection setCallback(const LLCacheNameCallback& cb)
	{
		return mSignal.connect(cb);
	}
	
	void done()			{ mID.setNull(); }
	bool isDone() const	{ return mID.isNull() != FALSE; }
};

class ReplySender
{
public:
	ReplySender(LLMessageSystem* msg);
	~ReplySender();

	void send(const LLUUID& id,
		const LLCacheNameEntry& entry, const LLHost& host);

private:
	void flush();

	LLMessageSystem*	mMsg;
	bool				mPending;
	bool				mCurrIsGroup;
	LLHost				mCurrHost;
};

ReplySender::ReplySender(LLMessageSystem* msg)
	: mMsg(msg), mPending(false), mCurrIsGroup(false)
{ }

ReplySender::~ReplySender()
{
	flush();
}

void ReplySender::send(const LLUUID& id,
	const LLCacheNameEntry& entry, const LLHost& host)
{
	if (mPending)
	{
		if (mCurrIsGroup != entry.mIsGroup
		||  mCurrHost != host)
		{
			flush();
		}
	}

	if (!mPending)
	{
		mPending = true;
		mCurrIsGroup = entry.mIsGroup;
		mCurrHost = host;

		if(mCurrIsGroup)
			mMsg->newMessageFast(_PREHASH_UUIDGroupNameReply);
		else
			mMsg->newMessageFast(_PREHASH_UUIDNameReply);
	}

	mMsg->nextBlockFast(_PREHASH_UUIDNameBlock);
	mMsg->addUUIDFast(_PREHASH_ID, id);
	if(mCurrIsGroup)
	{
		mMsg->addStringFast(_PREHASH_GroupName, entry.mGroupName);
	}
	else
	{
		mMsg->addStringFast(_PREHASH_FirstName,	entry.mFirstName);
		mMsg->addStringFast(_PREHASH_LastName, entry.mLastName);
	}

	if(mMsg->isSendFullFast(_PREHASH_UUIDNameBlock))
	{
		flush();
	}
}

void ReplySender::flush()
{
	if (mPending)
	{
		mMsg->sendReliable(mCurrHost);
		mPending = false;
	}
}


typedef std::set<LLUUID>					AskQueue;
typedef std::list<PendingReply*>			ReplyQueue;
typedef std::map<LLUUID,U32>				PendingQueue;
typedef std::map<LLUUID, LLCacheNameEntry*> Cache;
typedef std::map<std::string, LLUUID> 		ReverseCache;

class LLCacheName::Impl
{
public:
	LLMessageSystem*	mMsg;
	LLHost				mUpstreamHost;

	Cache				mCache;
		// the map of UUIDs to names
	ReverseCache   	  	mReverseCache;
		// map of names to UUIDs
	
	AskQueue			mAskNameQueue;
	AskQueue			mAskGroupQueue;
		// UUIDs to ask our upstream host about
	
	PendingQueue		mPendingQueue;
		// UUIDs that have been requested but are not in cache yet.

	ReplyQueue			mReplyQueue;
		// requests awaiting replies from us

	LLCacheNameSignal	mSignal;

	LLFrameTimer		mProcessTimer;

	Impl(LLMessageSystem* msg);
	~Impl();
	
	boost::signals2::connection addPending(const LLUUID& id, const LLCacheNameCallback& callback);
	void addPending(const LLUUID& id, const LLHost& host);
	
	void processPendingAsks();
	void processPendingReplies();
	void sendRequest(const char* msg_name, const AskQueue& queue);
	bool isRequestPending(const LLUUID& id);

	// Message system callbacks.
	void processUUIDRequest(LLMessageSystem* msg, bool isGroup);
	void processUUIDReply(LLMessageSystem* msg, bool isGroup);

	static void handleUUIDNameRequest(LLMessageSystem* msg, void** userdata);
	static void handleUUIDNameReply(LLMessageSystem* msg, void** userdata);
	static void handleUUIDGroupNameRequest(LLMessageSystem* msg, void** userdata);
	static void handleUUIDGroupNameReply(LLMessageSystem* msg, void** userdata);
};


/// --------------------------------------------------------------------------
/// class LLCacheName
/// ---------------------------------------------------------------------------

LLCacheName::LLCacheName(LLMessageSystem* msg)
	: impl(* new Impl(msg))
	{ }

LLCacheName::LLCacheName(LLMessageSystem* msg, const LLHost& upstream_host)
	: impl(* new Impl(msg))
{
	sCacheName["waiting"] = "(Loading...)";
	sCacheName["nobody"] = "(nobody)";
	sCacheName["none"] = "(none)";
	setUpstream(upstream_host);
}

LLCacheName::~LLCacheName()
{
	delete &impl;
}

LLCacheName::Impl::Impl(LLMessageSystem* msg)
	: mMsg(msg), mUpstreamHost(LLHost::invalid)
{
	mMsg->setHandlerFuncFast(
		_PREHASH_UUIDNameRequest, handleUUIDNameRequest, (void**)this);
	mMsg->setHandlerFuncFast(
		_PREHASH_UUIDNameReply, handleUUIDNameReply, (void**)this);
	mMsg->setHandlerFuncFast(
		_PREHASH_UUIDGroupNameRequest, handleUUIDGroupNameRequest, (void**)this);
	mMsg->setHandlerFuncFast(
		_PREHASH_UUIDGroupNameReply, handleUUIDGroupNameReply, (void**)this);
}


LLCacheName::Impl::~Impl()
{
	for_each(mCache.begin(), mCache.end(), DeletePairedPointer());
	for_each(mReplyQueue.begin(), mReplyQueue.end(), DeletePointer());
}

boost::signals2::connection LLCacheName::Impl::addPending(const LLUUID& id, const LLCacheNameCallback& callback)
{
	PendingReply* reply = new PendingReply(id, LLHost());
	boost::signals2::connection res = reply->setCallback(callback);
	mReplyQueue.push_back(reply);
	return res;
}

void LLCacheName::Impl::addPending(const LLUUID& id, const LLHost& host)
{
	PendingReply* reply = new PendingReply(id, host);
	mReplyQueue.push_back(reply);
}

void LLCacheName::setUpstream(const LLHost& upstream_host)
{
	impl.mUpstreamHost = upstream_host;
}

boost::signals2::connection LLCacheName::addObserver(const LLCacheNameCallback& callback)
{
	return impl.mSignal.connect(callback);
}

void LLCacheName::importFile(LLFILE* fp)
{
	S32 count = 0;

	const S32 BUFFER_SIZE = 1024;
	char buffer[BUFFER_SIZE];	/*Flawfinder: ignore*/

	// *NOTE: These buffer sizes are hardcoded into sscanf() below
	char id_string[MAX_STRING]; /*Flawfinder: ignore*/
	char firstname[MAX_STRING]; /*Flawfinder: ignore*/
	char lastname[MAX_STRING]; /*Flawfinder: ignore*/
	U32 create_time;

	// This is OK if the first line is actually a name.  We just don't load it.
	char* valid = fgets(buffer, BUFFER_SIZE, fp);
	if (!valid) return;

	// *NOTE: This buffer size is hardcoded into sscanf() below
	char version_string[BUFFER_SIZE]; /*Flawfinder: ignore*/
	S32 version = 0;
	S32 match = sscanf(	/* Flawfinder: ignore */
		buffer,
		"%1023s %d",
		version_string, &version);
	if (   match != 2
		|| strcmp(version_string, "version")
		|| version != CN_FILE_VERSION)
	{
		llwarns << "Ignoring old cache name file format" << llendl;
		return;
	}

	// We'll expire entries more than a week old
	U32 now = (U32)time(NULL);
	const U32 SECS_PER_DAY = 60 * 60 * 24;
	U32 delete_before_time = now - (7 * SECS_PER_DAY);

	while(!feof(fp))
	{
		valid = fgets(buffer, BUFFER_SIZE, fp);
		if (!valid) break;

		match = sscanf(	/* Flawfinder: ignore */
			buffer,
			"%254s %u %254s %254s",
			id_string, 
			&create_time,
			firstname, 
			lastname);
		if (4 != match) continue;

		LLUUID id(id_string);
		if (id.isNull()) continue;

		// undo trivial XOR
		S32 i;
		for (i = 0; i < UUID_BYTES; i++)
		{
			id.mData[i] ^= 0x33;
		}

		// Don't load entries that are more than a week old
		if (create_time < delete_before_time) continue;

		LLCacheNameEntry* entry = new LLCacheNameEntry();
		entry->mIsGroup = false;
		entry->mCreateTime = create_time;
		entry->mFirstName = firstname;
		entry->mLastName = lastname;
		impl.mCache[id] = entry;
		std::string fullname = entry->mFirstName + " " + entry->mLastName;
		impl.mReverseCache[fullname] = id;
		
		count++;
	}

	llinfos << "LLCacheName loaded " << count << " names" << llendl;
}

bool LLCacheName::importFile(std::istream& istr)
{
	LLSD data;
	if(LLSDSerialize::fromXML(data, istr) < 1)
		return false;

	// We'll expire entries more than a week old
	U32 now = (U32)time(NULL);
	const U32 SECS_PER_DAY = 60 * 60 * 24;
	U32 delete_before_time = now - (7 * SECS_PER_DAY);

	// iterate over the agents
	S32 count = 0;
	LLSD agents = data[AGENTS];
	LLSD::map_iterator iter = agents.beginMap();
	LLSD::map_iterator end = agents.endMap();
	for( ; iter != end; ++iter)
	{
		LLUUID id((*iter).first);
		LLSD agent = (*iter).second;
		U32 ctime = (U32)agent[CTIME].asInteger();
		if(ctime < delete_before_time) continue;

		LLCacheNameEntry* entry = new LLCacheNameEntry();
		entry->mIsGroup = false;
		entry->mCreateTime = ctime;
		entry->mFirstName = agent[FIRST].asString();
		entry->mLastName = agent[LAST].asString();
		impl.mCache[id] = entry;
		std::string fullname = entry->mFirstName + " " + entry->mLastName;
		impl.mReverseCache[fullname] = id;

		++count;
	}
	llinfos << "LLCacheName loaded " << count << " agent names" << llendl;

	count = 0;
	LLSD groups = data[GROUPS];
	iter = groups.beginMap();
	end = groups.endMap();
	for( ; iter != end; ++iter)
	{
		LLUUID id((*iter).first);
		LLSD group = (*iter).second;
		U32 ctime = (U32)group[CTIME].asInteger();
		if(ctime < delete_before_time) continue;

		LLCacheNameEntry* entry = new LLCacheNameEntry();
		entry->mIsGroup = true;
		entry->mCreateTime = ctime;
		entry->mGroupName = group[NAME].asString();
		impl.mCache[id] = entry;
		impl.mReverseCache[entry->mGroupName] = id;
		++count;
	}
	llinfos << "LLCacheName loaded " << count << " group names" << llendl;
	return true;
}

void LLCacheName::exportFile(std::ostream& ostr)
{
	LLSD data;
	Cache::iterator iter = impl.mCache.begin();
	Cache::iterator end = impl.mCache.end();
	for( ; iter != end; ++iter)
	{
		// Only write entries for which we have valid data.
		LLCacheNameEntry* entry = iter->second;
		if(!entry
		   || (std::string::npos != entry->mFirstName.find('?'))
		   || (std::string::npos != entry->mGroupName.find('?')))
		{
			continue;
		}

		// store it
		LLUUID id = iter->first;
		std::string id_str = id.asString();
		if(!entry->mFirstName.empty() /* && !entry->mLastName.empty() */ )  // IDEVO save SLIDs
		{
			data[AGENTS][id_str][FIRST] = entry->mFirstName;
			data[AGENTS][id_str][LAST] = entry->mLastName;
			data[AGENTS][id_str][CTIME] = (S32)entry->mCreateTime;
		}
		else if(entry->mIsGroup && !entry->mGroupName.empty())
		{
			data[GROUPS][id_str][NAME] = entry->mGroupName;
			data[GROUPS][id_str][CTIME] = (S32)entry->mCreateTime;
		}
	}

	LLSDSerialize::toPrettyXML(data, ostr);
}


BOOL LLCacheName::getName(const LLUUID& id, std::string& first, std::string& last)
{
	if(id.isNull())
	{
		first = sCacheName["nobody"];
		last.clear();
		return FALSE;
	}

	LLCacheNameEntry* entry = get_ptr_in_map(impl.mCache, id );
	if (entry)
	{
		first = entry->mFirstName;
		last =  entry->mLastName;
		return TRUE;
	}
	else
	{
		first = sCacheName["waiting"];
		last.clear();
		if (!impl.isRequestPending(id))
		{
			impl.mAskNameQueue.insert(id);
		}	
		return FALSE;
	}

}

// static
void LLCacheName::LocalizeCacheName(std::string key, std::string value)
{
	if (key!="" && value!= "" )
		sCacheName[key]=value;
	else
		llwarns<< " Error localizing cache key " << key << " To "<< value<<llendl;
}

BOOL LLCacheName::getFullName(const LLUUID& id, std::string& fullname)
{
	std::string first_name, last_name;
	BOOL res = getName(id, first_name, last_name);
	fullname = buildFullname(first_name, last_name);
	return res;
}

BOOL LLCacheName::getGroupName(const LLUUID& id, std::string& group)
{
	if(id.isNull())
	{
		group = sCacheName["none"];
		return FALSE;
	}

	LLCacheNameEntry* entry = get_ptr_in_map(impl.mCache,id);
	if (entry && entry->mGroupName.empty())
	{
		// COUNTER-HACK to combat James' HACK in exportFile()...
		// this group name was loaded from a name cache that did not
		// bother to save the group name ==> we must ask for it
		lldebugs << "LLCacheName queuing HACK group request: " << id << llendl;
		entry = NULL;
	}

	if (entry)
	{
		group = entry->mGroupName;
		return TRUE;
	}
	else 
	{
		group = sCacheName["waiting"];
		if (!impl.isRequestPending(id))
		{
			impl.mAskGroupQueue.insert(id);
		}
		return FALSE;
	}
}

BOOL LLCacheName::getUUID(const std::string& first, const std::string& last, LLUUID& id)
{
	std::string fullname = buildFullname(first, last);
	return getUUID(fullname, id);
}

BOOL LLCacheName::getUUID(const std::string& fullname, LLUUID& id)
{
	ReverseCache::iterator iter = impl.mReverseCache.find(fullname);
	if (iter != impl.mReverseCache.end())
	{
		id = iter->second;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//static
std::string LLCacheName::buildFullname(const std::string& first, const std::string& last)
{
	std::string fullname = first;
	if (!last.empty()
		&& last != "Resident")
	{
		fullname += ' ';
		fullname += last;
	}
	return fullname;
}

// This is a little bit kludgy. LLCacheNameCallback is a slot instead of a function pointer.
//  The reason it is a slot is so that the legacy get() function below can bind an old callback
//  and pass it as a slot. The reason it isn't a boost::function is so that trackable behavior
//  doesn't get lost. As a result, we have to bind the slot to a signal to call it, even when
//  we call it immediately. -Steve
// NOTE: Even though passing first and last name is a bit of extra overhead, it eliminates the
//  potential need for any parsing should any code need to handle first and last name independently.
boost::signals2::connection LLCacheName::get(const LLUUID& id, bool is_group, const LLCacheNameCallback& callback)
{
	boost::signals2::connection res;
	
	if(id.isNull())
	{
		LLCacheNameSignal signal;
		signal.connect(callback);
		signal(id, sCacheName["nobody"], is_group);
		return res;
	}

	LLCacheNameEntry* entry = get_ptr_in_map(impl.mCache, id );
	if (entry)
	{
		LLCacheNameSignal signal;
		signal.connect(callback);
		// id found in map therefore we can call the callback immediately.
		if (entry->mIsGroup)
		{
			signal(id, entry->mGroupName, entry->mIsGroup);
		}
		else
		{
			std::string fullname =
				buildFullname(entry->mFirstName, entry->mLastName);
			signal(id, fullname, entry->mIsGroup);
		}
	}
	else
	{
		// id not found in map so we must queue the callback call until available.
		if (!impl.isRequestPending(id))
		{
			if (is_group)
			{
				impl.mAskGroupQueue.insert(id);
			}
			else
			{
				impl.mAskNameQueue.insert(id);
			}
		}
		res = impl.addPending(id, callback);
	}
	return res;
}

boost::signals2::connection LLCacheName::get(const LLUUID& id, bool is_group, old_callback_t callback, void* user_data)
{
	return get(id, is_group, boost::bind(callback, _1, _2, _3, user_data));
}

void LLCacheName::processPending()
{
	LLMemType mt_pp(LLMemType::MTYPE_CACHE_PROCESS_PENDING);
	const F32 SECS_BETWEEN_PROCESS = 0.1f;
	if(!impl.mProcessTimer.checkExpirationAndReset(SECS_BETWEEN_PROCESS))
	{
		return;
	}

	if(!impl.mUpstreamHost.isOk())
	{
		lldebugs << "LLCacheName::processPending() - bad upstream host."
				 << llendl;
		return;
	}

	impl.processPendingAsks();
	impl.processPendingReplies();
}

void LLCacheName::deleteEntriesOlderThan(S32 secs)
{
	U32 now = (U32)time(NULL);
	U32 expire_time = now - secs;
	for(Cache::iterator iter = impl.mCache.begin(); iter != impl.mCache.end(); )
	{
		Cache::iterator curiter = iter++;
		LLCacheNameEntry* entry = curiter->second;
		if (entry->mCreateTime < expire_time)
		{
			delete entry;
			impl.mCache.erase(curiter);
		}
	}

	// These are pending requests that we never heard back from.
	U32 pending_expire_time = now - PENDING_TIMEOUT_SECS;
	for(PendingQueue::iterator p_iter = impl.mPendingQueue.begin();
		p_iter != impl.mPendingQueue.end(); )
	{
		PendingQueue::iterator p_curitor = p_iter++;
 
		if (p_curitor->second < pending_expire_time)
		{
			impl.mPendingQueue.erase(p_curitor);
		}
	}
}


void LLCacheName::dump()
{
	for (Cache::iterator iter = impl.mCache.begin(),
			 end = impl.mCache.end();
		 iter != end; iter++)
	{
		LLCacheNameEntry* entry = iter->second;
		if (entry->mIsGroup)
		{
			llinfos
				<< iter->first << " = (group) "
				<< entry->mGroupName
				<< " @ " << entry->mCreateTime
				<< llendl;
		}
		else
		{
			llinfos
				<< iter->first << " = "
				<< buildFullname(entry->mFirstName, entry->mLastName)
				<< " @ " << entry->mCreateTime
				<< llendl;
		}
	}
}

void LLCacheName::dumpStats()
{
	llinfos << "Queue sizes: "
			<< " Cache=" << impl.mCache.size()
			<< " AskName=" << impl.mAskNameQueue.size()
			<< " AskGroup=" << impl.mAskGroupQueue.size()
			<< " Pending=" << impl.mPendingQueue.size()
			<< " Reply=" << impl.mReplyQueue.size()
// 			<< " Observers=" << impl.mSignal.size()
			<< llendl;
}

//static 
std::string LLCacheName::getDefaultName()
{
	return sCacheName["waiting"];
}

void LLCacheName::Impl::processPendingAsks()
{
	LLMemType mt_ppa(LLMemType::MTYPE_CACHE_PROCESS_PENDING_ASKS);
	sendRequest(_PREHASH_UUIDNameRequest, mAskNameQueue);
	sendRequest(_PREHASH_UUIDGroupNameRequest, mAskGroupQueue);
	mAskNameQueue.clear();
	mAskGroupQueue.clear();
}

void LLCacheName::Impl::processPendingReplies()
{
	LLMemType mt_ppr(LLMemType::MTYPE_CACHE_PROCESS_PENDING_REPLIES);
	// First call all the callbacks, because they might send messages.
	for(ReplyQueue::iterator it = mReplyQueue.begin(); it != mReplyQueue.end(); ++it)
	{
		PendingReply* reply = *it;
		LLCacheNameEntry* entry = get_ptr_in_map(mCache, reply->mID);
		if(!entry) continue;

		if (!entry->mIsGroup)
		{
			std::string fullname =
				LLCacheName::buildFullname(entry->mFirstName, entry->mLastName);
			(reply->mSignal)(reply->mID, fullname, false);
		}
		else
		{
			(reply->mSignal)(reply->mID, entry->mGroupName, true);
		}
	}

	// Forward on all replies, if needed.
	ReplySender sender(mMsg);
	for(ReplyQueue::iterator it = mReplyQueue.begin(); it != mReplyQueue.end(); ++it)
	{
		PendingReply* reply = *it;
		LLCacheNameEntry* entry = get_ptr_in_map(mCache, reply->mID);
		if(!entry) continue;

		if (reply->mHost.isOk())
		{
			sender.send(reply->mID, *entry, reply->mHost);
		}

		reply->done();
	}
	
	for(ReplyQueue::iterator it = mReplyQueue.begin(); it != mReplyQueue.end(); )
	{
		ReplyQueue::iterator curit = it++;
		PendingReply* reply = *curit;
		if (reply->isDone())
		{
			delete reply;
			mReplyQueue.erase(curit);
		}
	}
}


void LLCacheName::Impl::sendRequest(
	const char* msg_name,
	const AskQueue& queue)
{
	if(queue.empty())
	{
		return;		
	}

	bool start_new_message = true;
	AskQueue::const_iterator it = queue.begin();
	AskQueue::const_iterator end = queue.end();
	for(; it != end; ++it)
	{
		if(start_new_message)
		{
			start_new_message = false;
			mMsg->newMessageFast(msg_name);
		}
		mMsg->nextBlockFast(_PREHASH_UUIDNameBlock);
		mMsg->addUUIDFast(_PREHASH_ID, (*it));

		if(mMsg->isSendFullFast(_PREHASH_UUIDNameBlock))
		{
			start_new_message = true;
			mMsg->sendReliable(mUpstreamHost);
		}
	}
	if(!start_new_message)
	{
		mMsg->sendReliable(mUpstreamHost);
	}
}

bool LLCacheName::Impl::isRequestPending(const LLUUID& id)
{
	U32 now = (U32)time(NULL);
	U32 expire_time = now - PENDING_TIMEOUT_SECS;

	PendingQueue::iterator iter = mPendingQueue.find(id);

	if (iter == mPendingQueue.end()
		|| (iter->second < expire_time) )
	{
		mPendingQueue[id] = now;
		return false;
	}

	return true;
}
	
void LLCacheName::Impl::processUUIDRequest(LLMessageSystem* msg, bool isGroup)
{
	// You should only get this message if the cache is at the simulator
	// level, hence having an upstream provider.
	if (!mUpstreamHost.isOk())
	{
		llwarns << "LLCacheName - got UUID name/group request, but no upstream provider!" << llendl;
		return;
	}

	LLHost fromHost = msg->getSender();
	ReplySender sender(msg);

	S32 count = msg->getNumberOfBlocksFast(_PREHASH_UUIDNameBlock);
	for(S32 i = 0; i < count; ++i)
	{
		LLUUID id;
		msg->getUUIDFast(_PREHASH_UUIDNameBlock, _PREHASH_ID, id, i);
		LLCacheNameEntry* entry = get_ptr_in_map(mCache, id);
		if(entry)
		{
			if (isGroup != entry->mIsGroup)
			{
				llwarns << "LLCacheName - Asked for "
						<< (isGroup ? "group" : "user") << " name, "
						<< "but found "
						<< (entry->mIsGroup ? "group" : "user")
						<< ": " << id << llendl;
			}
			else
			{
				// ...it's in the cache, so send it as the reply
				sender.send(id, *entry, fromHost);
			}
		}
		else
		{
			if (!isRequestPending(id))
			{
				if (isGroup)
				{
					mAskGroupQueue.insert(id);
				}
				else
				{
					mAskNameQueue.insert(id);
				}
			}
			
			addPending(id, fromHost);
		}
	}
}



void LLCacheName::Impl::processUUIDReply(LLMessageSystem* msg, bool isGroup)
{
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_UUIDNameBlock);
	for(S32 i = 0; i < count; ++i)
	{
		LLUUID id;
		msg->getUUIDFast(_PREHASH_UUIDNameBlock, _PREHASH_ID, id, i);
		LLCacheNameEntry* entry = get_ptr_in_map(mCache, id);
		if (!entry)
		{
			entry = new LLCacheNameEntry;
			mCache[id] = entry;
		}

		mPendingQueue.erase(id);

		entry->mIsGroup = isGroup;
		entry->mCreateTime = (U32)time(NULL);
		if (!isGroup)
		{
			msg->getStringFast(_PREHASH_UUIDNameBlock, _PREHASH_FirstName, entry->mFirstName, i);
			msg->getStringFast(_PREHASH_UUIDNameBlock, _PREHASH_LastName,  entry->mLastName, i);

			// IDEVO blank out last name for storage to reduce string compares on
			// retrieval.  Eventually need to convert to single mName field.
			if (entry->mLastName == "Resident")
			{
				entry->mLastName = "";
			}
		}
		else
		{	// is group
			msg->getStringFast(_PREHASH_UUIDNameBlock, _PREHASH_GroupName, entry->mGroupName, i);
			LLStringFn::replace_ascii_controlchars(entry->mGroupName, LL_UNKNOWN_CHAR);
		}

		if (!isGroup)
		{
			std::string fullname =
				LLCacheName::buildFullname(entry->mFirstName, entry->mLastName);
			mSignal(id, fullname, false);
			mReverseCache[fullname] = id;
		}
		else
		{
			mSignal(id, entry->mGroupName, true);
			mReverseCache[entry->mGroupName] = id;
		}
	}
}



// static call back functions

void LLCacheName::Impl::handleUUIDNameReply(LLMessageSystem* msg, void** userData)
{
	((LLCacheName::Impl*)userData)->processUUIDReply(msg, false);
}

void LLCacheName::Impl::handleUUIDNameRequest(LLMessageSystem* msg, void** userData)
{
	((LLCacheName::Impl*)userData)->processUUIDRequest(msg, false);
}

void LLCacheName::Impl::handleUUIDGroupNameRequest(LLMessageSystem* msg, void** userData)
{
	((LLCacheName::Impl*)userData)->processUUIDRequest(msg, true);
}

void LLCacheName::Impl::handleUUIDGroupNameReply(LLMessageSystem* msg, void** userData)
{
	((LLCacheName::Impl*)userData)->processUUIDReply(msg, true);
}
