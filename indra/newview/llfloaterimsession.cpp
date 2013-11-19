/** 
 * @file llfloaterimsession.cpp
 * @brief LLFloaterIMSession class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llfloaterimsession.h"

#include "lldraghandle.h"
#include "llnotificationsutil.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llbutton.h"
#include "llchannelmanager.h"
#include "llchiclet.h"
#include "llchicletbar.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llfloaterreg.h"
#include "llfloateravatarpicker.h"
#include "llfloaterimcontainer.h" // to replace separate IM Floaters with multifloater container
#include "llinventoryfunctions.h"
//#include "lllayoutstack.h"
#include "llchatentry.h"
#include "lllogchat.h"
#include "llscreenchannel.h"
#include "llsyswellwindow.h"
#include "lltrans.h"
#include "llchathistory.h"
#include "llnotifications.h"
#include "llviewerwindow.h"
#include "lltransientfloatermgr.h"
#include "llinventorymodel.h"
#include "llrootview.h"
#include "llspeakers.h"
#include "llviewerchat.h"
#include "llnotificationmanager.h"
#include "llautoreplace.h"

floater_showed_signal_t LLFloaterIMSession::sIMFloaterShowedSignal;

LLFloaterIMSession::LLFloaterIMSession(const LLUUID& session_id)
  : LLFloaterIMSessionTab(session_id),
	mLastMessageIndex(-1),
	mDialog(IM_NOTHING_SPECIAL),
	mTypingStart(),
	mShouldSendTypingState(false),
	mMeTyping(false),
	mOtherTyping(false),
	mSessionNameUpdatedForTyping(false),
	mTypingTimer(),
	mTypingTimeoutTimer(),
	mPositioned(false),
	mSessionInitialized(false)
{
	mIsNearbyChat = false;

	initIMSession(session_id);
		
	setOverlapsScreenChannel(true);

	LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::IM, this);
    mEnableCallbackRegistrar.add("Avatar.EnableGearItem", boost::bind(&LLFloaterIMSession::enableGearMenuItem, this, _2));
    mCommitCallbackRegistrar.add("Avatar.GearDoToSelected", boost::bind(&LLFloaterIMSession::GearDoToSelected, this, _2));
    mEnableCallbackRegistrar.add("Avatar.CheckGearItem", boost::bind(&LLFloaterIMSession::checkGearMenuItem, this, _2));

    setDocked(true);
}


// virtual
void LLFloaterIMSession::refresh()
{
	if (mMeTyping)
{
		// Time out if user hasn't typed for a while.
		if (mTypingTimeoutTimer.getElapsedTimeF32() > LLAgent::TYPING_TIMEOUT_SECS)
		{
	setTyping(false);
		}
	}
}

// virtual
void LLFloaterIMSession::onTearOffClicked()
{
    LLFloaterIMSessionTab::onTearOffClicked();
}

// virtual
void LLFloaterIMSession::onClickCloseBtn(bool)
{
	LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(mSessionID);

	if (session != NULL)
	{
		bool is_call_with_chat = session->isGroupSessionType()
				|| session->isAdHocSessionType() || session->isP2PSessionType();

		LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionID);

		if (is_call_with_chat && voice_channel != NULL
				&& voice_channel->isActive())
		{
			LLSD payload;
			payload["session_id"] = mSessionID;
			LLNotificationsUtil::add("ConfirmLeaveCall", LLSD(), payload, confirmLeaveCallCallback);
			return;
		}
	}
	else
	{
		llwarns << "Empty session with id: " << (mSessionID.asString()) << llendl;
		return;
	}

	LLFloaterIMSessionTab::onClickCloseBtn();
}

/* static */
void LLFloaterIMSession::newIMCallback(const LLSD& data)
{
	if (data["num_unread"].asInteger() > 0 || data["from_id"].asUUID().isNull())
	{
		LLUUID session_id = data["session_id"].asUUID();

		LLFloaterIMSession* floater = LLFloaterReg::findTypedInstance<LLFloaterIMSession>("impanel", session_id);

        // update if visible, otherwise will be updated when opened
		if (floater && floater->isInVisibleChain())
		{
			floater->updateMessages();
		}
	}
}

void LLFloaterIMSession::onVisibilityChange(const LLSD& new_visibility)
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

void LLFloaterIMSession::onSendMsg( LLUICtrl* ctrl, void* userdata )
{
	LLFloaterIMSession* self = (LLFloaterIMSession*) userdata;
	self->sendMsgFromInputEditor();
	self->setTyping(false);
}

