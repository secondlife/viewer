/** 
 * @file llfloaterchat.cpp
 * @brief LLFloaterChat class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

/**
 * Actually the "Chat History" floater.
 * Should be llfloaterchathistory, not llfloaterchat.
 */

#include "llviewerprecompiledheaders.h"

// project include
#include "llagent.h"
#include "llappviewer.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llconsole.h"
#include "llfloateractivespeakers.h"
#include "llfloaterchatterbox.h"
#include "llfloaterreg.h"
#include "llfloaterscriptdebug.h"
#include "llkeyboard.h"
//#include "lllineeditor.h"
#include "llmutelist.h"
//#include "llresizehandle.h"
#include "llchatbar.h"
#include "llrecentpeople.h"
#include "llpanelblockedlist.h"
#include "llslurl.h"
#include "llstatusbar.h"
#include "llviewertexteditor.h"
#include "llviewergesture.h"			// for triggering gestures
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "lllogchat.h"
#include "lltexteditor.h"
#include "lltextparser.h"
#include "llweb.h"
#include "llstylemap.h"

// linden library includes
#include "llaudioengine.h"
#include "llchat.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "llwindow.h"
#include "message.h"

//
// Constants
//
const F32 INSTANT_MSG_SIZE = 8.0f;
const F32 CHAT_MSG_SIZE = 8.0f;


//
// Global statics
//
LLColor4 get_text_color(const LLChat& chat);

//
// Member Functions
//
LLFloaterChat::LLFloaterChat(const LLSD& seed)
	: LLFloater(seed),
	  mPanel(NULL)
{
	mFactoryMap["chat_panel"] = LLCallbackMap(createChatPanel, NULL);
	mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, NULL);
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this,"floater_chat_history.xml");

}

LLFloaterChat::~LLFloaterChat()
{
	// Children all cleaned up by default view destructor.
}

void LLFloaterChat::draw()
{
	// enable say and shout only when text available
		
	childSetValue("toggle_active_speakers_btn", childIsVisible("active_speakers_panel"));

	LLChatBar* chat_barp = findChild<LLChatBar>("chat_panel", TRUE);
	if (chat_barp)
	{
		chat_barp->refresh();
	}

	mPanel->refreshSpeakers();
	LLFloater::draw();
}

BOOL LLFloaterChat::postBuild()
{
	// Hide the chat overlay when our history is visible.
	setVisibleCallback(boost::bind(&LLFloaterChat::updateConsoleVisibility, this));
	
	mPanel = (LLPanelActiveSpeakers*)getChild<LLPanel>("active_speakers_panel");

	childSetCommitCallback("show mutes",onClickToggleShowMute,this); //show mutes
	childSetVisible("Chat History Editor with mute",FALSE);
	childSetAction("toggle_active_speakers_btn", onClickToggleActiveSpeakers, this);

	return TRUE;
}

void LLFloaterChat::updateConsoleVisibility()
{
	if(gDisconnected)
	{
		return;
	}
	// determine whether we should show console due to not being visible
	gConsole->setVisible( !isInVisibleChain()								// are we not in part of UI being drawn?
							|| isMinimized()								// are we minimized?
							|| (getHost() && getHost()->isMinimized() ));	// are we hosted in a minimized floater?
}

void add_timestamped_line(LLViewerTextEditor* edit, LLChat chat, const LLColor4& color)
{
	std::string line = chat.mText;
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("ChatShowTimestamps"))
	{
		edit->appendTime(prepend_newline);
		prepend_newline = false;
	}

	// If the msg is from an agent (not yourself though),
	// extract out the sender name and replace it with the hotlinked name.
	if (chat.mSourceType == CHAT_SOURCE_AGENT &&
		chat.mFromID != LLUUID::null)
	{
		chat.mURL = LLSLURL::buildCommand("agent", chat.mFromID, "inspect");
	}

	// If the chat line has an associated url, link it up to the name.
	if (!chat.mURL.empty()
		&& (line.length() > chat.mFromName.length() && line.find(chat.mFromName,0) == 0))
	{
		std::string start_line = line.substr(0, chat.mFromName.length() + 1);
		line = line.substr(chat.mFromName.length() + 1);
		edit->appendText(start_line, prepend_newline, LLStyleMap::instance().lookup(chat.mFromID,chat.mURL));
		edit->blockUndo();
		prepend_newline = false;
	}
	edit->appendText(line, prepend_newline, LLStyle::Params().color(color));
	edit->blockUndo();
}

