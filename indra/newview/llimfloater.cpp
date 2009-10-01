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
#include "llfloaterreg.h"
#include "llimview.h"
#include "lllineeditor.h"
#include "llpanelimcontrolpanel.h"
#include "llscreenchannel.h"
#include "lltrans.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "lltransientfloatermgr.h"



LLIMFloater::LLIMFloater(const LLUUID& session_id)
  : LLDockableFloater(NULL, session_id),
	mControlPanel(NULL),
	mSessionID(session_id),
	mLastMessageIndex(-1),
	mLastFromName(),
	mDialog(IM_NOTHING_SPECIAL),
	mHistoryEditor(NULL),
	mInputEditor(NULL), 
	mPositioned(false)
{
	EInstantMessage type = LLIMModel::getInstance()->getType(session_id);
	if(IM_COUNT != type)
	{
		mDialog = type;

		if (IM_NOTHING_SPECIAL == mDialog || IM_SESSION_P2P_INVITE == mDialog)
		{
			mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelIMControl, this);
		}
		else
		{
			mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelGroupControl, this);
		}
	}

	LLUI::getRootView()->setFocusLostCallback(boost::bind(&LLIMFloater::focusChangeCallback, this));

	mCloseSignal.connect(boost::bind(&LLIMFloater::onClose, this));

	LLTransientFloaterMgr::getInstance()->registerTransientFloater(this);
}

void LLIMFloater::onClose()
{
	LLIMModel::instance().sendLeaveSession(mSessionID, mOtherParticipantUUID);

	//*TODO - move to the IMModel::sendLeaveSession() for the integrity (IB)
	gIMMgr->removeSession(mSessionID);
}

void LLIMFloater::setMinimized(BOOL minimize)
{
	if(!isDocked())
	{
		setVisible(!minimize);
	}	

	LLFloater::setMinimized(minimize);
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
			
			LLIMModel::sendMessage(utf8_text,
								mSessionID,
								mOtherParticipantUUID,
								mDialog);

			mInputEditor->setText(LLStringUtil::null);

			updateMessages();
		}
	}
}



LLIMFloater::~LLIMFloater()
{
	LLTransientFloaterMgr::getInstance()->unregisterTransientFloater(this);
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
	
	mHistoryEditor = getChild<LLViewerTextEditor>("im_text");
	mHistoryEditor->setParseHTML(TRUE);
		
	setTitle(LLIMModel::instance().getName(mSessionID));
	setDocked(true);
	
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
	self->mControlPanel = new LLPanelGroupControlPanel();
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
	
	LLDockableFloater::setDocked(docked, pop_on_undock);

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
	LLDockableFloater::setVisible(visible);

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
	if (floater && floater->getVisible())
	{
		// clicking on chiclet to close floater just hides it to maintain existing
		// scroll/text entry state
		floater->setVisible(false);
		return false;
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

void LLIMFloater::updateMessages()
{
	std::list<LLSD> messages = LLIMModel::instance().getMessages(mSessionID, mLastMessageIndex+1);
	std::string agent_name;

	gCacheName->getFullName(gAgentID, agent_name);

	if (messages.size())
	{
		LLUIColor divider_color = LLUIColorTable::instance().getColor("LtGray_50");
		LLUIColor chat_color = LLUIColorTable::instance().getColor("IMChatColor");

		std::ostringstream message;
		std::list<LLSD>::const_reverse_iterator iter = messages.rbegin();
		std::list<LLSD>::const_reverse_iterator iter_end = messages.rend();
	    for (; iter != iter_end; ++iter)
		{
			LLSD msg = *iter;

			const bool prepend_newline = true;
			std::string from = msg["from"].asString();
			if (from == agent_name)
				from = LLTrans::getString("You");
			if (mLastFromName != from)
			{
				message << from << " ----- " << msg["time"].asString();
				mHistoryEditor->appendColoredText(message.str(), false,
					prepend_newline, divider_color);
				message.str("");
				mLastFromName = from;
			}

			message << msg["message"].asString(); 
			mHistoryEditor->appendColoredText(message.str(), false,
				prepend_newline, chat_color);
			message.str("");

			mLastMessageIndex = msg["index"].asInteger();
		}

		mHistoryEditor->setCursorAndScrollToEnd();
	}
}

// static
void LLIMFloater::onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata )
{
	LLIMFloater* self= (LLIMFloater*) userdata;

	//in disconnected state IM input editor should be disabled
	self->mInputEditor->setEnabled(!gDisconnected);

	self->mHistoryEditor->setCursorAndScrollToEnd();
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

