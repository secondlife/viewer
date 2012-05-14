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
#include "llspeakers.h"
#include "lltrans.h"

LLPanelChatControlPanel::~LLPanelChatControlPanel()
{
}

BOOL LLPanelChatControlPanel::postBuild()
{
	return TRUE;
}

void LLPanelChatControlPanel::setSessionId(const LLUUID& session_id)
{
	//Method is called twice for AdHoc and Group chat. Second time when server init reply received
	mSessionId = session_id;
}

LLPanelIMControlPanel::LLPanelIMControlPanel()
{
}

LLPanelIMControlPanel::~LLPanelIMControlPanel()
{
//	LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarID, this);
}

BOOL LLPanelIMControlPanel::postBuild()
{
/*	childSetAction("view_profile_btn", boost::bind(&LLPanelIMControlPanel::onViewProfileButtonClicked, this));
	childSetAction("add_friend_btn", boost::bind(&LLPanelIMControlPanel::onAddFriendButtonClicked, this));

	childSetAction("share_btn", boost::bind(&LLPanelIMControlPanel::onShareButtonClicked, this));
	childSetAction("teleport_btn", boost::bind(&LLPanelIMControlPanel::onTeleportButtonClicked, this));
	childSetAction("pay_btn", boost::bind(&LLPanelIMControlPanel::onPayButtonClicked, this));

	childSetAction("block_btn", boost::bind(&LLPanelIMControlPanel::onClickBlock, this));
	childSetAction("unblock_btn", boost::bind(&LLPanelIMControlPanel::onClickUnblock, this));

	getChildView("add_friend_btn")->setEnabled(!LLAvatarActions::isFriend(getChild<LLAvatarIconCtrl>("avatar_icon")->getAvatarId()));

	setFocusReceivedCallback(boost::bind(&LLPanelIMControlPanel::onFocusReceived, this));
*/
	return LLPanelChatControlPanel::postBuild();

}

void LLPanelIMControlPanel::draw()
{
	bool is_muted = LLMuteList::getInstance()->isMuted(mAvatarID);

	getChild<LLUICtrl>("block_btn_panel")->setVisible(!is_muted);
	getChild<LLUICtrl>("unblock_btn_panel")->setVisible(is_muted);


	LLPanelChatControlPanel::draw();
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