// static
void LLFloaterChat::addChatHistory(const LLChat& chat, bool log_to_file)
{	
	if (log_to_file && (gSavedPerAccountSettings.getBOOL("LogChat"))) 
	{
		if (chat.mChatType != CHAT_TYPE_WHISPER && chat.mChatType != CHAT_TYPE_SHOUT)
		{
			LLLogChat::saveHistory("chat", chat.mFromName, chat.mFromID, chat.mText);
		}
		else
		{
			LLLogChat::saveHistory("chat", "", chat.mFromID, chat.mFromName + " " + chat.mText);
		}
	}
	
	LLColor4 color = get_text_color(chat);
	
	if (!log_to_file) color = LLColor4::grey;	//Recap from log file.

	if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
	{
		if(gSavedSettings.getBOOL("ShowScriptErrors") == FALSE)
			return;
		if (gSavedSettings.getS32("ShowScriptErrorsLocation") == 1)
		{
			LLFloaterScriptDebug::addScriptLine(chat.mText,
												chat.mFromName, 
												color, 
												chat.mFromID);
			return;
		}
	}
	
	// could flash the chat button in the status bar here. JC
	LLFloaterChat* chat_floater = LLFloaterChat::getInstance();
	LLViewerTextEditor*	history_editor = chat_floater->getChild<LLViewerTextEditor>("Chat History Editor");
	LLViewerTextEditor*	history_editor_with_mute = chat_floater->getChild<LLViewerTextEditor>("Chat History Editor with mute");

	if (!chat.mMuted)
	{
		add_timestamped_line(history_editor, chat, color);
		add_timestamped_line(history_editor_with_mute, chat, color);
	}
	else
	{
		// desaturate muted chat
		LLColor4 muted_color = lerp(color, LLColor4::grey, 0.5f);
		add_timestamped_line(history_editor_with_mute, chat, color);
	}
	
	// add objects as transient speakers that can be muted
	if (chat.mSourceType == CHAT_SOURCE_OBJECT)
	{
		chat_floater->mPanel->setSpeaker(chat.mFromID, chat.mFromName, LLSpeaker::STATUS_NOT_IN_CHANNEL, LLSpeaker::SPEAKER_OBJECT);
	}

	// start tab flashing on incoming text from other users (ignoring system text, etc)
	if (!chat_floater->isInVisibleChain() && chat.mSourceType == CHAT_SOURCE_AGENT)
	{
		LLFloaterChatterBox::getInstance()->setFloaterFlashing(chat_floater, TRUE);
	}
}

// static
void LLFloaterChat::setHistoryCursorAndScrollToEnd()
{
	LLViewerTextEditor*	history_editor = LLFloaterChat::getInstance()->getChild<LLViewerTextEditor>("Chat History Editor");
	LLViewerTextEditor*	history_editor_with_mute = LLFloaterChat::getInstance()->getChild<LLViewerTextEditor>("Chat History Editor with mute");
	
	if (history_editor) 
	{
		history_editor->setCursorAndScrollToEnd();
	}
	if (history_editor_with_mute)
	{
		 history_editor_with_mute->setCursorAndScrollToEnd();
	}
}


//static 
void LLFloaterChat::onClickMute(void *data)
{
	LLFloaterChat* self = (LLFloaterChat*)data;

	LLComboBox*	chatter_combo = self->getChild<LLComboBox>("chatter combobox");

	const std::string& name = chatter_combo->getSimple();
	LLUUID id = chatter_combo->getCurrentID();

	if (name.empty()) return;

	LLMute mute(id);
	mute.setFromDisplayName(name);
	LLMuteList::getInstance()->add(mute);
	LLPanelBlockedList::showPanelAndSelect(mute.mID);
}

//static
void LLFloaterChat::onClickToggleShowMute(LLUICtrl* caller, void *data)
{
	LLFloaterChat* floater = (LLFloaterChat*)data;


	//LLCheckBoxCtrl*	
	BOOL show_mute = floater->getChild<LLCheckBoxCtrl>("show mutes")->get();
	LLViewerTextEditor*	history_editor = floater->getChild<LLViewerTextEditor>("Chat History Editor");
	LLViewerTextEditor*	history_editor_with_mute = floater->getChild<LLViewerTextEditor>("Chat History Editor with mute");

	if (!history_editor || !history_editor_with_mute)
		return;

	//BOOL show_mute = floater->mShowMuteCheckBox->get();
	if (show_mute)
	{
		history_editor->setVisible(FALSE);
		history_editor_with_mute->setVisible(TRUE);
		history_editor_with_mute->setCursorAndScrollToEnd();
	}
	else
	{
		history_editor->setVisible(TRUE);
		history_editor_with_mute->setVisible(FALSE);
		history_editor->setCursorAndScrollToEnd();
	}
}

// Put a line of chat in all the right places
void LLFloaterChat::addChat(const LLChat& chat, BOOL local_agent)
{
	triggerAlerts(chat.mText);

	// Add the sender to the list of people with which we've recently interacted.
	// this is not the best place to add _all_ messages to recent list
	// comment this for now, may remove later on code cleanup
	//if(chat.mSourceType == CHAT_SOURCE_AGENT && chat.mFromID.notNull())
	//	LLRecentPeople::instance().add(chat.mFromID);
	
	addChatHistory(chat, true);
}

