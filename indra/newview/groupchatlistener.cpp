/**
 * @file   groupchatlistener.cpp
 * @author Nat Goodspeed
 * @date   2011-04-11
 * @brief  Implementation for groupchatlistener.
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "groupchatlistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llgroupactions.h"
#include "llimview.h"


namespace {
	void startIm_wrapper(LLSD const & event)
	{
		LLUUID session_id = LLGroupActions::startIM(event["id"].asUUID());
		sendReply(LLSDMap("session_id", LLSD(session_id)), event);
	}

	void send_message_wrapper(const std::string& text, const LLUUID& session_id, const LLUUID& group_id)
	{
		LLIMModel::sendMessage(text, session_id, group_id, IM_SESSION_GROUP_START);
	}
}


GroupChatListener::GroupChatListener():
    LLEventAPI("GroupChat",
               "API to enter, leave, send and intercept group chat messages")
{
    add("startIM",
        "Enter a group chat in group with UUID [\"id\"]\n"
        "Assumes the logged-in agent is already a member of this group.",
        &startIm_wrapper);
    add("endIM",
        "Leave a group chat in group with UUID [\"id\"]\n"
        "Assumes a prior successful startIM request.",
        &LLGroupActions::endIM,
        LLSDArray("id"));
	add("sendIM",
		"send a groupchat IM",
		&send_message_wrapper,
        LLSDArray("text")("session_id")("group_id"));
}
/*
	static void sendMessage(const std::string& utf8_text, const LLUUID& im_session_id,
								const LLUUID& other_participant_id, EInstantMessage dialog);
*/