bool LLFloaterIMSession::enableGearMenuItem(const LLSD& userdata)
{
    std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
    selected_uuids.push_back(mOtherParticipantUUID);

	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
	return floater_container->enableContextMenuItem(command, selected_uuids);
}

void LLFloaterIMSession::GearDoToSelected(const LLSD& userdata)
{
	std::string command = userdata.asString();
    uuid_vec_t selected_uuids;
    selected_uuids.push_back(mOtherParticipantUUID);

	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
	floater_container->doToParticipants(command, selected_uuids);
}

bool LLFloaterIMSession::checkGearMenuItem(const LLSD& userdata)
{
	std::string command = userdata.asString();
	uuid_vec_t selected_uuids;
	selected_uuids.push_back(mOtherParticipantUUID);

	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();
	return floater_container->checkContextMenuItem(command, selected_uuids);
}

void LLFloaterIMSession::sendMsgFromInputEditor()
{
	if (gAgent.isGodlike()
		|| (mDialog != IM_NOTHING_SPECIAL)
		|| !mOtherParticipantUUID.isNull())
	{
		if (mInputEditor)
		{
			LLWString text = mInputEditor->getWText();
			LLWStringUtil::trim(text);
			LLWStringUtil::replaceChar(text,182,'\n'); // Convert paragraph symbols back into newlines.
			if(!text.empty())
			{
				// Truncate and convert to UTF8 for transport
				std::string utf8_text = wstring_to_utf8str(text);

				sendMsg(utf8_text);

				mInputEditor->setText(LLStringUtil::null);
			}
		}
	}
	else
	{
		llinfos << "Cannot send IM to everyone unless you're a god." << llendl;
	}
}

void LLFloaterIMSession::sendMsg(const std::string& msg)
{
	const std::string utf8_text = utf8str_truncate(msg, MAX_MSG_BUF_SIZE - 1);

	if (mSessionInitialized)
	{
		LLIMModel::sendMessage(utf8_text, mSessionID, mOtherParticipantUUID, mDialog);
	}
	else
	{
		//queue up the message to send once the session is initialized
		mQueuedMsgsForInit.append(utf8_text);
	}

	updateMessages();
}

LLFloaterIMSession::~LLFloaterIMSession()
{
	mVoiceChannelStateChangeConnection.disconnect();
	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver(this);
	}

	LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::IM, this);
}


void LLFloaterIMSession::initIMSession(const LLUUID& session_id)
{
	// Change the floater key to bind it to a new session.
	setKey(session_id);

	mSessionID = session_id;
	mSession = LLIMModel::getInstance()->findIMSession(mSessionID);

	if (mSession)
	{
		mIsP2PChat = mSession->isP2PSessionType();
		mSessionInitialized = mSession->mSessionInitialized;
		mDialog = mSession->mType;
	}
}

void LLFloaterIMSession::initIMFloater()
{
	const LLUUID& other_party_id =
			LLIMModel::getInstance()->getOtherParticipantID(mSessionID);
	if (other_party_id.notNull())
	{
		mOtherParticipantUUID = other_party_id;
	}

	boundVoiceChannel();

	mTypingStart = LLTrans::getString("IM_typing_start_string");

	// Show control panel in torn off floaters only.
	mParticipantListPanel->setVisible(!getHost() && gSavedSettings.getBOOL("IMShowControlPanel"));

	// Disable input editor if session cannot accept text
	if ( mSession && !mSession->mTextIMPossible )
	{
		mInputEditor->setEnabled(FALSE);
		mInputEditor->setLabel(LLTrans::getString("IM_unavailable_text_label"));
	}

	if (!mIsP2PChat)
	{
		std::string session_name(LLIMModel::instance().getName(mSessionID));
		updateSessionName(session_name);
	}
}

