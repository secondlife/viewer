/** 
 * @file LLFloaterIMNearbyChat.cpp
 * @brief LLFloaterIMNearbyChat class implementation
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
#include "llfloaterimcontainer.h"
#include "llfloatersidepanelcontainer.h"
#include "llfocusmgr.h"
#include "lllogchat.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "lldraghandle.h"
#include "llmenugl.h"
#include "llviewermenu.h" // for gMenuHolder
#include "llfloaterimnearbychathandler.h"
#include "llchannelmanager.h"
#include "llchathistory.h"
#include "llstylemap.h"
#include "llavatarnamecache.h"
#include "llfloaterreg.h"
#include "lltrans.h"

#include "llfirstuse.h"
#include "llfloaterimnearbychat.h"
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
#include "llautoreplace.h"

S32 LLFloaterIMNearbyChat::sLastSpecialChatChannel = 0;

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


LLFloaterIMNearbyChat::LLFloaterIMNearbyChat(const LLSD& llsd)
:	LLFloaterIMSessionTab(llsd),
	//mOutputMonitor(NULL),
	mSpeakerMgr(NULL),
	mExpandedHeight(COLLAPSED_HEIGHT + EXPANDED_HEIGHT)
{
    mIsP2PChat = false;
	mIsNearbyChat = true;
	mSpeakerMgr = LLLocalSpeakerMgr::getInstance();
	mSessionID = LLUUID();
}

//static
LLFloaterIMNearbyChat* LLFloaterIMNearbyChat::buildFloater(const LLSD& key)
{
    LLFloaterReg::getInstance("im_container");
    return new LLFloaterIMNearbyChat(key);
}

//virtual
BOOL LLFloaterIMNearbyChat::postBuild()
{
    setIsSingleInstance(TRUE);
    BOOL result = LLFloaterIMSessionTab::postBuild();

	mInputEditor->setAutoreplaceCallback(boost::bind(&LLAutoReplace::autoreplaceCallback, LLAutoReplace::getInstance(), _1, _2, _3, _4, _5));
	mInputEditor->setCommitCallback(boost::bind(&LLFloaterIMNearbyChat::onChatBoxCommit, this));
	mInputEditor->setKeystrokeCallback(boost::bind(&LLFloaterIMNearbyChat::onChatBoxKeystroke, this));
	mInputEditor->setFocusLostCallback(boost::bind(&LLFloaterIMNearbyChat::onChatBoxFocusLost, this));
	mInputEditor->setFocusReceivedCallback(boost::bind(&LLFloaterIMNearbyChat::onChatBoxFocusReceived, this));
	mInputEditor->setLabel(LLTrans::getString("NearbyChatTitle"));

	// Title must be defined BEFORE call to addConversationListItem() because
	// it is used to show the item's name in the conversations list
	setTitle(LLTrans::getString("NearbyChatTitle"));

	// obsolete, but may be needed for backward compatibility?
	gSavedSettings.declareS32("nearbychat_showicons_and_names", 2, "NearByChat header settings", LLControlVariable::PERSIST_NONDFT);

	if (gSavedPerAccountSettings.getBOOL("LogShowHistory"))
	{
		loadHistory();
	}

	return result;
}

// virtual
void LLFloaterIMNearbyChat::closeHostedFloater()
{
	// If detached from conversations window close anyway
	if (!getHost())
	{
		setVisible(FALSE);
	}

	// Should check how many conversations are ongoing. Select next to "Nearby Chat" in case there are some other besides.
	// Close conversations window in case "Nearby Chat" is attached and the only conversation
	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
	if (floater_container->getConversationListItemSize() == 1)
	{
		if (getHost())
		{
			floater_container->closeFloater();
		}
	}
	else
	{
		if (!getHost())
		{
			floater_container->selectNextConversationByID(LLUUID());
		}
	}
}

// virtual
void LLFloaterIMNearbyChat::refresh()
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

void LLFloaterIMNearbyChat::reloadMessages(bool clean_messages/* = false*/)
{
	if (clean_messages)
	{
		mMessageArchive.clear();
		loadHistory();
	}

	mChatHistory->clear();

	LLSD do_not_log;
	do_not_log["do_not_log"] = true;
	for(std::vector<LLChat>::iterator it = mMessageArchive.begin();it!=mMessageArchive.end();++it)
	{
		// Update the messages without re-writing them to a log file.
		addMessage(*it,false, do_not_log);
	}
}

