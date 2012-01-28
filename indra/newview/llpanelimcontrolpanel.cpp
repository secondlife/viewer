/** 
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llfloaterreg.h"

#include "llpanelimcontrolpanel.h"

#include "llagent.h"
#include "llappviewer.h" // for gDisconnected
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llbutton.h"
#include "llgroupactions.h"
#include "llavatarlist.h"
#include "llparticipantlist.h"
#include "llimview.h"
#include "llvoicechannel.h"
#include "llspeakers.h"
#include "lltrans.h"

void LLPanelChatControlPanel::onCallButtonClicked()
{
	gIMMgr->startCall(mSessionId);
}

void LLPanelChatControlPanel::onEndCallButtonClicked()
{
	gIMMgr->endCall(mSessionId);
}

void LLPanelChatControlPanel::onOpenVoiceControlsClicked()
{
	LLFloaterReg::showInstance("voice_controls");
}

void LLPanelChatControlPanel::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
	{
		return;
	}

	updateCallButton();
}

void LLPanelChatControlPanel::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state)
{
	updateButtons(new_state);
}

void LLPanelChatControlPanel::updateCallButton()
{
	// hide/show call button
	bool voice_enabled = LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();

	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(mSessionId);
	
	if (!session) 
	{
		getChildView("call_btn")->setEnabled(false);
		return;
	}

	bool session_initialized = session->mSessionInitialized;
	bool callback_enabled = session->mCallBackEnabled;

	BOOL enable_connect = session_initialized
		&& voice_enabled
		&& callback_enabled;
	getChildView("call_btn")->setEnabled(enable_connect);
}

void LLPanelChatControlPanel::updateButtons(LLVoiceChannel::EState state)
{
	bool is_call_started = state >= LLVoiceChannel::STATE_CALL_STARTED;
	getChildView("end_call_btn_panel")->setVisible( is_call_started);
	getChildView("voice_ctrls_btn_panel")->setVisible( is_call_started && findChild<LLView>("voice_ctrls_btn_panel"));
	getChildView("call_btn_panel")->setVisible( ! is_call_started);
	
	getChildView("volume_ctrl_panel")->setVisible(state == LLVoiceChannel::STATE_CONNECTED);
	
	updateCallButton();
	
}

LLPanelChatControlPanel::~LLPanelChatControlPanel()
{
	mVoiceChannelStateChangeConnection.disconnect();
	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver(this);
	}
}

BOOL LLPanelChatControlPanel::postBuild()
{
	childSetAction("call_btn", boost::bind(&LLPanelChatControlPanel::onCallButtonClicked, this));
	childSetAction("end_call_btn", boost::bind(&LLPanelChatControlPanel::onEndCallButtonClicked, this));
	childSetAction("voice_ctrls_btn", boost::bind(&LLPanelChatControlPanel::onOpenVoiceControlsClicked, this));

	LLVoiceClient::getInstance()->addObserver(this);

	return TRUE;
}

void LLPanelChatControlPanel::setSessionId(const LLUUID& session_id)
{
	//Method is called twice for AdHoc and Group chat. Second time when server init reply received
	mSessionId = session_id;
	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionId);
	if(voice_channel)
	{
		mVoiceChannelStateChangeConnection = voice_channel->setStateChangedCallback(boost::bind(&LLPanelChatControlPanel::onVoiceChannelStateChanged, this, _1, _2));
		
		//call (either p2p, group or ad-hoc) can be already in started state
		updateButtons(voice_channel->getState());
	}
}

LLPanelIMControlPanel::LLPanelIMControlPanel()
{
}

LLPanelIMControlPanel::~LLPanelIMControlPanel()
{
	LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarID, this);
}

BOOL LLPanelIMControlPanel::postBuild()
{
	childSetAction("view_profile_btn", boost::bind(&LLPanelIMControlPanel::onViewProfileButtonClicked, this));
	childSetAction("add_friend_btn", boost::bind(&LLPanelIMControlPanel::onAddFriendButtonClicked, this));

	childSetAction("share_btn", boost::bind(&LLPanelIMControlPanel::onShareButtonClicked, this));
	childSetAction("teleport_btn", boost::bind(&LLPanelIMControlPanel::onTeleportButtonClicked, this));
	childSetAction("pay_btn", boost::bind(&LLPanelIMControlPanel::onPayButtonClicked, this));

	childSetAction("mute_btn", boost::bind(&LLPanelIMControlPanel::onClickMuteVolume, this));
	childSetAction("block_btn", boost::bind(&LLPanelIMControlPanel::onClickBlock, this));
	childSetAction("unblock_btn", boost::bind(&LLPanelIMControlPanel::onClickUnblock, this));
	
	getChild<LLUICtrl>("volume_slider")->setCommitCallback(boost::bind(&LLPanelIMControlPanel::onVolumeChange, this, _2));

	getChildView("add_friend_btn")->setEnabled(!LLAvatarActions::isFriend(getChild<LLAvatarIconCtrl>("avatar_icon")->getAvatarId()));

	setFocusReceivedCallback(boost::bind(&LLPanelIMControlPanel::onFocusReceived, this));
	
	return LLPanelChatControlPanel::postBuild();
}

void LLPanelIMControlPanel::draw()
{
	bool is_muted = LLMuteList::getInstance()->isMuted(mAvatarID);

	getChild<LLUICtrl>("block_btn_panel")->setVisible(!is_muted);
	getChild<LLUICtrl>("unblock_btn_panel")->setVisible(is_muted);

	if (getChildView("volume_ctrl_panel")->getVisible())
	{

		bool is_muted_voice = LLMuteList::getInstance()->isMuted(mAvatarID, LLMute::flagVoiceChat);

		LLUICtrl* mute_btn = getChild<LLUICtrl>("mute_btn");
		mute_btn->setValue( is_muted_voice );

		LLUICtrl* volume_slider = getChild<LLUICtrl>("volume_slider");
		volume_slider->setEnabled( !is_muted_voice );

		F32 volume;

		if (is_muted_voice)
		{
			// it's clearer to display their volume as zero
			volume = 0.f;
		}
		else
		{
			// actual volume
			volume = LLVoiceClient::getInstance()->getUserVolume(mAvatarID);
		}
		volume_slider->setValue( (F64)volume );
	}

	LLPanelChatControlPanel::draw();
}

void LLPanelIMControlPanel::onClickMuteVolume()
{
	// By convention, we only display and toggle voice mutes, not all mutes
	LLMuteList* mute_list = LLMuteList::getInstance();
	bool is_muted = mute_list->isMuted(mAvatarID, LLMute::flagVoiceChat);

	LLMute mute(mAvatarID, getChild<LLTextBox>("avatar_name")->getText(), LLMute::AGENT);
	if (!is_muted)
	{
		mute_list->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		mute_list->remove(mute, LLMute::flagVoiceChat);
	}
}

void LLPanelIMControlPanel::onClickBlock()
{
	LLMute mute(mAvatarID, getChild<LLTextBox>("avatar_name")->getText(), LLMute::AGENT);
	
	LLMuteList::getInstance()->add(mute);
}

void LLPanelIMControlPanel::onClickUnblock()
{
	LLMute mute(mAvatarID, getChild<LLTextBox>("avatar_name")->getText(), LLMute::AGENT);

	LLMuteList::getInstance()->remove(mute);
}

void LLPanelIMControlPanel::onVolumeChange(const LLSD& data)
{
	F32 volume = (F32)data.asReal();
	LLVoiceClient::getInstance()->setUserVolume(mAvatarID, volume);
}

void LLPanelIMControlPanel::onTeleportButtonClicked()
{
	LLAvatarActions::offerTeleport(mAvatarID);
}
void LLPanelIMControlPanel::onPayButtonClicked()
{
	LLAvatarActions::pay(mAvatarID);
}

void LLPanelIMControlPanel::onViewProfileButtonClicked()
{
	LLAvatarActions::showProfile(mAvatarID);
}

void LLPanelIMControlPanel::onAddFriendButtonClicked()
{
	LLAvatarIconCtrl* avatar_icon = getChild<LLAvatarIconCtrl>("avatar_icon");
	std::string full_name = avatar_icon->getFullName();
	LLAvatarActions::requestFriendshipDialog(mAvatarID, full_name);
}

void LLPanelIMControlPanel::onShareButtonClicked()
{
	LLAvatarActions::share(mAvatarID);
}

void LLPanelIMControlPanel::onFocusReceived()
{
	// Disable all the buttons (Call, Teleport, etc) if disconnected.
	if (gDisconnected)
	{
		setAllChildrenEnabled(FALSE);
	}
}

void LLPanelIMControlPanel::setSessionId(const LLUUID& session_id)
{
	LLPanelChatControlPanel::setSessionId(session_id);

	LLIMModel& im_model = LLIMModel::instance();

	LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarID, this);
	mAvatarID = im_model.getOtherParticipantID(session_id);
	LLAvatarTracker::instance().addParticularFriendObserver(mAvatarID, this);

	// Disable "Add friend" button for friends.
	getChildView("add_friend_btn")->setEnabled(!LLAvatarActions::isFriend(mAvatarID));
	
	// Disable "Teleport" button if friend is offline
	if(LLAvatarActions::isFriend(mAvatarID))
	{
		getChildView("teleport_btn")->setEnabled(LLAvatarTracker::instance().isBuddyOnline(mAvatarID));
	}

	getChild<LLAvatarIconCtrl>("avatar_icon")->setValue(mAvatarID);

	// Disable most profile buttons if the participant is
	// not really an SL avatar (e.g., an Avaline caller).
	LLIMModel::LLIMSession* im_session =
		im_model.findIMSession(session_id);
	if( im_session && !im_session->mOtherParticipantIsAvatar )
	{
		getChildView("view_profile_btn")->setEnabled(FALSE);
		getChildView("add_friend_btn")->setEnabled(FALSE);

		getChildView("share_btn")->setEnabled(FALSE);
		getChildView("teleport_btn")->setEnabled(FALSE);
		getChildView("pay_btn")->setEnabled(FALSE);

        getChild<LLTextBox>("avatar_name")->setValue(im_session->mName);
        getChild<LLTextBox>("avatar_name")->setToolTip(im_session->mName);
	}
	else
	{
		// If the participant is an avatar, fetch the currect name
		gCacheName->get(mAvatarID, false,
			boost::bind(&LLPanelIMControlPanel::onNameCache, this, _1, _2, _3));
	}
}

//virtual
void LLPanelIMControlPanel::changed(U32 mask)
{
	getChildView("add_friend_btn")->setEnabled(!LLAvatarActions::isFriend(mAvatarID));
	
	// Disable "Teleport" button if friend is offline
	if(LLAvatarActions::isFriend(mAvatarID))
	{
		getChildView("teleport_btn")->setEnabled(LLAvatarTracker::instance().isBuddyOnline(mAvatarID));
	}
}

void LLPanelIMControlPanel::onNameCache(const LLUUID& id, const std::string& full_name, bool is_group)
{
	if ( id == mAvatarID )
	{
		std::string avatar_name = full_name;
		getChild<LLTextBox>("avatar_name")->setValue(avatar_name);
		getChild<LLTextBox>("avatar_name")->setToolTip(avatar_name);

		bool is_linden = LLStringUtil::endsWith(full_name, " Linden");
		getChild<LLUICtrl>("mute_btn")->setEnabled( !is_linden);
	}
}

LLPanelGroupControlPanel::LLPanelGroupControlPanel(const LLUUID& session_id):
mParticipantList(NULL)
{
}

BOOL LLPanelGroupControlPanel::postBuild()
{
	childSetAction("group_info_btn", boost::bind(&LLPanelGroupControlPanel::onGroupInfoButtonClicked, this));

	return LLPanelChatControlPanel::postBuild();
}

LLPanelGroupControlPanel::~LLPanelGroupControlPanel()
{
	delete mParticipantList;
	mParticipantList = NULL;
}

// virtual
void LLPanelGroupControlPanel::draw()
{
	// Need to resort the participant list if it's in sort by recent speaker order.
	if (mParticipantList)
		mParticipantList->update();
	LLPanelChatControlPanel::draw();
}

void LLPanelGroupControlPanel::onGroupInfoButtonClicked()
{
	LLGroupActions::show(mGroupID);
}

void LLPanelGroupControlPanel::onSortMenuItemClicked(const LLSD& userdata)
{
	// TODO: Check this code when when sort order menu will be added. (EM)
	if (false && !mParticipantList)
		return;

	std::string chosen_item = userdata.asString();

	if (chosen_item == "sort_name")
	{
		mParticipantList->setSortOrder(LLParticipantList::E_SORT_BY_NAME);
	}

}

void LLPanelGroupControlPanel::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state)
{
	LLPanelChatControlPanel::onVoiceChannelStateChanged(old_state, new_state);
	mParticipantList->setSpeakingIndicatorsVisible(new_state >= LLVoiceChannel::STATE_CALL_STARTED);
}

void LLPanelGroupControlPanel::setSessionId(const LLUUID& session_id)
{
	LLPanelChatControlPanel::setSessionId(session_id);

	mGroupID = session_id;

	// for group and Ad-hoc chat we need to include agent into list 
	if(!mParticipantList)
	{
		LLSpeakerMgr* speaker_manager = LLIMModel::getInstance()->getSpeakerManager(session_id);
		mParticipantList = new LLParticipantList(speaker_manager, getChild<LLAvatarList>("speakers_list"), true,false);
	}
}


LLPanelAdHocControlPanel::LLPanelAdHocControlPanel(const LLUUID& session_id):LLPanelGroupControlPanel(session_id)
{
}

BOOL LLPanelAdHocControlPanel::postBuild()
{
	//We don't need LLPanelGroupControlPanel::postBuild() to be executed as there is no group_info_btn at AdHoc chat
	return LLPanelChatControlPanel::postBuild();
}

