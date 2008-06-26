/** 
 * @file llcachename.cpp
 * @brief A hierarchical cache of first and last names queried based on UUID.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

// Constants
static const std::string CN_WAITING("(Loading...)"); // *TODO: translate
static const std::string CN_NOBODY("(nobody)"); // *TODO: translate
static const std::string CN_NONE("(none)"); // *TODO: translate

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
	std::string mFirstName;
	std::string mLastName;
	std::string mGroupName;
};

LLCacheNameEntry::LLCacheNameEntry()
{
}


class PendingReply
{
public:
	LLUUID				mID;
	LLCacheNameCallback mCallback;
	LLHost				mHost;
	void*				mData;
	PendingReply(const LLUUID& id, LLCacheNameCallback callback, void* data = NULL)
		: mID(id), mCallback(callback), mData(data)
	{ }

	PendingReply(const LLUUID& id, const LLHost& host)
		: mID(id), mCallback(0), mHost(host)
	{ }

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
	: mMsg(msg), mPending(false)
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
typedef std::vector<PendingReply>			ReplyQueue;
typedef std::map<LLUUID,U32>				PendingQueue;
typedef std::map<LLUUID, LLCacheNameEntry*> Cache;
typedef std::vector<LLCacheNameCallback>	Observers;

class LLCacheName::Impl
{
public:
	LLMessageSystem*	mMsg;
	LLHost				mUpstreamHost;

	Cache				mCache;
		// the map of UUIDs to names

	AskQueue			mAskNameQueue;
	AskQueue			mAskGroupQueue;
		// UUIDs to ask our upstream host about
	
	PendingQueue		mPendingQueue;
		// UUIDs that have been requested but are not in cache yet.

	ReplyQueue			mReplyQueue;
		// requests awaiting replies from us

	Observers			mObservers;

	LLFrameTimer		mProcessTimer;

	Impl(LLMessageSystem* msg);
	~Impl();

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

	void notifyObservers(const LLUUID& id, const std::string& first, const std::string& last, BOOL group);
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
}


void LLCacheName::setUpstream(const LLHost& upstream_host)
{
	impl.mUpstreamHost = upstream_host;
}

void LLCacheName::addObserver(LLCacheNameCallback callback)
{
	impl.mObservers.push_back(callback);
}

void LLCacheName::removeObserver(LLCacheNameCallback callback)
{
	Observers::iterator it = impl.mObservers.begin();
	Observers::iterator end = impl.mObservers.end();

	for ( ; it != end; ++it)
	{
		const LLCacheNameCallback& cb = (*it);
		if (cb == callback)
		{
			impl.mObservers.erase(it);
			return;
		}
	}
}

void LLCacheName::cancelCallback(const LLUUID& id, LLCacheNameCallback callback, void* user_data)
{
	ReplyQueue::iterator it = impl.mReplyQueue.begin();
	ReplyQueue::iterator end = impl.mReplyQueue.end();
	
	for(; it != end; ++it)
	{
		const PendingReply& reply = (*it);

		if ((callback == reply.mCallback)
			&& (id == reply.mID)
			&& (user_data == reply.mData) )
		{
			impl.mReplyQueue.erase(it);
			return;
		}
	}
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
		if(!entry->mFirstName.empty() && !entry->mLastName.empty())
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
		first = CN_NOBODY;
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
		first = CN_WAITING;
		last.clear();
		if (!impl.isRequestPending(id))
		{
			impl.mAskNameQueue.insert(id);
		}	
		return FALSE;
	}

}

BOOL LLCacheName::getFullName(const LLUUID& id, std::string& fullname)
{
	std::string first_name, last_name;
	BOOL res = getName(id, first_name, last_name);
	fullname = first_name + " " + last_name;
	return res;
}

BOOL LLCacheName::getGroupName(const LLUUID& id, std::string& group)
{
	if(id.isNull())
	{
		group = CN_NONE;
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
		group = CN_WAITING;
		if (!impl.isRequestPending(id))
		{
			impl.mAskGroupQueue.insert(id);
		}
		return FALSE;
	}
}

// TODO: Make the cache name callback take a SINGLE std::string,
// not a separate first and last name.
void LLCacheName::get(const LLUUID& id, BOOL is_group, LLCacheNameCallback callback, void* user_data)
{
	if(id.isNull())
	{
		callback(id, CN_NOBODY, "", is_group, user_data);
	}

	LLCacheNameEntry* entry = get_ptr_in_map(impl.mCache, id );
	if (entry)
	{
		// id found in map therefore we can call the callback immediately.
		if (entry->mIsGroup)
		{
			callback(id, entry->mGroupName, "", entry->mIsGroup, user_data);
		}
		else
		{
			callback(id, entry->mFirstName, entry->mLastName, entry->mIsGroup, user_data);
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
		impl.mReplyQueue.push_back(PendingReply(id, callback, user_data));
	}
}

void LLCacheName::processPending()
{
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
				<< entry->mFirstName << " " << entry->mLastName
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
			<< " Observers=" << impl.mObservers.size()
			<< llendl;
}

//static 
std::string LLCacheName::getDefaultName()
{
	return CN_WAITING;
}

void LLCacheName::Impl::processPendingAsks()
{
	sendRequest(_PREHASH_UUIDNameRequest, mAskNameQueue);
	sendRequest(_PREHASH_UUIDGroupNameRequest, mAskGroupQueue);
	mAskNameQueue.clear();
	mAskGroupQueue.clear();
}

void LLCacheName::Impl::processPendingReplies()
{
	ReplyQueue::iterator it = mReplyQueue.begin();
	ReplyQueue::iterator end = mReplyQueue.end();
	
	// First call all the callbacks, because they might send messages.
	for(; it != end; ++it)
	{
		LLCacheNameEntry* entry = get_ptr_in_map(mCache, it->mID);
		if(!entry) continue;

		if (it->mCallback)
		{
			if (!entry->mIsGroup)
			{
				(it->mCallback)(it->mID,
					entry->mFirstName, entry->mLastName,
					FALSE, it->mData);
			}
			else {
				(it->mCallback)(it->mID,
					entry->mGroupName, "",
					TRUE, it->mData);
			}
		}
	}

	// Forward on all replies, if needed.
	ReplySender sender(mMsg);
	for (it = mReplyQueue.begin(); it != end; ++it)
	{
		LLCacheNameEntry* entry = get_ptr_in_map(mCache, it->mID);
		if(!entry) continue;

		if (it->mHost.isOk())
		{
			sender.send(it->mID, *entry, it->mHost);
		}

		it->done();
	}

	mReplyQueue.erase(
		remove_if(mReplyQueue.begin(), mReplyQueue.end(),
			std::mem_fun_ref(&PendingReply::isDone)),
		mReplyQueue.end());
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

void LLCacheName::Impl::notifyObservers(const LLUUID& id,
	const std::string& first, const std::string& last, BOOL is_group)
{
	for (Observers::const_iterator i = mObservers.begin(),
								   end = mObservers.end();
		i != end;
		++i)
	{
		(**i)(id, first, last, is_group, NULL);
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
			
			mReplyQueue.push_back(PendingReply(id, fromHost));
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
		}
		else
		{
			msg->getStringFast(_PREHASH_UUIDNameBlock, _PREHASH_GroupName, entry->mGroupName, i);
		}

		if (!isGroup)
		{
			notifyObservers(id, entry->mFirstName, entry->mLastName, FALSE);
		}
		else
		{
			notifyObservers(id, entry->mGroupName, "", TRUE);
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

