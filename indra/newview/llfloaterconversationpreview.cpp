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
#include "llspinctrl.h"
#include "lltrans.h"

LLFloaterConversationPreview::LLFloaterConversationPreview(const LLSD& session_id)
:	LLFloater(session_id),
	mChatHistory(NULL),
	mSessionID(session_id.asUUID()),
	mCurrentPage(0),
	mPageSize(gSavedSettings.getS32("ConversationHistoryPageSize"))
{}

BOOL LLFloaterConversationPreview::postBuild()
{
	mChatHistory = getChild<LLChatHistory>("chat_history");
	getChild<LLUICtrl>("more_history")->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onMoreHistoryBtnClick, this));

	const LLConversation* conv = LLConversationLog::instance().getConversation(mSessionID);
	std::string name;
	std::string file;

	if (mSessionID != LLUUID::null && conv)
	{
		name = conv->getConversationName();
		file = conv->getHistoryFileName();
	}
	else
	{
		name = LLTrans::getString("NearbyChatTitle");
		file = "chat";
	}

	LLStringUtil::format_map_t args;
	args["[NAME]"] = name;
	std::string title = getString("Title", args);
	setTitle(title);

	LLSD load_params;
	load_params["load_all_history"] = true;
	load_params["cut_off_todays_date"] = false;

	LLLogChat::loadChatHistory(file, mMessages, load_params);
	mCurrentPage = mMessages.size() / mPageSize;

	mPageSpinner = getChild<LLSpinCtrl>("history_page_spin");
	mPageSpinner->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onMoreHistoryBtnClick, this));
	mPageSpinner->setMinValue(1);
	mPageSpinner->setMaxValue(mCurrentPage + 1);
	mPageSpinner->set(mCurrentPage + 1);

	std::string total_page_num = llformat("/ %d", mCurrentPage + 1);
	getChild<LLTextBox>("page_num_label")->setValue(total_page_num);

	return LLFloater::postBuild();
}

void LLFloaterConversationPreview::draw()
{
	LLFloater::draw();
}

void LLFloaterConversationPreview::onOpen(const LLSD& key)
{
	showHistory();
}

void LLFloaterConversationPreview::showHistory()
{
	if (!mMessages.size())
	{
		return;
	}

	mChatHistory->clear();

	std::ostringstream message;
	std::list<LLSD>::const_iterator iter = mMessages.begin();

	int delta = 0;
	if (mCurrentPage)
	{
		int remainder = mMessages.size() % mPageSize;
		delta = (remainder == 0) ? 0 : (mPageSize - remainder);
	}

	std::advance(iter, (mCurrentPage * mPageSize) - delta);

	for (int msg_num = 0; (iter != mMessages.end() && msg_num < mPageSize); ++iter, ++msg_num)
	{
		LLSD msg = *iter;

		std::string time	= msg["time"].asString();
		LLUUID from_id		= msg["from_id"].asUUID();
		std::string from	= msg["from"].asString();
		std::string message	= msg["message"].asString();
		bool is_history		= msg["is_history"].asBoolean();

		LLChat chat;
		chat.mFromID = from_id;
		chat.mSessionID = mSessionID;
		chat.mFromName = from;
		chat.mTimeStr = time;
		chat.mChatStyle = is_history ? CHAT_STYLE_HISTORY : chat.mChatStyle;
		chat.mText = message;

		mChatHistory->appendMessage(chat);
	}

}

void LLFloaterConversationPreview::onMoreHistoryBtnClick()
{
	mCurrentPage = (int)(mPageSpinner->getValueF32());
	if (--mCurrentPage < 0)
	{
		return;
	}

	showHistory();
}