void LLFloaterIMNearbyChat::loadHistory()
{
	LLSD do_not_log;
	do_not_log["do_not_log"] = true;

	std::list<LLSD> history;
	LLLogChat::loadChatHistory("chat", history);

	std::list<LLSD>::const_iterator it = history.begin();
	while (it != history.end())
	{
		const LLSD& msg = *it;

		std::string from = msg[LL_IM_FROM];
		LLUUID from_id;
		if (msg[LL_IM_FROM_ID].isDefined())
		{
			from_id = msg[LL_IM_FROM_ID].asUUID();
		}
		else
 		{
			std::string legacy_name = gCacheName->buildLegacyName(from);
 			gCacheName->getUUID(legacy_name, from_id);
 		}

		LLChat chat;
		chat.mFromName = from;
		chat.mFromID = from_id;
		chat.mText = msg[LL_IM_TEXT].asString();
		chat.mTimeStr = msg[LL_IM_TIME].asString();
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

void LLFloaterIMNearbyChat::removeScreenChat()
{
	LLNotificationsUI::LLScreenChannelBase* chat_channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(LLUUID(gSavedSettings.getString("NearByChatChannelUUID")));
	if(chat_channel)
	{
		chat_channel->removeToastsFromChannel();
	}
}


void LLFloaterIMNearbyChat::setVisible(BOOL visible)
{
	LLFloaterIMSessionTab::setVisible(visible);

	if(visible)
	{
		removeScreenChat();
	}
}


void LLFloaterIMNearbyChat::setVisibleAndFrontmost(BOOL take_focus, const LLSD& key)
{
	LLFloaterIMSessionTab::setVisibleAndFrontmost(take_focus, key);

	if(matchesKey(key))
	{
		LLFloaterIMContainer::getInstance()->selectConversationPair(mSessionID, true, take_focus);
	}
}

// virtual
void LLFloaterIMNearbyChat::onTearOffClicked()
{
	LLFloaterIMSessionTab::onTearOffClicked();

	// see CHUI-170: Save torn-off state of the nearby chat between sessions
	BOOL in_the_multifloater = (BOOL)getHost();
	gSavedPerAccountSettings.setBOOL("NearbyChatIsNotTornOff", in_the_multifloater);
}


// virtual
void LLFloaterIMNearbyChat::onOpen(const LLSD& key)
{
	LLFloaterIMSessionTab::onOpen(key);
	if(!isMessagePaneExpanded())
	{
		restoreFloater();
		onCollapseToLine(this);
	}
	showTranslationCheckbox(LLTranslate::isTranslationConfigured());
}

// virtual
void LLFloaterIMNearbyChat::onClose(bool app_quitting)
{
	// Override LLFloaterIMSessionTab::onClose() so that Nearby Chat is not removed from the conversation floater
	LLFloaterIMSessionTab::restoreFloater();
}

// virtual
void LLFloaterIMNearbyChat::onClickCloseBtn(bool)

{
	if (!isTornOff())
	{
		return;
	}
	closeHostedFloater();
}

void LLFloaterIMNearbyChat::onChatFontChange(LLFontGL* fontp)
{
	// Update things with the new font whohoo
	if (mInputEditor)
	{
		mInputEditor->setFont(fontp);
	}
}


void LLFloaterIMNearbyChat::show()
{
		openFloater(getKey());
	}

bool LLFloaterIMNearbyChat::isChatVisible() const
{
	bool isVisible = false;
	LLFloaterIMContainer* im_box = LLFloaterIMContainer::getInstance();
	// Is the IM floater container ever null?
	llassert(im_box != NULL);
	if (im_box != NULL)
	{
		isVisible =
				isChatMultiTab() && gSavedPerAccountSettings.getBOOL("NearbyChatIsNotTornOff")?
						im_box->getVisible() && !im_box->isMinimized() :
						getVisible() && !isMinimized();
	}

	return isVisible;
}

void LLFloaterIMNearbyChat::showHistory()
{
	openFloater();
	LLFloaterIMContainer::getInstance()->selectConversation(LLUUID(NULL));

	if(!isMessagePaneExpanded())
	{
		restoreFloater();
		setFocus(true);
	}
	else
	{
		LLFloaterIMContainer::getInstance()->setFocus(TRUE);
	}
	setResizeLimits(getMinWidth(), EXPANDED_MIN_HEIGHT);
}

std::string LLFloaterIMNearbyChat::getCurrentChat()
{
	return mInputEditor ? mInputEditor->getText() : LLStringUtil::null;
}

// virtual
BOOL LLFloaterIMNearbyChat::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;

	if( KEY_RETURN == key && mask == MASK_CONTROL)
	{
		// shout
		sendChat(CHAT_TYPE_SHOUT);
		handled = TRUE;
	}
	else if (KEY_RETURN == key && mask == MASK_SHIFT)
	{
		// whisper
		sendChat(CHAT_TYPE_WHISPER);
		handled = TRUE;
	}


	if((mask == MASK_ALT) && isTornOff())
	{
		LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
		if ((KEY_UP == key) || (KEY_LEFT == key))
		{
			floater_container->selectNextorPreviousConversation(false);
			handled = TRUE;
		}
		if ((KEY_DOWN == key ) || (KEY_RIGHT == key))
		{
			floater_container->selectNextorPreviousConversation(true);
			handled = TRUE;
		}
	}

	return handled;
}

BOOL LLFloaterIMNearbyChat::matchChatTypeTrigger(const std::string& in_str, std::string* out_str)
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

void LLFloaterIMNearbyChat::onChatBoxKeystroke()
{
	LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
	if (im_box)
	{
		im_box->flashConversationItemWidget(mSessionID,false);
	}

	LLFirstUse::otherAvatarChatFirst(false);

	LLWString raw_text = mInputEditor->getWText();

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
		mInputEditor->setText( new_text );
		mInputEditor->setCursorToEnd();
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
			if (!rest_of_match.empty())
			{
				mInputEditor->setText(utf8_trigger + rest_of_match); // keep original capitalization for user-entered part
				// Select to end of line, starting from the character
				// after the last one the user typed.
				mInputEditor->selectByCursorPosition(utf8_out_str.size()-rest_of_match.size(),utf8_out_str.size());
			}

		}
		else if (matchChatTypeTrigger(utf8_trigger, &utf8_out_str))
		{
			std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
			mInputEditor->setText(utf8_trigger + rest_of_match + " "); // keep original capitalization for user-entered part
			mInputEditor->endOfDoc();
		}

		//llinfos << "GESTUREDEBUG " << trigger 
		//	<< " len " << length
		//	<< " outlen " << out_str.getLength()
		//	<< llendl;
	}
}