//virtual
BOOL LLFloaterIMSession::postBuild()
{
	BOOL result = LLFloaterIMSessionTab::postBuild();

	mInputEditor->setMaxTextLength(1023);
	mInputEditor->setAutoreplaceCallback(boost::bind(&LLAutoReplace::autoreplaceCallback, LLAutoReplace::getInstance(), _1, _2, _3, _4, _5));
	mInputEditor->setFocusReceivedCallback( boost::bind(onInputEditorFocusReceived, _1, this) );
	mInputEditor->setFocusLostCallback( boost::bind(onInputEditorFocusLost, _1, this) );
	mInputEditor->setKeystrokeCallback( boost::bind(onInputEditorKeystroke, _1, this) );
	mInputEditor->setCommitCallback(boost::bind(onSendMsg, _1, this));

	setDocked(true);

	LLButton* add_btn = getChild<LLButton>("add_btn");

	// Allow to add chat participants depending on the session type
	add_btn->setEnabled(isInviteAllowed());
	add_btn->setClickedCallback(boost::bind(&LLFloaterIMSession::onAddButtonClicked, this));

	childSetAction("voice_call_btn", boost::bind(&LLFloaterIMSession::onCallButtonClicked, this));

	LLVoiceClient::getInstance()->addObserver(this);
	
	//*TODO if session is not initialized yet, add some sort of a warning message like "starting session...blablabla"
	//see LLFloaterIMPanel for how it is done (IB)

	initIMFloater();

	return result;
}

void LLFloaterIMSession::onAddButtonClicked()
{
    LLView * button = findChild<LLView>("toolbar_panel")->findChild<LLButton>("add_btn");
    LLFloater* root_floater = gFloaterView->getParentFloater(this);
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLFloaterIMSession::addSessionParticipants, this, _1), TRUE, TRUE, FALSE, root_floater->getName(), button);
	if (!picker)
	{
		return;
	}

	// Need to disable 'ok' button when selected users are already in conversation.
	picker->setOkBtnEnableCb(boost::bind(&LLFloaterIMSession::canAddSelectedToChat, this, _1));
	
	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}
}

bool LLFloaterIMSession::canAddSelectedToChat(const uuid_vec_t& uuids)
{
	if (!mSession
		|| mDialog == IM_SESSION_GROUP_START
		|| mDialog == IM_SESSION_INVITE && gAgent.isInGroup(mSessionID))
	{
		return false;
	}

	if (mIsP2PChat)
	{
		// For a P2P session just check if we are not adding the other participant.

		for (uuid_vec_t::const_iterator id = uuids.begin();
				id != uuids.end(); ++id)
		{
			if (*id == mOtherParticipantUUID)
			{
				return false;
			}
		}
	}
	else
	{
		// For a conference session we need to check against the list from LLSpeakerMgr,
		// because this list may change when participants join or leave the session.

		LLSpeakerMgr::speaker_list_t speaker_list;
		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
		if (speaker_mgr)
		{
			speaker_mgr->getSpeakerList(&speaker_list, true);
		}
	
		for (uuid_vec_t::const_iterator id = uuids.begin();
				id != uuids.end(); ++id)
		{
			for (LLSpeakerMgr::speaker_list_t::const_iterator it = speaker_list.begin();
					it != speaker_list.end(); ++it)
			{
				const LLPointer<LLSpeaker>& speaker = *it;
				if (*id == speaker->mID)
				{
					return false;
				}
			}
		}
	}

	return true;
}

void LLFloaterIMSession::addSessionParticipants(const uuid_vec_t& uuids)
{
	if (mIsP2PChat)
	{
		LLSD payload;
		LLSD args;

		LLNotificationsUtil::add("ConfirmAddingChatParticipants", args, payload,
				boost::bind(&LLFloaterIMSession::addP2PSessionParticipants, this, _1, _2, uuids));
	}
	else
	{
		if(findInstance(mSessionID))
		{
			// remember whom we have invited, to notify others later, when the invited ones actually join
			mInvitedParticipants.insert(mInvitedParticipants.end(), uuids.begin(), uuids.end());
		}
		
		inviteToSession(uuids);
	}
}

void LLFloaterIMSession::addP2PSessionParticipants(const LLSD& notification, const LLSD& response, const uuid_vec_t& uuids)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)
	{
		return;
	}

	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionID);

	// first check whether this is a voice session
	bool is_voice_call = voice_channel != NULL && voice_channel->isActive();

	uuid_vec_t temp_ids;

	// Add the initial participant of a P2P session
	temp_ids.push_back(mOtherParticipantUUID);
	temp_ids.insert(temp_ids.end(), uuids.begin(), uuids.end());

	// then we can close the current session
	if(findInstance(mSessionID))
	{
		onClose(false);

		// remember whom we have invited, to notify others later, when the invited ones actually join
		mInvitedParticipants.insert(mInvitedParticipants.end(), uuids.begin(), uuids.end());
	}

	// we start a new session so reset the initialization flag
	mSessionInitialized = false;



	// Start a new ad hoc voice call if we invite new participants to a P2P call,
	// or start a text chat otherwise.
	if (is_voice_call)
	{
		LLAvatarActions::startAdhocCall(temp_ids, mSessionID);
	}
	else
	{
		LLAvatarActions::startConference(temp_ids, mSessionID);
	}
}

