/** 
 * @file llnearbychatbar.cpp
 * @brief LLNearbyChatBar class implementation
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

#include "llappviewer.h"
#include "llfloaterreg.h"
#include "lltrans.h"

#include "llfirstuse.h"
#include "llnearbychatbar.h"
#include "llnearbychatbarlistener.h"
#include "llagent.h"
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
#include "llnearbychat.h"
#include "lltranslate.h"

#include "llresizehandle.h"
#include "llautoreplace.h"

S32 LLNearbyChatBar::sLastSpecialChatChannel = 0;

const S32 EXPANDED_HEIGHT = 300;
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

LLNearbyChatBar::LLNearbyChatBar(const LLSD& key)
:	LLFloater(key),
	mChatBox(NULL),
	mNearbyChat(NULL),
	mOutputMonitor(NULL),
	mSpeakerMgr(NULL),
	mExpandedHeight(COLLAPSED_HEIGHT + EXPANDED_HEIGHT)
{
	mSpeakerMgr = LLLocalSpeakerMgr::getInstance();
	mListener.reset(new LLNearbyChatBarListener(*this));
}

//virtual
BOOL LLNearbyChatBar::postBuild()
{
	mChatBox = getChild<LLLineEditor>("chat_box");

	mChatBox->setAutoreplaceCallback(boost::bind(&LLAutoReplace::autoreplaceCallback, LLAutoReplace::getInstance(), _1, _2));
	mChatBox->setCommitCallback(boost::bind(&LLNearbyChatBar::onChatBoxCommit, this));
	mChatBox->setKeystrokeCallback(&onChatBoxKeystroke, this);
	mChatBox->setFocusLostCallback(boost::bind(&onChatBoxFocusLost, _1, this));
	mChatBox->setFocusReceivedCallback(boost::bind(&LLNearbyChatBar::onChatBoxFocusReceived, this));

	mChatBox->setIgnoreArrowKeys( FALSE ); 
	mChatBox->setCommitOnFocusLost( FALSE );
	mChatBox->setRevertOnEsc( FALSE );
	mChatBox->setIgnoreTab(TRUE);
	mChatBox->setPassDelete(TRUE);
	mChatBox->setReplaceNewlinesWithSpaces(FALSE);
	mChatBox->setEnableLineHistory(TRUE);
	mChatBox->setFont(LLViewerChat::getChatFont());

	mNearbyChat = getChildView("nearby_chat");

	gSavedSettings.declareBOOL("nearbychat_history_visibility", mNearbyChat->getVisible(), "Visibility state of nearby chat history", TRUE);
	BOOL show_nearby_chat = gSavedSettings.getBOOL("nearbychat_history_visibility");

	LLButton* show_btn = getChild<LLButton>("show_nearby_chat");
	show_btn->setCommitCallback(boost::bind(&LLNearbyChatBar::onToggleNearbyChatPanel, this));
	show_btn->setToggleState(show_nearby_chat);

	mOutputMonitor = getChild<LLOutputMonitorCtrl>("chat_zone_indicator");
	mOutputMonitor->setVisible(FALSE);

	showNearbyChatPanel(show_nearby_chat);

	// Register for font change notifications
	LLViewerChat::setFontChangedCallback(boost::bind(&LLNearbyChatBar::onChatFontChange, this, _1));

	enableResizeCtrls(true, true, false);

	return TRUE;
}

// virtual
void LLNearbyChatBar::onOpen(const LLSD& key)
{
	showTranslationCheckbox(LLTranslate::isTranslationConfigured());
}

bool LLNearbyChatBar::applyRectControl()
{
	bool rect_controlled = LLFloater::applyRectControl();

	if (!mNearbyChat->getVisible())
	{
		reshape(getRect().getWidth(), getMinHeight());
		enableResizeCtrls(true, true, false);
	}
	else
	{
		enableResizeCtrls(true);
		setResizeLimits(getMinWidth(), EXPANDED_MIN_HEIGHT);
	}
	
	return rect_controlled;
}

void LLNearbyChatBar::onChatFontChange(LLFontGL* fontp)
{
	// Update things with the new font whohoo
	if (mChatBox)
	{
		mChatBox->setFont(fontp);
	}
}

//static
LLNearbyChatBar* LLNearbyChatBar::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLNearbyChatBar>("chat_bar");
}

void LLNearbyChatBar::showHistory()
{
	openFloater();

	if (!getChildView("nearby_chat")->getVisible())
	{
		onToggleNearbyChatPanel();
	}
}

void LLNearbyChatBar::showTranslationCheckbox(BOOL show)
{
	getChild<LLUICtrl>("translate_chat_checkbox_lp")->setVisible(show);
}

void LLNearbyChatBar::draw()
{
	displaySpeakingIndicator();
	LLFloater::draw();
}

std::string LLNearbyChatBar::getCurrentChat()
{
	return mChatBox ? mChatBox->getText() : LLStringUtil::null;
}

// virtual
BOOL LLNearbyChatBar::handleKeyHere( KEY key, MASK mask )
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

BOOL LLNearbyChatBar::matchChatTypeTrigger(const std::string& in_str, std::string* out_str)
{
	U32 in_len = in_str.length();
	S32 cnt = sizeof(sChatTypeTriggers) / sizeof(*sChatTypeTriggers);
	
	for (S32 n = 0; n < cnt; n++)
	{
		if (in_len > sChatTypeTriggers[n].name.length())
			continue;

		std::string trigger_trunc = sChatTypeTriggers[n].name;
		LLStringUtil::truncate(trigger_trunc, in_len);

		if (!LLStringUtil::compareInsensitive(in_str, trigger_trunc))
		{
			*out_str = sChatTypeTriggers[n].name;
			return TRUE;
		}
	}

	return FALSE;
}

void LLNearbyChatBar::onChatBoxKeystroke(LLLineEditor* caller, void* userdata)
{
	LLFirstUse::otherAvatarChatFirst(false);

	LLNearbyChatBar* self = (LLNearbyChatBar *)userdata;

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
			S32 outlength = self->mChatBox->getLength(); // in characters

			// Select to end of line, starting from the character
			// after the last one the user typed.
			self->mChatBox->setSelection(length, outlength);
		}
		else if (matchChatTypeTrigger(utf8_trigger, &utf8_out_str))
		{
			std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
			self->mChatBox->setText(utf8_trigger + rest_of_match + " "); // keep original capitalization for user-entered part
			self->mChatBox->setCursorToEnd();
		}

		//llinfos << "GESTUREDEBUG " << trigger 
		//	<< " len " << length
		//	<< " outlen " << out_str.getLength()
		//	<< llendl;
	}
}

// static
void LLNearbyChatBar::onChatBoxFocusLost(LLFocusableElement* caller, void* userdata)
{
	// stop typing animation
	gAgent.stopTyping();
}

void LLNearbyChatBar::onChatBoxFocusReceived()
{
	mChatBox->setEnabled(!gDisconnected);
}

EChatType LLNearbyChatBar::processChatTypeTriggers(EChatType type, std::string &str)
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

void LLNearbyChatBar::sendChat( EChatType type )
{
	if (mChatBox)
	{
		LLWString text = mChatBox->getConvertedText();
		if (!text.empty())
		{
			// store sent line in history, duplicates will get filtered
			mChatBox->updateHistory();
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

void LLNearbyChatBar::showNearbyChatPanel(bool show)
{
	if (!show)
	{
		if (mNearbyChat->getVisible() && !isMinimized())
		{
			mExpandedHeight = getRect().getHeight();
		}
		setResizeLimits(getMinWidth(), COLLAPSED_HEIGHT);
		mNearbyChat->setVisible(FALSE);
		reshape(getRect().getWidth(), COLLAPSED_HEIGHT);
		enableResizeCtrls(true, true, false);
		storeRectControl();
	}
	else
	{
		mNearbyChat->setVisible(TRUE);
		setResizeLimits(getMinWidth(), EXPANDED_MIN_HEIGHT);
		reshape(getRect().getWidth(), mExpandedHeight);
		enableResizeCtrls(true);
		storeRectControl();
	}

	gSavedSettings.setBOOL("nearbychat_history_visibility", mNearbyChat->getVisible());
}

void LLNearbyChatBar::onToggleNearbyChatPanel()
{
	showNearbyChatPanel(!mNearbyChat->getVisible());
}

void LLNearbyChatBar::setMinimized(BOOL b)
{
	LLNearbyChat* nearby_chat = getChild<LLNearbyChat>("nearby_chat");
	// when unminimizing with nearby chat visible, go ahead and kill off screen chats
	if (!b && nearby_chat->getVisible())
	{
		nearby_chat->removeScreenChat();
	}
		LLFloater::setMinimized(b);
}

void LLNearbyChatBar::onChatBoxCommit()
{
	if (mChatBox->getText().length() > 0)
	{
		sendChat(CHAT_TYPE_NORMAL);
	}

	gAgent.stopTyping();
}

void LLNearbyChatBar::displaySpeakingIndicator()
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
		mOutputMonitor->setVisible(TRUE);
		mOutputMonitor->setSpeakerId(id);
	}
	else
	{
		mOutputMonitor->setVisible(FALSE);
	}
}

void LLNearbyChatBar::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLNearbyChatBar::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
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
void LLNearbyChatBar::startChat(const char* line)
{
	LLNearbyChatBar* cb = LLNearbyChatBar::getInstance();

	if (!cb )
		return;

	cb->setVisible(TRUE);
	cb->setFocus(TRUE);
	cb->mChatBox->setFocus(TRUE);

	if (line)
	{
		std::string line_string(line);
		cb->mChatBox->setText(line_string);
	}

	cb->mChatBox->setCursorToEnd();
}

// Exit "chat mode" and do the appropriate focus changes
// static
void LLNearbyChatBar::stopChat()
{
	LLNearbyChatBar* cb = LLNearbyChatBar::getInstance();

	if (!cb)
		return;

	cb->mChatBox->setFocus(FALSE);

 	// stop typing animation
 	gAgent.stopTyping();
}

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLNearbyChatBar::stripChannelNumber(const LLWString &mesg, S32* channel)
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


