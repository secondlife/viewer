/** 
 * @file LLNearbyChat.cpp
 * @brief LLNearbyChat class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "message.h"

#include "lliconctrl.h"
#include "llappviewer.h"
#include "llchatentry.h"
#include "llfloaterreg.h"
#include "lltrans.h"
#include "llimfloatercontainer.h"
#include "llfloatersidepanelcontainer.h"
#include "llfocusmgr.h"
#include "lllogchat.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "lldraghandle.h"
#include "llmenugl.h"
#include "llviewermenu.h" // for gMenuHolder
#include "llnearbychathandler.h"
#include "llchannelmanager.h"
#include "llchathistory.h"
#include "llstylemap.h"
#include "llavatarnamecache.h"
#include "llfloaterreg.h"
#include "lltrans.h"

#include "llfirstuse.h"
#include "llnearbychat.h"
#include "llagent.h" // gAgent
#include "llgesturemgr.h"
#include "llmultigesture.h"
#include "llkeyboard.h"
#include "llanimationstates.h"
#include "llviewerstats.h"
#include "llcommandhandler.h"
#include "llviewercontrol.h"
#include "llnavigationbar.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llrootview.h"
#include "llviewerchat.h"
#include "lltranslate.h"

S32 LLNearbyChat::sLastSpecialChatChannel = 0;

// --- 2 functions in the global namespace :( ---
bool isWordsName(const std::string& name)
{
	// checking to see if it's display name plus username in parentheses
	S32 open_paren = name.find(" (", 0);
	S32 close_paren = name.find(')', 0);

	if (open_paren != std::string::npos &&
		close_paren == name.length()-1)
	{
		return true;
	}
	else
	{
		//checking for a single space
		S32 pos = name.find(' ', 0);
		return std::string::npos != pos && name.rfind(' ', name.length()) == pos && 0 != pos && name.length()-1 != pos;
	}
}

std::string appendTime()
{
	time_t utc_time;
	utc_time = time_corrected();
	std::string timeStr ="["+ LLTrans::getString("TimeHour")+"]:["
		+LLTrans::getString("TimeMin")+"]";

	LLSD substitution;

	substitution["datetime"] = (S32) utc_time;
	LLStringUtil::format (timeStr, substitution);

	return timeStr;
}


const S32 EXPANDED_HEIGHT = 266;
const S32 COLLAPSED_HEIGHT = 60;
const S32 EXPANDED_MIN_HEIGHT = 150;

// legacy callback glue
void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel);

struct LLChatTypeTrigger {
	std::string name;
	EChatType type;
};

static LLChatTypeTrigger sChatTypeTriggers[] = {
	{ "/whisper"	, CHAT_TYPE_WHISPER},
	{ "/shout"	, CHAT_TYPE_SHOUT}
};


LLNearbyChat::LLNearbyChat(const LLSD& key)
:	LLIMConversation(key),
	mChatBox(NULL),
	mChatHistory(NULL),
	//mOutputMonitor(NULL),
	mSpeakerMgr(NULL),
	mExpandedHeight(COLLAPSED_HEIGHT + EXPANDED_HEIGHT)
{
	setIsChrome(TRUE);
	mKey = LLSD();
	mIsNearbyChat = true;
	mSpeakerMgr = LLLocalSpeakerMgr::getInstance();
}

//virtual
BOOL LLNearbyChat::postBuild()
{
	mChatBox = getChild<LLChatEntry>("chat_editor");

	mChatBox->setCommitCallback(boost::bind(&LLNearbyChat::onChatBoxCommit, this));
	mChatBox->setKeystrokeCallback(boost::bind(&onChatBoxKeystroke, _1, this));
	mChatBox->setFocusLostCallback(boost::bind(&onChatBoxFocusLost, _1, this));
	mChatBox->setFocusReceivedCallback(boost::bind(&LLNearbyChat::onChatBoxFocusReceived, this));
	mChatBox->setCommitOnFocusLost( FALSE );
	mChatBox->setPassDelete(TRUE);
	mChatBox->setFont(LLViewerChat::getChatFont());

//	mOutputMonitor = getChild<LLOutputMonitorCtrl>("chat_zone_indicator");
//	mOutputMonitor->setVisible(FALSE);

	// Register for font change notifications
	LLViewerChat::setFontChangedCallback(boost::bind(&LLNearbyChat::onChatFontChange, this, _1));

	enableResizeCtrls(true, true, false);

	addToHost();

	//for menu
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	enable_registrar.add("NearbyChat.Check", boost::bind(&LLNearbyChat::onNearbyChatCheckContextMenuItem, this, _2));
	registrar.add("NearbyChat.Action", boost::bind(&LLNearbyChat::onNearbyChatContextMenuItemClicked, this, _2));

	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_nearby_chat.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(menu)
	{
		mPopupMenuHandle = menu->getHandle();
	}

	// obsolete, but may be needed for backward compatibility?
	gSavedSettings.declareS32("nearbychat_showicons_and_names", 2, "NearByChat header settings", true);

	mChatHistory = getChild<LLChatHistory>("chat_history");
	if (gSavedPerAccountSettings.getBOOL("LogShowHistory"))
	{
		loadHistory();
	}

	setTitle(getString("NearbyChatTitle"));

	return LLIMConversation::postBuild();
}

// virtual
void LLNearbyChat::refresh()
{
	displaySpeakingIndicator();
	updateCallBtnState(LLVoiceClient::getInstance()->getUserPTTState());

	// *HACK: Update transparency type depending on whether our children have focus.
	// This is needed because this floater is chrome and thus cannot accept focus, so
	// the transparency type setting code from LLFloater::setFocus() isn't reached.
	if (getTransparencyType() != TT_DEFAULT)
	{
		setTransparencyType(hasFocus() ? TT_ACTIVE : TT_INACTIVE);
	}
}

void LLNearbyChat::onNearbySpeakers()
{
	LLSD param;
	param["people_panel_tab_name"] = "nearby_panel";
	LLFloaterSidePanelContainer::showPanel("people", "panel_people", param);
}

void	LLNearbyChat::onNearbyChatContextMenuItemClicked(const LLSD& userdata)
{
}

bool	LLNearbyChat::onNearbyChatCheckContextMenuItem(const LLSD& userdata)
{
	std::string str = userdata.asString();
	if(str == "nearby_people")
		onNearbySpeakers();
	return false;
}

void LLNearbyChat::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRectScaled();
}
////////////////////////////////////////////////////////////////////////////////
//
void LLNearbyChat::onFocusReceived()
{
	setBackgroundOpaque(true);
	LLIMConversation::onFocusReceived();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLNearbyChat::onFocusLost()
{
	setBackgroundOpaque(false);
	LLIMConversation::onFocusLost();
}

BOOL	LLNearbyChat::handleMouseDown(S32 x, S32 y, MASK mask)
{
	//fix for EXT-6625
	//highlight NearbyChat history whenever mouseclick happen in NearbyChat
	//setting focus to eidtor will force onFocusLost() call that in its turn will change
	//background opaque. This all happenn since NearByChat is "chrome" and didn't process focus change.

	if(mChatHistory)
		mChatHistory->setFocus(TRUE);
	return LLPanel::handleMouseDown(x, y, mask);
}

void LLNearbyChat::reloadMessages()
{
	mChatHistory->clear();

	LLSD do_not_log;
	do_not_log["do_not_log"] = true;
	for(std::vector<LLChat>::iterator it = mMessageArchive.begin();it!=mMessageArchive.end();++it)
	{
		// Update the messages without re-writing them to a log file.
		addMessage(*it,false, do_not_log);
	}
}

void LLNearbyChat::loadHistory()
{
	LLSD do_not_log;
	do_not_log["do_not_log"] = true;

	std::list<LLSD> history;
	LLLogChat::loadAllHistory("chat", history);

	std::list<LLSD>::const_iterator it = history.begin();
	while (it != history.end())
	{
		const LLSD& msg = *it;

		std::string from = msg[IM_FROM];
		LLUUID from_id;
		if (msg[IM_FROM_ID].isDefined())
		{
			from_id = msg[IM_FROM_ID].asUUID();
		}
		else
 		{
			std::string legacy_name = gCacheName->buildLegacyName(from);
 			gCacheName->getUUID(legacy_name, from_id);
 		}

		LLChat chat;
		chat.mFromName = from;
		chat.mFromID = from_id;
		chat.mText = msg[IM_TEXT].asString();
		chat.mTimeStr = msg[IM_TIME].asString();
		chat.mChatStyle = CHAT_STYLE_HISTORY;

		chat.mSourceType = CHAT_SOURCE_AGENT;
		if (from_id.isNull() && SYSTEM_FROM == from)
		{
			chat.mSourceType = CHAT_SOURCE_SYSTEM;

		}
		else if (from_id.isNull())
		{
			chat.mSourceType = isWordsName(from) ? CHAT_SOURCE_UNKNOWN : CHAT_SOURCE_OBJECT;
		}

		addMessage(chat, true, do_not_log);

		it++;
	}
}

void LLNearbyChat::removeScreenChat()
{
	LLNotificationsUI::LLScreenChannelBase* chat_channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(LLUUID(gSavedSettings.getString("NearByChatChannelUUID")));
	if(chat_channel)
	{
		chat_channel->removeToastsFromChannel();
	}
}

void	LLNearbyChat::setVisible(BOOL visible)
{
	if(visible)
	{
		removeScreenChat();
	}

	LLIMConversation::setVisible(visible);
}


void LLNearbyChat::enableDisableCallBtn()
{
	// bool btn_enabled = LLAgent::isActionAllowed("speak");

	getChildView("voice_call_btn")->setEnabled(false /*btn_enabled*/);
}

