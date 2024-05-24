/**
 * @file   llfloaterimnearbychatlistener.cpp
 * @author Dave Simmons
 * @date   2011-03-15
 * @brief  Implementation for LLFloaterIMNearbyChatListener.
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

#include "llfloaterimnearbychatlistener.h"
#include "llfloaterimnearbychat.h"

#include "llagent.h"
#include "llchat.h"
#include "llviewercontrol.h"
#include "stringize.h"

static const F32 CHAT_THROTTLE_PERIOD = 1.f;

LLFloaterIMNearbyChatListener::LLFloaterIMNearbyChatListener()
  : LLEventAPI("LLChatBar",
               "LLChatBar listener to (e.g.) sendChat, etc."),
    mLastThrottleTime(0)
{
    add("sendChat",
        "Send chat to the simulator:\n"
        "[\"message\"] chat message text [required]\n"
        "[\"channel\"] chat channel number [default = 0]\n"
        "[\"type\"] chat type \"whisper\", \"normal\", \"shout\" [default = \"normal\"]",
        &LLFloaterIMNearbyChatListener::sendChat);
}


// "sendChat" command
void LLFloaterIMNearbyChatListener::sendChat(LLSD const & chat_data)
{
    F64 cur_time = LLTimer::getElapsedSeconds();

    if (cur_time < mLastThrottleTime + CHAT_THROTTLE_PERIOD)
    {
        LL_DEBUGS("LLFloaterIMNearbyChatListener") << "'sendChat' was  throttled" << LL_ENDL;
        return;
    }
    mLastThrottleTime = cur_time;

    // Extract the data
    std::string chat_text = LUA_PREFIX + chat_data["message"].asString();

    S32 channel = 0;
    if (chat_data.has("channel"))
    {
        channel = chat_data["channel"].asInteger();
        if (channel < 0 || channel >= CHAT_CHANNEL_DEBUG)
        {   // Use 0 up to (but not including) CHAT_CHANNEL_DEBUG
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
    if (channel)
    {
        chat_text = stringize("/", chat_data["channel"].asString(), " ", chat_text);
    }

    // Send it as if it was typed in
    LLFloaterIMNearbyChat::sendChatFromViewer(chat_text, type_o_chat,
                                              (channel == 0) &&
                                              gSavedSettings.getBOOL("PlayChatAnim"));
}

