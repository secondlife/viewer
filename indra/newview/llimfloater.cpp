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

#include "llnotificationsutil.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llbutton.h"
#include "llbottomtray.h"
#include "llchannelmanager.h"
#include "llchiclet.h"
#include "llfloaterchat.h"
#include "llfloaterreg.h"
#include "llimfloatercontainer.h" // to replace separate IM Floaters with multifloater container
#include "lllayoutstack.h"
#include "lllineeditor.h"
#include "lllogchat.h"
#include "llpanelimcontrolpanel.h"
#include "llscreenchannel.h"
#include "llsyswellwindow.h"
#include "lltrans.h"
#include "llchathistory.h"
#include "llviewerwindow.h"
#include "llvoicechannel.h"
#include "lltransientfloatermgr.h"
#include "llinventorymodel.h"
#include "llrootview.h"



LLIMFloater::LLIMFloater(const LLUUID& session_id)
  : LLTransientDockableFloater(NULL, true, session_id),
	mControlPanel(NULL),
	mSessionID(session_id),
	mLastMessageIndex(-1),
	mDialog(IM_NOTHING_SPECIAL),
	mChatHistory(NULL),
	mInputEditor(NULL),
	mSavedTitle(),
	mTypingStart(),
	mShouldSendTypingState(false),
	mMeTyping(false),
	mOtherTyping(false),
	mTypingTimer(),
	mTypingTimeoutTimer(),
	mPositioned(false),
	mSessionInitialized(false)
{
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(mSessionID);
	if (im_session)
	{
		mSessionInitialized = im_session->mSessionInitialized;
		
		mDialog = im_session->mType;
		switch(mDialog){
		case IM_NOTHING_SPECIAL:
		case IM_SESSION_P2P_INVITE:
			mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelIMControl, this);
			break;
		case IM_SESSION_CONFERENCE_START:
			mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelAdHocControl, this);
			break;
		case IM_SESSION_GROUP_START:
			mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelGroupControl, this);
			break;
		case IM_SESSION_INVITE:		
			if (gAgent.isInGroup(mSessionID))
			{
				mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelGroupControl, this);
			}
			else
			{
				mFactoryMap["panel_im_control_panel"] = LLCallbackMap(createPanelAdHocControl, this);
			}
			break;
		default: break;
		}
	}
	setOverlapsScreenChannel(true);
}

void LLIMFloater::onFocusLost()
{
	LLIMModel::getInstance()->resetActiveSessionID();
}

void LLIMFloater::onFocusReceived()
{
	LLIMModel::getInstance()->setActiveSessionID(mSessionID);

	// return focus to the input field when active tab in the multitab container is clicked.
	if (isChatMultiTab() && mInputEditor)
	{
		mInputEditor->setFocus(TRUE);
	}
}

// virtual
void LLIMFloater::onClose(bool app_quitting)
{
	setTyping(false);

	// The source of much argument and design thrashing
	// Should the window hide or the session close when the X is clicked?
	//
	// Last change:
	// EXT-3516 X Button should end IM session, _ button should hide
	gIMMgr->leaveSession(mSessionID);
}

