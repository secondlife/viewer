/**
 * @file llfloaterconversationpreview.cpp
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llconversationlog.h"
#include "llfloaterconversationpreview.h"
#include "llimview.h"
#include "lllineeditor.h"

LLFloaterConversationPreview::LLFloaterConversationPreview(const LLSD& session_id)
:	LLFloater(session_id),
	mChatHistory(NULL),
	mSessionID(session_id.asUUID())
{}

BOOL LLFloaterConversationPreview::postBuild()
{
	mChatHistory = getChild<LLChatHistory>("chat_history");

	const LLConversation* conv = LLConversationLog::instance().getConversation(mSessionID);
	if (conv)
	{
		std::string name = conv->getConversationName();
		LLStringUtil::format_map_t args;
		args["[NAME]"] = name;
		std::string title = getString("Title", args);
		setTitle(title);

		getChild<LLLineEditor>("description")->setValue(name);
	}

	return LLFloater::postBuild();
}

void LLFloaterConversationPreview::draw()
{
	LLFloater::draw();
}

void LLFloaterConversationPreview::onOpen(const LLSD& session_id)
{
	const LLConversation* conv = LLConversationLog::instance().getConversation(session_id);
	if (!conv)
	{
		return;
	}
	std::list<LLSD> messages;
	std::string file = conv->getHistoryFileName();
	LLLogChat::loadAllHistory(file, messages);

	if (messages.size())
	{
		std::ostringstream message;
		std::list<LLSD>::const_iterator iter = messages.begin();
		for (; iter != messages.end(); ++iter)
		{
			LLSD msg = *iter;

			std::string time	= msg["time"].asString();
			LLUUID from_id		= msg["from_id"].asUUID();
			std::string from	= msg["from"].asString();
			std::string message	= msg["message"].asString();
			bool is_history	= msg["is_history"].asBoolean();

			LLChat chat;
			chat.mFromID = from_id;
			chat.mSessionID = session_id;
			chat.mFromName = from;
			chat.mTimeStr = time;
			chat.mChatStyle = is_history ? CHAT_STYLE_HISTORY : chat.mChatStyle;
			chat.mText = message;

			appendMessage(chat);
		}
	}
}

void LLFloaterConversationPreview::appendMessage(const LLChat& chat)
{
	if (!chat.mMuted)
	{
		LLSD args;
		args["use_plain_text_chat_history"] = true;
		args["show_time"] = true;
		args["show_names_for_p2p_conv"] = true;

		mChatHistory->appendMessage(chat);
	}
}
