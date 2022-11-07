/** 
 * @file llmessagethrottle.h
 * @brief LLMessageThrottle class used for throttling messages.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

    size_t  getHash() const { return mHash; }
    U64     getEntryTime() const { return mEntryTime; }
protected:
    size_t  mHash;
    U64     mEntryTime;
};


class LLMessageThrottle
{
public:
    LLMessageThrottle();
    ~LLMessageThrottle();

    BOOL addViewerAlert (const LLUUID& to, const std::string& mesg);
    BOOL addAgentAlert  (const LLUUID& agent, const LLUUID& task, const std::string& mesg);

    void pruneEntries();

protected:
    typedef std::deque<LLMessageThrottleEntry>                          message_list_t;
    typedef std::deque<LLMessageThrottleEntry>::iterator                message_list_iterator_t;
    typedef std::deque<LLMessageThrottleEntry>::reverse_iterator        message_list_reverse_iterator_t;
    typedef std::deque<LLMessageThrottleEntry>::const_iterator          message_list_const_iterator_t;
    typedef std::deque<LLMessageThrottleEntry>::const_reverse_iterator  message_list_const_reverse_iterator_t;
    message_list_t  mMessageList[MTC_EOF];
};

extern LLMessageThrottle gMessageThrottle;

#endif