/* static */
void LLIMFloater::newIMCallback(const LLSD& data){
	
	if (data["num_unread"].asInteger() > 0 || data["from_id"].asUUID().isNull())
	{
		LLUUID session_id = data["session_id"].asUUID();

		LLIMFloater* floater = LLFloaterReg::findTypedInstance<LLIMFloater>("impanel", session_id);
		if (floater == NULL) return;

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
	self->setTyping(false);
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
	// User-resizable control panels in P2P sessions look ugly (EXT-3470).
	if (mDialog == IM_NOTHING_SPECIAL || mDialog == IM_SESSION_P2P_INVITE)
	{
		getChild<LLLayoutStack>("im_panels")->setPanelUserResize("panel_im_control_panel", FALSE);
	}

	const LLUUID& other_party_id = LLIMModel::getInstance()->getOtherParticipantID(mSessionID);
	if (other_party_id.notNull())
	{
		mOtherParticipantUUID = other_party_id;
	}

	mControlPanel->setSessionId(mSessionID);
	mControlPanel->setVisible(gSavedSettings.getBOOL("IMShowControlPanel"));

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

	std::string session_name(LLIMModel::instance().getName(mSessionID));

	mInputEditor->setLabel(LLTrans::getString("IM_to_label") + " " + session_name);

	setTitle(session_name);

	childSetCommitCallback("chat_editor", onSendMsg, this);
	
	mChatHistory = getChild<LLChatHistory>("chat_history");

	setDocked(true);

	mTypingStart = LLTrans::getString("IM_typing_start_string");

	// Disable input editor if session cannot accept text
	LLIMModel::LLIMSession* im_session =
		LLIMModel::instance().findIMSession(mSessionID);
	if( im_session && !im_session->mTextIMPossible )
	{
		mInputEditor->setEnabled(FALSE);
		mInputEditor->setLabel(LLTrans::getString("IM_unavailable_text_label"));
	}

	//*TODO if session is not initialized yet, add some sort of a warning message like "starting session...blablabla"
	//see LLFloaterIMPanel for how it is done (IB)

	if(isChatMultiTab())
	{
		return LLFloater::postBuild();
	}
	else
	{
		return LLDockableFloater::postBuild();
	}
}

// virtual
void LLIMFloater::draw()
{
	if ( mMeTyping )
	{
		// Time out if user hasn't typed for a while.
		if ( mTypingTimeoutTimer.getElapsedTimeF32() > LLAgent::TYPING_TIMEOUT_SECS )
		{
			setTyping(false);
		}
	}

	LLTransientDockableFloater::draw();
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

// static
void* LLIMFloater::createPanelAdHocControl(void* userdata)
{
	LLIMFloater *self = (LLIMFloater*)userdata;
	self->mControlPanel = new LLPanelAdHocControlPanel(self->mSessionID);
	self->mControlPanel->setXMLFilename("panel_adhoc_control_panel.xml");
	return self->mControlPanel;
}

void LLIMFloater::onSlide()
{
	LLPanel* im_control_panel = getChild<LLPanel>("panel_im_control_panel");
	im_control_panel->setVisible(!im_control_panel->getVisible());

	gSavedSettings.setBOOL("IMShowControlPanel", im_control_panel->getVisible());

	getChild<LLButton>("slide_left_btn")->setVisible(im_control_panel->getVisible());
	getChild<LLButton>("slide_right_btn")->setVisible(!im_control_panel->getVisible());
}

//static
LLIMFloater* LLIMFloater::show(const LLUUID& session_id)
{
	if (!gIMMgr->hasSession(session_id)) return NULL;

	// we should make sure all related chiclets are in place when the session is a voice call
	// chiclets come firts, then comes IM window
	if (gIMMgr->isVoiceCall(session_id))
	{
		LLIMModel* im_model = LLIMModel::getInstance();
		LLBottomTray* b_tray = LLBottomTray::getInstance();
		
		//*TODO hide that into Bottom tray
		if (!b_tray->getChicletPanel()->findChiclet<LLChiclet>(session_id))
		{
			LLIMChiclet* chiclet = b_tray->createIMChiclet(session_id);
			if(chiclet)
			{
				chiclet->setIMSessionName(im_model->getName(session_id));
				chiclet->setOtherParticipantId(im_model->getOtherParticipantID(session_id));
			}
		}

		LLIMWellWindow::getInstance()->addIMRow(session_id);
	}
		
	bool not_existed = true;

	if(isChatMultiTab())
	{
		LLIMFloater* target_floater = findInstance(session_id);
		not_existed = NULL == target_floater;
	}
	else
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
	}

	LLIMFloater* floater = LLFloaterReg::showTypedInstance<LLIMFloater>("impanel", session_id);

	if(isChatMultiTab())
	{
		// do not add existed floaters to avoid adding torn off instances
		if (not_existed)
		{
			//		LLTabContainer::eInsertionPoint i_pt = user_initiated ? LLTabContainer::RIGHT_OF_CURRENT : LLTabContainer::END;
			// TODO: mantipov: use LLTabContainer::RIGHT_OF_CURRENT if it exists
			LLTabContainer::eInsertionPoint i_pt = LLTabContainer::END;

			LLIMFloaterContainer* floater_container = LLFloaterReg::showTypedInstance<LLIMFloaterContainer>("im_container");
			floater_container->addFloater(floater, TRUE, i_pt);
		}
	}
	else
	{
		// Docking may move chat window, hide it before moving, or user will see how window "jumps"
		floater->setVisible(false);

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

		// window is positioned, now we can show it.
		floater->setVisible(true);
	}

	return floater;
}

void LLIMFloater::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRectRaw();
	static S32 right_padding = 0;
	if (right_padding == 0)
	{
		LLPanel* side_bar_tabs =
				gViewerWindow->getRootView()->getChild<LLPanel> (
						"side_bar_tabs");
		right_padding = side_bar_tabs->getRect().getWidth();
		LLTransientFloaterMgr::getInstance()->addControlView(side_bar_tabs);
	}
	rect.mRight -= right_padding;
}