void LLFloaterIMSession::sendParticipantsAddedNotification(const uuid_vec_t& uuids)
{
	std::string names_string;
	LLAvatarActions::buildResidentsString(uuids, names_string);
	LLStringUtil::format_map_t args;
	args["[NAME]"] = names_string;

	sendMsg(getString(uuids.size() > 1 ? "multiple_participants_added" : "participant_added", args));
}

void LLFloaterIMSession::boundVoiceChannel()
{
	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionID);
	if(voice_channel)
	{
		mVoiceChannelStateChangeConnection = voice_channel->setStateChangedCallback(
				boost::bind(&LLFloaterIMSession::onVoiceChannelStateChanged, this, _1, _2));

		//call (either p2p, group or ad-hoc) can be already in started state
		bool callIsActive = voice_channel->getState() >= LLVoiceChannel::STATE_CALL_STARTED;
		updateCallBtnState(callIsActive);
	}
}

void LLFloaterIMSession::onCallButtonClicked()
{
	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionID);
	if (voice_channel)
	{
		bool is_call_active = voice_channel->getState() >= LLVoiceChannel::STATE_CALL_STARTED;
	    if (is_call_active)
	    {
		    gIMMgr->endCall(mSessionID);
	    }
	    else
	    {
		    gIMMgr->startCall(mSessionID);
	    }
	}
}

void LLFloaterIMSession::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if(status != STATUS_JOINING && status != STATUS_LEFT_CHANNEL)
	{
		enableDisableCallBtn();
	}
}

void LLFloaterIMSession::onVoiceChannelStateChanged(
		const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state)
{
	bool callIsActive = new_state >= LLVoiceChannel::STATE_CALL_STARTED;
	updateCallBtnState(callIsActive);
}

void LLFloaterIMSession::updateSessionName(const std::string& name)
{
	if (!name.empty())
	{
		LLFloaterIMSessionTab::updateSessionName(name);
		mTypingStart.setArg("[NAME]", name);
		setTitle (mOtherTyping ? mTypingStart.getString() : name);
		mSessionNameUpdatedForTyping = mOtherTyping;
	}
}

//static
LLFloaterIMSession* LLFloaterIMSession::show(const LLUUID& session_id)
{
	closeHiddenIMToasts();

	if (!gIMMgr->hasSession(session_id))
		return NULL;

	// Test the existence of the floater before we try to create it
	bool exist = findInstance(session_id);

	// Get the floater: this will create the instance if it didn't exist
	LLFloaterIMSession* floater = getInstance(session_id);
	if (!floater)
		return NULL;

	LLFloaterIMContainer* floater_container = LLFloaterIMContainer::getInstance();

	// Do not add again existing floaters
	if (!exist)
	{
		//		LLTabContainer::eInsertionPoint i_pt = user_initiated ? LLTabContainer::RIGHT_OF_CURRENT : LLTabContainer::END;
		// TODO: mantipov: use LLTabContainer::RIGHT_OF_CURRENT if it exists
		LLTabContainer::eInsertionPoint i_pt = LLTabContainer::END;
		if (floater_container)
		{
			floater_container->addFloater(floater, TRUE, i_pt);
		}
	}

	floater->openFloater(floater->getKey());

	floater->setVisible(TRUE);

	return floater;
}
//static
LLFloaterIMSession* LLFloaterIMSession::findInstance(const LLUUID& session_id)
{
    LLFloaterIMSession* conversation =
    		LLFloaterReg::findTypedInstance<LLFloaterIMSession>("impanel", session_id);

	return conversation;
}

LLFloaterIMSession* LLFloaterIMSession::getInstance(const LLUUID& session_id)
{
	LLFloaterIMSession* conversation =
				LLFloaterReg::getTypedInstance<LLFloaterIMSession>("impanel", session_id);

	return conversation;
}

