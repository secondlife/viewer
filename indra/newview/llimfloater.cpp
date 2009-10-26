/** 
 * @file llimfloater.cpp
 * @brief LLIMFloater class definition
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

#include "llviewerprecompiledheaders.h"

#include "llimfloater.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llbutton.h"
#include "llbottomtray.h"
#include "llchannelmanager.h"
#include "llchiclet.h"
#include "llfloaterchat.h"
#include "llfloaterreg.h"
#include "llimview.h"
#include "lllineeditor.h"
#include "lllogchat.h"
#include "llpanelimcontrolpanel.h"
#include "llscreenchannel.h"
#include "lltrans.h"
#include "llchathistory.h"
#include "llviewerwindow.h"
#include "lltransientfloatermgr.h"



LLIMFloater::LLIMFloater(const LLUUID& session_id)
  : LLTransientDockableFloater(NULL, true, session_id),
	mControlPanel(NULL),
	mSessionID(session_id),
	mLastMessageIndex(-1),
	mDialog(IM_NOTHING_SPECIAL),
	mChatHistory(NULL),
	mInputEditor(NULL), 
	mPositioned(false),
	mSessionInitialized(false)
{
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(mSessionID);
	if (im_session)
	{
		mSessionInitialized = im_session->mSessionInitialized;
		
		mDialog = im_session->mType;
		if (IM_NOTHING_SPECIAL == mDialog || IM_SESSION_P2P_INVITE == mDialog)
		{
			mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelIMControl, this);
		}
		else
		{
			mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelGroupControl, this);
		}
	}
}

void LLIMFloater::onFocusLost()
{
	LLIMModel::getInstance()->resetActiveSessionID();
}

void LLIMFloater::onFocusReceived()
{
	LLIMModel::getInstance()->setActiveSessionID(mSessionID);
}

// virtual
void LLIMFloater::onClose(bool app_quitting)
{
	gIMMgr->leaveSession(mSessionID);
}

/* static */
void LLIMFloater::newIMCallback(const LLSD& data){
	
	if (data["num_unread"].asInteger() > 0)
	{
		LLUUID session_id = data["session_id"].asUUID();

		LLIMFloater* floater = LLFloaterReg::findTypedInstance<LLIMFloater>("impanel", session_id);
		if (floater == NULL)
		{
			llwarns << "new_im_callback for non-existent session_id " << session_id << llendl;
			return;
		}

        // update if visible, otherwise will be updated when opened
		if (floater->getVisible())
		{
			floater->updateMessages();
		}
	}
}

void LLIMFloater::onVisibilityChange(const LLSD& new_visibility)
{
	bool visible = new_visibility.asBoolean();

	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionID);

	if (visible && voice_channel &&
		voice_channel->getState() == LLVoiceChannel::STATE_CONNECTED)
	{
		LLFloaterReg::showInstance("voice_call", mSessionID);
	}
	else
	{
		LLFloaterReg::hideInstance("voice_call", mSessionID);
	}
}

void LLIMFloater::onSendMsg( LLUICtrl* ctrl, void* userdata )
{
	LLIMFloater* self = (LLIMFloater*) userdata;
	self->sendMsg();
}

void LLIMFloater::sendMsg()
{
	if (!gAgent.isGodlike() 
		&& (mDialog == IM_NOTHING_SPECIAL)
		&& mOtherParticipantUUID.isNull())
	{
		llinfos << "Cannot send IM to everyone unless you're a god." << llendl;
		return;
	}

	if (mInputEditor)
	{
		LLWString text = mInputEditor->getConvertedText();
		if(!text.empty())
		{
			// Truncate and convert to UTF8 for transport
			std::string utf8_text = wstring_to_utf8str(text);
			utf8_text = utf8str_truncate(utf8_text, MAX_MSG_BUF_SIZE - 1);
			
			if (mSessionInitialized)
			{
				LLIMModel::sendMessage(utf8_text, mSessionID,
					mOtherParticipantUUID,mDialog);
			}
			else
			{
				//queue up the message to send once the session is initialized
				mQueuedMsgsForInit.append(utf8_text);
			}

			mInputEditor->setText(LLStringUtil::null);

			updateMessages();
		}
	}
}



LLIMFloater::~LLIMFloater()
{
}