void LLIMFloater::setDocked(bool docked, bool pop_on_undock)
{
	// update notification channel state
	LLNotificationsUI::LLScreenChannel* channel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>
		(LLNotificationsUI::LLChannelManager::getInstance()->
											findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));
	
	if(!isChatMultiTab())
	{
		LLTransientDockableFloater::setDocked(docked, pop_on_undock);
	}

	// update notification channel state
	if(channel)
	{
		channel->updateShowToastsState();
		channel->redrawToasts();
	}
}

void LLIMFloater::setTornOff(bool torn_off)
{
	// When IM Floater isn't torn off, "close" button should be hidden.
	// This call will just disables it, since there is a hack in LLFloater::updateButton,
	// which prevents hiding of close button in that case.
	setCanClose(torn_off);

	LLTransientDockableFloater::setTornOff(torn_off);
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
		channel->redrawToasts();
	}

	if (visible && mChatHistory && mInputEditor)
	{
		//only if floater was construced and initialized from xml
		updateMessages();
		mInputEditor->setFocus(TRUE);
	}
}

//static
bool LLIMFloater::toggle(const LLUUID& session_id)
{
	if(!isChatMultiTab())
	{
		LLIMFloater* floater = LLFloaterReg::findTypedInstance<LLIMFloater>("impanel", session_id);
		if (floater && floater->getVisible())
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
	}

	// ensure the list of messages is updated when floater is made visible
	show(session_id);
	return true;
}

//static
LLIMFloater* LLIMFloater::findInstance(const LLUUID& session_id)
{
	return LLFloaterReg::findTypedInstance<LLIMFloater>("impanel", session_id);
}

