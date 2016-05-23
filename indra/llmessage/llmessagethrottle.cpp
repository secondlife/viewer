/** 
 * @file llmessagethrottle.cpp
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

#include "linden_common.h"

#include "llhash.h"

#include "llmessagethrottle.h"
#include "llframetimer.h"

// This is used for the stl search_n function.
#if _MSC_VER >= 1500 // VC9 has a bug in search_n
struct eq_message_throttle_entry : public std::binary_function< LLMessageThrottleEntry, LLMessageThrottleEntry, bool >
{
	bool operator()(const LLMessageThrottleEntry& a, const LLMessageThrottleEntry& b) const
	{
		return a.getHash() == b.getHash();
	}
};
#else
bool eq_message_throttle_entry(LLMessageThrottleEntry a, LLMessageThrottleEntry b)
 		{ return a.getHash() == b.getHash(); }
#endif

const U64 SEC_TO_USEC = 1000000;
		
// How long (in microseconds) each type of message stays in its throttle list.
const U64 MAX_MESSAGE_AGE[MTC_EOF] =
{
	10 * SEC_TO_USEC,	// MTC_VIEWER_ALERT
	10 * SEC_TO_USEC	// MTC_AGENT_ALERT
};

LLMessageThrottle::LLMessageThrottle()
{
}

LLMessageThrottle::~LLMessageThrottle()
{
}

void LLMessageThrottle::pruneEntries()
{
	// Go through each message category, and prune entries older than max age.
	S32 cat;
	for (cat = 0; cat < MTC_EOF; cat++)
	{
		message_list_t* message_list = &(mMessageList[cat]);

		// Use a reverse iterator, since entries on the back will be the oldest.
		message_list_reverse_iterator_t r_iterator 	= message_list->rbegin();
		message_list_reverse_iterator_t r_last 		= message_list->rend();

		// Look for the first entry younger than the maximum age.
		F32 max_age = (F32)MAX_MESSAGE_AGE[cat]; 
		BOOL found = FALSE;
		while (r_iterator != r_last && !found)
		{
			if ( LLFrameTimer::getTotalTime() - (*r_iterator).getEntryTime() < max_age )
			{
				// We found a young enough entry.
				found = TRUE;

				// Did we find at least one entry to remove?
				if (r_iterator != message_list->rbegin())
				{
					// Yes, remove it.
					message_list->erase(r_iterator.base(), message_list->end());
				}
			}
			else
			{
				r_iterator++;
			}
		}

		// If we didn't find any entries young enough to keep, remove them all.
		if (!found)
		{
			message_list->clear();
		}
	}
}

BOOL LLMessageThrottle::addViewerAlert(const LLUUID& to, const std::string& mesg)
{
	message_list_t* message_list = &(mMessageList[MTC_VIEWER_ALERT]);

	// Concatenate from,to,mesg into one string.
	std::ostringstream full_mesg;
	full_mesg << to << mesg;

	// Create an entry for this message.
	size_t hash = llhash(full_mesg.str().c_str());
	LLMessageThrottleEntry entry(hash, LLFrameTimer::getTotalTime());

	// Check if this message is already in the list.
#if _MSC_VER >= 1500 // VC9 has a bug in search_n
	// SJB: This *should* work but has not been tested yet *TODO: Test!
	message_list_iterator_t found = std::find_if(message_list->begin(), message_list->end(),
												 std::bind2nd(eq_message_throttle_entry(), entry));
#else
 	message_list_iterator_t found = std::search_n(message_list->begin(), message_list->end(),
 												  1, entry, eq_message_throttle_entry);
#endif
	if (found == message_list->end())
	{
		// This message was not found.  Add it to the list.
		message_list->push_front(entry);
		return TRUE;
	}
	else
	{
		// This message was already in the list.
		return FALSE;
	}
}

BOOL LLMessageThrottle::addAgentAlert(const LLUUID& agent, const LLUUID& task, const std::string& mesg)
{
	message_list_t* message_list = &(mMessageList[MTC_AGENT_ALERT]);

	// Concatenate from,to,mesg into one string.
	std::ostringstream full_mesg;
	full_mesg << agent << task << mesg;

	// Create an entry for this message.
	size_t hash = llhash(full_mesg.str().c_str());
	LLMessageThrottleEntry entry(hash, LLFrameTimer::getTotalTime());

	// Check if this message is already in the list.
#if _MSC_VER >= 1500 // VC9 has a bug in search_n
	// SJB: This *should* work but has not been tested yet *TODO: Test!
	message_list_iterator_t found = std::find_if(message_list->begin(), message_list->end(),
												 std::bind2nd(eq_message_throttle_entry(), entry));
#else
	message_list_iterator_t found = std::search_n(message_list->begin(), message_list->end(),
												  1, entry, eq_message_throttle_entry);
#endif
	
	if (found == message_list->end())
	{
		// This message was not found.  Add it to the list.
		message_list->push_front(entry);
		return TRUE;
	}
	else
	{
		// This message was already in the list.
		return FALSE;
	}
}

