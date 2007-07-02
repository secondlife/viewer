/** 
 * @file llfloaterchat.cpp
 * @brief LLFloaterChat class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/**
 * Actually the "Chat History" floater.
 * Should be llfloaterchathistory, not llfloaterchat.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterchat.h"
#include "llfloaterscriptdebug.h"

#include "llchat.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"

// project include
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llconsole.h"
#include "llfloatermute.h"
#include "llkeyboard.h"
//#include "lllineeditor.h"
#include "llmutelist.h"
//#include "llresizehandle.h"
#include "llstatusbar.h"
#include "llviewertexteditor.h"
#include "llviewergesture.h"			// for triggering gestures
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llvieweruictrlfactory.h"
#include "llchatbar.h"
#include "lllogchat.h"
#include "lltexteditor.h"
#include "llfloaterhtml.h"
#include "llweb.h"

//
// Constants
//
const char FLOATER_TITLE[] = "Chat History";
const F32 INSTANT_MSG_SIZE = 8.0f;
const F32 CHAT_MSG_SIZE = 8.0f;
const LLColor4 INSTANT_MSG_COLOR(1, 1, 1, 1);
const LLColor4 MUTED_MSG_COLOR(0.5f, 0.5f, 0.5f, 1.f);
const S32 MAX_CHATTER_COUNT = 16;

//
// Global statics
//
LLFloaterChat* gFloaterChat = NULL;

LLColor4 get_text_color(const LLChat& chat);

//
// Member Functions
//
LLFloaterChat::LLFloaterChat()
:	LLFloater("chat floater", "FloaterChatRect", FLOATER_TITLE, 
			  RESIZE_YES, 440, 100, DRAG_ON_TOP, MINIMIZE_NO, CLOSE_YES)
{
	
	gUICtrlFactory->buildFloater(this,"floater_chat_history.xml");

	childSetAction("Mute resident",onClickMute,this);
	childSetAction("Chat", onClickChat, this);
	childSetCommitCallback("chatter combobox",onCommitUserSelect,this);
	childSetCommitCallback("show mutes",onClickToggleShowMute,this); //show mutes
	childSetVisible("Chat History Editor with mute",FALSE);
	setDefaultBtn("Chat");
}

LLFloaterChat::~LLFloaterChat()
{
	// Children all cleaned up by default view destructor.
}

void LLFloaterChat::setVisible(BOOL visible)
{
	LLFloater::setVisible( visible );

	gSavedSettings.setBOOL("ShowChatHistory", visible);

	// Hide the chat overlay when our history is visible.
	gConsole->setVisible( !visible );
}


// public virtual
void LLFloaterChat::onClose(bool app_quitting)
{
	LLFloater::setVisible( FALSE );

	if (!app_quitting)
	{
		gSavedSettings.setBOOL("ShowChatHistory", FALSE);
	}

	// Hide the chat overlay when our history is visible.
	gConsole->setVisible( TRUE );
}


// public
void LLFloaterChat::show()
{
	open();		/*Flawfinder: ignore*/
}

void add_timestamped_line(LLViewerTextEditor* edit, const LLString& line, const LLColor4& color)
{
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("ChatShowTimestamps"))
	{
		edit->appendTime(prepend_newline);
		prepend_newline = false;
	}
	edit->appendColoredText(line, false, prepend_newline, color);
}

void log_chat_text(const LLChat& chat)
{
		LLString histstr;
		if (gSavedPerAccountSettings.getBOOL("LogChatTimestamp"))
			histstr = LLLogChat::timestamp(gSavedPerAccountSettings.getBOOL("LogTimestampDate")) + chat.mText;
		else
			histstr = chat.mText;

		LLLogChat::saveHistory("chat",histstr);
}
// static
void LLFloaterChat::addChatHistory(const LLChat& chat, bool log_to_file)
{
	if ( gSavedPerAccountSettings.getBOOL("LogChat") && log_to_file) 
	{
		log_chat_text(chat);
	}
	
	LLColor4 color = get_text_color(chat);
	
	if (!log_to_file) color = LLColor4::grey;	//Recap from log file.

	if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
	{
		LLFloaterScriptDebug::addScriptLine(chat.mText,
											chat.mFromName, 
											color, 
											chat.mFromID);
		if (!gSavedSettings.getBOOL("ScriptErrorsAsChat"))
		{
			return;
		}
	}
	
	// could flash the chat button in the status bar here. JC
	if (!gFloaterChat) return;

	LLViewerTextEditor*	history_editor = (LLViewerTextEditor*)gFloaterChat->getChildByName("Chat History Editor");
	LLViewerTextEditor*	history_editor_with_mute = (LLViewerTextEditor*)gFloaterChat->getChildByName("Chat History Editor with mute");

	history_editor->setParseHTML(TRUE);
	history_editor_with_mute->setParseHTML(TRUE);
	
	if (!chat.mMuted)
	{
		add_timestamped_line(history_editor, chat.mText, color);
		add_timestamped_line(history_editor_with_mute, chat.mText, color);
	}
	else
	{
		// desaturate muted chat
		LLColor4 muted_color = lerp(color, LLColor4::grey, 0.5f);
		add_timestamped_line(history_editor_with_mute, chat.mText, color);
	}

	if (!chat.mMuted
		&& chat.mSourceType != CHAT_SOURCE_SYSTEM
		&& chat.mFromID.notNull()
		&& chat.mFromID != gAgent.getID())
	{
			
		LLComboBox*	chatter_combo = LLUICtrlFactory::getComboBoxByName(gFloaterChat,"chatter combobox");
		
		if(!chatter_combo)
		{
			return;
		}

		if (!chatter_combo->setCurrentByID(chat.mFromID))
		{
			// if we have too many items...
			if (chatter_combo->getItemCount() >= MAX_CHATTER_COUNT)
			{
				chatter_combo->remove(0);
			}
			
			LLMute mute(chat.mFromID, chat.mFromName);
			if (chat.mSourceType == CHAT_SOURCE_OBJECT)
			{
				mute.mType = LLMute::OBJECT;
			}
			else if (chat.mSourceType == CHAT_SOURCE_AGENT)
			{
				mute.mType = LLMute::AGENT;
			}
			LLString item = mute.getDisplayName();
			chatter_combo->add(item, chat.mFromID);
			chatter_combo->setCurrentByIndex(chatter_combo->getItemCount() - 1);
			gFloaterChat->childSetEnabled("Mute resident",TRUE);
		}
	}
}