void LLIMFloater::sessionInitReplyReceived(const LLUUID& im_session_id)
{
	mSessionInitialized = true;

	//will be different only for an ad-hoc im session
	if (mSessionID != im_session_id)
	{
		mSessionID = im_session_id;
		setKey(im_session_id);
		mControlPanel->setSessionId(im_session_id);
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
	bool use_plain_text_chat_history = gSavedSettings.getBOOL("PlainTextChatHistory");

	std::list<LLSD> messages;
	LLIMModel::instance().getMessages(mSessionID, messages, mLastMessageIndex+1);

	if (messages.size())
	{
//		LLUIColor chat_color = LLUIColorTable::instance().getColor("IMChatColor");

		std::ostringstream message;
		std::list<LLSD>::const_reverse_iterator iter = messages.rbegin();
		std::list<LLSD>::const_reverse_iterator iter_end = messages.rend();
		for (; iter != iter_end; ++iter)
		{
			LLSD msg = *iter;

			std::string time = msg["time"].asString();
			LLUUID from_id = msg["from_id"].asUUID();
			std::string from = msg["from"].asString();
			std::string message = msg["message"].asString();

			LLChat chat;
			chat.mFromID = from_id;
			chat.mFromName = from;
			chat.mText = message;
			chat.mTimeStr = time;
			
			mChatHistory->appendMessage(chat, use_plain_text_chat_history);
			mLastMessageIndex = msg["index"].asInteger();
		}
	}
}

// static
void LLIMFloater::onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata )
{
	LLIMFloater* self= (LLIMFloater*) userdata;

	// Allow enabling the LLIMFloater input editor only if session can accept text
	LLIMModel::LLIMSession* im_session =
		LLIMModel::instance().findIMSession(self->mSessionID);
	//TODO: While disabled lllineeditor can receive focus we need to check if it is enabled (EK)
	if( im_session && im_session->mTextIMPossible && self->mInputEditor->getEnabled())
	{
		//in disconnected state IM input editor should be disabled
		self->mInputEditor->setEnabled(!gDisconnected);
	}

	// when IM Floater is a part of the multitab container LLTabContainer set focus to the first
	// child on tab button's mouse up. This leads input field lost focus. See EXT-3852.
	if (isChatMultiTab())
	{
		// So, clear control captured mouse to prevent LLTabContainer set focus on the panel's first child.
		// do not pass self->mInputEditor, this leads to have "Edit Text" mouse pointer wherever it is.
		gFocusMgr.setMouseCapture(NULL);
	}
}

// static
void LLIMFloater::onInputEditorFocusLost(LLFocusableElement* caller, void* userdata)
{
	LLIMFloater* self = (LLIMFloater*) userdata;
	self->setTyping(false);
}

// static
void LLIMFloater::onInputEditorKeystroke(LLLineEditor* caller, void* userdata)
{
	LLIMFloater* self = (LLIMFloater*)userdata;
	std::string text = self->mInputEditor->getText();
	if (!text.empty())
	{
		self->setTyping(true);
	}
	else
	{
		// Deleting all text counts as stopping typing.
		self->setTyping(false);
	}
}

void LLIMFloater::setTyping(bool typing)
{
	if ( typing )
	{
		// Started or proceeded typing, reset the typing timeout timer
		mTypingTimeoutTimer.reset();
	}

	if ( mMeTyping != typing )
	{
		// Typing state is changed
		mMeTyping = typing;
		// So, should send current state
		mShouldSendTypingState = true;
		// In case typing is started, send state after some delay
		mTypingTimer.reset();
	}

	// Don't want to send typing indicators to multiple people, potentially too
	// much network traffic. Only send in person-to-person IMs.
	if ( mShouldSendTypingState && mDialog == IM_NOTHING_SPECIAL )
	{
		if ( mMeTyping )
		{
			if ( mTypingTimer.getElapsedTimeF32() > 1.f )
			{
				// Still typing, send 'start typing' notification
				LLIMModel::instance().sendTypingState(mSessionID, mOtherParticipantUUID, TRUE);
				mShouldSendTypingState = false;
			}
		}
		else
		{
			// Send 'stop typing' notification immediately
			LLIMModel::instance().sendTypingState(mSessionID, mOtherParticipantUUID, FALSE);
			mShouldSendTypingState = false;
		}
	}

	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
	if (speaker_mgr)
		speaker_mgr->setSpeakerTyping(gAgent.getID(), FALSE);

}

void LLIMFloater::processIMTyping(const LLIMInfo* im_info, BOOL typing)
{
	if ( typing )
	{
		// other user started typing
		addTypingIndicator(im_info);
	}
	else
	{
		// other user stopped typing
		removeTypingIndicator(im_info);
	}
}