//virtual
BOOL LLIMFloater::postBuild()
{
	const LLUUID& other_party_id = LLIMModel::getInstance()->getOtherParticipantID(mSessionID);
	if (other_party_id.notNull())
	{
		mOtherParticipantUUID = other_party_id;
		mControlPanel->setID(mOtherParticipantUUID);
	}

	LLButton* slide_left = getChild<LLButton>("slide_left_btn");
	slide_left->setVisible(mControlPanel->getVisible());
	slide_left->setClickedCallback(boost::bind(&LLIMFloater::onSlide, this));

	LLButton* slide_right = getChild<LLButton>("slide_right_btn");
	slide_right->setVisible(!mControlPanel->getVisible());
	slide_right->setClickedCallback(boost::bind(&LLIMFloater::onSlide, this));

	mInputEditor = getChild<LLLineEditor>("chat_editor");
	mInputEditor->setMaxTextLength(1023);
	// enable line history support for instant message bar
	mInputEditor->setEnableLineHistory(TRUE);
	
	mInputEditor->setFocusReceivedCallback( boost::bind(onInputEditorFocusReceived, _1, this) );
	mInputEditor->setFocusLostCallback( boost::bind(onInputEditorFocusLost, _1, this) );
	mInputEditor->setKeystrokeCallback( onInputEditorKeystroke, this );
	mInputEditor->setCommitOnFocusLost( FALSE );
	mInputEditor->setRevertOnEsc( FALSE );
	mInputEditor->setReplaceNewlinesWithSpaces( FALSE );

	childSetCommitCallback("chat_editor", onSendMsg, this);
	
	mChatHistory = getChild<LLChatHistory>("chat_history");
		
	setTitle(LLIMModel::instance().getName(mSessionID));
	setDocked(true);
	
	if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
	{
		LLLogChat::loadHistory(getTitle(), &chatFromLogFile, (void *)this);
	}

	//*TODO if session is not initialized yet, add some sort of a warning message like "starting session...blablabla"
	//see LLFloaterIMPanel for how it is done (IB)

	return LLDockableFloater::postBuild();
}



// static
void* LLIMFloater::createPanelIMControl(void* userdata)
{
	LLIMFloater *self = (LLIMFloater*)userdata;
	self->mControlPanel = new LLPanelIMControlPanel();
	self->mControlPanel->setXMLFilename("panel_im_control_panel.xml");
	return self->mControlPanel;
}


// static
void* LLIMFloater::createPanelGroupControl(void* userdata)
{
	LLIMFloater *self = (LLIMFloater*)userdata;
	self->mControlPanel = new LLPanelGroupControlPanel(self->mSessionID);
	self->mControlPanel->setXMLFilename("panel_group_control_panel.xml");
	return self->mControlPanel;
}

void LLIMFloater::onSlide()
{
	LLPanel* im_control_panel = getChild<LLPanel>("panel_im_control_panel");
	im_control_panel->setVisible(!im_control_panel->getVisible());

	getChild<LLButton>("slide_left_btn")->setVisible(im_control_panel->getVisible());
	getChild<LLButton>("slide_right_btn")->setVisible(!im_control_panel->getVisible());
}

//static
LLIMFloater* LLIMFloater::show(const LLUUID& session_id)
{
	//hide all
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("impanel");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
		 iter != inst_list.end(); ++iter)
	{
		LLIMFloater* floater = dynamic_cast<LLIMFloater*>(*iter);
		if (floater && floater->isDocked())
		{
			floater->setVisible(false);
		}
	}

	LLIMFloater* floater = LLFloaterReg::showTypedInstance<LLIMFloater>("impanel", session_id);

	floater->updateMessages();
	floater->mInputEditor->setFocus(TRUE);

	if (floater->getDockControl() == NULL)
	{
		LLChiclet* chiclet =
				LLBottomTray::getInstance()->getChicletPanel()->findChiclet<LLChiclet>(
						session_id);
		if (chiclet == NULL)
		{
			llerror("Dock chiclet for LLIMFloater doesn't exists", 0);
		}
		else
		{
			LLBottomTray::getInstance()->getChicletPanel()->scrollToChiclet(chiclet);
		}

		floater->setDockControl(new LLDockControl(chiclet, floater, floater->getDockTongue(),
				LLDockControl::TOP,  boost::bind(&LLIMFloater::getAllowedRect, floater, _1)));
	}

	return floater;
}

void LLIMFloater::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRect();
}

void LLIMFloater::setDocked(bool docked, bool pop_on_undock)
{
	// update notification channel state
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>
		(LLNotificationsUI::LLChannelManager::getInstance()->
											findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));

	setCanResize(!docked);
	
	LLTransientDockableFloater::setDocked(docked, pop_on_undock);

	// update notification channel state
	if(channel)
	{
		channel->updateShowToastsState();
	}
}

void LLIMFloater::setVisible(BOOL visible)
{
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>
		(LLNotificationsUI::LLChannelManager::getInstance()->
											findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));
	LLTransientDockableFloater::setVisible(visible);

	// update notification channel state
	if(channel)
	{
		channel->updateShowToastsState();
	}
}

