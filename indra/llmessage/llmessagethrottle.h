/** 
 * @file llmessagethrottle.h
 * @brief LLMessageThrottle class used for throttling messages.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

	size_t	getHash() const { return mHash; }
	U64		getEntryTime() const { return mEntryTime; }
protected:
	size_t	mHash;
	U64		mEntryTime;
};


class LLMessageThrottle
{
public:
	LLMessageThrottle();
	~LLMessageThrottle();

	BOOL addViewerAlert	(const LLUUID& to, const std::string& mesg);
	BOOL addAgentAlert	(const LLUUID& agent, const LLUUID& task, const std::string& mesg);

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


