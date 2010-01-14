/** 
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llfloaterreg.h"

#include "llpanelimcontrolpanel.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatariconctrl.h"
#include "llbutton.h"
#include "llgroupactions.h"
#include "llavatarlist.h"
#include "llparticipantlist.h"
#include "llimview.h"
#include "llvoicechannel.h"
#include "llsidetray.h"
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

void LLPanelChatControlPanel::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state)
{
	updateButtons(new_state >= LLVoiceChannel::STATE_CALL_STARTED);
}

void LLPanelChatControlPanel::updateButtons(bool is_call_started)
{
	childSetVisible("end_call_btn_panel", is_call_started);
	childSetVisible("voice_ctrls_btn_panel", is_call_started);
	childSetVisible("call_btn_panel", ! is_call_started);
}

LLPanelChatControlPanel::~LLPanelChatControlPanel()
{
	mVoiceChannelStateChangeConnection.disconnect();
}

BOOL LLPanelChatControlPanel::postBuild()
{
	childSetAction("call_btn", boost::bind(&LLPanelChatControlPanel::onCallButtonClicked, this));
	childSetAction("end_call_btn", boost::bind(&LLPanelChatControlPanel::onEndCallButtonClicked, this));
	childSetAction("voice_ctrls_btn", boost::bind(&LLPanelChatControlPanel::onOpenVoiceControlsClicked, this));

	return TRUE;
}

void LLPanelChatControlPanel::draw()
{
	// hide/show start call and end call buttons
	bool voice_enabled = LLVoiceClient::voiceEnabled();

	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(mSessionId);
	if (!session) return;

	bool session_initialized = session->mSessionInitialized;
	bool callback_enabled = session->mCallBackEnabled;

	BOOL enable_connect = session_initialized
		&& voice_enabled
		&& callback_enabled;
	childSetEnabled("call_btn", enable_connect);

	LLPanel::draw();
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
		updateButtons(voice_channel->getState() >= LLVoiceChannel::STATE_CALL_STARTED);
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
	childSetEnabled("add_friend_btn", !LLAvatarActions::isFriend(getChild<LLAvatarIconCtrl>("avatar_icon")->getAvatarId()));

	
	
	return LLPanelChatControlPanel::postBuild();
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
	std::string full_name = avatar_icon->getFirstName() + " " + avatar_icon->getLastName();
	LLAvatarActions::requestFriendshipDialog(mAvatarID, full_name);
}

void LLPanelIMControlPanel::onShareButtonClicked()
{
	LLAvatarActions::share(mAvatarID);
}

void LLPanelIMControlPanel::setSessionId(const LLUUID& session_id)
{
	LLPanelChatControlPanel::setSessionId(session_id);

	LLIMModel& im_model = LLIMModel::instance();

	LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarID, this);
	mAvatarID = im_model.getOtherParticipantID(session_id);
	LLAvatarTracker::instance().addParticularFriendObserver(mAvatarID, this);

	// Disable "Add friend" button for friends.
	childSetEnabled("add_friend_btn", !LLAvatarActions::isFriend(mAvatarID));
	
	// Disable "Teleport" button if friend is offline
	if(LLAvatarActions::isFriend(mAvatarID))
	{
		childSetEnabled("teleport_btn", LLAvatarTracker::instance().isBuddyOnline(mAvatarID));
	}

	getChild<LLAvatarIconCtrl>("avatar_icon")->setValue(mAvatarID);

	// Disable most profile buttons if the participant is
	// not really an SL avatar (e.g., an Avaline caller).
	LLIMModel::LLIMSession* im_session =
		im_model.findIMSession(session_id);
	if( im_session && !im_session->mOtherParticipantIsAvatar )
	{
		childSetEnabled("view_profile_btn", FALSE);
		childSetEnabled("add_friend_btn", FALSE);

		childSetEnabled("share_btn", FALSE);
		childSetEnabled("teleport_btn", FALSE);
		childSetEnabled("pay_btn", FALSE);

        getChild<LLTextBox>("avatar_name")->setValue(im_session->mName);
        getChild<LLTextBox>("avatar_name")->setToolTip(im_session->mName);
	}
	else
	{
		// If the participant is an avatar, fetch the currect name
		gCacheName->get(mAvatarID, FALSE, boost::bind(&LLPanelIMControlPanel::nameUpdatedCallback, this, _1, _2, _3, _4));
	}
}

//virtual
void LLPanelIMControlPanel::changed(U32 mask)
{
	childSetEnabled("add_friend_btn", !LLAvatarActions::isFriend(mAvatarID));
	
	// Disable "Teleport" button if friend is offline
	if(LLAvatarActions::isFriend(mAvatarID))
	{
		childSetEnabled("teleport_btn", LLAvatarTracker::instance().isBuddyOnline(mAvatarID));
	}
}

void LLPanelIMControlPanel::nameUpdatedCallback(const LLUUID& id, const std::string& first, const std::string& last, BOOL is_group)
{
	if ( id == mAvatarID )
	{
		std::string avatar_name;
		avatar_name.assign(first);
		avatar_name.append(" ");
		avatar_name.append(last);
		getChild<LLTextBox>("avatar_name")->setValue(avatar_name);
		getChild<LLTextBox>("avatar_name")->setToolTip(avatar_name);
	}
}

LLPanelGroupControlPanel::LLPanelGroupControlPanel(const LLUUID& session_id):
mParticipantList(NULL)
{
	mSpeakerManager = LLIMModel::getInstance()->getSpeakerManager(session_id);
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
	//Remove event does not raised until speakerp->mActivityTimer.hasExpired() is false, see LLSpeakerManager::update()
	//so we need update it to raise needed event
	mSpeakerManager->update(true);
	// Need to resort the participant list if it's in sort by recent speaker order.
	if (mParticipantList)
		mParticipantList->updateRecentSpeakersOrder();
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
		mParticipantList = new LLParticipantList(mSpeakerManager, getChild<LLAvatarList>("speakers_list"), true,false);
}


LLPanelAdHocControlPanel::LLPanelAdHocControlPanel(const LLUUID& session_id):LLPanelGroupControlPanel(session_id)
{
}

BOOL LLPanelAdHocControlPanel::postBuild()
{
	//We don't need LLPanelGroupControlPanel::postBuild() to be executed as there is no group_info_btn at AdHoc chat
	return LLPanelChatControlPanel::postBuild();
}