void LLNearbyChat::addToHost()
{
	if (LLIMConversation::isChatMultiTab())
	{
		LLIMFloaterContainer* im_box = LLIMFloaterContainer::getInstance();

		if (im_box)
		{
			im_box->addFloater(this, FALSE, LLTabContainer::END);
		}
	}
}

// virtual
void LLNearbyChat::onOpen(const LLSD& key)
{
	LLIMConversation::onOpen(key);
	showTranslationCheckbox(LLTranslate::isTranslationConfigured());
}

bool LLNearbyChat::applyRectControl()
{
	bool rect_controlled = LLFloater::applyRectControl();

/*	if (!mNearbyChat->getVisible())
	{
		reshape(getRect().getWidth(), getMinHeight());
		enableResizeCtrls(true, true, false);
	}
	else
	{*/
		enableResizeCtrls(true);
		setResizeLimits(getMinWidth(), EXPANDED_MIN_HEIGHT);
//	}
	
	return rect_controlled;
}

void LLNearbyChat::onChatFontChange(LLFontGL* fontp)
{
	// Update things with the new font whohoo
	if (mChatBox)
	{
		mChatBox->setFont(fontp);
	}
}

//static
LLNearbyChat* LLNearbyChat::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLNearbyChat>("chat_bar");
}

