/** 
* @file llbottomtray.cpp
* @brief LLBottomTray class implementation
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h" // must be first include
#include "llbottomtray.h"
#include "llagent.h"
#include "llchiclet.h"
#include "llfloaterreg.h"
#include "llflyoutbutton.h"
#include "llkeyboard.h"
#include "llgesturemgr.h"
#include "llanimationstates.h"
#include "llmultigesture.h"
#include "llviewerstats.h"
#include "llcommandhandler.h"

//FIXME: temporary, for stand up proto
#include "llselectmgr.h" 
#include "llvoavatarself.h"

S32 LLBottomTray::sLastSpecialChatChannel = 0;

// legacy calllback glue
void send_chat_from_viewer(const std::string& utf8_out_text, EChatType type, S32 channel);

static LLDefaultChildRegistry::Register<LLGestureComboBox> r("gesture_combo_box");

LLGestureComboBox::LLGestureComboBox(const LLComboBox::Params& p)
	: LLComboBox(p)
	, mGestureLabelTimer()
{
	setCommitCallback(boost::bind(&LLGestureComboBox::onCommitGesture, this, _1));

	// now register us as observer since we have a place to put the results
	gGestureManager.addObserver(this);

	// refresh list from current active gestures
	refreshGestures();
}

LLGestureComboBox::~LLGestureComboBox()
{
	gGestureManager.removeObserver(this);
}

void LLGestureComboBox::refreshGestures()
{
	//store current selection so we can maintain it
	std::string cur_gesture = getValue().asString();
	selectFirstItem();
	std::string label = getValue().asString();;
	// clear
	clearRows();

	// collect list of unique gestures
	std::map <std::string, BOOL> unique;
	LLGestureManager::item_map_t::iterator it;
	for (it = gGestureManager.mActive.begin(); it != gGestureManager.mActive.end(); ++it)
	{
		LLMultiGesture* gesture = (*it).second;
		if (gesture)
		{
			if (!gesture->mTrigger.empty())
			{
				unique[gesture->mTrigger] = TRUE;
			}
		}
	}

	// add unique gestures
	std::map <std::string, BOOL>::iterator it2;
	for (it2 = unique.begin(); it2 != unique.end(); ++it2)
	{
		addSimpleElement((*it2).first);
	}

	sortByName();
	// Insert label after sorting, at top, with separator below it
	addSeparator(ADD_TOP);		
	//FIXME: get it from xml
	addSimpleElement("Gestures", ADD_TOP);	

	if (!cur_gesture.empty())
	{ 
		selectByValue(LLSD(cur_gesture));
	}
	else
	{
		selectFirstItem();
	}
}

void LLGestureComboBox::onCommitGesture(LLUICtrl* ctrl)
{
	LLCtrlListInterface* gestures = getListInterface();
	if (gestures)
	{
		S32 index = gestures->getFirstSelectedIndex();
		if (index == 0)
		{
			return;
		}
		const std::string& trigger = gestures->getSelectedValue().asString();

		// pretend the user chatted the trigger string, to invoke
		// substitution and logging.
		std::string text(trigger);
		std::string revised_text;
		gGestureManager.triggerAndReviseString(text, &revised_text);

		revised_text = utf8str_trim(revised_text);
		if (!revised_text.empty())
		{
			// Don't play nodding animation
			LLBottomTray::sendChatFromViewer(revised_text, CHAT_TYPE_NORMAL, FALSE);
		}
	}

	mGestureLabelTimer.start();
	// free focus back to chat bar
	setFocus(FALSE);
}

//virtual
void LLGestureComboBox::draw()
{
	// HACK: Leave the name of the gesture in place for a few seconds.
	const F32 SHOW_GESTURE_NAME_TIME = 2.f;
	if (mGestureLabelTimer.getStarted() && mGestureLabelTimer.getElapsedTimeF32() > SHOW_GESTURE_NAME_TIME)
	{
		LLCtrlListInterface* gestures = getListInterface();
		if (gestures) gestures->selectFirstItem();
		mGestureLabelTimer.stop();
	}

	LLComboBox::draw();
}

LLBottomTray::LLBottomTray(const LLSD&)
	: mChatBox(NULL)
	, mChicletPanel(NULL)
	, mIMWell(NULL)
	, mSysWell(NULL)
	, mTalkBtn(NULL)
	, mGestureCombo(NULL)
	, mStandUpBtn(NULL)  ////FIXME: temporary, for stand up proto
{
	LLUICtrlFactory::getInstance()->buildPanel(this,"panel_bottomtray.xml");

	mChicletPanel = getChild<LLChicletPanel>("chiclet_list",TRUE,FALSE);
	mIMWell = getChild<LLNotificationChiclet>("im_well",TRUE,FALSE);
	mSysWell = getChild<LLNotificationChiclet>("sys_well",TRUE,FALSE);
	mChatBox = getChild<LLLineEditor>("chat_box",TRUE,FALSE);

	mChicletPanel->setChicletClickedCallback(boost::bind(&LLBottomTray::onChicletClick,this,_1));

	if (mChatBox)
	{
		mChatBox->setCommitCallback(boost::bind(&LLBottomTray::onChatBoxCommit, this));
		mChatBox->setKeystrokeCallback(&onChatBoxKeystroke, this);
		mChatBox->setFocusLostCallback(&onChatBoxFocusLost, this);

		mChatBox->setIgnoreArrowKeys(TRUE);
		mChatBox->setCommitOnFocusLost( FALSE );
		mChatBox->setRevertOnEsc( FALSE );
		mChatBox->setIgnoreTab(TRUE);
		mChatBox->setPassDelete(TRUE);
		mChatBox->setReplaceNewlinesWithSpaces(FALSE);
		mChatBox->setMaxTextLength(1023);
		mChatBox->setEnableLineHistory(TRUE);

	}

	mGestureCombo = getChild<LLGestureComboBox>( "Gesture", TRUE, FALSE);

	////FIXME: temporary, for stand up proto
	mStandUpBtn = getChild<LLButton> ("stand", TRUE, FALSE);
	if (mStandUpBtn)
	{
		mStandUpBtn->setCommitCallback(boost::bind(&LLBottomTray::onCommitStandUp, this, _1));
	}
	
	LLIMMgr::getInstance()->addSessionObserver(this);

	//this is to fix a crash that occurs because LLBottomTray is a singleton
	//and thus is deleted at the end of the viewers lifetime, but to be cleanly
	//destroyed LLBottomTray requires some subsystems that are long gone
	LLUI::getRootView()->addChild(this);

	// Necessary for focus movement among child controls
	setFocusRoot(TRUE);
}

LLBottomTray::~LLBottomTray()
{
	if (!LLSingleton<LLIMMgr>::destroyed())
	{
		LLIMMgr::getInstance()->removeSessionObserver(this);
	}
}

void LLBottomTray::onChicletClick(LLUICtrl* ctrl)
{
	LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(ctrl);
	if (chiclet)
	{
		LLFloaterReg::showInstance("communicate", chiclet->getSessionId());
	}
}

void LLBottomTray::onChatBoxCommit()
{
	if (mChatBox && mChatBox->getText().length() > 0)
	{
		sendChat(CHAT_TYPE_NORMAL);
	}

	gAgent.stopTyping();
}

void LLBottomTray::sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate)
{
	sendChatFromViewer(utf8str_to_wstring(utf8text), type, animate);
}

void LLBottomTray::sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate)
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
void LLBottomTray::onChatBoxKeystroke(LLLineEditor* caller, void* userdata)
{
	LLBottomTray* self = (LLBottomTray *)userdata;

	LLWString raw_text;
	if (self->getChatBox()) raw_text = self->getChatBox()->getWText();

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

		if (gGestureManager.matchPrefix(utf8_trigger, &utf8_out_str))
		{
			if (self->getChatBox())
			{
				std::string rest_of_match = utf8_out_str.substr(utf8_trigger.size());
				self->getChatBox()->setText(utf8_trigger + rest_of_match); // keep original capitalization for user-entered part
				S32 outlength = self->getChatBox()->getLength(); // in characters
			
				// Select to end of line, starting from the character
				// after the last one the user typed.
				self->getChatBox()->setSelection(length, outlength);
			}
		}

		//llinfos << "GESTUREDEBUG " << trigger 
		//	<< " len " << length
		//	<< " outlen " << out_str.getLength()
		//	<< llendl;
	}
}

// static
void LLBottomTray::onChatBoxFocusLost(LLFocusableElement* caller, void* userdata)
{
	// stop typing animation
	gAgent.stopTyping();
}

BOOL LLBottomTray::inputEditorHasFocus()
{
	return mChatBox && mChatBox->hasFocus();
}

std::string LLBottomTray::getCurrentChat()
{
	return mChatBox ? mChatBox->getText() : LLStringUtil::null;
}

//virtual
void LLBottomTray::draw()
{
	refreshStandUp();
	LLPanel::draw();
}

void LLBottomTray::refreshStandUp()
{
	//FIXME: temporary, for stand up proto
	BOOL sitting = FALSE;
	if (gAgent.getAvatarObject())
	{
		sitting = gAgent.getAvatarObject()->mIsSitting;
	}
	
	if (mStandUpBtn && mStandUpBtn->getVisible() != sitting)
	{
		mStandUpBtn->setVisible(sitting);
		sendChildToFront(mStandUpBtn);
		moveChildToBackOfTabGroup(mStandUpBtn);
	}
}

//FIXME: temporary, for stand up proto
void LLBottomTray::onCommitStandUp(LLUICtrl* ctrl)
{
	LLSelectMgr::getInstance()->deselectAllForStandingUp();
	gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
}

//virtual
void LLBottomTray::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	if(getChicletPanel())
	{
		if(getChicletPanel()->findChiclet<LLChiclet>(session_id))
		{

		}
		else
		{
			LLIMChiclet* chicklet = getChicletPanel()->createChiclet<LLIMChiclet>(session_id);
			chicklet->setIMSessionName(name);
			chicklet->setOtherParticipantId(other_participant_id);

			if(getChicletPanel()->getChicletCount())
			{
				setChicletPanelVisible(true);
			}
		}
	}
}

//virtual
void LLBottomTray::sessionRemoved(const LLUUID& session_id)
{
	if(getChicletPanel())
	{
		getChicletPanel()->removeChiclet(session_id);

		if(0 == getChicletPanel()->getChicletCount())
		{
			setChicletPanelVisible(false);
		}
	}
}

void LLBottomTray::setChicletPanelVisible(bool visible)
{
	// Chiclet panel is placed in layout_panel, which is child of layout_stack.
	// To gide chiclet panel we need to also hide layout_panel to make layout_stack resize its
	// content.
	getChicletPanel()->getParent()->setVisible(visible);
	if(visible)
	{
		// Reshape layout stack after making chiclet panel visible
		LLView* layout = getChild<LLView>("toolbar_stack");
		LLRect rc = layout->getRect();
		layout->reshape(rc.getWidth(), rc.getHeight());
		layout->setRect(rc);
	}
}

//virtual
void LLBottomTray::onFocusLost()
{
	if (gAgent.cameraMouselook())
	{
		setVisible(FALSE);
	}
}

// virtual
BOOL LLBottomTray::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;

	// ALT-RETURN is reserved for windowed/fullscreen toggle
	if( KEY_RETURN == key && mask == MASK_CONTROL)
	{
		// shout
		sendChat(CHAT_TYPE_SHOUT);
		handled = TRUE;
	}

	return handled;
}

//virtual
// setVisible used instead of onVisibilityChange, since LLAgent calls it on entering/leaving mouselook mode.
// If bottom tray is already visible in mouselook mode, then onVisibilityChange will not be called from setVisible(true),
void LLBottomTray::setVisible(BOOL visible)
{
	LLPanel::setVisible(visible);

	
	BOOL visibility = gAgent.cameraMouselook() ? false : true;

	LLViewBorder* separator = getChild<LLViewBorder>("well_separator",TRUE,FALSE);

	if (separator && separator->getVisible() == visibility)
		return;

	if (separator)
		separator->setVisible(visibility);

	LLPanel* p = getChild<LLPanel>("chiclet_list_panel",TRUE,FALSE);
	if (p)
		p->setVisible(visibility);

	p = getChild<LLPanel>("im_well_panel",TRUE,FALSE);
	if (p)
		p->setVisible(visibility);

	p = getChild<LLPanel>("sys_well_panel",TRUE,FALSE);
	if (p)
		p->setVisible(visibility);

}

// static 
void LLBottomTray::startChat(const char* line)
{
	LLBottomTray *bt = LLBottomTray::getInstance();

	if(bt && bt->getChatBox())
	{
		bt->setVisible(TRUE);
		bt->getChatBox()->setFocus(TRUE);

		if (line)
		{
			std::string line_string(line);
			bt->getChatBox()->setText(line_string);
		}

		bt->getChatBox()->setCursorToEnd();
	}
}

// Exit "chat mode" and do the appropriate focus changes
// static
void LLBottomTray::stopChat()
{
	LLBottomTray *bt = LLBottomTray::getInstance();

	if(bt && bt->getChatBox())
	{
		bt->getChatBox()->setFocus(FALSE);
	}

 	// stop typing animation
 	gAgent.stopTyping();
}

void LLBottomTray::sendChat( EChatType type )
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
				gGestureManager.triggerAndReviseString(utf8text, &utf8_revised_text);
			}
			else
			{
				utf8_revised_text = utf8text;
			}

			utf8_revised_text = utf8str_trim(utf8_revised_text);

			if (!utf8_revised_text.empty())
			{
				// Chat with animation
				sendChatFromViewer(utf8_revised_text, type, TRUE);
			}
		}

		mChatBox->setText(LLStringExplicit(""));
	}

	gAgent.stopTyping();
}

// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
// Otherwise returns input and channel 0.
LLWString LLBottomTray::stripChannelNumber(const LLWString &mesg, S32* channel)
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

class LLChatHandler : public LLCommandHandler
{
public:
	// not allowed from outside the app
	LLChatHandler() : LLCommandHandler("chat", true) { }

    // Your code here
	bool handle(const LLSD& tokens, const LLSD& query_map,
				LLWebBrowserCtrl* web)
	{
		if (tokens.size() < 2) return false;
		S32 channel = tokens[0].asInteger();
		std::string mesg = tokens[1].asString();
		send_chat_from_viewer(mesg, CHAT_TYPE_NORMAL, channel);
		return true;
	}
};

// Creating the object registers with the dispatcher.
LLChatHandler gChatHandler;