void LLFloaterIMSession::onClose(bool app_quitting)
{
	setTyping(false);

	// The source of much argument and design thrashing
	// Should the window hide or the session close when the X is clicked?
	//
	// Last change:
	// EXT-3516 X Button should end IM session, _ button should hide
	gIMMgr->leaveSession(mSessionID);
    // *TODO: Study why we need to restore the floater before we close it.
    // Might be because we want to save some state data in some clean open state.
	LLFloaterIMSessionTab::restoreFloater();
	// Clean up the conversation *after* the session has been ended
	LLFloaterIMSessionTab::onClose(app_quitting);
}

void LLFloaterIMSession::setDocked(bool docked, bool pop_on_undock)
{
	// update notification channel state
	LLNotificationsUI::LLScreenChannel* channel = static_cast<LLNotificationsUI::LLScreenChannel*>
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

void LLFloaterIMSession::setMinimized(BOOL b)
{
	bool wasMinimized = isMinimized();
	LLFloaterIMSessionTab::setMinimized(b);

	//Switching from minimized state to un-minimized state
	if(wasMinimized && !b)
	{
		//When in DND mode, remove stored IM notifications
		//Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
		if(gAgent.isDoNotDisturb())
		{
			LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, mSessionID);
		}
	}
}

void LLFloaterIMSession::setVisible(BOOL visible)
{
	LLNotificationsUI::LLScreenChannel* channel = static_cast<LLNotificationsUI::LLScreenChannel*>
		(LLNotificationsUI::LLChannelManager::getInstance()->
											findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));

	LLFloaterIMSessionTab::setVisible(visible);

	// update notification channel state
	if(channel)
	{
		channel->updateShowToastsState();
		channel->redrawToasts();
	}

	if(!visible)
	{
		LLChicletPanel * chiclet_panelp = LLChicletBar::getInstance()->getChicletPanel();
		if (NULL != chiclet_panelp)
		{
			LLIMChiclet * chicletp = chiclet_panelp->findChiclet<LLIMChiclet>(mSessionID);
			if(NULL != chicletp)
			{
				chicletp->setToggleState(false);
			}
		}
	}

	if (visible && isInVisibleChain())
	{
		sIMFloaterShowedSignal(mSessionID);
        
	}

}

BOOL LLFloaterIMSession::getVisible()
{
	bool visible;

	if(isChatMultiTab())
	{
		LLFloaterIMContainer* im_container =
				LLFloaterIMContainer::getInstance();
		
		// Treat inactive floater as invisible.
		bool is_active = im_container->getActiveFloater() == this;
	
		//torn off floater is always inactive
		if (!is_active && getHost() != im_container)
		{
			visible = LLTransientDockableFloater::getVisible();
		}
		else
		{
		// getVisible() returns TRUE when Tabbed IM window is minimized.
			visible = is_active && !im_container->isMinimized()
						&& im_container->getVisible();
		}
	}
	else
	{
		visible = LLTransientDockableFloater::getVisible();
	}

	return visible;
}

void LLFloaterIMSession::setFocus(BOOL focus)
{
	LLFloaterIMSessionTab::setFocus(focus);

	//When in DND mode, remove stored IM notifications
	//Nearby chat (Null) IMs are not stored while in DND mode, so can ignore removal
	if(focus && gAgent.isDoNotDisturb())
	{
		LLDoNotDisturbNotificationStorage::getInstance()->removeNotification(LLDoNotDisturbNotificationStorage::toastName, mSessionID);
	}
}