void LLNearbyChat::show()
{
	if (isChatMultiTab())
	{
		openFloater(getKey());
	}
	setVisible(TRUE);
}

void LLNearbyChat::showHistory()
{
	openFloater();
	setResizeLimits(getMinWidth(), EXPANDED_MIN_HEIGHT);
	reshape(getRect().getWidth(), mExpandedHeight);
	enableResizeCtrls(true);
	storeRectControl();
}

std::string LLNearbyChat::getCurrentChat()
{
	return mChatBox ? mChatBox->getText() : LLStringUtil::null;
}

// virtual
BOOL LLNearbyChat::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;

	if( KEY_RETURN == key && mask == MASK_CONTROL)
	{
		// shout
		sendChat(CHAT_TYPE_SHOUT);
		handled = TRUE;
	}

	return handled;
}

BOOL LLNearbyChat::matchChatTypeTrigger(const std::string& in_str, std::string* out_str)
{
	U32 in_len = in_str.length();
	S32 cnt = sizeof(sChatTypeTriggers) / sizeof(*sChatTypeTriggers);
	
	bool string_was_found = false;

	for (S32 n = 0; n < cnt && !string_was_found; n++)
	{
		if (in_len <= sChatTypeTriggers[n].name.length())
		{
			std::string trigger_trunc = sChatTypeTriggers[n].name;
			LLStringUtil::truncate(trigger_trunc, in_len);

			if (!LLStringUtil::compareInsensitive(in_str, trigger_trunc))
			{
				*out_str = sChatTypeTriggers[n].name;
				string_was_found = true;
			}
		}
	}

	return string_was_found;
}

