/** 
 * @file llmessagethrottle.h
 * @brief LLMessageThrottle class used for throttling messages.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMESSAGETHROTTLE_H
#define LL_LLMESSAGETHROTTLE_H

#include <deque>

#include "linden_common.h"
#include "lluuid.h"

typedef enum e_message_throttle_categories
{
	MTC_VIEWER_ALERT,
	MTC_AGENT_ALERT,
	MTC_EOF
} EMessageThrottleCats;

class LLMessageThrottleEntry
{
public:
	LLMessageThrottleEntry(const size_t hash, const U64 entry_time)
		: mHash(hash), mEntryTime(entry_time) {}

	size_t	getHash()	{ return mHash; }
	U64	getEntryTime()	{ return mEntryTime; }
protected:
	size_t	mHash;
	U64		mEntryTime;
};


class LLMessageThrottle
{
public:
	LLMessageThrottle();
	~LLMessageThrottle();

	BOOL addViewerAlert	(const LLUUID& to, const char* mesg);
	BOOL addAgentAlert	(const LLUUID& agent, const LLUUID& task, const char* mesg);

	void pruneEntries();

protected:
	typedef std::deque<LLMessageThrottleEntry>							message_list_t;
	typedef std::deque<LLMessageThrottleEntry>::iterator				message_list_iterator_t;
	typedef std::deque<LLMessageThrottleEntry>::reverse_iterator		message_list_reverse_iterator_t;
	typedef std::deque<LLMessageThrottleEntry>::const_iterator			message_list_const_iterator_t;
	typedef std::deque<LLMessageThrottleEntry>::const_reverse_iterator	message_list_const_reverse_iterator_t;
	message_list_t	mMessageList[MTC_EOF];
};

extern LLMessageThrottle gMessageThrottle;

#endif