void LLIMFloater::processAgentListUpdates(const LLSD& body)
{
	if ( !body.isMap() ) return;

	if ( body.has("agent_updates") && body["agent_updates"].isMap() )
	{
		LLSD agent_data = body["agent_updates"].get(gAgentID.asString());
		if (agent_data.isMap() && agent_data.has("info"))
		{
			LLSD agent_info = agent_data["info"];

			if (agent_info.has("mutes"))
			{
				BOOL moderator_muted_text = agent_info["mutes"]["text"].asBoolean(); 
				mInputEditor->setEnabled(!moderator_muted_text);
				std::string label;
				if (moderator_muted_text)
					label = LLTrans::getString("IM_muted_text_label");
				else
					label = LLTrans::getString("IM_to_label") + " " + LLIMModel::instance().getName(mSessionID);
				mInputEditor->setLabel(label);

				if (moderator_muted_text)
					LLNotificationsUtil::add("TextChatIsMutedByModerator");
			}
		}
	}
}

void LLIMFloater::updateChatHistoryStyle()
{
	mChatHistory->clear();
	mLastMessageIndex = -1;
	updateMessages();
}

void LLIMFloater::processChatHistoryStyleUpdate(const LLSD& newvalue)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("impanel");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin();
		 iter != inst_list.end(); ++iter)
	{
		LLIMFloater* floater = dynamic_cast<LLIMFloater*>(*iter);
		if (floater)
		{
			floater->updateChatHistoryStyle();
		}
	}

}

void LLIMFloater::processSessionUpdate(const LLSD& session_update)
{
	// *TODO : verify following code when moderated mode will be implemented
	if ( false && session_update.has("moderated_mode") &&
		 session_update["moderated_mode"].has("voice") )
	{
		BOOL voice_moderated = session_update["moderated_mode"]["voice"];
		const std::string session_label = LLIMModel::instance().getName(mSessionID);

		if (voice_moderated)
		{
			setTitle(session_label + std::string(" ") + LLTrans::getString("IM_moderated_chat_label"));
		}
		else
		{
			setTitle(session_label);
		}

		// *TODO : uncomment this when/if LLPanelActiveSpeakers panel will be added
		//update the speakers dropdown too
		//mSpeakerPanel->setVoiceModerationCtrlMode(voice_moderated);
	}
}