void LLNearbyChat::onChatBoxKeystroke(LLTextEditor* caller, void* userdata)
{
	LLFirstUse::otherAvatarChatFirst(false);

	LLNearbyChat* self = (LLNearbyChat *)userdata;

	LLWString raw_text = self->mChatBox->getWText();

	// Can't trim the end, because that will cause autocompletion
	// to eat trailing spaces that might be part of a gesture.
	LLWStringUtil::trimHead(raw_text);

	S32 length = raw_text.length();

	if( (length > 0) && (raw_text[0] != '/') )  // forward slash is used for escape (eg. emote) sequences
	{
		gAgent.startTyping();
	}
	else
	{
		gAgent.stopTyping();
	}

	/* Doesn't work -- can't tell the difference between a backspace
	   that killed the selection vs. backspace at the end of line.
	if (length > 1 
		&& text[0] == '/'
		&& key == KEY_BACKSPACE)
	{
		// the selection will already be deleted, but we need to trim
		// off the character before
		std::string new_text = raw_text.substr(0, length-1);
		self->mInputEditor->setText( new_text );
		self->mInputEditor->setCursorToEnd();
		length = length - 1;
	}
	*/

	KEY key = gKeyboard->currentKey();

	// Ignore "special" keys, like backspace, arrows, etc.
	if (length > 1 
		&& raw_text[0] == '/'
		&& key < KEY_SPECIAL)
	{
		// we're starting a gesture, attempt to autocomplete

		std::string utf8_trigger = wstring_to_utf8str(raw_text);
		std::string utf8_out_str(utf8_trigger);

		if (LLGestureMgr::instance().matchPrefix(utf8_trigger, &utf8_out_str))
		{
			std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
			self->mChatBox->setText(utf8_trigger + rest_of_match); // keep original capitalization for user-entered part

			// Select to end of line, starting from the character
			// after the last one the user typed.
			self->mChatBox->selectNext(rest_of_match, false);
		}
		else if (matchChatTypeTrigger(utf8_trigger, &utf8_out_str))
		{
			std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
			self->mChatBox->setText(utf8_trigger + rest_of_match + " "); // keep original capitalization for user-entered part
			self->mChatBox->endOfDoc();
		}

		//llinfos << "GESTUREDEBUG " << trigger 
		//	<< " len " << length
		//	<< " outlen " << out_str.getLength()
		//	<< llendl;
	}
}

// static
void LLNearbyChat::onChatBoxFocusLost(LLFocusableElement* caller, void* userdata)
{
	// stop typing animation
	gAgent.stopTyping();
}

void LLNearbyChat::onChatBoxFocusReceived()
{
	mChatBox->setEnabled(!gDisconnected);
}

EChatType LLNearbyChat::processChatTypeTriggers(EChatType type, std::string &str)
{
	U32 length = str.length();
	S32 cnt = sizeof(sChatTypeTriggers) / sizeof(*sChatTypeTriggers);
	
	for (S32 n = 0; n < cnt; n++)
	{
		if (length >= sChatTypeTriggers[n].name.length())
		{
			std::string trigger = str.substr(0, sChatTypeTriggers[n].name.length());

			if (!LLStringUtil::compareInsensitive(trigger, sChatTypeTriggers[n].name))
			{
				U32 trigger_length = sChatTypeTriggers[n].name.length();

				// It's to remove space after trigger name
				if (length > trigger_length && str[trigger_length] == ' ')
					trigger_length++;

				str = str.substr(trigger_length, length);

				if (CHAT_TYPE_NORMAL == type)
					return sChatTypeTriggers[n].type;
				else
					break;
			}
		}
	}

	return type;
}

void LLNearbyChat::sendChat( EChatType type )
{
	if (mChatBox)
	{
		LLWString text = mChatBox->getWText();
		LLWStringUtil::trim(text);
		LLWStringUtil::replaceChar(text,182,'\n'); // Convert paragraph symbols back into newlines.
		if (!text.empty())
		{
			// Check if this is destined for another channel
			S32 channel = 0;
			stripChannelNumber(text, &channel);
			
			std::string utf8text = wstring_to_utf8str(text);
			// Try to trigger a gesture, if not chat to a script.
			std::string utf8_revised_text;
			if (0 == channel)
			{
				// discard returned "found" boolean
				LLGestureMgr::instance().triggerAndReviseString(utf8text, &utf8_revised_text);
			}
			else
			{
				utf8_revised_text = utf8text;
			}

			utf8_revised_text = utf8str_trim(utf8_revised_text);

			type = processChatTypeTriggers(type, utf8_revised_text);

			if (!utf8_revised_text.empty())
			{
				// Chat with animation
				sendChatFromViewer(utf8_revised_text, type, TRUE);
			}
		}

		mChatBox->setText(LLStringExplicit(""));
	}

	gAgent.stopTyping();

	// If the user wants to stop chatting on hitting return, lose focus
	// and go out of chat mode.
	if (gSavedSettings.getBOOL("CloseChatOnReturn"))
	{
		stopChat();
	}
}