//static
bool LLIMFloater::toggle(const LLUUID& session_id)
{
	LLIMFloater* floater = LLFloaterReg::findTypedInstance<LLIMFloater>("impanel", session_id);
	if (floater && floater->getVisible() && floater->isDocked())
	{
		// clicking on chiclet to close floater just hides it to maintain existing
		// scroll/text entry state
		floater->setVisible(false);
		return false;
	}
	else if(floater && !floater->isDocked())
	{
		floater->setVisible(TRUE);
		floater->setFocus(TRUE);
		return true;
	}
	else
	{
		// ensure the list of messages is updated when floater is made visible
		show(session_id);
		// update number of unread notifications in the SysWell
		LLBottomTray::getInstance()->getSysWell()->updateUreadIMNotifications();
		return true;
	}
}

//static
LLIMFloater* LLIMFloater::findInstance(const LLUUID& session_id)
{
	return LLFloaterReg::findTypedInstance<LLIMFloater>("impanel", session_id);
}

void LLIMFloater::sessionInitReplyReceived(const LLUUID& im_session_id)
{
	mSessionInitialized = true;

	if (mSessionID != im_session_id)
	{
		mSessionID = im_session_id;
		setKey(im_session_id);
	}
	
	//*TODO here we should remove "starting session..." warning message if we added it in postBuild() (IB)


	//need to send delayed messaged collected while waiting for session initialization
	if (!mQueuedMsgsForInit.size()) return;
	LLSD::array_iterator iter;
	for ( iter = mQueuedMsgsForInit.beginArray();
		iter != mQueuedMsgsForInit.endArray();
		++iter)
	{
		LLIMModel::sendMessage(iter->asString(), mSessionID,
			mOtherParticipantUUID, mDialog);
	}
}

void LLIMFloater::updateMessages()
{
	std::list<LLSD> messages = LLIMModel::instance().getMessages(mSessionID, mLastMessageIndex+1);
	std::string agent_name;

	gCacheName->getFullName(gAgentID, agent_name);

	if (messages.size())
	{
		LLUIColor chat_color = LLUIColorTable::instance().getColor("IMChatColor");

		std::ostringstream message;
		std::list<LLSD>::const_reverse_iterator iter = messages.rbegin();
		std::list<LLSD>::const_reverse_iterator iter_end = messages.rend();
	    for (; iter != iter_end; ++iter)
		{
			LLSD msg = *iter;

			std::string from = msg["from"].asString();
			std::string time = msg["time"].asString();
			LLUUID from_id = msg["from_id"].asUUID();
			std::string message = msg["message"].asString();
			LLStyle::Params style_params;
			style_params.color(chat_color);

			if (from == agent_name)
				from = LLTrans::getString("You");

			mChatHistory->appendWidgetMessage(from_id, from, time, message, style_params);

			mLastMessageIndex = msg["index"].asInteger();
		}
	}
}

// static
void LLIMFloater::onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata )
{
	LLIMFloater* self= (LLIMFloater*) userdata;

	//in disconnected state IM input editor should be disabled
	self->mInputEditor->setEnabled(!gDisconnected);

	self->mChatHistory->setCursorAndScrollToEnd();
}

// static
void LLIMFloater::onInputEditorFocusLost(LLFocusableElement* caller, void* userdata)
{
	LLIMFloater* self = (LLIMFloater*) userdata;
	self->setTyping(FALSE);
}

// static
void LLIMFloater::onInputEditorKeystroke(LLLineEditor* caller, void* userdata)
{
	LLIMFloater* self = (LLIMFloater*)userdata;
	std::string text = self->mInputEditor->getText();
	if (!text.empty())
	{
		self->setTyping(TRUE);
	}
	else
	{
		// Deleting all text counts as stopping typing.
		self->setTyping(FALSE);
	}
}


//just a stub for now
void LLIMFloater::setTyping(BOOL typing)
{
}

void LLIMFloater::chatFromLogFile(LLLogChat::ELogLineType type, std::string line, void* userdata)
{
	if (!userdata) return;

	LLIMFloater* self = (LLIMFloater*) userdata;
	std::string message = line;
	S32 im_log_option =  gSavedPerAccountSettings.getS32("IMLogOptions");
	switch (type)
	{
	case LLLogChat::LOG_EMPTY:
		// add warning log enabled message
		if (im_log_option!=LOG_CHAT)
		{
			message = LLTrans::getString("IM_logging_string");
		}
		break;
	case LLLogChat::LOG_END:
		// add log end message
		if (im_log_option!=LOG_CHAT)
		{
			message = LLTrans::getString("IM_logging_string");
		}
		break;
	case LLLogChat::LOG_LINE:
		// just add normal lines from file
		break;
	default:
		// nothing
		break;
	}

	self->mChatHistory->appendText(message, true, LLStyle::Params().color(LLUIColorTable::instance().getColor("ChatHistoryTextColor")));
	self->mChatHistory->blockUndo();
}