BOOL LLIMFloater::handleDragAndDrop(S32 x, S32 y, MASK mask,
						   BOOL drop, EDragAndDropType cargo_type,
						   void *cargo_data, EAcceptance *accept,
						   std::string& tooltip_msg)
{

	if (mDialog == IM_NOTHING_SPECIAL)
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mOtherParticipantUUID, mSessionID, drop,
												 cargo_type, cargo_data, accept);
	}

	// handle case for dropping calling cards (and folders of calling cards) onto invitation panel for invites
	else if (isInviteAllowed())
	{
		*accept = ACCEPT_NO;

		if (cargo_type == DAD_CALLINGCARD)
		{
			if (dropCallingCard((LLInventoryItem*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
		else if (cargo_type == DAD_CATEGORY)
		{
			if (dropCategory((LLInventoryCategory*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
	}
	return TRUE;
}

BOOL LLIMFloater::dropCallingCard(LLInventoryItem* item, BOOL drop)
{
	BOOL rv = isInviteAllowed();
	if(rv && item && item->getCreatorUUID().notNull())
	{
		if(drop)
		{
			std::vector<LLUUID> ids;
			ids.push_back(item->getCreatorUUID());
			inviteToSession(ids);
		}
	}
	else
	{
		// set to false if creator uuid is null.
		rv = FALSE;
	}
	return rv;
}

BOOL LLIMFloater::dropCategory(LLInventoryCategory* category, BOOL drop)
{
	BOOL rv = isInviteAllowed();
	if(rv && category)
	{
		LLInventoryModel::cat_array_t cats;
		LLInventoryModel::item_array_t items;
		LLUniqueBuddyCollector buddies;
		gInventory.collectDescendentsIf(category->getUUID(),
										cats,
										items,
										LLInventoryModel::EXCLUDE_TRASH,
										buddies);
		S32 count = items.count();
		if(count == 0)
		{
			rv = FALSE;
		}
		else if(drop)
		{
			std::vector<LLUUID> ids;
			ids.reserve(count);
			for(S32 i = 0; i < count; ++i)
			{
				ids.push_back(items.get(i)->getCreatorUUID());
			}
			inviteToSession(ids);
		}
	}
	return rv;
}

BOOL LLIMFloater::isInviteAllowed() const
{

	return ( (IM_SESSION_CONFERENCE_START == mDialog)
			 || (IM_SESSION_INVITE == mDialog) );
}

class LLSessionInviteResponder : public LLHTTPClient::Responder
{
public:
	LLSessionInviteResponder(const LLUUID& session_id)
	{
		mSessionID = session_id;
	}

	void error(U32 statusNum, const std::string& reason)
	{
		llinfos << "Error inviting all agents to session" << llendl;
		//throw something back to the viewer here?
	}

private:
	LLUUID mSessionID;
};

BOOL LLIMFloater::inviteToSession(const std::vector<LLUUID>& ids)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
	{
		return FALSE;
	}

	S32 count = ids.size();

	if( isInviteAllowed() && (count > 0) )
	{
		llinfos << "LLIMFloater::inviteToSession() - inviting participants" << llendl;

		std::string url = region->getCapability("ChatSessionRequest");

		LLSD data;

		data["params"] = LLSD::emptyArray();
		for (int i = 0; i < count; i++)
		{
			data["params"].append(ids[i]);
		}

		data["method"] = "invite";
		data["session-id"] = mSessionID;
		LLHTTPClient::post(
			url,
			data,
			new LLSessionInviteResponder(
					mSessionID));
	}
	else
	{
		llinfos << "LLIMFloater::inviteToSession -"
				<< " no need to invite agents for "
				<< mDialog << llendl;
		// successful add, because everyone that needed to get added
		// was added.
	}

	return TRUE;
}

void LLIMFloater::addTypingIndicator(const LLIMInfo* im_info)
{
	// We may have lost a "stop-typing" packet, don't add it twice
	if ( im_info && !mOtherTyping )
	{
		mOtherTyping = true;

		// Create typing is started title string
		LLUIString typing_start(mTypingStart);
		typing_start.setArg("[NAME]", im_info->mName);

		// Save and set new title
		mSavedTitle = getTitle();
		setTitle (typing_start);

		// Update speaker
		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
		if ( speaker_mgr )
		{
			speaker_mgr->setSpeakerTyping(im_info->mFromID, TRUE);
		}
	}
}

void LLIMFloater::removeTypingIndicator(const LLIMInfo* im_info)
{
	if ( mOtherTyping )
	{
		mOtherTyping = false;

		// Revert the title to saved one
		setTitle(mSavedTitle);

		if ( im_info )
		{
			// Update speaker
			LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
			if ( speaker_mgr )
			{
				speaker_mgr->setSpeakerTyping(im_info->mFromID, FALSE);
			}
		}

	}
}

// static
bool LLIMFloater::isChatMultiTab()
{
	// Restart is required in order to change chat window type.
	static bool is_single_window = gSavedSettings.getS32("ChatWindow") == 1;
	return is_single_window;
}

// static
void LLIMFloater::initIMFloater()
{
	// This is called on viewer start up
	// init chat window type before user changed it in preferences
	isChatMultiTab();
}

//static
void LLIMFloater::sRemoveTypingIndicator(const LLSD& data)
{
	LLUUID session_id = data["session_id"];
	if (session_id.isNull()) return;

	LLUUID from_id = data["from_id"];
	if (gAgentID == from_id || LLUUID::null == from_id) return;

	LLIMFloater* floater = LLIMFloater::findInstance(session_id);
	if (!floater) return;

	if (IM_NOTHING_SPECIAL != floater->mDialog) return;

	floater->removeTypingIndicator();
}