void LLNearbyChat::appendMessage(const LLChat& chat, const LLSD &args)
{
	LLChat& tmp_chat = const_cast<LLChat&>(chat);

	if(tmp_chat.mTimeStr.empty())
		tmp_chat.mTimeStr = appendTime();

	if (!chat.mMuted)
	{
		tmp_chat.mFromName = chat.mFromName;
		LLSD chat_args;
		if (args) chat_args = args;
		chat_args["use_plain_text_chat_history"] =
				gSavedSettings.getBOOL("PlainTextChatHistory");
		chat_args["show_time"] = gSavedSettings.getBOOL("IMShowTime");
		chat_args["show_names_for_p2p_conv"] = true;

		mChatHistory->appendMessage(chat, chat_args);
	}
}

void	LLNearbyChat::addMessage(const LLChat& chat,bool archive,const LLSD &args)
{
	appendMessage(chat, args);

	if(archive)
	{
		mMessageArchive.push_back(chat);
		if(mMessageArchive.size()>200)
			mMessageArchive.erase(mMessageArchive.begin());
	}

	// logging
	if (!args["do_not_log"].asBoolean()
			&& gSavedPerAccountSettings.getBOOL("LogNearbyChat"))
	{
		std::string from_name = chat.mFromName;

		if (chat.mSourceType == CHAT_SOURCE_AGENT)
		{
			// if the chat is coming from an agent, log the complete name
			LLAvatarName av_name;
			LLAvatarNameCache::get(chat.mFromID, &av_name);

			if (!av_name.mIsDisplayNameDefault)
			{
				from_name = av_name.getCompleteName();
			}
		}

		LLLogChat::saveHistory("chat", from_name, chat.mFromID, chat.mText);
	}
}


void LLNearbyChat::onChatBoxCommit()
{
	if (mChatBox->getText().length() > 0)
	{
		sendChat(CHAT_TYPE_NORMAL);
	}

	gAgent.stopTyping();
}

void LLNearbyChat::displaySpeakingIndicator()
{
	LLSpeakerMgr::speaker_list_t speaker_list;
	LLUUID id;

	id.setNull();
	mSpeakerMgr->update(TRUE);
	mSpeakerMgr->getSpeakerList(&speaker_list, FALSE);

	for (LLSpeakerMgr::speaker_list_t::iterator i = speaker_list.begin(); i != speaker_list.end(); ++i)
	{
		LLPointer<LLSpeaker> s = *i;
		if (s->mSpeechVolume > 0 || s->mStatus == LLSpeaker::STATUS_SPEAKING)
		{
			id = s->mID;
			break;
		}
	}

	if (!id.isNull())
	{
		//mOutputMonitor->setVisible(TRUE);
		//mOutputMonitor->setSpeakerId(id);
	}
	else
	{
		//mOutputMonitor->setVisible(FALSE);
	}
}