//static
bool LLFloaterIMSession::toggle(const LLUUID& session_id)
{
	if(!isChatMultiTab())
	{
		LLFloaterIMSession* floater = LLFloaterReg::findTypedInstance<LLFloaterIMSession>(
				"impanel", session_id);
		if (floater && floater->getVisible() && floater->hasFocus())
		{
			// clicking on chiclet to close floater just hides it to maintain existing
			// scroll/text entry state
			floater->setVisible(false);
			return false;
		}
		else if(floater && (!floater->isDocked() || floater->getVisible() && !floater->hasFocus()))
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

void LLFloaterIMSession::sessionInitReplyReceived(const LLUUID& im_session_id)
{
	mSessionInitialized = true;

	//will be different only for an ad-hoc im session
	if (mSessionID != im_session_id)
	{
		initIMSession(im_session_id);
		buildConversationViewParticipant();
	}

	initIMFloater();
	LLFloaterIMSessionTab::updateGearBtn();
	//*TODO here we should remove "starting session..." warning message if we added it in postBuild() (IB)

	//need to send delayed messages collected while waiting for session initialization
	if (mQueuedMsgsForInit.size())
	{
		LLSD::array_iterator iter;
		for ( iter = mQueuedMsgsForInit.beginArray();
					iter != mQueuedMsgsForInit.endArray(); ++iter)
		{
			LLIMModel::sendMessage(iter->asString(), mSessionID,
				mOtherParticipantUUID, mDialog);
		}

		mQueuedMsgsForInit.clear();
	}
}

void LLFloaterIMSession::updateMessages()
{
	std::list<LLSD> messages;

	// we shouldn't reset unread message counters if IM floater doesn't have focus
    LLIMModel::instance().getMessages(
    		mSessionID, messages, mLastMessageIndex + 1, hasFocus());

	if (messages.size())
	{
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
			bool is_history = msg["is_history"].asBoolean();

			LLChat chat;
			chat.mFromID = from_id;
			chat.mSessionID = mSessionID;
			chat.mFromName = from;
			chat.mTimeStr = time;
			chat.mChatStyle = is_history ? CHAT_STYLE_HISTORY : chat.mChatStyle;

			// process offer notification
			if (msg.has("notification_id"))
			{
				chat.mNotifId = msg["notification_id"].asUUID();
				// if notification exists - embed it
				if (LLNotificationsUtil::find(chat.mNotifId) != NULL)
				{
					// remove embedded notification from channel
					LLNotificationsUI::LLScreenChannel* channel = static_cast<LLNotificationsUI::LLScreenChannel*>
							(LLNotificationsUI::LLChannelManager::getInstance()->
																findChannelByID(LLUUID(gSavedSettings.getString("NotificationChannelUUID"))));
					if (getVisible())
					{
						// toast will be automatically closed since it is not storable toast
						channel->hideToast(chat.mNotifId);
					}
				}
				// if notification doesn't exist - try to use next message which should be log entry
				else
				{
					continue;
				}
			}
			//process text message
			else
			{
				chat.mText = message;
			}
			
			// Add the message to the chat log
			appendMessage(chat);
			mLastMessageIndex = msg["index"].asInteger();

			// if it is a notification - next message is a notification history log, so skip it
			if (chat.mNotifId.notNull() && LLNotificationsUtil::find(chat.mNotifId) != NULL)
			{
				if (++iter == iter_end)
				{
					break;
				}
				else
				{
					mLastMessageIndex++;
				}
			}
		}
	}
}

void LLFloaterIMSession::reloadMessages(bool clean_messages/* = false*/)
{
	if (clean_messages)
	{
		LLIMModel::LLIMSession * sessionp = LLIMModel::instance().findIMSession(mSessionID);

		if (NULL != sessionp)
		{
			sessionp->loadHistory();
		}
	}

	mChatHistory->clear();
	mLastMessageIndex = -1;
	updateMessages();
	mInputEditor->setFont(LLViewerChat::getChatFont());
}

// static
void LLFloaterIMSession::onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata )
{
	LLFloaterIMSession* self= (LLFloaterIMSession*) userdata;

	// Allow enabling the LLFloaterIMSession input editor only if session can accept text
	LLIMModel::LLIMSession* im_session =
		LLIMModel::instance().findIMSession(self->mSessionID);
	//TODO: While disabled lllineeditor can receive focus we need to check if it is enabled (EK)
	if( im_session && im_session->mTextIMPossible && self->mInputEditor->getEnabled())
	{
		//in disconnected state IM input editor should be disabled
		self->mInputEditor->setEnabled(!gDisconnected);
	}
}

// static
void LLFloaterIMSession::onInputEditorFocusLost(LLFocusableElement* caller, void* userdata)
{
	LLFloaterIMSession* self = (LLFloaterIMSession*) userdata;
	self->setTyping(false);
}

// static
void LLFloaterIMSession::onInputEditorKeystroke(LLTextEditor* caller, void* userdata)
{
	LLFloaterIMSession* self = (LLFloaterIMSession*)userdata;
	LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
	if (im_box)
	{
		im_box->flashConversationItemWidget(self->mSessionID,false);
	}
	std::string text = self->mInputEditor->getText();

		// Deleting all text counts as stopping typing.
	self->setTyping(!text.empty());
}

void LLFloaterIMSession::setTyping(bool typing)
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
		// Still typing, send 'start typing' notification or
		// send 'stop typing' notification immediately
		if (!mMeTyping || mTypingTimer.getElapsedTimeF32() > 1.f)
		{
			LLIMModel::instance().sendTypingState(mSessionID,
					mOtherParticipantUUID, mMeTyping);
					mShouldSendTypingState = false;
		}
	}

	if (!mIsNearbyChat)
	{
		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
		if (speaker_mgr)
		{
			speaker_mgr->setSpeakerTyping(gAgent.getID(), FALSE);
		}
	}
}