// Moved from lltextparser.cpp to break llui/llaudio library dependency.
//static
void LLFloaterChat::triggerAlerts(const std::string& text)
{
	LLTextParser* parser = LLTextParser::getInstance();
//    bool spoken=FALSE;
	for (S32 i=0;i<parser->mHighlights.size();i++)
	{
		LLSD& highlight = parser->mHighlights[i];
		if (parser->findPattern(text,highlight) >= 0 )
		{
			if(gAudiop)
			{
				if ((std::string)highlight["sound_lluuid"] != LLUUID::null.asString())
				{
					gAudiop->triggerSound(highlight["sound_lluuid"].asUUID(), 
						gAgent.getID(),
						1.f,
						LLAudioEngine::AUDIO_TYPE_UI,
						gAgent.getPositionGlobal() );
				}
/*				
				if (!spoken) 
				{
					LLTextToSpeech* text_to_speech = NULL;
					text_to_speech = LLTextToSpeech::getInstance();
					spoken = text_to_speech->speak((LLString)highlight["voice"],text); 
				}
 */
			}
			if (highlight["flash"])
			{
				LLWindow* viewer_window = gViewerWindow->getWindow();
				if (viewer_window && viewer_window->getMinimized())
				{
					viewer_window->flashIcon(5.f);
				}
			}
		}
	}
}

LLColor4 get_text_color(const LLChat& chat)
{
	LLColor4 text_color;

	if(chat.mMuted)
	{
		text_color.setVec(0.8f, 0.8f, 0.8f, 1.f);
	}
	else
	{
		switch(chat.mSourceType)
		{
		case CHAT_SOURCE_SYSTEM:
			text_color = LLUIColorTable::instance().getColor("SystemChatColor");
			break;
		case CHAT_SOURCE_AGENT:
		    if (chat.mFromID.isNull())
			{
				text_color = LLUIColorTable::instance().getColor("SystemChatColor");
			}
			else
			{
				if(gAgent.getID() == chat.mFromID)
				{
					text_color = LLUIColorTable::instance().getColor("UserChatColor");
				}
				else
				{
					text_color = LLUIColorTable::instance().getColor("AgentChatColor");
				}
			}
			break;
		case CHAT_SOURCE_OBJECT:
			if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
			{
				text_color = LLUIColorTable::instance().getColor("ScriptErrorColor");
			}
			else if ( chat.mChatType == CHAT_TYPE_OWNER )
			{
				text_color = LLUIColorTable::instance().getColor("llOwnerSayChatColor");
			}
			else
			{
				text_color = LLUIColorTable::instance().getColor("ObjectChatColor");
			}
			break;
		default:
			text_color.setToWhite();
		}

		if (!chat.mPosAgent.isExactlyZero())
		{
			LLVector3 pos_agent = gAgent.getPositionAgent();
			F32 distance = dist_vec(pos_agent, chat.mPosAgent);
			if (distance > gAgent.getNearChatRadius())
			{
				// diminish far-off chat
				text_color.mV[VALPHA] = 0.8f;
			}
		}
	}

	return text_color;
}

//static
void LLFloaterChat::loadHistory()
{
	LLLogChat::loadHistory(std::string("chat"), &chatFromLogFile, (void *)LLFloaterChat::getInstance()); 
}

//static
void LLFloaterChat::chatFromLogFile(LLLogChat::ELogLineType type , const LLSD& line, void* userdata)
{
	switch (type)
	{
	case LLLogChat::LOG_EMPTY:
	case LLLogChat::LOG_END:
		// *TODO: nice message from XML file here
		break;
	case LLLogChat::LOG_LINE:
	case LLLogChat::LOG_LLSD:
		{
			LLChat chat;					
			chat.mText = line["message"].asString();
			get_text_color(chat);
			addChatHistory(chat,  FALSE);
		}
		break;
	default:
		// nothing
		break;
	}
}

//static
void* LLFloaterChat::createSpeakersPanel(void* data)
{
	return new LLPanelActiveSpeakers(LLLocalSpeakerMgr::getInstance(), TRUE);
}

//static
void* LLFloaterChat::createChatPanel(void* data)
{
	LLChatBar* chatp = new LLChatBar();
	return chatp;
}

// static
void LLFloaterChat::onClickToggleActiveSpeakers(void* userdata)
{
	LLFloaterChat* self = (LLFloaterChat*)userdata;

	self->childSetVisible("active_speakers_panel", !self->childIsVisible("active_speakers_panel"));
}

//static
 LLFloaterChat* LLFloaterChat::getInstance()
 {
	 return LLFloaterReg::getTypedInstance<LLFloaterChat>("chat", LLSD()) ;
	 
 }