void LLNearbyChat::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLNearbyChat::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
{
	// Look for "/20 foo" channel chats.
	S32 channel = 0;
	LLWString out_text = stripChannelNumber(wtext, &channel);
	std::string utf8_out_text = wstring_to_utf8str(out_text);
	std::string utf8_text = wstring_to_utf8str(wtext);

	utf8_text = utf8str_trim(utf8_text);
	if (!utf8_text.empty())
	{
		utf8_text = utf8str_truncate(utf8_text, MAX_STRING - 1);
	}

	// Don't animate for chats people can't hear (chat to scripts)
	if (animate && (channel == 0))
	{
		if (type == CHAT_TYPE_WHISPER)
		{
			lldebugs << "You whisper " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_WHISPER, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_NORMAL)
		{
			lldebugs << "You say " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_TALK, ANIM_REQUEST_START);
		}
		else if (type == CHAT_TYPE_SHOUT)
		{
			lldebugs << "You shout " << utf8_text << llendl;
			gAgent.sendAnimationRequest(ANIM_AGENT_SHOUT, ANIM_REQUEST_START);
		}
		else
		{
			llinfos << "send_chat_from_viewer() - invalid volume" << llendl;
			return;
		}
	}
	else
	{
		if (type != CHAT_TYPE_START && type != CHAT_TYPE_STOP)
		{
			lldebugs << "Channel chat: " << utf8_text << llendl;
		}
	}

	send_chat_from_viewer(utf8_out_text, type, channel);
}

// static 
void LLNearbyChat::startChat(const char* line)
{
	LLNearbyChat* cb = LLNearbyChat::getInstance();

	if (cb )
	{
		cb->show();
		cb->setVisible(TRUE);
		cb->setFocus(TRUE);
		cb->mChatBox->setFocus(TRUE);

		if (line)
		{
			std::string line_string(line);
			cb->mChatBox->setText(line_string);
		}

		cb->mChatBox->endOfDoc();
	}
}

// Exit "chat mode" and do the appropriate focus changes
// static
void LLNearbyChat::stopChat()
{
	LLNearbyChat* cb = LLNearbyChat::getInstance();

	if (cb)
	{
		cb->mChatBox->setFocus(FALSE);

		// stop typing animation
		gAgent.stopTyping();
	}
}

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLNearbyChat::stripChannelNumber(const LLWString &mesg, S32* channel)
{
	if (mesg[0] == '/'
		&& mesg[1] == '/')
	{
		// This is a "repeat channel send"
		*channel = sLastSpecialChatChannel;
		return mesg.substr(2, mesg.length() - 2);
	}
	else if (mesg[0] == '/'
			 && mesg[1]
			 && LLStringOps::isDigit(mesg[1]))
	{
		// This a special "/20" speak on a channel
		S32 pos = 0;

		// Copy the channel number into a string
		LLWString channel_string;
		llwchar c;
		do
		{
			c = mesg[pos+1];
			channel_string.push_back(c);
			pos++;
		}
		while(c && pos < 64 && LLStringOps::isDigit(c));
		
		// Move the pointer forward to the first non-whitespace char
		// Check isspace before looping, so we can handle "/33foo"
		// as well as "/33 foo"
		while(c && iswspace(c))
		{
			c = mesg[pos+1];
			pos++;
		}
		
		sLastSpecialChatChannel = strtol(wstring_to_utf8str(channel_string).c_str(), NULL, 10);
		*channel = sLastSpecialChatChannel;
		return mesg.substr(pos, mesg.length() - pos);
	}
	else
	{
		// This is normal chat.
		*channel = 0;
		return mesg;
	}
}

void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ChatFromViewer);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ChatData);
	msg->addStringFast(_PREHASH_Message, utf8_out_text);
	msg->addU8Fast(_PREHASH_Type, type);
	msg->addS32("Channel", channel);

	gAgent.sendReliableMessage();

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CHAT_COUNT);
}

class LLChatCommandHandler : public LLCommandHandler
{
public:
	// not allowed from outside the app
	LLChatCommandHandler() : LLCommandHandler("chat", UNTRUSTED_BLOCK) { }

    // Your code here
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLMediaCtrl* web)
	{
		bool retval = false;
		// Need at least 2 tokens to have a valid message.
		if (tokens.size() < 2)
		{
			retval = false;
		}
		else
		{
		S32 channel = tokens[0].asInteger();
			// VWR-19499 Restrict function to chat channels greater than 0.
			if ((channel > 0) && (channel < CHAT_CHANNEL_DEBUG))
			{
				retval = true;
		// Send unescaped message, see EXT-6353.
		std::string unescaped_mesg (LLURI::unescape(tokens[1].asString()));
		send_chat_from_viewer(unescaped_mesg, CHAT_TYPE_NORMAL, channel);
			}
			else
			{
				retval = false;
				// Tell us this is an unsupported SLurl.
			}
		}
		return retval;
	}
};

// Creating the object registers with the dispatcher.
LLChatCommandHandler gChatHandler;
