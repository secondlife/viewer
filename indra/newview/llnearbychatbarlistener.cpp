/**
 * @file   llnearbychatbarlistener.cpp
 * @author Dave Simmons
 * @date   2011-03-15
 * @brief  Implementation for LLNearbyChatBarListener.
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

#include "llviewerprecompiledheaders.h"

#include "llnearbychatbarlistener.h"
#include "llnearbychatbar.h"

#include "llagent.h"
#include "llchat.h"



LLNearbyChatBarListener::LLNearbyChatBarListener(LLNearbyChatBar & chatbar)
  : LLEventAPI("LLChatBar",
               "LLChatBar listener to (e.g.) sendChat, etc."),
	mChatbar(chatbar)
{
    add("sendChat",
        "Send chat to the simulator:\n"
        "[\"message\"] chat message text [required]\n"
        "[\"channel\"] chat channel number [default = 0]\n"
		"[\"type\"] chat type \"whisper\", \"normal\", \"shout\" [default = \"normal\"]",
        &LLNearbyChatBarListener::sendChat);
}


// "sendChat" command
void LLNearbyChatBarListener::sendChat(LLSD const & chat_data) const
{
	// Extract the data
	std::string chat_text = chat_data["message"].asString();

	S32 channel = 0;
	if (chat_data.has("channel"))
	{
		channel = chat_data["channel"].asInteger();
		if (channel < 0 || channel >= CHAT_CHANNEL_DEBUG)
		{	// Use 0 up to (but not including) CHAT_CHANNEL_DEBUG
			channel = 0;
		}
	}

	EChatType type_o_chat = CHAT_TYPE_NORMAL;
	if (chat_data.has("type"))
	{
		std::string type_string = chat_data["type"].asString();
		if (type_string == "whisper")
		{
			type_o_chat = CHAT_TYPE_WHISPER;
		}
		else if (type_string == "shout")
		{
			type_o_chat = CHAT_TYPE_SHOUT;
		}
	}

	// Have to prepend /42 style channel numbers
	std::string chat_to_send;
	if (channel == 0)
	{
		chat_to_send = chat_text;
	}
	else
	{
		chat_to_send += "/";
		chat_to_send += chat_data["channel"].asString();
		chat_to_send += " ";
		chat_to_send += chat_text;
	}

	// Send it as if it was typed in
	mChatbar.sendChatFromViewer(chat_to_send, type_o_chat, (BOOL)(channel == 0));
}