// static
void LLFloaterChat::setHistoryCursorAndScrollToEnd()
{
	if (gFloaterChat)
	{
		LLViewerTextEditor*	history_editor = (LLViewerTextEditor*)gFloaterChat->getChildByName("Chat History Editor");
		LLViewerTextEditor*	history_editor_with_mute = (LLViewerTextEditor*)gFloaterChat->getChildByName("Chat History Editor with mute");
		
		history_editor->setCursorAndScrollToEnd();
		history_editor_with_mute->setCursorAndScrollToEnd();
	}
}


// static
void LLFloaterChat::toggle(void*)
{
	if (gFloaterChat->getVisible())
	{
		gFloaterChat->close();
	}
	else
	{
		gFloaterChat->show();
	}
}

// static
BOOL LLFloaterChat::visible(void*)
{
	return (gFloaterChat && gFloaterChat->getVisible());
}

//static 
void LLFloaterChat::onClickMute(void *data)
{
	LLFloaterChat* self = (LLFloaterChat*)data;

	LLComboBox*	chatter_combo = LLUICtrlFactory::getComboBoxByName(self,"chatter combobox");

	const LLString& name = chatter_combo->getSimple();
	LLUUID id = chatter_combo->getCurrentID();

	if (name.empty()) return;

	LLMute mute(id);
	mute.setFromDisplayName(name);
	gMuteListp->add(mute);
	
	if (gFloaterMute)
	{
		gFloaterMute->show();
	}
}

//static
void LLFloaterChat::onClickChat(void*)
{
	// we need this function as a level of indirection because otherwise startChat would
	// cast the data pointer to a character string, and dump garbage in the chat
	LLChatBar::startChat(NULL);
}

//static 
void LLFloaterChat::onCommitUserSelect(LLUICtrl* caller, void* data)
{
	LLFloaterChat* floater = (LLFloaterChat*)data;
	LLComboBox* combo = (LLComboBox*)caller;

	if (combo->getCurrentIndex() == -1)
	{
		floater->childSetEnabled("Mute resident",FALSE);
	}
	else
	{
		floater->childSetEnabled("Mute resident",TRUE);
	}
}

//static
void LLFloaterChat::onClickToggleShowMute(LLUICtrl* caller, void *data)
{
	LLFloaterChat* floater = (LLFloaterChat*)data;


	//LLCheckBoxCtrl*	
	BOOL show_mute = LLUICtrlFactory::getCheckBoxByName(floater,"show mutes")->get();
	LLViewerTextEditor*	history_editor = (LLViewerTextEditor*)floater->getChildByName("Chat History Editor");
	LLViewerTextEditor*	history_editor_with_mute = (LLViewerTextEditor*)floater->getChildByName("Chat History Editor with mute");

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
void LLFloaterChat::addChat(const LLChat& chat, 
			  BOOL from_instant_message, 
			  BOOL local_agent)
{
	LLColor4 text_color = get_text_color(chat);

	BOOL invisible_script_debug_chat = 
			chat.mChatType == CHAT_TYPE_DEBUG_MSG
			&& !gSavedSettings.getBOOL("ScriptErrorsAsChat");

	if (!invisible_script_debug_chat 
		&& !chat.mMuted 
		&& gConsole 
		&& !local_agent)
	{
		F32 size = CHAT_MSG_SIZE;
		if(from_instant_message)
		{
			text_color = INSTANT_MSG_COLOR;
			size = INSTANT_MSG_SIZE;
		}
		gConsole->addLine(chat.mText, size, text_color);
	}

	if(from_instant_message && gSavedPerAccountSettings.getBOOL("LogChatIM"))
		log_chat_text(chat);
	
	if(from_instant_message && gSavedSettings.getBOOL("IMInChatHistory"))
		addChatHistory(chat,false);
	
	if(!from_instant_message)
		addChatHistory(chat);
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
			text_color = gSavedSettings.getColor4("SystemChatColor");
			break;
		case CHAT_SOURCE_AGENT:
		    if (chat.mFromID.isNull())
			{
				text_color = gSavedSettings.getColor4("SystemChatColor");
			}
			else
			{
				text_color = gSavedSettings.getColor4("AgentChatColor");
			}
			break;
		case CHAT_SOURCE_OBJECT:
			if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
			{
				text_color = gSavedSettings.getColor4("ScriptErrorColor");
			}
			else if ( chat.mChatType == CHAT_TYPE_OWNER )
			{
				text_color = gSavedSettings.getColor4("llOwnerSayChatColor");
			}
			else
			{
				text_color = gSavedSettings.getColor4("ObjectChatColor");
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
	LLLogChat::loadHistory("chat", &chatFromLogFile, (void *)gFloaterChat); 
}

//static
void LLFloaterChat::chatFromLogFile(LLString line, void* userdata)
{
	LLChat chat;
				
	chat.mText = line;
	addChatHistory(chat,  FALSE);
}