void LLFloaterIMSession::processIMTyping(const LLIMInfo* im_info, BOOL typing)
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

void LLFloaterIMSession::processAgentListUpdates(const LLSD& body)
{
	uuid_vec_t joined_uuids;

	if (body.isMap() && body.has("agent_updates") && body["agent_updates"].isMap())
	{
		LLSD::map_const_iterator update_it;
		for(update_it = body["agent_updates"].beginMap();
			update_it != body["agent_updates"].endMap();
			++update_it)
		{
			LLUUID agent_id(update_it->first);
			LLSD agent_data = update_it->second;

			if (agent_data.isMap())
			{
				// store the new participants in joined_uuids
				if (agent_data.has("transition") && agent_data["transition"].asString() == "ENTER")
				{
					joined_uuids.push_back(agent_id);
				}

				// process the moderator mutes
				if (agent_id == gAgentID && agent_data.has("info") && agent_data["info"].has("mutes"))
				{
					BOOL moderator_muted_text = agent_data["info"]["mutes"]["text"].asBoolean();
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

	// the vectors need to be sorted for computing the intersection and difference
	std::sort(mInvitedParticipants.begin(), mInvitedParticipants.end());
    std::sort(joined_uuids.begin(), joined_uuids.end());

    uuid_vec_t intersection; // uuids of invited residents who have joined the conversation
	std::set_intersection(mInvitedParticipants.begin(), mInvitedParticipants.end(),
						  joined_uuids.begin(), joined_uuids.end(),
						  std::back_inserter(intersection));

	if (intersection.size() > 0)
	{
		sendParticipantsAddedNotification(intersection);
	}

	// Remove all joined participants from invited array.
	// The difference between the two vectors (the elements in mInvitedParticipants which are not in joined_uuids)
	// is placed at the beginning of mInvitedParticipants, then all other elements are erased.
	mInvitedParticipants.erase(std::set_difference(mInvitedParticipants.begin(), mInvitedParticipants.end(),
												   joined_uuids.begin(), joined_uuids.end(),
												   mInvitedParticipants.begin()),
							   mInvitedParticipants.end());
}

void LLFloaterIMSession::processSessionUpdate(const LLSD& session_update)
{
	// *TODO : verify following code when moderated mode will be implemented
	if ( false && session_update.has("moderated_mode") &&
		 session_update["moderated_mode"].has("voice") )
	{
		BOOL voice_moderated = session_update["moderated_mode"]["voice"];
		const std::string session_label = LLIMModel::instance().getName(mSessionID);

		if (voice_moderated)
		{
			setTitle(session_label + std::string(" ")
							+ LLTrans::getString("IM_moderated_chat_label"));
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

// virtual
void LLFloaterIMSession::draw()
{
	// add people who were added via dropPerson()
	if (!mPendingParticipants.empty())
	{
		addSessionParticipants(mPendingParticipants);
		mPendingParticipants.clear();
	}

	LLFloaterIMSessionTab::draw();
}

// virtual
BOOL LLFloaterIMSession::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									EDragAndDropType cargo_type,
									void* cargo_data,
									EAcceptance* accept,
						   std::string& tooltip_msg)
{
	if (cargo_type == DAD_PERSON)
	{
		if (dropPerson(static_cast<LLUUID*>(cargo_data), drop))
		{
			*accept = ACCEPT_YES_MULTI;
		}
		else
		{
			*accept = ACCEPT_NO;
		}
	}
	else if (mDialog == IM_NOTHING_SPECIAL)
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mOtherParticipantUUID, mSessionID, drop,
				cargo_type, cargo_data, accept);
	}

	return TRUE;
}

bool LLFloaterIMSession::dropPerson(LLUUID* person_id, bool drop)
{
	bool res = person_id && person_id->notNull();
	if(res)
	{
		uuid_vec_t ids;
		ids.push_back(*person_id);

		res = canAddSelectedToChat(ids);
		if(res && drop)
		{
			// these people will be added during the next draw() call
			// (so they can be added all at once)
			mPendingParticipants.push_back(*person_id);
		}
	}

	return res;
}

BOOL LLFloaterIMSession::isInviteAllowed() const
{
	return ( (IM_SESSION_CONFERENCE_START == mDialog)
			 || (IM_SESSION_INVITE == mDialog && !gAgent.isInGroup(mSessionID))
			 || mIsP2PChat);
}

class LLSessionInviteResponder : public LLHTTPClient::Responder
{
public:
	LLSessionInviteResponder(const LLUUID& session_id)
	{
		mSessionID = session_id;
	}

	void errorWithContent(U32 statusNum, const std::string& reason, const LLSD& content)
	{
		llwarns << "Error inviting all agents to session [status:" 
				<< statusNum << "]: " << content << llendl;
		//throw something back to the viewer here?
	}

private:
	LLUUID mSessionID;
};

BOOL LLFloaterIMSession::inviteToSession(const uuid_vec_t& ids)
{
	LLViewerRegion* region = gAgent.getRegion();
	bool is_region_exist = region != NULL;

	if (is_region_exist)
	{
		S32 count = ids.size();

		if( isInviteAllowed() && (count > 0) )
		{
			llinfos << "LLFloaterIMSession::inviteToSession() - inviting participants" << llendl;

			std::string url = region->getCapability("ChatSessionRequest");

			LLSD data;
			data["params"] = LLSD::emptyArray();
			for (int i = 0; i < count; i++)
			{
				data["params"].append(ids[i]);
			}
			data["method"] = "invite";
			data["session-id"] = mSessionID;
			LLHTTPClient::post(url,	data,new LLSessionInviteResponder(mSessionID));
		}
		else
		{
			llinfos << "LLFloaterIMSession::inviteToSession -"
					<< " no need to invite agents for "
					<< mDialog << llendl;
			// successful add, because everyone that needed to get added
			// was added.
		}
	}

	return is_region_exist;
}

void LLFloaterIMSession::addTypingIndicator(const LLIMInfo* im_info)
{
	// We may have lost a "stop-typing" packet, don't add it twice
	if (im_info && !mOtherTyping)
	{
		mOtherTyping = true;

		// Update speaker
		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
		if ( speaker_mgr )
		{
			speaker_mgr->setSpeakerTyping(im_info->mFromID, TRUE);
		}
	}
}

void LLFloaterIMSession::removeTypingIndicator(const LLIMInfo* im_info)
{
	if (mOtherTyping)
	{
		mOtherTyping = false;

		if (im_info)
		{
			// Update speaker
			LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
			if (speaker_mgr)
			{
				speaker_mgr->setSpeakerTyping(im_info->mFromID, FALSE);
			}
		}
	}
}

// static
void LLFloaterIMSession::closeHiddenIMToasts()
{
	class IMToastMatcher: public LLNotificationsUI::LLScreenChannel::Matcher
	{
	public:
		bool matches(const LLNotificationPtr notification) const
		{
			// "notifytoast" type of notifications is reserved for IM notifications
			return "notifytoast" == notification->getType();
		}
	};

	LLNotificationsUI::LLScreenChannel* channel =
			LLNotificationsUI::LLChannelManager::getNotificationScreenChannel();
	if (channel != NULL)
	{
		channel->closeHiddenToasts(IMToastMatcher());
	}
}
// static
void LLFloaterIMSession::confirmLeaveCallCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	const LLSD& payload = notification["payload"];
	LLUUID session_id = payload["session_id"];

	LLFloater* im_floater = findInstance(session_id);
	if (option == 0 && im_floater != NULL)
	{
		im_floater->closeFloater();
	}

	return;
}

// static
void LLFloaterIMSession::sRemoveTypingIndicator(const LLSD& data)
{
	LLUUID session_id = data["session_id"];
	if (session_id.isNull())
		return;

	LLUUID from_id = data["from_id"];
	if (gAgentID == from_id || LLUUID::null == from_id)
		return;

	LLFloaterIMSession* floater = LLFloaterIMSession::findInstance(session_id);
	if (!floater)
		return;

	if (IM_NOTHING_SPECIAL != floater->mDialog)
		return;

	floater->removeTypingIndicator();
}

// static
void LLFloaterIMSession::onIMChicletCreated( const LLUUID& session_id )
{
	LLFloaterIMSession::addToHost(session_id);
}

boost::signals2::connection LLFloaterIMSession::setIMFloaterShowedCallback(const floater_showed_signal_t::slot_type& cb)
{
	return LLFloaterIMSession::sIMFloaterShowedSignal.connect(cb);
}
