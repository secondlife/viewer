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

void LLPanelChatControlPanel::onCallButtonClicked()
{
	gIMMgr->startCall(mSessionId);
}

void LLPanelChatControlPanel::onEndCallButtonClicked()
{
	gIMMgr->endCall(mSessionId);
}

BOOL LLPanelChatControlPanel::postBuild()
{
	childSetAction("call_btn", boost::bind(&LLPanelChatControlPanel::onCallButtonClicked, this));
	childSetAction("end_call_btn", boost::bind(&LLPanelChatControlPanel::onEndCallButtonClicked, this));

	return TRUE;
}

void LLPanelChatControlPanel::draw()
{
	// hide/show start call and end call buttons
	bool voice_enabled = LLVoiceClient::voiceEnabled();

	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(mSessionId);
	if (!session) return;

	LLVoiceChannel* voice_channel = session->mVoiceChannel;
	if (voice_channel && voice_enabled)
	{
		childSetVisible("end_call_btn", voice_channel->getState() >= LLVoiceChannel::STATE_CALL_STARTED);
		childSetVisible("call_btn", voice_channel->getState() < LLVoiceChannel::STATE_CALL_STARTED);
	}

	bool session_initialized = session->mSessionInitialized;
	bool callback_enabled = session->mCallBackEnabled;
	LLViewerRegion* region = gAgent.getRegion();

	BOOL enable_connect = (region && region->getCapability("ChatSessionRequest") != "")
		&& session_initialized
		&& voice_enabled
		&& callback_enabled;
	childSetEnabled("call_btn", enable_connect);

	LLPanel::draw();
}

LLPanelIMControlPanel::LLPanelIMControlPanel()
{
}

LLPanelIMControlPanel::~LLPanelIMControlPanel()
{
}

BOOL LLPanelIMControlPanel::postBuild()
{
	childSetAction("view_profile_btn", boost::bind(&LLPanelIMControlPanel::onViewProfileButtonClicked, this));
	childSetAction("add_friend_btn", boost::bind(&LLPanelIMControlPanel::onAddFriendButtonClicked, this));

	childSetAction("share_btn", boost::bind(&LLPanelIMControlPanel::onShareButtonClicked, this));
	childSetEnabled("add_friend_btn", !LLAvatarActions::isFriend(getChild<LLAvatarIconCtrl>("avatar_icon")->getAvatarId()));
	
	return LLPanelChatControlPanel::postBuild();
}

void LLPanelIMControlPanel::onViewProfileButtonClicked()
{
	LLAvatarActions::showProfile(getChild<LLAvatarIconCtrl>("avatar_icon")->getAvatarId());
}

void LLPanelIMControlPanel::onAddFriendButtonClicked()
{
	LLAvatarIconCtrl* avatar_icon = getChild<LLAvatarIconCtrl>("avatar_icon");
	std::string full_name = avatar_icon->getFirstName() + " " + avatar_icon->getLastName();
	LLAvatarActions::requestFriendshipDialog(avatar_icon->getAvatarId(), full_name);
}

void LLPanelIMControlPanel::onShareButtonClicked()
{
	// *TODO: Implement
}

void LLPanelIMControlPanel::setSessionId(const LLUUID& session_id)
{
	LLPanelChatControlPanel::setSessionId(session_id);

	LLIMModel& im_model = LLIMModel::instance();

	LLUUID avatar_id = im_model.getOtherParticipantID(session_id);

	// Disable "Add friend" button for friends.
	childSetEnabled("add_friend_btn", !LLAvatarActions::isFriend(avatar_id));

	getChild<LLAvatarIconCtrl>("avatar_icon")->setValue(avatar_id);

	// Disable profile button if participant is not realy SL avatar
	LLIMModel::LLIMSession* im_session =
		im_model.findIMSession(session_id);
	if( im_session && !im_session->mProfileButtonEnabled )
		childSetEnabled("view_profile_btn", FALSE);
}


LLPanelGroupControlPanel::LLPanelGroupControlPanel(const LLUUID& session_id)
{
	mSpeakerManager = LLIMModel::getInstance()->getSpeakerManager(session_id);
}

BOOL LLPanelGroupControlPanel::postBuild()
{
	childSetAction("group_info_btn", boost::bind(&LLPanelGroupControlPanel::onGroupInfoButtonClicked, this));

	mAvatarList = getChild<LLAvatarList>("speakers_list");
	mParticipantList = new LLParticipantList(mSpeakerManager, mAvatarList);

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
	mSpeakerManager->update(true);
	LLPanelChatControlPanel::draw();
}

void LLPanelGroupControlPanel::onGroupInfoButtonClicked()
{
	LLGroupActions::show(mGroupID);
}


void LLPanelGroupControlPanel::setSessionId(const LLUUID& session_id)
{
	LLPanelChatControlPanel::setSessionId(session_id);

	mGroupID = LLIMModel::getInstance()->getOtherParticipantID(session_id);
}