// static
void LLFloaterIMNearbyChat::onChatBoxFocusLost()
{
	// stop typing animation
	gAgent.stopTyping();
}

void LLFloaterIMNearbyChat::onChatBoxFocusReceived()
{
	mInputEditor->setEnabled(!gDisconnected);
}

EChatType LLFloaterIMNearbyChat::processChatTypeTriggers(EChatType type, std::string &str)
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

void LLFloaterIMNearbyChat::sendChat( EChatType type )
{
	if (mInputEditor)
	{
		LLWString text = mInputEditor->getWText();
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
				if(!LLGestureMgr::instance().triggerAndReviseString(utf8text, &utf8_revised_text))
				{
					utf8_revised_text = utf8text;
				}
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
				sendChatFromViewer(utf8_revised_text, type, gSavedSettings.getBOOL("PlayChatAnim"));
			}
		}

		mInputEditor->setText(LLStringExplicit(""));
	}

	gAgent.stopTyping();

	// If the user wants to stop chatting on hitting return, lose focus
	// and go out of chat mode.
	if (gSavedSettings.getBOOL("CloseChatOnReturn"))
	{
		stopChat();
	}
}

void LLFloaterIMNearbyChat::addMessage(const LLChat& chat,bool archive,const LLSD &args)
{
	appendMessage(chat, args);

	if(archive)
	{
		mMessageArchive.push_back(chat);
		if(mMessageArchive.size() > 200)
		{
			mMessageArchive.erase(mMessageArchive.begin());
		}
	}

	// logging
	if (!args["do_not_log"].asBoolean() && gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 1)
	{
		std::string from_name = chat.mFromName;

		if (chat.mSourceType == CHAT_SOURCE_AGENT)
		{
			// if the chat is coming from an agent, log the complete name
			LLAvatarName av_name;
			LLAvatarNameCache::get(chat.mFromID, &av_name);

			if (!av_name.isDisplayNameDefault())
			{
				from_name = av_name.getCompleteName();
			}
		}

		LLLogChat::saveHistory("chat", from_name, chat.mFromID, chat.mText);
	}
}


void LLFloaterIMNearbyChat::onChatBoxCommit()
{
	sendChat(CHAT_TYPE_NORMAL);

	gAgent.stopTyping();
}

void LLFloaterIMNearbyChat::displaySpeakingIndicator()
{
	LLSpeakerMgr::speaker_list_t speaker_list;
	LLUUID id;

	id.setNull();
	mSpeakerMgr->update(FALSE);
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
}

void LLFloaterIMNearbyChat::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLFloaterIMNearbyChat::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
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
bool LLFloaterIMNearbyChat::isWordsName(const std::string& name)
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

// static 
void LLFloaterIMNearbyChat::startChat(const char* line)
{
	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat)
	{
		if(!nearby_chat->isTornOff())
		{
			LLFloaterIMContainer::getInstance()->selectConversation(LLUUID(NULL));
		}
		if(nearby_chat->isMinimized())
		{
			nearby_chat->setMinimized(false);
		}
		nearby_chat->show();
		nearby_chat->setFocus(TRUE);

		if (line)
		{
			std::string line_string(line);
			nearby_chat->mInputEditor->setText(line_string);
		}

		nearby_chat->mInputEditor->endOfDoc();
	}
}

// Exit "chat mode" and do the appropriate focus changes
// static
void LLFloaterIMNearbyChat::stopChat()
{
	LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
	if (nearby_chat)
	{
		nearby_chat->mInputEditor->setFocus(FALSE);
	    gAgent.stopTyping();
	}
}

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLFloaterIMNearbyChat::stripChannelNumber(const LLWString &mesg, S32* channel)
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
